//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EditorCore/ec_interface_ex.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/utility.h>
#include <math/dag_e3dColor.h>

struct GPUGrassType
{
  eastl::string name;

  eastl::string diffuse;
  eastl::string normal;
  int variations = 0;
  float height = 1.0;
  float size_lod_mul = 1.3;
  float ht_rnd_add = 0.0;
  float hor_size = 1.0;
  float hor_size_rnd_add = 0.0;
  E3DCOLOR color_mask_r_from = 0;
  E3DCOLOR color_mask_r_to = 0;
  E3DCOLOR color_mask_g_from = 0;
  E3DCOLOR color_mask_g_to = 0;
  E3DCOLOR color_mask_b_from = 0;
  E3DCOLOR color_mask_b_to = 0;
  float height_from_weight_mul = 0.75;
  float height_from_weight_add = 0.25;
  float density_from_weight_mul = 0.999;
  float density_from_weight_add = 0.0;
  float vertical_angle_add = -0.1;
  float vertical_angle_mul = 0.2;

  float stiffness = 1.0;
  bool horizontal_grass = false;
  bool underwater = false;
};

struct GPUGrassDecal
{
  eastl::string name;

  int id = 0;
  eastl::vector<eastl::pair<eastl::string, float>> channels;
};

class IGPUGrassService : public IRenderingService
{
public:
  static constexpr unsigned HUID = 0x42424242u; // IGPUGrassService
  virtual void beforeRender(Stage stage) = 0;
  virtual void enableGrass(bool flag) = 0;
  virtual void createGrass(DataBlock &grassBlk) = 0;
  virtual void closeGrass() = 0;
  virtual DataBlock *createDefaultGrass() = 0;
  virtual BBox3 *getGrassBbox() = 0;

  virtual bool isGrassEnabled() const = 0;
  virtual int getTypeCount() const = 0;
  virtual GPUGrassType &getType(int index) = 0;
  virtual GPUGrassType &addType(const char *name) = 0;
  virtual void removeType(const char *name) = 0;
  virtual GPUGrassType *getTypeByName(const char *name) = 0;

  virtual int getDecalCount() const = 0;
  virtual GPUGrassDecal &getDecal(int index) = 0;
  virtual GPUGrassDecal &addDecal(const char *name) = 0;
  virtual void removeDecal(const char *name) = 0;
  virtual GPUGrassDecal *getDecalByName(const char *name) = 0;
  virtual bool findDecalId(int id) const = 0;

  virtual void invalidate() = 0;
};
