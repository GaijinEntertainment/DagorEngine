// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// for TexmapContainer/Texmap and for the MtlMakerCallback/TexHandle the viewport texture is built
// through, so that this header does not depend on what its includer pulled in first
#include <max.h>
#include <stdmat.h>

// for iequal()
#include "common.h"

#define NUMTEXMAPS 16

class Texmaps : public TexmapContainer
{
public:
  Texmap *texmap[NUMTEXMAPS] = {};

  Texmap *gettex(int i) { return (Texmap *)GetReference(i); }
  void settex(int i, Texmap *t) { ReplaceReference(i, t); }

  // the file the slot shows, NULL when it is empty or holds a texmap of another kind
  const wchar_t *gettexname(int i)
  {
    Texmap *tex = gettex(i);
    if (tex && tex->ClassID() == Class_ID(BMTEX_CLASS_ID, 0))
      return ((BitmapTex *)tex)->GetMapName();

    return NULL;
  }

  // an empty path asks whether the slot is empty
  bool holds_texname(int i, std::wstring_view path)
  {
    if (path.empty())
      return !gettex(i);

    const wchar_t *name = gettexname(i);
    return name && iequal(name, path);
  }

  Class_ID ClassID() override { return Texmaps_CID; }
  void GetClassName(TSTR &s, bool localized = true) { s = _T("DagorTexmaps"); }
  void DeleteThis() override { delete this; }
  int NumSubs() override { return NUMTEXMAPS; }
  Animatable *SubAnim(int i) override
  {
    if (i >= 0 && i < NUMTEXMAPS)
      return texmap[i];
    return NULL;
  }
  TSTR SubAnimName(int i, bool localized) override
  {
    TSTR s;
    s.printf(_T("tex %d"), i);
    return s;
  }
  int SubNumToRefNum(int n) override { return n; }
  int NumRefs() override { return NUMTEXMAPS; }
  RefTargetHandle GetReference(int i) override
  {
    if (i >= 0 && i < NUMTEXMAPS)
      return texmap[i];
    return NULL;
  }
  void SetReference(int i, RefTargetHandle rtarg) override
  {
    if (i >= 0 && i < NUMTEXMAPS)
      texmap[i] = (Texmap *)rtarg;
  }
  RefTargetHandle Clone(RemapDir &remap) override
  {
    Texmaps *mtl = new Texmaps;
    for (int i = 0; i < NUMTEXMAPS; ++i)
      mtl->ReplaceReference(i, remap.CloneRef(texmap[i]));
    BaseClone(this, mtl, remap);
    return mtl;
  }

  RefResult NotifyRefChanged(const Interval &changeInt, RefTargetHandle hTarget, PartID &partID, RefMessage message,
    BOOL propagate) override

  {
    switch (message)
    {
      case REFMSG_GET_PARAM_DIM:
      {
        GetParamDim *gpd = (GetParamDim *)partID;
        gpd->dim = defaultDim;
        break;
      }
      case REFMSG_GET_PARAM_NAME_NONLOCALIZED: return REF_STOP;
    }
    return (REF_SUCCEED);
  }
};

// The viewport texture of a Dagor material is the colour DIB of its first texture slot with the mono
// DIB of the same slot merged into the alpha. DagorMat2 builds it through make_vp_tex_handle();
// the two functions above it are its steps.

// GetVPDisplayDIB() hands over a DIB the caller owns, and MtlMakerCallback has no way to release one
// except MakeHandle(), which takes ownership. A throwaway handle is how an unused DIB gets freed.
inline void discard_vp_dib(MtlMakerCallback &cb, BITMAPINFO *bmi)
{
  if (TexHandle *th = cb.MakeHandle(bmi))
    th->DeleteThis();
}

// Writes the intensity of `alpha` into the alpha channel of `color`, in place. Leaves `color` alone
// unless both DIBs are palette-less 32bpp, the form that puts the pixels right after the header, and
// unless their geometry matches exactly: one lockstep walk only keeps the rows, and the top-down or
// bottom-up order biHeight's sign carries, aligned when both sides step the same grid.
inline void merge_vp_dib_alpha(BITMAPINFO *color, const BITMAPINFO *alpha)
{
  const BITMAPINFOHEADER &ch = color->bmiHeader;
  const BITMAPINFOHEADER &ah = alpha->bmiHeader;

  if (ch.biBitCount != 32 || ch.biCompression != BI_RGB)
    return;
  if (ah.biBitCount != 32 || ah.biCompression != BI_RGB)
    return;
  if (ch.biWidth != ah.biWidth || ch.biHeight != ah.biHeight)
    return;

  // biHeight is negative for a top-down DIB
  const size_t numPixels = size_t(ch.biWidth) * size_t(ch.biHeight < 0 ? -ch.biHeight : ch.biHeight);

  UBYTE *colorPtr = (UBYTE *)((BYTE *)color + sizeof(BITMAPINFOHEADER));
  const UBYTE *alphaPtr = (const UBYTE *)((const BYTE *)alpha + sizeof(BITMAPINFOHEADER));
  for (size_t i = 0; i < numPixels; ++i, colorPtr += 4, alphaPtr += 4)
    colorPtr[3] = (UBYTE)(((int)alphaPtr[0] + alphaPtr[1] + alphaPtr[2]) / 3);
}

// Builds the viewport texture handle of the first texture slot, NULL if the slot is empty or the
// driver would not take the DIB. Leaves nothing behind either way: MakeHandle() takes the colour DIB
// over, and the mono one is always discarded.
inline TexHandle *make_vp_tex_handle(Texmaps *texmaps, TimeValue t, MtlMakerCallback &cb, Interval &valid)
{
  Texmap *tex = texmaps ? texmaps->gettex(0) : NULL;
  if (!tex)
    return NULL;

  BITMAPINFO *bmiColor = tex->GetVPDisplayDIB(t, cb, valid, FALSE, 0, 0);
  if (!bmiColor)
    return NULL;

  BITMAPINFO *bmiAlpha = tex->GetVPDisplayDIB(t, cb, valid, TRUE, 0, 0);
  if (!bmiAlpha)
  {
    discard_vp_dib(cb, bmiColor);
    return NULL;
  }

  merge_vp_dib_alpha(bmiColor, bmiAlpha);
  discard_vp_dib(cb, bmiAlpha);

  return cb.MakeHandle(bmiColor);
}
