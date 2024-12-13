// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/dtx/ddsxPlugin.h>
#include <generic/dag_tab.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_symHlp.h>
#include <osApiWrappers/dag_dynLib.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>
#include <3d/ddsxTex.h>

bool ddsx::ConvertParams::forceZlibPacking = false;
bool ddsx::ConvertParams::preferZstdPacking = false;
bool ddsx::ConvertParams::allowOodlePacking = false;
unsigned ddsx::ConvertParams::zstdMaxWindowLog = 0;
int ddsx::ConvertParams::zstdCompressionLevel = 18;

class DdsxAlloc : public IDdsxCreatorPlugin::IAlloc
{
public:
  void *__stdcall alloc(int sz) override { return memalloc(sz, tmpmem); }
  void __stdcall free(void *p) override { memfree(p, tmpmem); }
};

struct PluginState
{
  IDdsxCreatorPlugin *p;
  void *dll;
  bool started;
};

static DdsxAlloc alloc;
static Tab<PluginState> plugins(inimem);
static const char *lastErr = NULL;

typedef IDdsxCreatorPlugin *(__stdcall *get_plugin_t)(IDdsxCreatorPlugin::IAlloc *);

/// loads compatible plugins from given location; returns number of plugins
int ddsx::load_plugins(const char *dirpath)
{
  alefind_t ff;
  const String mask(260, "%s/*" DAGOR_OS_DLL_SUFFIX, dirpath);
  String fname;
  int num = 0;

  if (::dd_find_first(mask, DA_FILE, &ff))
    do
    {
      fname.printf(260, "%s/%s", dirpath, ff.name);
      dd_simplify_fname_c(fname);
      debug("found: %s", fname.str());

      void *dllHandle = os_dll_load_deep_bind(fname);
      IDdsxCreatorPlugin *p = NULL;

      if (dllHandle)
      {
        get_plugin_t get_plugin = (get_plugin_t)os_dll_get_symbol(dllHandle, DDSX_GET_PLUGIN_PROC);
        debug("get_plugin=%p", (void *)get_plugin);

        if (get_plugin)
        {
          p = get_plugin(&alloc);
          if (p->checkVersion())
          {
            PluginState &st = plugins.push_back();
            st.p = p;
            st.dll = dllHandle;
            st.started = false;
            ::symhlp_load(fname);
            num++;
            debug("plugin for: %c%c%c%c", _DUMP4C(p->targetCode()));
          }
          else
            p = NULL;
        }

        if (!p)
          os_dll_close(dllHandle);
      }
      else
        logwarn("failed to load %s\nerror=%s", fname, os_dll_get_last_error_str());
    } while (::dd_find_next(&ff));

  dd_find_close(&ff);

  return num;
}

/// unloads plugins
void ddsx::unload_plugins()
{
  ddsx::shutdown_plugins();
  for (int i = 0; i < plugins.size(); i++)
    os_dll_close(plugins[i].dll);
  clear_and_shrink(plugins);
}

/// shutdowns all loaded plugins
void ddsx::shutdown_plugins()
{
  for (int i = 0; i < plugins.size(); i++)
    if (plugins[i].started)
    {
      plugins[i].p->shutdown();
      plugins[i].started = false;
    }
}

/// converts dds to target-specific DDSx format
bool ddsx::convert_dds(unsigned target_code, Buffer &dest, void *dds_data, int dds_len, const ConvertParams &params)
{
  dest.zero();
  for (int i = 0; i < plugins.size(); i++)
    if (target_code == plugins[i].p->targetCode())
    {
      if (!plugins[i].started)
      {
        if (!plugins[i].p->startup())
          continue;
        plugins[i].started = true;
      }

      bool ret = plugins[i].p->convertDds(dest, dds_data, dds_len, params);
      lastErr = plugins[i].p->getLastErrorText();
      if (ret && params.canPack && dest.len > 512)
      {
        MemorySaveCB mcwr(dest.len);
        int hdr_size = plugins[i].p->getTexDataHeaderSize(dest.ptr);
        int compr_flag = 0;

        if (ConvertParams::allowOodlePacking && plugins[i].p->checkCompressionSupport(ddsx::Header::FLG_OODLE))
        {
          InPlaceMemLoadCB crd((char *)dest.ptr + hdr_size, dest.len - hdr_size);
          if (oodle_compress_data(mcwr, crd, dest.len - hdr_size) > 0)
            compr_flag = ddsx::Header::FLG_OODLE;
          else
            logerr("failed to pack data (sz=%d) using OODLE", dest.len - hdr_size);
        }

        if (!compr_flag && ConvertParams::preferZstdPacking && plugins[i].p->checkCompressionSupport(ddsx::Header::FLG_ZSTD))
        {
          InPlaceMemLoadCB crd((char *)dest.ptr + hdr_size, dest.len - hdr_size);
          if (zstd_compress_data(mcwr, crd, dest.len - hdr_size, 1 << 20, ConvertParams::zstdCompressionLevel,
                ConvertParams::zstdMaxWindowLog) > 0)
            compr_flag = ddsx::Header::FLG_ZSTD;
          else
            logerr("failed to pack data (sz=%d) using ZSTD", dest.len - hdr_size);
        }

        if (!compr_flag && !ConvertParams::forceZlibPacking && dest.len > params.packSzThres &&
            plugins[i].p->checkCompressionSupport(ddsx::Header::FLG_7ZIP))
        {
          InPlaceMemLoadCB crd((char *)dest.ptr + hdr_size, dest.len - hdr_size);
          if (lzma_compress_data(mcwr, 9, crd, dest.len - hdr_size) > 0)
            compr_flag = ddsx::Header::FLG_7ZIP;
          else
            logerr("failed to pack data (sz=%d) using LZMA", dest.len - hdr_size);
        }

        if (!compr_flag && plugins[i].p->checkCompressionSupport(ddsx::Header::FLG_ZLIB))
        {
          compr_flag = ddsx::Header::FLG_ZLIB;
          ZlibSaveCB z_cwr(mcwr, ZlibSaveCB::CL_BestSpeed + 8);
          z_cwr.write((char *)dest.ptr + hdr_size, dest.len - hdr_size);
          z_cwr.finish();
        }

        int sz = mcwr.getSize();
        if (sz & 7)
        {
          __int64 zero = 0;
          mcwr.write(&zero, 8 - (sz & 7));
          sz = mcwr.getSize();
        }

        if (compr_flag && (sz + hdr_size < dest.len * 9 / 10 || sz + hdr_size + (16 << 10) < dest.len))
        {
          plugins[i].p->setTexDataHeaderCompression(dest.ptr, sz, compr_flag);
          memcpy((char *)dest.ptr + hdr_size, mcwr.getMem()->data, sz);
          dest.len = sz + hdr_size;
        }
      }
      return ret;
    }

  lastErr = "Can't find plugin for target code";
  return false;
}

const char *ddsx::get_last_error_text() { return lastErr; }

void ddsx::Buffer::free()
{
  if (ptr)
    alloc.free(ptr);
  ptr = NULL;
  len = 0;
}
