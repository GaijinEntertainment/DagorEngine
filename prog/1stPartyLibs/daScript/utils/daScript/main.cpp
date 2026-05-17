#include "daScript/ast/aot_templates.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/dyn_modules.h"
#include "daScript/daScript.h"
#include "daScript/daScriptModule.h"
#include "daScript/das_common.h"
#include "daScript/simulate/fs_file_info.h"
#include "../dasFormatter/fmt.h"
#include "daScript/ast/ast_aot_cpp.h"
#include "daScript/misc/crash_handler.h"
#include "daScript/misc/job_que.h"
#if defined(_WIN32) && defined(_DEBUG)
#include <crtdbg.h>
#endif

using namespace das;

void use_utf8();

void require_project_specific_modules();//link time resolved dependencies
das::FileAccessPtr get_file_access( char * pak );//link time resolved dependencies

TextPrinter tout;

static string projectFile;
// aot config
static bool aotMacros = false;
static bool isAotLib = false;
static string aotResult = "";
// aot config end
static bool paranoid_validation = false;
static bool profilerRequired = false;
static bool debuggerRequired = false;
static bool scopedStackAllocator = true;
static bool pauseAfterErrors = false;
static bool quiet = false;
enum class JitMode {
    None,
    Direct,
    Dll,
    Executable,
};
static JitMode jitEnabled = JitMode::None; // Disabled by default.
static string jitOutPath = ""; // Empty, JIT module will choose default.

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

static CodeOfPolicies getPolicies() {
    CodeOfPolicies policies;
    policies.aot = false;
    policies.aot_module = true;
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
    return policies;
}

bool aot_compile ( vector<pair<string, string>> &aot_files, bool dryRun, bool cross_platform ) {
    // call daslib/aot_cpp `aot()` from C++
    auto access = get_file_access((char*)(projectFile.empty() ? nullptr : projectFile.c_str()));
    ModuleGroup dummyGroup;
    CodeOfPolicies stubPolicies;
    stubPolicies.version_2_syntax = true;
    stubPolicies.aot_module = true;
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
        tout << "daslang -aot <in_script.das> <out_script.das.cpp> [-v2Syntax] [-v1Syntax] [-v2makeSyntax] [-project <project file>] [-dasroot <dasroot folder>] [-q] [-j] [-aot-macros] [-cross-platform] [-no-lint]\n";
        return -1;
    }
    bool dryRun = false;
    bool cross_platform = false; // strcmp("-aotlib", argv[1]) == 0;
    bool scriptArgs = false;
    bool das_mode = false;
    vector<pair<string, string>> aot_files;
    string project_root;
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
            } else if ( strcmp(argv[ai],"-das-mode")==0 ) {
                das_mode = true;
            } else if ( strcmp(argv[ai],"-cross-platform")==0 ) {
                cross_platform = true;
            } else if ( strcmp(argv[ai],"-aot-macros")==0 ) {
                aotMacros = true;
            } else if ( strcmp(argv[ai],"-project")==0 ) {
                if ( ai+1 > argc ) {
                    tout << "das-project requires argument";
                    return -1;
                }
                projectFile = argv[ai+1];
                ai += 1;
            } else if ( strcmp(argv[ai],"-dasroot")==0 ) {
                if ( ai+1 > argc ) {
                    tout << "dasroot requires argument";
                    return -1;
                }
                setDasRoot(argv[ai+1]);
                ai += 1;
            } else if ( strcmp(argv[ai],"-project-root")==0 || strcmp(argv[ai],"-project_root")==0 ) {
                project_root = argv[ai + 1];
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
            } else if ( strcmp(argv[ai],"-no-lint")==0 ) {
                noLint = true;
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
    #if !defined(DAS_ENABLE_DLL) || !defined(DAS_ENABLE_DYN_INCLUDES)
    // Otherwises search for static modules.
    #include "modules/external_need.inc"
    #endif
    #ifdef DAS_ENABLE_DYN_INCLUDES
    if ( !noDynamicModules ) {
        daScriptEnvironment::ensure();
        auto access = get_file_access((char*)(projectFile.empty() ? nullptr : projectFile.c_str()));
        require_dynamic_modules(access, getDasRoot(), project_root, tout);
    }
    #endif
    Module::Initialize();
    daScriptEnvironment::getBound()->g_isInAot = true;
    bool compiled = true;
    compiled = aot_compile(aot_files, dryRun, cross_platform);
    Module::Shutdown();
    return compiled ? 0 : -1;
}

// returns process exit code:
//   0 on success
//   non-zero from int main, or 1 on compile/simulate/verify/exception failure
int compile_and_run ( const string & fn, const string & mainFnName, bool outputProgramCode, bool dryRun, bool compileOnly, const char * introFile = nullptr ) {
    // Heap-leak tracker: arm the tail-memoize landmark so per-allocation stack
    // captures below this frame skip re-walking ancestors. No-op when
    // DAS_TRACK_ALLOC is off or on non-Win64.
    das::AllocTrackingLandmark _alloc_tracker_landmark;
    auto access = get_file_access((char*)(projectFile.empty() ? nullptr : projectFile.c_str()));
    if ( introFile ) {
        auto fileInfo = make_unique<TextFileInfo>(introFile, uint32_t(strlen(introFile)), false);
        access->setFileInfo("____intro____", das::move(fileInfo));
    }
    int exitCode = 1;
    ModuleGroup dummyGroup;
    CodeOfPolicies policies;
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
            case JitMode::Dll: policies.jit_dll_mode = true; break;
            case JitMode::Direct: break;
            default: break;
        }
        access->addExtraModule("just_in_time", getDasRoot() + "/daslib/just_in_time.das");
        policies.jit_output_path = jitOutPath;
        policies.dll_search_paths.emplace_back(getDasRoot() + "/lib");
    }
    if ( useAot ) {
        // don't set policies.aot here - the host program (e.g. dastest) doesn't need AOT linking
        // the --use-aot flag (after --) tells dastest to enable AOT for test files it compiles
        policies.fail_on_no_aot = false;
    } else {
        policies.fail_on_no_aot = false;
    }
    policies.fail_on_lack_of_aot_export = false;
    policies.version_2_syntax = version2syntax;
    policies.gen2_make_syntax = gen2MakeSyntax;
    policies.scoped_stack_allocator = scopedStackAllocator;
    policies.track_allocations = trackAllocations;
    policies.no_lint = noLint;
    policies.persistent_heap = true;
    if ( auto program = compileDaScript(fn,access,tout,dummyGroup,policies) ) {
        if ( program->failed() ) {
            for ( auto & err : program->errors ) {
                tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
            }
            if ( pauseAfterErrors ) {
                getchar();
            }
        } else {
            if ( outputProgramCode )
                tout << *program << "\n";
            if ( compileOnly )
                return 0;

            auto pctx = SimulateWithErrReport(program, tout);
            // Check for compiler leaks (TypeDecl nodes left on thread root after compile+simulate)
            {
                auto & root = gc_root::gc_get_thread_root();
                if (root.gc_count != 0) {
                    tout << "GC COMPILE LEAK: " << uint64_t(root.gc_count) << " gc_node(s) after compile\n";
                    root.gc_report();
                }
            }
            if ( !pctx ) {
                exitCode = 1;
            } else if ( program->thisModule->isModule ) {
                tout<< "WARNING: program is setup as both module, and endpoint.\n";
            } else if ( dryRun ) {
                exitCode = 0;
                tout << "dry run: " << fn << "\n";
            } else {
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
                    if ( debuggerRequired ) {
                        res = pctx->eval(fnTest, nullptr);
                    } else {
                        res = pctx->evalWithCatch(fnTest, nullptr);
                    }
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
                    {
                        auto & root = gc_root::gc_get_thread_root();
                        if (root.gc_count != 0) {
                            tout << "GC APP LEAK: " << uint64_t(root.gc_count) << " gc_node(s) after execution\n";
                            root.gc_report();
                        }
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
        << "    -exe        JIT compile to standalone executable (implies -dry-run)\n"
        << "    -output <path> set JIT output path\n"
        << "    -use-aot    enable AOT linking (requires AOT stubs linked into the binary)\n"
        << "    -project <path.das_project> path to project file\n"
        << "    -project_root <path> root directory of the project (used for dyn modules)\n"
        << "    -run-fmt    <-i/-d> <-v2/-v1> {--semicolon} run formatter\n"
        << "    -log        output program code\n"
        << "    -pause      pause after errors and pause again before exiting program\n"
        << "    -dry-run    compile and simulate script without execution\n"
        << "    -compile-only compile script without simulation and execution\n"
        << "    -dasroot <path> set path to daslang root folder (with daslib)\n"
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
        << "    -no-lint    skip the lint pass (Program::lint)\n"
        << "    --          separator for script arguments\n"
        << "daslang -aot <in_script.das> <out_script.das.cpp> {-q} {-p}\n"
        << "    -project <path.das_project> path to project file\n"
        << "    -p          paranoid validation of CPP AOT\n"
        << "    -q          suppress all output\n"
        << "    -dry-run    no changes will be written\n"
        << "    -dasroot <path> set path to daslang root folder (with daslib)\n"
    ;
}

#ifndef MAIN_FUNC_NAME
  #define MAIN_FUNC_NAME main
#endif

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
    install_das_crash_handler();
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
    das::vector<string> files;
    string mainName = "main";
    bool scriptArgs = false;
    bool outputProgramCode = false;
    bool pauseAfterDone = false;
    bool dryRun = false;
    bool compileOnly = false;
    bool dumpLeaks = true;
    string project_root;
    optional<format::FormatOptions> formatter;
    for ( int i=1; i < argc; ++i ) {
        if ( argv[i][0]=='-' ) {
            string cmd(argv[i]+1);
            if ( cmd=="-" ) {
                scriptArgs = true;
            }
            if ( cmd=="main" ) {
                if ( i+1 > argc ) {
                    printf("main requires argument\n");
                    print_help();
                    return -1;
                }
                mainName = argv[i+1];
                i += 1;
            } else if ( cmd=="dasroot" ) {
                if ( i+1 > argc ) {
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
            } else if ( cmd=="use-aot") {
                useAot = true;
            } else if ( cmd=="output") {
                if ( i+1 > argc ) {
                    printf("output requires argument\n");
                    print_help();
                    return -1;
                }
                jitOutPath = argv[i+1];
                i += 1;
            } else if ( cmd=="exe") {
                jitEnabled = JitMode::Executable;
                dryRun = true;
            } else if ( cmd=="log" ) {
                outputProgramCode = true;
            } else if ( cmd=="dry-run" ) {
                dryRun = true;
            } else if ( cmd=="compile-only" ) {
                compileOnly = true;
            } else if ( cmd=="no-lint" ) {
                noLint = true;
            } else if ( cmd=="project-root" || cmd=="project_root" ) {
                project_root = argv[i + 1];
                i++;
            } else if ( cmd=="run-fmt" ) {
                formatter.emplace();
                if ( i+2 > argc ) {
                    printf("formatter requires 2 arguments\n");
                    print_help();
                    return -1;
                }
                const string mode = string(argv[i+1]);
                if (mode == "-i" || mode == "--inplace") {
                    formatter->insert(format::FormatOpt::Inplace);
                } else if (string(argv[i+1]) == "-d" || string(argv[i+1]) == "--dry") {
                } else {
                    print_help();
                    return -1;
                }
                i += 1;
                const string to_v2 = string(argv[i+1]);
                if (to_v2 == "-v2") {
                    formatter->insert(format::FormatOpt::V2Syntax);
                } else if (to_v2 == "-v1") {
                } else {
                    print_help();
                    return -1;
                }
                i++;

                if (i + 1 < argc)  {
                    if (string(argv[i + 1]) == "--semicolon") {
                        formatter->insert(format::FormatOpt::SemicolonEOL);
                        ++i;
                    }
                }
            } else if ( cmd=="args" ) {
                break;
            } else if ( cmd=="pause" ) {
                pauseAfterErrors = true;
                pauseAfterDone = true;
            } else if ( cmd=="project") {
                if ( i+1 > argc ) {
                    printf("das-project requires argument\n");
                    print_help();
                    return -1;
                }
                projectFile = argv[i+1];
                i += 1;
            } else if ( cmd=="-track-smart-ptr" ) {
                // script will pick up next argument by itself
                if ( i+1 > argc ) {
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
                if ( i+1 > argc ) {
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
            } else if ( cmd=="-linear-stack-allocator") {
                scopedStackAllocator = false;
            } else if ( cmd=="-das-profiler") {
                profilerRequired = true;
            } else if ( cmd=="-das-profiler-log-file") {
                // script will pick up next argument by itself
                if ( i+1 > argc ) {
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
            } else if ( cmd=="-dump-leaks" ) {
                dumpLeaks = true;
            } else if ( cmd=="-no-dump-leaks" ) {
                dumpLeaks = false;
            } else if ( cmd=="h" || cmd=="-help" ) {
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
    // register modules
    register_builtin_modules();
    require_project_specific_modules();
    #if !defined(DAS_ENABLE_DLL) || !defined(DAS_ENABLE_DYN_INCLUDES)
    // Otherwises search for static modules.
    #include "modules/external_need.inc"
    #endif
    #ifdef DAS_ENABLE_DYN_INCLUDES
    if ( !noDynamicModules ) {
        // Search for external modules and init them. Only if flag is enabled.
        daScriptEnvironment::ensure();
        project_root = deduce_project_root(project_root, files.front());
        auto access = get_file_access((char*)(projectFile.empty() ? nullptr : projectFile.c_str()));
        require_dynamic_modules(access, getDasRoot(), project_root, tout);
    }
    #endif
    Module::Initialize();

    if (formatter) {
        return format::run(formatter.value(), files);
    }
    // compile and run
    int exitCode = 0;
    if (!aotResult.empty() && files.size() > 1) {
        printf("Aotting more than 1 file is not supported yet.\n");
        return -1;
    }

    for ( auto & fn : files ) {
        replace(fn, "_dasroot_", getDasRoot());
        int rc = compile_and_run(fn, mainName, outputProgramCode, dryRun, compileOnly);
        if ( rc != 0 ) {
            exitCode = rc;
        }
    }
    // and done
    if ( pauseAfterDone ) getchar();
    // Handle-leak dump runs inside Module::Shutdown, between module
    // destruction (drains job threads) and DLL unload (invalidates the
    // dumpHandleLeaks<T> function pointers registered from shared modules).
    Module::Shutdown(dumpLeaks);
    if ( dumpLeaks ) {
        JobStatus::DumpJobQueLeaks();
    }
    // das::dump_alloc_leaks is registered as an atexit handler via init_seg(lib),
    // so it fires after all static destructors — cleaner than dumping here.
    if ( g_smart_ptr_total!=0 ) {
        if ( dumpLeaks ) {
            TextPrinter tp;
            tp << "smart pointers leaked: " << uint64_t(g_smart_ptr_total) << "\n";
            ptr_ref_count::DumpTrackPtr();
        }
        // exit(1);
    }
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
