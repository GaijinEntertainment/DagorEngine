#include "daScript/ast/aot_templates.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/dyn_modules.h"
#include "daScript/daScript.h"
#include "daScript/daScriptModule.h"
#include "daScript/misc/das_common.h"
#include "daScript/misc/das_asan.h"
#include "daScript/simulate/fs_file_info.h"
#include "daScript/ast/ast_aot_cpp.h"
#include "daScript/ast/ast_serializer.h"
#include "daScript/misc/crash_handler.h"
#include "daScript/misc/dep_recorder.h"
#include "daScript/misc/job_que.h"
#include "daScript/misc/performance_time.h"
#ifdef __APPLE__
#include <pthread/qos.h>
#endif
#if defined(_WIN32) && defined(_DEBUG)
#include <crtdbg.h>
#endif

// dagor: no external (daspkg-generated) modules, so no external_declare.inc

using namespace das;

void use_utf8();

void require_project_specific_modules();//link time resolved dependencies
das::FileAccessPtr get_file_access( char * pak );//link time resolved dependencies

TextPrinter tout;

static string projectFile;
static string depFilePath;      // -MF; empty with -MD means DAS_DEPFILE names it
static string depFileTarget;    // -MT
static bool depFileArmed = false;
// aot config
static bool aotMacros = false;
static bool isAotLib = false;
static string aotResult = "";
// aot config end
static bool paranoid_validation = false;
static bool profilerRequired = false;
static bool debuggerRequired = false;
static bool astVerifyRequired = false;
static bool scopedStackAllocator = true;
static bool pauseAfterErrors = false;
static bool quiet = false;

#if DAS_ASAN
extern "C" const char * __asan_default_options() {
    return "quarantine_size_mb=32";
}
#endif

#ifndef MAIN_FUNC_NAME
  #define MAIN_FUNC_NAME main
  // the process ends when this returns: the fusion table is orphaned, not torn down - unless
  // the build tracks allocations or runs under a leak sanitizer, whose exit audit must see it
  // freed, or the browser host, which calls main once per run
  #ifndef __has_feature
    #define __has_feature(x) 0
  #endif
  #if DAS_TRACK_ALLOC || defined(__EMSCRIPTEN__) || defined(__SANITIZE_ADDRESS__) || __has_feature(address_sanitizer)
    #define DAS_ORPHAN_FUSION_AT_EXIT false
  #else
    #define DAS_ORPHAN_FUSION_AT_EXIT true
  #endif
#else
  // a host's entry point: the host may initialize again, so shutdown resets everything
  #define DAS_ORPHAN_FUSION_AT_EXIT false
#endif
enum class JitMode {
    None,
    Direct,
    Dll,
    Executable,
    Library,
};

static bool gc_thread_root_is_settled ( TextWriter & tout, const char * what, const char * when ) {
    auto & root = gc_root::gc_get_thread_root();
    if ( root.gc_count == 0 ) return true;
    tout << what << ": " << uint64_t(root.gc_count) << " gc_node(s) " << when << "\n";
    root.gc_report();
    return false;
}
static JitMode jitEnabled = JitMode::None; // Disabled by default.
static bool jitNoCache = false; // -jit-no-cache: bypass DLL-cache path, run in-memory.
static bool jitStack = false; // -jit-stack: retain every generated call in the logical das stack.
static string jitOutPath = ""; // Empty, JIT module will choose default.
static bool libExportAll = false;
static bool libNeedsOutput = false;
static string serFile = "";   // -ser <path>: write the AST module cache (env serializer rail) after compile
static string deserFile = ""; // -deser <path>: read the AST module cache during compile instead of parsing
static string moduleCacheFile = ""; // -module-cache <path>: both - read when present, refresh when the compile diverged
static bool moduleCacheExplicit = false; // -module-cache given
static bool noModuleCache = false;  // -no-module-cache: off, over -module-cache and the default alike
static bool noOptimization = false; // -no-optimization: `options optimize = false` for the whole program
static string hostBinary = "";      // argv[0]
static string hostOptions = "";     // argv up to "--": the compile's own options key the default cache

static bool noDynamicModules = false;
static bool noLint = false;
#ifdef DAS_TEST_AOT
static bool useAot = true;
#else
static bool useAot = false;
#endif

static bool version2syntax = true;
static bool gen2MakeSyntax = false;
static bool trackAllocations = false;
static bool heapReportAtExit = false;
static bool logModuleCompileTime = false;
static int64_t startupCompileUsec = 0;
static int64_t startupSimulateUsec = 0;
static int64_t startupRunUsec = 0;
static bool buildingDocumentation = false;

static vector<string> dllSearchPaths;

static CodeOfPolicies getPolicies() {
    CodeOfPolicies policies;
    policies.dll_search_paths = dllSearchPaths;
    policies.aot = false;
    policies.aot_module = true;
    policies.tune_frozen = true;    // -aot output is a cross-box artifact — no per-box [tune] stamps
    if (aotMacros) {
        policies.aot_macros = true;
        policies.export_all = true; // need it for aot to export macros
        policies.stack = 1 * 1024 * 1024; // For now, we need huge stack to aot macros
    }
    policies.fail_on_lack_of_aot_export = true;
    policies.version_2_syntax = version2syntax;
    policies.gen2_make_syntax = gen2MakeSyntax;
    policies.scoped_stack_allocator = scopedStackAllocator;
    policies.track_allocations = trackAllocations;
    policies.no_lint = noLint;
    policies.no_optimizations = noOptimization;
    policies.log_module_compile_time = logModuleCompileTime;
    policies.building_documentation = buildingDocumentation;
    return policies;
}

bool aot_compile ( vector<pair<string, string>> &aot_files, bool dryRun, bool cross_platform ) {
    // call daslib/aot_cpp `aot()` from C++
    auto access = get_file_access((char*)(projectFile.empty() ? nullptr : projectFile.c_str()));
    ModuleGroup dummyGroup;
    CodeOfPolicies stubPolicies;
    stubPolicies.version_2_syntax = true;
    stubPolicies.aot_module = true;
    stubPolicies.tune_frozen = true;
    string aotCppPath = getDasRoot() + "/daslib/aot_cpp.das";
    auto program = compileDaScript(aotCppPath, access, tout, dummyGroup, stubPolicies);
    if ( !program || program->failed() ) {
        tout << "failed to compile daslib/aot_cpp.das\n";
        if ( program ) for ( auto & err : program->errors ) {
            tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
        }
        return false;
    }
    auto pctx = SimulateWithErrReport(program, tout);
    if ( !pctx ) return false;
    bool isUnique = false;
    auto aotFn = pctx->findFunction("aot", isUnique);
    if ( !aotFn ) {
        tout << "daslib aot() not found\n";
        return false;
    }
    CodeOfPolicies cop = getPolicies();
    cop.aot = false;
    cop.aot_lib = isAotLib;
    cop.ignore_shared_modules = false;
    bool compiled = true;
    for (const auto &[in, out] : aot_files) {
        // Use `or` here, to return `true` if at least one file compiled successfully.
        string inputStr = in;
        vec4f args[4];
        args[0] = cast<char*>::from((char*)inputStr.c_str());
        args[1] = cast<bool>::from(paranoid_validation);
        args[2] = cast<bool>::from(cross_platform);
        args[3] = cast<CodeOfPolicies*>::from(&cop);
        pctx->restart();
        vec4f ret = pctx->evalWithCatch(aotFn, args);
        if ( auto ex = pctx->getException() ) {
            tout << "aot exception: " << ex << " at " << pctx->exceptionAt.describe() << "\n";
            return false;
        }
        const char * resultStr = cast<char*>::to(ret);
        if ( dryRun ) {
            tout << "dry run success, no changes will be written\n";
        } else if ( !resultStr || !resultStr[0] ) {
            tout << "aot returned empty result for " << in << "\n";
            compiled = false;
        } else {
            bool is_ok = saveToFile(tout, out, resultStr, quiet);
            if (!is_ok && !quiet) {
                tout << "Failed to compile `" << out << "` in aot.\n";
            }
            compiled &= is_ok;
        }
    }
    return compiled;
}

int das_aot_main ( int argc, char * argv[] ) {
    setCommandLineArguments(argc, argv);
    #ifdef _MSC_VER
    _CrtSetReportMode(_CRT_ASSERT, 0);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    #endif
    if ( argc<=3 ) {
        tout << "daslang -aot <in_script.das> <out_script.das.cpp> [-v2Syntax] [-v1Syntax] [-v2makeSyntax] [-project <project file>] [-dasroot <dasroot folder>] [-q] [-j] [-aot-macros] [-cross-platform] [-no-lint] [-log-compile-time]\n";
        return -1;
    }
    bool dryRun = false;
    bool cross_platform = false; // strcmp("-aotlib", argv[1]) == 0;
    bool scriptArgs = false;
    vector<pair<string, string>> aot_files;
    string project_root;
    vector<string> load_modules;
    if ( argc>3  ) {
        for (int ai = 1; ai != argc; ++ai) {
            if ( strcmp(argv[ai],"-q")==0 ) {
                quiet = true;
            } else if ( strcmp(argv[ai],"-aot")==0 || strcmp(argv[ai],"-aotlib")==0 ) {
                if (ai + 2 >= argc) {
                    tout << "-aot requires 2 arguments <in> <out>.";
                    return -1;
                }
                aot_files.emplace_back(argv[ai + 1], argv[ai + 2]);
                ai += 2;
            } else if ( strcmp(argv[ai],"-p")==0 ) {
                paranoid_validation = true;
            } else if ( strcmp(argv[ai],"-dry-run")==0 ) {
                dryRun = true;
            } else if ( strcmp(argv[ai],"-cross-platform")==0 ) {
                cross_platform = true;
            } else if ( strcmp(argv[ai],"-aot-macros")==0 ) {
                aotMacros = true;
            } else if ( strcmp(argv[ai],"-project")==0 ) {
                if ( ai+1 >= argc ) {
                    tout << "das-project requires argument";
                    return -1;
                }
                projectFile = argv[ai+1];
                ai += 1;
            } else if ( strcmp(argv[ai],"-dll-path")==0 ) {
                if ( ai+1 >= argc ) {
                    tout << "dll-path requires argument";
                    return -1;
                }
                dllSearchPaths.push_back(argv[ai+1]);
                ai += 1;
            } else if ( strcmp(argv[ai],"-dasroot")==0 ) {
                if ( ai+1 >= argc ) {
                    tout << "dasroot requires argument";
                    return -1;
                }
                setDasRoot(argv[ai+1]);
                ai += 1;
            } else if ( strcmp(argv[ai],"-project-root")==0 || strcmp(argv[ai],"-project_root")==0 ) {
                project_root = argv[ai + 1];
                ai++;
            } else if ( strcmp(argv[ai],"-load-module")==0 || strcmp(argv[ai],"-load_module")==0 ) {
                if ( ai+1 >= argc ) {
                    tout << "-load_module requires path argument";
                    return -1;
                }
                load_modules.push_back(argv[ai + 1]);
                ai++;
            } else if ( strcmp(argv[ai],"-v2syntax")==0 ) {
                version2syntax = true;
            } else if ( strcmp(argv[ai],"-v1syntax")==0 ) {
                version2syntax = false;
            } else if ( strcmp(argv[ai],"-v2makeSyntax")==0 ) {
                version2syntax = false;
                gen2MakeSyntax = true;
            } else if ( strcmp(argv[ai],"-no-dynamic-modules")==0 ) {
                noDynamicModules = true;
            } else if ( strcmp(argv[ai],"-ignore-manifest")==0 ) {
                ignore_dynamic_module_manifests(true);
            } else if ( strcmp(argv[ai],"-no-lint")==0 ) {
                noLint = true;
            } else if ( strcmp(argv[ai],"-log-compile-time")==0 ) {
                logModuleCompileTime = true;
            } else if ( strcmp(argv[ai],"--")==0 ) {
                scriptArgs = true;
            } else if ( !scriptArgs ) {
                tout << "unsupported option " << argv[ai];
                return -1;
            }
        }
    }
    // register all builtin modules
    register_builtin_modules();
    require_project_specific_modules();
    // dagor: no external (daspkg-generated) static modules to pull
    #ifdef DAS_ENABLE_DYN_INCLUDES
    if ( !noDynamicModules ) {
        daScriptEnvironment::ensure();
        auto access = get_file_access((char*)(projectFile.empty() ? nullptr : projectFile.c_str()));
        require_dynamic_modules(access, getDasRoot(), project_root, load_modules, tout);
    }
    #endif
    Module::Initialize();
    daScriptEnvironment::getBound()->g_isInAot = true;
    bool compiled = true;
    compiled = aot_compile(aot_files, dryRun, cross_platform);
    Module::Shutdown(true, !DAS_ORPHAN_FUSION_AT_EXIT);
    return compiled ? 0 : -1;
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

namespace das {
    // link-time resolved: src/builtin/module_builtin_jobque.cpp — bounded drain +
    // teardown of the global job que; false = jobs did not drain, LEAK the context
    bool shutdown_job_que_bounded ( int timeoutMs );
}

// Browser 3-call lifecycle: a wasm page cannot block in main()'s while(true) — it
// must yield to the browser each frame. So a program that exposes `update` is run
// as a browser main-loop instead of single-shot main(): init() once, then update()
// per frame via emscripten_set_main_loop, shutdown() when the loop ends. Desktop
// runs main() (init();while(!exit){update()};shutdown()) — one homogeneous .das.
namespace {
    // emscripten_set_main_loop(...,false) does NOT unwind the C++ stack, so
    // compile_and_run returns and its stack ContextPtr (a shared_ptr) would drop
    // to zero refs and destroy the Context — the per-frame update() would then
    // deref a dead context. We copy the shared_ptr into this heap struct so it
    // outlives that return; stop_browser_loop() frees it when the loop ends
    // (update() stops, or the next run supersedes it).
    struct WebLoop {
        ContextPtr      ctx;
        SimFunction *   updateFn = nullptr;
        SimFunction *   shutdownFn = nullptr;
    };

    // The single void/bool/int callable overload of `name`, or nullptr (none/ambiguous).
    SimFunction * pick_lifecycle_fn ( Context * ctx, const char * name, ModuleGroup & mg ) {
        SimFunction * found = nullptr;
        for ( auto fnAS : ctx->findFunctions(name) ) {
            if ( verifyCall<void>(fnAS->debugInfo, mg)
              || verifyCall<bool>(fnAS->debugInfo, mg)
              || verifyCall<int32_t>(fnAS->debugInfo, mg) ) {
                if ( found ) return nullptr;    // ambiguous overload set
                found = fnAS;
            }
        }
        return found;
    }

    // The single active browser loop. Emscripten supports ONE main loop and ONE
    // GLFW window at a time, so a new run must tear the previous one down first.
    WebLoop * g_activeWebLoop = nullptr;

    // Mirror of main()'s dumpLeaks (-no-dump-leaks), so the post-shutdown leak check
    // in stop_browser_loop respects the same flag as the end-of-callMain dump.
    bool g_webloop_dump_leaks = true;

    // main() returns while the loop still runs the program — Module::Shutdown
    // there destroys live state (~Module_JobQue killed init()'s persistent que:
    // the playground wedge). main() sets this; the loop's natural end consumes
    // it. The superseded path leaves modules alive — the next program needs them.
    bool g_webloop_defer_module_shutdown = false;

    // Stop the active loop: cancel it, run the script's shutdown() (destroys the
    // GLFW window — the next glfwCreateWindow aborts otherwise), drain the jobs,
    // free the Context. Idempotent; best-effort. False = que would not drain:
    // the Context is deliberately LEAKED (jobs still reference its memory) and
    // the caller must skip anything that joins the workers.
    bool stop_browser_loop () {
        if ( !g_activeWebLoop ) return true;
        auto loop = g_activeWebLoop;
        g_activeWebLoop = nullptr;
        emscripten_cancel_main_loop();
        if ( loop->shutdownFn ) {
            loop->ctx->evalWithCatch(loop->shutdownFn, nullptr);
            if ( auto ex = loop->ctx->getException() ) {
                // best-effort continues, but not silently — the throw also skipped
                // the rest of the program's shutdown()
                tout << "EXCEPTION in shutdown(): " << ex << " at " << loop->ctx->exceptionAt.describe() << "\n";
            }
        }
        // drain BEFORE the Context dies — fork contexts share its memory; bounded
        if ( !das::shutdown_job_que_bounded(3000) ) {
            tout << "job que did not drain in 3s — leaking the program's context rather than freeing memory under running jobs\n";
            return false;
        }
        delete loop;   // drops the Context shared_ptr -> Context + its objects freed
        // Real leak check for browser-loop programs: now that the program has ended
        // and shutdown() ran, report any JobStatus/Channel/LockBox the program failed
        // to free. (The end-of-callMain dump in main() is skipped while a loop is live
        // — at that point the program is still running and its objects are in use.)
        // Honors -no-dump-leaks via g_webloop_dump_leaks.
        if ( g_webloop_dump_leaks ) {
            if ( uint64_t n = JobStatus::CountJobQueLeaks() ) {
                tout << "JobQue leak after browser-loop shutdown: " << n << "\n";
                JobStatus::DumpJobQueLeaks();
            }
        }
        return true;
    }

    void web_loop_tick ( void * arg ) {
        auto loop = (WebLoop *) arg;
        vec4f res = loop->ctx->evalWithCatch(loop->updateFn, nullptr);
        bool keepGoing = true;
        if ( auto ex = loop->ctx->getException() ) {
            tout << "EXCEPTION: " << ex << " at " << loop->ctx->exceptionAt.describe() << "\n";
            keepGoing = false;
        } else if ( loop->updateFn->debugInfo && loop->updateFn->debugInfo->result ) {
            auto rt = loop->updateFn->debugInfo->result->type;
            if ( rt == Type::tBool ) {
                keepGoing = cast<bool>::to(res);            // bool update(): false stops the loop
            } else if ( rt == Type::tInt ) {
                keepGoing = cast<int32_t>::to(res) != 0;    // int update(): 0 stops the loop
            }
        }
        // void update(): runs until the page closes or the next run stops it.
        if ( keepGoing ) loop->ctx->collectHeapIfMostlyFree();
        if ( !keepGoing ) {
            bool drained = stop_browser_loop();
            // the deferred Module::Shutdown runs only on a drained que —
            // ~Module_JobQue joins workers unbounded; undrained = leak modules
            // alongside the already-leaked context (one program per frame)
            if ( g_webloop_defer_module_shutdown ) {
                g_webloop_defer_module_shutdown = false;
                if ( drained ) Module::Shutdown(g_webloop_dump_leaks, !DAS_ORPHAN_FUSION_AT_EXIT);
            }
        }
    }

    // True ⇒ the program was launched as a browser loop (Context persisted, main
    // loop installed, or init() threw and was reported). False ⇒ run single-shot main().
    bool start_browser_loop ( ContextPtr & pctx, ModuleGroup & mg ) {
        auto updateFn = pick_lifecycle_fn(pctx.get(), "update", mg);
        if ( !updateFn ) return false;
        auto initFn = pick_lifecycle_fn(pctx.get(), "init", mg);
        auto shutdownFn = pick_lifecycle_fn(pctx.get(), "shutdown", mg);
        pctx->restart();
        if ( initFn ) {
            pctx->evalWithCatch(initFn, nullptr);
            if ( auto ex = pctx->getException() ) {
                tout << "EXCEPTION in init(): " << ex << " at " << pctx->exceptionAt.describe() << "\n";
                return true;    // reported; do not fall back to main(), do not start the loop
            }
        }
        auto loop = new WebLoop();      // lives until the next run stops it (stop_browser_loop)
        loop->ctx = pctx;               // shared_ptr copy keeps the Context alive past return
        loop->updateFn = updateFn;
        loop->shutdownFn = shutdownFn;
        g_activeWebLoop = loop;
        emscripten_set_main_loop_arg(web_loop_tick, loop, 0, false);    // 0 = rAF; false = no unwind
        return true;
    }
}
#endif

// returns process exit code:
//   0 on success
//   non-zero from int main, or 1 on compile/simulate/verify/exception failure
int compile_and_run ( const string & fn, const string & mainFnName, bool outputProgramCode, bool dryRun, bool compileOnly, const char * introFile = nullptr ) {
    // Heap-leak tracker: arm the tail-memoize landmark so per-allocation stack
    // captures below this frame skip re-walking ancestors. No-op when
    // DAS_TRACK_ALLOC is off or on non-Win64.
    das::AllocTrackingLandmark _alloc_tracker_landmark;
#ifdef __EMSCRIPTEN__
    // A previous graphics program may have installed a browser loop that is still
    // running (and still owns the single GLFW window). Tear it down before running
    // a new program, else its glfwCreateWindow aborts. No-op when none is active.
    stop_browser_loop();
#endif
    auto access = get_file_access((char*)(projectFile.empty() ? nullptr : projectFile.c_str()));
    if ( introFile ) {
        auto fileInfo = make_unique<TextFileInfo>(introFile, uint32_t(strlen(introFile)), false);
        access->setFileInfo("____intro____", das::move(fileInfo));
    }
    int exitCode = 1;
    // the cache owns deserialized-AST FileInfos (LineInfos point at them) - it must outlive
    // dummyGroup, which owns the deserialized Modules, so it is declared first
    ModuleFileCache moduleCache;
    ModuleGroup dummyGroup;
    CodeOfPolicies policies;
    policies.dll_search_paths = dllSearchPaths;
    if ( debuggerRequired ) {
        policies.debugger = true;
        access->addExtraModule("debug", getDasRoot() + "/daslib/debug.das");
    } else if ( profilerRequired ) {
        policies.profiler = true;
        access->addExtraModule("profiler", getDasRoot() + "/daslib/profiler.das");
    } /*else*/ if ( jitEnabled != JitMode::None ) {
        policies.jit_enabled = true;
        switch (jitEnabled) {
            case JitMode::Executable: policies.jit_exe_mode = true; break;
            case JitMode::Library:
                policies.export_public_functions = libExportAll;
                break;
            case JitMode::Dll: policies.jit_dll_mode = true; break;
            case JitMode::Direct: break;
            default: break;
        }
        if ( jitNoCache ) policies.jit_dll_mode = false;
        policies.jit_emit_prologue = jitStack;
        access->addExtraModule("just_in_time", getDasRoot() + "/daslib/just_in_time.das");
        policies.jit_output_path = jitOutPath;
        policies.dll_search_paths.emplace_back(getDasRoot() + "/lib");
    }
    if ( astVerifyRequired ) {
        // force-include the AST verifier the same way -jit/-debugger pull in their
        // daslib support; its pre/post-infer macros then run over the program's modules
        // (--ast-verify-batch keeps only the post-infer one, read from argv by the module).
        access->addExtraModule("ast_verify", getDasRoot() + "/daslib/ast_verify.das");
    }
    // -use-aot links the stubs compiled into THIS binary against the script it runs; a function
    // with no stub interprets (never fail_on_no_aot here - the host may be dastest, whose driver
    // has no stubs even where its testing.das does, and which sets aot / fail_on_no_aot itself
    // for the test files it compiles)
    policies.aot = useAot;
    policies.fail_on_no_aot = false;
    if ( useAot ) {
        // the stubs were generated tune-frozen (a cross-box artifact carries no per-box [tune]
        // stamps); a stamped compile here would hash every tune-family kernel differently and
        // link none of them
        policies.tune_frozen = true;
    }
    policies.fail_on_lack_of_aot_export = false;
    policies.aot_macros = aotMacros;    // -aot-macros: force quote lowering (daslib/quote) in a normal run
    if ( aotMacros ) {
        policies.stack = 1 * 1024 * 1024;   // a lowered quote evaluates one large construction frame
    }
    policies.version_2_syntax = version2syntax;
    policies.gen2_make_syntax = gen2MakeSyntax;
    policies.scoped_stack_allocator = scopedStackAllocator;
    policies.track_allocations = trackAllocations;
    policies.no_lint = noLint;
    policies.no_optimizations = noOptimization;
    policies.log_module_compile_time = logModuleCompileTime;
    policies.building_documentation = buildingDocumentation;
    policies.persistent_heap = true;
    // The AST module-cache rail (env serializer): -ser captures every parsed module as the
    // compile runs, -deser feeds them back in the same order instead of parsing (loud
    // verdict below - the silent fallback-to-parse must not fake a cache hit). -module-cache
    // is the self-maintaining composition: read the file when present, keep a writer armed,
    // and save only when the writeback fired (first run, or a changed module cut the
    // stream). serialize_main_module defaults true, so the whole program rides the cache.
    string explicitCache = noModuleCache ? string() : moduleCacheFile;
    string cacheReadPath = !deserFile.empty() ? deserFile : explicitCache;
    string cacheWritePath = !serFile.empty() ? serFile : explicitCache;
    bool cacheQuiet = false;
    if ( !moduleCacheExplicit && !noModuleCache && serFile.empty() && deserFile.empty()
        && jitEnabled != JitMode::Executable && !compileOnly && !buildingDocumentation && !debuggerRequired && !useAot ) {
        cacheReadPath = cacheWritePath = ModuleFileCache::defaultPath(fn, hostBinary, hostOptions);
        cacheQuiet = true;
    }
    moduleCache.install(cacheReadPath, cacheWritePath, cacheQuiet);
    auto compile0 = ref_time_ticks();
    auto program = compileDaScript(fn,access,tout,dummyGroup,policies);
    startupCompileUsec += get_time_usec(compile0);
    // the cache stays armed through simulate: a require issued from a simulate macro or an
    // [init] (the JIT's emitter) is this compile's, so its records join this stream
    // (src/ast/ARCHITECTURE.md sec.3)
    bool cacheFinished = false;
    auto finishModuleCache = [&]() {
        if ( cacheFinished ) return;
        cacheFinished = true;
        auto cres = moduleCache.finish();
        if ( !cacheQuiet ) {
            switch ( cres.verdict ) {
            case ModuleFileCache::ReadVerdict::fallback:
                if ( cres.served > 0 ) {
                    tout << "deser: FALLBACK - " << cres.served << " module(s) served from cache, re-parsed from '" << cres.cutoffFile << "' (" << cres.cutoffReason << ")\n";
                } else if ( !cres.cutoffReason.empty() ) {
                    tout << "deser: FALLBACK - modules reparsed from source (" << cres.cutoffReason << " at '" << cres.cutoffFile << "')\n";
                } else {
                    tout << "deser: FALLBACK - modules reparsed from source (stale, truncated, or version-mismatched cache)\n";
                }
                break;
            case ModuleFileCache::ReadVerdict::partial:
                // records for unchanged files that failed to deserialize (e.g. a lazily
                // populated builtin such as dasbind) - reparsed in place, the rest served
                tout << "deser: partial - " << cres.resumed << " module(s) reparsed in place, rest from cache\n";
                break;
            case ModuleFileCache::ReadVerdict::clean:
                tout << "deser: clean - all modules from cache\n";
                break;
            case ModuleFileCache::ReadVerdict::unavailable:
                tout << "deser: module cache unavailable (built without file IO)\n";
                break;
            default: break;
            }
            if ( cres.wrote ) {
                tout << "ser: wrote " << cres.wroteBytes << " bytes to '" << cacheWritePath << "'\n";
            }
        }
        if ( cres.saveFailed ) {
            tout << "ser: cannot write '" << cacheWritePath << "'\n";
        }
    };
    if ( program ) {
        if ( program->failed() ) {
            finishModuleCache();
            for ( auto & err : program->errors ) {
                tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
            }
            if ( pauseAfterErrors ) {
                getchar();
            }
        } else {
            if ( outputProgramCode )
                tout << *program << "\n";
            if ( compileOnly ) {
                finishModuleCache();
                return 0;
            }

            if ( jitEnabled==JitMode::Library ) {
                program->options.push_back(AnnotationArgument("jit_lib", true));
            }
            auto simulate0 = ref_time_ticks();
            auto pctx = SimulateWithErrReport(program, tout);
            startupSimulateUsec += get_time_usec(simulate0);
            finishModuleCache();
            // Check for compiler leaks (TypeDecl nodes left on thread root after compile+simulate)
            if ( !gc_thread_root_is_settled(tout, "GC COMPILE LEAK", "after compile") ) {
                exitCode = 1;
            }
            if ( !pctx ) {
                exitCode = 1;
            } else if ( dryRun ) {
                exitCode = 0;
                auto li = LineInfoArg();
                toLog(LogLevel::info, (string("dry run: ") + fn + "\n").c_str(), pctx.get(), &li);
            } else if ( program->thisModule->isModule ) {
                tout<< "WARNING: program is setup as both module, and endpoint.\n";
            } else {
#ifdef __EMSCRIPTEN__
                // If the program exposes update(), run it as a browser main-loop
                // (the Context is persisted off the stack) instead of single-shot main().
                if ( start_browser_loop(pctx, dummyGroup) ) {
                    return 0;
                }
#endif
                auto fnVec = pctx->findFunctions(mainFnName.c_str());
                das::vector<SimFunction *> fnMVec;
                for ( auto fnAS : fnVec ) {
                    if ( verifyCall<void>(fnAS->debugInfo, dummyGroup)
                      || verifyCall<bool>(fnAS->debugInfo, dummyGroup)
                      || verifyCall<int32_t>(fnAS->debugInfo, dummyGroup) ) {
                        fnMVec.push_back(fnAS);
                    }
                }
                if ( fnMVec.size()==0 ) {
                    tout << "function '"  << mainFnName << "' not found\n";
                } else if ( fnMVec.size()>1 ) {
                    tout << "too many options for '" << mainFnName << "'\ncandidates are:\n";
                    for ( auto fnAS : fnMVec ) {
                        tout << "\t" << fnAS->mangledName << "\n";
                    }
                } else {
                    exitCode = 0;
                    auto fnTest = fnMVec.back();
                    pctx->restart();
                    vec4f res;
                    auto run0 = ref_time_ticks();
                    if ( debuggerRequired ) {
                        res = pctx->eval(fnTest, nullptr);
                    } else {
                        res = pctx->evalWithCatch(fnTest, nullptr);
                    }
                    startupRunUsec += get_time_usec(run0);
                    if ( auto ex = pctx->getException() ) {
                        tout << "EXCEPTION: " << ex << " at " << pctx->exceptionAt.describe() << "\n";
                        exitCode = 1;
                    } else if ( fnTest->debugInfo && fnTest->debugInfo->result ) {
                        auto resType = fnTest->debugInfo->result->type;
                        if ( resType == Type::tInt ) {
                            exitCode = cast<int32_t>::to(res);
                        } else if ( resType == Type::tBool ) {
                            exitCode = cast<bool>::to(res) ? 0 : -1;
                        }
                    }
                    // Check for app leaks (TypeDecl nodes created during execution)
                    if ( !gc_thread_root_is_settled(tout, "GC APP LEAK", "after execution") ) {
                        exitCode = 1;
                    }
                }
            }
            if ( heapReportAtExit && pctx ) {
                tout << "--- heap report ---\n";
                pctx->heap->report();
                tout << "--- string heap report ---\n";
                pctx->stringHeap->report();
            }
        }
    }
    finishModuleCache();    // every path unbinds the stream, a compile that produced no program included
    return exitCode;
}

// Deduces project_root for dyn modules.
// First attempt: from command line arguments
// Second try: project file
// Third try: path from compiled file
// Default: empty
static string deduce_project_root(string maybe_project_root, string compile_file) {
    if (!maybe_project_root.empty()) {
        return maybe_project_root;
    }
    if (!projectFile.empty()) {
        auto access = get_file_access((char*)(projectFile.empty() ? nullptr : projectFile.c_str()));
        auto maybe_result = access->getDynModulesFolder();
        if (!maybe_result.empty()) {
            return maybe_result;
        }
    }
    if (!compile_file.empty()) {
        auto filename_start = compile_file.find_last_of("\\/");
        string project_root;
        if (filename_start != string::npos) {
            // Try from directory where first script located
            project_root = compile_file.substr(0, filename_start);
        } else {
            // Try from current directory.
            project_root = "./";
        }
        return project_root;
    }
    return "";
}

void replace( string& str, const string& from, const string& to ) {
    size_t it = str.find(from);
    if( it != string::npos ) {
        str.replace(it, from.length(), to);
    }
}

void print_help() {
    tout
        << "daslang version " << DAS_VERSION_MAJOR << "." << DAS_VERSION_MINOR << "." << DAS_VERSION_PATCH << "\n"
        << "daslang scriptName1 {scriptName2} .. {-main mainFnName} {-log} {-pause} -- {script arguments}\n"
        << "    --version, -version  print daslang version and exit\n"
        << "    -main <fnName> set entry function name (default: main)\n"
        << "    -v2syntax   enable version 2 syntax (uses braces {} for code blocks) [default]\n"
        << "    -v1syntax   enable version 1 syntax (uses Python-style indentation for code blocks)\n"
        << "    -v2makeSyntax enable version 1 syntax with version 2 constructors syntax (for arrays/structures)\n"
        << "    -jit        enable Just-In-Time compilation\n"
        << "    -jit-no-cache  with -jit: skip the per-script DLL cache, codegen direct in-memory.\n"
        << "                Useful when the cached .jitted_scripts/ DLL is stale or unwanted.\n"
        << "    -jit-stack  with -jit: retain every generated call in the logical daslang stack.\n"
        << "    -exe        JIT compile to standalone executable (implies -dry-run)\n"
        << "    -lib        JIT compile to a C-ABI native library: <output>.so/.dylib/.dll plus <output>.h\n"
        << "                (add -- --jit-lib-static for a .a/.lib archive instead; implies -dry-run)\n"
        << "    -lib-export-all with -lib: export every public entry-module function whose signature has a C\n"
        << "                representation, instead of only the [export_c] ones\n"
        << "    -output <path> set JIT output path\n"
        << "    --list-shared-modules <path> with -exe: write JSON describing the program's shared modules and daspkg-package .das module sources to <path>\n"
        << "    --force-shared-module <name> with -exe: force-include a shared module by daslang or package name (repeatable)\n"
        << "    -module-cache <path> self-maintaining AST module cache at <path>: read when present, rewritten\n"
        << "                when the compile diverged; prints 'deser: clean'/'partial'/'FALLBACK'. Default ON,\n"
        << "                silently, at .jitted_scripts/module_cache/<script>-<hash>.dascache for a run that\n"
        << "                executes; off under -exe (one-unit codegen is the faster binary), -compile-only, -documentation, -use-aot\n"
        << "    -MD -MF <path> write a Make dependency file naming every source, data file and\n"
        << "                directory this run read, the way a C compiler's -MD does. -MT <target>\n"
        << "                names the rule (default: <path> without its trailing .d). DAS_DEPFILE in\n"
        << "                the environment is the twin a build system sets, and it reaches the\n"
        << "                processes this one spawns\n"
        << "    -no-module-cache  no AST module cache at all\n"
        << "    -no-optimization  compile every module unoptimized (a debugger then stops on every statement the source has)\n"
        << "    -ser <path> write the compiled AST module cache to <path> after compile (explicit write half)\n"
        << "    -deser <path> read the AST module cache from <path> during compile instead of parsing;\n"
        << "                prints 'deser: clean' when every module came from the cache, 'deser: FALLBACK' otherwise\n"
        << "    -use-aot    enable AOT linking (requires AOT stubs linked into the binary)\n"
        << "    -project <path.das_project> path to project file\n"
        << "    -project_root <path> root directory of the project (used for dyn modules)\n"
        << "    -load_module <path> directly load a single dynamic-module folder (the one containing .das_module); repeatable. Bypasses the project_root/modules/<name> scan and shadows same-basename entries from dasroot/project_root.\n"
        << "    --disable-module <name> never load/register the named dynamic module (case-insensitive folder match); repeatable. Keeps a native-only module (e.g. dashv) out of a wasm cross-compile so a guarded `require ?name` resolves as absent.\n"
        << "    -log        output program code\n"
        << "    -pause      pause after errors and pause again before exiting program\n"
        << "    -dry-run    compile and simulate script without execution\n"
        << "    -compile-only compile script without simulation and execution\n"
        << "    -documentation compile in documentation/reflection mode (disables per-box transforms)\n"
        << "    -dasroot <path> set path to daslang root folder (with daslib)\n"
        << "    -dll-path <path> additional search path for dasbind libraries (repeatable; also DAS_DLL_PATH)\n"
        << "    --track-smart-ptr <id> track smart pointer with id\n"
        << "    --track-job-status <id> track JobStatus/Channel/LockBox with id\n"
        << "    --no-dump-leaks  silence the JobStatus + HandleRegistry + smart_ptr leak dumps at exit (default: dump)\n"
        << "    --linear-stack-allocator  disable scoped stack allocator\n"
        << "    --das-wait-debugger wait for debugger to attach\n"
        << "    --das-profiler enable profiler\n"
        << "    --das-profiler-log-file <file> set profiler log file\n"
        << "    --das-profiler-manual manual profiler control\n"
        << "    --das-profiler-memory memory profiler\n"
        << "    --das-profiler-time-unit <ns|us|ms|s> time unit for profiler output\n"
        << "    --das-profiler-thread-local install profiler as per-thread agent (default when not tracking memory)\n"
        << "    --das-profiler-global install profiler as singleton agent (default with --das-profiler-memory)\n"
        << "    --das-profiler-leaks track live heap allocations and dump leaks on context destroy\n"
        << "    -no-dynamic-modules  skip loading dynamic modules from dasroot and project root\n"
        << "    -ignore-manifest  compile every .das_module descriptor and load every C++ module on start, reading\n"
        << "                and writing no .das_module.manifest - by default a manifest's C++ module loads at the first require that names it\n"
        << "    -no-lint    skip the lint pass (Program::lint)\n"
        << "    --ast-verify  force-include daslib/ast_verify; checks AST structural invariants before each inference pass\n"
        << "    --ast-verify-batch  checks the finished tree only (no per-pass walks, no cross-module sweeps): cheap enough to gate many files (CI)\n"
        << "    -log-compile-time  log detailed per-module compile-time breakdown (parse / infer with pass count / optimize / macro (in infer) / macro mods / simulate) + function count,\n"
        << "                a cached module's read time (and its macro-context simulate), and the process startup timeline (builtin modules / module scan / initialize / compile / simulate / run / teardown / shutdown)\n"
        << "    --          separator for script arguments\n"
        << "daslang -aot <in_script.das> <out_script.das.cpp> {-q} {-p}\n"
        << "    -project <path.das_project> path to project file\n"
        << "    -p          paranoid validation of CPP AOT\n"
        << "    -q          suppress all output\n"
        << "    -dry-run    no changes will be written\n"
        << "    -dasroot <path> set path to daslang root folder (with daslib)\n"
        << "    -dll-path <path> additional search path for dasbind libraries (repeatable; also DAS_DLL_PATH)\n"
        << "    -log-compile-time  log per-module compile-time breakdown during AOT generation\n"
    ;
}


#include <inttypes.h>

namespace das {
    extern AotListBase impl_aot_ast_boost;
    extern AotListBase impl_aot_printer_flags_visitor;
    extern AotListBase impl_aot_functional;
    extern AotListBase impl_aot_math_boost;
    extern AotListBase impl_aot_utf8_utils;
    extern AotListBase impl_aot_templates_boost;

}

int MAIN_FUNC_NAME ( int argc, char * argv[] ) {
#if defined(_WIN32) && defined(_DEBUG)
    // Suppress all CRT assertion/error dialogs — print to stderr instead
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    _set_error_mode(_OUT_TO_STDERR);
#endif
    auto startupMain0 = ref_time_ticks();
    int64_t startupScanUsec = 0;
    int64_t startupInitUsec = 0;
    int64_t startupCompileAndRunUsec = 0;
    int64_t startupPreScanUsec = 0;
    int64_t startupBuiltinUsec = 0;
    install_das_crash_handler();
#ifdef __APPLE__
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#endif
    das::arm_alloc_tracking();
    bool isArgAot = false;
    if (argc > 1) {
        isArgAot = strcmp(argv[1],"-aot")==0;
        isAotLib = !isArgAot && strcmp(argv[1],"-aotlib")==0;
    }

    if ( argc>2 && (isArgAot || isAotLib) ) {
        return das_aot_main(argc, argv);
    }
    use_utf8();
    if ( argc==2 && (strcmp(argv[1],"--version")==0 || strcmp(argv[1],"-version")==0) ) {
        tout << DAS_VERSION_MAJOR << "." << DAS_VERSION_MINOR << "." << DAS_VERSION_PATCH << "\n";
        return 0;
    }
    if ( argc<=1 ) {
        print_help();
        return -1;
    }
    setCommandLineArguments(argc,argv);
    char exePath[4096];
    size_t exeLen = getExecutablePathName(exePath, sizeof(exePath));
    hostBinary = exeLen ? string(exePath, exeLen) : string(argv[0]);
    for ( int i=1; i < argc && strcmp(argv[i],"--")!=0; ++i ) {
        if ( int skip = das_dep_flag_argc(argv[i]) ) { i += skip - 1; continue; }
        hostOptions += argv[i];
        hostOptions += '\n';
    }
    // a --jit-target after the separator is a compile input too: get_target_triple and every fold
    // over it bake the target into each module, so a cached native compile must not serve a cross one
    for ( int i=1, sep=0; i < argc; ++i ) {
        if ( !sep ) { sep = strcmp(argv[i],"--")==0; continue; }
        if ( strcmp(argv[i],"--jit-target")==0 || strncmp(argv[i],"--jit-target=",13)==0 ) {
            hostOptions += argv[i];
            hostOptions += '\n';
            if ( strcmp(argv[i],"--jit-target")==0 && i+1<argc ) {
                hostOptions += argv[i+1];
                hostOptions += '\n';
            }
        }
    }
    das::vector<string> files;
    string mainName = "main";
    bool scriptArgs = false;
    bool outputProgramCode = false;
    bool pauseAfterDone = false;
    bool dryRun = false;
    bool compileOnly = false;
    bool dumpLeaks = true;
    string project_root;
    vector<string> load_modules;
    vector<string> disabled_modules;
    for ( int i=1; i < argc; ++i ) {
        if ( argv[i][0]=='-' ) {
            string cmd(argv[i]+1);
            if ( cmd=="-" ) {
                scriptArgs = true;
            }
            if ( cmd=="main" ) {
                if ( i+1 >= argc ) {
                    printf("main requires argument\n");
                    print_help();
                    return -1;
                }
                mainName = argv[i+1];
                i += 1;
            } else if ( cmd=="dll-path" ) {
                if ( i+1 >= argc ) {
                    printf("dll-path requires argument\n");
                    print_help();
                    return -1;
                }
                dllSearchPaths.push_back(argv[i+1]);
                i += 1;
            } else if ( cmd=="dasroot" ) {
                if ( i+1 >= argc ) {
                    printf("dasroot requires argument\n");
                    print_help();
                    return -1;
                }
                setDasRoot(argv[i+1]);
                i += 1;
            } else if ( cmd=="v2syntax" ) {
                version2syntax = true;
            } else if ( cmd=="v1syntax" ) {
                version2syntax = false;
            } else if ( cmd=="v2makeSyntax" ) {
                version2syntax = false;
                gen2MakeSyntax = true;
            } else if ( cmd=="track-allocations" ) {
                trackAllocations = true;
            } else if ( cmd=="heap-report" ) {
                heapReportAtExit = true;
            } else if ( cmd=="jit") {
                jitEnabled = JitMode::Direct;
            } else if ( cmd=="jit-no-cache") {
                jitNoCache = true;
            } else if ( cmd=="jit-stack") {
                jitStack = true;
            } else if ( cmd=="use-aot") {
                useAot = true;
            } else if ( cmd=="aot-macros") {
                aotMacros = true;   // force quote lowering (daslib/quote) in a normal run
            } else if ( cmd=="output") {
                if ( i+1 >= argc ) {
                    printf("output requires argument\n");
                    print_help();
                    return -1;
                }
                jitOutPath = argv[i+1];
                i += 1;
            } else if ( cmd=="exe") {
                jitEnabled = JitMode::Executable;
                dryRun = true;
            } else if ( cmd=="lib") {
                jitEnabled = JitMode::Library;
                dryRun = true;
                libNeedsOutput = true;
            } else if ( cmd=="lib-export-all") {
                libExportAll = true;
            } else if ( cmd=="ser" ) {
                if ( i+1 >= argc ) {
                    printf("-ser requires path argument\n");
                    print_help();
                    return -1;
                }
                serFile = argv[i+1];
                i += 1;
            } else if ( cmd=="deser" ) {
                if ( i+1 >= argc ) {
                    printf("-deser requires path argument\n");
                    print_help();
                    return -1;
                }
                deserFile = argv[i+1];
                i += 1;
            } else if ( cmd=="module-cache" ) {
                if ( i+1 >= argc ) {
                    printf("-module-cache requires path argument\n");
                    print_help();
                    return -1;
                }
                moduleCacheFile = argv[i+1];
                moduleCacheExplicit = true;
                i += 1;
            } else if ( cmd=="MD" ) {
                depFileArmed = true;
            } else if ( cmd=="MF" ) {
                if ( i+1 >= argc ) {
                    printf("-MF requires path argument\n");
                    print_help();
                    return -1;
                }
                depFilePath = argv[i+1];
                depFileArmed = true;
                i += 1;
            } else if ( cmd=="MT" ) {
                if ( i+1 >= argc ) {
                    printf("-MT requires target argument\n");
                    print_help();
                    return -1;
                }
                depFileTarget = argv[i+1];
                i += 1;
            } else if ( cmd=="no-module-cache" ) {
                noModuleCache = true;
            } else if ( cmd=="no-optimization" ) {
                noOptimization = true;
            } else if ( cmd=="-list-shared-modules" ) {
                // script will pick up next argument by itself (read from llvm_exe.das via get_command_line_arguments())
                if ( i+1 >= argc ) {
                    printf("expecting --list-shared-modules path\n");
                    print_help();
                    return -1;
                }
                i += 1;
            } else if ( cmd=="-force-shared-module" ) {
                // script will pick up next argument by itself (force-include a shared module
                // in the release-deps list even when the program doesn't `require` it)
                if ( i+1 >= argc ) {
                    printf("expecting --force-shared-module name\n");
                    print_help();
                    return -1;
                }
                i += 1;
            } else if ( cmd=="log" ) {
                outputProgramCode = true;
            } else if ( cmd=="dry-run" ) {
                dryRun = true;
            } else if ( cmd=="compile-only" ) {
                compileOnly = true;
            } else if ( cmd=="documentation" ) {
                buildingDocumentation = true;
            } else if ( cmd=="no-lint" ) {
                noLint = true;
            } else if ( cmd=="log-compile-time" ) {
                logModuleCompileTime = true;
            } else if ( cmd=="project-root" || cmd=="project_root" ) {
                project_root = argv[i + 1];
                i++;
            } else if ( cmd=="load-module" || cmd=="load_module" ) {
                if ( i+1 >= argc ) {
                    printf("-load_module requires path argument\n");
                    print_help();
                    return -1;
                }
                load_modules.push_back(argv[i + 1]);
                i++;
            } else if ( cmd=="-disable-module" ) {
                if ( i+1 >= argc ) {
                    printf("--disable-module requires module-name argument\n");
                    print_help();
                    return -1;
                }
                disabled_modules.push_back(argv[i + 1]);
                i++;
            } else if ( cmd=="args" ) {
                break;
            } else if ( cmd=="pause" ) {
                pauseAfterErrors = true;
                pauseAfterDone = true;
            } else if ( cmd=="project") {
                if ( i+1 >= argc ) {
                    printf("das-project requires argument\n");
                    print_help();
                    return -1;
                }
                projectFile = argv[i+1];
                i += 1;
            } else if ( cmd=="-track-smart-ptr" ) {
                // script will pick up next argument by itself
                if ( i+1 >= argc ) {
                    printf("expecting smart pointer id\n");
                    print_help();
                    return -1;
                }
                uint64_t id = 0;
                if ( sscanf(argv[i+1], "%" PRIx64, &id)!=1 ) {
                    printf("expecting smart pointer id, got %s\n", argv[i+1]);
                    return -1;
                }
                ptr_ref_count::ref_count_track = id;
                i += 1;
                printf("tracking %" PRIx64 " aka %" PRIu64 "\n", id, id);
            } else if ( cmd=="-track-job-status" ) {
                if ( i+1 >= argc ) {
                    printf("expecting job status id\n");
                    print_help();
                    return -1;
                }
                uint64_t id = 0;
                if ( sscanf(argv[i+1], "%" PRIu64, &id)!=1 ) {
                    printf("expecting job status id, got %s\n", argv[i+1]);
                    return -1;
                }
                g_jobque_track_id.store(id);
                i += 1;
                printf("tracking JobStatus #%" PRIu64 "\n", id);
            } else if ( cmd=="-das-wait-debugger") {
                debuggerRequired = true;
            } else if ( cmd=="-ast-verify" || cmd=="-ast-verify-batch" ) {
                astVerifyRequired = true;   // daslib/ast_verify reads --ast-verify-batch from argv itself
            } else if ( cmd=="-linear-stack-allocator") {
                scopedStackAllocator = false;
            } else if ( cmd=="-das-profiler") {
                profilerRequired = true;
            } else if ( cmd=="-das-profiler-log-file") {
                // script will pick up next argument by itself
                if ( i+1 >= argc ) {
                    printf("expecting profiler log file name\n");
                    print_help();
                    return -1;
                }
                i += 1;
            } else if ( cmd=="-das-profiler-manual" ) {
                // do nohting, script handles it
            } else if ( cmd=="-das-profiler-memory" ) {
                // do nohting, script handles it
            } else if ( cmd=="-das-profiler-thread-local" ) {
                // do nothing, script handles it
            } else if ( cmd=="-das-profiler-global" ) {
                // do nothing, script handles it
            } else if ( cmd=="-das-profiler-leaks" ) {
                // do nothing, script handles it
            } else if ( cmd=="-das-profiler-time-unit" ) {
                // script will pick up next argument by itself
                if ( i+1 >= argc ) {
                    printf("expecting profiler time unit (ns, us, ms, s)\n");
                    print_help();
                    return -1;
                }
                i += 1;
            } else if ( cmd=="no-dynamic-modules" ) {
                noDynamicModules = true;
            } else if ( cmd=="ignore-manifest" ) {
                ignore_dynamic_module_manifests(true);
            } else if ( cmd=="-dump-leaks" ) {
                dumpLeaks = true;
            } else if ( cmd=="-no-dump-leaks" ) {
                dumpLeaks = false;
            } else if ( !scriptArgs && (cmd=="h" || cmd=="-help") ) {
                print_help();
                return 0;
            } else if ( !scriptArgs) {
                printf("unknown command line option %s\n", cmd.c_str());
                print_help();
                return -1;
            }
        }
        else if (!scriptArgs) {
            files.push_back(argv[i]);
        }
    }
    if (files.empty()) {
        print_help();
        return -1;
    }
    if ( moduleCacheExplicit && (!serFile.empty() || !deserFile.empty()) ) {
        printf("-module-cache already reads and writes; do not combine it with -ser/-deser\n");
        return -1;
    }
    if ( noModuleCache && (!serFile.empty() || !deserFile.empty()) ) {
        printf("-no-module-cache disables the cache; do not combine it with -ser/-deser\n");
        return -1;
    }
    if ( libNeedsOutput && jitOutPath.empty() ) {
        printf("-lib needs -output <path>: a host includes the generated header by name, and the\n"
               "default JIT cache path is hash-named and swept\n");
        return -1;
    }
    if ( depFileArmed && !depFilePath.empty() ) {
        das_dep_arm(depFilePath.c_str(), depFileTarget.c_str());
    } else if ( depFileArmed ) {
        printf("-MD needs -MF <path>, or DAS_DEPFILE in the environment\n");
        print_help();
        return -1;
    }
    startupPreScanUsec = get_time_usec(startupMain0);
    auto builtin0 = ref_time_ticks();
    // register modules
    register_builtin_modules();
    require_project_specific_modules();
    // dagor: no external (daspkg-generated) static modules to pull
    startupBuiltinUsec = get_time_usec(builtin0);
    auto scan0 = ref_time_ticks();
    #ifdef DAS_ENABLE_DYN_INCLUDES
    if ( !noDynamicModules ) {
        // Search for external modules and init them. Only if flag is enabled.
        daScriptEnvironment::ensure();
        project_root = deduce_project_root(project_root, files.front());
        auto access = get_file_access((char*)(projectFile.empty() ? nullptr : projectFile.c_str()));
        require_dynamic_modules(access, getDasRoot(), project_root, load_modules, disabled_modules, tout);
    }
    #endif
    startupScanUsec = get_time_usec(scan0);
    auto init0 = ref_time_ticks();
    Module::Initialize();
    startupInitUsec = get_time_usec(init0);

    // compile and run
    int exitCode = 0;
    if (!aotResult.empty() && files.size() > 1) {
        printf("Aotting more than 1 file is not supported yet.\n");
        return -1;
    }

#ifdef __EMSCRIPTEN__
    g_webloop_dump_leaks = dumpLeaks;   // stop_browser_loop's leak check honors -no-dump-leaks
#endif
    for ( auto & fn : files ) {
        replace(fn, "_dasroot_", getDasRoot());
        auto compileAndRun0 = ref_time_ticks();
        int rc = compile_and_run(fn, mainName, outputProgramCode, dryRun, compileOnly);
        startupCompileAndRunUsec += get_time_usec(compileAndRun0);
        if ( rc != 0 ) {
            exitCode = rc;
        }
    }
    // and done
    if ( pauseAfterDone ) getchar();
#ifdef __EMSCRIPTEN__
    // A browser-loop program is still RUNNING here: no Module::Shutdown (its
    // dtors destroy live state — see g_webloop_defer_module_shutdown), and no
    // leak dump/exit(1) — every in-use object would read as leaked.
    if ( g_activeWebLoop != nullptr ) {
        g_webloop_defer_module_shutdown = true;
        return exitCode;
    }
#endif
    // Handle-leak dump runs inside Module::Shutdown, between module
    // destruction (drains job threads) and DLL unload (invalidates the
    // dumpHandleLeaks<T> function pointers registered from shared modules).
    auto shutdown0 = ref_time_ticks();
    Module::Shutdown(dumpLeaks, !DAS_ORPHAN_FUSION_AT_EXIT);
    if ( dumpLeaks ) {
        JobStatus::DumpJobQueLeaks();
    }
    if ( logModuleCompileTime ) {
        auto shutdownUsec = get_time_usec(shutdown0);
        auto teardownUsec = startupCompileAndRunUsec - startupCompileUsec - startupSimulateUsec - startupRunUsec;
        tout << "startup: main total " << (get_time_usec(startupMain0) / 1000000.) << "\n"
             << "\targuments " << (startupPreScanUsec / 1000000.) << "\n"
             << "\tbuiltin modules " << (startupBuiltinUsec / 1000000.) << "\n"
             << "\tmodule scan " << (startupScanUsec / 1000000.) << "\n"
             << "\tinitialize " << (startupInitUsec / 1000000.) << "\n"
             << "\tcompile  " << (startupCompileUsec / 1000000.) << "\n"
             << "\tsimulate " << (startupSimulateUsec / 1000000.) << "\n"
             << "\trun      " << (startupRunUsec / 1000000.) << "\n"
             << "\tteardown " << (teardownUsec / 1000000.) << "\n"
             << "\tshutdown " << (shutdownUsec / 1000000.) << "\n"
        ;
    }
    // das::dump_alloc_leaks registers itself as the FIRST atexit handler, so it
    // fires after all static destructors — cleaner than dumping here.
#ifndef DAS_AOT_COMPILER
    if ( g_smart_ptr_total!=0 ) {
        // The exit is unconditional but the explanation used to sit inside `if (dumpLeaks)`, so
        // `-no-dump-leaks` turned this into a bare exit(1) -- indistinguishable from a script that
        // simply returned 1. Announce the reason either way; only the per-pointer dump is optional.
        TextPrinter tp;
        tp << "FATAL: exiting with code 1 -- " << uint64_t(g_smart_ptr_total)
           << " smart pointer(s) still alive at shutdown";
        if ( !dumpLeaks ) tp << " (re-run without -no-dump-leaks for the per-pointer dump)";
        tp << "\n";
        tp.output();    // ~TextWriter only frees the buffer; without this the message is discarded
        if ( dumpLeaks ) {
            ptr_ref_count::DumpTrackPtr();
        }
        exit(1);
    }
#endif
    return exitCode;
}

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

void use_utf8() {
#if defined(_WIN32)
    // you man need to set console output to utf-8 on windows. call
    //  CHCP 65001
    // from the command line. make sure appropriate font is selected
    SetConsoleOutputCP(CP_UTF8);
#endif
}
