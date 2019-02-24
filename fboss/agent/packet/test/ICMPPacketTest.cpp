/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <folly/Conv.h>
#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/Memory.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/logging/xlog.h>
#include <algorithm>
#include <memory>
#include <string>
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/ICMPHdr.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/test/CounterCache.h"

using namespace facebook::fboss;
using std::string;
using std::vector;
using folly::IOBuf;
using folly::io::Appender;
using folly::io::Cursor;
using folly::io::RWPrivateCursor;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;
using std::unique_ptr;

auto emptyBody = [](RWPrivateCursor* /*cursor*/) {};

IPv4Hdr makeIPv4Hdr() {
  const uint8_t version = IPV4_VERSION;
  const uint8_t ihl = 5;
  const uint8_t dscp = 0;
  const uint8_t ecn = 0;
  const uint16_t length = 20;
  const uint16_t id = 0;
  const bool dontFragment = false;
  const bool moreFragments = false;
  const uint16_t fragmentOffset = 0;
  const uint8_t ttl = 1;
  const uint8_t protocol = static_cast<uint8_t>(IP_PROTO::IP_PROTO_ICMP);
  const uint16_t csum = 0;
  IPAddressV4 srcAddr("10.0.0.15");
  IPAddressV4 dstAddr("10.0.0.1");
  IPv4Hdr ipv4(version, ihl, dscp, ecn, length, id, dontFragment, moreFragments,
      fragmentOffset, ttl, protocol, csum, srcAddr, dstAddr);
  ipv4.computeChecksum();
  return ipv4;
}

IPv6Hdr makeIPv6Hdr() {
  IPAddressV6 srcAddr("2620:0:1cfe:face:b00c::3");
  IPAddressV6 dstAddr("2620:0:1cfe:face:b00c::4");
  IPv6Hdr ipv6(srcAddr, dstAddr);

  ipv6.trafficClass = 0xe0;
  ipv6.payloadLength = ICMPHdr::SIZE;
  ipv6.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_ICMP);
  ipv6.hopLimit = 255;

  return ipv6;
}

TEST(ICMPv4Packet, serializeFullPacket) {
  auto ipv4 = makeIPv4Hdr();
  ICMPHdr icmp4(
      static_cast<uint8_t>(ICMPv4Type::ICMPV4_TYPE_DESTINATION_UNREACHABLE),
      static_cast<uint8_t>(
          ICMPv4Code::ICMPV4_CODE_DESTINATION_UNREACHABLE_NET_UNREACHABLE),
      0);
  auto totalLength = icmp4.computeTotalLengthV4(0);
  auto buf = IOBuf(IOBuf::CREATE, totalLength);
  buf.append(totalLength);
  RWPrivateCursor cursor(&buf);
  icmp4.serializeFullPacket(&cursor,
                        MacAddress("33:33:00:00:00:01"),
                        MacAddress("33:33:00:00:00:02"),
                        VlanID(1), ipv4, 0, emptyBody);

  XLOG(DBG1) << "ip csum: " << ipv4.csum << "icmp csum: " << icmp4.csum;

  auto pkt = MockRxPacket::fromHex(
    // Ethernet Header
    "33 33 00 00 00 01" // Destination MAC Address
    "33 33 00 00 00 02" // Source MAC Address
    "81 00 00 01"       // VLAN: 1
    "08 00"             // EtherType: IPv4
    // IPv4 Header
    "4"            // version: 4
    "5"            // IHL: 5
    "00"           // DSCP, ECN
    "00 14"        // length: 20
    "00 00"        // id: 0
    "00 00"        // flags and fragment offset
    "01"           // TTL: 1
    "01"           // protocol: ICMP
    "a5 da"        // ip checksum
    "0a 00 00 0f"  // Source Address: 10.0.0.15
    "0a 00 00 01"  // Destination Address: 10.0.0.1
    // ICMPHdr
    "03 00"           // ICMP type, code
    "fc ff"           // ICMP checksum
  );
  const uint8_t* serializedData = buf.data();
  const uint8_t* expectedData = pkt->buf()->data();
  for(int i = 0; i < totalLength; i++) {
    EXPECT_EQ(serializedData[i], expectedData[i]);
  }
  // parse the data back
  Cursor cursor2(&buf);
  EthHdr ethHdr(cursor2);
  IPv4Hdr ipHdr(cursor2);
  ICMPHdr icmpHdr(cursor2);
  EXPECT_EQ(ethHdr.srcAddr, MacAddress("33:33:00:00:00:02"));
  EXPECT_EQ(ethHdr.dstAddr, MacAddress("33:33:00:00:00:01"));
  EXPECT_EQ(ethHdr.vlanTags[0].vid(), 1);
  EXPECT_EQ(ipHdr, ipv4);
  EXPECT_EQ(icmpHdr, icmp4);
}

TEST(ICMPv6Packet, serializeFullPacket) {
  auto ipv6 = makeIPv6Hdr();
  ICMPHdr icmp6(
      static_cast<uint8_t>(ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION),
      0,
      0);

  auto totalLength = icmp6.computeTotalLengthV6(0);
  auto buf = IOBuf(IOBuf::CREATE, totalLength);
  buf.append(totalLength);
  RWPrivateCursor cursor(&buf);
  icmp6.serializeFullPacket(&cursor,
                            MacAddress("33:33:00:00:00:01"),
                            MacAddress("33:33:00:00:00:02"),
                            VlanID(1), ipv6, 0, emptyBody);
  // check the serialized data
  auto pkt = MockRxPacket::fromHex(
    // Ethernet Header
    "33 33 00 00 00 01" // Destination MAC Address
    "33 33 00 00 00 02" // Source MAC Address
    "81 00 00 01"       // VLAN: 1
    "86 dd"             // EtherType: IPv4
    // IPv6 Header
    "6"      // VERSION: 6
    "e0"     // Traffic Class
    "00000"  // Flow Label
    "00 04"  // Payload Length
    "3a"     // Next Header: IP_PROTO_IPV6_ICMP
    "ff"     // Hop Limit: 255
    // IPv6 Source Address
    "26 20 00 00 1c fe fa ce"
    "b0 0c 00 00 00 00 00 03"
    // IPv6 Destination Address
    "26 20 00 00 1c fe fa ce"
    "b0 0c 00 00 00 00 00 04"
    // ICMPHdr
    "87 00"           // ICMP type, code
    "9c c6"           // ICMP checksum
  );
  const uint8_t* serializedData = buf.data();
  const uint8_t* expectedData = pkt->buf()->data();
  for(int i = 0; i < totalLength; i++) {
    EXPECT_EQ(serializedData[i], expectedData[i]);
  }

  // parse the data back
  Cursor cursor2(&buf);
  EthHdr ethHdr(cursor2);
  IPv6Hdr ipHdr(cursor2);
  ICMPHdr icmpHdr(cursor2);

  EXPECT_EQ(ethHdr.srcAddr, MacAddress("33:33:00:00:00:02"));
  EXPECT_EQ(ethHdr.dstAddr, MacAddress("33:33:00:00:00:01"));
  EXPECT_EQ(ethHdr.vlanTags[0].vid(), 1);
  EXPECT_EQ(ipHdr, ipv6);
  EXPECT_EQ(icmpHdr, icmp6);
}
