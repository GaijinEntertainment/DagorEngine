// Copyright (C) Gaijin Games KFT.  All rights reserved.

#pragma once

#include <EASTL/vector.h>
#include <math/dag_Point2.h>
#include <math/dag_Point4.h>
#include <3d/dag_resPtr.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>

#define LENS_FLARE_VARS_LIST               \
  VAR(lens_flare_texture)                  \
  VAR(lens_flare_prepare_num_flares)       \
  VAR(lens_flare_prepare_flares_offset)    \
  VAR(lens_flare_light_source_id)          \
  VAR(lens_flare_prepare_camera_pos)       \
  VAR(lens_flare_prepare_is_sun)           \
  VAR(lens_flare_rounding_type)            \
  VAR(lens_flare_prepare_use_occlusion)    \
  VAR(lens_flare_has_fom_shadows)          \
  VAR(lens_flare_has_volfog)               \
  VAR(lens_flare_prepare_sun_color)        \
  VAR(lens_flare_prepare_fadeout_distance) \
  VAR(lens_flare_prepare_sun_screen_tc)    \
  VAR(lens_flare_resolution)               \
  VAR(lens_flare_global_scale)

#define VAR(a) extern ShaderVariableInfo a##VarId;
LENS_FLARE_VARS_LIST
#undef VAR

class ComputeShaderElement;
struct LensFlareVertex;
struct LensFlareInfo;

struct LensFlareConfig
{
  struct Gradient
  {
    float falloff = 1;     // curve of the fadeout effect: intensity = pow(saturate(edgeDistance/gradient), 1/falloff)
    float gradient = 0.1f; // distance of the fadeout effect (1=radius of the shape)
    bool inverted = false;
  };
  struct RadialDistortion
  {
    bool enabled = false;
    bool relativeToCenter = false;        // if not, it's relative to the light's source
    float distortionCurvePow = 1;         // distortion magnitude = pow(distance, distortionCurvePow)
    Point2 radialEdgeSize = Point2(1, 1); // size of the flare as it reaches the screen's edge = most extreme size (smallest or
                                          // largest) that it gets due to the distortion
  };
  struct Element
  {
    Gradient gradient;
    RadialDistortion radialDistortion;
    Point2 scale = Point2(1, 1);
    Point2 offset = Point2(0, 0);
    float preRotationOffset = 0; // applied before scaling
    float rotationOffset = 0;
    float axisOffset = 0; // 0 = light source; 0.5 = screen center
    bool autoRotation = false;
    float intensity = 1;
    eastl::string texture;
    float roundness = 0;
    int sideCount = 4;
    Point3 tint = Point3(1, 1, 1);
    bool useLightColor = true;
  };

  eastl::string name;
  float smoothScreenFadeoutDistance = 0;
  Point2 scale = Point2(1, 1);
  float intensity = 1;
  bool useOcclusion = true;
  eastl::vector<Element> elements;
};

class LensFlareRenderer
{
public:
  struct CachedFlareId
  {
    int cacheId = -1;
    int flareConfigId = -1;

    [[nodiscard]] bool isValid() const { return flareConfigId >= 0; }
    operator bool() const { return isValid(); }
  };

  LensFlareRenderer();
  ~LensFlareRenderer();

  void init();
  void markConfigsDirty();
  void startPreparingLights();
  void prepareFlare(const TMatrix4 &view_proj,
    const Point3 &camera_pos,
    const Point3 &camera_dir,
    const CachedFlareId &cached_flare_config_id,
    const Point4 &light_pos,
    const Point3 &color,
    bool is_sun);
  void collectAndPrepareECSSunFlares(
    const TMatrix4 &view_proj, const Point3 &camera_pos, const Point3 &camera_dir, const Point3 &sun_dir, const Point3 &color);
  bool endPreparingLights(const Point3 &camera_pos);
  void render(const Point2 &resolution) const;

  [[nodiscard]] bool isCachedFlareIdValid(const CachedFlareId &id) const;
  CachedFlareId cacheFlareId(const char *flare_config_name) const;

private:
  enum class RoundingType : int
  {
    SHARP = 0,
    ROUNDED = 1,
    CIRCLE = 2
  };

  struct LensFlareRenderBlock
  {
    int indexOffset = 0;
    int numTriangles = 0;
    SharedTex texture;
    RoundingType roundingType = RoundingType::SHARP;
  };

  class LensFlareData
  {
  public:
    struct Params
    {
      float smoothScreenFadeoutDistance;
      bool useOcclusion;
    };

    explicit LensFlareData(eastl::string config_name, const Params &params);
    LensFlareData(LensFlareData &&) = default;
    LensFlareData &operator=(LensFlareData &&) = default;

    static LensFlareData parse_lens_flare_config(const LensFlareConfig &config,
      eastl::vector<uint16_t> &ib,
      eastl::vector<LensFlareVertex> &vb,
      eastl::vector<LensFlareInfo> &flares,
      eastl::vector<Point2> &vpos);

    [[nodiscard]] const eastl::vector<LensFlareRenderBlock> &getRenderBlocks() const;
    [[nodiscard]] const eastl::string &getConfigName() const { return configName; }
    [[nodiscard]] const Params &getParams() const { return params; }

  private:
    eastl::string configName;
    Params params;
    eastl::vector<LensFlareRenderBlock> renderBlocks;
  };

  struct PreparedLight
  {
    Point3 lightColor;
    Point2 lightPos;
    int flareConfigId;
    bool isSun;
  };

  Ptr<ComputeShaderElement> prepareFlaresShader;
  Ptr<ShaderMaterial> lensShaderMaterial;
  dynrender::RElem lensShaderElement;
  // TODO increase this later to allow many point lights later
  int maxNumPreparedLights = 2;

  eastl::vector<LensFlareData> lensFlares;
  UniqueBufHolder lensFlareBuf;
  UniqueBufHolder preparedLightsBuf;
  UniqueBufHolder vertexPositionsBuf;
  UniqueBuf flareVB;
  UniqueBuf flareIB;
  int numTotalVertices = 0;
  int currentConfigCacheId = 0;
  bool isDirty = true;

  eastl::vector<PreparedLight> preparedLights;

  void prepareConfigBuffers(const eastl::vector<LensFlareConfig> &configs);
  void updateConfigsFromECS();
};

ECS_DECLARE_BOXED_TYPE(LensFlareRenderer);
