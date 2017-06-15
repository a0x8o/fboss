/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <folly/Memory.h>
#include "fboss/agent/Main.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/sim/SimPlatform.h"

#include <gflags/gflags.h>

using namespace facebook::fboss;
using folly::MacAddress;
using std::make_unique;
using std::unique_ptr;

DEFINE_int32(num_ports, 64,
             "The number of ports in the simulated switch");
DEFINE_string(local_mac, "02:00:00:00:00:01",
              "The local MAC address to use for the switch");

namespace facebook { namespace fboss {

unique_ptr<Platform> initSimPlatform() {
  // Disable the tun interface code by default.
  // We normally don't want the sim switch to create real interfaces
  // on the host.
  gflags::SetCommandLineOptionWithMode(
      "tun_intf", "no", gflags::SET_FLAGS_DEFAULT);

  MacAddress localMac(FLAGS_local_mac);
  return make_unique<SimPlatform>(localMac, FLAGS_num_ports);
}

}}

int main(int argc, char* argv[]) {
  return facebook::fboss::fbossMain(argc, argv, initSimPlatform);
}
