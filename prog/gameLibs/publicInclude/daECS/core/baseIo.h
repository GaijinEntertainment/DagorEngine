//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "component.h"

namespace ecs
{

class SerializerCb;
class DeserializerCb;
class EntityComponentRef;
class ChildComponent;

bool can_serialize_type(type_index_t typeId, const ecs::EntityManager &mgr);
bool should_replicate(component_index_t comp, const ecs::EntityManager &mgr);
// a bit optimized for componentRed
bool can_serialize(const ecs::EntityComponentRef &attr, const ecs::EntityManager &mgr);    // if there is custom serializer OR isPod
bool should_replicate(const ecs::EntityComponentRef &attr, const ecs::EntityManager &mgr); // if can_serialize & there is no flags
                                                                                           // (IS_COPY, DONT_REPLICATE) preventing it

// if component type is boxed, then this is void** (pointer to pointer to actual data)
void serialize_entity_component_ref_typeless(const void *comp_data, component_index_t cidx, component_type_t type_name,
  type_index_t type_id, SerializerCb &serializer, const ecs::EntityManager &mgr);

void serialize_entity_component_ref_typeless(const void *comp_data, // if component type is boxed, then this is void** (pointer to
                                                                    // pointer to actual data)
  component_type_t type_name, SerializerCb &serializer, const ecs::EntityManager &mgr);
void serialize_entity_component_ref_typeless(const EntityComponentRef &attr, SerializerCb &serializer,
  const ecs::EntityManager &mgr); // write without type

MaybeChildComponent deserialize_init_component_typeless(ecs::component_type_t type_name, ecs::component_index_t cidx,
  const DeserializerCb &serializer, ecs::EntityManager &mgr); // read type provided type
bool deserialize_component_typeless(EntityComponentRef &attr, const DeserializerCb &deserializer,
  const ecs::EntityManager &mgr); // type is in EntityComponentRef
bool deserialize_component_typeless(void *raw_data, component_type_t type_name, const DeserializerCb &deserializer,
  const ecs::EntityManager &mgr);

void serialize_child_component(const ChildComponent &comp, SerializerCb &serializer,
  ecs::EntityManager &mgr); // write type, than
                            // serialize_entity_component_ref_typeless
MaybeChildComponent deserialize_child_component(const DeserializerCb &serializer, ecs::EntityManager &mgr); // Empty optional<> means
                                                                                                            // non recoverable error
                                                                                                            // (bad stream, unknown
                                                                                                            // component type, ...)

void write_string(ecs::SerializerCb &cb, const char *pStr, uint32_t max_string_len);
int read_string(const ecs::DeserializerCb &cb, char *buf, uint32_t buf_size);

class SerializerCb
{
public:
  virtual void write(const void *, size_t sz_in_bits, component_type_t hint) = 0;
};

class DeserializerCb
{
public:
  virtual bool read(void *, size_t sz_in_bits, component_type_t hint) const = 0;
};

inline void write_compressed(SerializerCb &cb, uint32_t v)
{
  uint8_t data[sizeof(v) + 1];
  int i = 0;
  for (; i < sizeof(data); ++i)
  {
    data[i] = uint8_t(v) | (v >= (1 << 7) ? (1 << 7) : 0);
    v >>= 7;
    if (!v)
    {
      ++i;
      break;
    }
  }
  cb.write(data, i * CHAR_BIT, 0);
}

inline bool read_compressed(const DeserializerCb &cb, uint32_t &v)
{
  v = 0;
  for (int i = 0; i < sizeof(v) + 1; ++i)
  {
    uint8_t byte = 0;
    if (!cb.read(&byte, CHAR_BIT, 0))
      return false;
    v |= uint32_t(byte & ~(1 << 7)) << (i * 7);
    if ((byte & (1 << 7)) == 0)
      break;
  }
  return true;
}

// default serializer is just call as-is.
// if we want we can register different serializer, with quantization and stuff
class ComponentSerializer
{
public:
  virtual void serialize(SerializerCb &cb, const void *data, size_t sz, component_type_t hint) { cb.write(data, sz * CHAR_BIT, hint); }
  virtual bool deserialize(const DeserializerCb &cb, void *data, size_t sz, component_type_t hint)
  {
    return cb.read(data, sz * CHAR_BIT, hint);
  }
};

}; // namespace ecs
