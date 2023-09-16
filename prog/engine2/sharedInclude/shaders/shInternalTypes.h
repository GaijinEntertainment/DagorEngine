// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _GAIJIN_SHADERS_SHINTERNALTYPES_H
#define _GAIJIN_SHADERS_SHINTERNALTYPES_H
#pragma once

#include <generic/dag_patchTab.h>
#include <3d/dag_texMgr.h>

namespace shaders_internal
{
struct Tex
{
  PATCHABLE_DATA64(TEXTUREID, texId);
  PATCHABLE_DATA64(BaseTexture *, tex);

  void init(TEXTUREID tid)
  {
    texId = tid;
    tex = NULL;
  }
  void replace(TEXTUREID tid)
  {
    Tex prev = *this;
    init(tid);
    prev.term();
  }
  void term()
  {
    release();
    texId = BAD_TEXTUREID;
  }

  BaseTexture *get() { return tex ? tex : (texId != BAD_TEXTUREID ? (tex = acquire_managed_tex(texId)) : NULL); }
  bool release()
  {
    if (!tex)
      return false;
    if (texId != BAD_TEXTUREID)
      release_managed_tex(texId);
    tex = NULL;
    return true;
  }
};

struct Buf
{
  PATCHABLE_DATA64(D3DRESID, bufId);
  PATCHABLE_DATA64(Sbuffer *, buf);

  void init(D3DRESID rid)
  {
    bufId = rid;
    buf = NULL;
  }
  void replace(D3DRESID tid)
  {
    Buf prev = *this;
    init(tid);
    prev.term();
  }
  void term()
  {
    release();
    bufId = BAD_D3DRESID;
  }

  Sbuffer *get() { return buf ? buf : (bufId != BAD_D3DRESID ? (buf = acquire_managed_buf(bufId)) : NULL); }
  bool release()
  {
    if (!buf)
      return false;
    if (bufId != BAD_D3DRESID)
      release_managed_res(bufId);
    buf = NULL;
    return true;
  }
};
} // namespace shaders_internal

#endif
