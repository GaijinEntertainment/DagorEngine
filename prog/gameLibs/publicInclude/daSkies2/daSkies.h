//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <drv/3d/dag_renderPass.h>
#include <generic/dag_carray.h>
#include <generic/dag_smallTab.h>
#include <shaders/dag_postFxRenderer.h>
#include <math/dag_Point4.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_color.h>
#include <vecmath/dag_vecMathDecl.h>
#include <daSkies2/daScattering.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/vector_map.h>
#include <3d/dag_ringCPUQueryLock.h>
#include <../3rdPartyLibs/ska_hash_map/flat_hash_map2.hpp>
#include <render/mipRenderer.h>
#include <../../gameLibs/daSkies2/panoramaCompressor.h>

struct SkiesData;
struct Clouds2;
struct AuroraBorealis;


enum class CloudsResolution
{
  Default,
  FullresCloseClouds,
  ForceFullresClouds
};


class DaSkies
{
public:
#define _PARAM(type, name, def_value)                      type name = def_value;
#define _PARAM_RAND(type, name, def_value)                 type name = def_value;
#define _PARAM_RAND_CLAMP(type, name, def_value, min, max) _PARAM_RAND(type, name, def_value)

  struct CloudsSettingsParams // this will cause relighting and fields regeneration. Supposed to be set per type of game.
  {
#define CLOUDS_SETTINGS_PARAMS                                                                                                    \
  _PARAM(float, maximum_averaging_ratio, 1.0f) /*if maximum_averaging_ratio is 0 - we will use maximum for field in target        \
                                                  resolution, if it is 1 - average*/                                              \
  _PARAM(int, quality, 1)                                                                                                         \
  _PARAM(int, target_quality, 1)                                                                                                  \
  _PARAM(bool, competitive_advantage, false) /*This is to make low-quality not more competitive advantage, and not needed if game \
                                                doesn't allow change settings, or if it is single player, or if we can't hide in  \
                                                clouds*/                                                                          \
  _PARAM(bool, fastEvolution, false)

    CLOUDS_SETTINGS_PARAMS

    bool operator==(const CloudsSettingsParams &a) const;
    bool operator!=(const CloudsSettingsParams &a) const { return !(*this == a); }
  };

  struct CloudsWeatherGen // this will cause weather regen and thus completelys invalidates all clouds weather
  {
#define CLOUDS_WEATHER_GEN_PARAMS               \
  _PARAM_RAND(float, layers[0].coverage, 0.5f)  \
  _PARAM_RAND(float, layers[0].freq, 2.f)       \
  _PARAM_RAND(float, layers[0].seed, 0.f)       \
  _PARAM_RAND(float, layers[1].coverage, 0.5f)  \
  _PARAM_RAND(float, layers[1].freq, 3.f)       \
  _PARAM_RAND(float, layers[1].seed, 0.5f)      \
  _PARAM(float, epicness, 0.f)                  \
  _PARAM_RAND(float, cumulonimbusCoverage, 0.f) \
  _PARAM_RAND(float, cumulonimbusSeed, 0.f)     \
  _PARAM(float, worldSize, 65536.f)
    struct Layer
    {
      float coverage;
      float freq;
      float seed;
      bool operator==(const Layer &a) const { return coverage == a.coverage && freq == a.freq && seed == a.seed; }
    } layers[2] = {{0.5f, 2.f, 0.f}, {0.5f, 3.f, 0.5f}};
    float epicness = 0;
    float cumulonimbusCoverage = 0;
    float cumulonimbusSeed = 0;
    float worldSize = 65536.0;
    bool operator==(const CloudsWeatherGen &a) const;
    bool operator!=(const CloudsWeatherGen &a) const { return !(*this == a); }
  };


  struct CloudsForm // this has parameters that will cause relighting of both shadows and ambient
  {
#define CLOUDS_FORM_PARAMS                                                                                                        \
  _PARAM_RAND(float, layers[0].startAt, 0.8f)                                                                                     \
  _PARAM_RAND(float, layers[0].thickness, 8.f)                                                                                    \
  _PARAM_RAND(float, layers[0].density, 1.f)                                                                                      \
  _PARAM_RAND(float, layers[0].clouds_type, 0.5f)                                                                                 \
  _PARAM_RAND(float, layers[0].clouds_type_variance, 0.5f)                                                                        \
  _PARAM_RAND(float, layers[1].startAt, 8.f)                                                                                      \
  _PARAM_RAND(float, layers[1].thickness, 4.f)                                                                                    \
  _PARAM_RAND(float, layers[1].density, 0.5f)                                                                                     \
  _PARAM_RAND(float, layers[1].clouds_type, 0.f)                                                                                  \
  _PARAM_RAND(float, layers[1].clouds_type_variance, 0.5f)                                                                        \
  _PARAM_RAND(float, extinction, 0.75f) /*this is 0.06 multiplier. Clouds real extinction is within 0.04-0.24, which is 0.66..4*/ \
  _PARAM_RAND(float, turbulenceStrength, 0.21f)                                                                                   \
  _PARAM(int, shapeNoiseScale, 9)                                                                                                 \
  _PARAM(int, cumulonimbusShapeScale, 4)                                                                                          \
  _PARAM(int, turbulenceFreq, 2)
    struct Layer
    {
      float startAt, thickness, density;
      float clouds_type,      // 0 - strata, 1 - cumulus
        clouds_type_variance; // 0..1

      bool operator==(const Layer &a) const
      {
        return startAt == a.startAt && thickness == a.thickness && density == a.density &&
               clouds_type_variance == a.clouds_type_variance && clouds_type == a.clouds_type;
      }
    } layers[2] = {{0.8f, 8.f, 1.f, 0.5f, 0.5f}, {8.f, 4.f, 0.5f, 0.f, 0.5f}};
    float extinction = 0.75f; // this is 0.06 multiplier. Clouds real extinction is within 0.04-0.24, which is 0.66..4
    int shapeNoiseScale = 9, cumulonimbusShapeScale = 4;
    int turbulenceFreq = 2;
    float turbulenceStrength = 0.21;
    // float gradient_per1km = 1000;//both FB and HZD papers suggest to skew clouds in wind direction. I don't see the benefit, it
    // doesn't look natural
    bool operator==(const CloudsForm &a) const;
    bool operator!=(const CloudsForm &a) const { return !(*this == a); }
  };

  struct CloudsRendering // this structure has parameters that won't cause re-lighting, it just affects how it looks on screen
  {
#define CLOUDS_RENDERING_PARAMS                                                                           \
  _PARAM(float, forward_eccentricity, 0.8f)        /*FB and RDR2 uses 0.8 by default*/                    \
  _PARAM(float, back_eccentricity, 0.5f)           /*FB and RDR2 uses 0.5 by default*/                    \
  _PARAM(float, forward_eccentricity_weight, 0.5f) /*FB and RDR2 uses 0.5 by default*/                    \
  _PARAM(float, erosion_noise_size, 811.f / 32)    /*this is arguably affecting lighting as well*/        \
  _PARAM(float, ambient_desaturation, 0.5f)        /*FB uses 0.5 by default*/                             \
  _PARAM(float, ms_contribution, 0.7f)             /*on sunset 0.7 is better, in noon 0.5-0.6 is better*/ \
  _PARAM(float, ms_attenuation, 0.3f)              /*in sunset 0.25 is better, in noon 0.45 is better*/   \
  _PARAM(float, ms_ecc_attenuation, 0.6f)          /*probably better not to touch at all*/                \
  _PARAM(float, erosionWindSpeed, 0.6f) /*this should not affect rendering at all, but currently it actually will, causing rebuild*/

    CLOUDS_RENDERING_PARAMS
    bool operator==(const CloudsRendering &a) const;
    bool operator!=(const CloudsRendering &a) const { return !(*this == a); }
  };

  struct StrataClouds
  {

#define STRATA_CLOUDS_PARAMS                  \
  _PARAM_RAND_CLAMP(float, amount, 0.5, 0, 1) \
  _PARAM_RAND(float, altitude, 12)

    STRATA_CLOUDS_PARAMS
    bool operator==(const StrataClouds &a) const;
    bool operator!=(const StrataClouds &a) const { return !(*this == a); }
  };

#undef _PARAM
#undef _PARAM_RAND
#undef _PARAM_RAND_CLAMP

  const CloudsSettingsParams &getCloudsGameSettingsParams() const { return cloudsSettings; }
  const CloudsWeatherGen &getWeatherGen() const { return cloudsWeatherGen; }
  const CloudsForm &getCloudsForm() const { return cloudsForm; }
  const CloudsRendering &getCloudsRendering() const { return cloudsRendering; }

  const SkyAtmosphereParams &getSkyParams() const { return skyParams; }
  const StrataClouds &getStrataClouds() const { return strataCloudsParams; }

  void setCloudsGameSettingsParams(const CloudsSettingsParams &v) { cloudsSettings = v; }
  void setWeatherGen(const CloudsWeatherGen &v) { cloudsWeatherGen = v; }

  void setCloudsForm(const CloudsForm &form_)
  {
    if (cloudsForm == form_)
      return;
    cloudsForm = form_;
  }

  void setCloudsRendering(const CloudsRendering &v) { cloudsRendering = v; }

  // temporary function for a workaround on A8 iPads (gpu hang)
  void setNearCloudsRenderingEnabled(bool enabled);

  void setSkyParams(const SkyAtmosphereParams &p);
  void setStrataClouds(const StrataClouds &a);
  void setStrataCloudsTexture(const char *tex_name);

  // if project is relying on clouds being 'high in the sky' and never intersect anything close to camera
  //  it can uses CLOUDS_SHADOWS_BASE_2D in clouds_shadow.sh to shadow stuff from clouds
  //  this can be both faster and better looking
  // if you make a sky / flight sim, you'd better rely on CLOUDS_SHADOWS_BASE_REF  and you don't need that
  // off by default
  void projectUses2DShadows(bool on);

  void reset(); // todo: make protected, check generation
  void invalidate();

  bool isPrepareRequired() const;
  bool isCloudsReady() const;
  void prepare(const Point3 &dir_to_sun, bool force_update_cpu, float dt);

  void cloudsLayersHeightsBarrier();

  DaSkies(bool cpu_only = false);
  ~DaSkies();

  // todo: implement on client
  bool traceRayClouds(const Point3 &pos, const Point3 &dir, float &t, const int threshold) const;
  // todo: remove me
  float getCloudsDensityFiltered(const Point3 &pos) const;
  float getCloudsDensityFilteredPrecise(const Point3 &pos) const;
  //

  int addCloudsTrace(const Point3 &pos, const Point3 &dir, float dist, float threshold = 1.f, float scattering_threshold = 0.f);
  bool getCloudsTraceResult(int trace_id, float &out_t, float &out_transmittance);
  void dispatchCloudsTraces();

  float getRainAtPos(const Point3 &pos); // Not exactly at this point, results lag behind the position, call with consecutive
                                         // positions.

  void destroy_skies_data(SkiesData *&data);
  // required for each point of view (or the only one if panorama, for panorama center)
  SkiesData *createSkiesData(const char *base_name, const PreparedSkiesParams &params, bool render_stars = true);
  // legacy
  SkiesData *createSkiesData(const char *base_name, int prepared_quality = 128, bool panoramic = false)
  {
    const int quality = clamp(prepared_quality / 64, 1, 4);
    return createSkiesData(base_name,
      PreparedSkiesParams{quality, // skies
        quality * 2,               // scattering
        (quality + 1) * 32,        // slices
        quality,                   // transmittance
        1.0f,                      // range scale
        1000.f,                    // min range
        panoramic ? PreparedSkiesParams::Panoramic::ON : PreparedSkiesParams::Panoramic::OFF,
        panoramic ? PreparedSkiesParams::Reprojection::OFF : PreparedSkiesParams::Reprojection::ON});
  }
  IPoint2 getCloudsResolution(SkiesData *data);
  void skiesDataScatteringVolumeBarriers(SkiesData *data);
  // if sky_quality_div>1, additional textures required
  void changeSkiesData(int sky_detail_level, int clouds_detail_level, bool fly_through_clouds, int targetW, int targetH,
    SkiesData *data, CloudsResolution clouds_resolution = CloudsResolution::Default, bool use_blurred_clouds = false);
  void initSky(int clouds_fog_resolution = 32, uint32_t skyfmt = 0xFFFFFFFF, bool useHole = true);
  void closeSky();
  void updateCloudsOrigin();
  void setCloudsOrigin(double x, double z)
  {
    if (cloudsOrigin.x != x || cloudsOrigin.z != z)
    {
      cloudsOrigin.x = x;
      cloudsOrigin.z = z;
      updateCloudsOrigin();
    }
  }
  void setStrataCloudsOrigin(double x, double z)
  {
    if (cloudsOrigin.strataX != x || cloudsOrigin.strataZ != z)
    {
      cloudsOrigin.strataX = x;
      cloudsOrigin.strataZ = z;
      updateCloudsOrigin();
    }
  }
  // find hole where shadows are minimum (at clouds bottom)
  // right now, it causes stall (doesn't have to)
  // you need to call it before any other rendering calls, or updatePanorama right after.
  // won't affect wind and other cloudsOffset
  // hole_target_pos - that wont be in shadow, density - amount of shadow
  void resetCloudsHole(const Point3 &hole_target_pos, const float &hole_density = 0);
  void resetCloudsHole();          // but basically removes clouds hole. call updatePanorama right after, if you have one
  void setUseCloudsHole(bool set); // finding hole can be disabled to avoid sky jumping around while changing params
  bool getUseCloudsHole() const;
  Point2 getCloudsHolePosition() const;       // that's for debug only!
  void setCloudsHolePosition(const Point2 &); // that's for debug only!
  void setExternalWeatherTexture(TEXTUREID tid);

  DPoint2 getCloudsOrigin() const { return DPoint2(cloudsOrigin.x, cloudsOrigin.z); }
  DPoint2 getStrataCloudsOrigin() const { return DPoint2(cloudsOrigin.strataX, cloudsOrigin.strataZ); }

// 20 meters is enough so we don't reprepare skies for tanks when camera rotates
#define SKY_PREPARE_THRESHOLD 20.0f

  void useFog(const Point3 &origin, SkiesData *data, const TMatrix &view_tm, const TMatrix4 &proj_tm, bool update_sky = true,
    float altitude_tolerance = SKY_PREPARE_THRESHOLD);

  // either use renderEnvi or prepareSkyAndClouds + renderSky + renderClouds
  void renderEnvi(bool infinite, const DPoint3 &origin, const DPoint3 &viewdir, uint32_t render_sun_moon, // we can skip sun rendering
                                                                                                          // for, say cubic
    const ManagedTex &cloudsDepth, const ManagedTex &prevCloudsDepth, // clouds Depth(s) is supposed to be 1/2 of target depth with
                                                                      // mips, if required lower resolution

    TEXTUREID targetDepth, // targetDepth
    SkiesData *data,       // if data is null than only sky will be rendered
    const TMatrix &view_tm, const TMatrix4 &proj_tm, const Driver3dPerspective &persp, bool update_sky = true,
    bool fixed_offset = false, float altitude_tolerance = SKY_PREPARE_THRESHOLD, AuroraBorealis *aurora = nullptr);

  // if update_sky = false sky won't be changed, so we can use it for main data for cube
  // if fixed_offset is true, cloud offset will be 0
  void prepareSkyAndClouds(bool infinite, const DPoint3 &origin, const DPoint3 &dir, uint32_t render_sun_moon, // we can skip sun
                                                                                                               // rendering for, say
                                                                                                               // cubic
    const TextureIDPair &cloudsDepth, const TextureIDPair &prevCloudsDepth, // clouds Depth(s) is supposed to be 1/2 of target depth
                                                                            // with mips, if required lower resolution
    SkiesData *data, const TMatrix &view_tm, const TMatrix4 &proj_tm, bool update_sky, bool fixed_offset,
    float altitude_tolerance = SKY_PREPARE_THRESHOLD, AuroraBorealis *aurora = nullptr);


  // ResPtr variant
  void prepareSkyAndClouds(bool infinite, const DPoint3 &origin, const DPoint3 &dir, uint32_t render_sun_moon,
    const ManagedTex &cloudsDepth, const ManagedTex &prevCloudsDepth, SkiesData *data, const TMatrix &view_tm, const TMatrix4 &proj_tm,
    bool update_sky, bool fixed_offset, float altitude_tolerance = SKY_PREPARE_THRESHOLD, AuroraBorealis *aurora = nullptr);

  bool isCloudsVisible(SkiesData *data);

  void renderClouds(bool infinite, const ManagedTex &cloudsDepth, TEXTUREID targetDepth, SkiesData *data, const TMatrix &view_tm,
    const TMatrix4 &proj_tm, const Point4 &targetDepthTransform = Point4(1, 1, 0, 0));

  void renderClouds(bool infinite,
    const TextureIDPair &cloudsDepth, // clouds Depth(s) is supposed to be 1/2 of target depth with mips, if required lower resolution
    TEXTUREID targetDepth, SkiesData *data, // should have prepared data
    const TMatrix &view_tm, const TMatrix4 &proj_tm, const Point4 &targetDepthTransform = Point4(1, 1, 0, 0));

  void renderSky(SkiesData *data, // should have prepared data
    const TMatrix &view_tm, const TMatrix4 &proj_tm, const Driver3dPerspective &persp, bool render_prepared_lowres = true,
    AuroraBorealis *aurora = nullptr);
  void renderStars(const Driver3dPerspective &persp, const Point3 &origin, float stars_intensity_mul);
  float getCloudsStartAlt(); // effective, i.e. can change based on gpu readback
  float getCloudsTopAlt();   // effective, i.e. can change based on gpu readback
  float getCloudsStartAltSettings() const;
  float getCloudsTopAltSettings() const;
  void setWindDirection(const Point2 &windDir);

  void renderCelestialObject(const Point3 &dir, float phase, float intensity, float size);
  void renderCelestialObject(TEXTUREID tid, const Point3 &dir, float phase, float intensity, float size);

  void closePanorama();
  void initPanorama(SkiesData *main, bool blend_two, int resolutionW, int resolutionH = 0, // if resolutionH <=0 it will be
                                                                                           // resolutionW/2
    bool compress_panorama = false, bool sky_panorama_patch_enabled = true, bool allow_async_pipelines_feedback = false);
  bool panoramaEnabled() const
  {
    return (bool(cloudsPanoramaTex[0]) || bool(compressedCloudsPanoramaTex)) && !panoramaTemporarilyDisabled;
  }
  bool isPanoramaInitialized() { return bool(cloudsPanoramaTex[0]) || bool(compressedCloudsPanoramaTex); }
  void temporarilyDisablePanorama(bool disable) { panoramaTemporarilyDisabled = disable; }
  bool isPanoramaValid() const { return panoramaValid == PANORAMA_VALID; }

  bool currentSunSkyColor(float ht, float &sun_cos, float &moon_cos, Color3 &sun_color, Color3 &sun_sky_color, Color3 &moon_color,
    Color3 &moon_sky_color) const;
  bool currentGroundSunSkyColor(float &sun_cos, float &moon_cos, Color3 &sun_color, Color3 &sun_sky_color, Color3 &moon_color,
    Color3 &moon_sky_color) const;

  bool currentCloudsColor(Color3 &clouds_sun_color, Color3 &clouds_moon_color, Color3 &strata_sun_color,
    Color3 &strata_moon_color) const;
  Color3 getCpuSun(const Point3 &origin, const Point3 &lightDir) const;
  Color3 getCpuMoon(const Point3 &origin, const Point3 &lightDir) const;

  // single scattering numerical integration
  // higher values for steps are not only slower, but also somehow less correct, due we are not taking multiple scattering into account
  Color3 getCpuSunSky(const Point3 &camera, const Point3 &sun_dir, int sphere_steps = 12, int inscatter_steps = 12) const;
  Color3 getCpuMoonSky(const Point3 &camera, const Point3 &moon_dir, int sphere_steps = 12, int inscatter_steps = 12) const;
  Color3 getCpuIrradiance(const Point3 &origin, const Point3 &lightDir, int steps = 12, int csteps = 6) const; // normalized!

  Color3 getMieColor(const Point3 &cameraPosition, const Point3 &viewdir, float d) const;
  void getCpuFogSingle(const Point3 &cameraPosition, const Point3 &viewdir, float d, Color3 &insc, Color3 &loss) const;
  void getCpuFogSingle(const Point3 &cameraPosition, const Point3 &pos, Color3 &insc, Color3 &loss) const
  {
    Point3 view = pos - cameraPosition;
    float len = length(view);
    return getCpuFogSingle(cameraPosition, len > 0.0001f ? view / len : Point3(0, 0, 0), len, insc, loss);
  }
  Color3 calculateFogLoss(const Point3 &cameraPosition, const Point3 &worldPos) const;
  void setMoonDir(const Point3 &moon_dir) { moonDir = moon_dir; }
  const Point3 &getMoonDir() const { return moonDir; }
  const Point3 &getSunDir() const { return real_skies_sun_light_dir; }
  // Returns where the sun is visible on the panorama sky viewed from camera_pos, after the panorama reprojection
  const Point3 calcPanoramaSunDir(const Point3 &camera_pos) const;

  const Point3 &getPrimarySunDir() const { return primarySunDir; }     //
  const Point3 &getSecondarySunDir() const { return secondarySunDir; } //

  float getHazeStrength() const;

  float getMoonAge() const { return moonAge; } // from 0 to 29.530588853
  void setMoonAge(float age);
  float getCurrentMoonEffect() const
  {
    return moonEffect;
  } // power of moon being 'sun'. If 1, than the whole sky is lit by one moon, pretending to be sun
  float getCurrentSunEffect() const { return sunEffect; } // power of sun becoming dark so everything is lit by 'moon'.
  void setInitialAzimuthAngle(float value) { initialAzimuthAngle = value; }
  double getStarsJulianDay() const { return starsJulianDay; }
  void setStarsJulianDay(double julianDay) { starsJulianDay = julianDay; }
  void setStarsLatLon(float lat, float lon)
  {
    starsLatitude = lat;
    starsLongtitude = lon;
  }
  void getStarsLatLon(float &lat, float &lon) const
  {
    lat = starsLatitude;
    lon = starsLongtitude;
  }
  void setAstronomicalSunMoon();
  void calcSunMoonDir(float forward_time, Point3 &sun_dir, Point3 &moon_dir) const;
  void renderCloudVolume(VolTexture *cloudVolume, float max_dist, const TMatrix &view_tm, const TMatrix4 &proj_tm);
  void invalidatePanorama(bool force = false, bool waitResourcesLoaded = false);

  // void setSkiesRenderData(const Point3 &world_view_pos, const Point3 &sun_light_dir, const Color3 &sun_light_color);

  float getPanoramaReprojectionWeight() const { return panoramaReprojectionWeight; }
  void setPanoramaReprojectionWeight(float w)
  {
    panoramaReprojectionWeight = clamp(w, 0.f, 1.f);
  } // 1- fully reproject, 0 - no reprojection
  void setSunAndMoonDir(const Point3 &sun_dir, const Point3 &moon_dir);
  bool getMoonIsPrimary() const { return moonIsPrimary; }

  bool setMoonResourceName(const char *res_name);
  void setMoonSize(float size);

  // volumetric cloud raymarching can skip samples with low thickness
  float getMinCloudThickness() const { return minCloudThickness; }
  void setMinCloudThickness(float v) { minCloudThickness = v; }

  void enablePanoramaDownsampledDepth(bool state)
  {
    renderDownsampledDepth = state;
    invalidatePanorama();
  }

  void getCloudsTextureResourceDependencies(Tab<TEXTUREID> &dependencies) const;

protected:
  // internal
  union Request
  {
    struct
    {
      float mu, ht;
    };
    uint64_t hash;
    bool operator==(const Request &b) const { return hash == b.hash; }
  };
  struct RequestHasher
  {
    ska::hash_size_t operator()(const Request &r)
    {
      ska::hash_size_t rh = 2166136261U;
      rh = 16777619 * (rh ^ uint32_t(r.hash & 0xFFFFFFFF));
      rh = 16777619 * (rh ^ uint32_t(r.hash >> 31));
      return rh;
    }
  };
  mutable ska::flat_hash_map<Request, Color3, RequestHasher> transmittanceRequests, irradianceRequests;
  void shrinkRequests();
  void invalidateRequests();
  Color3 getTransmittance(float ht, float mu) const;
  Color3 getIrradiance(float ht, float mu) const;

  void gotSunSkyColorVars();
  float getMaxDensityFilteredInt(int x, int z, float fracX, float fracZ) const;
  void calcScatteringParams();
  void setScatteringParams(const SkyAtmosphereParams &params);
  void prepareRenderClouds(bool infinite, const Point3 &origin, const TextureIDPair &cloudsDepth,
    const TextureIDPair &prevCloudsDepth, // clouds Depth(s) is supposed to be 1/2 of target depth with mips, if required lower
                                          // resolution
    SkiesData *data, const TMatrix &view_tm, const TMatrix4 &proj_tm);
  void renderCloudsToTarget(bool infinite, const Point3 &origin, const TextureIDPair &downsampledDepth, TEXTUREID targetDepth,
    const Point4 &targetDepthTransform, SkiesData *data, const TMatrix &view_tm, const TMatrix4 &proj_tm);

  void closeCloudsTex();
  void initCloudsTex();
  void prepareSkies(bool invalidate_cpu = false);

  void initTracer();
  void closeTracer();

  void initCloudsRender(bool useHoles);
  void initPerlinNoise();

  void renderStrataClouds(const Point3 &origin, const TMatrix &view_tm, const TMatrix4 &proj_tm, bool panorama);
  void renderCloudsFullRes(const Point3 &origin, TEXTUREID targetDepth);
  void setCloudsVars();

  bool updatePanorama(const Point3 &origin, const TMatrix &view_tm, const TMatrix4 &proj_tm);
  void renderPanorama(const Point3 &origin, const TMatrix &view_tm, const TMatrix4 &proj_tm, const Driver3dPerspective &persp);
  void downsamplePanoramaDepth(UniqueTex &depth, UniqueTexHolder &downsampled_depth);
  void createPanoramaDepthTexHelper(UniqueTex &depth, const char *depth_name, UniqueTexHolder &downsampled_depth,
    const char *downsampled_depth_name, int w, int h, int addru, int addrv);
  void createPanoramaDepthTex();
  void createDepthPanoramaPatchTex();

  void createCloudsPanoramaTex(bool blend_two, int resolution_width, int resolution_height);
  void clearUncompressedPanoramaTex();

  void startUseOfCompressedTex();
  void initPanoramaCompression(int width, int height);

  virtual bool canRenderStars() const { return skyStars; }

  bool isPanoramaDepthTexReady = false;
  struct CloudsOrigin
  {
    double x = 0, z = 0;
    double strataX = 0, strataZ = 0;
    bool operator==(const CloudsOrigin &a) const { return x == a.x && z == a.z && strataX == a.strataX && strataZ == a.strataZ; }
    bool operator!=(const CloudsOrigin &a) const { return !(*this == a); }
  } cloudsOrigin;

  enum
  {
    PANORAMA_INVALID,
    PANORAMA_OLD,
    PANORAMA_VALID,
    PANORAMA_WAIT_RESOURCES
  };
  uint32_t panoramaValid;
  PostFxRenderer skyPanorama, cloudsPanorama, cloudsAlphaPanorama;
  PostFxRenderer applyCloudsPanorama;

  float panoramaRGBMScaleFactor = 1.f;
  // values more than 15 make compression and rgbm artifacts more noticeable
  static constexpr float maxRGBMScaleFactor = 15.f;
  bool isPanoramaCompressed{false};
  bool clearUncompressedPanorama{false};

  bool skyPanoramaPatchEnabled{true};
  UniqueTex compressedCloudsPanoramaTex;
  eastl::unique_ptr<PanoramaCompressor> panoramaCompressor{nullptr};

  UniqueTex cloudsPanoramaSplitTex;
  bool cleanupPanoramaSplitTex = false;
  bool allowCloudsPanoramaSplit = false;
  d3d::RenderPass *cloudsPanoramaSplitRP = nullptr;
  void createCloudsPanoramaSplitResources(int resolution_width, int resolution_height);
  void destroyCloudsPanoramaSplitResources();
  void createCloudsPanoramaSplitTex(int resolution_width, int resolution_height);

  carray<UniqueTex, 3> cloudsPanoramaTex, cloudsPanoramaPatchTex; // fixme: we need couple for blending + 1 for 'volume fog'
  UniqueTex cloudsDepthPanoramaTex, cloudsDepthPanoramaPatchTex;
  UniqueTexHolder cloudsDepthDownsampledPanoramaTex, cloudsDepthDownsampledPanoramaPatchTex;
  UniqueTexHolder cloudsAlphaPanoramaTex;
  UniqueTexHolder skyPanoramaTex;
  UniqueTexHolder cloudsPanoramaMipTex;
  PostFxRenderer cloudsPanoramaMip;

  bool panoramaTemporarilyDisabled = false;

  bool panoramaShouldWaitResourcesLoaded = false;

  struct PanoramaSavedSettings
  {
    CloudsOrigin cloudsOrigin;
#define COLOR4_PARAM(a) Color4 a;
#define FLOAT_PARAM(a)  float a;

#define PANORAMA_PARAMETERS                   \
  COLOR4_PARAM(skies_primary_sun_light_dir)   \
  COLOR4_PARAM(skies_secondary_sun_light_dir) \
  COLOR4_PARAM(skies_primary_sun_color)       \
  COLOR4_PARAM(skies_secondary_sun_color)     \
  COLOR4_PARAM(real_skies_sun_light_dir)      \
  COLOR4_PARAM(real_skies_sun_color)          \
  COLOR4_PARAM(skies_moon_color);             \
  COLOR4_PARAM(skies_moon_dir);               \
  FLOAT_PARAM(strata_pos_x);                  \
  FLOAT_PARAM(strata_pos_z);

    PANORAMA_PARAMETERS

#undef COLOR4_PARAM
#undef FLOAT_PARAM

    PanoramaSavedSettings()
    {

#define COLOR4_PARAM(a) a = Color4(0, 0, 0, 0);
#define FLOAT_PARAM(a)  a = 0;
      PANORAMA_PARAMETERS
#undef COLOR4_PARAM
#undef FLOAT_PARAM
    }
  };
  void get_panorama_settings(PanoramaSavedSettings &a);
  void set_panorama_settings(const PanoramaSavedSettings &a);
  PostFxRenderer cloudsPanoramaBlend;
  PreparedSkies *panoramaData = nullptr;
  PanoramaSavedSettings panoramaVariables;
  int targetPanorama = 0, currentPanorama = 0;
  uint32_t panoramaFrame = 0, panoramaValidFrame = 0, panoramaInvalidFrames = 0;
  uint32_t panoramaScatteringGeneration = 0, panoramaCloudsGeneration = 0;
  float panorama_sun_light_color_brightness = 0.f;
  Point3 panorama_sun_light_dir = {0, 0, 0};
  Point3 panorama_render_origin = {0, 0, 0};
  float panoramaReprojectionWeight = 0.5f; // 0 - we don't use reprojection, 1 - we fully reproject
  bool panoramaAllowAsyncPipelineFeedback = false;

  Point3 real_skies_sun_light_dir;
  float moonEffect = 0, sunEffect = 1;

  // primary sun is either real sun, or moon, depending on what is brighter
  Point3 primarySunDir = {0, 1, 0}, secondarySunDir = {0, 1, 0};
  Color3 primarySunColor = {0, 0, 0}, secondarySunColor = {0, 0, 0};
  float primarySunEffect = 0, secondarySunEffect = 0;
  bool moonIsPrimary = false;

  uint32_t computedScatteringGeneration = 0, // multiple scatttering computation generation
    nextScatteringGeneration = 0, cloudsGeneration = 0;
  DaScattering skies;

  PostFxRenderer strata;
  PostFxRenderer skiesApply;
  SharedTexHolder strataClouds;
  MipRenderer mipRenderer;
  float averageCloudsDensity = 0.5; // todo:

  // default min thickness, works for WT (for the most part)
  float minCloudThickness = 1.7f;

  SkyAtmosphereParams skyParams;
  StrataClouds strataCloudsParams;
  CloudsSettingsParams cloudsSettings;
  CloudsWeatherGen cloudsWeatherGen;
  CloudsForm cloudsForm;
  CloudsRendering cloudsRendering;

  bool cpuOnly; // server
  class DaStars *skyStars;
  float moonAge;
  Point3 moonDir;
  double starsJulianDay;
  float starsLatitude, starsLongtitude;
  float initialAzimuthAngle;

  bool shouldRenderStrata;
  uint32_t prepareFrame = 0;

  Clouds2 *clouds = nullptr;
  float moon_check_ht = 15000;

  bool renderDownsampledDepth = false;
  static constexpr uint32_t panoramaDepthTexMipNumber = 2;

  eastl::unique_ptr<ComputeShaderElement> traceCloudsCs;
  PostFxRenderer traceCloudsPs;
  RingCPUBufferLock cloudsTraceResultRingBuffer;
  int traceRaysCountVarId = -1;
  void initCloudsTracer();
  static const int numCloudsTracesPerGroup = 32;
  static const int numCloudsTracesPerFrame = numCloudsTracesPerGroup * 32;
  static const int cloudsTraceHistorySize = numCloudsTracesPerGroup * 256;
  static const int numCloudsTracesPerFramePs = numCloudsTracesPerGroup;
  struct CloudsTrace
  {
    enum class State
    {
      FREE,
      ADDED,
      DISPATCHED,
      FINISHED
    };
    State state = State::FREE;
    Point3 inPos = Point3(0.f, 0.f, 0.f);
    Point3 inDir = Point3(0.f, 0.f, 0.f);
    float inDist = 0.f;
    float inThreshold = 0.f;
    float inScatteringThreshold = 0.f;
    float outT = 0.f;
    float outTransmittance = 0.f;
  };
  carray<CloudsTrace, cloudsTraceHistorySize> cloudsTraces;
  uint64_t currentCloudsTraceId = 0;
  uint64_t currentCloudsTraceDispatchId = 0;

  struct CloudsTracesInFlight
  {
    int frameNo;
    uint64_t startTraceId;
    int dispatchSize;
    uint32_t dispatchedAtFrame;
  };
  eastl::vector<CloudsTracesInFlight> cloudsTracesInFlight;

  eastl::unique_ptr<ComputeShaderElement> rainQueryCs;
  RingCPUBufferLock rainQueryResultRingBuffer;
  float lastRainQueryResult = 0.f;
  void initRainQuery();
};

#define _PARAM(type, name, def_value)                      a.name == name &&
#define _PARAM_RAND(type, name, def_value)                 a.name == name &&
#define _PARAM_RAND_CLAMP(type, name, def_value, min, max) _PARAM_RAND(type, name, def_value)
inline bool DaSkies::CloudsSettingsParams::operator==(const DaSkies::CloudsSettingsParams &a) const
{
  return CLOUDS_SETTINGS_PARAMS true;
}
inline bool DaSkies::CloudsWeatherGen::operator==(const DaSkies::CloudsWeatherGen &a) const { return CLOUDS_WEATHER_GEN_PARAMS true; }
inline bool DaSkies::CloudsForm::operator==(const DaSkies::CloudsForm &a) const { return CLOUDS_FORM_PARAMS true; }
inline bool DaSkies::CloudsRendering::operator==(const DaSkies::CloudsRendering &a) const { return CLOUDS_RENDERING_PARAMS true; }
inline bool DaSkies::StrataClouds::operator==(const DaSkies::StrataClouds &a) const { return STRATA_CLOUDS_PARAMS true; }
#undef _PARAM
#undef _PARAM_RAND
#undef _PARAM_RAND_CLAMP
