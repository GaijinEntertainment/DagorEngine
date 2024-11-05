//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_shaders.h>
#include <generic/dag_smallTab.h>
#include <util/dag_stdint.h>

class RingDynamicVB;
class RingDynamicIB;


class DynShaderMeshBuf
{
public:
  DynShaderMeshBuf() { ctorInit(); }
  ~DynShaderMeshBuf() { close(); }

  void setRingBuf(RingDynamicVB *_vb, RingDynamicIB *_ib);

  void init(dag::ConstSpan<CompiledShaderChannelId> ch, int max_v, int max_f);
  void initCopy(const DynShaderMeshBuf &b);
  void close();

  inline ShaderElement *getCurrentShader() { return currentShader; }
  inline void setCurrentShader(ShaderElement *shader)
  {
    if (currentShader != shader)
    {
      flush();
      currentShader = shader;
      currentShader->replaceVdecl(vdecl);
    }
  }

  bool fillRawFaces(void **__restrict out_vdata, int num_v, uint16_t **__restrict out_ind, int num_f, int &v_base);
  template <typename T>
  bool fillRawFacesEx(T **__restrict ov, int nv, uint16_t **__restrict oi, int nf, int &v_base)
  {
    void *p;
    if (fillRawFaces(&p, nv, oi, nf, v_base))
    {
      *ov = (T *)p;
      return true;
    }
    *ov = NULL;
    return false;
  }

  void addFaces(const void *__restrict vdata, int num_v, const uint16_t *ind, int num_f);
  void addFacesI32(const void *__restrict vdata, int num_v, const int *i_data, int num_f)
  {
    void *vp;
    uint16_t *ip;
    int vbase;

    if (fillRawFaces(&vp, num_v, &ip, num_f, vbase))
    {
      memcpy(vp, vdata, num_v * stride);
      for (uint16_t *end = ip + num_f * 3; ip < end; ip++, i_data++)
        *ip = *i_data + vbase;
    }
  }

  void flush(bool clear_used = true);
  void clear() { vUsed = iUsed = 0; } //< clear prepared buffer without rendering
  void render(int vertices, int indices);
  int getVertices() const { return vUsed; }
  int getIndices() const { return iUsed; }

  bool hasEnoughBufFor(int v, int f) const { return ((vUsed + v) * stride <= data_size(vBuf)) && (iUsed + f * 3 <= iBuf.size()); }

private:
  SmallTab<uint8_t, InimemAlloc> vBuf;
  SmallTab<uint16_t, InimemAlloc> iBuf;
  Ptr<ShaderElement> currentShader;
  VDECL vdecl;
  RingDynamicVB *vb;
  RingDynamicIB *ib;
  int last_start_v, last_start_i;
  int stride;
  int vUsed, iUsed;

  void ctorInit()
  {
    vdecl = BAD_VDECL;
    vb = NULL;
    ib = NULL;
    last_start_v = last_start_i = 0;
    stride = 0;
    vUsed = iUsed = 0;
  }
};


class DynShaderQuadBuf
{
public:
  DynShaderQuadBuf() { ctorInit(); }
  ~DynShaderQuadBuf() { close(); }

  void setRingBuf(RingDynamicVB *_vb);

  void init(dag::ConstSpan<CompiledShaderChannelId> ch, int max_quads);
  void initCopy(const DynShaderQuadBuf &b);
  void close();

  inline ShaderElement *getCurrentShader() { return currentShader; }
  inline void setCurrentShader(ShaderElement *shader)
  {
    if (currentShader != shader)
    {
      flush();
      currentShader = shader;
      currentShader->replaceVdecl(vdecl);
    }
  }

  bool fillRawQuads(void **__restrict out_vdata, int quad_num);
  template <typename T>
  bool fillRawQuadsEx(T **__restrict ov, int nq)
  {
    void *p;
    if (fillRawQuads(&p, nq))
    {
      *ov = (T *)p;
      return true;
    }
    *ov = NULL;
    return false;
  }

  void addQuads(const void *__restrict vertices, int quad_num);
  void *lockData(int max_quad_num);
  void unlockDataAndFlush(int quad_num);
  void flush();
  void clear() { qUsed = 0; } //< clear prepared buffer without rendering

  bool hasEnoughBufFor(int q) const { return ((qUsed + q) * qStride <= data_size(vBuf)); }

  VDECL getVDecl() const { return vdecl; }

private:
  SmallTab<uint8_t, InimemAlloc> vBuf;
  Ptr<ShaderElement> currentShader;
  RingDynamicVB *vb;
  VDECL vdecl;
  int qStride;
  int qUsed;

  void ctorInit()
  {
    vdecl = BAD_VDECL;
    vb = NULL;
    qStride = 0;
    qUsed = 0;
  }
};


class DynShaderStripBuf
{
public:
  DynShaderStripBuf() { ctorInit(); }
  ~DynShaderStripBuf() { close(); }

  void setRingBuf(RingDynamicVB *_vb);

  void init(dag::ConstSpan<CompiledShaderChannelId> ch, int max_v_pairs);
  void initCopy(const DynShaderStripBuf &b);
  void close();

  inline ShaderElement *getCurrentShader() { return currentShader; }
  inline void setCurrentShader(ShaderElement *shader)
  {
    if (currentShader != shader)
    {
      flush();
      currentShader = shader;
      currentShader->replaceVdecl(vdecl);
    }
  }

  // adds joined vertex pairs
  bool fillRawVertPairs(void **out_vdata, int v_pair_num);
  template <typename T>
  bool fillRawVertPairsEx(T **ov, int vpn)
  {
    void *p;
    if (fillRawVertPairs(&p, vpn))
    {
      *ov = (T *)p;
      return true;
    }
    *ov = NULL;
    return false;
  }

  // adds disjoined vertex pairs; returns pointer to last vertex and preallocate one more pair if needed
  bool fillRawVertPairsDetached(void **__restrict out_vdata, int v_pair_num, void **out_lastv)
  {
    if (!pUsed || (pUsed + v_pair_num) * pStride > data_size(vBuf))
      *out_lastv = NULL;
    else
    {
      *out_lastv = &vBuf[pUsed * pStride - pStride / 2];
      v_pair_num++;
    }
    return fillRawVertPairsEx(out_vdata, v_pair_num);
  }
  template <typename T>
  bool fillRawVertPairsDetachedEx(T **__restrict ov, int vpn, T **out_lastv)
  {
    if (!pUsed || (pUsed + vpn) * pStride > data_size(vBuf))
      *out_lastv = NULL;
    else
    {
      *out_lastv = (T *)&vBuf[pUsed * pStride - pStride / 2];
      vpn++;
    }
    return fillRawVertPairsEx(ov, vpn);
  }

  void addVertPairs(const void *__restrict vertices, int v_pair_num, bool detached);
  void flush();
  void clear() { pUsed = 0; } //< clear prepared buffer without rendering

  VDECL getVDecl() const { return vdecl; }

private:
  SmallTab<uint8_t, InimemAlloc> vBuf;
  Ptr<ShaderElement> currentShader;
  RingDynamicVB *vb;
  VDECL vdecl;
  int pStride;
  int pUsed;

  void ctorInit()
  {
    vdecl = BAD_VDECL;
    pStride = 0;
    pUsed = 0;
    vb = NULL;
  }
};
