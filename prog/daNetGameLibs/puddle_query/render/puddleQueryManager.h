// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

#include "3d/dag_sbufferIDHolder.h"
#include "gpuReadbackQuery/gpuReadbackQuerySystem.h"

class Point3;
class Point4;
class ComputeShaderElement;
class RingCPUBufferLock;

class PuddleQueryManager
{
public:
  PuddleQueryManager();
  ~PuddleQueryManager();

  PuddleQueryManager(const PuddleQueryManager &) = delete;
  PuddleQueryManager(PuddleQueryManager &&other) = delete;

  PuddleQueryManager &operator=(const PuddleQueryManager &) = delete;
  PuddleQueryManager &operator=(PuddleQueryManager &&other) = delete;

  void update();
  void beforeDeviceReset();
  void afterDeviceReset();

  int queryPoint(const Point3 &point);
  GpuReadbackResultState getQueryValue(int query_id, float &value);

private:
  eastl::unique_ptr<GpuReadbackQuerySystem<Point4, float>> grqSystem;
};

PuddleQueryManager *get_puddle_query_mgr();
void init_puddle_query_mgr();
void close_puddle_query_mgr();