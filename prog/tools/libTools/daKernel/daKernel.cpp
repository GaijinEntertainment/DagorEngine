#include "pull.h"
#include <util/dag_stdint.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_statDrv.h>
#include <supp/dag_define_COREIMP.h>

bool g_enable_time_profiler = false; // legacy way
bool g_gpu_profiler_on = false;      // legacy way

#if _TARGET_64BIT
size_t dagormem_first_pool_sz = size_t(8192) << 20;
size_t dagormem_next_pool_sz = size_t(512) << 20;
#endif
namespace dblk
{
KRNLIMP void save_to_bbf3(const DataBlock &blk, IGenSave &cwr);
}

int pull_dll_sum =

#if _TARGET_PC
  (int)(intptr_t)(void *)(&da_profiler::dump_frames) + dll_pull_osapiwrappers_asyncRead + dll_pull_osapiwrappers_basePathMgr +
  dll_pull_osapiwrappers_critsec + dll_pull_osapiwrappers_timed_critsec + dll_pull_osapiwrappers_dbgStr +
  dll_pull_osapiwrappers_dbgStrFmt + dll_pull_osapiwrappers_directoryService + dll_pull_osapiwrappers_directory_watch +
  dll_pull_osapiwrappers_findFile + dll_pull_osapiwrappers_getRealFname + dll_pull_osapiwrappers_localCmp +
  dll_pull_osapiwrappers_cpuJobs + dll_pull_osapiwrappers_miscApi + dll_pull_osapiwrappers_simplifyFname +
  dll_pull_osapiwrappers_winProgGlobals + dll_pull_osapiwrappers_winSetThreadName + dll_pull_osapiwrappers_winSetTitle +
#if _TARGET_PC_WIN
  dll_pull_osapiwrappers_winXSaveFeatures +
#endif
  dll_pull_osapiwrappers_wndProcComponent + dll_pull_osapiwrappers_winClipboard + dll_pull_osapiwrappers_unicode +
  dll_pull_osapiwrappers_threads + dll_pull_osapiwrappers_events + dll_pull_osapiwrappers_globalMutex +
  dll_pull_osapiwrappers_sharedMem + dll_pull_osapiwrappers_spinlock + dll_pull_osapiwrappers_symHlp + dll_pull_osapiwrappers_dynLib +
  dll_pull_osapiwrappers_shellExecute +
#if _TARGET_PC_LINUX | _TARGET_PC_WIN
  dll_pull_osapiwrappers_messageBox +
#endif
  dll_pull_osapiwrappers_sockets + dll_pull_osapiwrappers_mmap + dll_pull_osapiwrappers_virtualMem +
  dll_pull_osapiwrappers_cpuFeatures +

  dll_pull_kernel_cpuFreq + dll_pull_kernel_dagorHwExcept + dll_pull_kernel_debug + dll_pull_kernel_fatalerr +
  dll_pull_kernel_kernelGlobalSetting + dll_pull_kernel_log + dll_pull_kernel_debugDumpStack + dll_pull_kernel_cpu_control +
  dll_pull_kernel_perfTimer +

  dll_pull_memory_dagmem + dll_pull_memory_mspaceAlloc + dll_pull_memory_framemem +

  dll_pull_iosys_asyncIo + dll_pull_iosys_asyncIoCached + dll_pull_iosys_asyncWrite + dll_pull_iosys_baseIo + dll_pull_iosys_fileIo +
  dll_pull_iosys_ioUtils + dll_pull_iosys_memIo + dll_pull_iosys_obsolete_cfg + dll_pull_iosys_zlibIo + dll_pull_iosys_lzmaDecIo +
  dll_pull_iosys_lzmaEnc + dll_pull_iosys_zstdIo + dll_pull_iosys_oodleIo + dll_pull_iosys_chainedMemIo + dll_pull_iosys_msgIo +
  dll_pull_iosys_fastSeqRead + dll_pull_iosys_vromfsLoad + dll_pull_iosys_findFiles +

  dll_pull_iosys_datablock_core + dll_pull_iosys_datablock_errors + dll_pull_iosys_datablock_parser +
  dll_pull_iosys_datablock_readbbf3 + dll_pull_iosys_datablock_serialize + dll_pull_iosys_datablock_zstd +
  dll_pull_iosys_datablock_to_json + (int)(intptr_t)(void *)&dblk::save_to_bbf3 +

  dll_pull_baseutil_bitArray + dll_pull_baseutil_dobject + dll_pull_baseutil_fatalCtx + dll_pull_baseutil_fastStrMap +
  dll_pull_baseutil_hierBitMem + dll_pull_baseutil_strImpl + dll_pull_baseutil_syncExecScheduler + dll_pull_baseutil_tabMem +
  dll_pull_baseutil_tabSorted + dll_pull_baseutil_restart + dll_pull_baseutil_texMetaData + dll_pull_baseutil_unicodeHlp +
  dll_pull_baseutil_threadPool + dll_pull_baseutil_watchdog + dll_pull_baseutil_delayedActions + dll_pull_baseutil_treeBitmap +
  dll_pull_baseutil_lag + dll_pull_baseutil_fnameMap + dll_pull_baseutil_fileMd5Validate +

  (int)(intptr_t)visibility_finder + (int)hdr_render_mode + (int)hdr_render_format + (int)(intptr_t)occlusion_map +

#endif
  0;

#include <../engine2/math/rndSeed.cpp>
#include <../engine2/math/gaussTbl.cpp>

#include <math/dag_TMatrix.h>
const TMatrix TMatrix::IDENT(1);
const TMatrix TMatrix::ZERO(0);
