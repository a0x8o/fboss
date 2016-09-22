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

#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/NeighborTable.h"

namespace facebook { namespace fboss {

class ArpTable : public NeighborTable<folly::IPAddressV4, ArpEntry,
                                      ArpTable> {
 public:
  using NeighborTable::NeighborTable;
};

}} // facebook::fboss
