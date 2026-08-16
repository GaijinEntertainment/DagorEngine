// The only TU allowed to touch the environment directly. Host configs (dagor)
// poison getenv for everyone else, so this must be defined before platform.h.
#define DAS_ALLOW_GETENV 1

#include "daScript/misc/platform.h"

#include "daScript/misc/env_cfg.h"

#include <stdlib.h>

namespace das {

    const char * das_getenv ( const char * name ) {
#if DAS_TARGET_PS
        (void)name;
        return nullptr;
#else
        return getenv(name);
#endif
    }

    void das_setenv ( const char * name, const char * value ) {
#if DAS_TARGET_PS
        (void)name; (void)value;
#elif defined(_WIN32)
        _putenv_s(name, value ? value : "");
#else
        setenv(name, value ? value : "", 1);
#endif
    }

    const char * get_dasenv_gc_stage_report ()         { return das_getenv("DAS_GC_STAGE_REPORT"); }
    const char * get_dasenv_gc_break_on_id ()          { return das_getenv("DAS_GC_BREAK_ON_ID"); }
    const char * get_dasenv_jobque_threads ()          { return das_getenv("DAS_JOBQUE_THREADS"); }
    const char * get_dasenv_jobque_affinity ()         { return das_getenv("DAS_JOBQUE_AFFINITY"); }
    const char * get_dasenv_jobque_limit_order ()      { return das_getenv("DAS_JOBQUE_LIMIT_ORDER"); }
    const char * get_dasenv_jobque_team_rank_gate ()   { return das_getenv("DAS_JOBQUE_TEAM_RANK_GATE"); }
    const char * get_dasenv_jobque_team_eager_exit ()  { return das_getenv("DAS_JOBQUE_TEAM_EAGER_EXIT"); }
    const char * get_dasenv_team_prof ()               { return das_getenv("DAS_TEAM_PROF"); }
    const char * get_dasenv_trace_module_load ()       { return das_getenv("DAS_TRACE_MODULE_LOAD"); }

    const char * get_columns ()                     { return das_getenv("COLUMNS"); }
}
