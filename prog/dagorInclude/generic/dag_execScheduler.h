//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <supp/dag_define_KRNLIMP.h>

class ExecutionScheduler
{
public:
  virtual bool canProceed(const char *src_fname, int src_line) = 0;
  virtual void wait(int msec) = 0;

  virtual void switchToMain() {}

  virtual void setAllowedTimeFactor(float) {}
};

class SyncExecutionScheduler : public ExecutionScheduler
{
public:
  KRNLIMP virtual bool canProceed(const char *src_fname, int src_line);
  KRNLIMP virtual void wait(int msec);
};

#if DAGOR_DBGLEVEL > 0
static inline bool es_cond_switch_can_proceed(ExecutionScheduler *es, const char *fn, int ln)
{
  return es ? es->canProceed(fn, ln) : true;
}
static inline bool es_cond_switch_can_proceed_ref(ExecutionScheduler &es, const char *fn, int ln) { return es.canProceed(fn, ln); }
#else
static inline bool es_cond_switch_can_proceed(ExecutionScheduler *es, const char *, int)
{
  return es ? es->canProceed(NULL, 0) : true;
}
static inline bool es_cond_switch_can_proceed_ref(ExecutionScheduler &es, const char *, int) { return es.canProceed(NULL, 0); }
#endif

#define ES_SWITCH_CAN_PROCEED(S_PTR) es_cond_switch_can_proceed(S_PTR, __FILE__, __LINE__)
#define ES_SWITCH_CAN_PROCEED_REF(S) es_cond_switch_can_proceed_ref(S, __FILE__, __LINE__)

#include <supp/dag_undef_KRNLIMP.h>
