#include <render/shaderVars.h>
#include <render/shaderBlockIds.h>

#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaders.h>
namespace shaderblocks
{
int frameBlkId = -1, staticSceneBlkId = -1, staticSceneDepthBlkId = -1, deferredSceneBlkId = -1, forwardSceneBlkId = -1;
static void init_shader_blocks(bool mandatory_shader_blocks)
{
  shaderblocks::frameBlkId = ShaderGlobal::getBlockId("global_frame", ShaderGlobal::LAYER_FRAME);
  shaderblocks::staticSceneBlkId = ShaderGlobal::getBlockId("static_scene", ShaderGlobal::LAYER_SCENE);
  shaderblocks::forwardSceneBlkId = ShaderGlobal::getBlockId("static_forward_scene", ShaderGlobal::LAYER_SCENE);
  shaderblocks::deferredSceneBlkId = ShaderGlobal::getBlockId("static_deferred_scene", ShaderGlobal::LAYER_SCENE);
  shaderblocks::staticSceneDepthBlkId = ShaderGlobal::getBlockId("static_scene_depth", ShaderGlobal::LAYER_SCENE);
  // shaderblocks::dynamicShadowSceneBlkId =
  //   ShaderGlobal::getBlockId("demon_dynamic_shadow_scene", ShaderGlobal::LAYER_SCENE);

  if (mandatory_shader_blocks)
  {
    G_ASSERT(shaderblocks::frameBlkId != -1);
    G_ASSERT(
      shaderblocks::staticSceneBlkId != -1 || (shaderblocks::deferredSceneBlkId != -1 && shaderblocks::forwardSceneBlkId != -1));
    G_ASSERT(shaderblocks::staticSceneDepthBlkId != -1);
  }
  if (shaderblocks::staticSceneBlkId != -1 && shaderblocks::deferredSceneBlkId == -1)
    shaderblocks::deferredSceneBlkId = shaderblocks::staticSceneBlkId;
  if (shaderblocks::staticSceneBlkId != -1 && shaderblocks::forwardSceneBlkId == -1)
    shaderblocks::forwardSceneBlkId = shaderblocks::staticSceneBlkId;
}
} // namespace shaderblocks

namespace shadervars
{
int render_type_glob_varId = -1;
int fog_color_and_density_glob_varId = -1;
int sun_fog_color_and_density_glob_varId = -1, sky_fog_color_and_density_glob_varId = -1, sun_fog_azimuth_glob_varId = -1;
int layered_fog_sun_color_glob_varId = -1, layered_fog_sky_color_glob_varId = -1, layered_fog_density_glob_varId = -1,
    layered_fog_min_height_glob_varId = -1, layered_fog_max_height_glob_varId = -1;

#define GET_VAR_ID(a) a##_glob_varId = ::get_shader_glob_var_id(#a, true)
void init(bool mandatory_blocks)
{
  GET_VAR_ID(render_type);
  GET_VAR_ID(sun_fog_color_and_density);
  GET_VAR_ID(sky_fog_color_and_density);
  GET_VAR_ID(sun_fog_azimuth);
  GET_VAR_ID(fog_color_and_density);

  GET_VAR_ID(layered_fog_sun_color);
  GET_VAR_ID(layered_fog_sky_color);
  GET_VAR_ID(layered_fog_density);
  GET_VAR_ID(layered_fog_min_height);
  GET_VAR_ID(layered_fog_max_height);
  shaderblocks::init_shader_blocks(mandatory_blocks);
}
#undef GET_VAR_ID
} // namespace shadervars
