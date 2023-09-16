#ifndef _DAGOR_GAMELIB_SOUNDSYSTEM_FRAMEMEMSTRING_H_
#define _DAGOR_GAMELIB_SOUNDSYSTEM_FRAMEMEMSTRING_H_
#pragma once

#include <EASTL/string.h>
#include <memory/dag_framemem.h>

namespace sndsys
{
using FrameStr = eastl::basic_string<char, framemem_allocator>;
} // namespace sndsys

#endif
