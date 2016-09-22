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

#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableMap.h"

namespace facebook { namespace fboss {

template<typename NodeT, typename FieldsT>
std::shared_ptr<NodeT> NodeBaseT<NodeT, FieldsT>::clone() const {
  return std::allocate_shared<NodeT>(CloneAllocator(), self());
}

template<typename NodeT, typename FieldsT>
void NodeBaseT<NodeT, FieldsT>::publish() {
  if (isPublished()) {
    return;
  }
  writableFields()->forEachChild([](NodeBase* child) {
    child->publish();
  });
  NodeBase::publish();
}

}} // facebook::fboss
