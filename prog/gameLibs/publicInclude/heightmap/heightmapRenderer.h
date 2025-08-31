//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <math/dag_Point4.h>
#include <heightmap/lodGrid.h>

class LodGrid;
struct LodGridCullData;
struct Frustum;
class ShaderMaterial;
class ShaderElement;
class HeightmapHeightCulling;
class Point2;
struct LodGridVertexData;
union LodGridPatchParams;

class HeightmapRenderer
{
public:
  bool init(const char *shader_name, const char *mat_script, const char *hmap_tess_factor_name, bool do_fatal,
    int dimBits = default_patch_bits);
  bool isInited() const { return shElem != NULL; }
  void setRenderClip(const BBox2 *clip) const; // no face will be render out of this box
  void render(const LodGrid &lodGrid, const LodGridCullData &cull_data, LodGridVertexData *vData = nullptr, int vDataDim = -1) const;
  void renderOnePatch(const Point2 &world_lt, const Point2 &world_rb) const; // no tesselation, render whole area
  void renderEmpty() const;                                                  // to flush states on dx9
  void renderPatchesByBatches(dag::ConstSpan<LodGridPatchParams> patches, const int buffer_size, int vDataIndex, int startFlipped,
    bool render_quads, int primitiveCount, int startInd = 0) const;
  int get_hmap_tess_factorVarId() const { return hmap_tess_factorVarId; }
  int getDim() const { return 1 << dimBits; }
  int getDimBits() const { return dimBits; }
  HeightmapRenderer(int dimBits = default_patch_bits);
  ~HeightmapRenderer() { close(); }
  void close();

  static void beforeResetDevice();
  static void afterResetDevice();

protected:
  ShaderMaterial *shmat;
  ShaderElement *shElem;
  int dimBits;
  int hmap_tess_factorVarId = -1;
};
