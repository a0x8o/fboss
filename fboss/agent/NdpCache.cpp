/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/NdpCache.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/types.h"
#include "fboss/agent/packet/ICMPHdr.h"

#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/logging/xlog.h>
#include <netinet/icmp6.h>

namespace facebook { namespace fboss {

NdpCache::NdpCache(SwSwitch* sw, const SwitchState* state,
                   VlanID vlanID, std::string vlanName, InterfaceID intfID)
    : NeighborCache<NdpTable>(sw, vlanID, vlanName, intfID,
                              state->getNdpTimeout(),
                              state->getMaxNeighborProbes(),
                              state->getStaleEntryInterval()) {}

void NdpCache::sentNeighborSolicitation(folly::IPAddressV6 ip) {
  setPendingEntry(ip);
}

void NdpCache::receivedNeighborSolicitationMine(
    folly::IPAddressV6 ip,
    folly::MacAddress mac,
    PortDescriptor port,
    ICMPv6Type type) {
  CHECK_EQ(type, ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION);

  auto fields = cloneEntryFields(ip);
  bool fieldsMatch = fields && fields->mac == mac && fields->port == port;

  // if we receive a neighbor solicitation that has different
  // fields, replace the entry with a new STALE entry with the
  // new fields
  if (!fields || !fieldsMatch) {
    setEntry(ip, mac, port, NeighborEntryState::STALE);
  }
}

void NdpCache::receivedNeighborAdvertisementMine(
    folly::IPAddressV6 ip,
    folly::MacAddress mac,
    PortDescriptor port,
    ICMPv6Type type,
    uint32_t flags) {
  CHECK_EQ(type, ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT);

  bool override = flags & ND_NA_FLAG_OVERRIDE;
  bool solicited = flags & ND_NA_FLAG_SOLICITED;

  auto fields = cloneEntryFields(ip);
  bool fieldsMatch = fields && fields->mac == mac && fields->port == port;
  bool isIncomplete = fields && fields->state == NeighborState::PENDING;

  if (!fields || isIncomplete || override) {
    if (solicited) {
      setEntry(ip, mac, port, NeighborEntryState::REACHABLE);
    } else {
      setEntry(ip, mac, port, NeighborEntryState::STALE);
    }
  } else if (!override) {
    if (solicited) {
      if (fieldsMatch) {
        updateEntryState(ip, NeighborEntryState::REACHABLE);
      } else {
        updateEntryState(ip, NeighborEntryState::STALE);
      }
    }
  }
}

void NdpCache::receivedNdpMine(
    folly::IPAddressV6 ip,
    folly::MacAddress mac,
    PortDescriptor port,
    ICMPv6Type type,
    uint32_t flags) {
  switch (type) {
    case ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT:
      receivedNeighborAdvertisementMine(ip, mac, port, type, flags);
      break;
    case ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION:
      receivedNeighborSolicitationMine(ip, mac, port, type);
      break;
    default:
      throw FbossError("Unexpected NDP packet type ", type);
  }
}

void NdpCache::receivedNdpNotMine(folly::IPAddressV6 /* ip */,
                                  folly::MacAddress /* mac */,
                                  PortDescriptor /* port */,
                                  ICMPv6Type /* type */,
                                  uint32_t /* flags */) {
  // Note that ARP updates the forward entry mapping here if necessary.
  // We could potentially do the same here, although the IPv6 NDP RFC
  // doesn't appear to recommend this--it states that we MUST silently
  // discard the packet if the target IP isn't ours.
}

inline void NdpCache::probeFor(folly::IPAddressV6 ip) const {
  auto vlan = getSw()->getState()->getVlans()->getVlanIf(getVlanID());
  if (!vlan) {
    XLOG(DBG2) << "Vlan " << getVlanID() << " not found. Skip sending probe";
    return;
  }
  IPv6Handler::sendNeighborSolicitation(getSw(), ip, vlan);
}

std::list<NdpEntryThrift> NdpCache::getNdpCacheData() {
  return getCacheData<NdpEntryThrift>();
}

}} // facebook::fboss
