//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <generic/dag_tab.h>
#include <render/dynmodelRenderer.h>
#include <3d/dag_textureIDHolder.h>
#include <math/dag_hlsl_floatx.h>
#include <render/decals/planar_decals_params.hlsli>

class DataBlock;

// Decals for dynmodels. Includes planar decals (permanent decorations) and dynamic (such as damage holes).
namespace dyn_decals
{

const int num_decals = 4;
struct DECLSPEC_ALIGN(16) DecalsData
{
  DecalsData(const DecalsData &from);
  ~DecalsData();
  DecalsData &operator=(const DecalsData &) = delete;
  bool operator==(const DecalsData &other) const;

  const TextureIDPair &getTexture(int slot_no) const { return decalTextures[slot_no]; }
  dag::ConstSpan<TextureIDPair> getTextures() const { return dag::ConstSpan<TextureIDPair>(decalTextures, num_decals); }
  void setTexture(int slot_no, const TextureIDPair &tex);

  Point4 decalLines[4 * num_decals];
  Point4 contactNormal[2 * num_decals];
  float widthBox[num_decals];
  bool wrap[num_decals];
  bool absDot[num_decals];
  bool oppositeMirrored[num_decals];

protected:
  DecalsData() {} //-V730
  TextureIDPair decalTextures[num_decals];

} ATTRIBUTE_ALIGN(16);

struct PlanePrim
{
  Point4 plane = Point4(0, 0, 0, 1);
  int material = 0;
};

struct CapsulePrim
{
  Point3 bot = Point3(0, 0, 0);
  float radius = 0;
  Point3 top = Point3(0, 0, 0);
  uint32_t isExplodedMaterial = 0; // bitField
};

struct SpherePrim
{
  Point4 sphere = Point4(0, 0, 0, 0);
};

struct BurnMarkPrim
{
  Point4 sphere = Point4(0, 0, 0, 0);
};

struct DynDecals
{
  int cuttingPlaneWingOrMode;
  dag::ConstSpan<PlanePrim> planes;
  dag::ConstSpan<CapsulePrim> capsules;
  dag::ConstSpan<SpherePrim> spheres;
  dag::ConstSpan<BurnMarkPrim> burnMarks;
};

void init(const DataBlock &blk);
void close();
void add_to_atlas(TEXTUREID texId);
void remove_from_atlas(TEXTUREID texId);
void add_to_atlas(dag::ConstSpan<TextureIDPair> textures);
void remove_from_atlas(dag::ConstSpan<TextureIDPair> textures);
int allocate_buffer(); // Returns id of decal params set.
void update_buffer(int params_id, const DecalsData &decals);
void remove_from_buffer(int decals_param_set_id);
void after_reset_device();
void clear_atlas();
Point4 get_atlas_uv(TEXTUREID texId);
bool is_pending_update(TEXTUREID texId);
bool are_decals_ready(const DecalsData &decals);
PlanarDecalsParamsSet construct_decal_params(const DecalsData &decals);
int get_valid_decal_count(const DecalsData &decals);
void set_planar_decals(int params_id, dynrend::PerInstanceRenderData &render_data);
void set_dyn_decals(const DynDecals &decals, Tab<Point4> &out_params);
void set_dyn_decals(const DynDecals &decals, int start_params, dynrend::PerInstanceRenderData &render_data);
bool update();

// to use uncomment DYNAMIC_DECALS_TWEAKING in dynamic_decals_params.hlsli
bool dyn_decals_console(int found, const char *argv[], int argc);
} // namespace dyn_decals
