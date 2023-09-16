#include <shaders/dag_shaders.h>
#include <EASTL/unique_ptr.h>
#include <render/screenDroplets.h>
#include <3d/dag_tex3d.h>
#include <3d/dag_drv3d_multi.h>
#include <perfMon/dag_statDrv.h>
#include <math/integer/dag_IPoint2.h>
#include <3d/dag_resourcePool.h>
#include <util/dag_convar.h>

#define SCREEN_DROPLETS_VARS     \
  VAR(screen_droplets_setup)     \
  VAR(screen_droplets_intensity) \
  VAR(screen_droplets_has_leaks) \
  VAR(frame_tex)

#define VAR(a) static int a##VarId = -1;
SCREEN_DROPLETS_VARS
#undef VAR

eastl::unique_ptr<ScreenDroplets> screen_droplets_mgr;

static constexpr float EPS = 0.001f;
// Can be changed from code through setLeaks function, so the value set from console may be lost.
CONSOLE_BOOL_VAL("screenDroplets", hasLeaks, true);
CONSOLE_FLOAT_VAL_MINMAX("screenDroplets", onRainFadeoutRadius, 0.3, 0, 1);
// screenTime is some delay time for avoid situations when rain is starting and ending frequently
// for example when player goes in building and goes outside very fast
// so this delay will start rain not earlier when rain ended + delay time
// screenTime name comes from drops go through full screen by this time
CONSOLE_FLOAT_VAL_MINMAX("screenDroplets", screenTime, 6, 0.01, 20);
CONSOLE_FLOAT_VAL_MINMAX("screenDroplets", rainConeMax, 0, -1, 1);
CONSOLE_FLOAT_VAL_MINMAX("screenDroplets", rainConeOff, 1, -1, 1);
CONSOLE_BOOL_VAL("screenDroplets", intensityDirectControl, false);
CONSOLE_FLOAT_VAL_MINMAX("screenDroplets", intensityChangeRate, 1.0 / 6, 0, 10);

void init_screen_droplets_params(bool has_leaks, float rain_cone_max, float rain_cone_off)
{
  rainConeMax.set(rain_cone_max);
  rainConeOff.set(rain_cone_off);
  hasLeaks.set(has_leaks);
}

void init_screen_droplets_mgr(int w, int h, bool has_leaks, float rain_cone_max, float rain_cone_off)
{
  init_screen_droplets_params(has_leaks, rain_cone_max, rain_cone_off);
  screen_droplets_mgr = eastl::make_unique<ScreenDroplets>(w, h);
}

void init_screen_droplets_mgr(int w, int h, uint32_t rtFmt, bool has_leaks, float rain_cone_max, float rain_cone_off)
{
  init_screen_droplets_params(has_leaks, rain_cone_max, rain_cone_off);
  screen_droplets_mgr = eastl::make_unique<ScreenDroplets>(w, h, rtFmt);
}
void close_screen_droplets_mgr() { screen_droplets_mgr.reset(); }

ScreenDroplets::ScreenDroplets(int w, int h, uint32_t rtFmt)
{
  resolution = IPoint2(w, h);
  screenDropletsFx.init("screen_droplets", nullptr, false);
#define VAR(a) a##VarId = ::get_shader_variable_id(#a, true);
  SCREEN_DROPLETS_VARS
#undef VAR
  mipRenderer.init();

  rtTemp = RTargetPool::get(w, h, rtFmt | TEXCF_RTARGET, 2);
}


ScreenDroplets::ScreenDroplets(int w, int h)
{
  screenDropletsFx.init("screen_droplets", nullptr, false);
#define VAR(a) a##VarId = ::get_shader_variable_id(#a, true);
  SCREEN_DROPLETS_VARS
#undef VAR
  mipRenderer.init();
  resolution = IPoint2(w, h);
}

ScreenDroplets::~ScreenDroplets() { abortDrops(); }

void ScreenDroplets::setEnabled(bool is_enabled) { enabled = is_enabled; }

void ScreenDroplets::setRain(float force, const Point3 &rain_dir)
{
  float oldRainOutside = rainOutside;
  rainOutside = eastl::min(force, 1.0f - EPS);
  rainDir = rain_dir;
  if (force == 0.0 && oldRainOutside != 0.0) // Stop the rain smoothly
  {
    rainEnded = get_shader_global_time_phase(0, 0);
    updateShaderState();
  }
  else if (force < 0.0 && oldRainOutside != force) // Immediately stop the drops
  {
    abortDrops();
  }
}

void ScreenDroplets::setLeaks(bool has_leaks)
{
  if (hasLeaks == has_leaks)
    return;
  hasLeaks.set(has_leaks);
  const float currTime = get_shader_global_time_phase(0, 0);
  if ((rainEnded > rainStarted && rainEnded > currTime) || (rainStarted > rainEnded && rainStarted < currTime))
    rainStarted = currTime;

  // Disable drying out
  if (rainEnded > rainStarted && rainEnded < currTime) // if rain ended
    abortDrops();
  if (rainEnded < rainStarted && rainEnded < currTime) // if not started yet
    rainEnded = 0.0;

  updateShaderState();
}

void ScreenDroplets::setInVehicle(bool in_vehicle) { inVehicle = in_vehicle; }

void ScreenDroplets::abortDrops()
{
  rainStarted = rainEnded = rainCurrent = -screenTime;
  isUnderwaterLast = false;
  ShaderGlobal::set_color4(screen_droplets_setupVarId, 0, 0, 0, 100);
}

void ScreenDroplets::updateShaderState() const
{
  if (isUnderwaterLast)
  {
    ShaderGlobal::set_color4(screen_droplets_setupVarId, 0, 0, 0, 100);
    return;
  }

  const float currTime = get_shader_global_time_phase(0, 0);
  const float transition = eastl::min(1.0f, (currTime - splashStarted) / screenTime);
  const float fadeout = onRainFadeoutRadius * transition;
  float rainForce = 1.0f - EPS;
  if (rainCurrent > 0)
    rainForce = lerp(1.0f, rainCurrent, transition);

  ShaderGlobal::set_color4(screen_droplets_setupVarId, rainStarted, rainEnded, intensity * rainForce, fadeout);
  ShaderGlobal::set_real(screen_droplets_intensityVarId, max(EPS, intensity));
  ShaderGlobal::set_int(screen_droplets_has_leaksVarId, hasLeaks);
}

void ScreenDroplets::update(bool is_underwater, const TMatrix &itm, float dt)
{
  if (!enabled)
    return;

  const Point3 camera_pos = itm.getcol(3);
  const Point3 camera_up = itm.getcol(2);
  const Point3 traceStart = camera_pos;
  const Point3 direction = -rainDir;
  float t = 25.0f;

  float cameraDot = dot(camera_up, -rainDir);
  bool shouldBeRain =
    !inVehicle && (cameraDot > min(rainConeOff.get(), rainConeMax.get())) && !rayHitNormalized(traceStart, direction, t);

  if (rainConeMax - rainConeOff > EPS)
  {
    float targetIntensity = clamp((cameraDot - rainConeOff) / (rainConeMax - rainConeOff), 0.0f, 1.0f);
    if (intensityDirectControl)
      intensity = targetIntensity;
    else
    {
      if (targetIntensity - intensity > 0)
        intensity += min(dt * intensityChangeRate, targetIntensity - intensity);
      else
        intensity -= min(dt * intensityChangeRate, intensity - targetIntensity);
    }
  }
  else
    intensity = 1;

  if (shouldBeRain)
  {
    if (rainCurrent <= 0 && rainOutside > 0)
    {
      rainStarted = eastl::max(rainEnded + screenTime, get_shader_global_time_phase(0, 0));
      rainCurrent = rainOutside;
    }
  }
  else
  {
    if (rainCurrent > 0)
    {
      const float endTime = get_shader_global_time_phase(0, 0) + 1;
      rainEnded = eastl::min(endTime, rainEnded);
      if (endTime < rainStarted)
        rainStarted = 0;
      else
        rainEnded = endTime;
      rainCurrent = 0;
    }
  }

  if (isUnderwaterLast && !is_underwater)
  {
    intensity = 1;
    splashStarted = get_shader_global_time_phase(0, 0);
    if (rainCurrent <= 0)
    {
      rainStarted = 0;
      rainEnded = splashStarted;
    }
  }
  isUnderwaterLast = is_underwater;
  updateShaderState();
}

bool ScreenDroplets::isVisible() const
{
  return enabled && !(rainStarted <= rainEnded && get_shader_global_time_phase(0, 0) >= rainEnded + screenTime);
}

IPoint2 ScreenDroplets::getSuggestedResolution(int rendering_w, int rendering_h)
{
  // The droplets are blurred with a gaussian downsampling, so the screen space blur is resolution dependent.
  static constexpr int TARGET_W = 1920, TARGET_H = 1080;
  G_ASSERT_RETURN(rendering_w > 0 && rendering_h > 0, IPoint2(TARGET_W / 2, TARGET_H / 2));
  int w = TARGET_W, h = TARGET_H;
  if (rendering_w * TARGET_H > TARGET_W * rendering_h) // Wide Screen
  {
    h = min(rendering_h, TARGET_H);
    w = h * rendering_w / rendering_h;
  }
  else
  {
    w = min(rendering_w, TARGET_W);
    h = w * rendering_h / rendering_w;
  }
  return IPoint2(w / 2, h / 2);
}

RTarget::CPtr ScreenDroplets::render(TEXTUREID frame_tex)
{
  if (!isVisible())
    return nullptr;

  // TODO This is using full resolution current frame.
  // Wanted to use downsampled previous frame, but that doesn't contain transparent objects.
  ShaderGlobal::set_texture(frame_texVarId, frame_tex);

  RTarget::Ptr rTargetTemp = rtTemp->acquire();
  render(rTargetTemp->getTex2D());

  return rTargetTemp;
}

void ScreenDroplets::render(ManagedTexView rtarget, TEXTUREID frame_tex)
{
  if (!isVisible())
    return;
  ShaderGlobal::set_texture(frame_texVarId, frame_tex);
  render(rtarget.getTex2D());
}

void ScreenDroplets::render(BaseTexture *rtarget)
{
  rtarget->texfilter(TEXFILTER_SMOOTH);
  rtarget->texaddr(TEXADDR_CLAMP);
  TIME_D3D_PROFILE(screen_droplets);
  {
    SCOPE_RENDER_TARGET;
    d3d::set_render_target(rtarget, 0);
    screenDropletsFx.render();
    mipRenderer.render(rtarget);
  }
}

void ScreenDroplets::setConvars(const ConvarParams &convar_params)
{
  hasLeaks.set(convar_params.hasLeaks);
  onRainFadeoutRadius.set(convar_params.onRainFadeoutRadius);
  screenTime.set(convar_params.screenTime);
  rainConeMax.set(convar_params.rainConeMax);
  rainConeOff.set(convar_params.rainConeOff);
  intensityDirectControl.set(convar_params.intensityDirectControl);
  intensityChangeRate.set(convar_params.intensityChangeRate);
}
