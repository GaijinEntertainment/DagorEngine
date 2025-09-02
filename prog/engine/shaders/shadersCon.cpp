// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_console.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_res.h>
#include <shaders/dag_shaders.h>
#include <osApiWrappers/dag_direct.h>
#include <generic/dag_initOnDemand.h>
#include <generic/dag_sort.h>
#include <util/dag_string.h>
#include <ioSys/dag_dataBlock.h>
#include <shaders/dag_shaderVar.h>
#include "shStateBlk.h"
#include <debug/dag_debug3d.h>
#include <util/dag_texMetaData.h>
#include <util/dag_string.h>
#include <osApiWrappers/dag_files.h>
#include <startup/dag_globalSettings.h>
#include "profileStcode.h"

extern void dump_exec_stcode_time();
class ShadersReloadProcessor : public console::ICommandProcessor
{
public:
  ShadersReloadProcessor(const ShaderReloadCb &shader_reload_cb) : console::ICommandProcessor(1000), reloadCb(shader_reload_cb) {}

  void destroy() {}

  virtual bool processCommand(const char *argv[], int argc)
  {
    int found = 0;

    CONSOLE_CHECK_NAME_EX("render", "reload_shaders", 1, 3, "", "[use_fence] [shaders_bindump_name]")
    {
      G_ASSERT(d3d::is_inited());
      const Driver3dDesc &dsc = d3d::get_driver_desc();
      extern const char *last_loaded_shaders_bindump(bool);
      bool useFence = argc > 1 ? console::to_bool(argv[1]) : false;
      const char *shname = argc > 2 ? argv[2] : NULL;
      for (auto version : d3d::smAll)
      {
        if (dsc.shaderModel < version) // || !dd_file_exist(String(0, "%s.%s.shdump.bin", shname, version.psName))
          continue;
        bool reloaded = useFence ? load_shaders_bindump_with_fence(shname ? shname : last_loaded_shaders_bindump(false), version)
                                 : load_shaders_bindump(shname, version);
        if (!reloaded)
        {
          console::print_d("failed to reload %s, ver=%d (%s)", last_loaded_shaders_bindump(false), version, version.psName);
          continue;
        }
        console::print_d("reloaded %s, ver=%s", last_loaded_shaders_bindump(false), version.psName);
        reloadCb(true);
        return true;
      }
      console::print_d("failed to reload %s", last_loaded_shaders_bindump(false));
      reloadCb(false);
      return true;
    }

    CONSOLE_CHECK_NAME_EX("render", "switch_debug_shaders", 2, 4, "Loads/unloads debug shaderdump compiled with -debug.",
      "[off/on] [optional:shader_model_version_major] [optional:shader_model_version_minor]")
    {
      if (d3d::get_driver_code() != d3d::dx12 || dgs_get_settings()->getBlockByNameEx("video")->getBool("compatibilityMode", false))
      {
        console::print_d("This feature is only availabel on DX12 PC version, with compability mode off");
        return true;
      }

      bool load = console::to_bool(argv[1]);

      if (argc > 3)
      {
        if (!(load ? load_shaders_debug_bindump(d3d::shadermodel::Version(console::to_int(argv[2]), console::to_int(argv[3])))
                   : unload_shaders_debug_bindump(d3d::shadermodel::Version(console::to_int(argv[2]), console::to_int(argv[3])))))
        {
          console::print("Failed to load/unload the debug shaderdump");
        }
      }
      else
      {
        const Driver3dDesc &dsc = d3d::get_driver_desc();

        for (auto version : d3d::smAll)
        {
          if (dsc.shaderModel < version)
            continue;

          bool success = load ? load_shaders_debug_bindump(version) : unload_shaders_debug_bindump(version);

          if (success)
          {
            console::print("Succesfully loaded/unloaded debug dump");
            break;
          }
          console::print_d("Failed to load/unload the debug dump of version: %s", version.psName);
        }
      }
    }

    return found;
  }

private:
  ShaderReloadCb reloadCb;
};

class ShadersCmdProcessor : public console::ICommandProcessor
{
public:
  ShadersCmdProcessor() : console::ICommandProcessor(1000) {}
  void destroy() {}

  virtual bool processCommand(const char *argv[], int argc)
  {
    int found = 0;

    CONSOLE_CHECK_NAME("render", "shaderVar", 2, 6)
    {
      if (argc < 2)
      {
        console::print("usage: shaderVar <var_name> [<var_value>]");
        return true;
      }
      int id = get_shader_variable_id(argv[1], true);
      if (id < 0)
      {
        console::print_d(" shaderVar <%s> is undefined", argv[1]);
        return true;
      }
      int type = ShaderGlobal::get_var_type(id);
      if (argc > 2)
      {
        switch (type)
        {
          case -1: console::print_d(" shaderVar <%s> is undefined", argv[1]); break;
          case SHVT_COLOR4:
            if (argc >= 6)
              ShaderGlobal::set_color4(id, Color4(atof(argv[2]), atof(argv[3]), atof(argv[4]), atof(argv[5])));
            else
              console::print_d(" four components needed");
            break;
          case SHVT_REAL: ShaderGlobal::set_real(id, atof(argv[2])); break;
          case SHVT_INT: ShaderGlobal::set_int(id, atoi(argv[2])); break;
          case SHVT_INT4:
          {
            if (argc >= 6)
              ShaderGlobal::set_int4(id, IPoint4(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5])));
            else
              console::print_d(" four components needed");
          }
          break;
          case SHVT_TEXTURE:
            if (stricmp(argv[2], "null") == 0)
              ShaderGlobal::set_texture(id, BAD_TEXTUREID);
            else
              ShaderGlobal::set_texture(id, get_managed_texture_id(argv[2]));
            break;
          case SHVT_BUFFER:
            if (stricmp(argv[2], "null") == 0)
              ShaderGlobal::set_buffer(id, BAD_TEXTUREID);
            else
              ShaderGlobal::set_buffer(id, get_managed_res_id(argv[2]));
            break;
          default: break;
        };
      }
      else
        switch (type)
        {
          case -1: console::print_d(" shaderVar <%s> is undefined", argv[1]); break;
          case SHVT_FLOAT4X4:
          {
            TMatrix4 mat = ShaderGlobal::get_float4x4(id);
            console::print_d("%g %g %g %g\n%g %g %g %g\n%g %g %g %g\n%g %g %g %g", P4D(mat.getrow(0)), P4D(mat.getrow(1)),
              P4D(mat.getrow(2)), P4D(mat.getrow(3)));
          }
          break;
          case SHVT_COLOR4:
          {
            Color4 color = ShaderGlobal::get_color4_fast(id);
            console::print_d(" %g %g %g %g", color.r, color.g, color.b, color.a);
          }
          break;
          case SHVT_REAL:
          {
            float val = ShaderGlobal::get_real_fast(id);
            console::print_d(" %g ", val);
          }
          break;
          case SHVT_INT:
          {
            int val = ShaderGlobal::get_int_fast(id);
            console::print_d(" %d ", val);
          }
          break;
          case SHVT_INT4:
          {
            IPoint4 v = ShaderGlobal::get_int4(id);
            console::print_d(" %d %d %d %d", v.x, v.y, v.z, v.w);
          }
          break;
          case SHVT_TEXTURE:
          {
            TEXTUREID val = ShaderGlobal::get_tex_fast(id);
            console::print_d(" #%d = <%s> ", val, get_managed_texture_name(val));
          }
          break;
          case SHVT_BUFFER:
          {
            D3DRESID val = ShaderGlobal::get_buf_fast(id);
            console::print_d(" buf:#%d = <%s> ", val, get_managed_res_name(val));
          }
          break;
          case SHVT_SAMPLER:
          {
            d3d::SamplerHandle val = ShaderGlobal::get_sampler(id);
            console::print_d(" sampler: 0x%llX ", eastl::to_underlying(val));
          }
          default: break;
        };
    }
    CONSOLE_CHECK_NAME("render", "shaderBlk", 2, 2)
    {
      DataBlock blk;
      if (blk.load(argv[1]))
        ShaderGlobal::set_vars_from_blk(blk);
    }
    CONSOLE_CHECK_NAME("app", "tex", 1, 2)
    {
      console::CommandStruct unused_cmd_info;
      if (find_console_command("app.mem", unused_cmd_info))
        console::command("app.mem");

      int curTex = -1, curVs = -1, curPs = -1, curVdecl = -1, curVb = -1, curIb = -1, curStblk = -1;
      d3d::get_cur_used_res_entries(curTex, curVs, curPs, curVdecl, curVb, curIb, curStblk);
      debug("\nResource allocation\n"
            "-------------------\n"
            "%d Tex, %d VS, %d PS, %d Vdecl, %d VB, %d IB, %d Stblk\n\n",
        curTex, curVs, curPs, curVdecl, curVb, curIb, curStblk);

      uint32_t numTextures = 0;
      uint64_t totalMem = 0;
      String texStat;
      if (argc == 2)
        d3d::get_texture_statistics(&numTextures, &totalMem, &texStat);
      else
        d3d::get_texture_statistics(NULL, NULL, &texStat);

      const char *line = texStat.str();
      bool the_end = texStat.empty();
      for (char *pos = texStat.str(); !the_end; pos++)
      {
        the_end = *pos == 0;
        if (*pos == '\n' || *pos == '\0')
        {
          if (*pos == '\n')
          {
            while (*pos == '\n' || *pos == '\r')
              ++pos;
            --pos;
          }
          *pos = 0;
          console::print_d(line);
          *pos = '\n';
          line = pos + 1;
        }
      }

      if (argc == 2)
      {
        file_ptr_t file = df_open(argv[1], DF_WRITE | DF_CREATE);
        if (file)
          df_write(file, texStat.str(), texStat.length());
        df_close(file);
      }
    }
    CONSOLE_CHECK_NAME("app", "tex_refs", 1, 1)
    {
      Tab<TEXTUREID> ids;
      ids.reserve(32 << 10);
      for (TEXTUREID i = first_managed_texture(1); i != BAD_TEXTUREID; i = next_managed_texture(i, 1))
        ids.push_back(i);
      sort(ids, &sort_texid_by_refc);

      debug("");
      for (TEXTUREID texId : ids)
        debug("  [%5d] refc=%-3d %s", texId, get_managed_texture_refcount(texId), get_managed_texture_name(texId));
      console::print_d("total %d referenced textures", ids.size());
    }
#if DAGOR_DBGLEVEL > 0
    CONSOLE_CHECK_NAME("app", "save_tex", 2, 3)
    {
#if _TARGET_PC
      TEXTUREID texId = get_managed_texture_id(argv[1]);
      if (texId == BAD_TEXTUREID) // worth trying to check shader_vars as well
        texId = ShaderGlobal::get_tex_fast(::get_shader_variable_id(argv[1], true));
      if (texId == BAD_TEXTUREID) // worth trying to *-terminated name as well
        texId = get_managed_texture_id(String(0, "%s*", argv[1]));
      bool srgb = argc > 2 ? (atoi(argv[2]) > 0) : false;

      const char *tex_nm = get_managed_texture_name(texId);
      if (BaseTexture *tex = acquire_managed_tex(texId))
      {
        String tex_fn(TextureMetaData::decodeFileName(tex_nm));
        tex_fn.replaceAll("*", "");
        tex_fn.replaceAll(":", "_");
        tex_fn.replaceAll(" ", "_");

        String outTexFn(0, "SavedTex/%s", tex_fn);
        dd_mkpath(outTexFn);
        switch (tex->getType())
        {
          case D3DResourceType::TEX:
            save_tex_as_ddsx((Texture *)tex, outTexFn + ".ddsx", srgb);
            console::print_d("saved Tex(%s) as %s.ddsx", tex_nm, outTexFn);
            break;
          case D3DResourceType::CUBETEX:
            save_cubetex_as_ddsx((CubeTexture *)tex, outTexFn + ".cube.ddsx", srgb);
            console::print_d("saved CubeTex(%s) as %s.cube.ddsx", tex_nm, outTexFn);
            break;
          case D3DResourceType::VOLTEX:
            save_voltex_as_ddsx((VolTexture *)tex, outTexFn + ".vol.ddsx", srgb);
            console::print_d("saved VolTex(%s) as %s.vol.ddsx", tex_nm, outTexFn);
            break;
          default: console::print_d("ERR: texture(%s) type=%d", tex_nm, eastl::to_underlying(tex->getType()));
        }
      }
      else
        console::print_d("ERR: failed to resolve texture name %s,  texId=%d (%s)", argv[1], texId, tex_nm);
      release_managed_tex(texId);
#else
      console::print_d("ERR: app.save_tex requires _TARGET_PC");
#endif
    }
    CONSOLE_CHECK_NAME("app", "save_all_tex", 1, 1)
    {
      int tex_cnt = 0;
      int64_t total_sz = 0;
      for (TEXTUREID i = first_managed_texture(1); i != BAD_TEXTUREID; i = next_managed_texture(i, 1))
        if (BaseTexture *tex = acquire_managed_tex(i))
        {
          String fn(TextureMetaData::decodeFileName(get_managed_texture_name(i)));
          fn.replaceAll("*", "");
          fn.replaceAll(":", "_");
          dd_mkpath("SavedTex/" + fn);
          bool res = false;
          if (tex->getType() == D3DResourceType::TEX)
            res = save_tex_as_ddsx((Texture *)tex, fn = String(0, "SavedTex/%s.ddsx", fn));
          else if (tex->getType() == D3DResourceType::CUBETEX)
            res = save_cubetex_as_ddsx((CubeTexture *)tex, fn = String(0, "SavedTex/%s.cube.ddsx", fn));
          else if (tex->getType() == D3DResourceType::VOLTEX)
            res = save_voltex_as_ddsx((VolTexture *)tex, fn = String(0, "SavedTex/%s.vol.ddsx", fn));
          release_managed_tex(i);
          if (res)
          {
            tex_cnt++;
            if (file_ptr_t fp = df_open(fn, DF_READ))
            {
              total_sz += df_length(fp);
              df_close(fp);
            }
          }
          else
            console::error("failed to save %s", fn);
        }
      console::print_d("saved %d textures, %dM total", tex_cnt, total_sz >> 20);
    }
#endif
    CONSOLE_CHECK_NAME("app", "stcode", 1, 1) { dump_exec_stcode_time(); }
    CONSOLE_CHECK_NAME("app", "stcode_avg_perf_dump", 1, 1) { stcode::profile::dump_avg_time(); }
    CONSOLE_CHECK_NAME("render", "reset_device", 1, 1) { dagor_d3d_force_driver_reset = true; }
    CONSOLE_CHECK_NAME("render", "hang_device", 2, 2)
    {
      logdbg("Registering GPU hang on %s", argv[1]);
      d3dhang::hang_gpu_on(argv[1]);
    }
#if _TARGET_PC_WIN
    CONSOLE_CHECK_NAME("render", "send_gpu_dump", 2, 3)
    {
      if (argc < 2)
      {
        console::print("usage: [<dump-type>] <dump-file-path>\nvalid <dump-type> values are:\nnv-gpudump (default)");
      }
      else
      {
        const char *dumpType = "nv-gpudmp";
        const char *dumpPath = "";
        if (argc > 2)
        {
          dumpType = argv[1];
          dumpPath = argv[2];
        }
        else
        {
          dumpPath = argv[1];
        }
        file_ptr_t file = df_open(dumpPath, DF_READ);
        if (file)
        {
          intptr_t length = df_length(file);
          if (length > 0)
          {
            auto buf = eastl::make_unique<uint8_t[]>(length);
            df_read(file, buf.get(), length);
            d3d::driver_command(Drv3dCommand::SEND_GPU_CRASH_DUMP, const_cast<char *>(dumpType), buf.get(),
              reinterpret_cast<void *>(length));
          }
          df_close(file);
        }
        else
          console::error("Can't open GPU dump file %s", dumpPath);
      }
    }
#endif

    return found;
  }
  static int sort_texid_by_refc(const TEXTUREID *a, const TEXTUREID *b)
  {
    if (int rc_diff = get_managed_texture_refcount(*a) - get_managed_texture_refcount(*b))
      return -rc_diff;
    return strcmp(get_managed_texture_name(*a), get_managed_texture_name(*b));
  }
};

void shaders_set_reload_flags()
{
#if (_TARGET_IOS | _TARGET_TVOS | _TARGET_ANDROID | _TARGET_C3) || DAGOR_DBGLEVEL == 0
  shaders_internal::shader_reload_allowed = false;
#else
  shaders_internal::shader_reload_allowed = true;
#endif
  shaders_internal::shader_reload_allowed =
    dgs_get_settings()->getBlockByNameEx("graphics")->getBool("shader_reload_allowed", shaders_internal::shader_reload_allowed);
  shaders_internal::shader_pad_for_reload =
    dgs_get_settings()->getBlockByNameEx("graphics")->getInt("shader_pad_for_reload", shaders_internal::shader_pad_for_reload);
}

static InitOnDemand<ShadersCmdProcessor> shaders_consoleproc;
static InitOnDemand<ShadersReloadProcessor> shaders_reload_consoleproc;

void shaders_register_console(bool allow_reload, const ShaderReloadCb &after_reload_cb)
{
  shaders_set_reload_flags();
  shaders_consoleproc.demandInit();
  add_con_proc(shaders_consoleproc);
  if (allow_reload)
  {
    shaders_reload_consoleproc.demandInit(after_reload_cb);
    add_con_proc(shaders_reload_consoleproc);
  }
}
