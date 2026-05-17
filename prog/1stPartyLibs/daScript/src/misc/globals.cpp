// Provides storage for daScript globals that upstream lives in src/hal/debug_break.cpp.
// Dagor's jamfile doesn't build src/hal/ (games supply their own os_debug_break, crash
// handlers, file access hooks), but the smart_ptr and JobStatus tracking globals are
// referenced unconditionally after the `smart_ptr: remove debug macros, always-on
// tracking` change -- put them here so every dagor executable gets them via daScript.lib.

#include "daScript/misc/platform.h"

#include "daScript/misc/smart_ptr.h"
#include "daScript/misc/job_que.h"

uint64_t               das::ptr_ref_count::ref_count_total = 0;
uint64_t               das::ptr_ref_count::ref_count_track = 0;
uint64_t               das::ptr_ref_count::ref_count_track_destructor = 0;
das::ptr_ref_count *   das::ptr_ref_count::ref_count_head = nullptr;
das::mutex             das::ptr_ref_count::ref_count_mutex;

DAS_API das::atomic<uint64_t> das::g_smart_ptr_total {0};

DAS_API das::atomic<uint64_t> das::g_jobque_track_total {0};
DAS_API das::atomic<uint64_t> das::g_jobque_track_id {0};
das::JobStatus *    das::JobStatus::sTrackHead = nullptr;
das::mutex          das::JobStatus::sTrackMutex;
