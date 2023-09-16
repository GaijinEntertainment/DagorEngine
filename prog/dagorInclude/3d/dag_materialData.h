//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_ptrTab.h>
#include <3d/dag_texMgr.h>
#include <util/dag_simpleString.h>
#include <math/dag_color.h>

// max number of textures per mat
static constexpr int MAXMATTEXNUM = 16;

// material flags
enum
{
  MATF_2SIDED = 32,
};

typedef Color4 D3dColor;

struct D3dMaterial
{
  D3dColor diff, amb, spec, emis;
  float power;

  inline bool operator==(const D3dMaterial &m) const { return memcmp(this, &m, sizeof(D3dColor) * 4 + sizeof(float)) == 0; }
  inline bool operator!=(const D3dMaterial &m) const { return memcmp(this, &m, sizeof(D3dColor) * 4 + sizeof(float)) != 0; }
};


class MaterialData : public DObject
{
public:
  DAG_DECLARE_NEW(midmem)

  SimpleString matName, className, matScript;

  uint32_t flags;

  TEXTUREID mtex[MAXMATTEXNUM];
  D3dMaterial mat;


  MaterialData()
  {
    flags = 0;
    for (int i = 0; i < MAXMATTEXNUM; ++i)
      mtex[i] = BAD_TEXTUREID;
    mat.diff = mat.amb = mat.spec = mat.emis = D3dColor(0, 0, 0, 0);
  }

  void freeTextures()
  {
    for (int i = 0; i < MAXMATTEXNUM; ++i)
      if (mtex[i] != BAD_TEXTUREID)
      {
        release_managed_tex(mtex[i]);
        mtex[i] = BAD_TEXTUREID;
      }
  }

  bool isEqual(const MaterialData &d)
  {
    if (className != (const char *)d.className || matScript != (const char *)d.matScript || flags != d.flags || mat != d.mat)
      return false;
    for (int i = 0; i < MAXMATTEXNUM; ++i)
      if (mtex[i] != d.mtex[i])
        return false;
    return true;
  }
};


class MaterialDataList
{
public:
  PtrTab<MaterialData> list;


  MaterialDataList() : list(midmem_ptr()) {}

  int subMatCount() const { return list.size(); }

  MaterialData *getSubMat(int i) const
  {
    if (i < 0 || i >= list.size())
      return NULL;
    return list[i];
  }

  int addSubMat(MaterialData *md)
  {
    list.push_back(md);
    return list.size() - 1;
  }
};
