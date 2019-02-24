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

#include "fboss/agent/platforms/wedge/Wedge40Platform.h"

#include <folly/experimental/TestUtil.h>


namespace facebook {
namespace fboss {

class FakeWedge40Platform : public Wedge40Platform {
 public:
   using Wedge40Platform::Wedge40Platform;

  std::string getVolatileStateDir() const override;
  std::string getPersistentStateDir() const override;
  bool isBcmShellSupported() const override {
    return false;
  }

 private:
  // Forbidden copy constructor and assignment operator
  FakeWedge40Platform(FakeWedge40Platform const&) = delete;
  FakeWedge40Platform& operator=(FakeWedge40Platform const&) = delete;

  folly::test::TemporaryDirectory tmpDir_;
};
} // namespace fboss
} // namespace facebook
