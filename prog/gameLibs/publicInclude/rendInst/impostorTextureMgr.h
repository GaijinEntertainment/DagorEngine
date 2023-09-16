//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_resPtr.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_color.h>
#include <math/integer/dag_IPoint4.h>
#include <math/integer/dag_IPoint3.h>
#include <shaders/dag_overrideStateId.h>
#include <shaders/dag_postFxRenderer.h>
#include <util/dag_stdint.h>

#include <EASTL/unique_ptr.h>

class DataBlock;
class RenderableInstanceLodsResource;
class BcCompressor;
class DeferredRenderTarget;

struct ImpostorTextureQualityLevel
{
  float minHeight; // in world space
  int textureHeight;
};

class ImpostorTextureManager
{
public:
  struct GenerationData
  {
    // Change version number if this struct changes
    // default values are used for missing impostor params
    // changing default values don't trigger impostor generation
    static constexpr uint32_t VERSION_NUMBER = 9;
    uint32_t textureWidth = 0;
    uint32_t textureHeight = 0;
    uint32_t mipCount = 0;
    uint32_t verticalSamples = 1;
    uint32_t horizontalSamples = 9;
    IPoint3 mipOffsets_hq_mq_lq = IPoint3(0, 1, 2);
    IPoint3 mobileMipOffsets_hq_mq_lq = IPoint3(0, 1, 2);
    bool preshadowEnabled = true;
    bool allowParallax = true;
    bool allowTriView = true;
    bool octahedralImpostor = false;
    uint32_t rotationPaletteSize = 3;
    Point3 tiltLimit = Point3(5, 5, 5) * DEG_TO_RAD;
  };

  struct ShadowGenInfo
  {
    Point3 sunDir;
    Color4 impostor_shadow_x;
    Color4 impostor_shadow_y;
    Color4 impostor_shadow_z;
    TEXTUREID impostor_shadowID;
  };

  ImpostorTextureManager();
  ~ImpostorTextureManager();

  ImpostorTextureManager(const ImpostorTextureManager &) = delete;
  ImpostorTextureManager &operator=(const ImpostorTextureManager &) = delete;

  static GenerationData load_data(const DataBlock *props, RenderableInstanceLodsResource *res, int smallest_mip,
    bool default_preshadows_enabled, const IPoint3 &default_mip_offsets_hq_mq_lq, const IPoint3 &default_mobile_mip_offsets_hq_mq_lq,
    int num_levels, const ImpostorTextureQualityLevel *sorted_levels);
  static Point2 get_scale(RenderableInstanceLodsResource *res);
  static float get_vertex_scaling(float cos_phi);
  static Point3 get_point_to_eye_octahedral(uint32_t h, uint32_t v, unsigned int horizontalSamples, unsigned int verticalSamples);
  static Point3 get_point_to_eye_octahedral(uint32_t h, uint32_t v, const GenerationData &gen_data);
  static Point3 get_point_to_eye_billboard(uint32_t sliceId);

  void generate_mask(const Point3 &point_to_eye, RenderableInstanceLodsResource *res, DeferredRenderTarget *rt, Texture *mask_tex);
  void render_slice_octahedral(uint32_t h, uint32_t v, const TMatrix &view_to_content, const GenerationData &gen_data,
    RenderableInstanceLodsResource *res, int block_id);
  void render_slice_billboard(uint32_t sliceId, const TMatrix &view_to_content, RenderableInstanceLodsResource *res, int block_id);
  void generate_mask_billboard(uint32_t sliceId, RenderableInstanceLodsResource *res, DeferredRenderTarget *rt, Texture *mask_tex);
  void generate_mask_octahedral(uint32_t h, uint32_t v, const GenerationData &gen_data, RenderableInstanceLodsResource *res,
    DeferredRenderTarget *rt, Texture *mask_tex);
  void start_rendering_slices(DeferredRenderTarget *rt);
  void end_rendering_slices();

  UniqueTex renderDepthAtlasForShadow(RenderableInstanceLodsResource *res);
  bool update_shadow(RenderableInstanceLodsResource *res, const ShadowGenInfo &info, int layer_id, int ri_id, int palette_id,
    const UniqueTex &impostorDepthBuffer);

  bool hasBcCompression() const;
  int getPreferredShadowAtlasMipOffset() const;

  void setTreeCrownDataBuf(Sbuffer *buf) { treeCrownDataBuf = buf; }

private:
  shaders::UniqueOverrideStateId impostorShaderState;
  PostFxRenderer impostorMaskShader;
  PostFxRenderer impostorShadowShader;

  eastl::unique_ptr<BcCompressor> shadowAtlasCompressor;
  UniqueTex impostorCompressionBuffer;
  Sbuffer *treeCrownDataBuf = nullptr;

  void render(const Point3 &point_to_eye, const TMatrix &post_view_tm, RenderableInstanceLodsResource *res, int block_id) const;

  static bool prefer_bc_compression();

  friend void impostor_texture_mgr_after_reset(bool);
};

void init_impostor_texture_mgr();
void close_impostor_texture_mgr();
ImpostorTextureManager *get_impostor_texture_mgr();
