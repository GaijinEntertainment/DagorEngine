//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMath.h>
#include <EASTL/unique_ptr.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_postFxRenderer.h>

namespace build_bvh
{
struct SplitHelper;
struct BLASData;
struct TLASData;
void destroy(BLASData *&);
void destroy(TLASData *&);
} // namespace build_bvh
class ComputeShaderElement;

struct RenderSWRT
{
  void init();
  void close();
  ~RenderSWRT();
  int addBoxModel(vec4f bmin, vec4f bmax);
  // if threshold_to_dim_as_box > 0, shadow rays would assume the model is it's bounding box, if ray originate farther than
  // threshold_to_dim_as_box*boxSize
  int addModel(const uint16_t *indices, int index_count, const Point3_vec4 *vertices, int vertex_count,
    float threshold_to_dim_as_box = 0);
  void addToLastModel(uint16_t const *indices, int index_count, const Point3_vec4 *vertices, int vertex_count);

  inline void addInstance(int model_index, const mat43f &tm)
  {
    G_ASSERT(uint32_t(model_index) < max(blasStartOffsets.size(), models.size()));
    instances.push_back(model_index);
    matrices.push_back(tm);
  }
  build_bvh::BLASData *buildBottomLevelStructures(uint32_t workers);
  build_bvh::TLASData *buildTopLevelStructures();
  void copyToGPUAndDestroy(build_bvh::BLASData *);
  void copyToGPUAndDestroy(build_bvh::TLASData *);
  static void destroy(build_bvh::BLASData *&);
  static void destroy(build_bvh::TLASData *&);

  void drawRT();
  void reserveAddInstances(int cnt);
  uint32_t currentInstancesCount() const { return instances.size(); }
  void purgeMeshData()
  {
    ibd.clear();
    ibd.shrink_to_fit();
    vbd.clear();
    vbd.shrink_to_fit();
  }
  void clearTLASSourceData();
  void clearBLASSourceData();
  void clearTLASBuffers();
  void clearBLASBuffers();
  void clearBLASBuiltData();

  static uint32_t getTileBufferSizeDwords(uint32_t w, uint32_t h);
  enum
  {
    NO_CHECKER_BOARD = 0
  };
  void renderShadows(const Point3 &dir_to_sun, float sun_size, uint32_t checkerboad_frame, const ManagedBufView &shadow_mask_buf,
    const ManagedTexView &shadow_target);
  bool hasData() const { return bool(topBuf); }

protected:
  ComputeShaderElement *checkerboard_shadows_swrt_cs = nullptr, *shadows_swrt_cs = nullptr, *mask_shadow_swrt_cs = nullptr;
  void renderShadowFrustumTiles(int w, int h, const Point3 &dir_to_sun, float sun_size);
  UniqueBufHolder bottomBuf;
  UniqueBufHolder topBuf;
  UniqueBufHolder topLeavesBuf;

  dag::Vector<uint16_t> ibd;
  dag::Vector<vec4f> vbd;
  struct Info
  {
    enum
    {
      FACES_IS_BOX = 0
    };
    int faces = 0, verts = 0;
    int ibO = 0, vbO = 0;
  };
  dag::Vector<Info> models;
  dag::Vector<int> blasStartOffsets;
  dag::Vector<uint16_t> blasDimAsBox;
  dag::Vector<bbox3f> blasBoxes;

  dag::Vector<int> tlasLeavesOffsets;
  dag::Vector<mat43f> matrices;
  dag::Vector<int> instances;
  eastl::unique_ptr<ShaderMaterial> mat;
  ShaderElement *elem = 0;
  PostFxRenderer rt;
  bool inited = false;

  void build_tlas(dag::Vector<bbox3f> &tlas_boxes, const dag::Vector<bbox3f> &blas_boxes);
  bool buildModel(int model, build_bvh::SplitHelper &h, dag::Vector<bbox3f> &tempBoxes, int &root);
};