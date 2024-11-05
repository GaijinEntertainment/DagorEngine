//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_safeArg.h>

class IPageSysHelper
{
public:
#define DSA_OVERLOADS_PARAM_DECL int flags, const char *caption,
#define DSA_OVERLOADS_PARAM_PASS flags, caption,
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void postMsg, postMsgV);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

  virtual void postMsgV(bool error, const char *caption, const char *fmt, const DagorSafeArg *arg, int anum) = 0;
  virtual const char *getDagorCdkDir() = 0;
  virtual const char *getSgCppDestDir() = 0;
};

// derived class implementing IPageSysHelper interface must be installed to this helper
// before using any page classes
extern IPageSysHelper *page_sys_hlp;
