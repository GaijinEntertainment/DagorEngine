//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <rendInst/impostor.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <math/dag_color.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_shaderVarsUtils.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <dag/dag_vector.h>
#include <drv/3d/dag_renderStateId.h>
#include <generic/dag_smallTab.h>
#include <3d/dag_multidrawContext.h>

namespace rendinst::render
{

extern int baked_impostor_multisampleVarId;
extern int vertical_impostor_slicesVarId;
extern int dynamicImpostorViewXVarId;
extern int dynamicImpostorViewYVarId;

class ImpostorRenderer final
{

private:
  struct Record final
  {
    ShaderElement *curShader;
    int curVar;
    uint32_t prog;
    ShaderStateBlockId state;
    shaders::RenderStateId rstate;
    shaders::TexStateIdx tstate;
    shaders::ConstStateIdx cstate;
    GlobalVertexData *vData;
    int si, numf, bv;
  };
  struct PackedDrawCallsRange final
  {
    uint32_t start;
    uint32_t count;
  };

  SmallTab<Record> multidrawList;
  dag::Vector<PackedDrawCallsRange> drawcallRanges;
  ska::flat_hash_map<shaders::ConstStateIdx, uint8_t> bindlessStatesToUpdateTexLevels;

  MultidrawContext<uint32_t> multidrawContext = {"rendinst_impostor_multidraw"};

  void sort_packed_drawcalls();
  void coalesce_packed_drawcalls();

public:
  void render(dag::Span<const ShaderMesh::RElem> elems);
  void close();
};

extern ImpostorRenderer impostorRenderer;

} // namespace rendinst::render

extern int impostorLastMipSize;

void render_impostors_ofs(int count, int start_ofs, int vectors_cnt);

void initImpostorsGlobals();
void termImpostorsGlobals();

__forceinline void get_up_left(Point3 &up, Point3 &left, const TMatrix &itm)
{
  Point3 forward = itm.getcol(2);
  if (abs(forward.y) < 0.999)
  {
    left = normalize(cross(Point3(0, 1, 0), forward));
    up = cross(forward, left);
  }
  else
  {
    left = itm.getcol(0);
    up = itm.getcol(1);
  }
}

__forceinline void set_up_left_to_shader(const TMatrix &itm)
{
  Point3 up, left;

  get_up_left(up, left, itm);

  ShaderGlobal::set_float4(rendinst::render::dynamicImpostorViewXVarId, Color4(left.x, left.y, left.z, 0));
  ShaderGlobal::set_float4(rendinst::render::dynamicImpostorViewYVarId, Color4(up.x, up.y, up.z, 0));
}
