// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_sharedMem.h>
#include <osApiWrappers/dag_vromfs.h>
#include <heightmap/heightmapPhysHandler.h>
#include <fftWater/fftWater.h>
#include <landMesh/landRayTracerSoA4.h>
#include <ioSys/dag_dataBlock.h>

extern "C" const char *dedicated_server_dll_fn;

static GlobalSharedMemStorage *shared_mem = nullptr;
static GlobalSharedMemStorage shared_mem_storage;

void init_shared_memory()
{
  if (!dedicated_server_dll_fn || ::shared_mem)
    return;

  constexpr int maxRecords = 16;
  if (shared_mem_storage.initLocal(maxRecords))
  {
    ::shared_mem = &shared_mem_storage;
    HeightmapPhysHandler::sharedMem = ::shared_mem;
    HeightmapPhysHandler::dumpSharingReadOnly = true;
    LandRayTracerSoA4::sharedMem = ::shared_mem;
    fft_water::WaterHeightmap::sharedMem = ::shared_mem;
    set_vromfs_shared_mem_storage(::shared_mem);
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
