#include "daScript/misc/platform.h"

#include "daScript/misc/smart_ptr.h"

#include <signal.h>

#if DAS_SMART_PTR_ID
    uint64_t das::ptr_ref_count::ref_count_total = 0;
    uint64_t das::ptr_ref_count::ref_count_track = 0;
    das::das_set<uint64_t> das::ptr_ref_count::ref_count_ids;
    das::mutex             das::ptr_ref_count::ref_count_mutex;
#endif

#if DAS_SMART_PTR_TRACKER
    das::atomic<uint64_t> das::g_smart_ptr_total {0};
#endif

void os_debug_break()
{
#ifdef _MSC_VER
    __debugbreak();
#else
    raise(SIGTRAP);
#endif
}
