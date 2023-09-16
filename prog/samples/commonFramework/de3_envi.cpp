#include "de3_envi.h"
#include "de3_worldrenderer.h"
#include "de3_ICamera.h"
#include <render/fx/dag_postfx.h>
#include <render/fx/dag_demonPostFx.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaders.h>
#include <render/shaderVars.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_render.h>
#include <debug/dag_debug.h>

int SceneEnviSettings::fromSunDirectionGvId = -1, SceneEnviSettings::sunColor0GvId = -1, SceneEnviSettings::sunLightDir1GvId = -1,
    SceneEnviSettings::sunColor1GvId = -1, SceneEnviSettings::skyColorGvId = -1, SceneEnviSettings::sunFogColorGvId = -1,
    SceneEnviSettings::skyFogColorGvId = -1, SceneEnviSettings::skyScaleGvId = -1;

void SceneEnviSettings::init()
{
  fromSunDirectionGvId = ::get_shader_glob_var_id("from_sun_direction", false);
  sunColor0GvId = ::get_shader_glob_var_id("sun_color_0", false);
  sunLightDir1GvId = ::get_shader_glob_var_id("sun_light_dir_1", true);
  sunColor1GvId = ::get_shader_glob_var_id("sun_color_1", true);
  skyColorGvId = ::get_shader_glob_var_id("sky_color", true);
  sunFogColorGvId = ::get_shader_glob_var_id("sun_fog_color_and_density", true);
  skyFogColorGvId = ::get_shader_glob_var_id("sky_fog_color_and_density", true);
  skyScaleGvId = get_shader_glob_var_id("sky_scale", true);
}

void SceneEnviSettings::applyOnRender(bool apply_zrange)
{
  if (apply_zrange)
  {
    Driver3dPerspective p;
    if (d3d::getpersp(p))
    {
      p.zn = zRange[0];
      p.zf = zRange[1];
      d3d::setpersp(p);
    }
  }

  if (useFog)
  {
    // Formulae from EnvironmentPlugin::renderObjects()
    Point2 view = normalize(Point2(::grs_cur_view.itm.getcol(2).x, ::grs_cur_view.itm.getcol(2).z));

    // to syncronize with DaEditor (should use set_pogrammable_fog())
    float baseInterp = (view * Point2(cosf(sunAzimuth), sinf(sunAzimuth)) + 1) / 2.0f;

    float interp = baseInterp * fx.fogSunUseFactor;

    Color4 baseFogColor = color4(fogColor) * (1 - interp) + color4(sunFogColor) * interp;
    ::grs_cur_fog.color = e3dcolor(lerp(baseFogColor, color4(fx.fogColor, 0), fx.fogColorBlend));
    ::grs_cur_fog.dens = lerp(fogDensity, sunFogDensity, interp) * fx.fogDensityFactor;

    float fogDensity = ::grs_cur_fog.dens;
    ShaderGlobal::set_color4_fast(sunFogColorGvId, color4(color3(grs_cur_fog.color), fogDensity));
    ShaderGlobal::set_color4_fast(skyFogColorGvId, color4(color3(grs_cur_fog.color), fogDensity));

    Color3 lfogSkyColorBase = color3(lfogSkyColor);
    Color3 lfogSunColorBase = lerp(color3(lfogSkyColor), color3(lfogSunColor), fx.fogSunUseFactor);

    Color3 lfogSkyColor = lerp(lfogSkyColorBase, fx.fogColor, fx.fogColorBlend);
    Color3 lfogSunColor = lerp(lfogSunColorBase, fx.fogColor, fx.fogColorBlend);

    shadervars::set_layered_fog(sunAzimuth, lfogDensity * fx.fogDensityFactor, lfogMinHeight, lfogMaxHeight, lfogSkyColor,
      lfogSunColor);
  }
}
void SceneEnviSettings::setBiDirLighting()
{
  /*if (brightness(sunColor1) > brightness(sunColor0) && sunDir1.lengthSq()>0.1)
  {
    Color3 col = sunColor1;
    Point3 dir = sunDir1;
    sunColor1 = sunColor0;
    sunDir1 = sunDir0;
    sunColor0 = col;
    sunDir0 = dir;
    debug("sun0 and sun1 swapped!");
  }*/

  ShaderGlobal::set_color4_fast(skyColorGvId, Color4(skyColor.r, skyColor.g, skyColor.b, 0.f));
  ShaderGlobal::set_color4_fast(fromSunDirectionGvId, Color4(sunDir0.x, sunDir0.y, sunDir0.z, 0.f));
  ShaderGlobal::set_color4_fast(sunColor0GvId, Color4(sunColor0.r, sunColor0.g, sunColor0.b, 0.f));
  ShaderGlobal::set_color4_fast(sunLightDir1GvId, Color4(sunDir1.x, sunDir1.y, sunDir1.z, 0.f));
  ShaderGlobal::set_color4_fast(sunColor1GvId, Color4(sunColor1.r, sunColor1.g, sunColor1.b, 0.f));

  ShaderGlobal::set_real_fast(skyScaleGvId, skyScale);
}
