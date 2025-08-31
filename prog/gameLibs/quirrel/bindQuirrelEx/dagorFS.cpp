// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <quirrel/sqModules/sqModules.h>
#include <quirrel/bindQuirrelEx/bindQuirrelEx.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_findFiles.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/basePath.h>
#include <osApiWrappers/dag_vromfs.h>
#include <memory/dag_framemem.h>

#if _TARGET_PC_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif


static void list_directories(Tab<SimpleString> &out_list, const String &root, bool recursive)
{
  String mask(0, "%s/*", root.c_str());

  for (const alefind_t &ff : dd_find_iterator(mask.c_str(), DA_SUBDIR))
  {
    if (ff.attr & DA_SUBDIR)
    {
      if (strcmp(ff.name, ".") == 0 || strcmp(ff.name, "..") == 0)
        continue;
      String path(0, "%s/%s", root.c_str(), ff.name);
      out_list.push_back() = path;
      if (recursive)
        list_directories(out_list, path, true);
    }
  }
}

static SQInteger sq_scan_folder_impl(HSQUIRRELVM vm, bool force_vrom_only)
{
  bool vromfs = true;
  bool realfs = true;
  bool recursive = true;
  bool directories_only = false;
  eastl::string root = ".";
  eastl::string files_suffix = "";

  Sqrat::Table args = Sqrat::Var<Sqrat::Table>(vm, 2).value;
  vromfs = force_vrom_only || args.GetSlotValue("vromfs", vromfs);
  realfs = !force_vrom_only && args.GetSlotValue("realfs", realfs);
  recursive = args.GetSlotValue("recursive", recursive);
  root = args.GetSlotValue("root", root);
  files_suffix = args.GetSlotValue("files_suffix", files_suffix);
  directories_only = args.GetSlotValue("directories_only", directories_only);

  Tab<SimpleString> filesList;

  if (directories_only)
  {
    if (realfs)
      list_directories(filesList, String(root.c_str()), recursive);
  }
  else
    find_files_in_folder(filesList, root.c_str(), files_suffix.c_str(), vromfs, realfs, recursive);

  Sqrat::Array result(vm, filesList.size());
  for (SQInteger i = 0; i < filesList.size(); ++i)
    result.SetValue(i, filesList[size_t(i)].str());
  Sqrat::PushVarR(vm, result);
  return 1;
}

static SQInteger sq_scan_folder(HSQUIRRELVM vm) { return sq_scan_folder_impl(vm, false); }

static SQInteger sq_scan_vrom_folder(HSQUIRRELVM vm) { return sq_scan_folder_impl(vm, true); }

static SQInteger sq_get_vromfs_dump_version(HSQUIRRELVM vm)
{
  const char *fname = nullptr;
  sq_getstring(vm, 2, &fname);

  uint32_t version = ::get_vromfs_dump_version(fname);

  sq_pushinteger(vm, version);
  return 1;
}

static SQInteger find_files(HSQUIRRELVM v)
{
  const char *mask = nullptr;
  SQInteger maxCount = -1;

  G_VERIFY(SQ_SUCCEEDED(sq_getstring(v, 2, &mask)));
  if (sq_gettop(v) > 2)
  {
    Sqrat::Var<Sqrat::Table> params(v, 3);
    maxCount = params.value.GetSlotValue("maxCount", -1);
  }

  Sqrat::Array res(v);

#if _TARGET_PC_WIN
  uint32_t prevErrorMode = SetErrorMode(0);
  SetErrorMode(prevErrorMode | SEM_FAILCRITICALERRORS);
#endif

  SQInteger fileIdx = 0;

  for (const alefind_t &fh : dd_find_iterator(mask, DA_SUBDIR | DA_READONLY | DA_HIDDEN | DA_SYSTEM))
  {
    Sqrat::Table file(v);
    file.SetValue("name", fh.name);
    file.SetValue("isDirectory", (fh.attr & DA_SUBDIR) != 0);
    file.SetValue("isReadOnly", (fh.attr & DA_READONLY) != 0);
    file.SetValue("isHidden", (fh.attr & DA_HIDDEN) != 0);
    file.SetValue("isSystem", (fh.attr & DA_SYSTEM) != 0);
    if (!(fh.attr & DA_SUBDIR))
      file.SetValue("size", fh.size);
    if (fh.atime != -1)
      file.SetValue("accessTime", fh.atime);
    if (fh.mtime != -1)
      file.SetValue("modifyTime", fh.mtime);
    if (fh.ctime != -1)
#if _TARGET_PC_WIN
      file.SetValue("creationTime", fh.ctime);
#else
      file.SetValue("metadataTime", fh.ctime);
#endif
    res.Append(file);
    if (++fileIdx == maxCount)
      break;
  }

#if _TARGET_PC_WIN
  SetErrorMode(prevErrorMode);
#endif

  sq_pushobject(v, res);
  return 1;
}


// TODO: implement Dagor file API for Squirrel
static SQInteger read_text_from_file(HSQUIRRELVM vm)
{
  const char *fn;
  sq_getstring(vm, 2, &fn);
  const int mode = SqModules::tryOpenFilesFromRealFS ? DF_READ : DF_VROM_ONLY | DF_READ;
  FullFileLoadCB cb(fn, mode);
  if (!cb.fileHandle)
    return sqstd_throwerrorf(vm, "Failed to open file '%s'", fn);

  int size;
  const void *data = df_mmap(cb.fileHandle, &size);
  if (data)
  {
    sq_pushstring(vm, (const char *)data, size);
    df_unmap(data, size);
  }
  else if (df_length(cb.fileHandle) == 0)
  {
    sq_pushstring(vm, "", 0);
  }
  else
    return sq_throwerror(vm, String(0, "Cannot 'mmap' content in file '%s'", fn));
  return 1;
}

static SQInteger read_text_from_file_on_disk(HSQUIRRELVM vm)
{
  const char *fn;
  sq_getstring(vm, 2, &fn);
  FullFileLoadCB cb(fn, DF_REALFILE_ONLY | DF_READ);
  if (!cb.fileHandle)
    return sqstd_throwerrorf(vm, "Failed to open file '%s'", fn);

  int size;
  const void *data = df_mmap(cb.fileHandle, &size);
  if (data)
  {
    sq_pushstring(vm, (const char *)data, size);
    df_unmap(data, size);
  }
  else if (df_length(cb.fileHandle) == 0)
  {
    sq_pushstring(vm, "", 0);
  }
  else
    return sq_throwerror(vm, String(0, "Cannot 'mmap' content in file '%s'", fn));
  return 1;
}

static SQInteger write_text_to_file(HSQUIRRELVM vm)
{
  const char *savePath = nullptr;
  sq_getstring(vm, 2, &savePath);

  const char *data = nullptr;
  sq_getstring(vm, 3, &data);

  if (!dd_file_exist(savePath))
  {
    dd_mkpath(savePath);
  }

  if (FullFileSaveCB saver(savePath); saver.fileHandle)
  {
    saver.write(data, strlen(data));
  }
  else
  {
    return sq_throwerror(vm, String(64, "Cannot write text to file \"%s\"", savePath));
  }

  return 1;
}

static SQInteger sq_resolve_mountpoint(HSQUIRRELVM vm)
{
  const char *path = nullptr;
  SQInteger pathSize = 0;
  sq_getstringandsize(vm, 2, &path, &pathSize);

  eastl::string resolvedPath;
  if (dd_resolve_named_mount(resolvedPath, path))
    sq_pushstring(vm, resolvedPath.c_str(), resolvedPath.size());
  else
    sq_pushstring(vm, path, pathSize);
  return 1;
}

namespace bindquirrel
{

static SQInteger stat(HSQUIRRELVM vm)
{
  const char *path = nullptr;
  sq_getstring(vm, 2, &path);

  DagorStat buf;
  if (df_stat(path, &buf) < 0)
    return 0;

  sq_newtableex(vm, 4);

  sq_pushstring(vm, "size", 4);
  sq_pushinteger(vm, buf.size);
  sq_rawset(vm, -3);

  sq_pushstring(vm, "atime", 5);
  sq_pushinteger(vm, buf.atime);
  sq_rawset(vm, -3);

  sq_pushstring(vm, "mtime", 5);
  sq_pushinteger(vm, buf.mtime);
  sq_rawset(vm, -3);

  sq_pushstring(vm, "ctime", 5);
  sq_pushinteger(vm, buf.ctime);
  sq_rawset(vm, -3);

  return 1;
}

void register_dagor_fs_module(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  Sqrat::Table exports(vm);

  /* qdox @module dagor.fs
      All functions in this module are sync.
      Better never use them in production code, cause this can cause freeze and fps stutter (and usually will).
  */
  exports //
    .Func("file_exists", dd_file_exists)
    ///@param path s
    ///@return b
    .Func("dir_exists", dd_dir_exists)
    ///@param path s
    ///@return b
    .Func("mkdir", dd_mkdir)
    ///@param path s
    ///@return b
    .Func("mkpath", dd_mkpath)
    ///@param path s
    ///@return b
    .Func("remove_file", dd_erase)
    ///@param path s
    ///@return b
    .Func("is_path_absolute", is_path_abs)
    ///@param path s
    ///@return b
    .SquirrelFunc("scan_folder", sq_scan_folder, 2, ".t")
    /* qdox
    @kwarged
    @param root s : where to start scan
    @param vromfs b : scan in vromfs
    @param realfs b : scan in real file system
    @param recursive b : scan in real file system
    @param files_suffix s : suffix of files to be included in result
    @param directories_only b : return only directories (works for realfs only)

    @return a : array of found files with path on root
    */
    .SquirrelFunc("find_files", find_files, -2, ".st")
    /* qdox
    @param file_mask s : file mask like *.nut or foo/some.nut
    @param params t : table of params, the only possible param is {maxCount:i = -1} (-1 is default, means no limit)
    @return a : array of file_info
    @code file_info sq
      {name:s, isDirectory:b, isReadOnly:b, isHidden:b, size:i, accessTime:i, modifyTime:i}
    */
    .SquirrelFunc("read_text_from_file", read_text_from_file, 2, ".s")
    /* qdox
    will open file from in VROMS only in some cases (on platforms different then PC, on not allowed modding)
    @return s : file as string
    */
    .SquirrelFunc("read_text_from_file_on_disk", read_text_from_file_on_disk, 2, ".s")
    /* qdox
    will open file from disk only
    @return s : file as string
    */
    .SquirrelFunc("write_text_to_file", write_text_to_file, 3, ".ss")
    /* qdox
    @param file_path s : path to file to write
    @param text s : text to write to file
    will write text to file, creating directories if needed
    */
    .SquirrelFunc("stat", stat, 2, ".s")
    /* qdox
    @param filename s : path to file to get info
    @return t|o : file_stat or or null if file not found
    @code file_stat sq
      {size:i, atime:i, mtime:i, ctime:i}
    code@
    */
    .SquirrelFunc("resolve_mountpoint", sq_resolve_mountpoint, 2, ".s")
    /* qdox
    @param file_path s : path with mount point
    @return s : path with resolved mount point or path as is
    */
    /**/;

  module_mgr->addNativeModule("dagor.fs", exports);
}


void register_dagor_fs_vrom_module(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  Sqrat::Table exports(vm);

  ///@module dagor.fs.vrom
  exports.SquirrelFunc("scan_vrom_folder", sq_scan_vrom_folder, 2, ".t");
  exports.SquirrelFunc("get_vromfs_dump_version", sq_get_vromfs_dump_version, 2, ".s");

  module_mgr->addNativeModule("dagor.fs.vrom", exports);
}


} // namespace bindquirrel
