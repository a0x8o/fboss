/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/SwitchStats.h"

#include "fboss/agent/PortStats.h"
#include "common/stats/ExportedStatMapImpl.h"
#include <folly/Memory.h>

using facebook::stats::SUM;
using facebook::stats::RATE;

namespace facebook { namespace fboss {

// set to empty string, we'll prepend prefix when fbagent collects counters
std::string SwitchStats::kCounterPrefix = "";

SwitchStats::SwitchStats()
    : SwitchStats(stats::ThreadCachedServiceData::get()->getThreadStats()) {
}

SwitchStats::SwitchStats(ThreadLocalStatsMap *map)
    : trapPkts_(map, kCounterPrefix + "trapped.pkts", SUM, RATE),
      trapPktDrops_(map, kCounterPrefix + "trapped.drops", SUM, RATE),
      trapPktBogus_(map, kCounterPrefix + "trapped.bogus", SUM, RATE),
      trapPktErrors_(map, kCounterPrefix + "trapped.error", SUM, RATE),
      trapPktUnhandled_(map, kCounterPrefix + "trapped.unhandled", SUM, RATE),
      trapPktToHost_(map, kCounterPrefix + "host.rx", SUM, RATE),
      trapPktToHostBytes_(map, kCounterPrefix + "host.rx.bytes", SUM, RATE),
      pktFromHost_(map, kCounterPrefix + "host.tx", SUM, RATE),
      pktFromHostBytes_(map, kCounterPrefix + "host.tx.bytes", SUM, RATE),
      trapPktArp_(map, kCounterPrefix + "trapped.arp", SUM, RATE),
      arpUnsupported_(map, kCounterPrefix + "arp.unsupported", SUM, RATE),
      arpNotMine_(map, kCounterPrefix + "arp.not_mine", SUM, RATE),
      arpRequestsRx_(map, kCounterPrefix + "arp.request.rx", SUM, RATE),
      arpRepliesRx_(map, kCounterPrefix + "arp.reply.rx", SUM, RATE),
      arpRequestsTx_(map, kCounterPrefix + "arp.request.tx", SUM, RATE),
      arpRepliesTx_(map, kCounterPrefix + "arp.reply.tx", SUM, RATE),
      arpBadOp_(map, kCounterPrefix + "arp.bad_op", SUM, RATE),
      trapPktNdp_(map, kCounterPrefix + "trapped.ndp", SUM, RATE),
      ipv6NdpBad_(map, kCounterPrefix + "ipv6.ndp.bad", SUM, RATE),
      ipv4Rx_(map, kCounterPrefix + "trapped.ipv4", SUM, RATE),
      ipv4TooSmall_(map, kCounterPrefix + "ipv4.too_small", SUM, RATE),
      ipv4WrongVer_(map, kCounterPrefix + "ipv4.wrong_version", SUM, RATE),
      ipv4Nexthop_(map, kCounterPrefix + "ipv4.nexthop", SUM, RATE),
      ipv4Mine_(map, kCounterPrefix + "ipv4.mine", SUM, RATE),
      ipv4NoArp_(map, kCounterPrefix + "ipv4.no_arp", SUM, RATE),
      ipv4TtlExceeded_(map, kCounterPrefix + "ipv4.ttl_exceeded", SUM, RATE),
      ipv6HopExceeded_(map, kCounterPrefix + "ipv6.hop_exceeded", SUM, RATE),
      udpTooSmall_(map, kCounterPrefix + "udp.too_small", SUM, RATE),
      dhcpV4Pkt_(map, kCounterPrefix + "dhcpV4.pkt", SUM, RATE),
      dhcpV4BadPkt_(map, kCounterPrefix + "dhcpV4.bad_pkt", SUM, RATE),
      dhcpV4DropPkt_(map, kCounterPrefix + "dhcpV4.drop_pkt", SUM, RATE),
      dhcpV6Pkt_(map, kCounterPrefix + "dhcpV6.pkt", SUM, RATE),
      dhcpV6BadPkt_(map, kCounterPrefix + "dhcpV6.bad_pkt", SUM, RATE),
      dhcpV6DropPkt_(map, kCounterPrefix + "dhcpV6.drop_pkt", SUM, RATE),
      addRouteV4_(map, kCounterPrefix + "route.v4.add", RATE),
      addRouteV6_(map, kCounterPrefix + "route.v6.add", RATE),
      delRouteV4_(map, kCounterPrefix + "route.v4.delete", RATE),
      delRouteV6_(map, kCounterPrefix + "route.v6.delete", RATE),
      updateState_(map, kCounterPrefix + "state_update.us", 50000, 0, 1000000),
      routeUpdate_(map,  kCounterPrefix + "route_update.us", 50, 0, 500),
      linkStateChange_(map, kCounterPrefix + "link_state.down", SUM) {
}

PortStats* SwitchStats::port(PortID portID) {
  auto it = ports_.find(portID);
  if (it != ports_.end()) {
    return it->second.get();
  }
  return createPortStats(portID);
}

PortStats* SwitchStats::createPortStats(PortID portID) {
  auto rv = ports_.emplace(portID, folly::make_unique<PortStats>(portID, this));
  const auto& it = rv.first;
  DCHECK(rv.second);
  return it->second.get();
}

}} // facebook::fboss