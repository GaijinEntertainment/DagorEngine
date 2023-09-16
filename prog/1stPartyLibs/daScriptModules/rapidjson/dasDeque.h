#ifndef _GAIJIN_DASCRIPT_MODULES_RAPIDJSON_DASDEQUE_H_
#define _GAIJIN_DASCRIPT_MODULES_RAPIDJSON_DASDEQUE_H_
#pragma once

#include <daScript/das_config.h>


#if DAS_USE_EASTL

#include <EASTL/deque.h>

namespace das
{
  template <typename T>
  using das_deque = eastl::deque<T>;
}

#else

#include <deque>

namespace das
{
  template <typename T>
  using das_deque = std::deque<T>;
}

#endif // DAS_USE_EASTL


#endif // _GAIJIN_DASCRIPT_MODULES_RAPIDJSON_DASDEQUE_H_
