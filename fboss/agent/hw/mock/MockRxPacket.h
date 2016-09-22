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

#include <folly/Range.h>
#include "fboss/agent/RxPacket.h"

namespace folly {
class IOBuf;
}

namespace facebook { namespace fboss {

class MockRxPacket : public RxPacket {
 public:
  static std::unique_ptr<MockRxPacket> fromHex(folly::StringPiece hex);

  explicit MockRxPacket(std::unique_ptr<folly::IOBuf> buf);

  std::unique_ptr<MockRxPacket> clone() const;

  void padToLength(uint32_t size, uint8_t pad = 0);

  void setSrcPort(PortID id) {
    srcPort_ = id;
  }
  void setSrcVlan(VlanID id) {
    srcVlan_ = id;
  }

 private:
  // Forbidden copy constructor and assignment operator
  MockRxPacket(MockRxPacket const &) = delete;
  MockRxPacket& operator=(MockRxPacket const &) = delete;
};

}} // facebook::fboss
