// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_sharedMem.h>
#include <landMesh/landRayTracer.h>
#include <heightmap/heightmapPhysHandler.h>
#include <fftWater/fftWater.h>
#include <ioSys/dag_dataBlock.h>

extern "C" const char *dedicated_server_dll_fn;

static GlobalSharedMemStorage *shared_mem = nullptr;
static GlobalSharedMemStorage shared_mem_storage;

void hosted_internal_server_init_shared_memory()
{
  if (!dedicated_server_dll_fn || ::shared_mem)
    return;

  constexpr int maxRecords = 16;
  if (shared_mem_storage.initLocal(maxRecords))
  {
    ::shared_mem = &shared_mem_storage;
    LandRayTracer::sharedMem = ::shared_mem;
    HeightmapPhysHandler::sharedMem = ::shared_mem;
    HeightmapPhysHandler::dumpSharingReadOnly = true;
    fft_water::WaterHeightmap::sharedMem = ::shared_mem;
    debug("inited local shared memory (%d records)", maxRecords);
  }
}

void hosted_internal_server_pass_shared_memory(DataBlock &startParams)
{
  startParams.setInt64("sharedMemPtr", (intptr_t)(void *)::shared_mem);
  startParams.setBool("shareHmapRO", HeightmapPhysHandler::dumpSharingReadOnly);
}

void hosted_internal_server_term_shared_memory()
{
  if (!::shared_mem)
    return;
  ::shared_mem = nullptr;
  shared_mem_storage.term();
  debug("terminated local shared memory");
}
