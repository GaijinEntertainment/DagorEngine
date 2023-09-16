#pragma once

#include "de3_ICamera.h"
#include <EASTL/string.h>

namespace samplebenchmark
{
static const eastl::string cameraFileName{"benchmarkCameras.blk"}; // camera settings file
static const eastl::string cmdArgumentName{"benchmark"};           // benchmark mode enabling command line argument

extern bool isBenchmark;             // is in benchmark mode (-benchmark argument)
extern int sampleBenchmarkQuitAfter; // quit after (ms)
extern void (*quitCallback)();

void quitIfBenchmarkHasEnded();
void setupBenchmark(IGameCamera *cam, void (*quitCallback)());
void setupBenchmark(void (*quitCallback)());

} // namespace samplebenchmark
