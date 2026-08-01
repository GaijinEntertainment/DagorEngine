//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <shaders/dag_shaderMesh.h>
#include "renderMesh.h"


namespace frx
{

struct MeshRenderList
{
  enum class PrepareState : uint8_t
  {
    NONE = 0,
    PREPARED, // prepare() done: vb/ib resolved and inst transforms offset; ready for a finalizeForXXX or simple render
    RI,
    DYNMODEL
  };

  struct RInst
  {
    TMatrix prevTm, tm;
    Point3 initialPos;
    PerInstRenderData instData;
  };
  struct RElem
  {
    ShaderMesh::Stage stage;
    int instId;
    ShaderElement *shElem;
    Sbuffer *vb, *ib;
    int ibOfs, vbOfs;
    int vStride, fCnt;
  };
  dag::Vector<RInst> insts;
  dag::Vector<RElem> elems;
  carray<int, ShaderMesh::STG_COUNT - 1> stageSpans;

  PrepareState curState = PrepareState::NONE;
  UniqueBuf perFrameDataBuf;

  void add(const RenderMesh &mesh, const TMatrix &tm, const TMatrix &prev_tm);
  void prepare();
  void finalizeForRi();
  void clear();

  // note: returns inclusive range of elements [from, to]
  dag::ConstSpan<RElem> getElems(ShaderMesh::Stage from, ShaderMesh::Stage to) const
  {
    G_ASSERT_RETURN(from <= to, {});
    G_ASSERT_RETURN(from < ShaderMesh::STG_COUNT && to < ShaderMesh::STG_COUNT, {});
    const int begin = int(from) == 0 ? 0 : stageSpans[int(from) - 1];
    const int end = int(to) == ShaderMesh::STG_COUNT - 1 ? elems.size() : stageSpans[int(to)];
    return make_span_const(elems.data() + begin, end - begin);
  }

  MeshRenderList() = default;
  ~MeshRenderList() { clear(); }
};

enum class RenderPass : uint8_t
{
  DEPTH = 0,
  OPAQUE, // renders both STG_opaque and STG_atest
  IMM_DECAL,
  DECAL,
  TRANSPARENT
};
void render_ri_meshes(const MeshRenderList &list, RenderPass pass);
void render_simple_dyn_meshes(const MeshRenderList &list, RenderPass render_pass);

} // namespace frx