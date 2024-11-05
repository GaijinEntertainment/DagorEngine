//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tabFwd.h>
class DataBlock;
class IGenericProgressIndicator;
class ILogWriter;
namespace mkbindump
{
class BinDumpSaveCB;
}


class DagorAssetMgr;
class DagorAsset;
class IDagorAssetExporter;
class String;


class IDaBuildInterface
{
public:
  inline bool checkVersion() { return apiVer == API_VERSION; }
  inline int getDllVer() { return apiVer; }
  inline int getReqVer() { return API_VERSION; }

  virtual void __stdcall release() = 0;

  virtual int __stdcall init(const char *startdir, DagorAssetMgr &mgr, const DataBlock &appblk, const char *appdir,
    const char *ddsxPluginsPath = nullptr) = 0;
  virtual void __stdcall term() = 0;

  virtual void __stdcall setupReports(ILogWriter *l, IGenericProgressIndicator *pb) = 0;

  virtual bool __stdcall loadExporterPlugins() = 0;
  virtual bool __stdcall loadExporterPluginsInFolder(const char *dirpath) = 0;
  virtual bool __stdcall loadSingleExporterPlugin(const char *dll_path) = 0;
  virtual void __stdcall unloadExporterPlugins() = 0;

  virtual bool __stdcall exportAll(dag::ConstSpan<unsigned> tc, const char *profile = NULL) = 0;
  virtual bool __stdcall exportPacks(dag::ConstSpan<unsigned> tc, dag::ConstSpan<const char *> packs, const char *profile = NULL) = 0;
  virtual bool __stdcall exportByFolders(dag::ConstSpan<unsigned> tc, dag::ConstSpan<const char *> folders,
    const char *profile = NULL) = 0;

  virtual bool __stdcall exportRaw(dag::ConstSpan<unsigned> tc, dag::ConstSpan<const char *> packs,
    dag::ConstSpan<const char *> packs_re, bool export_tex, bool export_res, const char *profile = NULL) = 0;

  virtual void __stdcall resetStat() = 0;
  virtual void __stdcall getStat(bool tex_pack, int &processed, int &built, int &failed) = 0;
  virtual int __stdcall getRemovedPackCount() = 0;

  virtual const char *__stdcall getPackName(DagorAsset *asset) = 0;
  virtual const char *__stdcall getPackNameFromFolder(int fld_idx, bool tex_or_res) = 0;
  virtual String __stdcall getPkgName(DagorAsset *asset) = 0;
  virtual bool __stdcall checkUpToDate(dag::ConstSpan<unsigned> tc, dag::Span<int> tc_flags,
    dag::ConstSpan<const char *> packs_to_check, const char *profile = NULL) = 0;

  virtual bool __stdcall isAssetExportable(DagorAsset *asset) = 0;

  virtual void __stdcall destroyCache(dag::ConstSpan<unsigned> tc, const char *profile = NULL) = 0;

  virtual void __stdcall invalidateBuiltRes(const DagorAsset &a, const char *cache_folder) = 0;

  // if asset builds very we don't save it in cache on disk
  virtual bool __stdcall getBuiltRes(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, IDagorAssetExporter *exp, const char *cache_folder,
    String &cache_path, int &data_offset, bool save_all_caches) = 0;

  virtual void __stdcall setExpCacheSharedData(void *) = 0;

  virtual void __stdcall processSrcHashForDestPacks() = 0;

protected:
  static const int API_VERSION = 5;
  int apiVer;
  IDaBuildInterface() : apiVer(API_VERSION) {}
};

//! root DLL function must be declared as:
//!   extern "C" __declspec(dllexport) IDaBuildInterface* __stdcall get_dabuild_interface();
typedef IDaBuildInterface *(__stdcall *get_dabuild_interface_t)();

//! helper routines to load and release daBuild DLL
IDaBuildInterface *get_dabuild(const char *dll_fname);
void release_dabuild(IDaBuildInterface *dabuild);


#if _TARGET_64BIT
#define GET_DABUILD_INTERFACE_PROC "get_dabuild_interface"
#else
#define GET_DABUILD_INTERFACE_PROC "_get_dabuild_interface@0"
#endif
