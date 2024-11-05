// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <3d/dag_texMgr.h>
#include <drv/3d/dag_commands.h>
#include <math/dag_e3dColor.h>
#include <math/dag_color.h>
#include <render/blkToConstBuffer.h>
#include <shaders/dag_shaders.h>
#include <stdio.h> // snprintf

extern TEXTUREID load_texture_array_immediate(const char *name, const char *param_name, const DataBlock &blk, int &count);

TEXTUREID load_land_micro_details(const DataBlock &micro)
{
  const int land_micro_detailsVarId = get_shader_variable_id("land_micro_details", true);
  if (land_micro_detailsVarId < 0)
  {
    logerr("load_land_micro_details() skipped due to missing \"%s\" shadervar", "land_micro_details");
    return BAD_TEXTUREID;
  }
  int microDetailCount = 0;
  TEXTUREID landMicrodetailsId = load_texture_array_immediate("land_micro_details*", "micro_detail", micro, microDetailCount);
  if (landMicrodetailsId != BAD_TEXTUREID)
  {
    Tab<Point4> param;
    const int microNameId = micro.getNameId("micro_detail");
    DataBlock scheme;
    scheme.addReal("coloring", 1);
    scheme.addReal("porosity", 1);
    scheme.addReal("puddles", 1);
    scheme.addReal("sparkles", 0);
    create_constant_buffer(scheme, scheme, param);
    Point4 def = param[0];
    for (int i = micro.findParam(microNameId, -1); i >= 0; i = micro.findParam(microNameId, i))
    {
      char buf[64];
      SNPRINTF(buf, sizeof(buf), "land_micro_details_params%X", i);

      const DataBlock *paramBlk = micro.getBlockByName(micro.getStr(i));
      if (!paramBlk)
      {
        ShaderGlobal::set_color4(get_shader_variable_id(buf, true), *(Color4 *)&def);
        continue;
      }
      param.clear();
      create_constant_buffer(scheme, *paramBlk, param);
      ShaderGlobal::set_color4(get_shader_variable_id(buf, true), *(Color4 *)&param[0]);
    }
  }
  ShaderGlobal::set_texture(land_micro_detailsVarId, landMicrodetailsId);
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.anisotropic_max = ::dgs_tex_anisotropy;
    ShaderGlobal::set_sampler(get_shader_variable_id("land_micro_details_samplerstate"), d3d::request_sampler(smpInfo));
  }
  int land_micro_details_countVarId = get_shader_variable_id("land_micro_details_count", true);
  if (land_micro_details_countVarId >= 0)
    ShaderGlobal::set_int(land_micro_details_countVarId, microDetailCount);
  int land_micro_details_cnt_scaleVarId = get_shader_variable_id("land_micro_details_cnt_scale", true);
  if (land_micro_details_cnt_scaleVarId >= 0)
    ShaderGlobal::set_real(land_micro_details_cnt_scaleVarId, microDetailCount > 1 ? 255.f / (255 / (microDetailCount - 1)) : 0.f);

  ShaderGlobal::set_real(get_shader_variable_id("land_micro_details_uv_scale", true),
    micro.getReal("land_micro_details_uv_scale", 2.01));

  // invoke flash after the loading to propagate changes to the main context if it was called from a thread
  // fixes black terrain on opengl
  d3d::driver_command(Drv3dCommand::D3D_FLUSH);

  return landMicrodetailsId;
}

void close_land_micro_details(TEXTUREID &id)
{
  if (id == BAD_TEXTUREID)
    return;
  ShaderGlobal::reset_from_vars(id);
  release_managed_tex(id);
  id = BAD_TEXTUREID;

  int land_micro_details_countVarId = get_shader_variable_id("land_micro_details_count", true);
  if (land_micro_details_countVarId >= 0)
    ShaderGlobal::set_int(land_micro_details_countVarId, 0);

  int land_micro_details_cnt_scaleVarId = get_shader_variable_id("land_micro_details_cnt_scale", true);
  if (land_micro_details_cnt_scaleVarId >= 0)
    ShaderGlobal::set_real(land_micro_details_cnt_scaleVarId, 0.f);
}
