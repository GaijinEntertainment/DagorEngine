// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dag_assert.h>
#include <osApiWrappers/dag_miscApi.h>
#include <fmod_errors.h>

#define SNDSYS_IS_MAIN_THREAD G_ASSERT(is_main_thread())

#define SNDSYS_IF_NOT_INITED_RETURN_(RET) \
  if (!::sndsys::is_inited())             \
    return RET;

#define SNDSYS_IF_NOT_INITED_RETURN \
  if (!::sndsys::is_inited())       \
    return;

#define SOUND_VERIFY(fmodOperation)                                                                                 \
  {                                                                                                                 \
    FMOD_RESULT fmodResult;                                                                                         \
    G_VERIFYF(FMOD_OK == (fmodResult = (fmodOperation)), "[SNDSYS] FMOD error '%s'", FMOD_ErrorString(fmodResult)); \
  }

#define SOUND_VERIFY_AND_DO(fmodOperation, cmd)                                                                     \
  {                                                                                                                 \
    FMOD_RESULT fmodResult;                                                                                         \
    G_VERIFYF(FMOD_OK == (fmodResult = (fmodOperation)), "[SNDSYS] FMOD error '%s'", FMOD_ErrorString(fmodResult)); \
    if (FMOD_OK != fmodResult)                                                                                      \
      cmd;                                                                                                          \
  }

#define SOUND_VERIFY_AND_RETURN(fmodOperation) SOUND_VERIFY_AND_DO(fmodOperation, return fmodResult)

#define SOUND_VERIFY_AND_RETURN_(fmodOperation, RET) SOUND_VERIFY_AND_DO(fmodOperation, return RET)

class DataBlock;

namespace sndsys
{
void debug_init(const DataBlock &blk);
} // namespace sndsys
