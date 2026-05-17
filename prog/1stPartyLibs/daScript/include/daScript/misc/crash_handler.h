#pragma once
#include "daScript/misc/platform.h"

#include <stdint.h>

#ifndef DAS_USE_BASE_CRASH_HANDLER
#define DAS_USE_BASE_CRASH_HANDLER 0
#endif


#if DAS_USE_BASE_CRASH_HANDLER && (defined(_WIN32) || ((defined(__linux__) || defined(__APPLE__)) && !defined(__EMSCRIPTEN__)))
#define DAS_CRASH_HANDLER_PLATFORM_SUPPORTED 1
#else
#define DAS_CRASH_HANDLER_PLATFORM_SUPPORTED 0
#endif


namespace das {
    struct CrashFrame {
        uint64_t     frameRbp;      // frame pointer (RBP/FP)
        uint64_t     frameRsp;      // stack pointer (RSP/SP)
    };

    // Per-frame filter: returns true if this symbol name is "interesting"
    // (e.g. contains "SimNode"). Only matching frames are collected.
    typedef bool (*CrashHandlerFrameFilterFunc)(const char * symbolName);

    // Called after C++ stack trace is printed, with collected interesting frames.
    typedef void (*CrashHandlerExtraInfoFunc)(CrashFrame * frames, int frameCount);

    DAS_API void set_crash_handler_extra_info(CrashHandlerFrameFilterFunc filter,
                                              CrashHandlerExtraInfoFunc callback);
    DAS_API void install_crash_handler();
    DAS_API void print_current_stack_trace();

    // Das-aware crash handler: installs SEH/signal handler + das stack walk callback.
    // Defined in project_specific_crash_handler.cpp.
    DAS_API void install_das_crash_handler();

} // namespace das
