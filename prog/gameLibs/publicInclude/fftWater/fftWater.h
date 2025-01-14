//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_carray.h>
#include <generic/dag_tab.h>
#include <drv/3d/dag_resId.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_resourcePool.h>
#include <3d/dag_lockTexture.h>
#include <shaders/dag_postFxRenderer.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <math/dag_hlsl_floatx.h>
#include <fftWater/flow_map_inc.hlsli>
#include <fftWater/fftWaterQuality.h>

class Point2;
class Point3;
class FFTWater;
class TextureIDHolderWithVar;
class BBox2;
struct IWaterDecalsRenderHelper;
class DataBlock;
class RiverRendererCB
{
public:
  virtual void renderRivers() = 0;
};

namespace fft_water
{
enum
{
  MIN_NUM_CASCADES = 4,
  MAX_NUM_CASCADES = 5,
  DEFAULT_NUM_CASCADES = MIN_NUM_CASCADES,
  BEAUFORT_SCALES_NUM = 7
};

enum class Spectrum : uint8_t
{
  PHILLIPS = 0,
  UNIFIED_DIRECTIONAL = 1,
  LAST = UNIFIED_DIRECTIONAL,
  DEFAULT = PHILLIPS
};

G_STATIC_ASSERT((int)fft_water::Spectrum::LAST == 1);
static const char *spectrum_names[(int)Spectrum::LAST + 1] = {"phillips", "unified_directional"};

enum
{
  WATER_RENDER_PASS_NORMAL = 0,
  WATER_RENDER_PASS_DEPTH = 1
};

struct SimulationParams
{
  SimulationParams() {}
  SimulationParams(carray<float, MAX_NUM_CASCADES> in_amplitude, float in_choppiness, float in_wind_dependency, float wind_alignment,
    float in_facet_size, Spectrum in_spectrum) :
    amplitude(in_amplitude),
    choppiness(in_choppiness),
    windDependency(in_wind_dependency),
    windAlignment(wind_alignment),
    facetSize(in_facet_size),
    spectrum(in_spectrum)
  {}

  bool operator==(const SimulationParams &ref) const
  {
    for (int ampNo = 0; ampNo < ref.amplitude.size(); ++ampNo)
      if (amplitude[ampNo] != ref.amplitude[ampNo])
        return false;
    return choppiness == ref.choppiness && windDependency == ref.windDependency && windAlignment == ref.windAlignment &&
           facetSize == ref.facetSize && spectrum == ref.spectrum;
  }
  bool operator!=(const SimulationParams &ref) const { return !operator==(ref); }

  G_STATIC_ASSERT(MAX_NUM_CASCADES == 5);
  carray<float, MAX_NUM_CASCADES> amplitude = {{0.7f, 0.7f, 0.7f, 0.7f, 0.7f}};
  float choppiness = 1.0f;
  float windDependency = 0.98f;
  float windAlignment = 1.0f;
  float facetSize = 0.05f;
  Spectrum spectrum = Spectrum::DEFAULT;
};

struct FoamParams
{
  FoamParams() {}
  FoamParams(float in_generation_threshold, float in_generation_amount, float in_dissipation_speed, float in_falloff_speed,
    float in_hats_mul, float in_hats_threshold, float in_hats_folding, float in_surface_folding_foam_mul = 0.f,
    float in_surface_folding_foam_pow = 1.f) :
    generation_threshold(in_generation_threshold),
    generation_amount(in_generation_amount),
    dissipation_speed(in_dissipation_speed),
    falloff_speed(in_falloff_speed),
    hats_mul(in_hats_mul),
    hats_threshold(in_hats_threshold),
    hats_folding(in_hats_folding),
    surface_folding_foam_mul(in_surface_folding_foam_mul),
    surface_folding_foam_pow(in_surface_folding_foam_pow)
  {}

  float generation_threshold = 0.875f, generation_amount = 0.095f, dissipation_speed = 0.7f, falloff_speed = 0.985f;
  float hats_mul = 4.0f, hats_threshold = 0.85f, hats_folding = 0.35f;
  float surface_folding_foam_mul = 0, surface_folding_foam_pow = 1;
};

struct WavePreset
{
  WavePreset();
  WavePreset(float in_period, float in_wind, float in_roughness, float small_wave_fraction, const SimulationParams &in_simulation,
    const FoamParams &in_foam) :
    period(in_period),
    wind(in_wind),
    roughness(in_roughness),
    smallWaveFraction(small_wave_fraction),
    simulation(in_simulation),
    foam(in_foam)
  {}

  float period = 500.0f;
  float wind = 2.5f;
  float roughness = 0.2f;
  float smallWaveFraction = 0.001f;
  SimulationParams simulation;
  FoamParams foam;
};

struct WavePresets
{
  carray<WavePreset, BEAUFORT_SCALES_NUM> presets;
  const WavePreset &operator[](int index) const { return presets[index]; }
  WavePreset &operator[](int index) { return presets[index]; }
};

struct WaterFlowmapCascade
{
  Point4 flowmapArea = Point4(1, 1, 0, 0);
  UniqueTex texA, texB;
  UniqueTex blurTexA, blurTexB;
  UniqueTex tempTex;
  int frameCount = 0;
  int frameTime = -1;
  float frameRate = 30;
};

struct WaterFlowmap
{
  bool enabled = false;
  String texName;
  Point4 texArea;
  float windStrength = 0.2f;
  float flowmapRange = 100;
  float flowmapFading = 3;
  float flowmapDamping = 0.9f;
  Point4 flowmapStrength = Point4(1, 5, 0.5f, 0);
  Point4 flowmapStrengthAdd = Point4(0.5f, 1, 1, 0.1f);
  Point4 flowmapFoam = Point4(5, 10, 0.5f, 0.1f);
  float flowmapFoamReflectivityMin = 0.0f;
  Point3 flowmapFoamColor = Point3(1, 1, 1);
  float flowmapFoamTiling = 1;
  float flowmapFoamDisplacement = 0.1f;
  float flowmapFoamDetail = 1;
  Point4 flowmapDepth = Point4(1, 0.1f, 0.3f, 1);
  float flowmapSlope = 1;
  Point2 flowmapWaveFade = Point2(1, 5);
  bool flowmapDetail = true;
  bool usingFoamFx = false;
  bool hasSlopes = false;

  SharedTexHolder tex;
  PostFxRenderer builder;
  PostFxRenderer fluidSolver;
  PostFxRenderer flowmapBlurX, flowmapBlurY;

  dag::Vector<FlowmapCircularObstacle> circularObstacles;
  dag::Vector<FlowmapRectangularObstacle> rectangularObstacles;
  UniqueBufHolder circularObstaclesBuf;
  UniqueBufHolder rectangularObstaclesBuf;
  PostFxRenderer circularObstaclesRenderer;
  PostFxRenderer rectangularObstaclesRenderer;

  dag::Vector<WaterFlowmapCascade> cascades;
};

struct WaterHeightmap
{
  static const int PATCHES_GRID_SIZE = 512;
  static const int HEIGHTMAP_PAGE_SIZE = 128;
  static const int PAGE_SIZE_PADDED = 130;
  eastl::vector<uint16_t> grid;
  eastl::vector<uint16_t> pages;
  Point4 tcOffsetScale;
  int gridSize = 0, pagesX = 0, pagesY = 0, scale = 1;
  float heightOffset = 0.0f, heightScale = 0.0f, heightMax = 0.0f;
  eastl::vector<Point2> patchHeights;
  void getHeightmapDataBilinear(float x, float z, float &result) const;
};

void init();
void close();

enum
{
  GEOM_INVISIBLE = 0,
  GEOM_NORMAL = 1,
  GEOM_HIGH = GEOM_NORMAL
};
enum RenderMode
{
  WATER_SHADER = 0,
  WATER_DEPTH_SHADER,
  WATER_SSR_SHADER,
  MAX
};
FFTWater *create_water(RenderQuality quality, float period = 1000.f, int res_bits = 7, bool depth_renderer = false,
  bool ssr_renderer = false, bool one_to_four_cascades = false, int min_render_res_bits = 6,
  RenderQuality geom_quality = RenderQuality::UNDEFINED);
void init_render(FFTWater *, int quality, bool depth_renderer, bool ssr_renderer, bool one_to_four_cascades,
  RenderQuality geom_quality = RenderQuality::UNDEFINED);
bool one_to_four_render_enabled(FFTWater *handle);
void set_grid_lod0_additional_tesselation(FFTWater *a, float amount);
void set_grid_lod0_area_radius(FFTWater *a, float radius);
void set_period(FFTWater *handle, float period);
int get_num_cascades(FFTWater *handle);
void set_num_cascades(FFTWater *handle, int cascades);
float get_period(FFTWater *handle);
void delete_water(FFTWater *&);
void simulate(FFTWater *handle, double time);
void before_render(FFTWater *handle);
void set_render_quad(FFTWater *handle, const BBox2 &quad);
void render(FFTWater *handle, const Point3 &pos, TEXTUREID distance_tex_id, const Frustum &frustum, const Driver3dPerspective &persp,
  int geom_lod_quality = GEOM_NORMAL, int survey_id = -1, IWaterDecalsRenderHelper *decals_renderer = NULL,
  RenderMode render_mode = WATER_SHADER);
float getGridLod0AreaSize(FFTWater *handle);
void setGridLod0AdditionalTesselation(FFTWater *handle, float additional_tesselation);
void setAnisotropy(FFTWater *handle, int aniso, float mip_bias = 0.f);
float get_small_wave_fraction(FFTWater *handle);
void set_small_wave_fraction(FFTWater *handle, float smallWaveFraction);
float get_cascade_window_length(FFTWater *handle);
void set_cascade_window_length(FFTWater *handle, float value);
float get_cascade_facet_size(FFTWater *handle);
void set_cascade_facet_size(FFTWater *handle, float value);
SimulationParams get_simulation_params(FFTWater *handle);
void set_simulation_params(FFTWater *handle, const SimulationParams &params);
void set_foam(FFTWater *handle, const FoamParams &params);
FoamParams get_foam(FFTWater *handle);

enum GraphicFeature
{
  GRAPHIC_SHORE,
  GRAPHIC_WAKE,
  GRAPHIC_SHADOWS,
  GRAPHIC_DEEPNESS,
  GRAPHIC_FOAM,
  GRAPHIC_ENV_REFL,
  GRAPHIC_PLANAR_REFL,
  GRAPHIC_SUN_REFL,
  GRAPHIC_SEABED_REFR,
  GRAPHIC_SSS_REFR,
  GRAPHIC_UNDERWATER_REFR,
  GRAPHIC_PROJ_EFF,
  GRAPHIC_FEATURE_END
};
void enable_graphic_feature(FFTWater *handle, GraphicFeature feature, bool enable);
void get_cascade_period(FFTWater *handle, int cascade_no, float &out_period, float &out_window_in, float &out_window_out);

// if detect_rivers_width <= 0 or reiversCB == 0, it won't be used
void build_distance_field(UniqueTexHolder &, int texture_size, int heightmap_texture_size, float detect_rivers_width,
  RiverRendererCB *riversCB, bool high_precision_distance_field = true, bool shore_waves_on = true);
void build_flowmap(FFTWater *handle, int flowmap_texture_size, int heightmap_texture_size, const Point3 &camera_pos, int cascade,
  bool obstacles);
void set_flowmap_tex(FFTWater *handle);
void set_flowmap_params(FFTWater *handle);
void set_flowmap_foam_params(FFTWater *handle);
void close_flowmap(FFTWater *handle);
bool is_flowmap_active(FFTWater *handle);
void flowmap_floodfill(int texSize, const LockedImage2DView<const uint16_t> &heightmapTexView,
  LockedImage2DView<uint16_t> &floodfillTexView, uint16_t heightmapLevel);
void deferred_wet_ground(FFTWater *handle, const Point3 &pos);
void prepare_refraction(FFTWater *handle, Texture *scene_target_tex);
void set_current_time(FFTWater *handle, double time); // remove me! should not be used!
void reset_physics(FFTWater *handle);
void wait_physics(FFTWater *handle);
bool validate_next_time_tick(FFTWater *handle, double next_time);
void reset_render(FFTWater *handle); // if device is lost
void set_render_quality(FFTWater *handle, int quality, bool depth_renderer, bool ssr_renderer);
float get_level(FFTWater *handle);
void set_level(FFTWater *handle, float level);
float get_height(FFTWater *handle, const Point3 &point);
float get_max_level(FFTWater *handle);
void get_wind_speed(FFTWater *handle, float &out_speed, Point2 &out_wind_dir);
void set_wind_speed(FFTWater *handle, float speed, const Point2 &wind_dir);
void get_roughness(FFTWater *handle, float &out_roughness_base, float &out_cascades_roughness_base);
void set_roughness(FFTWater *handle, float roughness_base, float cascades_roughness_base);
float get_max_wave(FFTWater *handle);
float get_significant_wave_height(FFTWater *handle);
void set_wave_displacement_distance(FFTWater *handle, const Point2 &value);
void shore_enable(FFTWater *handle, bool enable);
bool is_shore_enabled(FFTWater *handle);
float get_shore_wave_threshold(FFTWater *handle);
void set_shore_wave_threshold(FFTWater *handle, float value);
int get_fft_resolution(FFTWater *handle);
void set_fft_resolution(FFTWater *handle, int res_bits);
void set_render_pass(FFTWater *handle, int pass);

int intersect_segment(FFTWater *, const Point3 &start, const Point3 &end, float &result); // must return 1 if intersection found and 0
                                                                                          // if intersection is not found, updates
                                                                                          // result (T along the segment) if
                                                                                          // intersection is found
int intersect_segment_at_time(FFTWater *, double at_time, const Point3 &start, const Point3 &end, float &result); // must return 1 if
                                                                                                                  // intersection found
                                                                                                                  // and 0 if
                                                                                                                  // intersection is
                                                                                                                  // not found, updates
                                                                                                                  // result (T along
                                                                                                                  // the segment) if
                                                                                                                  // intersection is
                                                                                                                  // found
int getHeightAboveWater(FFTWater *, const Point3 &point, float &result, bool matchRenderGrid = false);
int getHeightAboveWaterAtTime(FFTWater *, double at_time, const Point3 &point, float &result, Point3 *out_displacement = NULL);
void setRenderParamsToPhysics(FFTWater *handle);
void setVertexSamplers(FFTWater *, int samplersCount); // samplersCount shows quality of sampling. Obviously, if higher cascades can
                                                       // not provide significant displacement, they should not be used

// returns number of samplers
int setWaterCell(FFTWater *, float water_cell_size, bool auto_set_samplers_cnt); // samplersCount shows quality of sampling. Obviously,
                                                                                 // if higher cascades can not provide significant
                                                                                 // displacement (i.e. 0.25 of cell), they should not
                                                                                 // be used
void set_water_dim(FFTWater *handle, int dim_bits);

void setWakeHtTex(FFTWater *handle, TEXTUREID wake_ht_tex_id);

void force_actual_waves(FFTWater *, bool enforce);

void init_gpu_queries();
void release_gpu_queries();
void update_gpu_queries(float dt);
void render_debug_gpu_queries();
void before_reset_gpu_queries();
uint32_t make_gpu_query_user();
// returns more correct results but costs time on the gpu, only by this way we can get
// the height for the ocean surf at a sea shore
bool query_gpu_water_height(uint32_t query_handle, uint32_t &ref_query_result, const Point3 &from_y_up, float max_height,
  bool &out_underwater, float &out_height, int priority = 0, float time_life = 1.0f);

void load_wave_preset(const DataBlock *presetBlk, WavePreset &preset, int def_preset_no = 0);
void load_wave_presets(const DataBlock &blk, WavePresets &out_waves);
void save_wave_preset(DataBlock *presetBlk, WavePreset &preset);

void apply_wave_preset(FFTWater *water, const WavePreset &preset, const Point2 &wind_dir);
void apply_wave_preset(FFTWater *water, const WavePreset &preset1, const WavePreset &preset2, float alpha, const Point2 &wind_dir);
void apply_wave_preset(FFTWater *water, const WavePresets &waves, float bf_scale, const Point2 &wind_dir);
void apply_wave_preset(FFTWater *water, float bf_scale, const Point2 &wind_dir, Spectrum spectrum);

void get_wave_preset(FFTWater *water, WavePreset &out_preset);
void set_wind(FFTWater *handle, float bf_scale, const Point2 &wind_dir);

WaterFlowmap *get_flowmap(FFTWater *handle);
void create_flowmap(FFTWater *handle);
void remove_flowmap(FFTWater *handle);

const WaterHeightmap *get_heightmap(FFTWater *water);
void set_heightmap(FFTWater *water, eastl::unique_ptr<WaterHeightmap> &&heightmap);
void remove_heightmap(FFTWater *water);
void load_heightmap(IGenLoad &loadCb, FFTWater *water);
} // namespace fft_water
