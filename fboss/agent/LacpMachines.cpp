/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/LacpMachines.h"
#include "fboss/agent/LacpController.h"

#include "fboss/agent/LacpTypes-defs.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/state/AggregatePortMap.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/SwitchState.h"

#include <algorithm>
#include <exception>
#include <folly/Conv.h>
#include <folly/ExceptionString.h>

namespace facebook {
namespace fboss {

using folly::ByteRange;

void toAppend(ReceiveMachine::ReceiveState state, std::string* result) {
  std::string stateAsString;

  switch (state) {
    case ReceiveMachine::ReceiveState::CURRENT:
      stateAsString = "current";
      break;
    case ReceiveMachine::ReceiveState::EXPIRED:
      stateAsString = "expired";
      break;
    case ReceiveMachine::ReceiveState::INITIALIZED:
      stateAsString = "initialized";
      break;
    case ReceiveMachine::ReceiveState::DEFAULTED:
      stateAsString = "defaulted";
      break;
    case ReceiveMachine::ReceiveState::DISABLED:
      stateAsString = "disabled";
      break;
  }

  folly::toAppend(stateAsString, result);
}

void toAppend(
    PeriodicTransmissionMachine::PeriodicState state,
    std::string* result) {
  std::string stateAsString;

  switch (state) {
    case PeriodicTransmissionMachine::PeriodicState::NONE:
      stateAsString = "none";
      break;
    case PeriodicTransmissionMachine::PeriodicState::SLOW:
      stateAsString = "slow";
      break;
    case PeriodicTransmissionMachine::PeriodicState::FAST:
      stateAsString = "fast";
      break;
    case PeriodicTransmissionMachine::PeriodicState::TX:
      stateAsString = "tx";
      break;
  }

  folly::toAppend(stateAsString, result);
}

void toAppend(MuxMachine::MuxState state, std::string* result) {
  std::string stateAsString;

  switch (state) {
    case MuxMachine::MuxState::DETACHED:
      stateAsString = "detached";
      break;
    case MuxMachine::MuxState::WAITING:
      stateAsString = "waiting";
      break;
    case MuxMachine::MuxState::ATTACHED:
      stateAsString = "attached";
      break;
    case MuxMachine::MuxState::COLLECTING_DISTRIBUTING:
      stateAsString = "collecting&distributing";
      break;
  }

  folly::toAppend(stateAsString, result);
}

// Needed for CHECK_* macros to work with ReceiveMachine::ReceiveState
std::ostream& operator<<(std::ostream& out, ReceiveMachine::ReceiveState s) {
  std::string stateAsString;

  switch (s) {
    case ReceiveMachine::ReceiveState::CURRENT:
      stateAsString = "current";
      break;
    case ReceiveMachine::ReceiveState::EXPIRED:
      stateAsString = "expired";
      break;
    case ReceiveMachine::ReceiveState::INITIALIZED:
      stateAsString = "initialized";
      break;
    case ReceiveMachine::ReceiveState::DEFAULTED:
      stateAsString = "defaulted";
      break;
    case ReceiveMachine::ReceiveState::DISABLED:
      stateAsString = "disabled";
      break;
  }

  return (out << stateAsString);
}

// Needed for CHECK_* macros to work with MuxMachine::MuxState
std::ostream& operator<<(std::ostream& out, MuxMachine::MuxState s) {
  std::string stateAsString;

  switch (s) {
    case MuxMachine::MuxState::DETACHED:
      stateAsString = "detached";
      break;
    case MuxMachine::MuxState::WAITING:
      stateAsString = "waiting";
      break;
    case MuxMachine::MuxState::ATTACHED:
      stateAsString = "attached";
      break;
    case MuxMachine::MuxState::COLLECTING_DISTRIBUTING:
      stateAsString = "collecting&distributing";
      break;
  }

  return (out << stateAsString);
}

const std::chrono::seconds ReceiveMachine::FAST_EPOCH_DURATION(3);
const std::chrono::seconds ReceiveMachine::SLOW_EPOCH_DURATION(90);

ReceiveMachine::ReceiveMachine(
    LacpController& controller,
    folly::EventBase* evb)
    : folly::AsyncTimeout(evb), controller_(controller) {}

ReceiveMachine::~ReceiveMachine() {}

void ReceiveMachine::start() {
  controller_.evb()->runInEventBaseThread([this] { this->initialize(); });
}

void ReceiveMachine::stop() {
  controller_.evb()->runImmediatelyOrRunInEventBaseThreadAndWait(
      [this] { this->cancelTimeout(); });
}

void ReceiveMachine::rx(LACPDU lacpdu) {
  CHECK(controller_.evb()->inRunningEventBaseThread());

  VLOG(4) << "ReceiveMachine[" << controller_.portID() << "]: "
            << "RX(" << lacpdu.describe() << ")";

  if (state_ == ReceiveState::DISABLED) {
    VLOG(4) << "ReceiveMachine[" << controller_.portID() << "]: "
            << "Ignoring frame reception in DISABLED state";
    return;
  }

  current(lacpdu);
}

void ReceiveMachine::portUp() {
  CHECK(controller_.evb()->inRunningEventBaseThread());
  CHECK_EQ(state_, ReceiveState::DISABLED);

  VLOG(4) << "ReceiveMachine[" << controller_.portID() << "]: UP";

  expired();
}

void ReceiveMachine::portDown() {
  CHECK(controller_.evb()->inRunningEventBaseThread());

  VLOG(4) << "ReceiveMachine[" << controller_.portID() << "]: DOWN";

  disabled();
}

void ReceiveMachine::initialize() {
  // This state is entered on either initialization or port movement. Since
  // port movement is not yet implemented, it can currently only be entered
  // during initialization, when state_ == ReceiveState::INITIALIZED.
  // TODO(samank): remove CHECK once port movement functionality has been added
  CHECK_EQ(state_, ReceiveState::INITIALIZED);

  controller_.unselected();

  recordDefault();

  controller_.setActorState(controller_.actorState() & ~LacpState::EXPIRED);

  disabled(); // UCT
}

void ReceiveMachine::defaulted() {
  updateState(ReceiveState::DEFAULTED);

  updateDefaultSelected();

  recordDefault();

  controller_.setActorState(controller_.actorState() & ~LacpState::EXPIRED);
}

void ReceiveMachine::expired() {
  updateState(ReceiveState::EXPIRED);

  partnerInfo_.state |= LacpState::SHORT_TIMEOUT;

  partnerInfo_.state &= ~LacpState::IN_SYNC;
  controller_.notMatched();

  controller_.setActorState(controller_.actorState() | LacpState::EXPIRED);

  startNextEpoch(FAST_EPOCH_DURATION);
}

void ReceiveMachine::disabled() {
  updateState(ReceiveState::DISABLED);

  endThisEpoch();

  partnerInfo_.state &= ~LacpState::IN_SYNC;
  controller_.notMatched();
}

void ReceiveMachine::updateSelected(LACPDU& lacpdu) {
  if (lacpdu.actorInfo.systemID == partnerInfo_.systemID &&
      lacpdu.actorInfo.systemPriority == partnerInfo_.systemPriority &&
      lacpdu.actorInfo.key == partnerInfo_.key &&
      lacpdu.actorInfo.portPriority == partnerInfo_.portPriority &&
      lacpdu.actorInfo.port == partnerInfo_.port &&
      (lacpdu.actorInfo.state & LacpState::AGGREGATABLE) ==
          (partnerInfo_.state & LacpState::AGGREGATABLE)) {
    return;
  }
  VLOG(4) << "ReceiveMachine[" << controller_.portID() << "]: "
            << "Partner claimed " << lacpdu.actorInfo.describe()
            << " but I have " << partnerInfo_.describe();
  controller_.unselected();
}

void ReceiveMachine::updateDefaultSelected() {
  auto adminDefault = ParticipantInfo();

  if (adminDefault != controller_.partnerInfo()) {
    controller_.unselected();
  }
}

bool ReceiveMachine::updateNTT(LACPDU& lacpdu) {
  // TODO(samank): skip over DEFAULTED, EXPIRED, COLLECTING, DISTRIBUTING?
  return lacpdu.partnerInfo != controller_.actorInfo();
}

void ReceiveMachine::current(LACPDU lacpdu) {
  updateState(ReceiveState::CURRENT);

  updateSelected(lacpdu);

  bool ntt = updateNTT(lacpdu);

  bool activityChanged = (lacpdu.actorInfo.state & LacpState::ACTIVE) !=
      (partnerInfo_.state & LacpState::ACTIVE);

  recordPDU(lacpdu);

  controller_.setActorState(controller_.actorState() & ~LacpState::EXPIRED);

  controller_.select();

  if (ntt) {
    controller_.ntt();
  }

  if (activityChanged) {
    controller_.startPeriodicTransmissionMachine();
  }

  startNextEpoch(epochDuration());
}

std::chrono::seconds ReceiveMachine::epochDuration() {
  return controller_.actorInfo().state & LacpState::SHORT_TIMEOUT
      ? FAST_EPOCH_DURATION
      : SLOW_EPOCH_DURATION;
}

void ReceiveMachine::recordPDU(LACPDU& lacpdu) {
  partnerInfo_ = lacpdu.actorInfo;
  // TODO(samank): needs to be moved after
  controller_.setActorState(controller_.actorState() & ~LacpState::DEFAULTED);

  if (((lacpdu.partnerInfo == controller_.actorInfo()) ||
       ((lacpdu.actorInfo.state & LacpState::AGGREGATABLE) == 0)) &&
      (lacpdu.actorInfo.state & LacpState::IN_SYNC)) {
    partnerInfo_.state |= LacpState::IN_SYNC;
    controller_.matched();
  } else {
    partnerInfo_.state &= ~LacpState::IN_SYNC;
    controller_.notMatched();
  }
}

void ReceiveMachine::startNextEpoch(std::chrono::seconds duration) {
  scheduleTimeout(duration);
}

void ReceiveMachine::endThisEpoch() {
  cancelTimeout();
}

void ReceiveMachine::timeoutExpired() noexcept {
  try {
    switch (state_) {
      case ReceiveState::CURRENT:
        expired();
        break;
      case ReceiveState::EXPIRED:
        defaulted();
        break;
      case ReceiveState::DISABLED:
      case ReceiveState::INITIALIZED:
      case ReceiveState::DEFAULTED:
        throw LACPError("invalid transition to ", state_);
        break;
    }
  } catch (...) {
    std::exception_ptr e = std::current_exception();
    CHECK(e);
    LOG(FATAL) << "ReceiveMachine::timeoutExpired(): "
               << folly::exceptionStr(e);
  }
}

void ReceiveMachine::recordDefault() {
  partnerInfo_ = ParticipantInfo();
  controller_.setActorState(controller_.actorState() | LacpState::DEFAULTED);
}

// TODO(samank): compiler not seeing overlaod of ostream& operator<< in
// in LOG macros but does see it for CHECK macros...
void ReceiveMachine::updateState(ReceiveState nextState) {
  prevState_ = state_;
  state_ = nextState;
  VLOG(4) << "ReceiveMachine[" << controller_.portID() << "]: " << prevState_
            << "-->" << state_;
}

ParticipantInfo ReceiveMachine::partnerInfo() const {
  return partnerInfo_;
}

const std::chrono::seconds PeriodicTransmissionMachine::SHORT_PERIOD(1);
const std::chrono::seconds PeriodicTransmissionMachine::LONG_PERIOD(30);

PeriodicTransmissionMachine::PeriodicTransmissionMachine(
    LacpController& controller,
    folly::EventBase* evb)
    : folly::AsyncTimeout(evb), controller_(controller) {}

PeriodicTransmissionMachine::~PeriodicTransmissionMachine() {}

void PeriodicTransmissionMachine::start() {
  controller_.evb()->runInEventBaseThread([this] {
    state_ = determineTransmissionRate();
    beginNextPeriod();
  });
}

void PeriodicTransmissionMachine::stop() {
  controller_.evb()->runImmediatelyOrRunInEventBaseThreadAndWait(
      [this] { this->cancelTimeout(); });
}

void PeriodicTransmissionMachine::portUp() {
  // TODO(samank): start vs portUp
  CHECK(controller_.evb()->inRunningEventBaseThread());

  state_ = determineTransmissionRate();
  beginNextPeriod();
}

void PeriodicTransmissionMachine::portDown() {
  CHECK(controller_.evb()->inRunningEventBaseThread());

  cancelTimeout();
}

void PeriodicTransmissionMachine::beginNextPeriod() {
  switch (state_) {
    case PeriodicState::SLOW:
      VLOG(4) << "PeriodicTransmissionMachine[" << controller_.portID()
                << "]: scheduling timeout for long period";
      scheduleTimeout(LONG_PERIOD);
      break;
    case PeriodicState::FAST:
      VLOG(4) << "PeriodicTransmissionMachine[" << controller_.portID()
                << "]: scheduling timeout for short period";
      scheduleTimeout(SHORT_PERIOD);
      break;
    case PeriodicState::NONE:
      VLOG(4) << "PeriodicTransmissionMachine[" << controller_.portID()
                << "]: not scheduling a timeout";
      break;
    case PeriodicState::TX:
      throw LACPError("invalid transition to ", state_);
      break;
  }
}

void PeriodicTransmissionMachine::timeoutExpired() noexcept {
  try {
    VLOG(4) << "PeriodicTransmissionMachine[" << controller_.portID()
              << "]: end of period";

    state_ = PeriodicState::TX;

    controller_.ntt();

    state_ = determineTransmissionRate();

    beginNextPeriod();
  } catch (...) {
    std::exception_ptr e = std::current_exception();
    CHECK(e);
    LOG(FATAL) << "PeriodicTranmissionMachine::timeoutExpired(): "
               << folly::exceptionStr(e);
  }
}

PeriodicTransmissionMachine::PeriodicState
PeriodicTransmissionMachine::determineTransmissionRate() {
  auto actorInfo = controller_.actorInfo();
  auto partnerInfo = controller_.partnerInfo();

  if ((actorInfo.state & LacpState::ACTIVE) == 0 &&
      (partnerInfo.state & LacpState::ACTIVE) == 0) {
    return PeriodicState::NONE;
  }

  return controller_.partnerInfo().state & LacpState::SHORT_TIMEOUT
      ? PeriodicState::FAST
      : PeriodicState::SLOW;
}

const std::chrono::seconds TransmitMachine::TX_REPLENISH_RATE(1);
const int TransmitMachine::MAX_TRANSMISSIONS_IN_SHORT_PERIOD = 3;

TransmitMachine::TransmitMachine(
    LacpController& controller,
    folly::EventBase* evb,
    SwSwitch* sw)
    : folly::AsyncTimeout(evb), controller_(controller), sw_(sw) {}

TransmitMachine::~TransmitMachine() {}

void TransmitMachine::start() {
  controller_.evb()->runInEventBaseThread(
      [this] { scheduleTimeout(TransmitMachine::TX_REPLENISH_RATE); });
}

void TransmitMachine::stop() {
  controller_.evb()->runImmediatelyOrRunInEventBaseThreadAndWait(
      [this] { this->cancelTimeout(); });
}

void TransmitMachine::timeoutExpired() noexcept {
  replenishTranmissionsLeft();
}

void TransmitMachine::replenishTranmissionsLeft() noexcept {
  transmissionsLeft_ = std::min(
      transmissionsLeft_ + 1,
      TransmitMachine::MAX_TRANSMISSIONS_IN_SHORT_PERIOD);
  scheduleTimeout(TransmitMachine::TX_REPLENISH_RATE);
}

void TransmitMachine::ntt(LACPDU toTransmit) {
  CHECK(controller_.evb()->inRunningEventBaseThread());

  if (transmissionsLeft_ == 0) {
    // TODO(samank): figure out stale ntt details
    VLOG(4) << "TransmitMachine[" << controller_.portID() << "]: "
              << "skipping ntt request";
    return;
  }

  auto pkt = sw_->allocatePacket(LACPDU::LENGTH);
  if (!pkt) {
    VLOG(4) << "Failed to allocate tx packet for LACPDU transmission";
    return;
  }

  folly::io::RWPrivateCursor writer(pkt->buf());

  folly::MacAddress cpuMac = sw_->getPlatform()->getLocalMac();

  auto port = sw_->getState()->getPorts()->getPortIf(controller_.portID());
  CHECK(port);

  TxPacket::writeEthHeader(
      &writer,
      LACPDU::kSlowProtocolsDstMac(),
      cpuMac,
      port->getIngressVlan(),
      LACPDU::EtherType::SLOW_PROTOCOLS);

  writer.writeBE<uint8_t>(LACPDU::EtherSubtype::LACP);

  toTransmit.to(&writer);

  VLOG(4) << "TransmitMachine[" << controller_.portID() << "]: "
            << "TX(" << toTransmit.describe() << ")";

  sw_->sendPacketOutOfPort(std::move(pkt), controller_.portID());
  --transmissionsLeft_;

  VLOG(4) << transmissionsLeft_ << " transmissions left";
}

// TODO(samank): move into anonymous namespace
class ProgramForwardingState {
 public:
  ProgramForwardingState(
      PortID portID,
      AggregatePortID aggPortID,
      AggregatePort::Forwarding fwdState);
  std::shared_ptr<SwitchState> operator()(
      const std::shared_ptr<SwitchState>& state);

 private:
  PortID portID_;
  AggregatePortID aggegatePortID_;
  AggregatePort::Forwarding forwardingState_;
};

ProgramForwardingState::ProgramForwardingState(
    PortID portID,
    AggregatePortID aggPortID,
    AggregatePort::Forwarding fwdState)
    : portID_(portID), aggegatePortID_(aggPortID), forwardingState_(fwdState) {}

std::shared_ptr<SwitchState> ProgramForwardingState::operator()(
    const std::shared_ptr<SwitchState>& state) {
  std::shared_ptr<SwitchState> nextState(state);
  auto* aggPort =
      nextState->getAggregatePorts()->getAggregatePortIf(aggegatePortID_).get();
  if (!aggPort) {
    return nullptr;
  }

  VLOG(4) << "Updating AggregatePort " << aggPort->getID()
            << ": ForwardingState[" << portID_ << "] --> "
            << (forwardingState_ == AggregatePort::Forwarding::ENABLED
                    ? "ENABLED"
                    : "DISABLED");

  aggPort = aggPort->modify(&nextState);
  aggPort->setForwardingState(portID_, forwardingState_);

  return nextState;
}

const std::chrono::seconds MuxMachine::AGGREGATE_WAIT_DURATION(2);
MuxMachine::MuxMachine(
    LacpController& controller,
    folly::EventBase* evb,
    SwSwitch* sw)
    : AsyncTimeout(evb), controller_(controller), sw_(sw) {}

MuxMachine::~MuxMachine() {}

void MuxMachine::selected(AggregatePortID selection) {
  CHECK(controller_.evb()->inRunningEventBaseThread());

  selection_ = selection;

  switch (state_) {
    case MuxState::DETACHED:
      VLOG(4) << "MuxMachine[" << controller_.portID() << "]: SELECTED in "
                << state_;
      waiting(true);
      break;
    case MuxState::WAITING:
      VLOG(4) << "MuxMachine[" << controller_.portID() << "]: SELECTED in "
                << state_;
      attached();
    case MuxState::ATTACHED:
    case MuxState::COLLECTING_DISTRIBUTING:
      LOG(WARNING) << "MuxMachine[" << controller_.portID()
                   << "]: Ignoring SELECTED in " << state_;
      break;
  }
}

void MuxMachine::unselected() {
  CHECK(controller_.evb()->inRunningEventBaseThread());

  switch (state_) {
    case MuxState::DETACHED:
      LOG(WARNING) << "MuxMachine[" << controller_.portID()
                   << "]: Ignoring UNSELECTED in " << state_;
      break;
    case MuxState::WAITING:
    case MuxState::ATTACHED:
      VLOG(4) << "MuxMachine[" << controller_.portID() << "]: UNSELECTED in "
                << state_;
      detached();
      break;
    case MuxState::COLLECTING_DISTRIBUTING:
      VLOG(4) << "MuxMachine[" << controller_.portID() << "]: UNSELECTED in "
                << state_;
      attached();
      break;
  }
}

void MuxMachine::standby() {
  CHECK(controller_.evb()->inRunningEventBaseThread());

  switch (state_) {
    case MuxState::DETACHED:
      VLOG(4) << "MuxMachine[" << controller_.portID() << "]: STANDBY in "
              << state_;
      waiting(false);
      break;
    case MuxState::WAITING:
      VLOG(4) << "MuxMachine[" << controller_.portID()
              << "]: Ignoring STANDBY in " << state_;
      break;
    case MuxState::ATTACHED:
    case MuxState::COLLECTING_DISTRIBUTING:
      VLOG(4) << "MuxMachine[" << controller_.portID() << "]: STANDBY in "
              << state_;
      detached();
      break;
  }
}

void MuxMachine::matched() {
  CHECK(controller_.evb()->inRunningEventBaseThread());

  matched_ = true;

  switch (state_) {
    case MuxState::DETACHED:
    case MuxState::WAITING:
    case MuxState::COLLECTING_DISTRIBUTING:
      LOG(WARNING) << "MuxMachine[" << controller_.portID()
                   << "]: Ignoring MATCHED in " << state_;
      break;
    case MuxState::ATTACHED:
      VLOG(4) << "MuxMachine[" << controller_.portID() << "]: MATCHED in "
                << state_;
      collectingDistributing();
      break;
  }
}

void MuxMachine::notMatched() {
  CHECK(controller_.evb()->inRunningEventBaseThread());

  matched_ = false;

  switch (state_) {
    case MuxState::DETACHED:
    case MuxState::WAITING:
    case MuxState::ATTACHED:
      LOG(WARNING) << "MuxMachine[" << controller_.portID()
                   << "]: Ignoring NOT MATCHED in " << state_;
      break;
    case MuxState::COLLECTING_DISTRIBUTING:
      VLOG(4) << "MuxMachine[" << controller_.portID() << "]: NOT MATCHED in "
                << state_;
      attached();
      break;
  }
}

void MuxMachine::start() {}

void MuxMachine::stop() {}

void MuxMachine::enableCollectingDistributing() const {
  auto portID = controller_.portID();
  auto aggPortID = selection_;

  /* In testing, we've found that it can take up to 100 ms _after_ a link-up
   * interrupt has been issued for the link to actually be usable (ie. able to
   * transmit and receive frames). Thus, immediately expanding the LAG group
   * to include the newly-up port can result in packet loss.
   * Packet loss of this nature is avoided in the case of dynamic link
   * aggregation by an application of fate-sharing: instead of immediately
   * expanding the LAG group, the LACP machinery is first notified of the event,
   * resulting in a LAC PDU exchange between the local interface and the remote
   * interface. Once the LACP handshake has completed, the LAG group is expanded
   * to include the newly-up port. Since the link must have been able to
   * transmit and receive to complete the handshake, expanding the LAG group
   * afterwards must avoid packet loss. Conversely, if the LACP handshake does
   * not complete, whether due to the link not yet being usable or due to normal
   * LACP operation, then we would not want to add the newly-up port to the LAG
   * group, as dictated by LACP.
   */
  auto enableFwdStateFn = ProgramForwardingState(
      portID, aggPortID, AggregatePort::Forwarding::ENABLED);

  sw_->updateStateNoCoalescing(
      "AggregatePort ForwardingState", std::move(enableFwdStateFn));
}

void MuxMachine::disableCollectingDistributing() const {
  auto portID = controller_.portID();
  auto aggPortID = selection_;

  auto disableFwdStateFn = ProgramForwardingState(
      portID, aggPortID, AggregatePort::Forwarding::DISABLED);

  sw_->updateStateNoCoalescing(
      "AggregatePort ForwardingState", std::move(disableFwdStateFn));
}

void MuxMachine::detached() {
  updateState(MuxState::DETACHED);

  // Detach_Mux_From_Aggregator

  controller_.setActorState(
      controller_.actorState() & ~(LacpState::IN_SYNC | LacpState::COLLECTING));

  disableCollectingDistributing();

  controller_.setActorState(
      controller_.actorState() & ~LacpState::DISTRIBUTING);

  controller_.ntt();
}

void MuxMachine::waiting(bool shouldScheduleTimeout) {
  updateState(MuxState::WAITING);

  if (shouldScheduleTimeout) {
    scheduleTimeout(AGGREGATE_WAIT_DURATION);
  }
}

void MuxMachine::timeoutExpired() noexcept {
  try {
    attached();
    if (matched_) {
      collectingDistributing();
    }
  } catch (...) {
    std::exception_ptr e = std::current_exception();
    CHECK(e);
    LOG(FATAL) << "MuxMachine::timeoutExpired(): " << folly::exceptionStr(e);
  }
}

void MuxMachine::attached() {
  updateState(MuxState::ATTACHED);

  // Attach_Mux_To_Aggregator

  // TODO(samank): this needs to change on port down for min-links
  controller_.setActorState(controller_.actorState() | LacpState::IN_SYNC);

  controller_.setActorState(controller_.actorState() & ~LacpState::COLLECTING);

  disableCollectingDistributing();

  controller_.setActorState(
      controller_.actorState() & ~LacpState::DISTRIBUTING);

  controller_.ntt();
}

void MuxMachine::collectingDistributing() {
  updateState(MuxState::COLLECTING_DISTRIBUTING);

  controller_.setActorState(controller_.actorState() | LacpState::DISTRIBUTING);

  enableCollectingDistributing();

  controller_.setActorState(controller_.actorState() | LacpState::COLLECTING);

  controller_.ntt();
}

void MuxMachine::updateState(MuxState nextState) {
  prevState_ = state_;
  state_ = nextState;

  VLOG(4) << "MuxMachine[" << controller_.portID() << "]: " << prevState_
            << "-->" << state_;
}

Selector::Selector(LacpController& controller, uint8_t minLinkCount)
    : controller_(controller), minLinkCount_(minLinkCount) {}

void Selector::start() {}

void Selector::stop() {}

// TODO(samank): Factor out into MininumLinksSelector
void Selector::select() {
  auto targetLagID = LinkAggregationGroupID::from(
      controller_.actorInfo(), controller_.partnerInfo());

  // If the Link Aggregation Group this port has SELECTED or STANDBYed is the
  // same as that identified by targetLagID, then there's nothing to do.
  auto maybeSelection = getSelectionIf();
  if (maybeSelection && maybeSelection->lagID == targetLagID) {
    VLOG(4) << "Selection[" << controller_.portID()
            << "]: skipping selection logic";
    return;
  }

  auto chooseTargetLag = [this, targetLagID](SelectionState state) {
    Selector::portToSelection().insert(
        std::make_pair(controller_.portID(), Selection(targetLagID, state)));

    if (state == SelectionState::SELECTED) {
      VLOG(4) << "Selection[" << controller_.portID() << "]: selected "
              << targetLagID.describe();
      controller_.selected();
    } else {
      VLOG(4) << "Selection[" << controller_.portID() << "]: standby "
              << targetLagID.describe();
      controller_.standby();
    }
  };

  // If either the actor or the partner has specified it must operate as
  // INDIVIDUAL, we must select the Aggregator that uniquely belongs to it
  if ((controller_.partnerInfo().state & LacpState::AGGREGATABLE) == 0 ||
      (controller_.actorInfo().state & LacpState::AGGREGATABLE) == 0) {
    chooseTargetLag(SelectionState::SELECTED);
    return;
  }

  bool targetLagSelectedByOtherPort = false;
  for (const auto& portAndSelection : portToSelection()) {
    if (portAndSelection.second.lagID == targetLagID) {
      targetLagSelectedByOtherPort = true;
      break;
    }
  }

  bool chosen = false;
  if (targetLagSelectedByOtherPort) {
    chooseTargetLag(SelectionState::STANDBY);
    chosen = true;
  } else {
    // The target Link Aggregation Group can only be chosen if no other port has
    // SELECTED or STANDBYed a LAG with the same AggregatePortID (ie. the other
    // port's LinkAggregationGroupID.actorKey == controller_.aggregatePortID())
    if (isAvailable(controller_.aggregatePortID())) {
      chooseTargetLag(SelectionState::STANDBY);
      chosen = true;
    }
  }

  if (chosen) {
    auto targetLagMemberCount = std::count_if(
        portToSelection().begin(),
        portToSelection().end(),
        [targetLagID](PortIDToSelection::value_type el) {
          Selection s = el.second;
          return s.lagID == targetLagID && s.state == SelectionState::STANDBY;
        });

    if (targetLagMemberCount == minLinkCount_) {
      auto portsToSignal = getPortsWithSelection(
          Selection(targetLagID, SelectionState::STANDBY));
      controller_.selected(
          folly::range(portsToSignal.begin(), portsToSignal.end()));
    }
  }

  // TODO(samank): aggregate with the nil aggregator
}

Selector::Selection Selector::getSelection() {
  auto it = portToSelection().find(controller_.portID());
  if (it == portToSelection().end()) {
    throw LACPError();
  }
  return it->second;
}

folly::Optional<Selector::Selection> Selector::getSelectionIf() {
  auto it = portToSelection().find(controller_.portID());
  if (it == portToSelection().end()) {
    return folly::none;
  }
  return it->second;
}

bool Selector::isAvailable(AggregatePortID aggPortID) const {
  for (const auto& portAndSelection : portToSelection()) {
    Selection s = portAndSelection.second;
    if (static_cast<AggregatePortID>(s.lagID.actorKey) == aggPortID) {
      return false;
    }
  }
  return true;
}

Selector::PortIDToSelection& Selector::portToSelection() {
  static Selector::PortIDToSelection portToSelection_;
  return portToSelection_;
}

std::vector<PortID> Selector::getPortsWithSelection(Selection s) const {
  std::vector<PortID> ports;

  for (const auto& portIDAndSelection : portToSelection()) {
    auto portID = portIDAndSelection.first;
    auto selection = portIDAndSelection.second;
    if (selection == s) {
      ports.push_back(portID);
    }
  }

  return ports;
}

void Selector::portDown() {
  auto maybeSelection = getSelectionIf();
  if (!maybeSelection) {
    VLOG(4) << "Selection[" << controller_.portID() << "]: no LAG chosen";
    return;
  }

  // ReceiveMachine::portDown() was invoked prior to this, which will eventually
  // signal NOT partner.SYNC. If the MuxMachine was in state
  // MuxState::COLLECTING_DISTRIBUTING, then it would have transitioned to
  // MuxMachine::ATTACHED on this signal. Signalling UNSELECTED at this point
  // will transition it to MuxMachine::DETACHED.
  controller_.unselected();
}

void Selector::unselected() {
  auto it = portToSelection().find(controller_.portID());
  if (it == portToSelection().end()) {
    return;
  }

  LinkAggregationGroupID myLagID = it->second.lagID;

  portToSelection().erase(it);

  auto ports =
      getPortsWithSelection(Selection(myLagID, SelectionState::SELECTED));

  controller_.standby(folly::range(ports.begin(), ports.end()));
}

void Selector::selected() {
  auto it = portToSelection().find(controller_.portID());
  CHECK(it != portToSelection().end());
  it->second.state = SelectionState::SELECTED;
}

void Selector::standby() {
  auto it = portToSelection().find(controller_.portID());
  if (it == portToSelection().end()) {
    return;
  }

  it->second.state = SelectionState::STANDBY;
}

} // namespace fboss
} // namespace facebook
