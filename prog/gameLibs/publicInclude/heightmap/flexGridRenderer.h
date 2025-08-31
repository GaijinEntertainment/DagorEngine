//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <math/dag_Point2.h>
#include <math/dag_Point4.h>
#include <math/integer/dag_IBBox2.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <3d/dag_resPtr.h>
#include <heightmap/flexGridSubdivision.h>

class ShaderMaterial;
class ShaderElement;

class FlexGridRenderer
{
public:
  bool isInited() const;
  bool init(const FlexGridConfig &cfg, bool do_fatal);
  void createPatchBuffers();

  void prepareSubdivisionForHeightmap(const HeightmapHandler &hmap_handler, bool force_rebuild = false);
  bool isSubdivisionForHeightmapPrepared() const;

  void prepareSubdivision(int flex_grid_subdiv_id, const Point3 &subdiv_camera_pos, const TMatrix4 &subdiv_camera_view,
    const TMatrix4 &subdiv_camera_proj, const Frustum &view_frustum, const HeightmapHandler &hmap_handler, const Occlusion *occlusion,
    int tess_quality, bool force_low_quality, const FlexGridConfig &cfg);
  void render(int flex_grid_subdiv_id, const FlexGridConfig &cfg, bool log_debug);
  void renderNonIndexed(int flex_grid_subdiv_uid, const FlexGridConfig &cfg, bool log_debug);

  void close();
  ~FlexGridRenderer() { close(); }

private:
  struct VertexData
  {
    E3DCOLOR posEdgeShiftDirection = 0;
    E3DCOLOR edgeShiftMask = 0;
    E3DCOLOR averageShiftPrevLod = 0;
    E3DCOLOR data1 = 0;
  };
  struct InstanceData
  {
    Point2 patchOffset;
    float patchScale = 0.f;
    float patchMorph = 0.f;

    Point4 neighborLodOffset;
    Point4 neighborPatchMorph;

    uint32_t edgesDirectionPacked1 = 0;
    uint32_t edgesDirectionPacked2 = 0;
    uint32_t parentEdgeDataOffset = 0;
    uint32_t parentEdgeDataScale = 0; // can be packed into parentEdgeDataOffsetm but now dummy is needed for 16-align anyway..
  };

  bool initMaterial(const char *shader_name, const char *mat_script, bool do_fatal);

  const FlexGridSubdivision::Patch *findLastTerminatedPatch(const FlexGridSubdivision::Patch *subdiv_root, uint32_t level, uint32_t x,
    uint32_t y);

  void processTerminatedPatch(const FlexGridSubdivision::Patch *subdiv_root, const FlexGridSubdivision::Patch *patch,
    InstanceData *instance_buffer_data);
  void processSubdivisionHierarchically(const FlexGridSubdivision::Patch *subdiv_root, const FlexGridSubdivision::Patch *patch,
    InstanceData *&instance_buffer_data, uint32_t *instance_count, uint32_t *total_instance_count, const FlexGridConfig &cfg);

  void updateHeightInfoRecursive(const HeightmapHandler &hmap_handler, FlexGridSubdivisionAsset *subdiv, uint32_t level, uint32_t x,
    uint32_t y, FlexGridSubdivisionAsset::HeightRange *parent_height_range, FlexGridSubdivisionAsset::HeightError *parent_height_error,
    const IBBox2 &rect) const;

  // material
  ShaderMaterial *shmat = nullptr;
  ShaderElement *shElem = nullptr;

  // runtime render
  UniqueBuf vb;
  UniqueBuf ib;
  UniqueBuf vb_nonindexed;

  dag::Vector<FlexGridRenderer::InstanceData> instanceData;
  uint32_t patchTriangleCount = 0;

  // grid
  FlexGridSubdivision subdivision;
  FlexGridSubdivisionAsset heightfieldSubdivAsset;

  // common
  bool initialized = false;

  // debug
  int debugFrameCounter = 0;
  uint32_t debugMaxTris = 0;
};