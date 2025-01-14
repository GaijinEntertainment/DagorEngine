// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// solution to track GPU execution timeline based on incremental frame hash markers
// done on simple buffer copy and memory barriers,
// to avoid issues with EXTs availability, vendor tooling dependencies,
// and scarce backtrace information in production environment that
// is usually needed to find something usefull
//
// we hash input data of marker and put it on host buffer,
// together with adding copy command on GPU between host and device buffers that is globally barriered inside serial buffer
// on dump request, we just dump both buffer contents to fault report, that should give us info where we crashed on GPU
//
// markers setted up on frame structurial points outside of render passes
// so full frame hash should represent at least overall scene which caused crash
// frame with exact hash can be catched inside debug environment
// based on this info we can trace to faulty block in code and try to find solution for crash

#include "buffer_resource.h"

class FaultReportDump;

namespace drv3d_vulkan
{

struct DeviceExecutionTracker
{
  void init();
  void shutdown();

  void addMarker(const void *data, size_t data_sz);
  void restart(size_t job_id);
  void dumpFaultData(FaultReportDump &dump) const;
  void verify();

  uint64_t getPrevJobHash() const { return interlocked_acquire_load(prevJobHash); }

private:
  static const int MARKER_ALLOC_SZ = 4096;

  struct Marker
  {
    uint64_t jobId;
    uint64_t hash;
#if DAGOR_DBGLEVEL > 0
    uint64_t caller;
#endif
  };

  Buffer *hostMarkers = nullptr;
  Buffer *deviceMarkers = nullptr;
  bool allocatedBuffers = false;

  Marker currentMarker = {};
  volatile uint64_t prevJobHash = 0;
  size_t markerCount = 0;
  size_t maxMarkerCount = 0;
};

} // namespace drv3d_vulkan
