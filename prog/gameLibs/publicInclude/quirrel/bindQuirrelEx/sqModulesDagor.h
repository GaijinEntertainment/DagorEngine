//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sqmodules/sqmodules.h>
#include <generic/dag_enumBitMask.h>
#include <dag/dag_vector.h>


enum class SqModulesConfigBits
{
  None = 0,
  PermissiveModuleNames = 1, // Do not check native|script modules file extension (otherwise script modules should have ".nut"
                             // extension, while native shouldn't)
  Default = None
};
DAGOR_ENABLE_ENUM_BITMASK(SqModulesConfigBits);


struct SqModulesDagorFileAccess : public ISqModulesFileAccess
{
  SqModulesDagorFileAccess(bool delete_self, SqModulesConfigBits cfg = SqModulesConfigBits::Default);

  virtual void destroy() override;

  virtual void resolveFileName(const char *requested_fn, const char *running_script, string &res) override;
  // when file can't be read, return false and fill out_err_msg
  virtual bool readFile(const string &resolved_fn, const char *requested_fn, vector<char> &buf, string &out_err_msg) override;

  virtual void getSearchTargets(const char * /*fn*/, bool &search_native, bool &search_script) override;

  string makeRelativeFilename(const char *cur_file, const char *requested_fn);

  SqModulesConfigBits configBits;
  bool useAbsolutePath = false;
  bool needToDeleteSelf = false;

  // if this flag is false we won't try to open files from real file system if there are missing in vromfs.
  // this is to allow modding on PC and useAddonVromfs in dev build
#if _TARGET_PC || DAGOR_DBGLEVEL > 0
  static bool tryOpenFilesFromRealFS;
#else
  static constexpr bool tryOpenFilesFromRealFS = false;
#endif

private:
  dag::Vector<char> scriptPathBuf; // cache
};


extern SqModulesDagorFileAccess sq_modules_dagor_file_access;
