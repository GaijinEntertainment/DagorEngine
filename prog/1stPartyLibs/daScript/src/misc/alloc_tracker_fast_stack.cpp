#include "daScript/misc/platform.h"

// Fast Win64 stack unwinder for the heap-leak tracker. Replaces the
// per-allocation call to CaptureStackBackTrace (which takes a spinlock inside
// RtlLookupFunctionEntry) with:
//   1) A cached PE .pdata table -- binary-searched, no OS locks.
//   2) A lightweight UNWIND_INFO interpreter that avoids RtlVirtualUnwind for
//      the common opcodes.
//   3) A single-level "landmark" RAII that fully walks once, stores the
//      remaining chain, and lets later captures tail-memoize from a known
//      frame (matched by RSP).
//
// Non-Win64 platforms fall through to the generic path in alloc_tracker.cpp.
// Ported from Gaijin's dag_fastStackCapture.cpp.

#if DAS_TRACK_ALLOC && defined(_MSC_VER) && defined(_M_X64)

#include <windows.h>
#include <winnt.h>
#include <tlhelp32.h>
#include <intrin.h>

#include <atomic>
#include <mutex>
#include <new>         // placement new for the immortal mutex storage
#include <algorithm>
#include <cstring>
#include <cstdint>

namespace das {

// ---------------------------------------------------------------------------
// Module cache
// ---------------------------------------------------------------------------

struct CachedModule {
    ULONG64             base;
    ULONG64             end;
    PRUNTIME_FUNCTION   pdata;
    ULONG               pdataLast;
};

static constexpr int MAX_CACHED_MODULES = 512;
static CachedModule         g_modules[MAX_CACHED_MODULES];
static std::atomic<int>     g_moduleCount{0};
static std::atomic<bool>    g_cacheReady{false};
static std::mutex &         getCacheMutex() {
    alignas(std::mutex) static unsigned char storage[sizeof(std::mutex)];
    static std::mutex *m = ::new (storage) std::mutex();
    return *m;
}

// Negative cache for PCs that don't belong to any module (JIT code, etc).
// Direct-mapped, lossy on collisions; page granularity is 64KB.
static constexpr int NEG_CACHE_BITS = 6;
static constexpr int NEG_CACHE_SIZE = 1 << NEG_CACHE_BITS;
static std::atomic<uint64_t> g_negCache[NEG_CACHE_SIZE];

static __forceinline bool isNegCached(ULONG64 pc) {
    uint64_t page = pc >> 16;
    return g_negCache[page & (NEG_CACHE_SIZE - 1)].load(std::memory_order_relaxed) == page;
}
static __forceinline void addNegCache(ULONG64 pc) {
    uint64_t page = pc >> 16;
    g_negCache[page & (NEG_CACHE_SIZE - 1)].store(page, std::memory_order_relaxed);
}

static void buildModuleCache() noexcept {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
    if (snap == INVALID_HANDLE_VALUE) return;
    MODULEENTRY32W me;
    me.dwSize = sizeof(me);
    int count = 0;
    BOOL ok = Module32FirstW(snap, &me);
    while (ok && count < MAX_CACHED_MODULES) {
        const BYTE *dllBase = me.modBaseAddr;
        if (dllBase && me.modBaseSize) {
            __try {
                const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER*)dllBase;
                if (dos->e_magic == IMAGE_DOS_SIGNATURE) {
                    const IMAGE_NT_HEADERS64 *nt = (const IMAGE_NT_HEADERS64*)(dllBase + dos->e_lfanew);
                    if (nt->Signature == IMAGE_NT_SIGNATURE &&
                        nt->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_EXCEPTION) {
                        const IMAGE_DATA_DIRECTORY &excDir =
                            nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
                        if (excDir.VirtualAddress && excDir.Size >= sizeof(RUNTIME_FUNCTION)) {
                            CachedModule &m = g_modules[count];
                            m.base       = (ULONG64)dllBase;
                            m.end        = (ULONG64)dllBase + me.modBaseSize;
                            m.pdata      = (PRUNTIME_FUNCTION)(dllBase + excDir.VirtualAddress);
                            m.pdataLast  = (excDir.Size / sizeof(RUNTIME_FUNCTION)) - 1;
                            ++count;
                        }
                    }
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {}
        }
        ok = Module32NextW(snap, &me);
    }
    CloseHandle(snap);
    std::sort(g_modules, g_modules + count,
              [](const CachedModule &a, const CachedModule &b) { return a.base < b.base; });
    g_moduleCount.store(count, std::memory_order_release);
}

static __declspec(noinline) void ensureCacheReady() noexcept {
    if (g_cacheReady.load(std::memory_order_acquire)) return;
    std::lock_guard<std::mutex> lock(getCacheMutex());
    if (g_cacheReady.load(std::memory_order_relaxed)) return;
    buildModuleCache();
    g_cacheReady.store(true, std::memory_order_release);
}

static __forceinline bool findModule(ULONG64 pc, const CachedModule *&lastModule, ULONG64 &imageBase) {
    if (lastModule && pc >= imageBase && pc < lastModule->end)
        return true;
    lastModule = nullptr;
    int count = g_moduleCount.load(std::memory_order_acquire);
    int lo = 0, hi = count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) >> 1;
        const CachedModule &mod = g_modules[mid];
        if (pc < mod.base)       hi = mid - 1;
        else if (pc >= mod.end)  lo = mid + 1;
        else { lastModule = &mod; imageBase = mod.base; return true; }
    }
    return false;
}

static __forceinline PRUNTIME_FUNCTION findFunctionEntry(const CachedModule *mod, DWORD rva, int &hint) {
    PRUNTIME_FUNCTION table = mod->pdata;
    int lo = 0, hi = (int)mod->pdataLast;
    int mid = hint;
    goto first_check;
    do {
        mid = (lo + hi) >> 1;
    first_check:
        if (rva < table[mid].BeginAddress)      hi = mid - 1;
        else if (rva >= table[mid].EndAddress)  lo = mid + 1;
        else { hint = mid; return &table[mid]; }
    } while (lo <= hi);
    return nullptr;
}

static __forceinline PRUNTIME_FUNCTION cachedLookupOnly(ULONG64 pc, const CachedModule *&lastModule,
                                                       ULONG64 &imageBase, int &pdataHint) {
    const CachedModule *prev = lastModule;
    if (findModule(pc, lastModule, imageBase)) {
        if (lastModule != prev) pdataHint = (int)(lastModule->pdataLast) >> 1;
        return findFunctionEntry(lastModule, (DWORD)(pc - imageBase), pdataHint);
    }
    return nullptr;
}

// SEH-wrapped PE parse: reads from dllBase may fault on unmapped pages during
// module teardown. Returns true if newMod was populated. Kept free of C++
// objects with destructors so MSVC allows the __try/__except (C2712).
static bool parseModuleHeader(ULONG64 pc, const BYTE *dllBase, CachedModule &newMod) noexcept {
    __try {
        const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER*)dllBase;
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) { addNegCache(pc); return false; }
        const IMAGE_NT_HEADERS64 *nt = (const IMAGE_NT_HEADERS64*)(dllBase + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE ||
            nt->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_EXCEPTION) {
            addNegCache(pc); return false;
        }
        const IMAGE_DATA_DIRECTORY &excDir =
            nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
        if (!excDir.VirtualAddress || excDir.Size < sizeof(RUNTIME_FUNCTION)) {
            addNegCache(pc); return false;
        }
        newMod.base       = (ULONG64)dllBase;
        newMod.end        = (ULONG64)dllBase + nt->OptionalHeader.SizeOfImage;
        newMod.pdata      = (PRUNTIME_FUNCTION)(dllBase + excDir.VirtualAddress);
        newMod.pdataLast  = (excDir.Size / sizeof(RUNTIME_FUNCTION)) - 1;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        addNegCache(pc);
        return false;
    }
}

// Incremental add for late-loaded DLLs via GetModuleHandleExW — avoids full snapshot reinit.
//
// Concurrency: writers serialize through getCacheMutex(); readers in findModule()
// load g_modules[] without locking. The memmove below shifts existing entries
// in place, so a concurrent reader's binary search may briefly observe torn
// base/end/pdataLast values from an in-flight shift. Possible outcomes:
//   1. Mis-comparison → findModule returns nullptr → caller falls into
//      tryAddAndLookup again, which re-acquires the writer lock and finds the
//      module already present. Wasted work, no functional damage.
//   2. Garbage pdataLast or pdata → findFunctionEntry walks an invalid table →
//      access violation → outer __try/__except in walk_from_context catches it
//      and we return a partial stack.
// The race window is the duration of one memmove (≤256 entries × 24 bytes,
// micro-second scale) and only opens when a DLL is loaded after the initial
// snapshot — i.e. fio::register_dynamic_module, JIT codegen, or transitive
// LoadLibrary. Net effect at the diagnostic level is rare truncated frames,
// no crashes, which matches the trade-off in the upstream Gaijin source.
static bool tryAddModuleForPC(ULONG64 pc) noexcept {
    if (isNegCached(pc)) return false;
    HMODULE hMod = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            (LPCWSTR)(uintptr_t)pc, &hMod) || !hMod) {
        addNegCache(pc);
        return false;
    }
    CachedModule newMod{};
    if (!parseModuleHeader(pc, (const BYTE*)hMod, newMod)) return false;

    std::lock_guard<std::mutex> lock(getCacheMutex());
    int count = g_moduleCount.load(std::memory_order_relaxed);
    if (count >= MAX_CACHED_MODULES) return false;
    // sorted insert
    int lo = 0, hi = count - 1, insertPos = count;
    while (lo <= hi) {
        int mid = (lo + hi) >> 1;
        if (newMod.base < g_modules[mid].base)      { insertPos = mid; hi = mid - 1; }
        else if (newMod.base > g_modules[mid].base) { lo = mid + 1; }
        else                                         { return true; } // already added by other thread
    }
    memmove(&g_modules[insertPos + 1], &g_modules[insertPos],
            (count - insertPos) * sizeof(CachedModule));
    g_modules[insertPos] = newMod;
    g_moduleCount.store(count + 1, std::memory_order_release);
    return true;
}

static __declspec(noinline) PRUNTIME_FUNCTION tryAddAndLookup(ULONG64 pc, const CachedModule *&lastMod,
                                                              ULONG64 &imageBase, int &pdataHint) noexcept {
    return tryAddModuleForPC(pc) ? cachedLookupOnly(pc, lastMod, imageBase, pdataHint) : nullptr;
}

// ---------------------------------------------------------------------------
// Lightweight UNWIND_INFO interpreter
// ---------------------------------------------------------------------------

#define UNW_UWOP_PUSH_NONVOL     0
#define UNW_UWOP_ALLOC_LARGE     1
#define UNW_UWOP_ALLOC_SMALL     2
#define UNW_UWOP_SET_FPREG       3
#define UNW_UWOP_SAVE_NONVOL     4
#define UNW_UWOP_SAVE_NONVOL_FAR 5
#define UNW_UWOP_SAVE_XMM128     8
#define UNW_UWOP_SAVE_XMM128_FAR 9
#define UNW_UWOP_PUSH_MACHFRAME  10

struct UNWIND_CODE_ENTRY_X {
    BYTE CodeOffset;
    BYTE UnwindOp : 4;
    BYTE OpInfo   : 4;
};
struct UNWIND_INFO_HEADER_X {
    BYTE VersionAndFlags;
    BYTE SizeOfProlog;
    BYTE CountOfCodes;
    BYTE FrameRegAndOffset;
    UNWIND_CODE_ENTRY_X UnwindCode[1]; // variable length
};

static __forceinline DWORD64 &contextGpReg(CONTEXT &ctx, unsigned regNum) {
    return (&ctx.Rax)[regNum];
}

static __forceinline bool lightweightVirtualUnwind(PRUNTIME_FUNCTION func, ULONG64 imageBase, CONTEXT &ctx) {
    struct ChainEntry {
        const UNWIND_INFO_HEADER_X *info;
        PRUNTIME_FUNCTION           func;
    };
    constexpr int MAX_CHAIN = 8;
    ChainEntry chain[MAX_CHAIN];
    int chainLen = 0;

    {
        const UNWIND_INFO_HEADER_X *cur = (const UNWIND_INFO_HEADER_X*)(imageBase + func->UnwindData);
        PRUNTIME_FUNCTION curFunc = func;
        while (chainLen < MAX_CHAIN) {
            chain[chainLen++] = { cur, curFunc };
            if (!(cur->VersionAndFlags & 0x20)) break; // UNW_FLAG_CHAININFO
            unsigned codeSlots = (cur->CountOfCodes + 1) & ~1u;
            curFunc = (PRUNTIME_FUNCTION)&cur->UnwindCode[codeSlots];
            cur = (const UNWIND_INFO_HEADER_X*)(imageBase + curFunc->UnwindData);
        }
    }

    const UNWIND_INFO_HEADER_X *primaryInfo = chain[0].info;
    unsigned frameReg = primaryInfo->FrameRegAndOffset & 0x0F;
    if (frameReg != 0) {
        unsigned frameOffset = (primaryInfo->FrameRegAndOffset >> 4) & 0x0F;
        ctx.Rsp = contextGpReg(ctx, frameReg) - (DWORD64)frameOffset * 16;
    }
    DWORD64 initialRsp = ctx.Rsp;
    DWORD ripRva = (DWORD)(ctx.Rip - imageBase);

    for (int c = 0; c < chainLen; ++c) {
        const UNWIND_INFO_HEADER_X *info = chain[c].info;
        DWORD funcBegin = chain[c].func->BeginAddress;
        int count = info->CountOfCodes;
        for (int i = 0; i < count;) {
            const UNWIND_CODE_ENTRY_X &code = info->UnwindCode[i];
            bool executed = (c > 0) || (ripRva >= (DWORD)(funcBegin + code.CodeOffset));
            switch (code.UnwindOp) {
                case UNW_UWOP_PUSH_NONVOL:
                    if (executed) {
                        contextGpReg(ctx, code.OpInfo) = *(DWORD64*)ctx.Rsp;
                        ctx.Rsp += 8;
                    }
                    i += 1; break;
                case UNW_UWOP_ALLOC_LARGE:
                    if (code.OpInfo == 0) {
                        if (executed && (i + 1) < count)
                            ctx.Rsp += *(const USHORT*)&info->UnwindCode[i + 1] * 8;
                        i += 2;
                    } else {
                        if (executed && (i + 2) < count)
                            ctx.Rsp += *(const ULONG*)&info->UnwindCode[i + 1];
                        i += 3;
                    }
                    break;
                case UNW_UWOP_ALLOC_SMALL:
                    if (executed) ctx.Rsp += code.OpInfo * 8 + 8;
                    i += 1; break;
                case UNW_UWOP_SET_FPREG: i += 1; break;
                case UNW_UWOP_SAVE_NONVOL:
                    if (executed && (i + 1) < count) {
                        ULONG offset = *(const USHORT*)&info->UnwindCode[i + 1] * 8;
                        contextGpReg(ctx, code.OpInfo) = *(DWORD64*)(initialRsp + offset);
                    }
                    i += 2; break;
                case UNW_UWOP_SAVE_NONVOL_FAR:
                    if (executed && (i + 2) < count) {
                        ULONG offset = *(const ULONG*)&info->UnwindCode[i + 1];
                        contextGpReg(ctx, code.OpInfo) = *(DWORD64*)(initialRsp + offset);
                    }
                    i += 3; break;
                case UNW_UWOP_SAVE_XMM128:     i += 2; break;
                case UNW_UWOP_SAVE_XMM128_FAR: i += 3; break;
                case UNW_UWOP_PUSH_MACHFRAME:  return false;
                default:                       return false;
            }
        }
    }
    ctx.Rip = *(DWORD64*)ctx.Rsp;
    ctx.Rsp += 8;
    return true;
}

// ---------------------------------------------------------------------------
// Landmark (tail-memoize cache)
//
// Tail-memoize matches by RSP, which is inherently per-thread, so the landmark
// is thread_local: each thread arms its own landmark on entry and consumes its
// own on subsequent captures. No cross-thread races, no atomics required.
// Before this was thread_local, enterCount was a plain int on a process-global
// landmark and concurrent enter/exit on different threads was a data race.
// ---------------------------------------------------------------------------

static constexpr int kLandmarkFrames = 32;

struct Landmark {
    bool     active = false;
    DWORD64  rsp = 0;
    int      frameCount = 0;
    void *   frames[kLandmarkFrames] = {};
    int      enterCount = 0; // nested enters no-op; only outermost arms
};
static thread_local Landmark g_landmark;

// ---------------------------------------------------------------------------
// Walk inner: given a captured CONTEXT, fill `stack[]` by unwinding.
// Uses the landmark cache if armed and the match flag is set.
// ---------------------------------------------------------------------------

static __declspec(noinline) unsigned walk_from_context(void **stack, unsigned maxFrames,
                                                      CONTEXT &ctx, int skipFrames,
                                                      bool useLandmark) noexcept {
    const CachedModule *lastMod = nullptr;
    ULONG64             imageBase = 0;
    int                 pdataHint = -1;
    int                 frame = -skipFrames;

    DWORD64 landmarkRsp = 0;
    bool    canMatch    = false;
    if (useLandmark && g_landmark.active) {
        landmarkRsp = g_landmark.rsp;
        canMatch    = (landmarkRsp != 0);
    }

    __try {
        while (ctx.Rip && frame < (int)maxFrames) {
            // Try to tail-memoize from landmark before recording this frame.
            if (canMatch && frame >= 0 && ctx.Rsp == landmarkRsp) {
                int copy = g_landmark.frameCount;
                int room = (int)maxFrames - frame;
                if (copy > room) copy = room;
                memcpy(&stack[frame], g_landmark.frames, copy * sizeof(void*));
                return (unsigned)(frame + copy);
            }
            if (frame >= 0) stack[frame] = (void*)(uintptr_t)ctx.Rip;

            PRUNTIME_FUNCTION pFunc = cachedLookupOnly(ctx.Rip, lastMod, imageBase, pdataHint);
            if (!pFunc)
                pFunc = tryAddAndLookup(ctx.Rip, lastMod, imageBase, pdataHint);
            if (!pFunc) break;

            if (!lightweightVirtualUnwind(pFunc, imageBase, ctx)) {
                // PUSH_MACHFRAME or unknown opcode → fall back to OS.
                PVOID handlerData = nullptr;
                ULONG64 establisherFrame = 0;
                RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, ctx.Rip, pFunc,
                                 &ctx, &handlerData, &establisherFrame, nullptr);
            }
            ++frame;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // Partial result is fine for diagnostics.
    }
    return frame > 0 ? (unsigned)frame : 0;
}

// ---------------------------------------------------------------------------
// Public API (internal to this translation unit / alloc_tracker.cpp)
// ---------------------------------------------------------------------------

// Default skipFrames=2 drops this function's own frame AND the caller (the
// tracker hook, which is always track_alloc_hook and carries no signal). Both
// this function and track_alloc_hook are declared noinline so the skip count
// is stable across /Ob levels.
__declspec(noinline) unsigned das_fast_stack_capture(void **stack, unsigned maxFrames, int skipFrames) noexcept {
    if (!stack || !maxFrames) return 0;
    CONTEXT ctx;
    RtlCaptureContext(&ctx);
    ensureCacheReady();
    return walk_from_context(stack, maxFrames, ctx, skipFrames, /*useLandmark*/ true);
}

// Arm/disarm the landmark. First enter fully walks the stack from the caller's
// frame and stores it; later captures matching the caller's RSP short-circuit
// with a memcpy. Nested enters are no-ops. disarm clears when refcount hits 0.
//
// SEH-wrapped: cachedLookupOnly / tryAddAndLookup / lightweightVirtualUnwind
// may AV on torn reads during the late-DLL-load race window or on freshly
// unmapped DLLs. Failure degrades to "landmark disabled" (active stays false
// and subsequent captures just walk fully), never a process crash.
__declspec(noinline) void das_fast_stack_landmark_enter() noexcept {
    // Guard against nested enters (e.g. recursive compile_and_run).
    if (g_landmark.enterCount++ != 0) return;

    __try {
        CONTEXT ctx;
        RtlCaptureContext(&ctx);
        ensureCacheReady();

        // Unwind once to leave this function — ctx then describes the caller frame.
        const CachedModule *lastMod = nullptr;
        ULONG64 imageBase = 0;
        int     pdataHint = -1;
        PRUNTIME_FUNCTION pFunc = cachedLookupOnly(ctx.Rip, lastMod, imageBase, pdataHint);
        if (!pFunc) pFunc = tryAddAndLookup(ctx.Rip, lastMod, imageBase, pdataHint);
        if (!pFunc) return; // give up; leave landmark disarmed
        if (!lightweightVirtualUnwind(pFunc, imageBase, ctx)) {
            PVOID handlerData = nullptr;
            ULONG64 establisherFrame = 0;
            RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, ctx.Rip, pFunc,
                             &ctx, &handlerData, &establisherFrame, nullptr);
        }

        DWORD64 landmarkRsp = ctx.Rsp;
        // Walk the rest WITHOUT landmark match (we're building it). walk_from_context
        // has its own __try/__except, so any fault inside it returns a partial frame
        // count rather than escaping.
        g_landmark.frameCount = (int)walk_from_context(g_landmark.frames, kLandmarkFrames, ctx,
                                                       /*skipFrames*/ 0, /*useLandmark*/ false);
        g_landmark.rsp    = landmarkRsp;
        g_landmark.active = true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // Leave landmark disarmed; subsequent captures will walk without memoization.
    }
}

__declspec(noinline) void das_fast_stack_landmark_exit() noexcept {
    if (--g_landmark.enterCount != 0) return;
    g_landmark.active     = false;
    g_landmark.rsp        = 0;
    g_landmark.frameCount = 0;
}

} // namespace das

#endif // DAS_TRACK_ALLOC && _MSC_VER && _M_X64
