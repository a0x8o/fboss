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

#include <folly/IPAddressV4.h>
#include "fboss/agent/state/NeighborEntry.h"

namespace facebook { namespace fboss {

/*
 * ArpEntry represents an entry in our IPv4 neighbor table.
 *
 * Note that we define ArpEntry as its own class here rather than using a
 * typedef purely to make it easier for other classes to forward declare
 * ArpEntry.
 */
class ArpEntry : public NeighborEntry<folly::IPAddressV4, ArpEntry> {
 public:
  using NeighborEntry::NeighborEntry;
};

}} // facebook::fboss
