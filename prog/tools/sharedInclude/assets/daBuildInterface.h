//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tabFwd.h>
#include <libTools/util/iLogWriter.h>
#include <util/dag_string.h>
class DataBlock;
class IGenericProgressIndicator;
namespace mkbindump
{
class BinDumpSaveCB;
}


class DagorAssetMgr;
class DagorAsset;
class IDagorAssetExporter;


class IDaBuildInterface
{
public:
  struct BuildWarning
  {
    ILogWriter::MessageType messageType;
    String message;
  };

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

  virtual String __stdcall getPackName(DagorAsset *asset) = 0;
  virtual String __stdcall getPackNameFromFolder(int fld_idx, bool tex_or_res) = 0;
  virtual String __stdcall getPkgName(DagorAsset *asset) = 0;
  virtual bool __stdcall checkUpToDate(dag::ConstSpan<unsigned> tc, dag::Span<int> tc_flags,
    dag::ConstSpan<const char *> packs_to_check, const char *profile = NULL) = 0;

  // Fast, UI-only counterpart to checkUpToDate() (e.g. AssetViewer's startup scan): never calls
  // gatherSrcDataFiles(), never removes an outdated cache file, marks readiness per-pack not per-asset.
  // Never use this to decide whether to actually rebuild anything; use checkUpToDate() for that.
  // out_* get the totals accumulated across all requested platforms.
  virtual bool __stdcall quickCheckUpToDate(dag::ConstSpan<unsigned> tc, dag::Span<int> tc_flags,
    dag::ConstSpan<const char *> packs_to_check, int &out_ready_packs, int &out_total_packs, int &out_removed_cache_files,
    int &out_worker_threads, const char *profile = NULL) = 0;

  virtual bool __stdcall isAssetExportable(DagorAsset *asset) = 0;

  virtual void __stdcall destroyCache(dag::ConstSpan<unsigned> tc, const char *profile = NULL) = 0;

  virtual void __stdcall invalidateBuiltRes(const DagorAsset &a, const char *cache_folder) = 0;

  // Writes asset built data to cwr (uses cache or builds on the fly using exporter) and caches it;
  // if asset is built very fast we don't save it to cache on disk (unless save_all_caches=true passed).
  // Optional out_served_from_respack is set to true when the bytes came from an already-built respack rather
  // than a fresh/cached per-asset rebuild - the latter always embeds materials (see setFastBuildFlag),
  // so only a respack-served result can have materials split into *Desc.bin.
  virtual bool __stdcall getBuiltRes(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, IDagorAssetExporter *exp, const char *cache_folder,
    String &cache_path, int &data_offset, bool save_all_caches, bool *out_served_from_respack = nullptr) = 0;

  // Get build warnings for the given asset from the saved cache file.
  // cache_up_to_date: output parameter. It is set to true if the cache file is up-to-date (so a getBuiltRes would not rebuild it).
  // Returns true if the cache file exists.
  virtual bool __stdcall getBuiltResWarnings(DagorAsset &asset, IDagorAssetExporter &exp, const char *cache_folder,
    dag::Vector<BuildWarning> &warnings, bool &cache_up_to_date) = 0;

  virtual void __stdcall setExpCacheSharedData(void *) = 0;

  virtual void __stdcall allowPatchBuild(bool) = 0;

  // True if pkg_name (NULL/"*" for main) has a valid patch build - see detect_valid_patch() in daBuild.cpp.
  virtual bool __stdcall isPatchBuildActive(const char *pkg_name, const char *target_str) = 0;

  // Discards memoized isPatchBuildActive()/getBuiltRes() state; call after an out-of-process dabuild run.
  virtual void __stdcall invalidateRespackCaches() = 0;

  virtual void __stdcall processSrcHashForDestPacks() = 0;

protected:
  static const int API_VERSION = 7;
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
