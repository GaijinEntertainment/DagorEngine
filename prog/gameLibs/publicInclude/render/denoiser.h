//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <3d/dag_textureIDHolder.h>
#include <drv/3d/dag_tex3d.h>
#include <memory/dag_framemem.h>
#include <EASTL/vector_map.h>

namespace denoiser
{

struct Config
{
  int width = 0;
  int height = 0;
  bool useRayReconstruction = false;
  void init(int w, int h, bool use_rr)
  {
    width = w;
    height = h;
    useRayReconstruction = use_rr;
  }
  struct RTRSM
  {
    int width = 0;
    int height = 0;
  } rtsm;
  void initRTSM()
  {
    rtsm.width = width;
    rtsm.height = height;
  }
  void closeRTSM() { rtsm = {}; }
  struct RTR
  {
    int width = 0;
    int height = 0;
    bool isHalfRes = false;
    bool isCheckerboard = false;
  } rtr;
  void initRTR(bool is_half_res, bool is_checkerboard)
  {
    if (useRayReconstruction)
    {
      G_ASSERT(is_half_res == false && is_checkerboard == false);
      rtr.width = width;
      rtr.height = height;
      rtr.isHalfRes = rtr.isCheckerboard = false;
    }
    else
    {
      rtr.width = is_half_res ? width / 2 : width;    // TODO: Ceil division.
      rtr.height = is_half_res ? height / 2 : height; // TODO: Ceil
      rtr.isHalfRes = is_half_res;
      rtr.isCheckerboard = is_checkerboard;
    }
  }
  void closeRTR() { rtr = {}; }
  struct RTAO
  {
    int width = 0;
    int height = 0;
    bool isHalfRes = false;
  } rtao;
  void initRTAO(bool is_half_res)
  {
    if (useRayReconstruction)
    {
      G_ASSERT(is_half_res == false);
      rtao.width = width;
      rtao.height = height;
      rtao.isHalfRes = false;
    }
    else
    {
      rtao.width = is_half_res ? width / 2 : width;    // TODO: Ceil division.
      rtao.height = is_half_res ? height / 2 : height; // TODO: Ceil
      rtao.isHalfRes = is_half_res;
    }
  }
  void closeRTAO() { rtao = {}; }
  struct PTGI
  {
    int width = 0;
    int height = 0;
    bool isHalfRes = false;
  } ptgi;
  void initPTGI(bool is_half_res)
  {
    if (useRayReconstruction)
    {
      G_ASSERT(is_half_res == false);
      ptgi.width = width;
      ptgi.height = height;
      ptgi.isHalfRes = false;
    }
    else
    {
      ptgi.width = is_half_res ? width / 2 : width;    // TODO: Ceil division.
      ptgi.height = is_half_res ? height / 2 : height; // TODO: Ceil
      ptgi.isHalfRes = is_half_res;
    }
  }
  void closePTGI() { ptgi = {}; }
  void closeAll() { *this = {}; }
};
extern Config resolution_config;

using TexMap = eastl::vector_map<const char *, TextureIDPair, eastl::less<const char *>, framemem_allocator>;
using TexInfoMap = eastl::vector_map<const char *, TextureInfo, eastl::less<const char *>, framemem_allocator>;

struct FrameParams
{
  Point3 viewPos = Point3::ZERO;
  Point3 prevViewPos = Point3::ZERO;
  Point3 viewDir = Point3::ZERO;
  Point3 prevViewDir = Point3::ZERO;
  TMatrix4 viewItm = TMatrix4::IDENT;
  TMatrix4 projTm = TMatrix4::IDENT;
  TMatrix4 prevViewItm = TMatrix4::IDENT;
  TMatrix4 prevProjTm = TMatrix4::IDENT;
  Point2 jitter = Point2::ZERO;
  Point2 prevJitter = Point2::ZERO;
  Point3 motionMultiplier = Point3::ZERO;
  bool reset = false;

  TexMap textures;
};

enum class ReflectionMethod
{
  Reblur,
  Relax,
};

bool is_available();
void initialize(int width, int height, bool use_ray_reconstruction);
void teardown();

bool is_ray_reconstruction_enabled();

void prepare(const FrameParams &params);

// The RT system is working with the pointer values, not the string contents!
void get_required_persistent_texture_descriptors(TexInfoMap &persistent_textures, bool need_half_res);

void get_required_persistent_texture_descriptors_for_ao(TexInfoMap &persistent_textures);
void get_required_transient_texture_descriptors_for_ao(TexInfoMap &transient_textures);

void get_required_persistent_texture_descriptors_for_rtr(TexInfoMap &persistent_textures, ReflectionMethod method);
void get_required_transient_texture_descriptors_for_rtr(TexInfoMap &transient_textures, ReflectionMethod method);

void get_required_persistent_texture_descriptors_for_rtsm(TexInfoMap &persistent_textures, bool translucent);
void get_required_transient_texture_descriptors_for_rtsm(TexInfoMap &transient_textures, bool translucent);

void get_required_persistent_texture_descriptors_for_gi(TexInfoMap &persistent_textures);
void get_required_transient_texture_descriptors_for_gi(TexInfoMap &transient_textures);

struct TextureNames
{
#define ENUM_PERSISTENT_NAMES          \
  NAME(denoiser_normal_roughness)      \
  NAME(denoiser_view_z)                \
  NAME(denoiser_half_normal_roughness) \
  NAME(denoiser_half_view_z)

#define NAME(name) static const char *name;
  ENUM_PERSISTENT_NAMES
#undef NAME

  static const char *motion_vectors;
  static const char *half_motion_vectors;
  static const char *half_normals;
  static const char *half_depth;
  static const char *reblur_history_confidence;
};

struct ShadowDenoiser
{
  static constexpr int MAX_HISTORY_FRAME_NUM = 7;

  Point3 lightDirection;
  int maxStabilizedFrameNum = 5;
  Texture *csmTexture = nullptr;
  d3d::SamplerHandle csmSampler = d3d::INVALID_SAMPLER_HANDLE;
  Texture *vsmTexture = nullptr;
  d3d::SamplerHandle vsmSampler = d3d::INVALID_SAMPLER_HANDLE;

  TexMap textures;

#define ENUM_RTSM_DENOISED_PERSISTENT_NAMES \
  NAME(rtsm_value)                          \
  NAME(rtsm_shadows_denoised)               \
  NAME(rtsm_historyLengthPersistent)
#define ENUM_RTSM_DENOISED_PERSISTENT_OPTIONAL_NAMES NAME(rtsm_translucency)
#define ENUM_RTSM_DENOISED_TRANSIENT_NAMES \
  NAME(rtsm_tiles)                         \
  NAME(rtsm_smoothTiles)                   \
  NAME(rtsm_denoiseData1)                  \
  NAME(rtsm_denoiseData2)                  \
  NAME(rtsm_denoiseTemp1)                  \
  NAME(rtsm_denoiseTemp2)                  \
  NAME(rtsm_denoiseHistory)                \
  NAME(rtsm_denoiseHistoryLengthTransient)
#define ENUM_RTSM_DENOISED_NAMES \
  ENUM_RTSM_DENOISED_PERSISTENT_NAMES ENUM_RTSM_DENOISED_PERSISTENT_OPTIONAL_NAMES ENUM_RTSM_DENOISED_TRANSIENT_NAMES

  struct TextureNames
  {
#define NAME(name) static const char *name;
    ENUM_RTSM_DENOISED_NAMES
#undef NAME
  };
};

struct AODenoiser
{
  static constexpr int MAX_HISTORY_FRAME_NUM = 63;

  Point4 hitDistParams = Point4::ZERO;
  Point2 antilagSettings = Point2(4, 3);
  int maxStabilizedFrameNum = MAX_HISTORY_FRAME_NUM;
  bool performanceMode = true;
  bool halfResolution = false;
  bool checkerboard = true;
  TexMap textures;

#define ENUM_AO_DENOISED_PERSISTENT_NAMES \
  NAME(rtao_tex_unfiltered)               \
  NAME(denoised_ao)                       \
  NAME(ao_prev_view_z)                    \
  NAME(ao_prev_normal_roughness)          \
  NAME(ao_prev_internal_data)             \
  NAME(ao_diff_fast_history)
#define ENUM_AO_DENOISED_TRANSIENT_NAMES \
  NAME(ao_tiles)                         \
  NAME(ao_diffTemp2)                     \
  NAME(ao_diffFastHistory)               \
  NAME(ao_data1)
#define ENUM_AO_DENOISED_NAMES ENUM_AO_DENOISED_PERSISTENT_NAMES ENUM_AO_DENOISED_TRANSIENT_NAMES

  struct TextureNames
  {
#define NAME(name) static const char *name;
    ENUM_AO_DENOISED_NAMES
#undef NAME
  };
};

struct GIDenoiser
{
  static constexpr int MAX_HISTORY_FRAME_NUM = 63;

  Point4 hitDistParams = Point4::ZERO;
  Point2 antilagSettings = Point2(4, 3);
  int maxStabilizedFrameNum = MAX_HISTORY_FRAME_NUM;
  bool performanceMode = true;
  bool checkerboard = true;
  bool halfResolution = true;
  TexMap textures;

#define ENUM_GI_DENOISED_PERSISTENT_NAMES \
  NAME(ptgi_tex_unfiltered)               \
  NAME(denoised_gi)                       \
  NAME(gi_prev_view_z)                    \
  NAME(gi_prev_normal_roughness)          \
  NAME(gi_prev_internal_data)             \
  NAME(gi_diff_history)                   \
  NAME(gi_diff_fast_history)              \
  NAME(gi_diff_history_stabilized_ping)   \
  NAME(gi_diff_history_stabilized_pong)
#define ENUM_GI_DENOISED_TRANSIENT_NAMES \
  NAME(gi_data1)                         \
  NAME(gi_data2)                         \
  NAME(gi_diffTemp2)                     \
  NAME(gi_diffFastHistory)               \
  NAME(gi_tiles)
#define ENUM_GI_DENOISED_NAMES ENUM_GI_DENOISED_PERSISTENT_NAMES ENUM_GI_DENOISED_TRANSIENT_NAMES

  struct TextureNames
  {
#define NAME(name) static const char *name;
    ENUM_GI_DENOISED_NAMES
#undef NAME
  };
};

struct ReflectionDenoiser
{
  static constexpr int REBLUR_MAX_HISTORY_FRAME_NUM = 63;
  static constexpr int RELAX_MAX_HISTORY_FRAME_NUM = 255;

  ReflectionMethod method = ReflectionMethod::Reblur;
  Point4 hitDistParams = Point4::ZERO;
  Point2 antilagSettings = Point2(4, 3);
  int maxStabilizedFrameNum = 31;
  int maxFastStabilizedFrameNum = 6;
  bool halfResolution = false;
  bool antiFirefly = false;
  bool performanceMode = true;
  bool checkerboard = true;
  TexMap textures;

#define ENUM_RTR_DENOISED_REBLUR_PERSISTENT_NAMES \
  NAME(reblur_spec_prev_view_z)                   \
  NAME(reblur_spec_prev_normal_roughness)         \
  NAME(reblur_spec_prev_internal_data)            \
  NAME(reblur_spec_history)                       \
  NAME(reblur_spec_fast_history)                  \
  NAME(reblur_spec_history_stabilized_ping)       \
  NAME(reblur_spec_history_stabilized_pong)       \
  NAME(reblur_spec_hitdist_for_tracking_ping)     \
  NAME(reblur_spec_hitdist_for_tracking_pong)
#define ENUM_RTR_DENOISED_RELAX_PERSISTENT_NAMES \
  NAME(relax_spec_illum_prev)                    \
  NAME(relax_spec_illum_responsive_prev)         \
  NAME(relax_spec_reflection_hit_t_curr)         \
  NAME(relax_spec_reflection_hit_t_prev)         \
  NAME(relax_spec_history_length_prev)           \
  NAME(relax_spec_normal_roughness_prev)         \
  NAME(relax_spec_material_id_prev)              \
  NAME(relax_spec_view_z_prev)
#define ENUM_RTR_DENOISED_SHARED_PERSISTENT_NAMES \
  NAME(rtr_tex_unfiltered)                        \
  NAME(rtr_tex)                                   \
  NAME(rtr_sample_tiles)
#define ENUM_RTR_DENOISED_PERSISTENT_NAMES  \
  ENUM_RTR_DENOISED_SHARED_PERSISTENT_NAMES \
  ENUM_RTR_DENOISED_REBLUR_PERSISTENT_NAMES \
  ENUM_RTR_DENOISED_RELAX_PERSISTENT_NAMES
#define ENUM_RTR_DENOISED_REBLUR_TRANSIENT_NAMES \
  NAME(rtr_data1)                                \
  NAME(rtr_data2)                                \
  NAME(rtr_hitdistForTracking)                   \
  NAME(rtr_tmp2)                                 \
  NAME(rtr_fastHistory)                          \
  NAME(rtr_reblur_tiles)
#define ENUM_RTR_DENOISED_RELAX_TRANSIENT_NAMES \
  NAME(rtr_historyLength)                       \
  NAME(rtr_specReprojectionConfidence)          \
  NAME(rtr_specIllumPing)                       \
  NAME(rtr_specIllumPong)                       \
  NAME(rtr_relax_tiles)
#define ENUM_RTR_DENOISED_SHARED_TRANSIENT_NAMES
#define ENUM_RTR_DENOISED_TRANSIENT_NAMES  \
  ENUM_RTR_DENOISED_SHARED_TRANSIENT_NAMES \
  ENUM_RTR_DENOISED_REBLUR_TRANSIENT_NAMES \
  ENUM_RTR_DENOISED_RELAX_TRANSIENT_NAMES
#define ENUM_RTR_DENOISED_NAMES ENUM_RTR_DENOISED_PERSISTENT_NAMES ENUM_RTR_DENOISED_TRANSIENT_NAMES

  struct TextureNames
  {
#define NAME(name) static const char *name;
    ENUM_RTR_DENOISED_NAMES
#undef NAME
    static const char *rtr_validation;
  };
};

void denoise_shadow(const ShadowDenoiser &params);
void denoise_ao(const AODenoiser &params);
void denoise_gi(const GIDenoiser &params);
void denoise_reflection(const ReflectionDenoiser &params);

int get_frame_number();

} // namespace denoiser

#define ACQUIRE_DENOISER_TEXTURE(p, name)                                                          \
  Texture *name = nullptr;                                                                         \
  TEXTUREID name##_id = BAD_TEXTUREID;                                                             \
  {                                                                                                \
    auto name##Iter = p.textures.find(eastl::remove_reference_t<decltype(p)>::TextureNames::name); \
    if (name##Iter == p.textures.end())                                                            \
    {                                                                                              \
      G_ASSERT(false);                                                                             \
      return;                                                                                      \
    }                                                                                              \
    name = name##Iter->second.getTex2D();                                                          \
    name##_id = name##Iter->second.getId();                                                        \
  }                                                                                                \
  if (!name)                                                                                       \
  {                                                                                                \
    G_ASSERT(false);                                                                               \
    return;                                                                                        \
  }

#define ACQUIRE_OPTIONAL_DENOISER_TEXTURE(p, name)                                                 \
  Texture *name = nullptr;                                                                         \
  TEXTUREID name##_id = BAD_TEXTUREID;                                                             \
  {                                                                                                \
    auto name##Iter = p.textures.find(eastl::remove_reference_t<decltype(p)>::TextureNames::name); \
    if (name##Iter != p.textures.end())                                                            \
    {                                                                                              \
      name = name##Iter->second.getTex2D();                                                        \
      name##_id = name##Iter->second.getId();                                                      \
    }                                                                                              \
  }
