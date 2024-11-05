// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_buffers.h>
#include <3d/dag_texMgr.h>

Sbuffer *acquire_managed_buf(D3DRESID id)
{
  D3dResource *res = acquire_managed_res(id);
  if (!res)
    return nullptr;
  int t = res->restype();
  G_ASSERTF_RETURN(t == RES3D_SBUF, nullptr, "non-buf res in acquire_managed_buf(0x%x), type=%d", id, t);
  Sbuffer *buf = (Sbuffer *)res;
  G_ASSERTF_RETURN((buf->getFlags() & SBCF_BIND_MASK) != SBCF_BIND_INDEX && (buf->getFlags() & SBCF_BIND_MASK) != SBCF_BIND_VERTEX,
    nullptr, "ibuf/vbuf res in acquire_managed_buf(0x%x)", id); // index/vertex buffer intetionally not returned
  return buf;
}
