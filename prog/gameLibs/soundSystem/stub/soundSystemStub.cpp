#include <math/dag_TMatrix.h>
#include <soundSystem/soundSystem.h>
#include <soundSystem/fmodApi.h>
#include <soundSystem/banks.h>
#include <soundSystem/debug.h>
#include <soundSystem/delayed.h>
#include <soundSystem/dsp.h>
#include <soundSystem/events.h>
#include <soundSystem/vars.h>
#include <soundSystem/visualLabels.h>
#include <soundSystem/streams.h>
#include <soundSystem/eventInstanceStealing.h>

class DataBlock;

namespace sndsys
{
void init(const DataBlock &) {}
void shutdown() {}
bool is_inited() { return false; }
void get_memory_statistics(unsigned & /*system_allocated*/, unsigned & /*current_allocated*/, unsigned & /*max_allocated*/) {}

void set_output_device(int) {}
eastl::vector<DeviceInfo> get_output_devices() { return {}; }
eastl::vector<DeviceInfo> get_record_devices() { return {}; }

void flush_commands() {}

namespace fmodapi
{
FMOD::System *get_system() { return nullptr; }
FMOD::Studio::System *get_studio_system() { return nullptr; }
FMOD::Studio::EventInstance *get_instance(EventHandle) { return nullptr; }
FMOD::Studio::EventDescription *get_description(EventHandle) { return nullptr; }
FMOD::Studio::EventDescription *get_description(const char *, const char *) { return nullptr; }
} // namespace fmodapi

namespace banks
{
void init(const DataBlock &) {}
void enable(const char *, bool, const PathTags &) {}
void enable_starting_with(const char *, bool, const PathTags &) {}

const char *get_default_preset(const DataBlock &) { return ""; }
bool is_enabled(const char *preset_name) { return false; }
bool is_loaded(const char *preset_name) { return false; }
bool is_exist(const char *preset_name) { return false; }
bool is_preset_has_failed_banks(const char *preset_name) { return false; }
void get_failed_banks_names(eastl::vector<eastl::string, framemem_allocator> &) {}
void get_loaded_banks_names(eastl::vector<eastl::string, framemem_allocator> &) {}
void prohibit_bank_events(const eastl::string &) {}
bool is_guid_prohibited(const FMODGUID &) { return false; }
void clear_prohibited_guids() {}

void set_preset_loaded_cb(PresetLoadedCallback) {}
bool any_banks_pending() { return false; }

void set_err_cb(ErrorCallback cb) {}
void unload_banks_sample_data(void) {}
} // namespace banks

namespace delayed
{
void enable_distant_delay(bool) {}
void release_delayed_events() {}
} // namespace delayed

namespace dsp
{
void init(const DataBlock &) {}
void close() {}
void apply() {}
} // namespace dsp

// events.cpp
bool has_event(const char *, const char *) { return false; }
int get_num_event_instances(const char *, const char *) { return 0; }
EventHandle init_event(const char *, const char *, ieff_t, const Point3 *) { return {}; }
EventHandle init_event(const FMODGUID &) { return {}; }
bool has_event(const FMODGUID &) { return false; }
bool play_one_shot(const char *, const char *, const Point3 *, ieff_t, float) { return false; }
void release_immediate(EventHandle &, bool) {}
void release_delayed(EventHandle &, float) {}
void release(EventHandle &, float, bool) {}
void abandon_immediate(EventHandle &) {}
void abandon_delayed(EventHandle &, float) {}
void abandon(EventHandle &, float) {}
void release_all_instances(const char *) {}
bool is_playing(EventHandle) { return false; }
bool get_playback_state(EventHandle, PlayBackState &) { return false; }
bool get_property(const FMODGUID &, int, int &) { return false; }
bool is_valid_handle(EventHandle) { return false; }
bool is_valid_event_instance(EventHandle) { return false; }
bool is_one_shot(EventHandle) { return false; }
bool is_3d(EventHandle) { return false; }
bool is_virtual(EventHandle) { return false; }
const char *debug_event_state(EventHandle) { return ""; }
void start_immediate(EventHandle) {}
void start_delayed(EventHandle, float) {}
void start(EventHandle, float) {}
void start(EventHandle, const Point3 &, float) {}
void stop_immediate(EventHandle, bool, bool) {}
void stop_delayed(EventHandle, bool, bool, float) {}
void stop(EventHandle, bool, bool, float) {}
bool keyoff(EventHandle) { return false; }
void pause(EventHandle, bool /*pause = true*/) {}
bool get_paused(EventHandle) { return false; }
void set_volume(EventHandle, float /* volume*/) {}
void set_timeline_position(EventHandle, int) {}
int get_timeline_position(EventHandle) { return -1; }
void set_pitch(EventHandle, float) {}
void set_3d_attr(EventHandle, const Point3 &) {}
void set_3d_attr(EventHandle, const TMatrix4 &, const Point3 &) {}
void set_3d_attr(EventHandle, const TMatrix &, const Point3 &) {}
bool should_play(const Point3 &, float) { return false; }
bool get_var_desc(EventHandle, const char *, VarDesc &) { return false; }
bool get_var_desc(const char *, const char *, VarDesc &) { return false; }
VarId get_var_id(EventHandle, const char *) { return {}; }
VarId get_var_id_global(const char *) { return {}; }
bool set_var(EventHandle, const VarId &, float) { return false; }
bool set_var(EventHandle, const char *, float) { return false; }
bool set_var_optional(EventHandle, const char *, float) { return false; }
void set_vars(EventHandle, dag::ConstSpan<VarId>, dag::ConstSpan<float>) {}
bool set_var_global(const char *, float) { return false; }
bool set_var_global(const VarId &, float) { return false; }
void get_info(uint32_t &, uint32_t &) {}
void set_node_id(EventHandle, dag::Index16) {}
dag::Index16 get_node_id(EventHandle) { return {}; }
float get_max_distance(EventHandle) { return -1.f; }
float get_max_distance(const char *) { return -1.f; }
bool get_length(const char *, int &) { return false; }
float get_audibility(FMOD::Studio::EventInstance *eventInstance) { return 0.f; }
void block_programmer_sounds(bool) {}
int get_programmer_sounds_generation() { return 0; }
void increment_programmer_sounds_generation() {}

// streams.cpp
StreamHandle init_stream(const char *, const Point2 &, const char *) { return {}; }
void release(StreamHandle &) {}
void open(StreamHandle) {}
void close(StreamHandle) {}
void set_pos(StreamHandle, const Point3 &) {}
bool set_paused(StreamHandle, bool) { return false; }
bool set_volume(StreamHandle, float) { return false; }
void set_fader(StreamHandle, const Point2 &) {}
StreamState get_state(StreamHandle) { return StreamState::ERROR; }
const char *get_state_name(StreamHandle) { return ""; }
bool is_valid_handle(StreamHandle) { return false; }

// visualLabels.cpp
VisualLabels query_visual_labels() { return {}; }

// mixing.cpp
void set_volume(const char *, float) {}
float get_volume(const char *) { return 0.f; }
void set_mute(const char *, bool) {}
bool get_mute(const char *) { return false; }
void lock_channel_group(const char *, bool) {}
void set_pitch(const char *, float) {}
void set_pitch(float) {}

// update.cpp
void update_listener(float, const TMatrix &) {}
void reset_3d_listener() {}
void update(float, float) {}
void override_time_speed(float) {}
Point3 get_3d_listener_pos() { return {}; }
TMatrix get_3d_listener() { return {}; }

// eventInstanceStealing.cpp
void EventInstanceStealingGroup::append(EventHandle) {}
void EventInstanceStealingGroup::update(float, float, float, float, int) {}

} // namespace sndsys
