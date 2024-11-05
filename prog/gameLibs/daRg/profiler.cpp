// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "profiler.h"

#include <perfMon/dag_cpuFreq.h>
#include <util/dag_globDef.h>


namespace darg
{

const char *profiler_metric_names[NUM_PROFILER_METRICS] = {
  "Render total",
  "Before render total",
  "Render text",
  "Render 9rect",
  "Update total",
  "Component script",
  "RList rebuild",
  "RList render",
  "Bhv update",
  "Bhv br",
  "ETree br",
  "Recalc layout *",
  "<dbg>",
};


static bool is_metric_cumulative(ProfilerMetricId m)
{
  return m == M_RENDER_TEXT || m == M_RENDER_9RECT || m == M_COMPONENT_SCRIPT || m == M_RECALC_LAYOUT || m == M_ETREE_BEFORE_RENDER ||
         m == M_RENDER_LIST_REBUILD;
}


Metric::Metric()
{
  G_ASSERT(profiler_metric_names[NUM_PROFILER_METRICS - 1]);

  memset(history, 0, sizeof(history));
  count = 0;
  firstPos = 0;
  sum = sqSum = 0;
  minVal = FLT_MAX;
  maxVal = -FLT_MAX;

  curAccum = 0;
  nCur = 0;
}


void Metric::add(float val)
{
  curAccum += val;
  ++nCur;
}


void Metric::apply()
{
  if (nCur)
  {
    push(curAccum);
    nCur = 0;
    curAccum = 0;
  }
}


void Metric::push(float val)
{
  G_ASSERT(count <= HISTORY_SIZE);
  if (count >= HISTORY_SIZE)
  {
    float val0 = history[firstPos];
    history[firstPos] = val;
    firstPos = (firstPos + 1) % HISTORY_SIZE;

    sqSum -= val0 * val0;
    sum -= val0;

    if (val0 == minVal || val0 == maxVal)
      recalcMinMax();
    else
    {
      minVal = min(minVal, val);
      maxVal = max(maxVal, val);
    }
  }
  else
  {
    history[count] = val;
    ++count;
    minVal = min(minVal, val);
    maxVal = max(maxVal, val);
  }

  sum += val;
  sqSum += val * val;
}

void Metric::recalcMinMax()
{
  minVal = FLT_MAX;
  maxVal = -FLT_MAX;
  for (int i = 0; i < count; ++i)
  {
    minVal = min(minVal, history[i]);
    maxVal = max(maxVal, history[i]);
  }
}


float Metric::avg() { return count ? (sum / count) : 0; }


float Metric::stddev()
{
  if (!count)
    return 0;

  float d = sqSum / count - (sum * sum) / float(count * count);
  return sqrtf(::max(0.0f, d));
}


AutoProfileScope::AutoProfileScope(Profiler *pfl_, ProfilerMetricId metric_) : pfl(pfl_), metric(metric_)
{
  refTime = ref_time_ticks();
}


AutoProfileScope::AutoProfileScope(const eastl::unique_ptr<Profiler> &pfl_, ProfilerMetricId metric_) :
  pfl(pfl_.get()), metric(metric_)
{
  refTime = ref_time_ticks();
}


AutoProfileScope::~AutoProfileScope()
{
  if (!pfl)
    return;

  float dtMsec = get_time_usec(refTime) * 1e-3f;
  if (is_metric_cumulative(metric))
    pfl->metrics[metric].add(dtMsec);
  else
    pfl->metrics[metric].push(dtMsec);
}


bool Profiler::getStats(ProfilerMetricId m_id, float &avg, float &stddev, float &min_val, float &max_val)
{
  Metric &m = metrics[m_id];
  avg = m.avg();
  stddev = m.stddev();
  min_val = m.minVal;
  max_val = m.maxVal;
  return m.count != 0;
}


void Profiler::afterRender()
{
  metrics[M_RENDER_TEXT].apply();
  metrics[M_RENDER_9RECT].apply();
  metrics[M_RECALC_LAYOUT].apply();
  metrics[M_ETREE_BEFORE_RENDER].apply();
  metrics[M_RENDER_LIST_REBUILD].apply();
}


void Profiler::afterUpdate()
{
  metrics[M_COMPONENT_SCRIPT].apply();
  metrics[M_RECALC_LAYOUT].apply();
}


} // namespace darg
