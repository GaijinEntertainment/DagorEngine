// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/tql.h>
#include <drv/3d/dag_tex3d.h>
#include <math/dag_adjpow2.h>
#include <util/dag_delayedAction.h>
#include <osApiWrappers/dag_miscApi.h>

// #define TRACE debug  // trace resource manager's DDSx loads
#ifndef TRACE
#define TRACE(...) ((void)0)
#endif

BaseTexture *tql::makeResizedTmpTexResCopy(BaseTexture *t, unsigned w, unsigned h, unsigned d, unsigned l, unsigned tex_ld_lev)
{
  TextureInfo ti;
  t->getinfo(ti, 0);

  const unsigned tex_w = ti.w, tex_h = ti.h, tex_d = ti.d, tex_a = ti.a, tex_l = ti.mipLevels;
  const unsigned tex_lev = get_log2i(max(max(ti.w, ti.h), ti.d));
  const unsigned start_src_level = tex_ld_lev >= tex_lev ? 0 : tex_lev - tex_ld_lev;
  unsigned a = 1;

  if (t->restype() == RES3D_CUBETEX)
  {
    d = 1;
    a = 6;
  }
  else if (t->restype() == RES3D_ARRTEX)
  {
    a = d;
    d = 1;
  }

  G_ASSERTF_RETURN(tex_a == a, nullptr, "restype=%d a=%d tex_a=%d tex=%dx%dx%d,L%d -> %dx%dx%d,L%d", t->restype(), a, tex_a, tex_w,
    tex_h, max(tex_d, tex_a), tex_l, w, h, max(d, a), l);

  if (tex_w == w && tex_h == h && tex_d == d && tex_l == l)
    return t;
  else if (tex_w >= w && tex_h >= h && tex_d >= d && tex_l >= l)
  {
    // resize down
    unsigned lev_ofs = 0;
    while (lev_ofs < tex_l &&
           (max(tex_w >> lev_ofs, 1u) != w || max(tex_h >> lev_ofs, 1u) != h || max(tex_d >> lev_ofs, tex_a) != max(d, a)))
      lev_ofs++;
    if (max(tex_w >> lev_ofs, 1u) != w || max(tex_h >> lev_ofs, 1u) != h || max(tex_d >> lev_ofs, tex_a) != max(d, a))
    {
      logerr("can't resize tex=%p(%s) %dx%dx%d,L%d  to %dx%dx%d,L%d", t, t->getTexName(), tex_w, tex_h, max(tex_d, tex_a), tex_l, w, h,
        max(d, a), l);
      return nullptr;
    }

    return t->downSize(w, h, d, l, start_src_level, lev_ofs);
  }
  else
  {
    // resize up
    unsigned lev_ofs = 0;
    while (
      lev_ofs < 16 && (max(w >> lev_ofs, 1u) != tex_w || max(h >> lev_ofs, 1u) != tex_h || max(d >> lev_ofs, a) != max(tex_d, tex_a)))
      lev_ofs++;
    if (max(w >> lev_ofs, 1u) != tex_w || max(h >> lev_ofs, 1u) != tex_h || max(d >> lev_ofs, a) != max(tex_d, tex_a))
    {
      logerr("can't resize tex=%p(%s) %dx%dx%d,L%d  to %dx%dx%d,L%d", t, t->getTexName(), tex_w, tex_h, max(tex_d, tex_a), tex_l, w, h,
        max(d, a), l);
      return nullptr;
    }

    return t->upSize(w, h, d, l, start_src_level, lev_ofs);
  }

  return nullptr;
}
