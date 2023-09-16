#ifndef _DAGOR_GAMELIB_SOUNDSYSTEM_DELAYEDINTERNAL_H_
#define _DAGOR_GAMELIB_SOUNDSYSTEM_DELAYEDINTERNAL_H_
#pragma once

#include <soundSystem/handle.h>

class DataBlock;
class Point3;

namespace sndsys
{
namespace delayed
{
bool is_enable_distant_delay();
bool is_far_enough_for_distant_delay(const Point3 &pos);
bool is_delayed(EventHandle event_handle);
void start(EventHandle event_handle, float add_delay);
void stop(EventHandle event_handle, bool allow_fadeout, bool try_keyoff, float add_delay);
void release(EventHandle event_handle, float add_delay);
void abandon(EventHandle event_handle, float add_delay);
bool is_starting(EventHandle event_handle);
void init(const DataBlock &blk);
void close();
void get_debug_info(size_t &events, size_t &actions, size_t &max_events, size_t &max_actions);
void update(float dt);
} // namespace delayed
} // namespace sndsys

#endif
