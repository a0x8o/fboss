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

#include <memory>
#include <functional>
#include <gmock/gmock.h>

#include <folly/Function.h>
#include <folly/MacAddress.h>
#include <folly/Range.h>
#include <folly/Optional.h>
#include <folly/synchronization/Baton.h>

#include "fboss/agent/hw/mock/MockHwSwitch.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/StateObserver.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/RouteNextHopEntry.h"

namespace facebook { namespace fboss {

class MockHwSwitch;
class MockPlatform;
class MockTunManager;
class Platform;
class RxPacket;
class SwitchState;
class SwSwitch;
class TxPacket;
class HwTestHandle;

namespace cfg {
class SwitchConfig;
}

/*
 * In the non unit test code state passed to apply*Config is the state
 * returned from SwSwitch init, which is always published. However this
 * is not the case for unit test code where we are constructing config
 * by hand and applying it to a (often empty) initial state. Now in a rare
 * case if such a config had only route updates in it, since the previous
 * state was not published, we would apply these route updates inline
 * and now when we compare the old and new route tables to determine
 * if anything changed we would come up empty. The end result in such
 * a case would be to return a null new state falsely signifying no change.
 * To avoid this, provide convenience wrappers for unit test code to call.
 */
std::shared_ptr<SwitchState> publishAndApplyConfig(
    std::shared_ptr<SwitchState>& state,
    const cfg::SwitchConfig* config,
    const Platform* platform,
    const cfg::SwitchConfig* prevCfg=nullptr);

/*
 * Create a SwSwitch for testing purposes, with the specified initial state.
 *
 * This will use a MockHwSwitch for the HwSwitch implementation, with the
 * specified initial state.  The returned SwSwitch will have already been
 * initialized.
 */
std::unique_ptr<HwTestHandle> createTestHandle(
  const std::shared_ptr<SwitchState>& = nullptr,
  const folly::Optional<folly::MacAddress>& = folly::none,
  SwitchFlags flags = DEFAULT);
std::unique_ptr<HwTestHandle> createTestHandle(
    cfg::SwitchConfig* cfg,
    folly::MacAddress mac,
    SwitchFlags flags = DEFAULT);
std::unique_ptr<HwTestHandle> createTestHandle(
    cfg::SwitchConfig* cfg,
    SwitchFlags flags = DEFAULT);


std::unique_ptr<MockPlatform> createMockPlatform();

/*
 * Get the MockHwSwitch from a SwSwitch.
 */
MockHwSwitch* getMockHw(SwSwitch* sw);
MockHwSwitch* getMockHw(std::unique_ptr<SwSwitch>& sw);

/*
 * Get the MockPlatform from a SwSwitch.
 */
MockPlatform* getMockPlatform(SwSwitch* sw);
MockPlatform* getMockPlatform(std::unique_ptr<SwSwitch>& sw);

/*
 * Wait until all pending StateUpdates have been applied. Take a
 * snapshot and return the state at the time all previous updates have
 * been processed.
 */
std::shared_ptr<SwitchState> waitForStateUpdates(SwSwitch* sw);

/*
 * Wait until all pending actions on the background thread
 * have been processed
 */
void waitForBackgroundThread(SwSwitch* sw);

/**
 * check the field value
 */
template<typename ExpectedType, typename ActualType>
void checkField(const ExpectedType& expected, const ActualType& actual,
                folly::StringPiece fieldName) {
  if (expected != actual) {
    throw FbossError("expected ", fieldName, " to be ", expected,
                     ", but found ", actual);
    return;
  }
}

/*
 * Create a SwitchState for testing.
 *
 * Profile A:
 *   - 20 ports, 2 VLANS, 2 interfaces
 *   - Ports 1-10 are in VLAN 1
 *   - Ports 11-20 are in VLAN 55
 *   - Interface 1:
 *     - VLAN 1
 *     - MAC 00:02:00:00:00:01
 *     - IPs:
 *       10.0.0.1/24
 *       192.168.0.1/24
 *       2401:db00:2110:3001::0001/64
 *   - Interface 55:
 *     - VLAN 55
 *     - MAC 00:02:00:00:00:55
 *     - IPs:
 *       10.0.55.1/24
 *       192.168.55.1/24
 *       2401:db00:2110:3055::0001/64
 */
std::shared_ptr<SwitchState> testStateA();
/*
 * Same as testStateA but with all ports
 * enabled and up
 */
std::shared_ptr<SwitchState> testStateAWithPortsUp();

/*
 * Test may want to tweak default configuration
 */
std::shared_ptr<SwitchState> testState(cfg::SwitchConfig config);
/*
 * Bring all ports up for a given input state
 */
std::shared_ptr<SwitchState> bringAllPortsUp(
    const std::shared_ptr<SwitchState>& in);

/*
 * Bring all ports down for a given input state
 */
std::shared_ptr<SwitchState> bringAllPortsDown(
    const std::shared_ptr<SwitchState>& in);

/*
 * The returned configuration object, if applied to a SwitchState with ports
 * 1-20, will yield the same SwitchState as that returned by testStateA().
 */
cfg::SwitchConfig testConfigA();

/*
 * Create a SwitchState for testing.
 *
 * Profile B:
 *   - 10 ports, 1 VLAN, 1 interface
 *   - Ports 1-10 are in VLAN 1
 *   - Interface 1:
 *     - VLAN 1
 *     - MAC 00:02:00:00:00:01
 *     - IPs:
 *       10.0.0.1/24
 *       192.168.0.1/24
 *       2401:db00:2110:3001::0001/64
 */
std::shared_ptr<SwitchState> testStateB();

/*
 * Same as testStateA but with all ports
 * enabled and up
 */
std::shared_ptr<SwitchState> testStateBWithPortsUp();
/*
 * Convenience macro that wraps EXPECT_CALL() on the underlying MockHwSwitch
 */
#define EXPECT_HW_CALL(sw, method) \
  EXPECT_CALL(*getMockHw(sw), method)

#define EXPECT_MANY_HW_CALLS(sw, method) \
  EXPECT_CALL(*getMockHw(sw), method).Times(testing::AtLeast(1))

/*
 * Convenience macro that wraps EXPECT_CALL() on the underlying MockPlatform
 */
#define EXPECT_PLATFORM_CALL(sw, method) \
  EXPECT_CALL(*getMockPlatform(sw), method)

/*
 * Helper functions for comparing buffer-like objects
 */
#define EXPECT_BUF_EQ(a, b) EXPECT_EQ(fbossHexDump(a), fbossHexDump(b))
#define ASSERT_BUF_EQ(a, b) ASSERT_EQ(fbossHexDump(a), fbossHexDump(b))

std::string fbossHexDump(const folly::IOBuf* buf);
std::string fbossHexDump(const folly::IOBuf& buf);
std::string fbossHexDump(folly::ByteRange buf);
std::string fbossHexDump(folly::StringPiece buf);
std::string fbossHexDump(const std::string& buf);

RouteNextHopSet makeNextHops(std::vector<std::string> ipStrs);

RoutePrefixV4 makePrefixV4(std::string str);

RoutePrefixV6 makePrefixV6(std::string str);

std::shared_ptr<Route<folly::IPAddressV4>>
GET_ROUTE_V4(const std::shared_ptr<RouteTableMap>& tables,
             RouterID rid, RoutePrefixV4 prefix);

std::shared_ptr<Route<folly::IPAddressV4>>
GET_ROUTE_V4(const std::shared_ptr<RouteTableMap>& tables,
             RouterID rid, std::string prefixStr);

std::shared_ptr<Route<folly::IPAddressV6>>
GET_ROUTE_V6(const std::shared_ptr<RouteTableMap>& tables,
             RouterID rid, RoutePrefixV6 prefix);

std::shared_ptr<Route<folly::IPAddressV6>>
GET_ROUTE_V6(const std::shared_ptr<RouteTableMap>& tables,
             RouterID rid, std::string prefixStr);

void EXPECT_NO_ROUTE(const std::shared_ptr<RouteTableMap>& tables,
                     RouterID rid, std::string prefixStr);

/*
 * Convenience macro for expecting a packet to be transmitted by the switch
 *
 * The name should be a brief description of the match being performed.
 *
 * The matchFn should be a function that accepts a single "const TxPacket*"
 * argument.  If the packet matches it should return void, and if it doesn't
 * match it should throw an exception describing why the packet did not match.
 *
 * Note that you should not use gtest EXPECT_* macros when checking the
 * packet.  Just because one packet is not a match does not mean that another
 * packet will not arrive and match the filter.  For instance, consider the
 * following code:
 *
 *   EXPECT_PKT(sw, "ARP reply", arpReplyChecker).Times(1);
 *   EXPECT_PKT(sw, "ICMP packet", icmpChecker).Times(1);
 *
 * The code expects to see one ARP reply and one ICMP packet, but it may have
 * to call both filters on a packet to see which one it is.
 */
#define EXPECT_PKT(sw, name, matchFn) \
  EXPECT_HW_CALL(sw, sendPacketSwitchedAsync_( \
                 TxPacketMatcher::createMatcher(name, matchFn)))

#define EXPECT_MANY_PKTS(sw, name, matchFn) \
  EXPECT_MANY_HW_CALLS(sw, sendPacketSwitchedAsync_( \
                 TxPacketMatcher::createMatcher(name, matchFn)))

/**
 * Convenience macro for expecting RxPacket to be transmitted by TunManager.
 * usage:
 *  EXPECT_TUN_PKT(tunMgr, "Unicast Packet", packetChecker).Times(1)
 */
#define EXPECT_TUN_PKT(tun, name, dstIfID, matchFn) \
  EXPECT_CALL( \
    *tun, \
    sendPacketToHost_( \
      RxPacketMatcher::createMatcher(name, dstIfID, matchFn)))

/**
 * Templatized version of Matching function for Tx/Rx packet.
 */
template <typename T>
using MatchFn = std::function<void(const T* pkt)>;
using RxMatchFn = MatchFn<RxPacket>;
using TxMatchFn = MatchFn<TxPacket>;

/*
 * A gmock MatcherInterface for matching TxPacket objects.
 */
using TxPacketPtr = TxPacket*;
class TxPacketMatcher
  : public ::testing::MatcherInterface<TxPacketPtr> {
 public:
  TxPacketMatcher(folly::StringPiece name, TxMatchFn fn);

  static ::testing::Matcher<TxPacket*> createMatcher(
      folly::StringPiece name,
      TxMatchFn&& fn);

#ifndef IS_OSS
  bool MatchAndExplain(
      const TxPacketPtr& pkt,
      ::testing::MatchResultListener* l) const override;
#else
  bool MatchAndExplain(
      TxPacketPtr pkt,
      ::testing::MatchResultListener* l) const override;
#endif

  void DescribeTo(std::ostream* os) const override;
  void DescribeNegationTo(std::ostream* os) const override;

 private:
  const std::string name_;
  const TxMatchFn fn_;
};

/*
 * A gmock MatcherInterface for matching RxPacket objects.
 */
using RxMatchFnArgs = std::tuple<InterfaceID, std::shared_ptr<RxPacket>>;
class RxPacketMatcher : public ::testing::MatcherInterface<RxMatchFnArgs> {
 public:
  RxPacketMatcher(folly::StringPiece name, InterfaceID dstIfID, RxMatchFn fn);

  static ::testing::Matcher<RxMatchFnArgs> createMatcher(
      folly::StringPiece name,
      InterfaceID dstIfID,
      RxMatchFn&& fn);

#ifndef IS_OSS
  bool MatchAndExplain(
      const RxMatchFnArgs& args,
      ::testing::MatchResultListener* l) const override;
#else
  bool MatchAndExplain(
      RxMatchFnArgs args,
      ::testing::MatchResultListener* l) const override;
#endif

  void DescribeTo(std::ostream* os) const override;
  void DescribeNegationTo(std::ostream* os) const override;

 private:
  const std::string name_;
  const InterfaceID dstIfID_;
  const RxMatchFn fn_;
};

using SwitchStatePredicate = folly::Function<bool(const StateDelta&)>;

class WaitForSwitchState : public AutoRegisterStateObserver {
 public:
  explicit WaitForSwitchState(
      SwSwitch* sw,
      SwitchStatePredicate predicate,
      const std::string& name);
  ~WaitForSwitchState();

  void stateUpdated(const StateDelta& delta) override;
  template <class Rep = int64_t, class Period = std::ratio<1>>
  bool wait(
      const std::chrono::duration<Rep, Period>& timeout =
          std::chrono::seconds(5)) {
    std::unique_lock<std::mutex> lock(mtx_);
    while (!done_) {
      if (cv_.wait_for(lock, timeout) == std::cv_status::timeout) {
        XLOG(ERR) << "timed out on " << name_;
        return false;
      }
    }
    done_ = false;
    return true;
  }

 private:
  SwitchStatePredicate predicate_;
  std::mutex mtx_;
  std::condition_variable cv_;
  bool done_{false};
  std::string name_;
};
}} // facebook::fboss
