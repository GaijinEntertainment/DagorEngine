//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include "daScript/simulate/fs_file_info.h"
#include <daScript/misc/sysos.h>
#include <osApiWrappers/dag_files.h>
#include <ska_hash_map/flat_hash_map2.hpp>

#define DASLIB_MODULE_NAME "daslib"

namespace bind_dascript
{
enum class HotReload
{
  DISABLED,
  ENABLED,
};
enum class LoadDebugCode
{
  NO,
  YES,
};

class FsFileInfo final : public das::FileInfo
{
  const char *source = nullptr;
  uint32_t sourceLength = 0;
  bool owner = false;

public:
  void freeSourceData() override
  {
    if (owner && source)
      memfree_anywhere((void *)source);
    owner = false;
    source = nullptr;
    sourceLength = 0u;
  }
  virtual void getSourceAndLength(const char *&src, uint32_t &len)
  {
    if (owner)
    {
      src = source;
      len = sourceLength;
      return;
    }
    src = nullptr;
    len = 0;
    file_ptr_t f = df_open(name.c_str(), DF_READ | DF_IGNORE_MISSING);
    if (!f)
    {
      logerr("das: script file %s not found", name.c_str());
      return;
    }
    int fileLength = 0;
    src = df_get_vromfs_file_data_for_file_ptr(f, fileLength);
    if (src)
    {
      len = (uint32_t)fileLength;
      df_close(f);
      return;
    }
    fileLength = df_length(f);
    source = (char *)memalloc(fileLength, strmem);
    G_VERIFY(df_read(f, (void *)source, fileLength) == fileLength);

    owner = true;
    src = source;
    sourceLength = len = (uint32_t)fileLength;
    df_close(f);
  }

  virtual ~FsFileInfo() { freeSourceData(); }
};


class DagFileAccess final : public das::ModuleFileAccess
{
  das::das_map<das::string, das::string> extraRoots;
  das::FileAccess *localAccess;
  das::FileAccessPtr makeLocalFileAccess()
  {
    auto res = das::make_smart<DagFileAccess>(bind_dascript::HotReload::DISABLED);
    localAccess = res.get();
    localAccess->addRef();
    return res;
  }

public:
  ska::flat_hash_map<eastl::string, int64_t, ska::power_of_two_std_hash<eastl::string>> filesOpened; // in order to reload changed
                                                                                                     // scripts, we would need to store
                                                                                                     // pointers to FsFileInfo in
                                                                                                     // script
  bool storeOpenedFiles = true;
  bool derivedAccess = false;
  DagFileAccess *owner = nullptr;
  DagFileAccess(const char *pak = nullptr, HotReload allow_hot_reload = HotReload::ENABLED) // -V730 Not all members of a class are
                                                                                            // initialized inside the constructor.
                                                                                            // Consider inspecting: localAccess. (@see
                                                                                            // makeLocalFileAccess)
    :
    das::ModuleFileAccess(pak, makeLocalFileAccess()), storeOpenedFiles(allow_hot_reload == HotReload::ENABLED)
  {}
  DagFileAccess(HotReload allow_hot_reload = HotReload::ENABLED) : storeOpenedFiles(allow_hot_reload == HotReload::ENABLED)
  {
    localAccess = nullptr;
  }
  DagFileAccess(DagFileAccess *modAccess, HotReload allow_hot_reload = HotReload::ENABLED) :
    storeOpenedFiles(allow_hot_reload == HotReload::ENABLED), owner(modAccess)
  {
    localAccess = nullptr;
    if (modAccess)
    {
      context = modAccess->context;
      modGet = modAccess->modGet;
      includeGet = modAccess->includeGet;
      moduleAllowed = modAccess->moduleAllowed;
      derivedAccess = true;
    }
  }
  virtual ~DagFileAccess() override
  {
    if (derivedAccess)
      context = nullptr;
    if (localAccess)
      localAccess->delRef();
  }
  bool invalidateFileInfo(const das::string &fileName) override
  {
    bool res = false;
    if (owner)
      res = owner->invalidateFileInfo(fileName) || res;
    res = das::ModuleFileAccess::invalidateFileInfo(fileName) || res;
    return res;
  }
  virtual das::FileInfo *getNewFileInfo(const das::string &fname) override
  {
    if (owner)
    {
      auto res = owner->getFileInfo(fname);
      if (storeOpenedFiles && res)
      {
        bool found = false;
        for (auto &f : owner->filesOpened)
          if (f.first == fname)
          {
            filesOpened.emplace(fname, f.second);
            found = true;
            break;
          }
        if (EASTL_UNLIKELY(!found))
        {
          DagorStat stat;
          int64_t mtime = 0;
          if (df_stat(fname.c_str(), &stat) != -1)
            mtime = stat.mtime;
          filesOpened.emplace(fname, mtime);
        }
      }
      return res;
    }
    file_ptr_t f = df_open(fname.c_str(), DF_READ | DF_IGNORE_MISSING);
    if (!f)
    {
      logerr("Script file %s not found", fname.c_str());
      return nullptr;
    }
    if (storeOpenedFiles)
    {
      DagorStat stat;
      int64_t mtime = 0;
      if (df_fstat(f, &stat) != -1)
        mtime = stat.mtime;
      filesOpened.emplace(fname, mtime);
    }
    df_close(f);
    return setFileInfo(fname, das::make_unique<bind_dascript::FsFileInfo>());
  }

  virtual das::ModuleInfo getModuleInfo(const das::string &req, const das::string &from) const override
  {
    if (!failed())
      return das::ModuleFileAccess::getModuleInfo(req, from);
    auto np = req.find_first_of("./");
    if (np != das::string::npos)
    {
      das::string top = req.substr(0, np);
      das::string mod_name = req.substr(np + 1);
      das::ModuleInfo info;
      if (top == DASLIB_MODULE_NAME)
      {
        info.moduleName = mod_name;
        info.fileName = das::getDasRoot() + "/" + DASLIB_MODULE_NAME + "/" + info.moduleName + ".das";
        return info;
      }
      const auto extraRemappedPath = extraRoots.find(top);
      if (extraRemappedPath != extraRoots.end())
      {
        info.moduleName = mod_name;
        info.fileName = das::string(das::string::CtorSprintf(), "%s/%s.das", extraRemappedPath->second.c_str(), mod_name.c_str());
        return info;
      }
    }
    bool reqUsesMountPoint = strncmp(req.c_str(), "%", 1) == 0;
    return das::ModuleFileAccess::getModuleInfo(req, reqUsesMountPoint ? das::string() : from);
  }

  das::string getIncludeFileName(const das::string &fileName, const das::string &incFileName) const override
  {
    const bool incUsesMount = strncmp(incFileName.c_str(), "%", 1) == 0;
    return das::ModuleFileAccess::getIncludeFileName(incUsesMount ? das::string() : fileName, incFileName);
  }
  virtual bool addFsRoot(const das::string &mod, const das::string &path) override
  {
    extraRoots[mod] = path;
    return true;
  }
};
} // namespace bind_dascript
