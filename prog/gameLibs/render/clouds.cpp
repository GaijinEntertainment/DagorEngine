#include <shaders/dag_shaders.h>
#include <3d/dag_texMgr.h>
#include <math/dag_Point2.h>
#include <math/dag_color.h>
#include <render/clouds.h>

static TEXTUREID enviCloudsTextureId = BAD_TEXTUREID;
static int enviCloudsTextureParamId = -1;
static int enviTexMoveParamId = -1;

void init_clouds(const char *clouds_tex_name, const Point2 &scale, const Point2 &speed)
{
  enviCloudsTextureParamId = ::get_shader_glob_var_id("environmentCloudsTex", true);
  if (enviCloudsTextureParamId != -1)
  {
    enviCloudsTextureId = ::get_managed_texture_id(clouds_tex_name);
    if (enviCloudsTextureId == BAD_TEXTUREID)
      enviCloudsTextureId = ::add_managed_texture(clouds_tex_name);
    ::acquire_managed_tex(enviCloudsTextureId);
    ShaderGlobal::set_texture_fast(enviCloudsTextureParamId, enviCloudsTextureId);

    enviTexMoveParamId = ::get_shader_glob_var_id("environmentCloudsMove");
    ShaderGlobal::set_color4_fast(enviTexMoveParamId, Color4(scale.x, scale.y, speed.x, speed.y));
  }
}

void close_clouds()
{
  ShaderGlobal::set_texture_fast(enviCloudsTextureParamId, BAD_TEXTUREID);
  ::release_managed_tex(enviCloudsTextureId);
  enviCloudsTextureId = BAD_TEXTUREID;
  enviCloudsTextureParamId = -1;
  enviTexMoveParamId = -1;
}

void change_clouds_tex(TEXTUREID tex_id)
{
  if (tex_id != BAD_TEXTUREID)
    ShaderGlobal::set_texture_fast(enviCloudsTextureParamId, tex_id);
  else
    ShaderGlobal::set_texture_fast(enviCloudsTextureParamId, enviCloudsTextureId);
}
