/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/capture/PcapWriter.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

#include <folly/Range.h>
#include <string>
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/TxPacket.h"
#include <boost/container/flat_set.hpp>

namespace facebook { namespace fboss {

class RxPacketFilter {
 public:
  explicit RxPacketFilter(const RxCaptureFilter& rxCaptureFilter) :
  cosQueues_(rxCaptureFilter.get_cosQueues().begin(),
    rxCaptureFilter.get_cosQueues().end()) {
  }
  bool passes(const RxPacket* pkt) const {
    return (cosQueues_.empty() ||
            cosQueues_.find(static_cast<CpuCosQueueId>(pkt->cosQueue()))
              != cosQueues_.end());
  }
 private:
  boost::container::flat_set<CpuCosQueueId> cosQueues_;
};

class PacketFilter {
 public:
   explicit PacketFilter(const CaptureFilter & captureFilter) :
   rxPacketFilter_(captureFilter.get_rxCaptureFilter()) {}

   bool passes(const RxPacket* pkt) {
     return rxPacketFilter_.passes(pkt);
   }
 private:
   RxPacketFilter rxPacketFilter_;
};

/*
 * A packet capture job.
 */
class PktCapture {
 public:
  PktCapture(folly::StringPiece name, uint64_t maxPackets,
             CaptureDirection direction);
  PktCapture(folly::StringPiece name, uint64_t maxPackets,
             CaptureDirection direction, const CaptureFilter& captureFilter);

  const std::string& name() const {
    return name_;
  }

  void start(folly::StringPiece path);
  void stop();

  bool packetReceived(const RxPacket* pkt);
  bool packetSent(const TxPacket* pkt);

  std::string toString(bool withStats = false) const;

 private:
  // Forbidden copy constructor and assignment operator
  PktCapture(PktCapture const &) = delete;
  PktCapture& operator=(PktCapture const &) = delete;

  const std::string name_;

  // Note: the rest of the state in this class is protcted by
  // the PcapWriter's mutex.
  PcapWriter writer_;
  uint64_t maxPackets_{0};
  uint64_t numPacketsReceived_{0};
  uint64_t numPacketsSent_{0};
  CaptureDirection direction_{CaptureDirection::CAPTURE_TX_RX};
  PacketFilter packetFilter_;
};
}} // facebook::fboss
