// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <fx/dag_commonFx.h>
#include <startup/dag_globalSettings.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameResId.h>
#include <ioSys/dag_dataBlock.h>
#include <memory/dag_framemem.h>
#include <startup/dag_startupTex.h>
#include <osApiWrappers/dag_messageBox.h>
#include <osApiWrappers/dag_direct.h>
#include <shaders/dag_shaders.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_render.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_files.h>
#include <daECS/core/entityManager.h>
#include "render/fx/fx.h"
#include "render/renderEvent.h"
#include "render/rendererFeatures.h"
#include "main/version.h"

void init_shaders()
{
  shaders_register_console(true, [](bool success) {
    if (success)
      g_entity_mgr->broadcastEventImmediate(AfterShaderReload{});
  });

#if _TARGET_PC_WIN
  if (d3d::get_driver_desc().shaderModel < 5.0_sm)
  {
    os_message_box("This game requires a DirectX 11 compatible video card.", "Initialization error", GUI_MB_OK | GUI_MB_ICON_ERROR);
    _exit(0);
  }
#endif

  const char *sh_bindump_prefix = ::dgs_get_settings()->getStr("shadersPath", "compiledShaders/game");
#if _TARGET_XBOXONE || _TARGET_C1
  if (strcmp(::dgs_get_settings()->getBlockByNameEx("graphics")->getStr("consolePreset", "HighFPS"), "bareMinimum") == 0)
    sh_bindump_prefix = "compiledShaders/compatPC/game";
#elif _TARGET_PC || _TARGET_APPLE
  if (strcmp(::dgs_get_settings()->getBlockByNameEx("graphics")->getStr("preset", "medium"), "bareMinimum") == 0)
    sh_bindump_prefix = "compiledShaders/compatPC/game";
#endif

  String renderingPath = get_corrected_rendering_path();
  if (renderingPath == "forward")
    sh_bindump_prefix = ::dgs_get_settings()->getStr("forwardShaders", "compiledShaders/gameAndroidForward");
  else if (renderingPath == "mobile_deferred")
    sh_bindump_prefix = ::dgs_get_settings()->getStr("deferredShaders", "compiledShaders/gameAndroidDeferred");

  set_stcode_special_tag_interp([](eastl::string_view directive) -> eastl::optional<eastl::string> {
    if (directive == "version")
    {
      exe_version32_t ver = get_exe_version32();
      return eastl::string{
        eastl::string::CtorSprintf{}, "%hhu.%hhu.%hhu.%hhu", ver >> 24, (ver >> 16) & 0xFF, (ver >> 8) & 0xFF, ver & 0xFF};
    }
    else
    {
      return eastl::nullopt;
    }
  });

  ::startup_shaders(sh_bindump_prefix);
}

void init_res_factories()
{
  ::set_gameres_undefined_res_loglevel(LOGLEVEL_ERR);
  ::register_common_game_tex_factories();
  ::register_svg_tex_load_factory();
  ::register_loadable_tex_create_factory();

  auto &dynmodel_blk = *::dgs_get_settings()->getBlockByNameEx("unitedVdata.dynModel");
  bool dynmodel_uvd = dynmodel_blk.getBool("use", false);
  bool dynmodel_uvd_can_rebuild = dynmodel_blk.getBool("canRebuild", false);
  debug("unitedVdata.dynModel: use=%d canRebuild=%d", dynmodel_uvd, dynmodel_uvd_can_rebuild);
  ::register_dynmodel_gameres_factory(dynmodel_uvd, dynmodel_uvd_can_rebuild);

  // Note: factories are freed in reverse to registration order
  ::register_geom_node_tree_gameres_factory();
  ::register_fast_phys_gameres_factory();
  ::register_a2d_gameres_factory();
  ::register_animchar_gameres_factory();
  ::register_character_gameres_factory();

  bool ri_uvd_streaming = ::dgs_get_settings()->getBlockByNameEx("unitedVdata.rendInst")->getBool("useStreaming", true);
  debug("unitedVdata.rendInst: useStreaming=%d", ri_uvd_streaming);
  ::register_rendinst_gameres_factory(ri_uvd_streaming);
  ::register_phys_obj_gameres_factory();
  ::register_effect_gameres_factory();
  ::register_png_tex_load_factory();
  ::register_avif_tex_load_factory();
  ::register_tga_tex_load_factory();
  ::register_jpeg_tex_load_factory();
  ::register_any_vromfs_tex_create_factory("avif|png|jpg|tga|ddsx");
  ::register_any_tex_load_factory();
  ::register_lshader_gameres_factory();
}
void term_res_factories() {}

void init_fx()
{
  const char *dafxType = ::dgs_get_settings()->getBlockByNameEx("graphics")->getStr("dafx", "cpu");
  bool dafxGpuSim = strcmp(dafxType, "gpu") == 0;
  bool dafxEnabled = acesfx::init_dafx(dafxGpuSim); // we need to create dafx context before fx factories
  G_ASSERT_RETURN(dafxEnabled, );

  ::register_dafx_sparks_factory();
  ::register_dafx_modfx_factory();
  ::register_dafx_compound_factory();
  ::register_light_fx_factory();

  static const char *fx_blk_fn = "config/fx.blk";
  DataBlock fxBlk;
  if (dd_file_exists(fx_blk_fn))
    fxBlk.load(fx_blk_fn);
  else
    LOGWARN_CTX("skip missing %s", fx_blk_fn);

  eastl::string templatesPatchPath = fx_blk_fn;
  int dot_pos = templatesPatchPath.find_last_of('.');
  if (dot_pos < templatesPatchPath.size())
  {
    const eastl::string blk_ext = ".blk";
    if (templatesPatchPath.compare(dot_pos, blk_ext.size(), blk_ext) == 0)
    {
      templatesPatchPath.erase(dot_pos + 1);
      templatesPatchPath += get_platform_string_id();
      templatesPatchPath += ".patch";
    }
  }
  file_ptr_t fp = df_open(templatesPatchPath.c_str(), DF_READ | DF_IGNORE_MISSING);
  if (fp)
  {
    Tab<char> text;
    text.resize(df_length(fp) + 3);
    df_read(fp, text.data(), data_size(text) - 3);
    df_close(fp);
    strcpy(&text[text.size() - 3], "\n\n");
    bool ret = dblk::load_text(fxBlk, text, dblk::ReadFlags(), ".patch");
    debug("patching fx list with %s: %d", templatesPatchPath, (int)ret);
  }

  acesfx::init(fxBlk);
}

void term_fx()
{
  acesfx::shutdown(); // this frees all effects memory
  acesfx::term_dafx();
  find_gameres_factory(EffectGameResClassId)->freeUnusedResources(false);
}
