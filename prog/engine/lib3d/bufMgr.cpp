// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_buffers.h>
#include <3d/dag_texMgr.h>

Sbuffer *acquire_managed_buf(D3DRESID id)
{
  D3dResource *res = acquire_managed_res(id);
  if (!res)
    return nullptr;
  auto t = res->getType();
  G_ASSERTF_RETURN(t == D3DResourceType::SBUF, nullptr, "non-buf res in acquire_managed_buf(0x%x), type=%d", id,
    eastl::to_underlying(t));
  Sbuffer *buf = (Sbuffer *)res;
  G_ASSERTF_RETURN((buf->getFlags() & SBCF_BIND_MASK) != SBCF_BIND_INDEX && (buf->getFlags() & SBCF_BIND_MASK) != SBCF_BIND_VERTEX,
    nullptr, "ibuf/vbuf res in acquire_managed_buf(0x%x)", id); // index/vertex buffer intetionally not returned
  return buf;
}
