//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_buffers.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <perfMon/dag_statDrv.h>
#include <vecmath/dag_vecMath.h>
#include <EASTL/vector_multimap.h>

namespace bvh
{

struct BufferProcessor
{
  static const uint32_t bvhAttributeShort2TC = 25 << 16;

  BufferProcessor(const char *name) : shader(name ? new_compute_shader(name) : nullptr) {}

  struct ProcessArgs
  {
    uint64_t meshId;

    uint32_t indexStart;
    uint32_t indexCount;
    uint32_t indexFormat;
    uint32_t baseVertex;
    uint32_t startVertex;
    uint32_t vertexCount;
    uint32_t vertexStride;
    uint32_t positionFormat;
    uint32_t positionOffset;
    uint32_t texcoordOffset;
    uint32_t texcoordFormat;
    uint32_t secTexcoordOffset;
    uint32_t secTexcoordFormat;
    uint32_t normalOffset;
    uint32_t colorOffset;
    uint32_t indicesOffset;
    uint32_t weightsOffset;
    uint32_t outputOffset;
    Point4 posMul;
    Point4 posAdd;
    TEXTUREID texture;
    uint32_t textureLevel;
    mat43f worldTm;
    TMatrix4 invWorldTm;
    eastl::function<void()> setTransformsFn;
    eastl::function<void(Point4 &, Point4 &)> getHeliParamsFn;
    eastl::function<void(float &, Point2 &)> getDeformParamsFn;
    TreeData tree;
    FlagData flag;

    float impostorHeightOffset;
    Point4 impostorScale;
    Point4 impostorSliceTm1;
    Point4 impostorSliceTm2;
    Point4 impostorSliceClippingLines1;
    Point4 impostorSliceClippingLines2;
    Point4 impostorOffsets[4];

    bool recycled;
  };

  virtual ~BufferProcessor() = default;
  virtual bool isOneTimeOnly() const = 0;
  virtual bool isReady(const ProcessArgs &) const { return true; }
  virtual bool isGeneratingSecondaryVertices() const { return false; }
  virtual bool process(ContextId context_id, Sbuffer *source, UniqueOrReferencedBVHBuffer &processed_buffer, ProcessArgs &args,
    bool skip_processing) const = 0;

  eastl::unique_ptr<ComputeShaderElement> shader;
};

struct IndexProcessor
{
  IndexProcessor() : shader(new_compute_shader("bvh_process_dynrend_indices")) {}

  void process(Sbuffer *source, UniqueBVHBufferWithOffset &processed_buffer, int index_format, int index_count, int index_start,
    int start_vertex);

  eastl::unique_ptr<ComputeShaderElement> shader;
  UniqueBVHBuffer outputs[32];
  int ringCursor = 0;
};

struct SkinnedVertexProcessor : public BufferProcessor
{
  mutable uint32_t counter = 0;

  SkinnedVertexProcessor() : BufferProcessor("bvh_process_skinned_vertices") {}

  bool isOneTimeOnly() const override { return false; }

  bool process(ContextId context_id, Sbuffer *source, UniqueOrReferencedBVHBuffer &processed_buffer, ProcessArgs &args,
    bool skip_processing) const override;
};

struct TreeVertexProcessor : public BufferProcessor
{
  mutable uint32_t counter = 0;

  TreeVertexProcessor() : BufferProcessor("bvh_process_tree_vertices") {}

  bool isOneTimeOnly() const override { return false; }

  bool process(ContextId context_id, Sbuffer *source, UniqueOrReferencedBVHBuffer &processed_buffer, ProcessArgs &args,
    bool skip_processing) const override;
};

struct LeavesVertexProcessor : public BufferProcessor
{
  mutable uint32_t counter = 0;

  LeavesVertexProcessor() : BufferProcessor("bvh_process_leaves_vertices") {}

  bool isOneTimeOnly() const override { return false; }

  bool process(ContextId context_id, Sbuffer *source, UniqueOrReferencedBVHBuffer &processed_buffer, ProcessArgs &args,
    bool skip_processing) const override;
};

struct HeliRotorVertexProcessor : public BufferProcessor
{
  mutable uint32_t counter = 0;

  HeliRotorVertexProcessor() : BufferProcessor("bvh_process_heli_rotor_vertices") {}

  bool isOneTimeOnly() const override { return false; }

  bool process(ContextId context_id, Sbuffer *source, UniqueOrReferencedBVHBuffer &processed_buffer, ProcessArgs &args,
    bool skip_processing) const override;
};

struct FlagVertexProcessor : public BufferProcessor
{
  mutable uint32_t counter = 0;

  FlagVertexProcessor() : BufferProcessor("bvh_process_flag_vertices") {}

  bool isOneTimeOnly() const override { return false; }

  bool process(ContextId context_id, Sbuffer *source, UniqueOrReferencedBVHBuffer &processed_buffer, ProcessArgs &args,
    bool skip_processing) const override;
};

struct DeformedVertexProcessor : public BufferProcessor
{
  mutable uint32_t counter = 0;

  DeformedVertexProcessor() : BufferProcessor("bvh_process_deformed_vertices") {}

  bool isOneTimeOnly() const override { return false; }

  bool process(ContextId context_id, Sbuffer *source, UniqueOrReferencedBVHBuffer &processed_buffer, ProcessArgs &args,
    bool skip_processing) const override;
};

struct ImpostorVertexProcessor : public BufferProcessor
{
  mutable uint32_t counter = 0;

  ImpostorVertexProcessor() : BufferProcessor("bvh_process_impostor_vertices") {}

  bool isOneTimeOnly() const override { return true; }

  bool isGeneratingSecondaryVertices() const override { return true; }

  bool process(ContextId context_id, Sbuffer *source, UniqueOrReferencedBVHBuffer &processed_buffer, ProcessArgs &args,
    bool skip_processing) const override;
};

struct BakeTextureToVerticesProcessor : public BufferProcessor
{
  mutable uint32_t counter = 0;

  BakeTextureToVerticesProcessor() : BufferProcessor("bvh_process_bake_texture_to_vertices") {}

  bool isOneTimeOnly() const override { return true; }

  bool isReady(const ProcessArgs &args) const override
  {
    return args.texture == BAD_TEXTUREID || get_managed_res_loaded_lev(args.texture) >= args.textureLevel;
  }

  bool process(ContextId context_id, Sbuffer *source, UniqueOrReferencedBVHBuffer &processed_buffer, ProcessArgs &args,
    bool skip_processing) const override;
};

struct ProcessorInstances
{
  static IndexProcessor &getIndexProcessor()
  {
    if (!dynrendIndex)
      dynrendIndex.reset(new IndexProcessor());
    return *dynrendIndex;
  }

  static const BufferProcessor &getSkinnedVertexProcessor()
  {
    if (!skinnedVertex)
      skinnedVertex.reset(new SkinnedVertexProcessor());
    return *skinnedVertex;
  }

  static const BufferProcessor &getTreeVertexProcessor()
  {
    if (!treeVertex)
      treeVertex.reset(new TreeVertexProcessor());
    return *treeVertex;
  }

  static const BufferProcessor &getLeavesVertexProcessor()
  {
    if (!leavesVertex)
      leavesVertex.reset(new LeavesVertexProcessor());
    return *leavesVertex;
  }

  static const BufferProcessor &getHeliRotorVertexProcessor()
  {
    if (!heliRotorVertex)
      heliRotorVertex.reset(new HeliRotorVertexProcessor());
    return *heliRotorVertex;
  }

  static const BufferProcessor &getFlagVertexProcessor()
  {
    if (!flagVertex)
      flagVertex.reset(new FlagVertexProcessor());
    return *flagVertex;
  }

  static const BufferProcessor &getDeformedVertexProcessor()
  {
    if (!deformedVertex)
      deformedVertex.reset(new DeformedVertexProcessor());
    return *deformedVertex;
  }

  static const BufferProcessor &getImpostorVertexProcessor()
  {
    if (!impostorVertex)
      impostorVertex.reset(new ImpostorVertexProcessor());
    return *impostorVertex;
  }

  static const BufferProcessor &getBakeTextureToVerticesProcessor()
  {
    if (!bakeTexture)
      bakeTexture.reset(new BakeTextureToVerticesProcessor());
    return *bakeTexture;
  }

  static void teardown()
  {
    dynrendIndex.reset();
    skinnedVertex.reset();
    treeVertex.reset();
    leavesVertex.reset();
    heliRotorVertex.reset();
    flagVertex.reset();
    deformedVertex.reset();
    impostorVertex.reset();
    bakeTexture.reset();
  }

private:
  static inline eastl::unique_ptr<IndexProcessor> dynrendIndex = {};
  static inline eastl::unique_ptr<SkinnedVertexProcessor> skinnedVertex = {};
  static inline eastl::unique_ptr<TreeVertexProcessor> treeVertex = {};
  static inline eastl::unique_ptr<LeavesVertexProcessor> leavesVertex = {};
  static inline eastl::unique_ptr<HeliRotorVertexProcessor> heliRotorVertex = {};
  static inline eastl::unique_ptr<FlagVertexProcessor> flagVertex = {};
  static inline eastl::unique_ptr<DeformedVertexProcessor> deformedVertex = {};
  static inline eastl::unique_ptr<ImpostorVertexProcessor> impostorVertex = {};
  static inline eastl::unique_ptr<BakeTextureToVerticesProcessor> bakeTexture = {};
};

} // namespace bvh
