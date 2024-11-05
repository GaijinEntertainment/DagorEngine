//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_TMatrix4.h>
#include <math/dag_e3dColor.h>
#include <drv/3d/dag_resId.h>


class DynShaderMeshBuf;


enum
{
  FX_STD_ABLEND = 0, // DEPRICATED, USE PREMULTALPHA
  FX_STD_ADDITIVE,   // DEPRICATED, USE PREMULTALPHA
  FX_STD_ADDSMOOTH,
  FX_STD_ATEST,
  FX_STD_PREMULTALPHA,
  FX_STD_GBUFFERATEST,
  FX_STD_ATEST_REFRACTION,
  FX_STD_GBUFFER_PATCH,

  FX__NUM_STD_SHADERS
};

inline bool isFxShaderWithFakeHdr(int type)
{
  return type == FX_STD_ADDITIVE || type == FX_STD_ADDSMOOTH || type == FX_STD_PREMULTALPHA;
}


class StandardFxParticle
{
public:
  Point3 pos;
  Point3 uvec, vvec;
  E3DCOLOR color;
  Point2 tcu, tcv, tco, next_tco;
  float frameBlend = 0;
  float specularFade = 0;
  union
  {
    real sortOrder;
    int visMark;
  };


  void setFacing(const Point3 &xdir, const Point3 &ydir, real radius)
  {
    uvec = xdir * radius;
    vvec = ydir * (-radius);
  }

  void setFacingRotated(const Point3 &xdir, const Point3 &ydir, real radius, real angle)
  {
    real c, s;
    ::sincos(angle, s, c);
    c *= radius;
    s *= radius;

    uvec = xdir * c + ydir * s;
    vvec = xdir * s - ydir * c;
  }


  void setFullFrame()
  {
    tcu = Point2(1, 0);
    tcv = Point2(0, 1);
    tco = Point2(0, 0);
    next_tco = Point2(0, 0);
  }

  void setFrame(int u, int v, real du, real dv)
  {
    tcu = Point2(du, 0);
    tcv = Point2(0, dv);
    tco = Point2(u * du, v * dv);
    next_tco = Point2(u * du + du, v * dv);
    if (next_tco.x + du > 1)
      next_tco = Point2(0, v * dv + dv);
    if (next_tco.y + dv > 1)
      next_tco = Point2(0, 0);
  }


  bool isVisible() const { return visMark; }

  void hide() { visMark = 0; }

  void show() { visMark = 0x3F800000; /*=1.0f*/ }

  void calcSortOrder(const Point3 &fx_pos, const TMatrix4 &globtm)
  {
    float z = fx_pos.x * globtm(0, 2) + fx_pos.y * globtm(1, 2) + fx_pos.z * globtm(2, 2) + globtm(3, 2);
    sortOrder = fsel(z, z, 0);
  }
};


namespace EffectsInterface
{
struct StdParticleVertex
{
  Point3 pos;
  E3DCOLOR color;
  Point3 tc;
  Point3 next_tc_blend;
};

// init dynamic VB & IB for lockless rendering of FX quads and meshes;
// quad_num is estimated maximum number of quads rendered per frame (most fx render quads)
// mesh_faces is estimated maximum number of faces rendered per frame (some fx render meshes as TRILIST)
void initDynamicBuffers(int quad_num = 4096, int mesh_faces = 1024);


void startup();
void shutdown();

void setColorScale(real scale);

void setD3dWtm(const TMatrix &tm);

int registerStdParticleCustomShader(const char *name, bool optional = false);
void setStdParticleCustomShader(int id);

void setStdParticleShader(int shader_id);
void setStd2dParticleShader(int shader_id);
void setStdParticleTexture(TEXTUREID tex_id, int slot = 0);
void setStdParticleNormalMap(TEXTUREID tex_id, int shading_type);

void setLightingParams(real lighting_power, real sun_power, real amb_power);

void renderStdFlatParticles(StandardFxParticle *parts, int count, int *sort_indices);
void renderBatchFlatParticles(StandardFxParticle *parts, int count, int *sort_indices);

real getColorScale();

// should be called before rendering FX quads of each frame to reset positions in dynamic buffers
// (unless target d3d driver natively supports lockless ring buffers)
// returns current buffer position (in quads); it can be slightly greater than real number of rendered quads
// when one or more rounds occured in last frame
int resetPerFrameDynamicBufferPos();

void startStdRender();
void endStdRender();
DynShaderMeshBuf *getStdRenderBuffer();

int getMaxQuadCount();
void beginQuads(StdParticleVertex **ptr, int num_quads);
void endQuads();
}; // namespace EffectsInterface
