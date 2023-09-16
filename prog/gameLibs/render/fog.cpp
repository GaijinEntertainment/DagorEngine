#include <math/dag_color.h>
#include <math/dag_e3dColor.h>
#include <render/shaderVars.h>
#include <3d/dag_render.h>
#include <shaders/dag_shaders.h>


namespace shadervars
{

void set_progammable_fog(float sun_fog_azimuth, float sky_dens, float sun_dens, const Color3 &sky_color, const Color3 &sun_color)
{
  if (fog_color_and_density_glob_varId < 0 && sky_fog_color_and_density_glob_varId < 0)
    init();
  Point2 view = normalize(Point2(::grs_cur_view.itm.getcol(2).x, ::grs_cur_view.itm.getcol(2).z));
  float interp = (view * Point2(cosf(sun_fog_azimuth), sinf(sun_fog_azimuth)) + 1) / 2.0f;

  ::grs_cur_fog.color = e3dcolor(::lerp(sun_color, sky_color, interp));

  ::grs_cur_fog.dens = ::lerp(sky_dens, sun_dens, interp);

  ShaderGlobal::set_color4_fast(fog_color_and_density_glob_varId,
    Color4(::grs_cur_fog.color.r / 255.f, ::grs_cur_fog.color.g / 255.f, ::grs_cur_fog.color.b / 255.f, ::grs_cur_fog.dens));

  ShaderGlobal::set_real_fast(sun_fog_azimuth_glob_varId, sun_fog_azimuth);

  ShaderGlobal::set_color4_fast(sun_fog_color_and_density_glob_varId, Color4(sun_color.r, sun_color.g, sun_color.b, sun_dens));

  ShaderGlobal::set_color4_fast(sky_fog_color_and_density_glob_varId, Color4(sky_color.r, sky_color.g, sky_color.b, sky_dens));
}

void set_layered_fog(float sun_fog_azimuth, float dens, float min_height, float max_height, const Color3 &sky_color,
  const Color3 &sun_color)
{

  ShaderGlobal::set_real_fast(sun_fog_azimuth_glob_varId, sun_fog_azimuth);

  ShaderGlobal::set_real_fast(layered_fog_density_glob_varId, dens);
  ShaderGlobal::set_real_fast(layered_fog_min_height_glob_varId, min_height);
  ShaderGlobal::set_real_fast(layered_fog_max_height_glob_varId, max_height);

  ShaderGlobal::set_color4_fast(layered_fog_sun_color_glob_varId, Color4(sun_color.r, sun_color.g, sun_color.b, 0));

  ShaderGlobal::set_color4_fast(layered_fog_sky_color_glob_varId, Color4(sky_color.r, sky_color.g, sky_color.b, 0));
}

} // namespace shadervars
