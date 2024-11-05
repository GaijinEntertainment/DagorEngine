//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <terraform/terraform.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <3d/dag_resPtr.h>
#include <ska_hash_map/flat_hash_map2.hpp>

class TerraformRenderer : Terraform::Renderer
{
public:
  using Pcd = Terraform::Pcd;
  using PcdAlt = Terraform::PcdAlt;

  enum RenderMode : int
  {
    RENDER_MODE_COLOR = 0,
    RENDER_MODE_HEIGHT,
    RENDER_MODE_HEIGHT_MASK,
    RENDER_MODE_MASK
  };

  struct Desc
  {
    TEXTUREID diffuseTexId = BAD_TEXTUREID;
    TEXTUREID diffuseAboveTexId = BAD_TEXTUREID;
    TEXTUREID normalsTexId = BAD_TEXTUREID;
    TEXTUREID detailRTexId = BAD_TEXTUREID;

    Color4 aboveGroundColor = Color4(0.42f, 0.1764f, 0.04f, 0.5f);
    Color4 underGroundColor = Color4(0.14f, 0.06f, 0.0f, 0.5f);
    float digGroundAlpha = 1.0f;
    float digRoughness = 0.2f;
    Point2 textureTile = Point2(1, 1);
    float textureBlend = 1.0f;
    float textureBlendAbove = 1.0f;
    IPoint2 heightMaskSize = IPoint2(1024, 1024);
    Point2 heightMaskWorldSize = Point2(128.0f, 128.0f);
    float maxPhysicsError = 100.0f;
    float minPhysicsError = -100.0f;
  };

  TerraformRenderer(Terraform &in_tform, const Desc &in_desc);
  ~TerraformRenderer();

  void initHeightMask();
  void closeHeightMask();

  void update();
  void render(RenderMode render_mode, const BBox2 &region_box);
  void renderHeightMask(const Point2 &pivot);
  void invalidate();

  void drawDebug(const Point3 &pos, int mode);

  const Desc &getDesc() const { return desc; }
  const Terraform &getTForm() const { return tform; }
  const Point2 &getHeightMaskWorldSize() const { return heightMaskWorldSize; }

private:
  struct Patch;
  struct Prim;

  Patch *getPatch(int patch_no);
  void invalidatePatch(int patch_no);
  void deletePatches();

  void invalidatePrims();
  void deletePrims();

  Terraform &tform;
  Desc desc;
  Point2 heightMaskWorldSize = Point2(1, 1);
  DynamicShaderHelper patchShader;
  UniqueTexHolder hmapSavedTex;
  ska::flat_hash_map<uint32_t, Patch *> patches;
  ska::flat_hash_map<uint32_t, Prim *> prims;
  uint32_t curTimestep = 0;

  int terraform_tex_data_pivot_sizeVarId = -1;
  int terraform_min_max_levelVarId = -1;
  int terraform_render_modeVarId = -1;
  int terraform_tex_data_sizeVarId = -1;
  int terraform_enabledVarId = -1;
  int tform_height_mask_scale_offsetVarId = -1;

  int tform_diffuse_texVarId = -1;
  int tform_diffuse_above_texVarId = -1;
  int tform_detail_rtexVarId = -1;
  int tform_normals_texVarId = -1;

  UniqueTexHolder heightMaskTex;
};
