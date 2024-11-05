//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/baseIo.h>

namespace danet
{
class BitStream;
}
namespace ecs
{
struct DataComponent;
}

namespace net
{

struct InternedStrings;
class IConnection;

struct BitstreamDeserializer final : public ecs::DeserializerCb
{
  const danet::BitStream &bs;
  ecs::EntityManager &mgr;
  InternedStrings *objectKeys = nullptr;
  BitstreamDeserializer(ecs::EntityManager &mgr, const danet::BitStream &bs_, InternedStrings *keys = nullptr) :
    mgr(mgr), bs(bs_), objectKeys(keys)
  {}
  bool read(void *to, size_t sz_in_bits, ecs::component_type_t user_type) const override;
  bool skip(ecs::component_index_t cidx, const ecs::DataComponent &compInfo);
};
struct BitstreamSerializer final : public ecs::SerializerCb
{
  danet::BitStream &bs;
  ecs::EntityManager &mgr;
  InternedStrings *objectKeys = nullptr;
  BitstreamSerializer(ecs::EntityManager &mgr, danet::BitStream &bs_, InternedStrings *keys = nullptr) :
    mgr(mgr), bs(bs_), objectKeys(keys)
  {}
  void write(const void *from, size_t sz_in_bits, ecs::component_type_t user_type) override;
};

} // namespace net
