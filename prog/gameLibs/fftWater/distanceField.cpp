#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>
#include <3d/dag_tex3d.h>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_textureIDHolder.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_postFxRenderer.h>
#include <fftWater/fftWater.h>

namespace fft_water
{

void build_distance_field(UniqueTexHolder &distField, int textureSize, int heightmapTextureSize, float detect_rivers_width,
  RiverRendererCB *riversCB)
{
  distField.close();
  Color4 world_to_heightmap = ShaderGlobal::get_color4_fast(get_shader_variable_id("world_to_heightmap", true));
  ShaderGlobal::set_real(get_shader_variable_id("sdf_texture_size_meters"), world_to_heightmap.r > 0 ? 1.0 / world_to_heightmap.r : 1);

  UniqueTex tempDistField[2]; //|TEXFMT_L8
  tempDistField[0] = dag::create_tex(NULL, textureSize, textureSize, TEXCF_RTARGET | TEXFMT_G16R16, 1, "shore_sdf_t0");
  tempDistField[1] = dag::create_tex(NULL, textureSize, textureSize, TEXCF_RTARGET | TEXFMT_G16R16, 1, "shore_sdf_t1");
  tempDistField[0].getTex2D()->texaddr(TEXADDR_CLAMP);
  tempDistField[1].getTex2D()->texaddr(TEXADDR_CLAMP);
  tempDistField[0].getTex2D()->texfilter(TEXFILTER_POINT);
  tempDistField[1].getTex2D()->texfilter(TEXFILTER_POINT);

  ShaderGlobal::set_int(get_shader_variable_id("distance_field_texture_size"), textureSize);
  ShaderGlobal::set_int(get_shader_variable_id("height_texture_size"), heightmapTextureSize);

  PostFxRenderer distanceFieldBuilder;
  distanceFieldBuilder.init("water_distance_field");
  Driver3dRenderTarget prevRt;
  d3d::driver_command(DRV3D_COMMAND_ACQUIRE_OWNERSHIP, NULL, NULL, NULL);
  d3d::get_render_target(prevRt);
  d3d::set_render_target();
  int frameId = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
  int sceneId = ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE);

  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
  {
    TIME_D3D_PROFILE(build_sdf)
    int sdf_stageVarId = get_shader_variable_id("sdf_stage");
    int sdf_temp_texVarId = get_shader_variable_id("sdf_temp_tex");
    ShaderGlobal::set_int(sdf_stageVarId, -1);
    d3d::set_render_target(tempDistField[0].getTex2D(), 0);
    distanceFieldBuilder.render();
    // save_rt_image_as_tga(tempDistField[0].getTex2D(), "temp.tga");

    // jump flooding voronoi diagram
    int currentTemp = 1;
    int maxLogDist = 6; // 64 pixels max sdf
    for (int i = 0; i <= maxLogDist; ++i, currentTemp = 1 - currentTemp)
    {
      ShaderGlobal::set_int(sdf_stageVarId, 1 << (maxLogDist - i));
      ShaderGlobal::set_texture(sdf_temp_texVarId, tempDistField[1 - currentTemp]);
      d3d::resource_barrier({tempDistField[1 - currentTemp].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      d3d::set_render_target(tempDistField[currentTemp].getTex2D(), 0);
      distanceFieldBuilder.render();
      // save_rt_image_as_tga(tempDistField[currentTemp].getTex2D(), String(128, "temp%d.tga", i));
    }
    ShaderGlobal::set_texture(sdf_temp_texVarId, tempDistField[currentTemp]);
    d3d::resource_barrier({tempDistField[currentTemp].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
    tempDistField[1 - currentTemp].close();


    UniqueTexHolder river_mask;
    if (riversCB || detect_rivers_width > 0)
    {
      const int river_levels = 3;
      river_mask = dag::create_tex(NULL, textureSize, textureSize, TEXCF_RTARGET | TEXFMT_L8, river_levels, "temp_river_mask");
      if (!river_mask.getTex2D())
        river_mask = dag::create_tex(NULL, textureSize, textureSize, TEXCF_RTARGET | TEXFMT_A8R8G8B8, river_levels, "temp_river_mask");
      d3d_err(river_mask.getTex2D());
      river_mask.getTex2D()->texaddr(TEXADDR_CLAMP);
      d3d::set_render_target(river_mask.getTex2D(), 0);
      if (detect_rivers_width > 0)
      {
        ShaderGlobal::set_real(get_shader_variable_id("max_river_width", true), detect_rivers_width);
        PostFxRenderer riverBuilder;
        riverBuilder.init("build_river");
        riverBuilder.render();
      }
      if (riversCB)
        riversCB->renderRivers();

      river_mask.setVar();
      d3d::resource_barrier({river_mask.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 1});
      PostFxRenderer riverBuilderMip;
      riverBuilderMip.init("build_river_mip");
      int current_mip_sizeVarId = get_shader_variable_id("current_mip_size", true);
      for (int i = 1; i < river_levels; ++i)
      {
        d3d::set_render_target(river_mask.getTex2D(), i);
        river_mask.getTex2D()->texmiplevel(i - 1, i - 1);
        ShaderGlobal::set_int(current_mip_sizeVarId, max(1, textureSize >> i));
        riverBuilderMip.render();
        d3d::resource_barrier({river_mask.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, (unsigned)i, 1});
      }
      river_mask.getTex2D()->texmiplevel(-1, -1);
    }

    // build sdf
    distField = dag::create_tex(NULL, textureSize, textureSize, TEXCF_RTARGET, 1, "shore_distance_field_tex");

    distField.getTex2D()->texaddr(TEXADDR_CLAMP);
    d3d::set_render_target(distField.getTex2D(), 0);
    PostFxRenderer distanceGradientBuilder;
    distanceGradientBuilder.init("water_gradient_field");

    distanceGradientBuilder.render();

    ShaderGlobal::set_texture(sdf_temp_texVarId, BAD_TEXTUREID);
    distField.setVar();
    d3d::resource_barrier({distField.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

    /*tempDistField = d3d::create_tex(
      NULL,
      shoreDistanceFieldTexSize/16,
      shoreDistanceFieldTexSize/16,
      TEXCF_RTARGET|TEXFMT_L8,
      1,
      "shoreDistanceFieldTex");

    PostFxRenderer countTiles;countTiles.init("water_gradient_count_tiles", NULL, true);
    d3d::set_render_target(tempDistField, 0, false);

    void * query = 0;
    d3d::driver_command(DRV3D_COMMAND_GETVISIBILITYBEGIN, &query, 0, 0);
    countTiles.render();
    d3d::driver_command(DRV3D_COMMAND_GETVISIBILITYEND, query, 0, 0);
    int pixelCount = -1;
    while (pixelCount==-1)
    {
      pixelCount = d3d::driver_command(DRV3D_COMMAND_GETVISIBILITYCOUNT, query, (void*)true, 0);
      dagor_idle_cycle();
    };
    d3d::driver_command(DRV3D_COMMAND_RELEASE_QUERY, &query, (void*)true, 0);
    debug("pixelCount = %d out of %d (%dx%d)", pixelCount,
      shoreDistanceFieldTexSize*shoreDistanceFieldTexSize/256, shoreDistanceFieldTexSize, shoreDistanceFieldTexSize);
    */
  }

  d3d::set_render_target(prevRt);

  if (frameId >= 0)
    ShaderGlobal::setBlock(frameId, ShaderGlobal::LAYER_FRAME);
  if (sceneId >= 0)
    ShaderGlobal::setBlock(sceneId, ShaderGlobal::LAYER_SCENE);

  d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, NULL, NULL, NULL);
}

} // namespace fft_water