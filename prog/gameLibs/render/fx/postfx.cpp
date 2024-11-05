// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_globDef.h>
#include <render/fx/dag_postfx.h>
#include <fx/dag_hdrRender.h>
#include <render/fx/dag_demonPostFx.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <osApiWrappers/dag_localConv.h>

#include <debug/dag_debug.h>


PostFx::PostFx(const DataBlock &level_settings, const DataBlock &game_settings, const PostFxUserSettings &user_settings)
{
  demonPostFx = NULL;
  genPostFx = NULL;

  tempTex = NULL;
  tempTexId = BAD_TEXTUREID;

  restart(&level_settings, &game_settings, &user_settings);
}


PostFx::~PostFx()
{
  del_it(demonPostFx);
  del_it(genPostFx);
  //  del_it(motionBlurFx);
  //  del_it(colorCurveFx);
  //  del_it(oldTvFx);

  ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(tempTexId, tempTex);
}


void PostFx::restart(const DataBlock *level_settings, const DataBlock *game_settings, const PostFxUserSettings *user_settings)
{
  // Mess with last settings.

  if (level_settings)
    lastLevelSettings = *level_settings;
  else
    level_settings = &lastLevelSettings;

  if (game_settings)
    lastGameSettings = *game_settings;
  else
    game_settings = &lastGameSettings;

  if (user_settings)
    lastUserSettings = *user_settings;
  else
    user_settings = &lastUserSettings;


  // Update target size.
  d3d::get_render_target_size(targetW, targetH, NULL);

  ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(tempTexId, tempTex);

  const DataBlock *postfxRtBlk = game_settings->getBlockByName("postfx_rt");
  if (postfxRtBlk)
  {
    targetW = postfxRtBlk->getInt("use_w", targetW);
    targetH = postfxRtBlk->getInt("use_h", targetH);
    // debug("use target %dx%d for postfx", targetW, targetH);
  }

  // HDR.

  const DataBlock *hdrBlk = game_settings->getBlockByName("hdr");
  DataBlock hdrBlkCopy;
  if (!hdrBlk)
    hdrBlk = level_settings->getBlockByNameEx("hdr");

  if (!user_settings->shouldTakeStateFromBlk)
  {
    hdrBlkCopy = *hdrBlk;
    hdrBlkCopy.setStr("mode", user_settings->hdrMode);
    hdrBlk = &hdrBlkCopy;
  }

  const char *hdrMode = hdrBlk->getStr("mode", "none");
  if (!dd_stricmp(hdrMode, "none"))
    ::hdr_render_mode = HDR_MODE_NONE;
  else if (!dd_stricmp(hdrMode, "fake"))
    ::hdr_render_mode = HDR_MODE_FAKE;
  else if (!dd_stricmp(hdrMode, "real"))
    ::hdr_render_mode = HDR_MODE_10BIT;
  else if (!dd_stricmp(hdrMode, "ps3"))
    ::hdr_render_mode = HDR_MODE_PS3;
  else if (!dd_stricmp(hdrMode, "xblades"))
    ::hdr_render_mode = HDR_MODE_XBLADES;
  else if (!dd_stricmp(hdrMode, "float"))
    ::hdr_render_mode = HDR_MODE_FLOAT;
  else
    DAG_FATAL("Invalid hdr mode <%s>.", hdrMode);

  unsigned working_flags = d3d::USAGE_FILTER | d3d::USAGE_BLEND | d3d::USAGE_RTARGET;

  if (::hdr_render_mode == HDR_MODE_10BIT)
  {
    ::hdr_max_overbright = 1;
    ::hdr_max_bright_val = 1;
    if ((d3d::get_texformat_usage(TEXFMT_A2B10G10R10) & working_flags) == working_flags)
    {
      ::hdr_render_format = TEXFMT_A2B10G10R10;
      ::hdr_max_overbright = 2;
      ::hdr_max_value = 1;
      ::hdr_max_bright_val = 2;
    }
    else if ((d3d::get_texformat_usage(TEXFMT_A2R10G10B10) & working_flags) == working_flags)
    {
      ::hdr_render_format = TEXFMT_A2R10G10B10;
      ::hdr_max_overbright = 2;
      ::hdr_max_value = 1;
      ::hdr_max_bright_val = 2;
    }
    else if ((d3d::get_texformat_usage(TEXFMT_A16B16G16R16F) & working_flags) == working_flags)
    {
      ::hdr_render_format = TEXFMT_A16B16G16R16F;
      ::hdr_render_mode = HDR_MODE_FLOAT;
      ::hdr_max_overbright = 4; // that is not an actual truth. but result will be better
      ::hdr_max_value = 64;     // also not true
      ::hdr_max_bright_val = 4;
    }
    else if ((d3d::get_texformat_usage(TEXFMT_A16B16G16R16) & working_flags) == working_flags)
    {
      ::hdr_render_format = TEXFMT_A16B16G16R16;
      ::hdr_max_overbright = 8; // just to make glow results closer to 10-bit
      ::hdr_max_value = 1;
      ::hdr_render_mode = HDR_MODE_16BIT;
      ::hdr_max_bright_val = 4;
    }
    else
    {
      ::hdr_render_format = TEXCF_BEST | TEXCF_ABEST;
      ::hdr_render_mode = HDR_MODE_FAKE;
      ::hdr_max_overbright = 1;
      ::hdr_max_value = 1;
      ::hdr_max_bright_val = 1;
    }
  }
  else if (::hdr_render_mode == HDR_MODE_FLOAT)
  {
    if ((d3d::get_texformat_usage(TEXFMT_R11G11B10F) & working_flags) == working_flags)
    {
      ::hdr_render_format = TEXFMT_R11G11B10F;
      ::hdr_render_mode = HDR_MODE_FLOAT;
      ::hdr_max_overbright = 1;
      ::hdr_max_bright_val = 1;
    }
    else if ((d3d::get_texformat_usage(TEXFMT_A16B16G16R16F) & working_flags) == working_flags)
    {
      ::hdr_render_format = TEXFMT_A16B16G16R16F;
      ::hdr_render_mode = HDR_MODE_FLOAT;
      ::hdr_max_overbright = 1;
      ::hdr_max_bright_val = 1;
    }
  }
  else if (::hdr_render_mode == HDR_MODE_FAKE)
  {
    if ((d3d::get_texformat_usage(TEXFMT_A2B10G10R10) & working_flags) == working_flags)
    {
      ::hdr_render_format = TEXFMT_A2B10G10R10;
      ::hdr_render_mode = HDR_MODE_10BIT;
      ::hdr_max_overbright = 4;
      ::hdr_max_bright_val = 4;
    }
    else if ((d3d::get_texformat_usage(TEXFMT_A2R10G10B10) & working_flags) == working_flags)
    {
      ::hdr_render_format = TEXFMT_A2R10G10B10;
      ::hdr_render_mode = HDR_MODE_10BIT;
      ::hdr_max_overbright = 4;
      ::hdr_max_bright_val = 4;
    }
    else
    {
      ::hdr_render_format = TEXCF_BEST | TEXCF_ABEST;
      ::hdr_max_overbright = 1;
      ::hdr_max_bright_val = 1;
    }
  }
  else if (::hdr_render_mode == HDR_MODE_PS3)
  {
    ::hdr_opaque_render_format = TEXCF_BEST | TEXCF_ABEST;
    /*if ((d3d::get_texformat_usage(TEXFMT_A16B16G16R16F) & working_flags) == working_flags)
    {
      ::hdr_render_format = TEXFMT_A16B16G16R16F;
      ::hdr_max_overbright = 4;//that is not an actual truth. but result will be better
      ::hdr_max_value = 64;//also not true
      ::hdr_max_bright_val = 4;
    } else {
      debug("NOT SUPPORTING 16 bit float");
      ::hdr_opaque_render_format = TEXCF_BEST | TEXCF_ABEST;
    }*/
    ::hdr_render_format = TEXCF_BEST | TEXCF_ABEST;
    ::hdr_max_overbright = 1;
    ::hdr_max_bright_val = 1;
  }
  else
  {
    ::hdr_render_format = TEXCF_BEST | TEXCF_ABEST;
    ::hdr_max_overbright = 1;
    ::hdr_max_bright_val = 1;
  }

  if (::hdr_render_mode != HDR_MODE_PS3)
    ::hdr_opaque_render_format = ::hdr_render_format;

  const char *hdr_modes[] = {"none", "fake", "float", "10bit", "16bit", "ps3", "xblades"};

  static bool loggingEnabled = dgs_get_settings()->getBlockByNameEx("debug")->getBool("view_resizing_related_logging_enabled", true);
  if (loggingEnabled)
    DEBUG_CTX("HDR mode: %s, maxoverbr = %g", hdr_modes[::hdr_render_mode], ::hdr_max_overbright);

  ShaderGlobal::set_int(::get_shader_variable_id("hdr_mode", true), ::hdr_render_mode);
  ShaderGlobal::set_real(::get_shader_variable_id("hdr_overbright", true), ::hdr_max_overbright);
  ShaderGlobal::set_real(::get_shader_variable_id("max_hdr_overbright", true), ::hdr_max_overbright);

  if (const char *pfx = level_settings->getStr("postfx", game_settings->getStr("postfx", NULL)))
  {
    del_it(demonPostFx);
    del_it(genPostFx);
    genPostFx = new PostFxRenderer;
    genPostFx->init(pfx);
    return;
  }

  // deMon post-fx
  const DataBlock *demonBlk = game_settings->getBlockByName("DemonPostFx");
  if (!demonBlk)
    demonBlk = level_settings->getBlockByNameEx("DemonPostFx");
  if (demonBlk->getBool("use", false) || ::hdr_render_mode != HDR_MODE_NONE)
  {
    del_it(demonPostFx);
    demonPostFx = new DemonPostFx(*demonBlk, *hdrBlk, targetW, targetH, ::hdr_render_format);
  }
  else
  {
    del_it(demonPostFx);
  }
}

bool PostFx::updateSettings(const DataBlock *level_settings, const DataBlock *game_settings)
{
  // Mess with last settings.

  if (level_settings)
    lastLevelSettings = *level_settings;
  else
    level_settings = &lastLevelSettings;

  if (game_settings)
    lastGameSettings = *game_settings;
  else
    game_settings = &lastGameSettings;


  // Update target size.
  int rt_w, rt_h;
  d3d::get_render_target_size(rt_w, rt_h, NULL);

  const DataBlock *postfxRtBlk = game_settings->getBlockByName("postfx_rt");
  if (postfxRtBlk)
  {
    rt_w = postfxRtBlk->getInt("use_w", rt_w);
    rt_h = postfxRtBlk->getInt("use_h", rt_h);
    // debug("use target %dx%d for postfx", targetW, targetH);
  }
  if (rt_w != targetW || rt_h != targetH)
    return false;

  // HDR.
  int hdr_mode = HDR_MODE_NONE, hdr_fmt = 0;
  const DataBlock *hdrBlk = game_settings->getBlockByName("hdr");
  if (!hdrBlk)
    hdrBlk = level_settings->getBlockByNameEx("hdr");

  DataBlock hdrBlkCopy;
  if (!lastUserSettings.shouldTakeStateFromBlk)
  {
    hdrBlkCopy = *hdrBlk;
    hdrBlkCopy.setStr("mode", lastUserSettings.hdrMode);
    hdrBlk = &hdrBlkCopy;
  }

  const char *hdrMode = hdrBlk->getStr("mode", "none");
  if (!dd_stricmp(hdrMode, "none"))
    hdr_mode = HDR_MODE_NONE;
  else if (!dd_stricmp(hdrMode, "fake"))
    hdr_mode = HDR_MODE_FAKE;
  else if (!dd_stricmp(hdrMode, "real"))
    hdr_mode = HDR_MODE_10BIT;
  else if (!dd_stricmp(hdrMode, "ps3"))
    hdr_mode = HDR_MODE_PS3;
  else if (!dd_stricmp(hdrMode, "float"))
    ::hdr_render_mode = HDR_MODE_FLOAT;
  else if (!dd_stricmp(hdrMode, "xblades"))
    hdr_mode = HDR_MODE_XBLADES;
  else
    DAG_FATAL("Invalid hdr mode <%s>.", hdrMode);

  unsigned working_flags = d3d::USAGE_FILTER | d3d::USAGE_BLEND | d3d::USAGE_RTARGET;

  if (hdr_mode == HDR_MODE_10BIT)
  {
    ::hdr_max_overbright = 1;
    ::hdr_max_bright_val = 1;
    if ((d3d::get_texformat_usage(TEXFMT_A2B10G10R10) & working_flags) == working_flags)
    {
      hdr_fmt = TEXFMT_A2B10G10R10;
      ::hdr_max_overbright = 2;
      ::hdr_max_value = 1;
      ::hdr_max_bright_val = 2;
    }
    else if ((d3d::get_texformat_usage(TEXFMT_A2R10G10B10) & working_flags) == working_flags)
    {
      hdr_fmt = TEXFMT_A2R10G10B10;
      ::hdr_max_overbright = 2;
      ::hdr_max_value = 1;
      ::hdr_max_bright_val = 2;
    }
    else if ((d3d::get_texformat_usage(TEXFMT_A16B16G16R16F) & working_flags) == working_flags)
    {
      hdr_fmt = TEXFMT_A16B16G16R16F;
      hdr_mode = HDR_MODE_FLOAT;
      ::hdr_max_overbright = 4; // that is not an actual truth. but result will be better
      ::hdr_max_value = 64;     // also not true
      ::hdr_max_bright_val = 4;
    }
    else if ((d3d::get_texformat_usage(TEXFMT_A16B16G16R16) & working_flags) == working_flags)
    {
      hdr_fmt = TEXFMT_A16B16G16R16;
      ::hdr_max_overbright = 8; // just to make glow results closer to 10-bit
      ::hdr_max_value = 1;
      hdr_mode = HDR_MODE_16BIT;
      ::hdr_max_bright_val = 4;
    }
    else
    {
      hdr_fmt = TEXCF_BEST | TEXCF_ABEST;
      hdr_mode = HDR_MODE_FAKE;
      ::hdr_max_overbright = 1;
      ::hdr_max_value = 1;
      ::hdr_max_bright_val = 1;
    }
  }
  else if (::hdr_render_mode == HDR_MODE_FLOAT)
  {
    if ((d3d::get_texformat_usage(TEXFMT_R11G11B10F) & working_flags) == working_flags)
    {
      hdr_fmt = TEXFMT_R11G11B10F;
      hdr_mode = HDR_MODE_FLOAT;
      ::hdr_max_overbright = 1;
      ::hdr_max_bright_val = 1;
    }
    else if ((d3d::get_texformat_usage(TEXFMT_A16B16G16R16F) & working_flags) == working_flags)
    {
      hdr_fmt = TEXFMT_A16B16G16R16F;
      hdr_mode = HDR_MODE_FLOAT;
      ::hdr_max_overbright = 1;
      ::hdr_max_bright_val = 1;
    }
  }
  else if (hdr_mode == HDR_MODE_FAKE)
  {
    if ((d3d::get_texformat_usage(TEXFMT_A2B10G10R10) & working_flags) == working_flags)
    {
      hdr_fmt = TEXFMT_A2B10G10R10;
      hdr_mode = HDR_MODE_10BIT;
      ::hdr_max_overbright = 4;
      ::hdr_max_bright_val = 4;
    }
    else if ((d3d::get_texformat_usage(TEXFMT_A2R10G10B10) & working_flags) == working_flags)
    {
      hdr_fmt = TEXFMT_A2R10G10B10;
      hdr_mode = HDR_MODE_10BIT;
      ::hdr_max_overbright = 4;
      ::hdr_max_bright_val = 4;
    }
    else
    {
      hdr_fmt = TEXCF_BEST | TEXCF_ABEST;
      ::hdr_max_overbright = 1;
      ::hdr_max_bright_val = 1;
    }
  }
  else if (hdr_mode == HDR_MODE_PS3)
  {
    ::hdr_opaque_render_format = TEXCF_BEST | TEXCF_ABEST;
    /*if ((d3d::get_texformat_usage(TEXFMT_A16B16G16R16F) & working_flags) == working_flags)
    {
      hdr_fmt = TEXFMT_A16B16G16R16F;
      ::hdr_max_overbright = 4;//that is not an actual truth. but result will be better
      ::hdr_max_value = 64;//also not true
      ::hdr_max_bright_val = 4;
    } else {
      debug("NOT SUPPORTING 16 bit float");
      ::hdr_opaque_render_format = TEXCF_BEST | TEXCF_ABEST;
    }*/
    hdr_fmt = TEXCF_BEST | TEXCF_ABEST;
    ::hdr_max_overbright = 1;
    ::hdr_max_bright_val = 1;
  }
  else
  {
    hdr_fmt = TEXCF_BEST | TEXCF_ABEST;
    ::hdr_max_overbright = 1;
    ::hdr_max_bright_val = 1;
  }

  if (hdr_mode != ::hdr_render_mode || hdr_fmt != ::hdr_render_format)
    return false;

  ShaderGlobal::set_real(::get_shader_variable_id("hdr_overbright", true), ::hdr_max_overbright);

  if (const char *pfx = level_settings->getStr("postfx", game_settings->getStr("postfx", NULL)))
  {
    del_it(demonPostFx);
    del_it(genPostFx);
    genPostFx = new PostFxRenderer;
    genPostFx->init(pfx);
    return true;
  }

  // deMon post-fx
  const DataBlock *demonBlk = game_settings->getBlockByName("DemonPostFx");
  if (!demonBlk)
    demonBlk = level_settings->getBlockByNameEx("DemonPostFx");

  if (demonBlk->getBool("use", false) || ::hdr_render_mode != HDR_MODE_NONE)
  {
    if (demonPostFx && !demonPostFx->updateSettings(*demonBlk))
      return false;
    else if (!demonPostFx)
      demonPostFx = new DemonPostFx(*demonBlk, *hdrBlk, targetW, targetH, ::hdr_render_format);
  }
  else
  {
    del_it(demonPostFx);
  }

  // Motion blur.
  return true;
}

TEXTUREID PostFx::downsample(Texture *input_tex, TEXTUREID input_id)
{
  if (demonPostFx)
    return demonPostFx->downsample(input_tex, input_id).getId();
  return BAD_TEXTUREID;
}

void PostFx::apply(Texture *source, TEXTUREID sourceId, Texture *target, TEXTUREID targtexId, const TMatrix &view_tm,
  const TMatrix4 &proj_tm, bool force_disable_motion_blur)
{
  G_UNREFERENCED(force_disable_motion_blur);
  if (genPostFx)
  {
    static int varId = get_shader_variable_id("frame_tex", true);
    Driver3dRenderTarget rt;
    d3d::get_render_target(rt);

    d3d::set_render_target(target, 0);
    ShaderGlobal::set_texture(varId, sourceId);
    genPostFx->render();

    d3d::set_render_target(rt);
    return;
  }
  if (!demonPostFx)
  {
    if (source != target)
      d3d_err(d3d::stretch_rect(source, target));
    return;
  }

  if (source == target)
  {
    createTempTex();
    d3d_err(d3d::stretch_rect(source, tempTex));
    demonPostFx->apply(false, tempTex, tempTexId, target, targtexId, view_tm, proj_tm);
  }
  else
    demonPostFx->apply(false, source, sourceId, target, targtexId, view_tm, proj_tm);
}


void PostFx::createTempTex()
{
  if (tempTex)
    return;

  tempTex = d3d::create_tex(NULL, targetW, targetH, ::hdr_render_format | TEXCF_RTARGET, 1);
  d3d_err(tempTex);
  tempTex->texaddr(TEXADDR_CLAMP);
  tempTexId = register_managed_tex("postfx@temp", tempTex);
}