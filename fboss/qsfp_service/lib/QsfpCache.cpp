/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/lib/QsfpCache.h"

#include "fboss/qsfp_service/lib/QsfpClient.h"

#include <chrono>

namespace facebook { namespace fboss {

namespace {
constexpr std::chrono::seconds kLivenessCheckInterval(30);
}

void QsfpCache::init(folly::EventBase* evb, const PortMapThrift& ports) {
  if (!evb) {
    throw std::runtime_error("must pass in non-null evb");
  }

  if (initialized_.load(std::memory_order_acquire)) {
    // already initialized. Should we throw here?
    return;
  }
  evb_ = evb;
  initialized_.store(true, std::memory_order_release);

  portsChanged(ports);

  attachEventBase(evb);
  scheduleTimeout(kLivenessCheckInterval);

}

void QsfpCache::init(folly::EventBase* evb) {
  PortMapThrift initialPorts;
  init(evb, initialPorts);
}

void QsfpCache::portsChanged(const PortMapThrift& ports) {
  if (!initialized_.load(std::memory_order_acquire)) {
    throw std::runtime_error("Cache not yet initialized...");
  }

  ports_.withWLock([this, &ports](auto& lockedPorts) {
    auto gen = this->incrementGen();
    for (const auto& item : ports) {
      lockedPorts[PortID(item.first)] = {item.second, gen};
    }
  });

  folly::via(evb_).then(&QsfpCache::maybeSync, this);
}

void QsfpCache::portChanged(const int32_t id, const PortStatus& port) {
  if (!initialized_.load(std::memory_order_acquire)) {
    throw std::runtime_error("Cache not yet initialized...");
  }

  // TODO: explore using atomic exchanges on shared_ptr to get rid of lock.
  (*ports_.wlock())[PortID(id)] = {port, incrementGen()};
  folly::via(evb_).then(&QsfpCache::maybeSync, this);
}

void QsfpCache::maybeSync() {
  DCHECK(evb_);
  CHECK(evb_->isInEventBaseThread());

  if (activeReq_.hasValue() && !activeReq_->isFulfilled()) {
    // Already an inflight request. This is pretty restrictive, but
    // due to qsfp_service implementation right now there is no reason
    // to support multiple inflight requests.
    //
    // TODO(aeckert): refactor qsfp_service locking to have different
    // locks for cache and bus to be more performant on syncPorts +
    // getTransceiver calls.
    VLOG(3) << "Already an active outstanding request to qsfp_service";
    return;
  }

  PortMapThrift portsToSync;

  {
    auto lockedPorts = ports_.rlock();
    for (const auto& item : *lockedPorts) {
      const auto& port = item.second;
      if (port.generation > remoteGen_) {
        VLOG(3) << "Will sync port " << item.first;
        portsToSync[item.first] = item.second.port;
      }
    }

    if (portsToSync.size() == 0) {
      VLOG(3) << "All " << lockedPorts->size() << " ports up to date";
      return;
    }
  }

  // make sure aliveSince is set before doing anything else
  doSync(std::move(portsToSync));
}

folly::Future<folly::Unit> QsfpCache::confirmAlive() {
  CHECK(evb_->isInEventBaseThread());

  auto getAliveSince = [](auto& client) {
    VLOG(3) << "Polling qsfp_service aliveSince...";
    auto options = QsfpClient::getRpcOptions();
    return client->future_aliveSince(options);
  };
  auto storeIt = [this](int64_t aliveSince) {
    VLOG(3) << "qsfp_service aliveSince=" << aliveSince;
    if (aliveSince > remoteAliveSince_) {
      // restart detected (or starting up). Store remoteAliveSince_ +
      // set remoteGen_ back to 0 since we have no knowledge of state
      // on qsfp_service side
      VLOG(1) << "qsfp_service restarted. aliveSince: "
              << remoteAliveSince_ << " -> " << aliveSince;
      std::tie(remoteAliveSince_,  remoteGen_) = std::make_tuple(aliveSince, 0);
    }
  };

  return QsfpClient::createClient(evb_)
    .then(getAliveSince)
    .then(storeIt);
}

folly::Future<folly::Unit> QsfpCache::doSync(PortMapThrift&& toSync) {
  CHECK(evb_->isInEventBaseThread());

  auto syncPorts = [ports = std::move(toSync)](auto& client) mutable {
    VLOG(1) << "Will try to sync " << ports.size() << " ports to qsfp_service";
    auto options = QsfpClient::getRpcOptions();
    return client->future_syncPorts(options, std::move(ports));
  };

  auto onSuccess = [
    this,
    gen = incrementGen(),
    oldAliveSince = remoteAliveSince_
  ](auto& tcvrs) {
    VLOG(1) << "Got " << tcvrs.size() << " transceivers from qsfp_service";
    this->updateCache(tcvrs);
    if (remoteAliveSince_ == oldAliveSince || oldAliveSince < 0) {
      // no restart occurred in middle of request, store gen
      remoteGen_ = gen;
    }
  };

  // lock out other request attempts
  VLOG(4) << "Starting new request";
  activeReq_ = folly::SharedPromise<folly::Unit>();

  auto baseFut = (remoteAliveSince_ < 0) ? confirmAlive() : folly::makeFuture();
  return baseFut
    .then([evb = evb_]() { return QsfpClient::createClient(evb); })
    .then(evb_, syncPorts)
    .then(evb_, onSuccess)
    .onError([this](const std::exception& e) {
      LOG(ERROR) << "Exception talking to qsfp_service: " << e.what();
      this->maybeSync();
    }).ensure([this]() {
      // make sure we allow other requests in again
      activeReq_->setValue();
      VLOG(4) << "Finished request";
    });
}

void QsfpCache::updateCache(const TcvrMapThrift& tcvrs) {
  tcvrs_.withWLock([&tcvrs](auto& lockedTcvrs) {
    for (const auto& item : tcvrs) {
      lockedTcvrs[TransceiverID(item.first)] = item.second;
    }
  });
}

folly::Optional<TransceiverInfo> QsfpCache::getIf(TransceiverID tcvrId) {
  if (!initialized_.load(std::memory_order_acquire)) {
    throw std::runtime_error("Cache not yet initialized...");
  }

  auto lockedTcvrs = tcvrs_.rlock();
  auto it = lockedTcvrs->find(tcvrId);
  if (it != lockedTcvrs->end()) {
    return it->second;
  }
  return folly::Optional<TransceiverInfo>();
}

TransceiverInfo QsfpCache::get(TransceiverID tcvrId) {
  auto fromCache = getIf(tcvrId);
  if (fromCache) {
    return fromCache.value();
  }
  throw std::runtime_error(
    folly::to<std::string>("Transceiver ", tcvrId, " not in cache"));
}

folly::Future<TransceiverInfo> QsfpCache::futureGet(TransceiverID tcvrId) {
  auto fromCache = getIf(tcvrId);
  if (fromCache) {
    return fromCache.value();
  }

  return via(evb_).then([this, tcvrId]() {
    // if active request, query cache when request is
    // fulfilled. Otherwise query now.
    auto baseFut = (activeReq_) ? activeReq_->getFuture() : folly::makeFuture();
    return baseFut.then([this, tcvrId]() {
      return this->get(tcvrId);
    });
  });
}

void QsfpCache::timeoutExpired() noexcept {
  confirmAlive().then(&QsfpCache::maybeSync, this);
  scheduleTimeout(kLivenessCheckInterval);
}

void QsfpCache::dump() {
  // for now just dumps presence info. We should expand later
  auto lockedTcvrs = tcvrs_.rlock();
  for (const auto& item : *lockedTcvrs) {
    LOG(INFO) << folly::to<std::string>(
      item.first, " -> present=", item.second.present);
  }
}

uint32_t QsfpCache::incrementGen() {
  // can use std::atomic_uint32_t in gcc7
  static std::atomic<std::uint32_t> gen{0};
  return ++gen;
}

}} // facebook::fboss
