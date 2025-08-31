// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

extern int dll_pull_osapiwrappers_asyncRead;
extern int dll_pull_osapiwrappers_basePathMgr;
extern int dll_pull_osapiwrappers_critsec;
extern int dll_pull_osapiwrappers_timed_critsec;
extern int dll_pull_osapiwrappers_dbgStr;
extern int dll_pull_osapiwrappers_dbgStrFmt;
extern int dll_pull_osapiwrappers_directoryService;
extern int dll_pull_osapiwrappers_directory_watch;
extern int dll_pull_osapiwrappers_findFile;
extern int dll_pull_osapiwrappers_getRealFname;
extern int dll_pull_osapiwrappers_localCmp;
extern int dll_pull_osapiwrappers_cpuJobs;
extern int dll_pull_osapiwrappers_miscApi;
extern int dll_pull_osapiwrappers_ip2str;
extern int dll_pull_osapiwrappers_simplifyFname;
extern int dll_pull_osapiwrappers_progGlobals;
extern int dll_pull_osapiwrappers_setThreadName;
extern int dll_pull_osapiwrappers_setTitle;
extern int dll_pull_osapiwrappers_winXSaveFeatures;
extern int dll_pull_osapiwrappers_wndProcComponent;
extern int dll_pull_osapiwrappers_clipboard;
extern int dll_pull_osapiwrappers_unicode;
extern int dll_pull_osapiwrappers_threads;
extern int dll_pull_osapiwrappers_events;
extern int dll_pull_osapiwrappers_globalMutex;
extern int dll_pull_osapiwrappers_sharedMem;
extern int dll_pull_osapiwrappers_spinlock;
extern int dll_pull_osapiwrappers_symHlp;
extern int dll_pull_osapiwrappers_dynLib;
extern int dll_pull_osapiwrappers_shellExecute;
extern int dll_pull_osapiwrappers_messageBox;
extern int dll_pull_osapiwrappers_sockets;
extern int dll_pull_osapiwrappers_mmap;
extern int dll_pull_osapiwrappers_virtualMem;
extern int dll_pull_osapiwrappers_cpuFeatures;

extern int dll_pull_kernel_cpuFreq;
extern int dll_pull_kernel_dagorHwExcept;
extern int dll_pull_kernel_debug;
extern int dll_pull_kernel_fatalerr;
extern int dll_pull_kernel_kernelGlobalSetting;
extern int dll_pull_kernel_log;
extern int dll_pull_kernel_debugDumpStack;
extern int dll_pull_kernel_cpu_control;
extern int dll_pull_kernel_perfTimer;
extern int dll_pull_perfMon_daProfilerLogDump;

extern int dll_pull_memory_dagmem;
extern int dll_pull_memory_mspaceAlloc;
extern int dll_pull_memory_framemem;
extern int dll_pull_memory_physmem;

extern int dll_pull_iosys_asyncIo;
extern int dll_pull_iosys_asyncIoCached;
extern int dll_pull_iosys_asyncWrite;
extern int dll_pull_iosys_baseIo;
extern int dll_pull_iosys_fileIo;
extern int dll_pull_iosys_ioUtils;
extern int dll_pull_iosys_memIo;
extern int dll_pull_iosys_obsolete_cfg;
extern int dll_pull_iosys_zlibIo;
extern int dll_pull_iosys_lzmaDecIo;
extern int dll_pull_iosys_lzmaEnc;
extern int dll_pull_iosys_zstdIo;
extern int dll_pull_iosys_oodleIo;
extern int dll_pull_iosys_chainedMemIo;
extern int dll_pull_iosys_msgIo;
extern int dll_pull_iosys_fastSeqRead;
extern int dll_pull_iosys_vromfsLoad;
extern int dll_pull_iosys_findFiles;

extern int dll_pull_iosys_datablock_core;
extern int dll_pull_iosys_datablock_errors;
extern int dll_pull_iosys_datablock_parser;
extern int dll_pull_iosys_datablock_readbbf3;
extern int dll_pull_iosys_datablock_serialize;
extern int dll_pull_iosys_datablock_zstd;
extern int dll_pull_iosys_datablock_to_json;

extern int dll_pull_baseutil_bitArray;
extern int dll_pull_baseutil_dobject;
extern int dll_pull_baseutil_fatalCtx;
extern int dll_pull_baseutil_fastStrMap;
extern int dll_pull_baseutil_hierBitMem;
extern int dll_pull_baseutil_strImpl;
extern int dll_pull_baseutil_syncExecScheduler;
extern int dll_pull_baseutil_tabMem;
extern int dll_pull_baseutil_tabSorted;
extern int dll_pull_baseutil_restart;
extern int dll_pull_baseutil_texMetaData;
extern int dll_pull_baseutil_unicodeHlp;
extern int dll_pull_baseutil_threadPool;
extern int dll_pull_baseutil_watchdog;
extern int dll_pull_baseutil_delayedActions;
extern int dll_pull_baseutil_treeBitmap;
extern int dll_pull_baseutil_lag;
extern int dll_pull_baseutil_fnameMap;
extern int dll_pull_baseutil_fileMd5Validate;
extern int dll_pull_baseutil_parallel_for;

extern int pull_dll_sum;

extern class OcclusionMap *occlusion_map;
extern int hdr_render_mode;
extern unsigned hdr_render_format;
