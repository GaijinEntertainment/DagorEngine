//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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
  InternedStrings *objectKeys = nullptr;
  BitstreamDeserializer(const danet::BitStream &bs_, InternedStrings *keys = nullptr) : bs(bs_), objectKeys(keys) {}
  bool read(void *to, size_t sz_in_bits, ecs::component_type_t user_type) const override;
  bool skip(ecs::component_index_t cidx, const ecs::DataComponent &compInfo);
};
struct BitstreamSerializer final : public ecs::SerializerCb
{
  danet::BitStream &bs;
  InternedStrings *objectKeys = nullptr;
  BitstreamSerializer(danet::BitStream &bs_, InternedStrings *keys = nullptr) : bs(bs_), objectKeys(keys) {}
  void write(const void *from, size_t sz_in_bits, ecs::component_type_t user_type) override;
};

} // namespace net
