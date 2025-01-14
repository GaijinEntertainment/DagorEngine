// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/string.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_files.h>
#include <vecmath/dag_vecMathDecl.h> // DAGOR_ASAN_ENABLED
#include <soundSystem/soundSystem.h>
#include <soundSystem/debug.h>
#include <soundSystem/fmodApi.h>
#include <soundSystem/streams.h>
#include <memory/dag_memBase.h>
#include <memory/dag_physMem.h>
#include <memory/dag_framemem.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_autoFuncProf.h>

#include <fmod_studio.hpp>

#include <debug/dag_log.h>
#include <stdio.h> // for SNPRINTF

#include "internal/delayed.h"
#include "internal/streams.h"
#include "internal/soundSystem.h"
#include "internal/events.h"
#include "internal/occlusion.h"
#include "internal/debug.h"

#include <math/random/dag_random.h>
#include <atomic>

#if _TARGET_IOS
#include "soundSystem_ios.h"
#endif

#if FMOD_VERSION < 0x00020100 && _TARGET_XBOX
#include <fmod_gamecore.h>
#define XBOX_THREAD_AFFINITY FMOD_GAMECORE_THREADAFFINITY
#define XBOX_SET_AFFINITY    FMOD_GameCore_SetThreadAffinity
#endif

#if _TARGET_ANDROID
#include <jni.h>
extern JavaVM *g_jvm;
ScoopedJavaThreadAttacher::ScoopedJavaThreadAttacher()
{
  if (!g_jvm)
    DAG_FATAL("JavaVirtual Machive pointer not setted");
  JNIEnv *env;
  isAttach = false;
  int getEnvStat = g_jvm->GetEnv((void **)&env, JNI_VERSION_1_6);
  if (getEnvStat == JNI_EDETACHED)
  {
    //      DEBUG_CTX("GetEnv: expected behavior not attached");
    if (g_jvm->AttachCurrentThread(&env, NULL) != 0)
      DAG_FATAL("Failed to attach JavaThread");
    isAttach = true;
  }
  else if (getEnvStat == JNI_OK)
  {
    // Not expected behavior but normal
    DEBUG_CTX("GetEnv: not expected behavior attached");
  }
  else if (getEnvStat == JNI_EVERSION)
    DAG_FATAL("Java GetEnv: version not supported");
  else
    DAG_FATAL("Unexpected error Java GetEnv: %u", getEnvStat);
}
ScoopedJavaThreadAttacher::~ScoopedJavaThreadAttacher()
{
  if (!g_jvm)
    DAG_FATAL("JavaVirtual Machive pointer not setted");
  if (isAttach)
    g_jvm->DetachCurrentThread();
}
#endif // _TARGET_ANDROID


#define SOUNDINIT_VERIFY(fmodOperation) SOUND_VERIFY_AND_DO(fmodOperation, return false)

#if (_TARGET_C1 | _TARGET_C2) && !FMOD_SRC_VERSION

#endif

namespace sndsys
{
enum Defaults
{
  DF_MAX_CHANNELS = 1024,
  DF_MAX_SOFTWARE_CHANNELS = 128,
  DF_STACK_SIZE_NON_BLOCKING = 131072,
};

static std::atomic_bool g_is_inited = ATOMIC_VAR_INIT(false);
static uint32_t g_version = 0;
static size_t g_memory_block_size = 0;
static void *g_memory_block = nullptr;
static int g_max_channels = 0;
static int g_max_software_channels = 0;
static bool g_fixed_mem_pool = true;

static record_list_changed_cb_t g_record_list_changed_cb = nullptr;
static output_list_changed_cb_t g_output_list_changed_cb = nullptr;
static device_lost_cb_t g_device_lost_cb = nullptr;

static bool g_auto_change_device = false;

static FMOD::Studio::System *g_system = nullptr;
static FMOD::System *g_low_level_system = nullptr;
static void validate_output(FMOD_OUTPUTTYPE default_output = FMOD_OUTPUTTYPE_AUTODETECT);

namespace settings
{
static eastl::string speakermode;
static int stack_size_nonblocking = 0;
static float virtual_vol_limit = 0;
static bool init_profile = false;
static bool init_live_update = false;
static int maxVorbisCodecs = 0;
static int maxFADPCMCodecs = 0;
static bool fmod_synchronous_mode = false;
static bool enable_spatial_sound = false;
static int samplerate = 48000;
#if DAGOR_DBGLEVEL > 0
static bool memory_tracking_mode = false;
#endif
}; // namespace settings

static void settings_init(const DataBlock &blk)
{
  settings::speakermode = blk.getStr("speakerMode", "");
  settings::stack_size_nonblocking = blk.getInt("stackSizeNonBlocking", DF_STACK_SIZE_NON_BLOCKING);
  settings::virtual_vol_limit = blk.getReal("vol0virtualvol", 0.001f);
  settings::init_profile = blk.getBool("fmodProfile", false);
  settings::init_live_update = blk.getBool("fmodLiveUpdate", false);
  settings::maxVorbisCodecs = blk.getInt("maxVorbisCodecs", 0);
  settings::maxFADPCMCodecs = blk.getInt("maxFADPCMCodecs", 0);
  settings::fmod_synchronous_mode = blk.getBool("fmodSynchronousMode", false);
  settings::samplerate = blk.getInt("sampleRate", 48000);
  settings::enable_spatial_sound = blk.getBool("fmodEnableSpatialSound", false);
#if DAGOR_DBGLEVEL > 0
  settings::memory_tracking_mode = blk.getBool("memoryTrackingMode", false);
#endif

  debug("Fmod settings:");
#define LOG_VALUE(VAL, FMT) debug(" - " #VAL ": " FMT, settings::VAL)
  debug(" - speakermode: \"%s\"", settings::speakermode.c_str());
  LOG_VALUE(stack_size_nonblocking, "%llu");
  LOG_VALUE(virtual_vol_limit, "%f");
  LOG_VALUE(init_profile, "%d");
  LOG_VALUE(init_live_update, "%d");
  LOG_VALUE(maxVorbisCodecs, "%u");
  LOG_VALUE(maxFADPCMCodecs, "%u");
  LOG_VALUE(fmod_synchronous_mode, "%d");
  LOG_VALUE(samplerate, "%u");
  LOG_VALUE(enable_spatial_sound, "%d");
#if DAGOR_DBGLEVEL > 0
  LOG_VALUE(memory_tracking_mode, "%d");
#endif
#undef LOG_VALUE
}

void *F_CALLBACK fmod_malloc_cb(unsigned int size, FMOD_MEMORY_TYPE type, const char *sourcestr)
{
  G_UNREFERENCED(sourcestr);
  G_UNREFERENCED(type);
  void *ptr = memalloc_default(size);
  return ptr;
}

void *F_CALLBACK fmod_realloc_cb(void *ptr, unsigned int size, FMOD_MEMORY_TYPE type, const char *sourcestr)
{
  G_UNREFERENCED(sourcestr);
  G_UNREFERENCED(type);
  ptr = memrealloc_default(ptr, size);
  return ptr;
}

void F_CALLBACK fmod_free_cb(void *ptr, FMOD_MEMORY_TYPE type, const char *sourcestr)
{
  G_UNREFERENCED(sourcestr);
  G_UNREFERENCED(type);
  memfree_default(ptr);
  return;
}

static bool memory_init_custom_allocators()
{
  SOUNDINIT_VERIFY(FMOD::Memory_Initialize(NULL, 0, &fmod_malloc_cb, &fmod_realloc_cb, &fmod_free_cb));
  return true;
}

static bool memory_init_fixed_pool(int blockSize)
{
#if _TARGET_C1 | _TARGET_C2


#else
  g_memory_block = memalloc(blockSize, midmem);
#endif
  G_ASSERT_RETURN(g_memory_block, false);
  SOUNDINIT_VERIFY(FMOD::Memory_Initialize(g_memory_block, blockSize, 0, 0, 0));
  return true;
}

static bool memory_init(const DataBlock &blk)
{
  const int defBlockSizeMb = 175;
  const int defBlockSizeMbLowMemMode = 140;

  bool isLowMemMode = blk.getBool("isLowMemMode", false);
  const char *memParamName = blk.getBool("isHighMemMode", false) ? "memoryPCHighMode" : "memoryPc";
  int blockSizeMb = isLowMemMode ? blk.getInt("memoryPCLowMode", defBlockSizeMbLowMemMode) : blk.getInt(memParamName, defBlockSizeMb);
#if _TARGET_C1 | _TARGET_C2

#endif
  G_ASSERTF_RETURN(blockSizeMb > 0 && blockSizeMb < 1024, false, "Wrong memory block size(%d)", blockSizeMb);
  int blockSize = blockSizeMb << 20;
  g_memory_block_size = blockSize;

#ifdef DAGOR_ASAN_ENABLED
  g_fixed_mem_pool = blk.getBool("isFixedPool", false);
#elif _TARGET_C1 | _TARGET_C2 // FIXME: remove this hardcode (it need to be properly configured in data)

#else
  g_fixed_mem_pool = blk.getBool("isFixedPool", true);
#endif
  if (g_fixed_mem_pool)
    return memory_init_fixed_pool(blockSize);
  else
    return memory_init_custom_allocators();
}

static inline uint32_t major_version(uint32_t version) { return version >> 16; }
static inline uint32_t minor_version(uint32_t version) { return (version >> 8) & 0xff; }
static inline uint32_t dev_version(uint32_t version) { return version & 0xff; }

static bool system_create()
{
  SOUNDINIT_VERIFY(FMOD::Studio::System::create(&g_system));
  SOUNDINIT_VERIFY(g_system->getCoreSystem(&g_low_level_system));
  g_version = 0;
  SOUNDINIT_VERIFY(g_low_level_system->getVersion(&g_version));

  G_VERIFYF(g_version >= FMOD_VERSION, "FMOD version is %x.%02x.%02x, but %x.%02x.%02x is expected", major_version(g_version),
    minor_version(g_version), dev_version(g_version), major_version(FMOD_VERSION), minor_version(FMOD_VERSION),
    dev_version(FMOD_VERSION));

  if (g_version < FMOD_VERSION)
    return false;

  return true;
}

static bool channels_setup()
{
  SOUNDINIT_VERIFY(g_low_level_system->setSoftwareChannels(g_max_software_channels));
  return true;
}
static bool speakermode_setup()
{
#if _TARGET_PC
  int sampleRate = 0;
  int numRawSpeakers = 0;
  FMOD_SPEAKERMODE fmodSpeakerMode = FMOD_SPEAKERMODE_DEFAULT;
  SOUNDINIT_VERIFY(g_low_level_system->getSoftwareFormat(&sampleRate, &fmodSpeakerMode, &numRawSpeakers));

  if (settings::speakermode == "stereo")
    fmodSpeakerMode = FMOD_SPEAKERMODE_STEREO;
  else if (settings::speakermode == "speakers5.1")
    fmodSpeakerMode = FMOD_SPEAKERMODE_5POINT1;
  else if (settings::speakermode == "speakers7.1")
    fmodSpeakerMode = FMOD_SPEAKERMODE_7POINT1;

  sampleRate = settings::samplerate;

  SOUNDINIT_VERIFY(g_low_level_system->setSoftwareFormat(sampleRate, fmodSpeakerMode, numRawSpeakers));
#endif
  return true;
}
static bool advanced_tweak(const DataBlock &blk)
{
#if FMOD_VERSION > 0x00020100
  FMOD::Thread_SetAttributes(FMOD_THREAD_TYPE_NONBLOCKING, FMOD_THREAD_AFFINITY_GROUP_DEFAULT, FMOD_THREAD_PRIORITY_DEFAULT,
    settings::stack_size_nonblocking);
#endif

  FMOD_ADVANCEDSETTINGS advancedSettings;
  memset(&advancedSettings, 0, sizeof(advancedSettings));
  advancedSettings.cbSize = sizeof(advancedSettings);
  SOUNDINIT_VERIFY(g_low_level_system->getAdvancedSettings(&advancedSettings));
#if FMOD_VERSION < 0x00020100
  advancedSettings.stackSizeNonBlocking = settings::stack_size_nonblocking;
#endif
  advancedSettings.maxVorbisCodecs = settings::maxVorbisCodecs;
  advancedSettings.maxFADPCMCodecs = settings::maxFADPCMCodecs;
  if (blk.getBool("randomizeSeed", false))
    advancedSettings.randomSeed = g_rnd_seed;
  // channels will go virtual when their audibility drops below this limit with FMOD_INIT_VOL0_BECOMES_VIRTUAL
  advancedSettings.vol0virtualvol = settings::virtual_vol_limit;

  SOUNDINIT_VERIFY(g_low_level_system->setAdvancedSettings(&advancedSettings));

  FMOD_STUDIO_ADVANCEDSETTINGS fsas = {sizeof(fsas)};
  SOUNDINIT_VERIFY(g_system->getAdvancedSettings(&fsas));
  fsas.commandqueuesize = blk.getInt("commandQueueSize", /*default*/ 0);
  SOUNDINIT_VERIFY(g_system->setAdvancedSettings(&fsas));

  return true;
}

static bool g_null_output = false;
static bool system_init(const DataBlock &blk, bool null_output)
{
  if (!system_create() || !channels_setup() || !speakermode_setup() || !advanced_tweak(blk))
    return false;

  uint32_t fmodInitFlags = FMOD_INIT_NORMAL | FMOD_INIT_VOL0_BECOMES_VIRTUAL;
  if (settings::init_profile)
  {
    debug("FMOD profiling enabled");
    fmodInitFlags |= FMOD_INIT_PROFILE_ENABLE;
  }

  uint32_t fmodStudioInitFlags = FMOD_STUDIO_INIT_NORMAL;
  if (settings::init_live_update)
  {
    debug("FMOD live update enabled");
    fmodStudioInitFlags |= FMOD_STUDIO_INIT_LIVEUPDATE | FMOD_STUDIO_INIT_SYNCHRONOUS_UPDATE; // to prevent freezes because of blocking
                                                                                              // calls to the OS
  }
  if (settings::fmod_synchronous_mode)
    fmodStudioInitFlags |= FMOD_STUDIO_INIT_SYNCHRONOUS_UPDATE;

#if DAGOR_DBGLEVEL > 0
  if (settings::memory_tracking_mode)
  {
    fmodInitFlags |= FMOD_INIT_MEMORY_TRACKING;
    fmodStudioInitFlags |= FMOD_STUDIO_INIT_MEMORY_TRACKING;
  }
#endif
  g_null_output = null_output;
  FMOD_OUTPUTTYPE currentOutput = FMOD_OUTPUTTYPE_UNKNOWN;
  g_low_level_system->getOutput(&currentOutput);
  if (null_output)
  {
    debug("[SNDSYS] initializing with FMOD_OUTPUTTYPE_NOSOUND");
    SOUNDINIT_VERIFY(g_low_level_system->setOutput(FMOD_OUTPUTTYPE_NOSOUND));
  }
#if _TARGET_PC | _TARGET_XBOX
  else if (settings::enable_spatial_sound && settings::samplerate == 48000)
  {
    FMOD_RESULT result = g_low_level_system->setOutput(FMOD_OUTPUTTYPE_WINSONIC);
    if (result != FMOD_OK)
      debug("[SNDSYS] cannot set output FMOD_OUTPUTTYPE_WINSONIC %s", FMOD_ErrorString(result));
  }
#elif _TARGET_C1






#elif _TARGET_ANDROID
  else if (blk.getBool("androidUseOpenSLFallback", false))
  {
    FMOD_RESULT result = g_low_level_system->setOutput(FMOD_OUTPUTTYPE_OPENSL);
    if (result != FMOD_OK)
      debug("[SNDSYS] cannot set output FMOD_OUTPUTTYPE_OPENSL %s", FMOD_ErrorString(result));
  }
#endif

#if _TARGET_XBOX
#if FMOD_VERSION < 0x00020100
  XBOX_THREAD_AFFINITY affinity = {0};
  affinity.mixer = FMOD_THREAD_CORE5;
  affinity.stream = FMOD_THREAD_CORE5;
  affinity.nonblocking = FMOD_THREAD_CORE5;
  affinity.file = FMOD_THREAD_CORE5;
  affinity.geometry = FMOD_THREAD_CORE5;
  affinity.profiler = FMOD_THREAD_CORE5;
  affinity.studioUpdate = FMOD_THREAD_CORE5;
  affinity.studioLoadBank = FMOD_THREAD_CORE5;
  affinity.studioLoadSample = FMOD_THREAD_CORE5;
  XBOX_SET_AFFINITY(&affinity);
#else
  // Allow medium & low priority threads use 6th core as well (it can't interfere with TP Workers since it has above normal prio)
  constexpr FMOD_THREAD_AFFINITY core56 = FMOD_THREAD_AFFINITY_CORE_5 | FMOD_THREAD_AFFINITY_CORE_6;
  FMOD::Thread_SetAttributes(FMOD_THREAD_TYPE_MIXER, FMOD_THREAD_AFFINITY_CORE_5);
  FMOD::Thread_SetAttributes(FMOD_THREAD_TYPE_FEEDER, FMOD_THREAD_AFFINITY_CORE_4); // FMOD_THREAD_PRIORITY_CRITICAL by default, use
                                                                                    // dedicated core
  FMOD::Thread_SetAttributes(FMOD_THREAD_TYPE_STREAM, FMOD_THREAD_AFFINITY_CORE_5);
  FMOD::Thread_SetAttributes(FMOD_THREAD_TYPE_NONBLOCKING, FMOD_THREAD_AFFINITY_CORE_5);
  FMOD::Thread_SetAttributes(FMOD_THREAD_TYPE_FILE, FMOD_THREAD_AFFINITY_CORE_5);
  FMOD::Thread_SetAttributes(FMOD_THREAD_TYPE_GEOMETRY, core56);
  FMOD::Thread_SetAttributes(FMOD_THREAD_TYPE_PROFILER, core56);
  FMOD::Thread_SetAttributes(FMOD_THREAD_TYPE_STUDIO_UPDATE, core56);
  FMOD::Thread_SetAttributes(FMOD_THREAD_TYPE_STUDIO_LOAD_BANK, core56);
  FMOD::Thread_SetAttributes(FMOD_THREAD_TYPE_STUDIO_LOAD_SAMPLE, core56);
#endif
#endif

  {
    AutoFuncProf _prof("FMOD System initialize %d usec");
    FMOD_RESULT result = g_system->initialize(g_max_channels, fmodStudioInitFlags, fmodInitFlags, nullptr);
    switch (result)
    {
      case FMOD_ERR_OUTPUT_INIT:
        debug("[SNDSYS] returned FMOD_ERR_OUTPUT_INIT");
        if (null_output)
          return false;
        g_system->release();
        g_system = nullptr;
        g_low_level_system = nullptr;
        return system_init(blk, true);
      case FMOD_ERR_NET_SOCKET_ERROR: debug("[SNDSYS] returned FMOD_ERR_NET_SOCKET_ERROR"); return false;
      default:
#if _TARGET_ANDROID
        if (result != FMOD_OK)
        {
          if (null_output)
            return false;
          logerr("[SNDSYS] FMOD error: %s", FMOD_ErrorString(result));
          g_system->release();
          g_system = nullptr;
          g_low_level_system = nullptr;
          return system_init(blk, true);
        }
#endif
        SOUNDINIT_VERIFY(result);
    }
  }
#if _TARGET_PC | _TARGET_XBOX | _TARGET_C1
  if (!g_null_output && settings::enable_spatial_sound)
  {
    FMOD_OUTPUTTYPE output = FMOD_OUTPUTTYPE_UNKNOWN;
    SOUND_VERIFY(g_low_level_system->getOutput(&output));
    if (output == FMOD_OUTPUTTYPE_NOSOUND || output == FMOD_OUTPUTTYPE_UNKNOWN)
      debug("[SNDSYS] spatial sound was not set, reset to previous");
    validate_output(currentOutput);
  }
#elif _TARGET_ANDROID
  FMOD_OUTPUTTYPE output = FMOD_OUTPUTTYPE_UNKNOWN;
  SOUND_VERIFY(g_low_level_system->getOutput(&output));
  debug("[SNDSYS] using output %i", (int)output);
#endif
  fmodapi::setup_dagor_audiosys();
  return true;
}

#if DAGOR_DBGLEVEL > 0
static bool treat_fmod_errors_as_warnings = false;
static int get_loglevel(FMOD_DEBUG_FLAGS flags, const char *message)
{
  if (flags & FMOD_DEBUG_LEVEL_ERROR)
  {
    if (treat_fmod_errors_as_warnings)
      return LOGLEVEL_WARN;

    if (strstr(message, "assertion: 'inchannels <= mMixMatrixCurrent.numin()' failed"))
      return LOGLEVEL_WARN; // workaround: suppress FMOD assert to disable logerr reporting
                            // qa.fmod.com/t/assertion-inchannels-mmixmatrixcurrent-numin/14511/2

    if (strstr(message, "returned 0x88890004")) // suppress FMOD assert (0x88890004 = AUDCLNT_E_DEVICE_INVALIDATED)
      return LOGLEVEL_WARN;                     // youtrack.gaijin.team/issue/12-135114 youtrack.gaijin.team/issue/38-45149

    return LOGLEVEL_ERR;
  }

  if (flags & FMOD_DEBUG_LEVEL_WARNING)
    return LOGLEVEL_WARN;

  return LOGLEVEL_DEBUG;
}

static FMOD_RESULT F_CALLBACK fmod_debug_cb(FMOD_DEBUG_FLAGS flags, const char * /*file*/, int /*line*/, const char *func,
  const char *message)
{
  const int ll = get_loglevel(flags, message);

  logmessage(ll, "[FMOD] %s: %.*s", func, /*cut `\n' off */ strlen(message) - 1, message);

  return FMOD_OK;
}
#endif

bool init(const DataBlock &blk)
{
  SNDSYS_IS_MAIN_THREAD;

  G_ASSERT(!is_inited());
  if (is_inited())
    return false;

  ScoopedJavaThreadAttacher scoopedJavaThreadAttacher;

  debug_init(blk);

#if (_TARGET_C1 | _TARGET_C2) && !FMOD_SRC_VERSION

#endif

  g_max_channels = blk.getInt("maxChannels", DF_MAX_CHANNELS);
  g_max_software_channels = blk.getInt("maxSoftwareChannels", DF_MAX_SOFTWARE_CHANNELS);

  settings_init(blk);

#if DAGOR_DBGLEVEL > 0
  treat_fmod_errors_as_warnings = blk.getBool("fmodErrorsAsWarnings", false);
  const char *fmodLoglevel = blk.getStr("fmodLoglevel", "errors");
  FMOD_DEBUG_FLAGS debugFlags = FMOD_DEBUG_LEVEL_NONE;
  if (strcmp(fmodLoglevel, "errors") == 0)
    debugFlags = FMOD_DEBUG_LEVEL_ERROR;
  else if (strcmp(fmodLoglevel, "warnings") == 0)
    debugFlags = FMOD_DEBUG_LEVEL_ERROR | FMOD_DEBUG_LEVEL_WARNING;
  else if (strcmp(fmodLoglevel, "log") == 0)
    debugFlags = FMOD_DEBUG_LEVEL_LOG;
  FMOD_RESULT debugInitializeCbRes = FMOD::Debug_Initialize(debugFlags, FMOD_DEBUG_MODE_CALLBACK, &fmod_debug_cb, nullptr);
  G_VERIFYF(debugInitializeCbRes == FMOD_ERR_UNSUPPORTED || // might be if fmod built without FMOD_DEBUG
              debugInitializeCbRes == FMOD_OK,
    "[SNDSYS] FMOD error: %s", FMOD_ErrorString(debugInitializeCbRes));
#endif

  const bool nullOutput = blk.getBool("nullOutput", false);
  if (!memory_init(blk) || !system_init(blk, nullOutput))
  {
    debug("[SNDSYS] Not inited.");
    shutdown();
    return false;
  }

  events_init(blk);

  streams::init(blk, settings::virtual_vol_limit, g_low_level_system);

  delayed::init(blk);

  if (blk.getBool("occlusionInit", false))
    occlusion::init(blk, g_low_level_system);

  eastl::basic_string<char, framemem_allocator> memoryInfo;
  if (g_fixed_mem_pool)
    memoryInfo.sprintf("memory pool at 0x%p, size", g_memory_block);
  else
    memoryInfo.sprintf("memory limit");

  debug("[SNDSYS] Inited, FMOD version is %x.%02x.%02x, %s %uK, hch %d, sch %d", major_version(g_version), minor_version(g_version),
    dev_version(g_version), memoryInfo.c_str(), g_memory_block_size >> 10, g_max_channels, g_max_software_channels);

#if _TARGET_IOS
  registerIOSNotifications(&set_snd_suspend);
#endif

  g_is_inited = true;

  return g_is_inited;
}

void shutdown()
{
  SNDSYS_IS_MAIN_THREAD;

  ScoopedJavaThreadAttacher scoopedJavaThreadAttacher;

  streams::close();

  delayed::close();
  events_close();

  occlusion::close();

  // debug fmod memory usage
  int used = 0, used_max = 0;
  FMOD::Memory_GetStats(&used, &used_max);
  DEBUG_CTX("[SNDSYS] Shutdown, memory usage (current/max/total): %d/%d/%dK", used >> 10, used_max >> 10, g_memory_block_size >> 10);

  // release system
  if (g_system)
    g_system->release();
  g_system = nullptr;
  g_low_level_system = nullptr;

  // release memory
  if (g_fixed_mem_pool)
  {
#if _TARGET_C1 | _TARGET_C2

#else
    memfree(g_memory_block, midmem);
#endif
    g_memory_block = nullptr;
    g_memory_block_size = 0u;
  }

#if (_TARGET_C1 | _TARGET_C2) && !FMOD_SRC_VERSION

#endif

  g_is_inited = false;
}

bool is_inited() { return g_is_inited; }

void get_memory_statistics(unsigned &system_allocated, unsigned &current_allocated, unsigned &max_allocated)
{
  system_allocated = current_allocated = max_allocated = 0;
  SNDSYS_IF_NOT_INITED_RETURN;
  ScoopedJavaThreadAttacher scoopedJavaThreadAttacher;
  system_allocated = g_memory_block_size;
  FMOD::Memory_GetStats((int *)&current_allocated, (int *)&max_allocated);
}

void get_cpu_usage(sndsys::FmodCpuUsage &fcpu)
{
  FMOD_STUDIO_CPU_USAGE fmodCpu = {};
#if FMOD_VERSION >= 0x00020200
  FMOD_CPU_USAGE fmodCoreCpu = {};
  if (is_inited())
  {
    ScoopedJavaThreadAttacher scoopedJavaThreadAttacher;
    fmodapi::get_studio_system()->getCPUUsage(&fmodCpu, &fmodCoreCpu);
  }
  fcpu.dspusage = fmodCoreCpu.dsp;
  fcpu.streamusage = fmodCoreCpu.stream;
  fcpu.geometryusage = fmodCoreCpu.geometry;
  fcpu.updateusage = fmodCoreCpu.update;
  fcpu.studiousage = fmodCpu.update;
#else
  if (is_inited())
  {
    ScoopedJavaThreadAttacher scoopedJavaThreadAttacher;
    fmodapi::get_studio_system()->getCPUUsage(&fmodCpu);
  }
  fcpu.dspusage = fmodCpu.dspusage;
  fcpu.streamusage = fmodCpu.streamusage;
  fcpu.geometryusage = fmodCpu.geometryusage;
  fcpu.updateusage = fmodCpu.updateusage;
  fcpu.studiousage = fmodCpu.studiousage;
#endif
}
void get_mem_usage(FmodMemoryUsage &fmem)
{
  FMOD_STUDIO_MEMORY_USAGE fmodMem = {};
  if (is_inited())
  {
    ScoopedJavaThreadAttacher scoopedJavaThreadAttacher;
    fmodapi::get_studio_system()->getMemoryUsage(&fmodMem);
    FMOD::Memory_GetStats(&fmem.currentalloced, &fmem.maxalloced, false);
  }
  fmem.inclusive = fmodMem.inclusive;
  fmem.exclusive = fmodMem.exclusive;
  fmem.sampledata = fmodMem.sampledata;
}

static void select_default_device()
{
  SNDSYS_IF_NOT_INITED_RETURN;
  debug("[SNDSYS] select default sound device");
  int driversNum = 0;
  FMOD_RESULT result = g_low_level_system->getNumDrivers(&driversNum);
  if (driversNum > 0)
  {
    result = g_low_level_system->setDriver(0); // 0 = primary or main sound device as selected
                                               // by the operating system settings
    G_ASSERTF(FMOD_OK == result, "[SNDSYS] FMOD error: %s", FMOD_ErrorString(result));
  }
}

static void validate_output(FMOD_OUTPUTTYPE default_output /*= FMOD_OUTPUTTYPE_AUTODETECT*/)
{
  if (g_null_output)
    return;
  FMOD_OUTPUTTYPE output = FMOD_OUTPUTTYPE_UNKNOWN;
  SOUND_VERIFY(g_low_level_system->getOutput(&output));
  if (FMOD_OUTPUTTYPE_NOSOUND == output || FMOD_OUTPUTTYPE_UNKNOWN == output)
    SOUND_VERIFY(g_low_level_system->setOutput(default_output));
}

static FMOD_RESULT F_CALLBACK fmod_system_cb(FMOD_SYSTEM * /*system*/, FMOD_SYSTEM_CALLBACK_TYPE type, void * /*commanddata1*/,
  void * /*commanddata2*/, void * /*userdata*/)
{
  if (type == FMOD_SYSTEM_CALLBACK_RECORDLISTCHANGED)
  {
    debug("[SNDSYS] Record devices list changed");
    if (g_record_list_changed_cb != nullptr)
      g_record_list_changed_cb();
  }
  else if (type == FMOD_SYSTEM_CALLBACK_DEVICELISTCHANGED)
  {
    debug("[SNDSYS] Devices list changed");
    validate_output();
    if (g_output_list_changed_cb != nullptr)
      g_output_list_changed_cb();
    if (g_auto_change_device)
      select_default_device();
  }
  else if (type == FMOD_SYSTEM_CALLBACK_DEVICELOST)
  {
    debug("[SNDSYS] Device lost");
    if (g_device_lost_cb != nullptr)
      g_device_lost_cb();
    if (g_auto_change_device)
      select_default_device();
  }
  else if (type == FMOD_SYSTEM_CALLBACK_MEMORYALLOCATIONFAILED)
  {
    int currentAllocated = 0;
    int maxAllocated = 0;
    FMOD::Memory_GetStats(&currentAllocated, &maxAllocated);
    debug("[SNDSYS] memory allocation failed current %f max %f", currentAllocated, maxAllocated);
    debug_dump_stack();
  }
  return FMOD_OK;
}

void set_system_callbacks(CallbackType ctype)
{
  SNDSYS_IF_NOT_INITED_RETURN;
  ScoopedJavaThreadAttacher scoopedJavaThreadAttacher;
  int flags = 0;
  if (ctype.device_lost)
    flags |= FMOD_SYSTEM_CALLBACK_DEVICELOST;
  if (ctype.device_list_changed)
    flags |= FMOD_SYSTEM_CALLBACK_DEVICELISTCHANGED;
  if (ctype.record_list_changed)
    flags |= FMOD_SYSTEM_CALLBACK_RECORDLISTCHANGED;
  if (ctype.memory_alloc_failed)
    flags |= FMOD_SYSTEM_CALLBACK_MEMORYALLOCATIONFAILED;

  FMOD_RESULT result = g_low_level_system->setCallback(fmod_system_cb, flags);
  G_ASSERTF_RETURN(FMOD_OK == result, , "FMOD::System::setCallback(%d) failed, fmod result is \"%s\"", flags,
    FMOD_ErrorString(result));
}

void set_auto_change_device(bool enable) { g_auto_change_device = enable; }

eastl::vector<DeviceInfo> get_output_devices()
{
  eastl::vector<DeviceInfo> devices;
  SNDSYS_IF_NOT_INITED_RETURN_(devices);
  ScoopedJavaThreadAttacher scoopedJavaThreadAttacher;
  int numDrivers = 0;
  FMOD_RESULT result = g_low_level_system->getNumDrivers(&numDrivers);
  if (FMOD_OK != result)
  {
    debug("[SNDSYS] FMOD error: %s", FMOD_ErrorString(result));
    return devices;
  }
  devices.reserve(numDrivers);

  char deviceName[512]; // max name length is 512 according to documentation
  for (int i = 0; i < numDrivers; ++i)
  {
    int rate;
    result = g_low_level_system->getDriverInfo(i, deviceName, sizeof(deviceName), nullptr, &rate, nullptr, nullptr);
    if (result != FMOD_OK)
    {
      debug("[SNDSYS] failure to retrieve device info %d: %s", i, FMOD_ErrorString(result));
      continue;
    }
    devices.emplace_back(DeviceInfo{i, deviceName, rate});
  }
  return devices;
}

static bool is_loopback_record_device(const char *dev_name)
{
  static const char *loopback_patterns[] = {"loopback", "monitor of"};

  eastl::string devName(dev_name);
  devName.make_lower();
  for (const char *pattern : loopback_patterns)
  {
    if (devName.find(pattern) != eastl::string::npos)
      return true;
  }
  return false;
}

eastl::vector<DeviceInfo> get_record_devices()
{
  eastl::vector<DeviceInfo> devices;
  SNDSYS_IF_NOT_INITED_RETURN_(devices);
  ScoopedJavaThreadAttacher scoopedJavaThreadAttacher;
  int numDrivers = 0;
  FMOD_RESULT result = g_low_level_system->getRecordNumDrivers(&numDrivers, nullptr);
  if (FMOD_OK != result)
  {
    debug("[SNDSYS] FMOD error: %s", FMOD_ErrorString(result));
    return devices;
  }
  devices.reserve(numDrivers);

  int sysDefaultDevIdx = 0;
  char deviceName[512]; // max name length is 512 according to documentation
  for (int i = 0; i < numDrivers; ++i)
  {
    FMOD_DRIVER_STATE state;
    int rate;
    result = g_low_level_system->getRecordDriverInfo(i, deviceName, sizeof(deviceName), nullptr, &rate, nullptr, nullptr, &state);
    if (result != FMOD_OK)
    {
      debug("[SNDSYS] failure to retrieve device info %d: %s", i, FMOD_ErrorString(result));
      continue;
    }
    if (state & FMOD_DRIVER_STATE_CONNECTED && !is_loopback_record_device(deviceName))
    {
      if (state & FMOD_DRIVER_STATE_DEFAULT)
        sysDefaultDevIdx = devices.size();
      devices.emplace_back(DeviceInfo{i, deviceName, rate});
    }
  }
  if (sysDefaultDevIdx != 0)
    eastl::swap(devices[0], devices[sysDefaultDevIdx]);
  return devices;
}

void set_output_device(int device_id)
{
  SNDSYS_IF_NOT_INITED_RETURN;
  ScoopedJavaThreadAttacher scoopedJavaThreadAttacher;
  FMOD_RESULT result = g_low_level_system->setDriver(device_id);
  G_ASSERTF_RETURN(FMOD_OK == result, , "[SNDSYS] FMOD error: %s", FMOD_ErrorString(result));
}

void set_snd_suspend(bool suspend)
{
  SNDSYS_IF_NOT_INITED_RETURN;
  suspend ? fmodapi::get_system()->mixerSuspend() : fmodapi::get_system()->mixerResume();
}

void set_device_changed_async_callbacks(record_list_changed_cb_t record_list_changed_cb,
  output_list_changed_cb_t output_list_changed_cb, device_lost_cb_t device_lost_cb)
{
  SNDSYS_IS_MAIN_THREAD;
  g_record_list_changed_cb = record_list_changed_cb;
  g_output_list_changed_cb = output_list_changed_cb;
  g_device_lost_cb = device_lost_cb;
}

void flush_commands()
{
  SNDSYS_IS_MAIN_THREAD;
  SNDSYS_IF_NOT_INITED_RETURN;
  g_system->flushCommands();
}

int get_max_channels() { return g_max_channels; }

namespace fmodapi
{
FMOD::System *get_system()
{
  G_ASSERT_RETURN(is_inited(), nullptr);
  return g_low_level_system;
}
FMOD::Studio::System *get_studio_system()
{
  G_ASSERT_RETURN(is_inited(), nullptr);
  return g_system;
}
eastl::pair<FMOD::System *, FMOD::Studio::System *> get_systems()
{
  G_ASSERT_RETURN(is_inited(), {});
  return eastl::make_pair(g_low_level_system, g_system);
}
} // namespace fmodapi
} // namespace sndsys
