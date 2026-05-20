// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sqmodules/sqmodules.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_pathDelim.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_threads.h>
#include <ioSys/dag_findFiles.h>
#include <util/dag_string.h>
#include <util/dag_simpleString.h>
#include <util/dag_delayedAction.h>
#include <memory/dag_mem.h>
#include <quirrel/sqEventBus/sqEventBus.h>
#include <json/value.h>
#include <zip.h>
#include <string.h>


namespace bindquirrel
{

static bool add_file_to_zip(zip_t *zip_archive, const char *file_path, const char *archive_path, String *error_details)
{
  zip_source_t *source = zip_source_file(zip_archive, file_path, 0, 0);
  if (!source)
  {
    if (error_details)
      error_details->printf(0, "Failed to create zip source for file '%s'", file_path);
    return false;
  }

  zip_int64_t idx = zip_file_add(zip_archive, archive_path, source, ZIP_FL_OVERWRITE);
  if (idx < 0)
  {
    zip_source_free(source);
    if (error_details)
      error_details->printf(0, "Failed to add file '%s' to archive as '%s'", file_path, archive_path);
    return false;
  }

  return true;
}

static bool add_directory_to_zip(zip_t *zip_archive, const char *dir_path, const char *base_path, String *error_details)
{
  String mask(0, "%s/*", dir_path);

  for (const alefind_t &ff : dd_find_iterator(mask.c_str(), DA_FILE))
  {
    String full_path(0, "%s/%s", dir_path, ff.name);

    const char *rel_start = full_path.str() + strlen(base_path);
    if (*rel_start == PATH_DELIM || *rel_start == PATH_DELIM_BACK)
      rel_start++;
    String rel_path(rel_start);

    if (!add_file_to_zip(zip_archive, full_path.str(), rel_path.str(), error_details))
      return false;
  }

  for (const alefind_t &ff : dd_find_iterator(mask.c_str(), DA_SUBDIR))
  {
    if (strcmp(ff.name, ".") == 0 || strcmp(ff.name, "..") == 0)
      continue;

    String full_path(0, "%s/%s", dir_path, ff.name);

    if (!add_directory_to_zip(zip_archive, full_path.str(), base_path, error_details))
      return false;
  }

  return true;
}

static bool zip_folder_impl(const char *folder_path, const char *zip_path, eastl::string &out_error)
{
  if (!dd_dir_exist(folder_path))
  {
    out_error.sprintf("Folder '%s' does not exist", folder_path);
    return false;
  }

  int zip_error_code = 0;
  zip_t *zip_archive = zip_open(zip_path, ZIP_CREATE | ZIP_TRUNCATE, &zip_error_code);
  if (!zip_archive)
  {
    zip_error_t error;
    zip_error_init_with_code(&error, zip_error_code);
    out_error.sprintf("Failed to create zip archive '%s': %s", zip_path, zip_error_strerror(&error));
    zip_error_fini(&error);
    return false;
  }

  String error_details;
  bool success = add_directory_to_zip(zip_archive, folder_path, folder_path, &error_details);

  if (!success)
  {
    out_error.sprintf("Failed to add files to zip archive: %s%s%s", zip_strerror(zip_archive), error_details.empty() ? "" : ". ",
      error_details.empty() ? "" : error_details.str());
    zip_discard(zip_archive);
    return false;
  }

  if (zip_close(zip_archive) < 0)
  {
    out_error.sprintf("Failed to close zip archive: %s", zip_strerror(zip_archive));
    zip_discard(zip_archive);
    return false;
  }

  return true;
}

static int zip_job_mgr_id = -1;

static int get_zip_job_mgr_id()
{
  if (zip_job_mgr_id < 0)
  {
    G_ASSERT(cpujobs::is_inited());
    zip_job_mgr_id = cpujobs::create_virtual_job_manager(128 << 10, WORKER_THREADS_AFFINITY_MASK, "ZipFolder");

    if (DAGOR_UNLIKELY(zip_job_mgr_id < 0))
    {
      logerr("Failed to create zip job manager");
    }
  }
  return zip_job_mgr_id;
}

class ZipFolderJob : public cpujobs::IJob
{
  eastl::string folderPath;
  eastl::string zipPath;
  eastl::string eventName;

public:
  ZipFolderJob(const char *folder_path, const char *zip_path, const char *event_name) :
    folderPath(folder_path), zipPath(zip_path), eventName(event_name)
  {}

  const char *getJobName(bool &) const override { return "ZipFolderJob"; }

  void doJob() override
  {
    eastl::string errorMsg;
    bool success = zip_folder_impl(folderPath.c_str(), zipPath.c_str(), errorMsg);

    eastl::string evtName = eastl::move(eventName);
    eastl::string evtError = eastl::move(errorMsg);
    bool evtSuccess = success;

    run_action_on_main_thread([evtName = eastl::move(evtName), evtSuccess, evtError = eastl::move(evtError)]() {
      Json::Value data;
      data["success"] = evtSuccess;
      if (!evtSuccess)
        data["error"] = evtError.c_str();
      sqeventbus::send_event(evtName.c_str(), data);
    });
  }

  void releaseJob() override { delete this; }
};

static SQInteger sq_zip_folder_async(HSQUIRRELVM vm)
{
  const char *folder_path = nullptr;
  const char *zip_path = nullptr;
  const char *event_name = nullptr;

  sq_getstring(vm, 2, &folder_path);
  sq_getstring(vm, 3, &zip_path);
  sq_getstring(vm, 4, &event_name);

  if (!dd_dir_exist(folder_path))
  {
    return sq_throwerror(vm, String(0, "Folder '%s' does not exist", folder_path));
  }

  int mgr_id = get_zip_job_mgr_id();
  if (DAGOR_UNLIKELY(mgr_id < 0))
  {
    return sq_throwerror(vm, "Zip job manager was not created");
  }

  cpujobs::add_job(mgr_id, new ZipFolderJob(folder_path, zip_path, event_name));

  return 0;
}

static SQInteger sq_zip_folder(HSQUIRRELVM vm)
{
  const char *folder_path = nullptr;
  const char *zip_path = nullptr;

  sq_getstring(vm, 2, &folder_path);
  sq_getstring(vm, 3, &zip_path);

  if (!dd_dir_exist(folder_path))
  {
    return sq_throwerror(vm, String(0, "Folder '%s' does not exist", folder_path));
  }

  if (eastl::string errorMsg; !zip_folder_impl(folder_path, zip_path, errorMsg))
  {
    return sq_throwerror(vm, errorMsg.c_str());
  }

  sq_pushbool(vm, SQTrue);
  return 1;
}

static SQInteger sq_unzip_folder(HSQUIRRELVM vm)
{
  const char *zip_path = nullptr;
  const char *dest_path = nullptr;

  if (SQ_FAILED(sq_getstring(vm, 2, &zip_path)))
    return sq_throwerror(vm, "Expected zip file path as first argument");

  if (SQ_FAILED(sq_getstring(vm, 3, &dest_path)))
    return sq_throwerror(vm, "Expected destination folder path as second argument");

  int zip_error_code = 0;
  zip_t *zip_archive = zip_open(zip_path, ZIP_RDONLY, &zip_error_code);
  if (!zip_archive)
  {
    zip_error_t error;
    zip_error_init_with_code(&error, zip_error_code);
    String errorMessage(0, "Failed to open zip archive '%s': %s", zip_path, zip_error_strerror(&error));
    zip_error_fini(&error);
    return sq_throwerror(vm, errorMessage);
  }

  dd_mkdir(dest_path);

  zip_int64_t num_entries = zip_get_num_entries(zip_archive, 0);
  bool success = true;
  String errorMessage;

  for (zip_int64_t i = 0; i < num_entries; ++i)
  {
    const char *name = zip_get_name(zip_archive, i, 0);
    if (!name)
    {
      errorMessage.printf(0, "Failed to get entry name at index %d", (int)i);
      success = false;
      break;
    }

    String entry_path(0, "%s/%s", dest_path, name);

    size_t name_len = strlen(name);
    if (name_len > 0 && (name[name_len - 1] == PATH_DELIM || name[name_len - 1] == PATH_DELIM_BACK))
    {
      dd_mkpath(entry_path.str());
      continue;
    }

    const char *last_slash = strrchr(entry_path.str(), PATH_DELIM);
    const char *last_backslash = strrchr(entry_path.str(), PATH_DELIM_BACK);
    const char *last_sep = (last_slash > last_backslash) ? last_slash : last_backslash;

    if (last_sep)
    {
      String dir_path(0, "%.*s", (int)(last_sep - entry_path.str()), entry_path.str());
      dd_mkpath(dir_path.str());
    }

    zip_file_t *zf = zip_fopen_index(zip_archive, i, 0);
    if (!zf)
    {
      errorMessage.printf(0, "Failed to open file '%s' in archive", name);
      success = false;
      break;
    }

    zip_stat_t stat;
    if (zip_stat_index(zip_archive, i, 0, &stat) < 0)
    {
      zip_fclose(zf);
      errorMessage.printf(0, "Failed to get file info for '%s'", name);
      success = false;
      break;
    }

    Tab<uint8_t> buffer(tmpmem);
    buffer.resize(stat.size);

    zip_int64_t bytes_read = zip_fread(zf, buffer.data(), stat.size);
    zip_fclose(zf);

    if (bytes_read < 0)
    {
      errorMessage.printf(0, "Failed to read file '%s' from archive", name);
      success = false;
      break;
    }

    file_ptr_t out_file = df_open(entry_path.str(), DF_WRITE | DF_CREATE);
    if (!out_file)
    {
      errorMessage.printf(0, "Failed to create file '%s'", entry_path.str());
      success = false;
      break;
    }

    if (df_write(out_file, buffer.data(), bytes_read) != bytes_read)
    {
      df_close(out_file);
      errorMessage.printf(0, "Failed to write file '%s'", entry_path.str());
      success = false;
      break;
    }

    df_close(out_file);
  }

  zip_close(zip_archive);

  if (!success)
    return sq_throwerror(vm, errorMessage);

  sq_pushbool(vm, SQTrue);
  return 1;
}

void register_zip_archive(SqModules *module_mgr)
{
  Sqrat::Table exports(module_mgr->getVM());

  ///@module zip
  exports //
    .SquirrelFuncDeclString(sq_zip_folder, "zip_folder(folder_path: string, zip_path: string): bool")
    .SquirrelFuncDeclString(sq_zip_folder_async, "zip_folder_async(folder_path: string, zip_path: string, event_name: string): bool")
    .SquirrelFuncDeclString(sq_unzip_folder, "unzip_folder(zip_path: string, dest_path: string): bool")
    /**/;

  module_mgr->addNativeModule("zip", exports);
}

} // namespace bindquirrel
