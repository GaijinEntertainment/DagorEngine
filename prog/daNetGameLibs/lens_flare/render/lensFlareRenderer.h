// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/vector.h>
#include <EASTL/vector_set.h>
#include <math/dag_Point2.h>
#include <math/dag_Point4.h>
#include <3d/dag_resPtr.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>

#define LENS_FLARE_VARS_LIST                                \
  VAR(lens_flare_texture)                                   \
  VAR(lens_flare_prepare_num_manual_flares)                 \
  VAR(lens_flare_prepare_num_omni_light_flares)             \
  VAR(lens_flare_prepare_num_spot_light_flares)             \
  VAR(lens_flare_prepare_camera_pos)                        \
  VAR(lens_flare_prepare_camera_dir)                        \
  VAR(lens_flare_rounding_type)                             \
  VAR(lens_flare_prepare_has_fom_shadows)                   \
  VAR(lens_flare_resolution)                                \
  VAR(lens_flare_prepare_flare_type)                        \
  VAR(lens_flare_prepare_max_num_instance)                  \
  VAR(lens_flare_prepare_dynamic_lights_fadeout_distance)   \
  VAR(lens_flare_prepare_dynamic_lights_use_occlusion)      \
  VAR(lens_flare_prepare_dynamic_lights_exposure_pow_param) \
  VAR(lens_flare_prepare_indirect_draw_buf)                 \
  VAR(lens_flare_prepare_indirect_dispatch_buf)             \
  VAR(lens_flare_prepare_far_depth_mip)                     \
  VAR(lens_flare_global_scale)

#define VAR(a) extern ShaderVariableInfo a##VarId;
LENS_FLARE_VARS_LIST
#undef VAR

class ComputeShaderElement;
struct LensFlareVertex;
struct LensFlareInfo;
struct ManualLightFlareData;

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
  // Exposure reduction: in range of [0, 1]
  //  0     -> no effect, physically correct rendering, higher exposure makes flare more intense
  //  0<x<1 -> flare gets more intense with higher exposure, but not linearly. Artistic effect.
  //  1     -> Exposure's effect is fully negated, visual intensity of flare is constant no matter the exposure. Artistic effect.
  float exposureReduction = 0;
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
  void prepareManualFlare(const CachedFlareId &cached_flare_config_id, const Point4 &light_pos, const Point3 &color, bool is_sun);
  void prepareDynamicLightFlares(const CachedFlareId &cached_flare_config_id);
  void collectAndPrepareECSFlares(const Point3 &camera_dir, const TMatrix4 &view_proj, const Point3 &sun_dir, const Point3 &sun_color);
  bool endPreparingLights(const Point3 &camera_pos, const Point3 &camera_dir, int omni_light_count, int spot_light_count);
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


  struct RenderConfig
  {
    TEXTUREID textureId = BAD_TEXTUREID;
    RoundingType roundingType = RoundingType::SHARP;

    // Ordering is important to avoid context switches. Draw calls should be grouped by parameters that produce variants
    bool operator<(const RenderConfig &other) const
    {
      // These affect the shader variant
      if (roundingType != other.roundingType)
        return static_cast<int>(roundingType) < static_cast<int>(other.roundingType);

      // These don't affect the shader variant
      return textureId < other.textureId;
    }

    bool operator==(const RenderConfig &other) const { return roundingType == other.roundingType && textureId == other.textureId; }
  };

  struct RenderBlock
  {
    RenderBlock(SharedTex texture, RoundingType rounding_type);
    int indexOffset = 0;
    int numTriangles = 0;
    int globalRenderConfigId = -1;
    SharedTex texture;
    RoundingType roundingType;

    [[nodiscard]] RenderConfig getRenderConfig() const { return {texture.getTexId(), roundingType}; }
  };

  class LensFlareData
  {
  public:
    struct Params
    {
      float smoothScreenFadeoutDistance;
      bool useOcclusion;
      float exposurePowParam;
    };

    explicit LensFlareData(eastl::string config_name, const Params &params);
    LensFlareData(LensFlareData &&) = default;
    LensFlareData &operator=(LensFlareData &&) = default;

    static LensFlareData parse_lens_flare_config(const LensFlareConfig &config,
      eastl::vector<uint16_t> &ib,
      eastl::vector<LensFlareVertex> &vb,
      eastl::vector<LensFlareInfo> &flares,
      eastl::vector<Point2> &vpos);

    void setRenderConfigIndices(const eastl::vector_set<RenderConfig> &global_render_configs);

    [[nodiscard]] const eastl::vector<RenderBlock> &getRenderBlocks() const;
    [[nodiscard]] const eastl::string &getConfigName() const { return configName; }
    [[nodiscard]] const Params &getParams() const { return params; }
    [[nodiscard]] int getLocalRenderBlockId(int global_render_block_id) const
    {
      return globalToLocalRenderBlockId[global_render_block_id];
    }

  private:
    eastl::string configName;
    Params params;
    eastl::vector<int> globalToLocalRenderBlockId;
    eastl::vector<RenderBlock> renderBlocks;
  };

  struct PreparedLight
  {
    Point3 lightColor;
    Point2 lightPos;
    int flareConfigId;
    bool isSun;
  };
  struct DrawArguments
  {
    // Instance count is filled in a compute shader
    uint32_t indexCountPerInstance;
    uint32_t startIndexLocation;
    int32_t baseVertexLocation;
    uint32_t startInstanceLocation;
    operator DrawIndexedIndirectArgs() const;
  };

  Ptr<ComputeShaderElement> prepareFlaresShader;
  Ptr<ComputeShaderElement> prepareFlaresOcclusionShader;
  Ptr<ShaderMaterial> lensShaderMaterial;
  dynrender::RElem lensShaderElement;
  int maxNumManualPreparedLights = 2; // non-dynamic lights
  int maxNumInstances = 256;

  eastl::vector<LensFlareData> lensFlares;
  // vector_set is used to sort the array, not just for quick lookups! If it's changed to another type, manual sort is needed
  eastl::vector_set<RenderConfig> globalRenderConfigs;
  UniqueBufHolder lensFlareBuf;
  UniqueBufHolder manualLightDataBuf;
  UniqueBufHolder vertexPositionsBuf;

  // Instances are grouped together according to the flare configs to allow instanced draw calls
  //  First part of buffer: instance data for manual lights, there can be gaps between data for different flare configs. Starting
  //  indices are allocated first, data is culled later on GPU Second part of the buffer: continuous data for the dynamic light
  //  instances
  UniqueBufHolder lensFlareInstancesBuf;

  // This buffer stores lists of data concatenated into a single buffer (indices point into this same buffer:
  //  header: <index of data for dynamic lights> + <index of data for the i-th flare config>...
  //  data (repeated several times for dynamic lights and all used flare configs): <index of i-th draw call (for the indirect draw args
  //  buffer)> <~0u (terminating value)>
  UniqueBufHolder drawCallIndicesBufferBuf;

  // All parameters except for instance count are filled on CPU, compute shader fills out instance counts after culling
  UniqueBuf indirectDrawBuf;

  // Used for occlusion test right after the prepare shader, runs on pre-culled instances
  UniqueBuf indirectDispatchBuf;
  // Stores instance indices of instances that passed the pre-culling
  // The instance buffer is not continuously filled with visible instances
  UniqueBuf preCulledInstanceIndicesBuf;

  UniqueBuf flareVB;
  UniqueBuf flareIB;
  int currentConfigCacheId = 0;
  bool isDirty = true;

  // Working memory and counters. These are reserved at initialization, reset and filled every frame.
  int numPreparedManualInstances = 0;
  int dynamicLightsFlareId = -1;
  bool hadDynamicLights = false;
  eastl::vector<bool> usedRenderConfigs;  // global render config id -> used or not
  eastl::vector<int> usedRenderConfigIds; // list of <global render config id>
  // flare config id = index in arrays preparedLightsPerFlareId/lensFlares
  eastl::vector<bool> usedFlareConfigs;          // flare config id -> used or not
  eastl::vector<int> usedFlareConfigIds;         // list of <flare config id>
  eastl::vector<IPoint2> flareIdToInstanceRange; // <start index; instance count> for each flare id used for manual flares
  eastl::vector<DrawArguments> indirectDrawArguments;
  eastl::vector<uint32_t> dynamicLightDrawIndices; // list of indices of draw calls used for dynamic lights
  // flare config id -> list of draw call indices used for manual flares
  eastl::vector<eastl::vector<uint32_t>> manualFlareDrawCallIndicesPerLensFlareId;
  eastl::vector<eastl::vector<ManualLightFlareData>> manualPreparedLightsPerFlareId;
  eastl::vector<int> multiDrawCountPerRenderConfig;

  void prepareConfigBuffers(const eastl::vector<LensFlareConfig> &configs);
  void updateConfigsFromECS();
  [[nodiscard]] bool prepareUseLensFlareConfig(const CachedFlareId &id);
};

ECS_DECLARE_BOXED_TYPE(LensFlareRenderer);
