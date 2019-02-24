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
#include "fboss/lib/usb/GalaxyI2CBus.h"

#include <folly/Range.h>
#include <memory>
#include <map>

namespace facebook { namespace fboss {

class WedgeProductInfo;

class GalaxyPlatform : public WedgeTomahawkPlatform {
 public:
  explicit GalaxyPlatform(std::unique_ptr<WedgeProductInfo> productInfo) :
      WedgeTomahawkPlatform(std::move(productInfo)) {}

  ~GalaxyPlatform() override {}

 private:
  GalaxyPlatform(GalaxyPlatform const &) = delete;
  GalaxyPlatform& operator=(GalaxyPlatform const &) = delete;

  std::unique_ptr<BaseWedgeI2CBus> getI2CBus() override {
    return std::make_unique<GalaxyI2CBus>();
  }

  folly::ByteRange defaultLed0Code() override;
  folly::ByteRange defaultLed1Code() override;
};

}}
