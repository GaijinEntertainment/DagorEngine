#include "daScript/misc/crash_handler.h"

#include <cstdio>

namespace das {
    namespace {
        constexpr int DAS_CRASH_HANDLER_MAX_STACK_FRAMES = 64;

        CrashHandlerFrameFilterFunc g_crash_handler_frame_filter = nullptr;
        CrashHandlerExtraInfoFunc g_crash_handler_extra_info = nullptr;
    }

    void set_crash_handler_extra_info(CrashHandlerFrameFilterFunc filter,
                                      CrashHandlerExtraInfoFunc callback) {
        g_crash_handler_frame_filter = filter;
        g_crash_handler_extra_info = callback;
    }
}

#if DAS_CRASH_HANDLER_PLATFORM_SUPPORTED && defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dbghelp.h>
#include <process.h>
#pragma comment(lib, "dbghelp.lib")

namespace das {
    static void print_stack_trace(CONTEXT *ctx) {
        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
        if (!SymInitialize(process, NULL, TRUE)) {
            fprintf(stderr, "SymInitialize failed: %lu\n", GetLastError());
            return;
        }
        STACKFRAME64 frame = {};
#if defined(_M_X64)
        frame.AddrPC.Offset = ctx->Rip;
        frame.AddrFrame.Offset = ctx->Rbp;
        frame.AddrStack.Offset = ctx->Rsp;
        DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
#elif defined(_M_ARM64)
        frame.AddrPC.Offset = ctx->Pc;
        frame.AddrFrame.Offset = ctx->Fp;
        frame.AddrStack.Offset = ctx->Sp;
        DWORD machineType = IMAGE_FILE_MACHINE_ARM64;
#else
        frame.AddrPC.Offset = ctx->Eip;
        frame.AddrFrame.Offset = ctx->Ebp;
        frame.AddrStack.Offset = ctx->Esp;
        DWORD machineType = IMAGE_FILE_MACHINE_I386;
#endif
        frame.AddrPC.Mode = AddrModeFlat;
        frame.AddrFrame.Mode = AddrModeFlat;
        frame.AddrStack.Mode = AddrModeFlat;

        alignas(SYMBOL_INFO) char symbolBuf[sizeof(SYMBOL_INFO) + 256];
        SYMBOL_INFO *symbol = (SYMBOL_INFO *)symbolBuf;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = 255;

        IMAGEHLP_LINE64 line = {};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        CrashFrame collectedFrames[DAS_CRASH_HANDLER_MAX_STACK_FRAMES];
        int frameCount = 0;

        fprintf(stderr, "Stack trace:\n");
        for (int i = 0; i < DAS_CRASH_HANDLER_MAX_STACK_FRAMES; i++) {
            if (!StackWalk64(machineType, process, thread, &frame, ctx, NULL,
                             SymFunctionTableAccess64, SymGetModuleBase64, NULL))
                break;
            if (frame.AddrPC.Offset == 0)
                break;
            DWORD64 displacement64 = 0;
            DWORD displacement32 = 0;
            const char * symName = nullptr;
            if (SymFromAddr(process, frame.AddrPC.Offset, &displacement64, symbol)) {
                symName = symbol->Name;
                if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &displacement32, &line)) {
                    fprintf(stderr, "  [%2d] %s + 0x%llx (%s:%lu)\n", i, symName,
                            (unsigned long long)displacement64, line.FileName, line.LineNumber);
                } else {
                    fprintf(stderr, "  [%2d] %s + 0x%llx\n", i, symName,
                            (unsigned long long)displacement64);
                }
            } else {
                fprintf(stderr, "  [%2d] 0x%llx\n", i, (unsigned long long)frame.AddrPC.Offset);
            }
            if (g_crash_handler_frame_filter && symName
                    && g_crash_handler_frame_filter(symName)
                    && frameCount < DAS_CRASH_HANDLER_MAX_STACK_FRAMES) {
                collectedFrames[frameCount].frameRbp = frame.AddrFrame.Offset;
                collectedFrames[frameCount].frameRsp = frame.AddrStack.Offset;
                frameCount++;
            }
        }
        SymCleanup(process);
        if (g_crash_handler_extra_info && frameCount > 0) {
            g_crash_handler_extra_info(collectedFrames, frameCount);
        }
    }

    static const char * exception_code_to_string(DWORD code) {
        switch (code) {
            case EXCEPTION_ACCESS_VIOLATION:       return "EXCEPTION_ACCESS_VIOLATION";
            case EXCEPTION_STACK_OVERFLOW:         return "EXCEPTION_STACK_OVERFLOW";
            case EXCEPTION_INT_DIVIDE_BY_ZERO:     return "EXCEPTION_INT_DIVIDE_BY_ZERO";
            case EXCEPTION_ILLEGAL_INSTRUCTION:    return "EXCEPTION_ILLEGAL_INSTRUCTION";
            case EXCEPTION_FLT_DIVIDE_BY_ZERO:     return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
            case EXCEPTION_FLT_OVERFLOW:           return "EXCEPTION_FLT_OVERFLOW";
            case EXCEPTION_FLT_UNDERFLOW:          return "EXCEPTION_FLT_UNDERFLOW";
            case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:  return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
            case EXCEPTION_BREAKPOINT:             return "EXCEPTION_BREAKPOINT";
            case EXCEPTION_DATATYPE_MISALIGNMENT:  return "EXCEPTION_DATATYPE_MISALIGNMENT";
            case EXCEPTION_IN_PAGE_ERROR:          return "EXCEPTION_IN_PAGE_ERROR";
            default:                               return "UNKNOWN_EXCEPTION";
        }
    }

    static bool is_fatal_exception(DWORD code) {
        switch (code) {
            case EXCEPTION_ACCESS_VIOLATION:
            case EXCEPTION_STACK_OVERFLOW:
            case EXCEPTION_INT_DIVIDE_BY_ZERO:
            case EXCEPTION_ILLEGAL_INSTRUCTION:
            case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            case EXCEPTION_FLT_OVERFLOW:
            case EXCEPTION_FLT_UNDERFLOW:
            case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            case EXCEPTION_DATATYPE_MISALIGNMENT:
            case EXCEPTION_IN_PAGE_ERROR:
                return true;
            default:
                return false;
        }
    }

    static LONG WINAPI crash_handler_callback(EXCEPTION_POINTERS *ep) {
        DWORD code = ep->ExceptionRecord->ExceptionCode;
        if (!is_fatal_exception(code))
            return EXCEPTION_CONTINUE_SEARCH;
        static volatile long in_handler = 0;
        if (InterlockedExchange(&in_handler, 1)) {
            _exit(3);
        }
        if (code == EXCEPTION_STACK_OVERFLOW) {
            static const char msg[] = "\nCRASH: EXCEPTION_STACK_OVERFLOW (0xC00000FD)\n"
                                      "  (stack overflow - stack trace unavailable)\n";
            WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, sizeof(msg) - 1, NULL, NULL);
            _exit(3);
        }
        fprintf(stderr, "\nCRASH: %s (0x%08lX) at address 0x%llx\n",
                exception_code_to_string(code), (unsigned long)code,
                (unsigned long long)ep->ExceptionRecord->ExceptionAddress);
        if (code == EXCEPTION_ACCESS_VIOLATION && ep->ExceptionRecord->NumberParameters >= 2) {
            fprintf(stderr, "  %s address 0x%llx\n",
                    ep->ExceptionRecord->ExceptionInformation[0] ? "writing" : "reading",
                    (unsigned long long)ep->ExceptionRecord->ExceptionInformation[1]);
        }
        print_stack_trace(ep->ContextRecord);
        fflush(stderr);
        _exit(3);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    void install_crash_handler() {
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
        ULONG stackGuarantee = 32 * 1024;
        SetThreadStackGuarantee(&stackGuarantee);
        SetUnhandledExceptionFilter(crash_handler_callback);
    }

    void print_current_stack_trace() {
        CONTEXT ctx;
        RtlCaptureContext(&ctx);
        print_stack_trace(&ctx);
        fflush(stderr);
    }
}

#elif DAS_CRASH_HANDLER_PLATFORM_SUPPORTED && ((defined(__linux__) || defined(__APPLE__)) && !defined(__EMSCRIPTEN__))

#include <signal.h>
#include <execinfo.h>
#include <cxxabi.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <utility>

namespace das {
    static const char * signal_to_string(int sig) {
        switch (sig) {
            case SIGSEGV: return "SIGSEGV (Segmentation fault)";
            case SIGBUS:  return "SIGBUS (Bus error)";
            case SIGABRT: return "SIGABRT (Aborted)";
            case SIGFPE:  return "SIGFPE (Floating point exception)";
            case SIGILL:  return "SIGILL (Illegal instruction)";
            default:      return "UNKNOWN SIGNAL";
        }
    }

#if defined(__APPLE__) && defined(__aarch64__)
    static inline uintptr_t strip_pac(uintptr_t p) { return p & (uintptr_t)0x0000FFFFFFFFFFFF; }
#else
    static inline uintptr_t strip_pac(uintptr_t p) { return p; }
#endif

    static char * demangle(char * line) {
        int status = -1;
#if defined(__linux__)
        char *ms = strchr(line, '(');
        char *me = ms ? strchr(ms, '+') : nullptr;
        if (ms && me) {
            char c = *me; *me = '\0';
            char *d = abi::__cxa_demangle(ms + 1, nullptr, nullptr, &status);
            *me = c;
            if (status == 0) return d;
        }
#elif defined(__APPLE__)
        char *zs = strstr(line, "_Z");
        char *ze = zs ? strstr(zs, " + ") : nullptr;
        if (zs && ze) {
            char c = *ze; *ze = '\0';
            char *d = abi::__cxa_demangle(zs, nullptr, nullptr, &status);
            *ze = c;
            if (status == 0) return d;
        }
#endif
        return nullptr;
    }

    static std::pair<uintptr_t, uintptr_t> get_fp_sp(void *ucontext_ptr) {
        uintptr_t fp = 0, sp = 0;
        if (!ucontext_ptr) return {fp, sp};
        ucontext_t *uc = (ucontext_t *)ucontext_ptr;
#if defined(__linux__)
#  if defined(__x86_64__)
        fp = (uintptr_t)uc->uc_mcontext.gregs[REG_RBP];
        sp = (uintptr_t)uc->uc_mcontext.gregs[REG_RSP];
#  elif defined(__aarch64__)
        fp = (uintptr_t)uc->uc_mcontext.regs[29];
        sp = (uintptr_t)uc->uc_mcontext.sp;
#  endif
#elif defined(__APPLE__)
#  if defined(__x86_64__)
        fp = (uintptr_t)uc->uc_mcontext->__ss.__rbp;
        sp = (uintptr_t)uc->uc_mcontext->__ss.__rsp;
#  elif defined(__aarch64__)
        fp = (uintptr_t)uc->uc_mcontext->__ss.__fp;
        sp = (uintptr_t)uc->uc_mcontext->__ss.__sp;
#  endif
#endif
        return {fp, sp};
    }

    static void print_backtrace_frames(void **pcs, CrashFrame *frameInfo, int nframes) {
        if (nframes <= 0) return;
        CrashFrame collectedFrames[DAS_CRASH_HANDLER_MAX_STACK_FRAMES];
        int frameCount = 0;
        fprintf(stderr, "Stack trace:\n");
        char **symbols = backtrace_symbols(pcs, nframes);
        if (symbols) {
            for (int i = 0; i < nframes; i++) {
                char *d = demangle(symbols[i]);
                fprintf(stderr, "  [%2d] %s\n", i, d ? d : symbols[i]);
                if (g_crash_handler_frame_filter && d
                        && g_crash_handler_frame_filter(d)
                        && frameCount < DAS_CRASH_HANDLER_MAX_STACK_FRAMES) {
                    collectedFrames[frameCount++] = frameInfo[i];
                }
                free(d);
            }
            free(symbols);
        } else {
            backtrace_symbols_fd(pcs, nframes, STDERR_FILENO);
        }
        fflush(stderr);
        if (g_crash_handler_extra_info && frameCount > 0) {
            g_crash_handler_extra_info(collectedFrames, frameCount);
        }
    }

    static void crash_signal_handler(int sig, siginfo_t *info, void *ucontext_ptr) {
        fprintf(stderr, "\nCRASH: %s (signal %d)", signal_to_string(sig), sig);
        if (info->si_addr)
            fprintf(stderr, " at address %p", info->si_addr);
        fprintf(stderr, "\n");

        void *fpPCs[DAS_CRASH_HANDLER_MAX_STACK_FRAMES];
        CrashFrame fpFrames[DAS_CRASH_HANDLER_MAX_STACK_FRAMES];
        int fpCount = 0;
        {
            auto [fp, sp] = get_fp_sp(ucontext_ptr);
            for (int i = 0; i < DAS_CRASH_HANDLER_MAX_STACK_FRAMES && fp != 0; i++) {
                uintptr_t *frame_words = (uintptr_t *)fp;
                uintptr_t saved_fp = strip_pac(frame_words[0]);
                uintptr_t ret_pc   = strip_pac(frame_words[1]);
                if (saved_fp == 0 || ret_pc == 0) break;
                fpPCs[fpCount] = (void *)ret_pc;
                fpFrames[fpCount].frameRbp = fp;
                fpFrames[fpCount].frameRsp = sp;
                fpCount++;
                if (saved_fp <= fp) break;
                sp = fp + 2 * sizeof(uintptr_t);
                fp = saved_fp;
            }
        }

        print_backtrace_frames(fpPCs, fpFrames, fpCount);

        signal(sig, SIG_DFL);
        raise(sig);
    }

    void install_crash_handler() {
        static thread_local char alt_stack_buf[64 * 1024];
        stack_t ss = {};
        ss.ss_sp = alt_stack_buf;
        ss.ss_size = sizeof(alt_stack_buf);
        ss.ss_flags = 0;
        sigaltstack(&ss, nullptr);

        struct sigaction sa = {};
        sa.sa_sigaction = crash_signal_handler;
        sa.sa_flags = SA_SIGINFO | SA_RESETHAND | SA_ONSTACK;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGSEGV, &sa, nullptr);
        sigaction(SIGBUS,  &sa, nullptr);
        sigaction(SIGABRT, &sa, nullptr);
        sigaction(SIGFPE,  &sa, nullptr);
        sigaction(SIGILL,  &sa, nullptr);
    }

    void print_current_stack_trace() {
        void *pcs[DAS_CRASH_HANDLER_MAX_STACK_FRAMES];
        CrashFrame frameInfo[DAS_CRASH_HANDLER_MAX_STACK_FRAMES];
        int nframes = 0;
        uintptr_t fp = (uintptr_t)__builtin_frame_address(0);
        uintptr_t sp = (uintptr_t)&fp;
        for (int i = 0; i < DAS_CRASH_HANDLER_MAX_STACK_FRAMES && fp != 0; i++) {
            uintptr_t *frame_words = (uintptr_t *)fp;
            uintptr_t saved_fp = strip_pac(frame_words[0]);
            uintptr_t ret_pc = strip_pac(frame_words[1]);
            if (saved_fp == 0 || ret_pc == 0) break;
            pcs[nframes] = (void *)ret_pc;
            frameInfo[nframes].frameRbp = fp;
            frameInfo[nframes].frameRsp = sp;
            nframes++;
            if (saved_fp <= fp) break;
            sp = fp + 2 * sizeof(uintptr_t);
            fp = saved_fp;
        }
        print_backtrace_frames(pcs, frameInfo, nframes);
    }
}

#else

namespace das {
    void install_crash_handler() {}

    void print_current_stack_trace() {}
}

#endif

void print_current_stack_trace() {
    das::print_current_stack_trace();
}
