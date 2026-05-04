// daslang-live: Live-reloading application host for daScript
//
// Provides init/update/shutdown lifecycle with hot-reload support.
// Scripts use `require live_host` for lifecycle API.
//
// Mode detection:
//   - If script exports `init` → lifecycle mode (init/update/shutdown loop)
//   - If script only exports `main` → call main() directly (same as daslang.exe)

#include "daScript/daScript.h"
#include "daScript/daScriptModule.h"
#include "daScript/das_common.h"
#include "daScript/simulate/fs_file_info.h"
#include "daScript/ast/dyn_modules.h"
#include "daScript/misc/crash_handler.h"
#include <sys/stat.h>

#if defined(_WIN32) && defined(_DEBUG)
#include <crtdbg.h>
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#include <dlfcn.h>
#endif

using namespace das;

void require_project_specific_modules();
das::FileAccessPtr get_file_access( char * pak );

static TextPrinter tout;
static string projectFile;
static string project_root;
static bool version2syntax = true;

// --- DLL function pointers (loaded at runtime from dasModuleLiveHost) ---
// All use POD types only — safe across DLL boundary.

typedef bool (*BoolFn)();
typedef void (*VoidFn)();
typedef void (*SetFloatFn)(float);
typedef void (*SetBoolFn)(bool);

static BoolFn  dll_exit_requested = nullptr;
static BoolFn  dll_reload_requested = nullptr;
static BoolFn  dll_full_reload = nullptr;
static BoolFn  dll_is_paused = nullptr;
static BoolFn  dll_files_changed = nullptr;
static SetFloatFn dll_set_dt = nullptr;
static SetFloatFn dll_set_uptime = nullptr;
static SetFloatFn dll_set_fps = nullptr;
static SetBoolFn  dll_set_is_reload = nullptr;
static SetBoolFn  dll_set_paused = nullptr;
static VoidFn  dll_clear_reload_flags = nullptr;
static VoidFn  dll_clear_live_vars = nullptr;
static VoidFn  dll_clear_store = nullptr;
static VoidFn  dll_clear_error = nullptr;
static BoolFn  dll_is_context_dead = nullptr;
static VoidFn  dll_clear_context_dead = nullptr;

typedef void (*SetStringFn)(const char *);
static SetStringFn dll_set_last_error = nullptr;

typedef void (*SetContextFn)(das::Context *);
typedef void (*SetWatchedFilesFn)(const char **, int);
static SetBoolFn  dll_set_live_mode = nullptr;
static SetContextFn dll_set_dispatch_context = nullptr;
static SetWatchedFilesFn dll_set_watched_files = nullptr;

static void * get_dll_symbol(const char * name) {
#ifdef _WIN32
    HMODULE hMod = GetModuleHandleA("dasModuleLiveHost.shared_module");
    if (!hMod) hMod = GetModuleHandleA("dasModuleLiveHost");
    return hMod ? (void*)GetProcAddress(hMod, name) : nullptr;
#else
    return dlsym(RTLD_DEFAULT, name);
#endif
}

static bool load_live_host_functions() {
    dll_exit_requested    = (BoolFn)get_dll_symbol("live_host_exit_requested");
    dll_reload_requested  = (BoolFn)get_dll_symbol("live_host_reload_requested");
    dll_full_reload       = (BoolFn)get_dll_symbol("live_host_full_reload");
    dll_is_paused         = (BoolFn)get_dll_symbol("live_host_is_paused");
    dll_files_changed     = (BoolFn)get_dll_symbol("live_host_files_changed");
    dll_set_dt            = (SetFloatFn)get_dll_symbol("live_host_set_dt");
    dll_set_uptime        = (SetFloatFn)get_dll_symbol("live_host_set_uptime");
    dll_set_fps           = (SetFloatFn)get_dll_symbol("live_host_set_fps");
    dll_set_is_reload     = (SetBoolFn)get_dll_symbol("live_host_set_is_reload");
    dll_set_paused        = (SetBoolFn)get_dll_symbol("live_host_set_paused");
    dll_clear_reload_flags = (VoidFn)get_dll_symbol("live_host_clear_reload_flags");
    dll_clear_live_vars   = (VoidFn)get_dll_symbol("live_host_clear_live_vars");
    dll_clear_store       = (VoidFn)get_dll_symbol("live_host_clear_store");
    dll_clear_error       = (VoidFn)get_dll_symbol("live_host_clear_error");
    dll_is_context_dead   = (BoolFn)get_dll_symbol("live_host_is_context_dead");
    dll_clear_context_dead = (VoidFn)get_dll_symbol("live_host_clear_context_dead");
    dll_set_last_error    = (SetStringFn)get_dll_symbol("live_host_set_last_error");
    dll_set_live_mode        = (SetBoolFn)get_dll_symbol("live_host_set_live_mode");
    dll_set_dispatch_context = (SetContextFn)get_dll_symbol("live_host_set_dispatch_context");
    dll_set_watched_files    = (SetWatchedFilesFn)get_dll_symbol("live_host_set_watched_files");

    // Check that at least the essential functions are available
    if (!dll_exit_requested || !dll_set_dt) {
        tout << "WARNING: could not find live_host DLL exports\n";
        return false;
    }
    return true;
}

// --- Timing ---

static double get_time_sec() {
#ifdef _WIN32
    static LARGE_INTEGER freq = {};
    if (!freq.QuadPart) QueryPerformanceFrequency(&freq);
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return double(now.QuadPart) / double(freq.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return double(ts.tv_sec) + double(ts.tv_nsec) * 1e-9;
#endif
}

// --- GC ---

static void maybe_collect_gc(Context * ctx) {
    // collectHeap with stringHeap=true marks both heaps.
    // Check string heap first (fills faster, lower threshold).
    auto sUsed = ctx->stringHeap->bytesAllocated();
    auto sTotal = ctx->stringHeap->totalAlignedMemoryAllocated();
    if (sTotal > 0 && sUsed * 3 < sTotal * 2) {  // >1/3 unused
        ctx->collectHeap(nullptr, /*stringHeap*/true, /*validate*/false);
        return;
    }
    // Regular heap — higher threshold
    auto hUsed = ctx->heap->bytesAllocated();
    auto hTotal = ctx->heap->totalAlignedMemoryAllocated();
    if (hTotal > 0 && hUsed * 3 < hTotal) {  // >2/3 unused
        ctx->collectHeap(nullptr, /*stringHeap*/true, /*validate*/false);
    }
}

// --- Agent ticking ---

static void auto_tick_agents() {
    tickDebugAgent();
}

// --- Compilation ---

struct CompileResult {
    ProgramPtr program;
    ContextPtr ctx;
    FileAccessPtr access;
    ModuleGroup moduleGroup;
    string errors;  // non-empty if compile/simulate failed
};

static CompileResult compile_script(const string & fn) {
    CompileResult result;
    result.access = get_file_access((char*)(projectFile.empty() ? nullptr : projectFile.c_str()));

    CodeOfPolicies policies;
    policies.version_2_syntax = version2syntax;
    policies.fail_on_no_aot = false;
    policies.fail_on_lack_of_aot_export = false;
    // Live host policies
    policies.persistent_heap = true;
    policies.threadlock_context = true;
    policies.ignore_shared_modules = true;
    policies.rtti = true;

    result.program = compileDaScript(fn, result.access, tout, result.moduleGroup, policies);
    if (!result.program) {
        result.errors = "failed to compile " + fn;
        tout << "ERROR: " << result.errors << "\n";
        return result;
    }
    if (result.program->failed()) {
        TextWriter tw;
        for (auto & err : result.program->errors) {
            auto report = reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
            tout << report;
            tw << report;
        }
        result.errors = tw.str();
        result.program.reset();
        return result;
    }

    result.ctx = SimulateWithErrReport(result.program, tout);
    if (!result.ctx) {
        result.errors = "simulation failed for " + fn;
        tout << "ERROR: " << result.errors << "\n";
        result.program.reset();
    }
    return result;
}

// --- Find a void() function by name ---

static SimFunction * find_void_function(Context * ctx, const char * name) {
    auto fnVec = ctx->findFunctions(name);
    for (auto fn : fnVec) {
        if (fn->debugInfo && fn->debugInfo->count == 0) {
            return fn;
        }
    }
    return nullptr;
}

// --- Cached annotated function lists ---

struct AnnotatedFunctions {
    vector<SimFunction *> before_reload;
    vector<SimFunction *> after_reload;
    vector<SimFunction *> before_update;

    void build(Context * ctx) {
        before_reload.clear();
        after_reload.clear();
        before_update.clear();
        int32_t total = ctx->getTotalFunctions();
        for (int32_t i = 0; i < total; i++) {
            auto fn = ctx->getFunction(i);
            if (!fn || !fn->name) continue;
            if (fn->debugInfo && fn->debugInfo->count == 0) {
                if (strncmp(fn->name, "__before_reload_", 16) == 0) {
                    before_reload.push_back(fn);
                } else if (strncmp(fn->name, "__after_reload_", 15) == 0) {
                    after_reload.push_back(fn);
                } else if (strncmp(fn->name, "__before_update_", 16) == 0) {
                    before_update.push_back(fn);
                }
            }
        }
    }
};

static AnnotatedFunctions g_annotated;

static bool call_annotated_list(Context * ctx, const vector<SimFunction *> & fns) {
    for (auto fn : fns) {
        ctx->evalWithCatch(fn, nullptr);
        if (auto ex = ctx->getException()) {
            tout << "EXCEPTION in " << fn->name << ": " << ex << " at " << ctx->exceptionAt.describe() << "\n";
            return false;
        }
    }
    return true;
}

// --- Set watched files ---

static void set_watched_files(Context * ctx) {
    if (!dll_set_watched_files) return;
    auto allFiles = ctx->getAllFiles();
    vector<string> names;
    names.reserve(allFiles.size());
    for (auto * fi : allFiles) {
        if (fi && !fi->name.empty()) {
            struct stat st;
            if (stat(fi->name.c_str(), &st) == 0) {
                names.push_back(fi->name);
            }
        }
    }
    vector<const char *> ptrs;
    ptrs.reserve(names.size());
    for (auto & n : names) {
        ptrs.push_back(n.c_str());
    }
    if (!ptrs.empty()) {
        dll_set_watched_files(ptrs.data(), int(ptrs.size()));
    }
}

// --- Main lifecycle loop ---

static int run_lifecycle(const string & fn) {
    auto cr = compile_script(fn);

    Context * ctx = cr.ctx ? cr.ctx.get() : nullptr;
    SimFunction * fnInit = nullptr;
    SimFunction * fnUpdate = nullptr;
    SimFunction * fnShutdown = nullptr;
    bool ctx_had_exception = false;

    if (!ctx) {
        // Initial compile failed — enter main loop with no context, paused,
        // so the REST API can report the error and accept reload requests
        tout << "daslang-live: initial compile FAILED, waiting for reload\n";
        if (dll_set_last_error) dll_set_last_error(cr.errors.c_str());
        if (dll_set_paused) dll_set_paused(true);
    } else {
        // Detect mode: init exists → lifecycle, otherwise → main
        fnInit = find_void_function(ctx, "init");
        fnUpdate = find_void_function(ctx, "update");
        fnShutdown = find_void_function(ctx, "shutdown");
        auto * fnMain = find_void_function(ctx, "main");

        if (!fnInit && fnMain) {
            // Simple mode: just call main() like daslang.exe
            tout << "daslang-live: running main()\n";
            ctx->restart();
            ctx->evalWithCatch(fnMain, nullptr);
            if (auto ex = ctx->getException()) {
                tout << "EXCEPTION: " << ex << " at " << ctx->exceptionAt.describe() << "\n";
                return 1;
            }
            return 0;
        }

        if (!fnInit) {
            tout << "ERROR: script must export either init() or main()\n";
            return 1;
        }

        // Lifecycle mode
        tout << "daslang-live: lifecycle mode (init/update/shutdown)\n";

        if (dll_set_is_reload) dll_set_is_reload(false);
        if (dll_set_dispatch_context) dll_set_dispatch_context(ctx);
        set_watched_files(ctx);
        g_annotated.build(ctx);
        ctx->restart();

        // Call init()
        ctx->evalWithCatch(fnInit, nullptr);
        if (auto ex = ctx->getException()) {
            string msg = string("EXCEPTION in init(): ") + ex + " at " + ctx->exceptionAt.describe();
            tout << msg << "\n";
            if (dll_set_last_error) dll_set_last_error(msg.c_str());
            if (dll_set_paused) dll_set_paused(true);
            if (dll_clear_store) dll_clear_store();
            ctx_had_exception = true;
        }
    }

    double lastTime = get_time_sec();
    double startTime = lastTime;
    int frameCount = 0;
    double fpsTimer = lastTime;

    // Main loop
    while (!(dll_exit_requested && dll_exit_requested())) {
        double now = get_time_sec();
        float dt = float(now - lastTime);
        float uptime = float(now - startTime);
        lastTime = now;

        if (dll_set_dt) dll_set_dt(dt);
        if (dll_set_uptime) dll_set_uptime(uptime);

        // FPS calculation
        frameCount++;
        if (now - fpsTimer >= 1.0) {
            float fps = float(frameCount) / float(now - fpsTimer);
            if (dll_set_fps) dll_set_fps(fps);
            frameCount = 0;
            fpsTimer = now;
        }

        // Before-update hooks (lockbox dispatch, etc.)
        // Skip if context had an exception — it's kept alive only for shutdown()
        if (ctx && !ctx_had_exception) {
            call_annotated_list(ctx, g_annotated.before_update);
            // Check if a command exception killed the context during before_update
            if (dll_is_context_dead && dll_is_context_dead()) {
                ctx_had_exception = true;
                if (dll_clear_store) dll_clear_store();
            }
        }

        // Update (unless paused, exception, or no context)
        bool paused = dll_is_paused ? dll_is_paused() : false;
        if (!paused && ctx && !ctx_had_exception && fnUpdate) {
            ctx->evalWithCatch(fnUpdate, nullptr);
            if (auto ex = ctx->getException()) {
                string msg = string("EXCEPTION in update(): ") + ex + " at " + ctx->exceptionAt.describe();
                tout << msg << "\n";
                if (dll_set_last_error) dll_set_last_error(msg.c_str());
                if (dll_set_paused) dll_set_paused(true);
                if (dll_clear_store) dll_clear_store();
                ctx_had_exception = true;
            }
        }
        // Avoid busy-spinning when there's no context or paused
        if (!ctx || paused || ctx_had_exception) {
            builtin_sleep(16);
        }

        // GC
        if (ctx && !ctx_had_exception) maybe_collect_gc(ctx);

        // Auto-tick debug agents
        auto_tick_agents();

        // Check for reload
        bool needsReload = (dll_reload_requested && dll_reload_requested())
                        || (dll_files_changed && dll_files_changed());
        if (needsReload) {
            tout << "daslang-live: reloading...\n";

            // Set reload flag BEFORE shutdown so is_reload() returns true
            if (dll_set_is_reload) dll_set_is_reload(true);

            // Call [before_reload] functions and shutdown (skip if no context, e.g. initial compile failed)
            // After exception: skip before_reload (state is corrupted) but still call shutdown
            // so resources (audio, GL) are properly released before context is destroyed.
            if (ctx) {
                if (ctx_had_exception) {
                    // Clear stale exception state so before_reload/shutdown can run
                    ctx->restart();
                }
                call_annotated_list(ctx, g_annotated.before_reload);
                if (fnShutdown) {
                    ctx->evalWithCatch(fnShutdown, nullptr);
                    if (auto ex = ctx->getException()) {
                        tout << "EXCEPTION in shutdown() during reload: " << ex << "\n";
                    }
                }
            }

            // Recompile
            auto newCr = compile_script(fn);
            if (!newCr.ctx) {
                tout << "daslang-live: reload FAILED, keeping old context (paused)\n";
                if (dll_set_last_error) dll_set_last_error(newCr.errors.c_str());
                if (dll_set_paused) dll_set_paused(true);
                if (dll_clear_reload_flags) dll_clear_reload_flags();
                // Re-init old context if we have one (shutdown was already called)
                if (ctx && !ctx_had_exception) {
                    if (dll_set_is_reload) dll_set_is_reload(true);
                    ctx->restart();
                    call_annotated_list(ctx, g_annotated.after_reload);
                    if (fnInit) {
                        ctx->evalWithCatch(fnInit, nullptr);
                    }
                }
                continue;
            }

            // Swap to new context
            cr = das::move(newCr);
            ctx = cr.ctx.get();

            // Re-find functions
            fnInit = find_void_function(ctx, "init");
            fnUpdate = find_void_function(ctx, "update");
            fnShutdown = find_void_function(ctx, "shutdown");

            if (!fnInit || !fnUpdate) {
                tout << "ERROR: reloaded script missing init() or update()\n";
                if (dll_set_paused) dll_set_paused(true);
                if (dll_clear_reload_flags) dll_clear_reload_flags();
                continue;
            }

            // Reset state
            bool isFullReload = dll_full_reload && dll_full_reload();
            if (dll_set_dispatch_context) dll_set_dispatch_context(ctx);
            set_watched_files(ctx);
            g_annotated.build(ctx);
            if (dll_set_is_reload) dll_set_is_reload(true);
            if (dll_set_paused) dll_set_paused(false);
            if (dll_clear_reload_flags) dll_clear_reload_flags();
            if (dll_clear_error) dll_clear_error();
            if (dll_clear_context_dead) dll_clear_context_dead();
            ctx_had_exception = false;

            // Full reload: clear @live var entries so they reset to code defaults
            if (isFullReload && dll_clear_live_vars) {
                tout << "daslang-live: full reload — resetting @live variables\n";
                dll_clear_live_vars();
            }

            // Restart context and restore state before init
            ctx->restart();

            // Call [after_reload] functions BEFORE init — restores persistent
            // state (DECS, globals) so init() can use it immediately.
            // On full reload the store is empty, so these are no-ops.
            if (!call_annotated_list(ctx, g_annotated.after_reload)) {
                string msg = "EXCEPTION during [after_reload], clearing store";
                tout << msg << "\n";
                if (dll_set_last_error) dll_set_last_error(msg.c_str());
                if (dll_set_paused) dll_set_paused(true);
                if (dll_clear_store) dll_clear_store();
                ctx_had_exception = true;
                continue;
            }

            // Call init() in new context
            ctx->evalWithCatch(fnInit, nullptr);
            if (auto ex = ctx->getException()) {
                string msg = string("EXCEPTION in init() after reload: ") + ex + " at " + ctx->exceptionAt.describe();
                tout << msg << "\n";
                if (dll_set_last_error) dll_set_last_error(msg.c_str());
                if (dll_set_paused) dll_set_paused(true);
                if (dll_clear_store) dll_clear_store();
                ctx_had_exception = true;
            }

            tout << "daslang-live: reload complete\n";
        }
    }

    // Shutdown
    if (ctx && fnShutdown) {
        ctx->evalWithCatch(fnShutdown, nullptr);
        if (auto ex = ctx->getException()) {
            tout << "EXCEPTION in shutdown(): " << ex << "\n";
        }
    }

    return 0;
}

// --- Arg parsing ---

static void print_help() {
    tout << "daslang-live — live-reloading application host for daScript\n";
    tout << "Usage: daslang-live [options] <script.das> [-- script arguments]\n";
    tout << "  -project <file>   — project file (.das_project)\n";
    tout << "  -dasroot <path>   — override DAS_ROOT\n";
    tout << "  -cwd              — change working directory to script's folder\n";
    tout << "  -v1syntax         — use v1 syntax (default: v2)\n";
    tout << "  --no-dyn-modules  — skip loading dynamic modules\n";
    tout << "  --                — separator for script arguments\n";
    tout << "  -h, --help        — this help\n";
}

// --- Single instance ---

#ifdef _WIN32
static HANDLE g_singleInstanceMutex = nullptr;
#else
#include <sys/file.h>
#include <unistd.h>
static int g_lockFd = -1;
#endif

static bool acquire_single_instance() {
#ifdef _WIN32
    g_singleInstanceMutex = CreateMutexA(nullptr, TRUE, "daslang-live-single-instance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (g_singleInstanceMutex) {
            CloseHandle(g_singleInstanceMutex);
            g_singleInstanceMutex = nullptr;
        }
        return false;
    }
    return g_singleInstanceMutex != nullptr;
#else
    const char * lockPath = "/tmp/daslang-live.lock";
    g_lockFd = open(lockPath, O_CREAT | O_RDWR, 0600);
    if (g_lockFd < 0) return true;  // can't create lock file — allow running
    if (flock(g_lockFd, LOCK_EX | LOCK_NB) != 0) {
        close(g_lockFd);
        g_lockFd = -1;
        return false;
    }
    return true;
#endif
}

static void release_single_instance() {
#ifdef _WIN32
    if (g_singleInstanceMutex) {
        ReleaseMutex(g_singleInstanceMutex);
        CloseHandle(g_singleInstanceMutex);
        g_singleInstanceMutex = nullptr;
    }
#else
    if (g_lockFd >= 0) {
        flock(g_lockFd, LOCK_UN);
        close(g_lockFd);
        g_lockFd = -1;
    }
#endif
}

// --- Entry point ---

int main(int argc, char * argv[]) {
#if defined(_WIN32) && defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    install_das_crash_handler();

    string scriptFile;
    bool noDynamicModules = false;
    bool changeCwd = false;

    // Parse args
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-project" && i + 1 < argc) {
            projectFile = argv[++i];
        } else if (arg == "-dasroot" && i + 1 < argc) {
            setDasRoot(argv[++i]);
        } else if (arg == "-cwd") {
            changeCwd = true;
        } else if (arg == "-v1syntax") {
            version2syntax = false;
        } else if (arg == "--no-dyn-modules") {
            noDynamicModules = true;
        } else if (arg == "-h" || arg == "--help") {
            print_help();
            return 0;
        } else if (arg == "--") {
            break;  // Rest is for the script
        } else if (arg[0] != '-') {
            scriptFile = arg;
        } else {
            tout << "unknown option: " << arg << "\n";
            print_help();
            return 1;
        }
    }

    if (scriptFile.empty()) {
        print_help();
        return 1;
    }

    // -cwd: change working directory to script's folder, strip path from scriptFile
    if (changeCwd && !scriptFile.empty()) {
        auto slash = scriptFile.find_last_of("\\/");
        if (slash != string::npos) {
            string dir = scriptFile.substr(0, slash);
            scriptFile = scriptFile.substr(slash + 1);
#ifdef _WIN32
            SetCurrentDirectoryA(dir.c_str());
#else
            chdir(dir.c_str());
#endif
        }
    }

    if (!acquire_single_instance()) {
        fprintf(stderr, "ERROR: another instance of daslang-live is already running\n");
        return 1;
    }

    // Register modules
    register_builtin_modules();
    require_project_specific_modules();

#if !defined(DAS_ENABLE_DLL) || !defined(DAS_ENABLE_DYN_INCLUDES)
    #include "modules/external_need.inc"
#endif

#ifdef DAS_ENABLE_DYN_INCLUDES
    if (!noDynamicModules) {
        daScriptEnvironment::ensure();
        if (project_root.empty() && !scriptFile.empty()) {
            auto slash = scriptFile.find_last_of("\\/");
            project_root = (slash != string::npos) ? scriptFile.substr(0, slash) : "./";
        }
        auto access = get_file_access((char*)(projectFile.empty() ? nullptr : projectFile.c_str()));
        require_dynamic_modules(access, getDasRoot(), project_root, tout);
    }
#endif
    Module::Initialize();

    // Load live_host DLL functions
    if (!load_live_host_functions()) {
        tout << "WARNING: live_host module not found — lifecycle functions will use defaults\n";
    }

    // Set live mode flag before compile — agents check this in [_macro]
    if (dll_set_live_mode) dll_set_live_mode(true);

    int result = run_lifecycle(scriptFile);

    Module::Shutdown();
    release_single_instance();
    return result;
}
