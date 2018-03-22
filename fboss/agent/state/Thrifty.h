#pragma once

#include <folly/dynamic.h>
#include <folly/json.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/agent/state/NodeBase.h"

namespace facebook { namespace fboss {

//
// Special version of NodeBaseT that features methods to convert
// object to/from JSON using Thrift serializers. For this to work
// one must supply Thrift type (ThrifT) that stores FieldsT state.
//
// TODO: in future, FieldsT and ThrifT should be one type
//
template <typename ThriftT, typename NodeT, typename FieldsT>
class ThriftyBaseT : public NodeBaseT<NodeT, FieldsT> {
 public:
  using NodeBaseT<NodeT, FieldsT>::NodeBaseT;

  static
  std::shared_ptr<NodeT> fromFollyDynamic(folly::dynamic const& dyn) {
    auto const jsonStr = folly::toJson(dyn);
    auto inBuf =
        folly::IOBuf::wrapBufferAsValue(jsonStr.data(), jsonStr.size());
    auto obj = apache::thrift::SimpleJSONSerializer::deserialize<ThriftT>(
        folly::io::Cursor{&inBuf});
    auto fields = FieldsT::fromThrift(obj);
    return std::make_shared<NodeT>(fields);
  }

  static
  std::shared_ptr<NodeT> fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    auto obj = this->getFields()->toThrift();
    std::string jsonStr;
    apache::thrift::SimpleJSONSerializer::serialize(obj, &jsonStr);
    return folly::parseJson(jsonStr);
  }
};

}} // facebook::fboss
