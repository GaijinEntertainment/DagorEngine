// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <debug/dag_assert.h>
#include <math/dag_Point3.h>
#include <util/dag_stdint.h>
#include <util/dag_stringTableAllocator.h>
#include <string.h>

namespace dacsg
{
enum AttrSetBits : uint8_t
{
  ATTR_SET_COLOR = 1 << 0,
  ATTR_SET_MAT = 1 << 1,
  ATTR_SET_UV = 1 << 2,
};

// CH_USER + x, x counting from 0, is a user channel with an optional name
enum ChannelId : uint32_t
{
  CH_UV = 0, // dim 2, present after a resolve or addDefaultChannels(), not on a new Mesh
  CH_COLOR,  // dim 4, linear RGBA, present as CH_UV is
  CH_NORMAL, // dim 3, a leaf may supply it; execute sets it on every result
  CH_UV2,
  CH_UV3,
  CH_UV4,
  CH_UV5,
  CH_UV6,
  CH_UV7,
  CH_UV8,
  CH_UV9,
  CH_UV10,
  CH_USER = 0x10000,
};
constexpr float DEFAULT_COLOR[4] = {0.8f, 0.8f, 0.8f, 1.f};
// 0: a user channel, whose dim its maker picks, or an id past CH_UV10, which the contract refuses
constexpr uint8_t channel_dim(uint32_t id) { return id == CH_COLOR ? 4 : id == CH_NORMAL ? 3 : id <= CH_UV10 ? 2 : 0; }

struct FaceInfo
{
  uint32_t prim; // the source primitive's node; a Cut's cap face: the Cut node, not a Prim
  uint32_t srcFace;
  uint32_t smgr;
  uint16_t mat;
  uint8_t attrSet;
};

struct Channel
{
  uint32_t id = CH_UV;
  uint8_t dim = 0;
  dag::Vector<float> data; // per corner: face count * 3 * dim
};

struct Mesh
{
  static constexpr uint32_t NO_NAME = ~0u;

  dag::Vector<Point3> pos;
  dag::Vector<uint32_t> tri; // CCW seen from outside
  dag::Vector<FaceInfo> face;
  dag::Vector<Channel> channels;
  StringTableAllocator userNames;    // the names of user channels
  dag::Vector<uint32_t> userNameOfs; // by id - CH_USER: an offset in userNames or NO_NAME

  uint32_t faceCount() const { return face.size(); }

  const Channel *findChannel(uint32_t id) const
  {
    for (const Channel &c : channels)
      if (c.id == id)
        return &c;
    return nullptr;
  }
  Channel *findChannel(uint32_t id) { return const_cast<Channel *>(static_cast<const Mesh *>(this)->findChannel(id)); }

  // Zero-filled, in the dim of a predefined id or the dim asked; an existing id of that dim returns as is, one of another
  // dim is zero-filled anew; the reference lasts until a new id is added
  Channel &addChannel(uint32_t id, uint8_t dim)
  {
    dim = channel_dim(id) ? channel_dim(id) : dim;
    Channel *c = findChannel(id);
    if (c && c->dim == dim)
      return *c;
    if (!c)
      c = &channels.push_back();
    c->id = id;
    c->dim = dim;
    c->data.assign(face.size() * 3 * dim, 0.f);
    return *c;
  }
  // The always-present channels a source left out: zero uv, DEFAULT_COLOR
  void addDefaultChannels()
  {
    if (!findChannel(CH_UV))
      addChannel(CH_UV, 2);
    if (findChannel(CH_COLOR))
      return;
    Channel &c = addChannel(CH_COLOR, 4);
    for (uint32_t i = 0; i < c.data.size(); ++i)
      c.data[i] = DEFAULT_COLOR[i % 4];
  }

  // nullptr for a predefined id or an unnamed user id
  const char *userChannelName(uint32_t id) const
  {
    const uint32_t x = id - CH_USER;
    return id >= CH_USER && x < userNameOfs.size() && userNameOfs[x] != NO_NAME ? userNames.getDataRawUnsafe(userNameOfs[x]) : nullptr;
  }
  void setUserChannelName(uint32_t id, const char *name)
  {
    G_ASSERT_RETURN(id >= CH_USER && name, );
    const uint32_t x = id - CH_USER;
    if (userNameOfs.size() <= x)
      userNameOfs.resize(x + 1, NO_NAME);
    userNameOfs[x] = userNames.addDataRaw(name, strlen(name) + 1);
  }
};

// nullptr, or why m breaks the Mesh contract above, which write_obj and to_dag_mesh take as given
const char *mesh_contract_breach(const Mesh &m);
} // namespace dacsg
