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

#include <folly/Conv.h>
#include <iosfwd>

#include <boost/serialization/strong_typedef.hpp>
#include <cstdint>
#include <type_traits>

#include "fboss/agent/gen-cpp2/switch_config_types.h"

#define FBOSS_STRONG_TYPE(primitive, new_type) \
  namespace facebook { namespace fboss { \
  \
  BOOST_STRONG_TYPEDEF(primitive, new_type);      \
  \
  /* Define toAppend() so folly::to<string> will work */ \
  inline void toAppend(new_type value, std::string* result) { \
    folly::toAppend(static_cast<primitive>(value), result); \
  } \
  inline void toAppend(new_type value, folly::fbstring* result) { \
    folly::toAppend(static_cast<primitive>(value), result); \
  } \
  }} /* facebook::fboss */ \
  namespace std { \
  \
  template <> struct hash<facebook::fboss::new_type> {  \
    size_t operator()(const facebook::fboss::new_type & x) const {      \
      return hash<primitive>()(static_cast<primitive>(x));      \
    } \
  }; \
  }


FBOSS_STRONG_TYPE(uint8_t, ChannelID)
FBOSS_STRONG_TYPE(uint16_t, TransceiverID)
FBOSS_STRONG_TYPE(uint16_t, AggregatePortID)
FBOSS_STRONG_TYPE(uint16_t, PortID)
FBOSS_STRONG_TYPE(uint16_t, VlanID)
FBOSS_STRONG_TYPE(uint32_t, RouterID)
FBOSS_STRONG_TYPE(uint32_t, InterfaceID)
FBOSS_STRONG_TYPE(int, VrfID)
FBOSS_STRONG_TYPE(uint32_t, ClientID)

/*
 * A unique ID identifying a node in our state tree.
 */
FBOSS_STRONG_TYPE(uint64_t, NodeID)

namespace facebook {
namespace fboss {

using LoadBalancerID = cfg::LoadBalancerID;

} // namespace fboss
} // namespace facebook

namespace facebook {
namespace fboss {
namespace cfg {

std::ostream& operator<<(std::ostream& out, LoadBalancerID id);

} // namespace cfg
} // namespace fboss
} // namespace facebook
