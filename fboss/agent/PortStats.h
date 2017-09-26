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

#include "fboss/agent/types.h"
#include "common/stats/ThreadCachedServiceData.h"

namespace facebook { namespace fboss {

class SwitchStats;

class PortStats {
 public:
  typedef stats::ThreadCachedServiceData::TLTimeseries TLTimeseries;

  PortStats(PortID portID, std::unique_ptr<TLTimeseries> linkState,
      SwitchStats *switchStats);

  void trappedPkt();
  void pktDropped();
  void pktBogus();
  void pktError();
  void pktUnhandled();
  void pktToHost(uint32_t bytes); // number of packets forward to host

  void arpPkt();
  void arpUnsupported();
  void arpNotMine();
  void arpRequestRx();
  void arpRequestTx();
  void arpReplyRx();
  void arpReplyTx();
  void arpBadOp();

  void ipv6NdpPkt();
  void ipv6NdpBad();

  void ipv4Rx();
  void ipv4TooSmall();
  void ipv4WrongVer();
  void ipv4Nexthop();
  void ipv4Mine();
  void ipv4NoArp();
  void ipv4TtlExceeded();
  void udpTooSmall();

  void ipv6HopExceeded();

  void dhcpV4Pkt();
  void dhcpV4BadPkt();
  void dhcpV4DropPkt();
  void dhcpV6Pkt();
  void dhcpV6BadPkt();
  void dhcpV6DropPkt();

  void linkStateChange();

  void ipv4DstLookupFailure();
  void ipv6DstLookupFailure();

 private:
  // Forbidden copy constructor and assignment operator
  PortStats(PortStats const &) = delete;
  PortStats& operator=(PortStats const &) = delete;

  /*
   * It's useful to store this
   */
  PortID portID_;

  /*
   * Port down count for the specific port
   */
  std::unique_ptr<TLTimeseries> linkStateChange_;

  // Pointer to main SwitchStats object so that we can forward method calls
  // that we do not want to track ourselves.
  SwitchStats *switchStats_;
};

}} // facebook::fboss
