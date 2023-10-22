#include <image/dag_texPixel.h>

TexImage32 *TexImage32::create(int w, int h, IMemAlloc *mem)
{
  if (!mem)
    mem = tmpmem;
  TexImage32 *im = (TexImage32 *)mem->alloc(w * h * 4 + sizeof(TexImage));
  im->w = w;
  im->h = h;
  return im;
}

TexImage8a *TexImage8a::create(int w, int h, IMemAlloc *mem)
{
  if (!mem)
    mem = tmpmem;
  TexImage8a *im = (TexImage8a *)mem->alloc(w * h * 2 + sizeof(TexImage));
  im->w = w;
  im->h = h;
  return im;
}

TexImage8 *TexImage8::create(int w, int h, IMemAlloc *mem)
{
  if (!mem)
    mem = tmpmem;
  TexImage8 *im = (TexImage8 *)mem->alloc(w * h * 1 + sizeof(TexImage));
  im->w = w;
  im->h = h;
  return im;
}

TexImageR *TexImageR::create(int w, int h, IMemAlloc *mem)
{
  if (!mem)
    mem = tmpmem;
  TexImageR *im = (TexImageR *)mem->alloc(w * h * 4 + sizeof(TexImage));
  im->w = w;
  im->h = h;
  return im;
}

TexImageF *TexImageF::create(int w, int h, IMemAlloc *mem)
{
  if (!mem)
    mem = tmpmem;
  TexImageF *im = (TexImageF *)mem->alloc(w * h * 4 * 3 + sizeof(TexImage));
  im->w = w;
  im->h = h;
  return im;
}
