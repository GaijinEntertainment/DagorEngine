// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/ddsxTex.h>
#include <generic/dag_tab.h>
#include <util/dag_globDef.h>

//
// process mips to lay down in reversed order (smaller mips first) and faces-inside-level cubemaps
//
static inline void ddsx_reverse_mips_inplace(ddsx::Header &hdr, char *tex_data, int tex_data_size)
{
  if (hdr.flags & hdr.FLG_REV_MIP_ORDER)
    return;
  if (hdr.flags & (hdr.FLG_GENMIP_BOX | hdr.FLG_GENMIP_KAIZER))
    return;
  if (hdr.levels <= 1)
    return;
  if (tex_data_size != hdr.memSz || hdr.packedSz)
    return;

  Tab<char> rev_data(defaultmem);
  rev_data.resize(hdr.memSz);

  if (hdr.flags & hdr.FLG_CUBTEX)
  {
    int mip_ofs[32], face_sz = 0;
    for (int mip = 0; mip < hdr.levels; mip++)
    {
      mip_ofs[mip] = face_sz;
      face_sz += hdr.getSurfaceSz(mip);
    }

    char *rev_data_p = rev_data.data();
    for (int mip = hdr.levels - 1; mip >= 0; mip--)
    {
      int sz = hdr.getSurfaceSz(mip);
      for (int face = 0; face < 6; face++, rev_data_p += sz)
        memcpy(rev_data_p, tex_data + face_sz * face + mip_ofs[mip], sz);
    }
  }
  else
  {
    int d = (hdr.flags & hdr.FLG_VOLTEX) ? hdr.depth : 1;
    char *rev_data_p = rev_data.data() + data_size(rev_data);
    const char *fwd_data_p = tex_data;
    for (int mip = 0; mip < hdr.levels; mip++)
    {
      int sz = hdr.getSurfaceSz(mip) * max(d >> mip, 1);
      rev_data_p -= sz;
      memcpy(rev_data_p, fwd_data_p, sz);
      fwd_data_p += sz;
    }
  }

  hdr.flags |= hdr.FLG_REV_MIP_ORDER;
  mem_copy_to(rev_data, tex_data);
}

static inline void ddsx_reverse_mips_inplace(void *data, int data_size)
{
  ddsx_reverse_mips_inplace(*(ddsx::Header *)data, (char *)data + sizeof(ddsx::Header), data_size - sizeof(ddsx::Header));
}


//
// process mips to lay down in DDS order (bigger mips first)
//
static inline void ddsx_forward_mips_inplace(ddsx::Header &hdr, char *tex_data, int tex_data_size)
{
  if (!(hdr.flags & hdr.FLG_REV_MIP_ORDER))
    return;
  if (hdr.flags & (hdr.FLG_GENMIP_BOX | hdr.FLG_GENMIP_KAIZER))
    return;
  if (hdr.levels <= 1)
    return;
  if (tex_data_size != hdr.memSz)
    return;

  Tab<char> fwd_data(defaultmem);
  fwd_data.resize(hdr.memSz);

  if (hdr.flags & hdr.FLG_CUBTEX)
  {
    int mip_ofs[32], face_sz = 0;
    for (int mip = 0; mip < hdr.levels; mip++)
    {
      mip_ofs[mip] = face_sz;
      face_sz += hdr.getSurfaceSz(mip);
    }

    char *rev_data_p = tex_data;
    for (int mip = hdr.levels - 1; mip >= 0; mip--)
    {
      int sz = hdr.getSurfaceSz(mip);
      for (int face = 0; face < 6; face++, rev_data_p += sz)
        memcpy(fwd_data.data() + face_sz * face + mip_ofs[mip], rev_data_p, sz);
    }
  }
  else
  {
    char *fwd_data_p = fwd_data.data() + data_size(fwd_data);
    const char *rev_data_p = tex_data;
    int d = (hdr.flags & hdr.FLG_VOLTEX) ? hdr.depth : 1;

    for (int mip = hdr.levels - 1; mip >= 0; mip--)
    {
      int sz = hdr.getSurfaceSz(mip) * max(d >> mip, 1);
      fwd_data_p -= sz;
      memcpy(fwd_data_p, rev_data_p, sz);
      rev_data_p += sz;
    }
  }

  hdr.flags &= ~hdr.FLG_REV_MIP_ORDER;
  mem_copy_to(fwd_data, tex_data);
}

static inline void ddsx_forward_mips_inplace(void *data, int data_size)
{
  ddsx_forward_mips_inplace(*(ddsx::Header *)data, (char *)data + sizeof(ddsx::Header), data_size - sizeof(ddsx::Header));
}
