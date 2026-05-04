#include "daScript/misc/platform.h"
#include "daScript/misc/crash_handler.h"
#include "daScript/simulate/simulate.h"

#include <cstring>

using namespace das;

// Separate function because C++ objects (std::string) cannot coexist with
// __try/__except on MSVC.
static void print_das_stack_walk(void * ctxPtr) {
    auto * ctx = (Context *)ctxPtr;
    string str;
    if (ctx->stack.empty()) {
        str = " (daslang stack empty - reset by recover before C++ overflow)\n";
    } else {
        str = ctx->getStackWalk(nullptr, false, false);
    }
    fprintf(stderr, "%s", str.c_str());
}

static bool das_crash_frame_filter(const char * symbolName) {
    return strstr(symbolName, "SimNode") != nullptr;
}

#if DAS_CRASH_HANDLER_PLATFORM_SUPPORTED

// ---- Platform-specific safe memory reads --------------------------------

#if defined(_WIN32)

static uint32_t safe_read_u32(uint64_t addr) {
    uint32_t result = 0;
    __try {
        result = *(uint32_t *)addr;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        result = 0;
    }
    return result;
}

static void * safe_read_ptr(uint64_t addr) {
    void * result = nullptr;
    __try {
        result = *(void **)addr;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        result = nullptr;
    }
    return result;
}

#elif defined(__linux__) && !defined(__EMSCRIPTEN__)

#include <sys/mman.h>

// mincore() returns ENOMEM for unmapped ranges, 0 for mapped ones.
static bool is_addr_mapped(uint64_t addr, size_t size) {
    const uintptr_t PAGE = 4096;
    uintptr_t start = (uintptr_t)addr & ~(PAGE - 1);
    uintptr_t len = (((uintptr_t)addr + size - start) + PAGE - 1) & ~(PAGE - 1);
    unsigned char vec[8] = {};  // covers up to 8 pages
    if (len > sizeof(vec) * PAGE) return false;
    return mincore((void *)start, len, vec) == 0;
}

static uint32_t safe_read_u32(uint64_t addr) {
    if (!is_addr_mapped(addr, sizeof(uint32_t))) return 0;
    return *(uint32_t *)(uintptr_t)addr;
}

static void * safe_read_ptr(uint64_t addr) {
    if (!is_addr_mapped(addr, sizeof(void *))) return nullptr;
    return *(void **)(uintptr_t)addr;
}

#elif defined(__APPLE__) && !defined(__EMSCRIPTEN__)

#include <mach/mach.h>
#include <mach/mach_vm.h>

// mach_vm_read_overwrite() handles unmapped/unreadable pages gracefully and
// is safe to call from a signal handler (it's a Mach trap, not a POSIX call).
static uint32_t safe_read_u32(uint64_t addr) {
    uint32_t result = 0;
    mach_vm_size_t bytes_read = 0;
    mach_vm_read_overwrite(mach_task_self(),
        (mach_vm_address_t)addr, sizeof(uint32_t),
        (mach_vm_address_t)&result, &bytes_read);
    return (bytes_read == sizeof(uint32_t)) ? result : 0;
}

static void * safe_read_ptr(uint64_t addr) {
    void * result = nullptr;
    mach_vm_size_t bytes_read = 0;
    mach_vm_read_overwrite(mach_task_self(),
        (mach_vm_address_t)addr, sizeof(void *),
        (mach_vm_address_t)&result, &bytes_read);
    return (bytes_read == sizeof(void *)) ? result : nullptr;
}

#endif  // platform safe reads

// ---- Shared: context validation and das stack walk ----------------------

// Validate a candidate Context* by checking the magic number.
// context_magic is the first non-vtable field: layout [vtable(8)][magic(4)]...
static bool validate_context_ptr(void * ctxPtr) {
    uint64_t addr = (uint64_t)(uintptr_t)ctxPtr + offsetof(Context, context_magic);
    return safe_read_u32(addr) == Context::CONTEXT_MAGIC;
}

// Scan a pointer-aligned window around each collected frame pointer for a
// valid Context*.  The window covers both negative offsets (GCC/Clang spill
// slots on Linux/Mac) and [FP+0] (MSVC virtual-frame slot on Windows).
static void das_crash_extra_info(CrashFrame * frames, int frameCount) {
    static const int MAX_CONTEXTS = 16;
    void * contexts[MAX_CONTEXTS];
    int contextCount = 0;
    for (int i = 0; i < frameCount; i++) {
        const auto rbp = frames[i].frameRbp;
        for (int64_t off = -128; off <= 8; off += (int64_t)sizeof(void *)) {
            uint64_t probe = (uint64_t)((int64_t)rbp + off);
            void * candidate = safe_read_ptr(probe);
            if (!candidate || !validate_context_ptr(candidate))
                continue;
            bool found = false;
            for (int j = 0; j < contextCount; j++) {
                if (contexts[j] == candidate) {
                    found = true;
                    break;
                }
            }
            if (!found && contextCount < MAX_CONTEXTS) {
                contexts[contextCount++] = candidate;
                break;
            }
        }
    }
    for (int i = 0; i < contextCount; i++) {
        fprintf(stderr, "\ndaslang stack (Context 0x%llx):\n",
            (unsigned long long)(uintptr_t)contexts[i]);
        print_das_stack_walk(contexts[i]);
    }
}

#endif  // DAS_CRASH_HANDLER_PLATFORM_SUPPORTED

namespace das {
    void install_das_crash_handler() {
        #if DAS_CRASH_HANDLER_PLATFORM_SUPPORTED
        set_crash_handler_extra_info(das_crash_frame_filter, das_crash_extra_info);
        #endif
        install_crash_handler();
    }
}
