#include <render/rain.h>
#include <shaders/dag_shaders.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_texMgr.h>

RainDroplets::RainDroplets()
{
  puddleMapTexId = BAD_TEXTUREID;
  rain_scale_var_id = -1;
  // car_dropletsTexId = BAD_TEXTUREID;
  envi_dropletsTexId = BAD_TEXTUREID;
  inited = false;
}
void RainDroplets::init()
{
  if (inited)
    return;
  // car_dropletsTexId = BAD_TEXTUREID;
  envi_dropletsTexId = BAD_TEXTUREID;
  if (d3d::get_driver_desc().fshver > DDFSH_2_0 || getMaxFSHVersion() > FSHVER_R300)
  {
    VolTexture *droplets;
    envi_dropletsTexId = ::get_managed_texture_id("dynrender_envi_droplets*");
    if (envi_dropletsTexId == BAD_TEXTUREID)
      envi_dropletsTexId = ::add_managed_texture("tex/envi_droplets.ddsx");
    droplets = (VolTexture *)::acquire_managed_tex(envi_dropletsTexId);
    d3d_err(droplets);
    d3d_err(droplets->restype() == RES3D_VOLTEX);

    /*car_dropletsTexId = ::add_managed_texture("tex/droplets.ddsx");
    droplets = (VolTexture*)::acquire_managed_tex(car_dropletsTexId);
    d3d_err(droplets);
    d3d_err(droplets->restype()==RES3D_VOLTEX);*/
  }
  ShaderGlobal::set_texture(::get_shader_variable_id("envi_droplets", true), envi_dropletsTexId);
  // ShaderGlobal::set_texture(::get_shader_variable_id("car_droplets", true), car_dropletsTexId);
  rain_scale_var_id = ::get_shader_glob_var_id("rain_scale");

  puddleMapTexId = ::get_managed_texture_id("dynrender_puddle*");
  if (puddleMapTexId == BAD_TEXTUREID)
    puddleMapTexId = ::add_managed_texture("tex/puddle.ddsx");
  if (puddleMapTexId != BAD_TEXTUREID)
    ::acquire_managed_tex(puddleMapTexId);
  ShaderGlobal::set_texture(::get_shader_variable_id("puddle", true), puddleMapTexId);
  inited = true;
}
void RainDroplets::setRainRate(float rain_rate) { ShaderGlobal::set_real_fast(rain_scale_var_id, rain_rate); }
void RainDroplets::close()
{
  if (!inited)
    return;
  // ShaderGlobal::set_texture(::get_shader_variable_id("car_droplets"), BAD_TEXTUREID);
  ShaderGlobal::set_texture(::get_shader_variable_id("envi_droplets", true), BAD_TEXTUREID);
  /*  if ( car_dropletsTexId != BAD_TEXTUREID )
    {
      ::release_managed_tex(car_dropletsTexId);
      car_dropletsTexId = BAD_TEXTUREID;
    }
  */
  if (envi_dropletsTexId != BAD_TEXTUREID)
  {
    ::release_managed_tex(envi_dropletsTexId);
    envi_dropletsTexId = BAD_TEXTUREID;
  }

  ShaderGlobal::set_texture(::get_shader_variable_id("puddle", true), BAD_TEXTUREID);
  if (puddleMapTexId != BAD_TEXTUREID)
    ::release_managed_tex(puddleMapTexId);
  puddleMapTexId = BAD_TEXTUREID;

  inited = false;
}
