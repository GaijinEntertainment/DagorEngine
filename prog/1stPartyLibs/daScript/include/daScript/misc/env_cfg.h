#pragma once

#include "daScript/misc/platform.h"

// PS4 (_TARGET_C1) and PS5 (_TARGET_C2); their libc has no environment functions
#ifndef DAS_TARGET_PS
    #if defined(_TARGET_C1) || defined(_TARGET_C2)

    #else
        #define DAS_TARGET_PS 0
    #endif
#endif

namespace das {

    // generic read, for a name which is not known at compile time; on PS there is no
    // environment, and absent means the documented default for every knob below
    DAS_API const char * das_getenv ( const char * name );

    // generic write; on PS there is no environment and the call is a no-op
    DAS_API void das_setenv ( const char * name, const char * value );

    // every entry of the environment as "NAME=VALUE", NULL-terminated, for a reader that needs
    // the whole set rather than one name; null on PS, which has no environment. The raw
    // `environ` symbol is private to env_cfg.cpp - PS does not export it at all.
    DAS_API char * const * das_environ ();

    // every environment variable daslang core reads, one accessor each.
    // documented at https://daslang.io/doc/reference/environment_variables.html

    DAS_API const char * get_dasenv_gc_stage_report ();
    DAS_API const char * get_dasenv_gc_break_on_id ();
    DAS_API const char * get_dasenv_jobque_threads ();
    DAS_API const char * get_dasenv_jobque_affinity ();
    DAS_API const char * get_dasenv_jobque_limit_order ();
    DAS_API const char * get_dasenv_jobque_team_rank_gate ();
    DAS_API const char * get_dasenv_jobque_team_eager_exit ();
    DAS_API const char * get_dasenv_team_prof ();
    DAS_API const char * get_dasenv_trace_module_load ();
    DAS_API const char * get_dasenv_module_cache_limit ();
    DAS_API const char * get_dasenv_log_level ();
    DAS_API const char * get_dasenv_depfile ();
    DAS_API const char * get_dasenv_depfile_target ();
    DAS_API const char * get_dasenv_depfile_exclude ();

    // ambient variables daslang reads but does not own
    DAS_API const char * get_columns ();
}
