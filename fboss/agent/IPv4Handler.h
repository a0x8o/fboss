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

#include <memory>

#include <folly/IPAddressV4.h>
#include <folly/MacAddress.h>
#include "fboss/agent/packet/IPv4Hdr.h"

namespace folly { namespace io {
class Cursor;
}}

namespace facebook { namespace fboss {

class RxPacket;
class SwitchState;
class SwSwitch;
class Vlan;

class IPv4Handler {
 public:
  enum : uint16_t { ETHERTYPE_IPV4 = 0x0800 };

  explicit IPv4Handler(SwSwitch* sw);

  void handlePacket(std::unique_ptr<RxPacket> pkt,
                    folly::MacAddress dst,
                    folly::MacAddress src,
                    folly::io::Cursor cursor);

 private:
  void sendICMPTimeExceeded(VlanID srcVlan,
                            folly::MacAddress dst,
                            folly::MacAddress src,
                            IPv4Hdr& v4Hdr,
                            folly::io::Cursor cursor);

  // Forbidden copy constructor and assignment operator
  IPv4Handler(IPv4Handler const &) = delete;
  IPv4Handler& operator=(IPv4Handler const &) = delete;

  bool resolveMac(SwitchState* state, folly::IPAddressV4 dest);

  SwSwitch* sw_{nullptr};
};

}} // facebook::fboss
