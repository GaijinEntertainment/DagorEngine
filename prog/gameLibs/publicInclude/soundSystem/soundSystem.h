//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <soundSystem/events.h>

#include <EASTL/string.h>
#include <EASTL/vector.h>

class DataBlock;
class TMatrix;

class ScoopedJavaThreadAttacher
{
  bool isAttach = false;

public:
#if _TARGET_ANDROID
  ScoopedJavaThreadAttacher();
  ~ScoopedJavaThreadAttacher();
#else
  ScoopedJavaThreadAttacher() {}
  ~ScoopedJavaThreadAttacher() {}
#endif
};

namespace sndsys
{
struct FmodCpuUsage
{
  float dspusage;
  float streamusage;
  float geometryusage;
  float updateusage;
  float studiousage;
};
struct FmodMemoryUsage
{
  int inclusive;
  int exclusive;
  int sampledata;
  int currentalloced, maxalloced;
};

struct DeviceInfo
{
  int deviceId;
  eastl::string name;
  int rate;
};

bool init(const DataBlock &blk);
void shutdown();
bool is_inited();

void update_listener(float dt, const TMatrix &listener_tm);
void set_time_speed(float time_speed);
void update(float dt);
void lazy_update();

void override_time_speed(float time_speed); // should be > 0 to override value provided within update(float dt, float time_speed = 1.f)

Point3 get_3d_listener_pos();
TMatrix get_3d_listener();
Point3 get_3d_listener_vel();

void reset_3d_listener();

void set_volume(const char *vca_name, float volume);
float get_volume(const char *vca_name);

void set_volume_bus(const char *bus_name, float volume);
float get_volume_bus(const char *bus_name);

void set_mute(const char *bus_name, bool mute);
bool get_mute(const char *bus_name);

bool is_bus_exists(const char *bus_name);

void lock_channel_group(const char *bus_name, bool immediately);

void set_pitch(const char *bus_name, float pitch);
void set_pitch(float pitch);

void get_memory_statistics(unsigned &system_allocated, unsigned &current_allocated, unsigned &max_allocated);
void get_cpu_usage(FmodCpuUsage &fcpu);
void get_mem_usage(FmodMemoryUsage &fmem);
void block_programmer_sounds(bool do_mute);
int get_programmer_sounds_generation();
void increment_programmer_sounds_generation();
void set_snd_suspend(bool suspend);

struct CallbackType
{
  bool device_lost = false;
  bool device_list_changed = false;
  bool record_list_changed = false;
  bool memory_alloc_failed = false;
};

void set_system_callbacks(CallbackType cype);
void set_auto_change_device(bool enable);

eastl::vector<DeviceInfo> get_output_devices();
eastl::vector<DeviceInfo> get_record_devices();

void set_output_device(int device_id);

typedef void (*record_list_changed_cb_t)();
typedef void (*output_list_changed_cb_t)();
typedef void (*device_lost_cb_t)();

void set_device_changed_async_callbacks(record_list_changed_cb_t record_list_changed_cb,
  output_list_changed_cb_t output_list_changed_cb, device_lost_cb_t device_lost_cb);

void flush_commands();

} // namespace sndsys
