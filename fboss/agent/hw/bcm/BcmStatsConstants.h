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

namespace facebook {
namespace fboss {

inline folly::StringPiece constexpr kCapacity() {
  return "capacity";
}

inline folly::StringPiece constexpr kInBytes() {
  return "in_bytes";
}

inline folly::StringPiece constexpr kInUnicastPkts() {
  return "in_unicast_pkts";
}

inline folly::StringPiece constexpr kInMulticastPkts() {
  return "in_multicast_pkts";
}

inline folly::StringPiece constexpr kInBroadcastPkts() {
  return "in_broadcast_pkts";
}

inline folly::StringPiece constexpr kInDiscards() {
  return "in_discards";
}

inline folly::StringPiece constexpr kInErrors() {
  return "in_errors";
}

inline folly::StringPiece constexpr kInPause() {
  return "in_pause_frames";
}

inline folly::StringPiece constexpr kInIpv4HdrErrors() {
  return "in_ipv4_header_errors";
}

inline folly::StringPiece constexpr kInIpv6HdrErrors() {
  return "in_ipv6_header_errors";
}

inline folly::StringPiece constexpr kInNonPauseDiscards() {
  return "in_non_pause_discards";
}

inline folly::StringPiece constexpr kOutBytes() {
  return "out_bytes";
}

inline folly::StringPiece constexpr kOutUnicastPkts() {
  return "out_unicast_pkts";
}

inline folly::StringPiece constexpr kOutMulticastPkts() {
  return "out_multicast_pkts";
}

inline folly::StringPiece constexpr kOutBroadcastPkts() {
  return "out_broadcast_pkts";
}

inline folly::StringPiece constexpr kOutDiscards() {
  return "out_discards";
}

inline folly::StringPiece constexpr kOutErrors() {
  return "out_errors";
}

inline folly::StringPiece constexpr kOutPause() {
  return "out_pause_frames";
}

inline folly::StringPiece constexpr kOutCongestionDiscards() {
  return "out_congestion_discards";
}

inline folly::StringPiece constexpr kOutCongestionDiscardsBytes() {
  return "out_congestion_discards_bytes";
}

inline folly::StringPiece constexpr kOutEcnCounter() {
  return "out_ecn_counter";
}

} // namespace fboss
} // namespace facebook
