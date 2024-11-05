// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de3_benchmark.h"
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_cpuFreq.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_direct.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_Point3.h>
#include <debug/dag_fatal.h>

namespace samplebenchmark
{

bool isBenchmark{false};            // is in benchmark mode (-benchmark argument)
int sampleBenchmarkQuitAfter{1000}; // quit after (ms)
void (*quitCallback)(){nullptr};

void quitIfBenchmarkHasEnded()
{
  if (isBenchmark && get_time_msec() > sampleBenchmarkQuitAfter)
  {
    if (quitCallback != nullptr)
      quitCallback();
  }
}

void setupBenchmark(IGameCamera *cam, void (*quitCallback)())
{
  isBenchmark = dgs_get_argv(cmdArgumentName.c_str());
  if (isBenchmark)
  {
    samplebenchmark::quitCallback = quitCallback;
    DataBlock cameraBlock;

    if (dd_file_exist(cameraFileName.c_str()))
      cameraBlock.load(cameraFileName.c_str());
    else
      DAG_FATAL((eastl::string("Benchmark mode: Can't locate ") + cameraFileName).c_str());

    const DataBlock *IDblock = cameraBlock.getBlockByNameEx("selected_camera");
    const unsigned int id = IDblock->getInt("id", 1);

    const DataBlock *otherBlock = cameraBlock.getBlockByNameEx("other");
    sampleBenchmarkQuitAfter = otherBlock->getInt("quit_after_ms", 1000);

    const DataBlock *viewTrBlock = cameraBlock.getBlockByNameEx((eastl::string("camera") + eastl::to_string(id)).c_str());

    TMatrix viewTr;
    viewTr.setcol(0, viewTrBlock->getPoint3("tm0", Point3(1.f, 0.f, 0.f)));
    viewTr.setcol(1, viewTrBlock->getPoint3("tm1", Point3(0.f, 1.f, 0.f)));
    viewTr.setcol(2, viewTrBlock->getPoint3("tm2", Point3(0.f, 0.f, 1.f)));
    viewTr.setcol(3, viewTrBlock->getPoint3("tm3", ZERO<Point3>()));

    if (cam != nullptr)
      cam->setInvViewMatrix(viewTr);
  }
}

void setupBenchmark(void (*quitCallback)()) { setupBenchmark(nullptr, quitCallback); }

} // namespace samplebenchmark