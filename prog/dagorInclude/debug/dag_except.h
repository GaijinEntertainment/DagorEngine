//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#if DAGOR_EXCEPTIONS_ENABLED
#include <osApiWrappers/dag_stackHlp.h>

//! base dagor exception class
//! it has code, textual description and stack
class DagorException
{
public:
  int excCode;
  const char *excDesc;

  __forceinline DagorException(int code, const char *desc)
  {
    excCode = code;
    excDesc = desc;
    ::stackhlp_fill_stack(excStack, 32, 0);
  }
  __forceinline DagorException(int code, const char *desc, void *ctx_ptr)
  {
    excCode = code;
    excDesc = desc;
    ::stackhlp_fill_stack_exact(excStack, 32, ctx_ptr);
  }
  virtual ~DagorException() {}


  void **getStackPtr() { return excStack; }

protected:
  void *excStack[32];
};

#define DAGOR_TRY              try
#define DAGOR_THROW(x)         throw x
#define DAGOR_RETHROW()        throw
#define DAGOR_CATCH(x)         catch (x)
#define DAGOR_EXC_STACK_STR(e) ::stackhlp_get_call_stack_str(e.getStackPtr(), 32)

#else
#include <debug/dag_fatal.h>

class DagorException
{
public:
  int excCode;
  const char *excDesc;

  __forceinline DagorException(int code, const char *desc)
  {
    excCode = code;
    excDesc = desc;
  }

  void **getStackPtr() { return nullptr; }
};

#define DAGOR_TRY      if (1)
#define DAGOR_THROW(x) fatal("exception: " #x)
#define DAGOR_RETHROW()
//-V:DAGOR_CATCH:646
#define DAGOR_CATCH(x)         if (0)
#define DAGOR_EXC_STACK_STR(e) "n/a"

#endif
