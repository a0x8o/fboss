/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "FakeSai.h"
#include "FakeSaiNeighbor.h"

#include "fboss/agent/hw/sai/api/AddressUtil.h"

#include <folly/logging/xlog.h>
#include <folly/Optional.h>

using facebook::fboss::FakeSai;
using facebook::fboss::FakeNeighbor;

sai_status_t create_neighbor_entry_fn(
    const sai_neighbor_entry_t* neighbor_entry,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  auto ip = facebook::fboss::fromSaiIpAddress(neighbor_entry->ip_address);
  folly::Optional<folly::MacAddress> dstMac;
  for (int i = 0; i < attr_count; ++i) {
    switch(attr_list[i].id) {
      case SAI_NEIGHBOR_ENTRY_ATTR_DST_MAC_ADDRESS:
        dstMac = facebook::fboss::fromSaiMacAddress(attr_list[i].value.mac);
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  if (!dstMac) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  fs->nm.create(
      std::make_tuple(neighbor_entry->switch_id, neighbor_entry->rif_id, ip),
      dstMac.value());
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_neighbor_entry_fn(
    const sai_neighbor_entry_t* neighbor_entry) {
  auto fs = FakeSai::getInstance();
  auto ip = facebook::fboss::fromSaiIpAddress(neighbor_entry->ip_address);
  fs->nm.remove(
      std::make_tuple(neighbor_entry->switch_id, neighbor_entry->rif_id, ip));
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_neighbor_entry_attribute_fn(
    const sai_neighbor_entry_t* neighbor_entry,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto ip = facebook::fboss::fromSaiIpAddress(neighbor_entry->ip_address);
  auto n =
      std::make_tuple(neighbor_entry->switch_id, neighbor_entry->rif_id, ip);
  auto& fn = fs->nm.get(n);
  switch (attr->id) {
    case SAI_NEIGHBOR_ENTRY_ATTR_DST_MAC_ADDRESS:
      fn.dstMac = facebook::fboss::fromSaiMacAddress(attr->value.mac);
      break;
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_neighbor_entry_attribute_fn(
    const sai_neighbor_entry_t* neighbor_entry,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  auto ip = facebook::fboss::fromSaiIpAddress(neighbor_entry->ip_address);
  auto n =
      std::make_tuple(neighbor_entry->switch_id, neighbor_entry->rif_id, ip);
  auto& fn = fs->nm.get(n);
  for (int i = 0; i < attr_count; ++i) {
    switch(attr_list[i].id) {
      case SAI_NEIGHBOR_ENTRY_ATTR_DST_MAC_ADDRESS:
        facebook::fboss::toSaiMacAddress(fn.dstMac, attr_list[i].value.mac);
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook {
namespace fboss {

static sai_neighbor_api_t _neighbor_api;

void populate_neighbor_api(sai_neighbor_api_t** neighbor_api) {
  _neighbor_api.create_neighbor_entry = &create_neighbor_entry_fn;
  _neighbor_api.remove_neighbor_entry = &remove_neighbor_entry_fn;
  _neighbor_api.set_neighbor_entry_attribute = &set_neighbor_entry_attribute_fn;
  _neighbor_api.get_neighbor_entry_attribute = &get_neighbor_entry_attribute_fn;
  *neighbor_api = &_neighbor_api;
}


} // namespace fboss
} // namespace facebook
