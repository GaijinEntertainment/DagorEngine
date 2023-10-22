#include <render/dynamicCube.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <3d/dag_tex3d.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3d_platform.h>
#include <math/dag_mathUtils.h>
#include <3d/dag_drv3dCmd.h>


#define NUM_CUBE_FACES 6


DynamicCube::DynamicCube(unsigned int num_mips, unsigned int size, float blur, unsigned format) :
  dynamicCubeTex1(NULL),
  dynamicCubeTex1Id(BAD_TEXTUREID),
  dynamicCubeTex1VarId(-1),
  dynamicCubeTex2(NULL),
  dynamicCubeTex2Id(BAD_TEXTUREID),
  dynamicCubeTex2VarId(-1),
  dynamicCubeTex(NULL),
  dynamicCubeTexId(BAD_TEXTUREID),
  dynamicCubeTexVarId(-1),
  dynamicCubeTexBlendVarId(-1),
  dynamicCubeDepthTex(NULL),
  numDynamicCubeTexMips(num_mips),
  dynamicCubeSize(size),
  dynamicCubeBlur(blur),
  dynamicCubeFaceNo(-1),
  blendCubesStage(-1)
{

  blendCubesRenderer = create_postfx_renderer("blend_cubes");
  blurCubesRenderer = create_postfx_renderer("blur_cubes");
  blendCubesParamsVarId = get_shader_variable_id("blend_cubes_params");
  if (!blurCubesRenderer && num_mips != 1)
  {
    numDynamicCubeTexMips = 0;
    logerr("we can't blur mips of dynamic Cube, use generate mipmap instead");
  }


  uint32_t cubeFlags = format | TEXCF_RTARGET;

  dynamicCubeTex1 = d3d::create_cubetex(dynamicCubeSize, cubeFlags, 1, "dynamicCubeTex1");
  G_ASSERT(dynamicCubeTex1);
  dynamicCubeTex1Id = register_managed_tex("dynamicCubeTex1", dynamicCubeTex1);
  dynamicCubeTex1VarId = ::get_shader_variable_id("dynamic_cube_tex_1", true);
  ShaderGlobal::set_texture(dynamicCubeTex1VarId, dynamicCubeTex1Id);

  dynamicCubeTex2 = d3d::create_cubetex(dynamicCubeSize, cubeFlags, 1, "dynamicCubeTex2");
  G_ASSERT(dynamicCubeTex2);
  dynamicCubeTex2Id = register_managed_tex("dynamicCubeTex2", dynamicCubeTex2);
  dynamicCubeTex2VarId = ::get_shader_variable_id("dynamic_cube_tex_2", true);
  ShaderGlobal::set_texture(dynamicCubeTex2VarId, dynamicCubeTex2Id);

  dynamicCubeTex = d3d::create_cubetex(dynamicCubeSize,
    cubeFlags | ((numDynamicCubeTexMips > 1 && !blurCubesRenderer) ? TEXCF_GENERATEMIPS : 0), numDynamicCubeTexMips, "dynamicCubeTex");
  G_ASSERT(dynamicCubeTex);
  dynamicCubeTexId = register_managed_tex("dynamicCubeTex", dynamicCubeTex);
  dynamicCubeTexVarId = ::get_shader_variable_id("dynamic_cube_tex", true);
  ShaderGlobal::set_texture(dynamicCubeTexVarId, dynamicCubeTexId);

  dynamicCubeTexBlendVarId = ::get_shader_variable_id("dynamic_cube_tex_blend", true);

  ShaderGlobal::set_color4(get_shader_variable_id("dynamic_cube_params", true),
    numDynamicCubeTexMips ? numDynamicCubeTexMips - 1 : num_mips, 0.f, 0.f, 0.f);
}


DynamicCube::~DynamicCube()
{
  ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(dynamicCubeTex1Id, dynamicCubeTex1);
  ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(dynamicCubeTex2Id, dynamicCubeTex2);
  ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(dynamicCubeTexId, dynamicCubeTex);

  del_d3dres(dynamicCubeDepthTex);

  del_it(blendCubesRenderer);
  del_it(blurCubesRenderer);
}


bool DynamicCube::refresh()
{
  if (dynamicCubeFaceNo != -1)
    return false;

  eastl::swap(dynamicCubeTex1Id, dynamicCubeTex2Id);
  eastl::swap(dynamicCubeTex1, dynamicCubeTex2);
  ShaderGlobal::set_texture(dynamicCubeTex1VarId, dynamicCubeTex1Id);

  dynamicCubeFaceNo = 5;

  return true;
}


void DynamicCube::beforeRender(float blend_to_next, IRenderDynamicCubeFace *render)
{


  if (dynamicCubeFaceNo >= 0)
  {
    G_ASSERT(render);

    render->renderDynamicCubeFace(dynamicCubeTex2VarId, dynamicCubeTex2Id, dynamicCubeTex2, dynamicCubeFaceNo);

    dynamicCubeFaceNo--;
  }

  float blend = 0.f;
  if (dynamicCubeFaceNo < 0)
    blend = min(1.f, blend_to_next);

  ShaderGlobal::set_real(dynamicCubeTexBlendVarId, blend);


  // Blend and blur cubes.

  Driver3dRenderTarget prevRt;
  d3d::get_render_target(prevRt);
  ShaderGlobal::set_texture(dynamicCubeTexVarId, BAD_TEXTUREID);

  unsigned int fromMip = 0;
  unsigned int toMip = max(1, (int)numDynamicCubeTexMips);
  if (blendCubesStage >= 0)
  {
    if (blendCubesStage < NUM_CUBE_FACES)
    {
      fromMip = 0;
      toMip = 1;
    }
    else
    {
      fromMip = 1;
      toMip = max(1, (int)numDynamicCubeTexMips);
    }
  }

  for (unsigned int mipNo = fromMip; mipNo < toMip; mipNo++)
  {
    unsigned int fromFace = 0;
    unsigned int toFace = NUM_CUBE_FACES;
    if (blendCubesStage >= 0)
    {
      fromFace = blendCubesStage % NUM_CUBE_FACES;
      toFace = fromFace + 1;
    }
    if (!blurCubesRenderer && mipNo)
    {
      // rely on generateMips
      dynamicCubeTex->generateMips();
      continue;
    }
    for (unsigned int faceNo = fromFace; faceNo < toFace; faceNo++)
    {
      if (mipNo == 0)
      {
        d3d::set_render_target(prevRt);
        d3d::set_render_target(dynamicCubeTex, faceNo, 0);

        ShaderGlobal::set_color4(blendCubesParamsVarId, 1.f / dynamicCubeSize, -1.f / dynamicCubeSize, faceNo, 0.f);

        blendCubesRenderer->render();
      }
      else
      {
        d3d::set_render_target(prevRt);
        d3d::set_render_target(dynamicCubeTex, faceNo, (int)mipNo);

        ShaderGlobal::set_color4(blendCubesParamsVarId, 0.f, 0.f, faceNo, dynamicCubeBlur * mipNo);

        blurCubesRenderer->render();
      }
    }
  }
  blendCubesStage = (blendCubesStage + 1) % (2 * NUM_CUBE_FACES);

  d3d::set_render_target(prevRt);
  ShaderGlobal::set_texture(dynamicCubeTexVarId, dynamicCubeTexId);
}


void DynamicCube::reset(IRenderDynamicCubeFace *render)
{
  dynamicCubeFaceNo = -1;
  blendCubesStage = -1;

  for (unsigned int faceNo = 0; faceNo < 6; faceNo++)
  {
    render->renderDynamicCubeFace(dynamicCubeTex1VarId, dynamicCubeTex1Id, dynamicCubeTex1, faceNo);
  }

  for (unsigned int faceNo = 0; faceNo < 6; faceNo++)
  {
    render->renderDynamicCubeFace(dynamicCubeTex2VarId, dynamicCubeTex2Id, dynamicCubeTex2, faceNo);
  }

  beforeRender(0.f, render);
}
