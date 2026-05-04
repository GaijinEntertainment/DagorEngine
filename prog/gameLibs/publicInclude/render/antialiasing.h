//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <resourcePool/resourcePool.h>
#include <3d/dag_textureIDHolder.h>
#include <render/daFrameGraph/daFG.h>

class TemporalAA;
class TemporalSuperResolution;
struct GameSettings;
class FlightControlMode;

namespace render
{
struct RenderView;
}

namespace render::antialiasing
{

enum class UpscalingQuality
{
  Native,
  UltraQualityPlus, // XeSS only
  UltraQuality,     // XeSS only
  Quality,
  Balanced,
  Performance,
  UltraPerformance,
};
constexpr UpscalingQuality firstUpscalingQuality = UpscalingQuality::Native;
constexpr UpscalingQuality lastUpscalingQuality = UpscalingQuality::UltraPerformance;

enum class AntialiasingMethod
{
  None,
  MSAA,
  FXAALow,
  FXAAHigh,
  TAA,
  MobileTAA,
  MobileTAALow,
  TSR,
  DLSS,
  XeSS,
  FSR,
  SGSR,
  METALFX,
  MOBILE_MSAA,
  SGSR2,
  SMAA,
  SSAA,
#if _TARGET_C2

#endif
};

enum class SettingsChange : int
{
  NeedDeviceReset = 0x01,
  NeedRecreate = 0x02,
};

struct ApplyContext
{
  TMatrix4 projection;
  TMatrix4 projectionInverse;
  TMatrix4 reprojection;
  TMatrix4 reprojectionInverse;
  TMatrix4 worldToView;
  TMatrix4 viewToWorld;

  Point3 position;
  Point3 up;
  Point3 right;
  Point3 forward;

  float nearZ = 0;
  float farZ = 0;
  float fov = 0;
  float aspect = 0;

  Texture *depthTexture = nullptr;
  Texture *motionTexture = nullptr;
  Texture *albedoTexture = nullptr;
  Texture *hitDistTexture = nullptr;
  Texture *normalRoughnessTexture = nullptr;
  Texture *specularAlbedoTexture = nullptr;
  Texture *reactiveTexture = nullptr;
  Texture *exposureTexture = nullptr;
  Texture *ssssGuideTexture = nullptr;
  Texture *colorBeforeTransparencyTexture = nullptr;
  Point2 jitterPixelOffset = Point2::ZERO;
  IPoint2 inputOffset = IPoint2::ZERO;
  IPoint2 inputResolution = IPoint2::ZERO;
  int viewIndex = 0;

  // Needed for FSR
  Driver3dPerspective persp;
  float timeElapsed = 0; // seconds

  // Needed for TSR
  Point4 depthTexTransform = Point4::ZERO;
  Texture *vrVrsMask = nullptr;
  bool resetHistory = false;

#if _TARGET_C2



#endif
};

struct FrameGenContext
{
  TMatrix viewItm;
  TMatrix4 noJitterProjTm;
  TMatrix4 noJitterGlobTm;
  TMatrix4 prevNoJitterGlobTm;
  Driver3dPerspective noJitterPersp;
  Texture *finalImageTexture;
  Texture *depthTexture;
  Texture *motionVectorTexture;
  Texture *uiTexture;
  float timeElapsed;
  Point2 jitterPixelOffset;
  bool resetHistory;
};

struct AppGlue
{
  struct SGSR2Interface
  {
    virtual ~SGSR2Interface() = default;
    virtual float getLodBias() const = 0;
    virtual void apply(Texture *sourceTex, Texture *destTex, bool reset = false) = 0;
    virtual void beforeRenderView(const RenderView &view) const = 0;
    virtual Point2 getJitterOffset() const = 0;
  };

  virtual SGSR2Interface *createSGSR2(const IPoint2 &input_resolution, const IPoint2 &output_resolution) const = 0;
  virtual TMatrix4 getUvReprojectionToPrevFrameTmNoJitter() const = 0;
  virtual IPoint2 getInputResolution() const = 0;
  virtual IPoint2 getOutputResolution() const = 0;
  virtual IPoint2 getDisplayResolution() const = 0;
  virtual bool hasMinimumRenderFeatures() const = 0;
  virtual bool isTiledRender() const = 0;
  virtual bool useMobileCodepath() const = 0;
  virtual bool isRayTracingEnabled() const = 0;
  virtual int getTargetFormat() const = 0;
  virtual int getHangarPassValue() const = 0;
  virtual bool isVrHmdEnabled() const = 0;
  virtual bool pushSSAAoption() const = 0;
};

void migrate(const char *config_name);

void init(AppGlue *app_glue);
void close();
void on_render_resolution_changed(const IPoint2 &rendering_resolution);

void reinit();
void recreate(const IPoint2 &display_resolution, const IPoint2 &postfx_resolution, IPoint2 &rendering_resolution,
  IPoint2 &min_dynamic_resolution, IPoint2 &max_dynamic_resolution, const char *input_name);

AntialiasingMethod get_method();
void set_method(AntialiasingMethod method);
AntialiasingMethod get_method_from_settings();
AntialiasingMethod get_method_from_name(const char *name);
const char *get_method_name(AntialiasingMethod method);
UpscalingQuality get_quality();
const char *get_quality_name(UpscalingQuality quality);
void set_quality_override(const char *quality_name);
bool is_quality_override_active();

bool is_valid_method_name(const char *name);
bool is_valid_upscaling_name(const char *name);

bool is_ssaa_compatible();

bool is_temporal();
bool need_motion_vectors();
bool support_dynamic_resolution();

int pull_settings_change();

bool need_prev_depth();
bool need_prev_motion_vectors();
bool need_reactive_tex(bool vr_mode);
bool need_reactive_tex_from_settings(bool vr_mode);

TemporalAA *get_taa();
TemporalSuperResolution *get_tsr();

float get_mip_bias();
void adjust_mip_bias(const IPoint2 &dynamic_resolution, const IPoint2 &output_resolution);

bool is_metalfx_upscale_supported();

void before_render_view(int view_index);
void before_render_frame();

Point2 get_jitter_offset(const RenderView &view, bool vr_mode);

void apply_fxaa(AntialiasingMethod method, Texture *src_color, Texture *src_depth, const Point4 &tc_scale_offset);
void apply_mobile_aa(Texture *source_tex, Texture *dest_tex, bool temporal_reset = false);

const char *get_available_methods(bool is_vr, bool names_only);
const char *get_available_upscaling_options(const char *method);

int process_console_cmd(const char *argv[], int argc);

bool need_gui_in_texture();

bool allows_noise();

void suppress_frame_generation(bool suppress);
void enable_frame_generation(bool enable);
void schedule_generated_frames(const FrameGenContext &ctx);
int get_supported_generated_frames(const char *method, bool exclusive_fullscreen);
const char *get_frame_generation_unsupported_reason(const char *method, bool exclusive_fullscreen);
int get_presented_frame_count();
bool is_frame_generation_enabled();
bool is_frame_generation_enabled_in_config();

unsigned int get_frame_after_aa_flags();

bool try_init_dlss(IPoint2 postfx_resolution, IPoint2 &rendering_resolution, const char *input_name = nullptr);
void apply_dlss(Texture *in_color, const ApplyContext &apply_context, Texture *target);
bool is_ray_reconstruction_enabled();
bool try_init_tsr(IPoint2 postfx_resolution, IPoint2 &rendering_resolution, const char *input_name = nullptr);
void apply_tsr(Texture *in_color, const ApplyContext &apply_context, Texture *out_color, Texture *history_color,
  Texture *out_confidence, Texture *history_confidence, Texture *reactive_tex);
bool try_init_fsr(IPoint2 postfx_resolution, IPoint2 &rendering_resolution, const char *input_name = nullptr);
void apply_fsr(Texture *in_color, const ApplyContext &apply_context, Texture *target);
bool try_init_xess(IPoint2 postfx_resolution, IPoint2 &rendering_resolution, const char *input_name = nullptr);
void apply_xess(Texture *in_color, const ApplyContext &apply_context, Texture *target);
#if _TARGET_C2


#endif

} // namespace render::antialiasing
