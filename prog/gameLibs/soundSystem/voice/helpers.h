// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "rateHelpers.h"
#include <fmod.hpp>
#include <fmod_errors.h>
#include <math/dag_mathBase.h>
#include <EASTL/unique_ptr.h>

#define CHECK_FMOD(res, message) \
  if (res != FMOD_OK)            \
    LOGWARN_CTX("[VC] %s %s: %s", __FUNCTION__, message, FMOD_ErrorString(res));

#define CHECK_FMOD_RETURN(res, message)                                           \
  if (res != FMOD_OK)                                                             \
  {                                                                               \
    LOGWARN_CTX("[VC] %s: %s: %s", __FUNCTION__, message, FMOD_ErrorString(res)); \
    return;                                                                       \
  }

#define CHECK_FMOD_RETURN_VAL(res, message, ret)                                  \
  if (res != FMOD_OK)                                                             \
  {                                                                               \
    LOGWARN_CTX("[VC] %s: %s: %s", __FUNCTION__, message, FMOD_ErrorString(res)); \
    return ret;                                                                   \
  }

namespace voicechat
{

inline int get_bits_from_format(FMOD_SOUND_FORMAT format)
{
  switch (format)
  {
    case FMOD_SOUND_FORMAT_PCM8: return 8;
    case FMOD_SOUND_FORMAT_PCM16: return 16;
    case FMOD_SOUND_FORMAT_PCM24: return 24;
    case FMOD_SOUND_FORMAT_PCM32: return 32; //-V1037
    case FMOD_SOUND_FORMAT_PCMFLOAT: return 32;
    default: break;
  }
  return 8;
}

inline int get_bytes_from_format(FMOD_SOUND_FORMAT format) { return get_bits_from_format(format) / 8; }


struct ReleaseDeleter
{
  template <typename T>
  void operator()(T *ptr)
  {
    if (ptr)
      ptr->release();
  }
};

template <typename T>
using fmod_ptr = eastl::unique_ptr<T, ReleaseDeleter>;

} // namespace voicechat
