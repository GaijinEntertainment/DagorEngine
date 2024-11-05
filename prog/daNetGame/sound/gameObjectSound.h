// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/vector.h>
#include <math/dag_TMatrix.h>
#include <soundSystem/strHash.h>

struct GameObjectSoundAttributes
{
  typedef uint16_t id_t;
  typedef sndsys::str_hash_t hash_t;
  static constexpr size_t max_object_count = 8192;

  struct Attributes
  {
    hash_t soundTypeHash;
    hash_t soundShapeHash;
    Point3 extents;
    Attributes() = delete;
  };

  GameObjectSoundAttributes() = default;
  EA_NON_COPYABLE(GameObjectSoundAttributes);

  id_t add(const char *sound_type, const char *sound_shape, const TMatrix &tm)
  {
    const Point3 extents(length(tm.getcol(0)) * 0.5f, length(tm.getcol(1)) * 0.5f, length(tm.getcol(2)) * 0.5f);
    attributes.push_back(Attributes{SND_HASH_SLOW(sound_type), SND_HASH_SLOW(sound_shape), extents});
    G_ASSERT(attributes.size() != max_object_count + 1);
    return attributes.size() - 1;
  }

  const Attributes *get(id_t id) const
  {
    const intptr_t idx = intptr_t(id);
    G_ASSERT_RETURN(size_t(idx) < attributes.size(), nullptr);
    return &attributes[idx];
  }

  size_t size() const { return attributes.size(); }

  void reserve(size_t size)
  {
    G_ASSERT(size <= max_object_count);
    attributes.reserve(size);
  }

  void clear() { attributes.clear(); }

private:
  eastl::vector<Attributes> attributes;
};
