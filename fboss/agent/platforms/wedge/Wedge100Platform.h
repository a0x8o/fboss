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

#include "fboss/agent/platforms/wedge/WedgeTomahawkPlatform.h"

#include <folly/Range.h>
#include <memory>

namespace facebook { namespace fboss {

class BcmSwitch;
class Wedge100Port;
class WedgeProductInfo;

class Wedge100Platform : public WedgeTomahawkPlatform {
 public:
  explicit Wedge100Platform(std::unique_ptr<WedgeProductInfo> productInfo) :
      WedgeTomahawkPlatform(std::move(productInfo)) {}

  std::unique_ptr<WedgePortMapping> createPortMapping() override;
  void onHwInitialized(SwSwitch* sw) override;
  void onUnitAttach(int unit) override;

 private:
  Wedge100Platform(Wedge100Platform const &) = delete;
  Wedge100Platform& operator=(Wedge100Platform const &) = delete;

  enum : uint8_t {
    ADDR_SYSCPLD = 0x32,
    LED_MODE_REG = 0x3c,
    TWELVE_BIT_MODE = 0x6,
  };

  std::unique_ptr<BaseWedgeI2CBus> getI2CBus() override;

  folly::ByteRange defaultLed0Code() override;
  folly::ByteRange defaultLed1Code() override;
  void enableLedMode();
  void setPciPreemphasis(int unit) const;
};

}} // namespace facebook::fboss
