// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <memory>

namespace facebook {
namespace fboss {

using folly::IPAddressV4;
using folly::IPAddressV6;
using std::shared_ptr;

template <typename IPTYPE>
struct NeighborEntryTestUtil {
  template <typename T = IPTYPE>
  static DeltaValue<NdpEntry> getNeighborEntryDelta(
      const StateDelta& delta,
      typename std::
          enable_if<std::is_same<T, IPAddressV6>::value, IPAddressV6>::type ip,
      VlanID vlan) {
    shared_ptr<NdpEntry> oldEntry = nullptr;
    shared_ptr<NdpEntry> newEntry = nullptr;
    const auto& newVlan = delta.getVlansDelta().getNew()->getNodeIf(vlan);
    const auto& oldVlan = delta.getVlansDelta().getOld()->getNodeIf(vlan);
    if (nullptr != newVlan) {
      newEntry = newVlan->getNdpTable()->getEntryIf(ip);
    }
    if (nullptr != oldVlan) {
      oldEntry = oldVlan->getNdpTable()->getEntryIf(ip);
    }
    return DeltaValue<NdpEntry>(oldEntry, newEntry);
  }

  template <typename T = IPTYPE>
  static DeltaValue<ArpEntry> getNeighborEntryDelta(
      const StateDelta& delta,
      typename std::
          enable_if<std::is_same<T, IPAddressV4>::value, IPAddressV4>::type ip,
      VlanID vlan) {
    shared_ptr<ArpEntry> oldEntry = nullptr;
    shared_ptr<ArpEntry> newEntry = nullptr;
    const auto& newVlan = delta.getVlansDelta().getNew()->getNodeIf(vlan);
    const auto& oldVlan = delta.getVlansDelta().getOld()->getNodeIf(vlan);
    if (nullptr != newVlan) {
      newEntry = newVlan->getArpTable()->getEntryIf(ip);
    }
    if (nullptr != oldVlan) {
      oldEntry = oldVlan->getArpTable()->getEntryIf(ip);
    }
    return DeltaValue<ArpEntry>(oldEntry, newEntry);
  }
};

template <class IPTYPE>
class WaitForNeighborEntryExpiration : public WaitForSwitchState {
 public:
  WaitForNeighborEntryExpiration(
      SwSwitch* sw,
      IPTYPE ipAddress,
      VlanID vlan = VlanID(1))
      : WaitForSwitchState(
            sw,
            [ipAddress, vlan](const StateDelta& delta) {
              const auto& neighborEntryDelta =
                  NeighborEntryTestUtil<IPTYPE>::getNeighborEntryDelta(
                      delta, ipAddress, vlan);
              const auto& oldEntry = neighborEntryDelta.getOld();
              const auto& newEntry = neighborEntryDelta.getNew();
              return (oldEntry != nullptr) && (newEntry == nullptr);
            },
            "WaitForNeighborEntryExpiration@" + ipAddress.str()) {}
  ~WaitForNeighborEntryExpiration() {}
};

template <class IPTYPE>
class WaitForNeighborEntryCreation : public WaitForSwitchState {
 public:
  WaitForNeighborEntryCreation(
      SwSwitch* sw,
      IPTYPE ipAddress,
      VlanID vlan = VlanID(1))
      : WaitForSwitchState(
            sw,
            [ipAddress, vlan](const StateDelta& delta) {
              const auto& neighborEntryDelta =
                  NeighborEntryTestUtil<IPTYPE>::getNeighborEntryDelta(
                      delta, ipAddress, vlan);
              const auto& oldEntry = neighborEntryDelta.getOld();
              const auto& newEntry = neighborEntryDelta.getNew();
              return (oldEntry == nullptr) && (newEntry != nullptr);
            },
            "WaitForNeighborEntryCreation@" + ipAddress.str()) {}
  ~WaitForNeighborEntryCreation() {}
};

template <class IPTYPE>
class WaitForNeighborEntryPending : public WaitForSwitchState {
 public:
  WaitForNeighborEntryPending(
      SwSwitch* sw,
      IPTYPE ipAddress,
      VlanID vlan = VlanID(1))
      : WaitForSwitchState(
            sw,
            [ipAddress, vlan](const StateDelta& delta) {
              const auto& neighborEntryDelta =
                  NeighborEntryTestUtil<IPTYPE>::getNeighborEntryDelta(
                      delta, ipAddress, vlan);
              const auto& oldEntry = neighborEntryDelta.getOld();
              const auto& newEntry = neighborEntryDelta.getNew();
              return (oldEntry != nullptr) && (newEntry != nullptr) &&
                  (!oldEntry->isPending()) && (newEntry->isPending());
            },
            "WaitForNeighborEntryPending@" + ipAddress.str()) {}
  ~WaitForNeighborEntryPending() {}
};

template <class IPTYPE>
class WaitForNeighborEntryReachable : public WaitForSwitchState {
 public:
  WaitForNeighborEntryReachable(
      SwSwitch* sw,
      IPTYPE ipAddress,
      VlanID vlan = VlanID(1))
      : WaitForSwitchState(
            sw,
            [ipAddress, vlan](const StateDelta& delta) {
              const auto& neighborEntryDelta =
                  NeighborEntryTestUtil<IPTYPE>::getNeighborEntryDelta(
                      delta, ipAddress, vlan);
              const auto& oldEntry = neighborEntryDelta.getOld();
              const auto& newEntry = neighborEntryDelta.getNew();
              return (oldEntry != nullptr) && (newEntry != nullptr) &&
                  (oldEntry->isPending()) && (!newEntry->isPending());
            },
            "WaitForNeighborEntryReachable@" + ipAddress.str()) {}
  ~WaitForNeighborEntryReachable() {}
};

typedef WaitForNeighborEntryExpiration<IPAddressV4> WaitForArpEntryExpiration;
typedef WaitForNeighborEntryCreation<IPAddressV4> WaitForArpEntryCreation;
typedef WaitForNeighborEntryPending<IPAddressV4> WaitForArpEntryPending;
typedef WaitForNeighborEntryReachable<IPAddressV4> WaitForArpEntryReachable;

typedef WaitForNeighborEntryExpiration<IPAddressV6> WaitForNdpEntryExpiration;
typedef WaitForNeighborEntryCreation<IPAddressV6> WaitForNdpEntryCreation;
typedef WaitForNeighborEntryPending<IPAddressV6> WaitForNdpEntryPending;
typedef WaitForNeighborEntryReachable<IPAddressV6> WaitForNdpEntryReachable;

} // namespace fboss
} // namespace facebook
