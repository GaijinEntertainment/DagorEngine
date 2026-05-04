// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if _TARGET_PC_LINUX && !DAGOR_HOSTED_INTERNAL_SERVER
#include <osApiWrappers/dag_sharedMem.h>
#include <landMesh/landRayTracer.h>
#include <heightmap/heightmapPhysHandler.h>
#include <fftWater/fftWater.h>
#include <daECS/core/entityManager.h>
#include "main/gameProjConfig.h"
#include "main/version.h"
#include "game/gameEvents.h"

void init_shared_memory()
{
  static GlobalSharedMemStorage shared_mem_stor;
  const DataBlock *shmemBlk = ::dgs_get_settings()->getBlockByNameEx("dedicated")->getBlockByNameEx("shmem");
  int buildNumber = get_build_number();
  int sizeMb = shmemBlk->getInt("sizeMb", buildNumber > 0 ? 512 : 0);
  if (sizeMb <= 0)
  {
    debug("shared memory disabled");
    return;
  }
  int maxRecords = shmemBlk->getInt("maxRecords", 64);
  char nameBuf[64];
#if _TARGET_64BIT
  static constexpr const char *mbx32 = "";
#else
  static constexpr const char *mbx32 = ".x32";
#endif
  if (get_exe_version32())
    SNPRINTF(nameBuf, sizeof(nameBuf), "%s_%s%s.shared", gameproj::game_codename(), get_exe_version_str(), mbx32);
  else
    SNPRINTF(nameBuf, sizeof(nameBuf), "%s_test_build_%d%s.shared", gameproj::game_codename(), buildNumber, mbx32);

  if (GlobalSharedMemStorage::Data *shdata = shared_mem_stor.init(nameBuf, sizeMb << 20, maxRecords))
  {
    LandRayTracer::sharedMem = &shared_mem_stor;
    HeightmapPhysHandler::sharedMem = &shared_mem_stor;
    fft_water::WaterHeightmap::sharedMem = &shared_mem_stor;
    g_entity_mgr->broadcastEventImmediate(EventOnSharedMemInited(&shared_mem_stor));
    debug("enabled shared memory '%s' of size %dMB and %d records", nameBuf, sizeMb, maxRecords);
  }
  else
    logwarn("failed to init shared memory '%s'", nameBuf);
}

#else
void init_shared_memory() {}
#endif
