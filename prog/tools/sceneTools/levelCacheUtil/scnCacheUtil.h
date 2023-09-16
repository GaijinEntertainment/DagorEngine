#pragma once

#include <util/dag_oaHashNameMap.h>


/// Generic progress indicator interface
class IGenProgressInd
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

  /// closes and destroys progressbar
  virtual void destroy() = 0;

#define DSA_OVERLOADS_PARAM_DECL
#define DSA_OVERLOADS_PARAM_PASS
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void setActionDesc, setActionDescFmt);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS
};

/// return true is update was successful (event when nothing updated); return false on error
bool update_level_caches(const char *cache_dir, const FastNameMapEx &level_dirs, bool clean, IGenProgressInd *pbar, bool quiet);

/// returns true when all files are consistent; return false when some files were inconsistent and been removed
bool verify_level_caches_files(const char *cache_dir, IGenProgressInd *pbar, bool quiet);
