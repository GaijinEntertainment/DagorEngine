// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <nvCacheTracker/nvCacheTracker.h>

#include <debug/dag_log.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_findFiles.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <startup/dag_globalSettings.h>
#include <statsd/statsd.h>
#include <util/dag_finally.h>
#include <util/dag_globDef.h>
#include <util/dag_hash.h>
#include <util/dag_string.h>

#include <cstring>
#include <EASTL/iterator.h>
#include <EASTL/sort.h>
#include <EASTL/type_traits.h>
#include <EASTL/unique_ptr.h>
#include <MinHook.h>
#include <Windows.h>


static uint64_t nv_dx_cache_limit = UINT64_MAX;
static constexpr const char *SHADER_DUMPS_STAMP_FILE_NAME = "cache/nv_dx_cache_tracker.blk";

static int64_t get_shader_dumps_stamp(const char *shader_dump_dir)
{
  Tab<SimpleString> dumps;
  find_files_in_folder(dumps, shader_dump_dir, ".shdump.bin", false, true, true);
  eastl::sort(dumps.begin(), dumps.end(), [](const SimpleString &a, const SimpleString &b) { return strcmp(a, b) < 0; });

  uint64_t stamp = FNV1Params<64>::offset_basis;
  for (const SimpleString &dump : dumps)
  {
    DagorStat st;
    if (df_stat(dump, &st) < 0)
      continue;
    stamp = str_hash_fnv1<64>(dump, stamp);
    stamp = mem_hash_fnv1<64>(reinterpret_cast<const char *>(&st.size), sizeof(st.size), stamp);
    stamp = mem_hash_fnv1<64>(reinterpret_cast<const char *>(&st.mtime), sizeof(st.mtime), stamp);
  }
  return int64_t(stamp);
}

static bool are_shader_dumps_changed(const char *shader_dump_dir)
{
  const int64_t stamp = get_shader_dumps_stamp(shader_dump_dir);
  DataBlock storedBlk;
  const bool loaded =
    dd_file_exists(SHADER_DUMPS_STAMP_FILE_NAME) && dblk::load(storedBlk, SHADER_DUMPS_STAMP_FILE_NAME, dblk::ReadFlag::ROBUST);
  if (loaded && storedBlk.getInt64("shaderDumps", 0) == stamp)
    return false;
  DataBlock blk;
  blk.setInt64("shaderDumps", stamp);
  dd_mkpath(SHADER_DUMPS_STAMP_FILE_NAME);
  if (!blk.saveToTextFile(SHADER_DUMPS_STAMP_FILE_NAME))
    logwarn("Failed to save <%s>, the NV DX cache warm limit stays off", SHADER_DUMPS_STAMP_FILE_NAME);
  return true;
}

static bool ends_with_nvph(LPCWSTR path)
{
  static constexpr const wchar_t suffix[] = L".nvph";
  constexpr size_t suffixLen = eastl::size(suffix) - 1; // exclude null
  const size_t len = wcslen(path);
  return len >= suffixLen && std::memcmp(path + (len - suffixLen), suffix, suffixLen * sizeof(wchar_t)) == 0;
}

using CreateFileW_t = eastl::add_pointer_t<decltype(::CreateFileW)>;
static CreateFileW_t fp_create_file_w;

DAGOR_NOINLINE
static bool handle_nvph(LPCWSTR lpFileName, HANDLE handle)
{
  constexpr size_t sufficientLenght = 512;
  auto logEntry = eastl::make_unique<char[]>(sufficientLenght);
  if (int result = WideCharToMultiByte(CP_UTF8, 0, lpFileName, -1, logEntry.get(), sufficientLenght, nullptr, nullptr); //
      result == 0) [[unlikely]]
  {
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
      int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, lpFileName, -1, nullptr, 0, nullptr, nullptr);
      logEntry = eastl::make_unique<char[]>(sizeNeeded);
      WideCharToMultiByte(CP_UTF8, 0, lpFileName, -1, logEntry.get(), sizeNeeded, nullptr, nullptr);
    }
    else
      return false;
  }

  logdbg("%s is %s", logEntry.get(), handle != INVALID_HANDLE_VALUE ? "opened" : "going to be created");

  if (handle != INVALID_HANDLE_VALUE)
  {
    BY_HANDLE_FILE_INFORMATION fileInfo;
    SYSTEMTIME ct, mt;
    if (GetFileInformationByHandle(handle, &fileInfo) &&       //
        FileTimeToSystemTime(&fileInfo.ftCreationTime, &ct) && //
        FileTimeToSystemTime(&fileInfo.ftLastWriteTime, &mt))
    {
      const auto size = (uint64_t(fileInfo.nFileSizeHigh) << 32) | fileInfo.nFileSizeLow;

      statsd::profile("nv_dx_cache_size_kb", long(size >> 10));

      logdbg("Size: %" PRId64 ", creation time: %d-%02d-%02d %02d:%02d:%02d, modification time: %d-%02d-%02d %02d:%02d:%02d", size,
        ct.wYear, ct.wMonth, ct.wDay, ct.wHour, ct.wMinute, ct.wSecond, mt.wYear, mt.wMonth, mt.wDay, mt.wHour, mt.wMinute,
        mt.wSecond);

      if (size > nv_dx_cache_limit)
      {
        logdbg(".nvph file is going to be deleted!");
        ::CloseHandle(handle);
        ::DeleteFileW(lpFileName);
        statsd::counter("nv_dx_cache_file_deleted");
        // reset shader warmup status, to do it again
        ::DeleteFile("shader_warmup_status.blk");
        dd_erase("cache/dx12.cache");

        return true;
      }
    }
  }

  return false;
}

// clang-format off

static HANDLE WINAPI hooked_create_file_w(
    LPCWSTR lpFileName,
    DWORD   dwDesiredAccess,
    DWORD   dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD   dwCreationDisposition,
    DWORD   dwFlagsAndAttributes,
    HANDLE  hTemplateFile)
{
  // Call the original function
  auto handle = fp_create_file_w(
    lpFileName,
    dwDesiredAccess,
    dwShareMode,
    lpSecurityAttributes,
    dwCreationDisposition,
    dwFlagsAndAttributes,
    hTemplateFile);

  // clang-format on

  if (handle != INVALID_HANDLE_VALUE && lpFileName && ends_with_nvph(lpFileName)) [[unlikely]]
  {
    if (handle_nvph(lpFileName, handle))
      return INVALID_HANDLE_VALUE;
  }

  return handle;
}

void init_nv_cache_tracker(const char *shader_dump_dir)
{
  auto &videoBlk = *::dgs_get_settings()->getBlockByNameEx("video");
  // if not set in blk the exact value, do not perform delete.
  if (int nvDxCacheLimitMb = videoBlk.getInt("nvDxCacheLimitMb", 0); nvDxCacheLimitMb > 0)
    nv_dx_cache_limit = uint64_t(nvDxCacheLimitMb) << 20;

  // allow disabling tracker for safety
  if (!videoBlk.getBool("nvDxCacheTracker", true))
    return;

  if (int warmLimitMb = videoBlk.getInt("nvDxCacheWarmLimitMb", 0); warmLimitMb > 0 && shader_dump_dir)
  {
    const bool changed = are_shader_dumps_changed(shader_dump_dir);
    if (!changed)
      nv_dx_cache_limit = uint64_t(warmLimitMb) << 20;
    logdbg("NV DX cache limit: %llu MiB, shader dumps in <%s> %s", nv_dx_cache_limit >> 20, shader_dump_dir,
      changed ? "changed" : "unchanged");
  }

  if (auto result = MH_Initialize(); result == MH_OK || result == MH_ERROR_ALREADY_INITIALIZED)
  {
    bool mh_initialized = result == MH_OK;
    FARPROC createFileW = ::GetProcAddress(::GetModuleHandleW(L"KernelBase.dll"), "CreateFileW");
    if (result = MH_CreateHook(createFileW, &hooked_create_file_w, reinterpret_cast<LPVOID *>(&fp_create_file_w)); result == MH_OK)
    {
      if (result = MH_EnableHook(createFileW); result == MH_OK)
        ;
      else
        LOGERR_CTX("MH_EnableHook is failed with %d", result);
    }
    else
      LOGERR_CTX("MH_CreateHook is failed with %d", result);

    static Finally hookUninitializer{[mh_initialized] {
      if (mh_initialized)
        MH_Uninitialize();
    }};
  }
  else
    LOGERR_CTX("MH_Initialize is failed with %d", result);
}
