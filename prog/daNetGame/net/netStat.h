// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>
#include <string.h>
#include <generic/dag_span.h>

#include "netStatCounters.inl"

namespace netstat
{
typedef uint32_t cnt_t; // 32 bits should be enough in order to hold 1 second of counters data (without overflowing)

enum CounterType
{
#define _NS(V, ...) CT_##V,
  DEF_NS_COUNTERS
#undef _NS
    NUM_COUNTER_TYPES
};

enum AggregationType
{
  AG_SUM,
  AG_AVG,
  AG_MIN,
  AG_MAX,
  AG_CNT,
  AG_VAR,
  AG_LAST,

  NUM_AG_TYPES
};

union Sample
{
  struct
  {
#define _NS(_1, var, ...) cnt_t var;
    DEF_NS_COUNTERS
#undef _NS
  };
  cnt_t counters[NUM_COUNTER_TYPES];
  Sample() { memset(this, 0, sizeof(*this)); }
};

void init();
void term();
void update(uint32_t ct_ms);
void inc(CounterType type, cnt_t val = 1);
void set(CounterType type, cnt_t val);
void max(CounterType type, cnt_t val);
dag::ConstSpan<Sample> get_aggregations();

void toggle_render();
void render();

}; // namespace netstat
