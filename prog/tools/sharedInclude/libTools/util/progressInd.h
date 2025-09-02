//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_safeArg.h>
#include <util/dag_globDef.h>

/// Progress callback
class IProgressCB
{
public:
  /// calls on user press "Cancel" button
  virtual void onCancel() = 0;
};


/// Simple progress callback implementation
class SimpleProgressCB : public IProgressCB
{
public:
  /// indicate a need to stop current process
  bool mustStop;

  SimpleProgressCB() : mustStop(false) {}

  void onCancel() override { mustStop = true; }
};


/// Generic progress indicator interface
class IGenericProgressIndicator
{
public:
  /// sets description of current action, uses printf-like formatted string (with dagor extensions)
  virtual void setActionDescFmt(const char *desc, const DagorSafeArg *arg, int anum) = 0;
  /// sets total count of steps
  virtual void setTotal(int total_cnt) = 0;
  /// sets count of performed steps
  virtual void setDone(int done_cnt) = 0;
  /// increments count of performed steps
  virtual void incDone(int inc = 1) = 0;

  /// forces screen redraw (even though progress indicator didn't change)
  virtual void redrawScreen() = 0;

  /// closes and destroys progressbar
  virtual void destroy() = 0;

  /// causes higher-level progressbar implementation to show indicator
  virtual void startProgress(IProgressCB *progress_cb = NULL) = 0;
  /// causes higher-level progressbar implementation to hide indicator
  virtual void endProgress() = 0;

#define DSA_OVERLOADS_PARAM_DECL
#define DSA_OVERLOADS_PARAM_PASS
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void setActionDesc, setActionDescFmt);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS
};


/// stub implementation of progress indicator interface
/// (can be used when progress indicator required but is not useful)
class QuietProgressIndicator : public IGenericProgressIndicator
{
public:
  void setActionDescFmt(const char *desc, const DagorSafeArg *arg, int anum) override
  {
    G_UNUSED(desc);
    G_UNUSED(arg);
    G_UNUSED(anum);
  }
  void setTotal(int total_cnt) override { G_UNUSED(total_cnt); }
  void setDone(int done_cnt) override { G_UNUSED(done_cnt); }
  void incDone(int inc = 1) override { G_UNUSED(inc); }
  void redrawScreen() override {}
  void destroy() override {}
  void startProgress(IProgressCB *progress_cb = NULL) override { G_UNUSED(progress_cb); }
  void endProgress() override {}
};


class ConsoleProgressIndicator : public IGenericProgressIndicator
{
  int total = 1, done = 0;
  int lastPct = -1;
  bool showPct;

public:
  ConsoleProgressIndicator(bool show_pct) { showPct = show_pct; }
  ~ConsoleProgressIndicator();

  void setActionDescFmt(const char *fmt, const DagorSafeArg *arg, int anum) override;
  void setTotal(int total_cnt) override
  {
    total = total_cnt < 1 ? 1 : total_cnt;
    updatePct();
  }
  void setDone(int done_cnt) override
  {
    done = done_cnt;
    updatePct();
  }
  void incDone(int inc = 1) override
  {
    done += inc;
    updatePct();
  }
  void redrawScreen() override
  {
    lastPct = -1;
    updatePct();
  }
  void destroy() override { delete this; }
  void startProgress(IProgressCB *) override {}
  void endProgress() override {}

protected:
  virtual void outputPct(int pct);
  void updatePct()
  {
    int pct = done * 100 / total;
    if (pct != lastPct)
    {
      if (showPct)
        outputPct(pct);
      lastPct = pct;
    }
  }
};

/// creates console progress indicator (useful for console applications)
IGenericProgressIndicator *create_con_progressbar(bool show_pct = true);
