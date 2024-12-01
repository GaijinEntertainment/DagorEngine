// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unique_ptr.h>


namespace darg
{

class Profiler;

enum ProfilerMetricId
{
  M_RENDER_TOTAL = 0,
  M_BEFORE_RENDER_TOTAL,
  M_RENDER_TEXT,
  M_RENDER_9RECT,
  M_RENDER_TEXTAREA,
  M_RECALC_TEXTAREA,
  M_UPDATE_TOTAL,
  M_COMPONENT_SCRIPT,
  M_RENDER_LIST_REBUILD,
  M_RENDER_LIST_RENDER,
  M_BHV_UPDATE,
  M_BHV_BEFORE_RENDER,
  M_ETREE_BEFORE_RENDER,
  M_RECALC_LAYOUT,
  M_DBG,
  NUM_PROFILER_METRICS
};

extern const char *profiler_metric_names[NUM_PROFILER_METRICS];


struct Metric
{
public:
  static constexpr int HISTORY_SIZE = 400;

  Metric();

  void add(float val);
  void apply();
  void push(float val);

  float avg();
  float stddev();

  void recalcMinMax();

public:
  float history[HISTORY_SIZE];
  int firstPos;
  int count;
  float sum, sqSum;
  float minVal, maxVal;

  float curAccum;
  int nCur;
};


class AutoProfileScope
{
public:
  AutoProfileScope(Profiler *pfl, ProfilerMetricId metric);
  AutoProfileScope(const eastl::unique_ptr<Profiler> &pfl, ProfilerMetricId metric);
  ~AutoProfileScope();

private:
  Profiler *pfl;
  ProfilerMetricId metric;
  int64_t refTime;
};


class Profiler
{
public:
  bool getStats(ProfilerMetricId m, float &avg, float &stddev, float &min_val, float &max_val);

  void afterRender();
  void afterUpdate();

public:
  Metric metrics[NUM_PROFILER_METRICS];
};


} // namespace darg
