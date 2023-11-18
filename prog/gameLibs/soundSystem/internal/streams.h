#ifndef _DAGOR_GAMELIB_SOUNDSYSTEM_STREAMSINTERNAL_H_
#define _DAGOR_GAMELIB_SOUNDSYSTEM_STREAMSINTERNAL_H_
#pragma once

#include <EASTL/functional.h>
#include <osApiWrappers/dag_miscApi.h>

class DataBlock;
class Point3;

namespace FMOD
{
class System;
namespace Studio
{
class System;
}
} // namespace FMOD

namespace sndsys
{
namespace streams
{
void init(const DataBlock &blk, float virtual_vol_limit, FMOD::System *system);
void close();
void update(float dt);
void debug_get_info(uint32_t &num_handles, uint32_t &to_release_count);
void debug_enum(eastl::function<void(const char *info, const Point3 &pos, bool is_3d)> f);
} // namespace streams
} // namespace sndsys

#endif
