#include "daScript/ast/aot_templates.h"
#include "daScript/daScript.h"
#include "daScript/das_common.h"
#include "daScript/simulate/fs_file_info.h"
#include "../dasFormatter/fmt.h"
#include "daScript/ast/ast_aot_cpp.h"

// aot das-mode temporary disabled
// #include "../../src/das/ast/_standalone_ctx_generated/ast_aot_cpp.das.h"
// #include "../../src/das/ast/_standalone_ctx_generated/standalone_contexts.das.h"

using namespace das;

void use_utf8();

void require_project_specific_modules();//link time resolved dependencies
das::FileAccessPtr get_file_access( char * pak );//link time resolved dependencies

TextPrinter tout;

static string projectFile;
static bool aotMacros = false;
static bool profilerRequired = false;
static bool debuggerRequired = false;
static bool scopedStackAllocator = true;
static bool pauseAfterErrors = false;
static bool quiet = false;
static bool paranoid_validation = false;
static bool jitEnabled = false;
static bool isAotLib = false;
static bool version2syntax = true;
static bool gen2MakeSyntax = false;

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
    return policies;
}

bool compile ( const string & fn, const string & cppFn, bool dryRun, bool cross_platform ) {
    auto access = get_file_access((char*)(projectFile.empty() ? nullptr : projectFile.c_str()));
    ModuleGroup dummyGroup;
    if ( auto program = compileDaScript(fn,access,tout,dummyGroup,getPolicies()) ) {
        if ( program->failed() ) {
            tout << "failed to compile\n";
            for ( auto & err : program->errors ) {
                tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
            }
            return false;
        } else {
            auto pctx = SimulateWithErrReport(program, tout);
            if (!pctx) {
                return false;
            }
            if ( dryRun ) {
                tout << "dry run success, no changes will be written\n";
                return true;
            }

            // AOT time
            TextWriter tw;
            bool noAotModule = false;
            // header
            tw << AOT_INCLUDES;
            // lets comment on required modules
            program->library.foreach_in_order([&](Module * mod){
                if ( mod->name=="" ) {
                    // nothing, its main program module. i.e ::
                } else {
                    if ( mod->name=="$" ) {
                        tw << " // require builtin\n";
                    } else {
                        tw << " // require " << mod->name << "\n";
                    }
                    if ( mod->aotRequire(tw)==ModuleAotType::no_aot ) {
                        tw << "  // AOT disabled due to this module\n";
                        noAotModule = true;
                    }
                }
                return true;
            }, program->getThisModule());
            if ( program->options.getBoolOption("no_aot",false) ) {
                TextWriter noTw;
                if (!noAotModule)
                  noTw << "// AOT disabled due to options no_aot=true. There are no modules which require no_aot\n\n";
                else
                  noTw << "// AOT disabled due to options no_aot=true. There are also some modules which require no_aot\n\n";
                return saveToFile(tout, cppFn, noTw.str(), quiet);
            } else if ( noAotModule ) {
                TextWriter noTw;
                noTw << "// AOT disabled due to module requirements\n";
                noTw << "#if 0\n\n";
                noTw << tw.str();
                noTw << "\n#endif\n";
                return saveToFile(tout, cppFn, noTw.str(), quiet);
            } else {
                tw << AOT_HEADERS;
                {
                    NamespaceGuard das_guard(tw, "das");
                    {
                        NamespaceGuard anon_guard(tw, program->thisNamespace); // anonymous
                        (*daScriptEnvironment::bound)->g_Program = program;    // setting it for the AOT macros
                        program->aotCpp(*pctx, tw, cross_platform);
                        (*daScriptEnvironment::bound)->g_Program.reset();
                        // list STUFF
                        tw << "\nstatic void registerAotFunctions ( AotLibrary & aotLib ) {\n";
                        program->registerAotCpp(tw, *pctx, false);
                        tw << "    resolveTypeInfoAnnotations();\n";
                        tw << "}\n";
                        tw << "\n";
                        if ( !isAotLib ) tw << "static AotListBase impl(registerAotFunctions);\n";
                        // validation stuff
                        if ( paranoid_validation ) {
                            program->validateAotCpp(tw,*pctx);
                            tw << "\n";
                        }
                        // footer
                    }
                    if ( isAotLib ) tw << "AotListBase impl_aot_" << program->thisModule->name << "(" << program->thisNamespace << "::registerAotFunctions);\n";
                }
                tw << AOT_FOOTER;
                return saveToFile(tout, cppFn, tw.str(), quiet);
            }
        }
    } else {
        tout << "failed to compile\n";
        return false;
    }
}

bool compileStandalone ( const string & inputFile, const string & outDir, const StandaloneContextCfg &cfg ) {
    auto access = get_file_access((char*)(projectFile.empty() ? nullptr : projectFile.c_str()));
    ModuleGroup dummyGroup;
    auto policies = getPolicies();
    policies.ignore_shared_modules = true;
    if ( auto program = compileDaScript(inputFile,access,tout,dummyGroup,policies) ) {
        if ( program->failed() ) {
            tout << "failed to compile\n";
            for ( auto & err : program->errors ) {
                tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
            }
            return false;
        } else {
            runStandaloneVisitor(program, outDir, cfg);
            return true;
        }
    } else {
        tout << "failed to compile\n";
        return false;
    }
}

int das_aot_main ( int argc, char * argv[] ) {
    setCommandLineArguments(argc, argv);
    #ifdef _MSC_VER
    _CrtSetReportMode(_CRT_ASSERT, 0);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    #endif
    if ( argc<=3 ) {
        tout << "daslang -aot <in_script.das> <out_script.das.cpp> [-v2Syntax] [-v1Syntax] [-v2makeSyntax] [-standalone-context <ctx_name>] [-project <project file>] [-dasroot <dasroot folder>] [-q] [-j] [-aot-macros] [-cross-platform] [-standalone-class <class_name>]\n";
        return -1;
    }
    bool dryRun = false;
    bool cross_platform = false; // strcmp("-aotlib", argv[1]) == 0;
    bool scriptArgs = false;
    bool standaloneContext = false;
    bool das_mode = false;
    char * standaloneContextName = nullptr;
    char * standaloneClassName = nullptr;
    if ( argc>3  ) {
        for (int ai = 4; ai != argc; ++ai) {
            if ( strcmp(argv[ai],"-q")==0 ) {
                quiet = true;
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
            } else if ( strcmp(argv[ai],"-standalone-context")==0 ) {
                standaloneContextName = argv[ai + 1];
                standaloneContext = true;
                ai += 1;
            } else if ( strcmp(argv[ai],"-standalone-class")==0 ) {
                standaloneClassName = argv[ai + 1];
                ai += 1;
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
            } else if ( strcmp(argv[ai],"-v2syntax")==0 ) {
                version2syntax = true;
            } else if ( strcmp(argv[ai],"-v1syntax")==0 ) {
                version2syntax = false;
            } else if ( strcmp(argv[ai],"-v2makeSyntax")==0 ) {
                version2syntax = false;
                gen2MakeSyntax = true;
            } else if ( strcmp(argv[ai],"--")==0 ) {
                scriptArgs = true;
            } else if ( !scriptArgs ) {
                tout << "unsupported option " << argv[ai];
                return -1;
            }
        }
    }
    // register modules
    if (!Module::require("$")) {
        NEED_MODULE(Module_BuiltIn);
    }
    if (!Module::require("math")) {
        NEED_MODULE(Module_Math);
    }
    if (!Module::require("raster")) {
        NEED_MODULE(Module_Raster);
    }
    if (!Module::require("strings")) {
        NEED_MODULE(Module_Strings);
    }
    if (!Module::require("rtti")) {
        NEED_MODULE(Module_Rtti);
    }
    if (!Module::require("ast")) {
        NEED_MODULE(Module_Ast);
    }
    if (!Module::require("jit")) {
        NEED_MODULE(Module_Jit);
    }
    if (!Module::require("debugapi")) {
        NEED_MODULE(Module_Debugger);
    }
    if (!Module::require("network")) {
        NEED_MODULE(Module_Network);
    }
    if (!Module::require("uriparser")) {
        NEED_MODULE(Module_UriParser);
    }
    if (!Module::require("jobque")) {
        NEED_MODULE(Module_JobQue);
    }
    if (!Module::require("fio")) {
        NEED_MODULE(Module_FIO);
    }
    if (!Module::require("dasbind")) {
        NEED_MODULE(Module_DASBIND);
    }
    require_project_specific_modules();
    #include "modules/external_need.inc"
    Module::Initialize();
    (*daScriptEnvironment::bound)->g_isInAot = true;
    bool compiled = false;
    if ( standaloneContext ) {
        if (das_mode) {
            // aot das-mode temporary disabled
            DAS_FATAL_LOG("aot das mode is not ready");
            // standalone_contexts::Standalone st;
            // st.standalone_aot(argv[2], argv[3], isAotLib, cross_platform, paranoid_validation, getPolicies());
        } else {
            StandaloneContextCfg cfg = {standaloneContextName, standaloneClassName ? standaloneClassName : "StandaloneContext"};
            cfg.cross_platform = cross_platform;
            compiled = compileStandalone(argv[2], argv[3], cfg);
        }
    } else {
        if (das_mode) {
            // aot das-mode temporary disabled
            DAS_FATAL_LOG("aot das mode is not ready");
            // ast_aot_cpp::Standalone st;
            // auto res = st.aot(argv[2], isAotLib, paranoid_validation, cross_platform, getPolicies());
            // TextPrinter printer;
            // saveToFile(printer, argv[3], res);
            // compiled = true;
        } else {
            compiled = compile(argv[2], argv[3], dryRun, cross_platform);
        }
    }
    Module::Shutdown();
    return compiled ? 0 : -1;
}

bool compile_and_run ( const string & fn, const string & mainFnName, bool outputProgramCode, bool dryRun, const char * introFile = nullptr ) {
    auto access = get_file_access((char*)(projectFile.empty() ? nullptr : projectFile.c_str()));
    if ( introFile ) {
        auto fileInfo = make_unique<TextFileInfo>(introFile, uint32_t(strlen(introFile)), false);
        access->setFileInfo("____intro____", das::move(fileInfo));
    }
    bool success = false;
    ModuleGroup dummyGroup;
    CodeOfPolicies policies;
    if ( debuggerRequired ) {
        policies.debugger = true;
        policies.debug_module = getDasRoot() + "/daslib/debug.das";
    } else if ( profilerRequired ) {
        policies.profiler = true;
        policies.profile_module = getDasRoot() + "/daslib/profiler.das";
    } /*else*/ if ( jitEnabled ) {
        policies.jit = true;
        policies.jit_module = getDasRoot() + "/daslib/just_in_time.das";
    }
    policies.fail_on_no_aot = false;
    policies.fail_on_lack_of_aot_export = false;
    policies.version_2_syntax = version2syntax;
    policies.gen2_make_syntax = gen2MakeSyntax;
    policies.scoped_stack_allocator = scopedStackAllocator;
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
            auto pctx = SimulateWithErrReport(program, tout);
            if ( !pctx ) {
                success = false;
            } else if ( program->thisModule->isModule ) {
                tout<< "WARNING: program is setup as both module, and endpoint.\n";
            } else if ( dryRun ) {
                success = true;
                tout << "dry run: " << fn << "\n";
            } else {
                auto fnVec = pctx->findFunctions(mainFnName.c_str());
                das::vector<SimFunction *> fnMVec;
                for ( auto fnAS : fnVec ) {
                    if ( verifyCall<void>(fnAS->debugInfo, dummyGroup) || verifyCall<bool>(fnAS->debugInfo, dummyGroup) ) {
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
                    success = true;
                    auto fnTest = fnMVec.back();
                    pctx->restart();
                    if ( debuggerRequired ) {
                        pctx->eval(fnTest, nullptr);
                    } else {
                        pctx->evalWithCatch(fnTest, nullptr);
                    }
                    if ( auto ex = pctx->getException() ) {
                        tout << "EXCEPTION: " << ex << " at " << pctx->exceptionAt.describe() << "\n";
                        success = false;
                    }
                }
            }
        }
    }
    return success;
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
        << "    -v2syntax   enable version 2 syntax (uses braces {} for code blocks) [default]\n"
        << "    -v1syntax   enable version 1 syntax (uses Python-style indentation for code blocks)\n"
        << "    -v2makeSyntax enable version 1 syntax with version 2 constructors syntax (for arrays/structures)\n"
        << "    -jit        enable Just-In-Time compilation\n"
        << "    -project <path.das_project> path to project file\n"
        << "    -run-fmt    <inplace/dry> <v2/v1> <semicolon> run formatter, requires 2 or more arguments\n"
        << "    -log        output program code\n"
        << "    -pause      pause after errors and pause again before exiting program\n"
        << "    -dry-run    compile and simulate script without execution\n"
        << "    -dasroot    set path to dascript root folder (with daslib)\n"
#if DAS_SMART_PTR_ID
        << "    -track-smart-ptr <id> track smart pointer with id\n"
#endif
        << "    -linear-stack-allocator  disable scoped stack allocator\n"
        << "    -das-wait-debugger wait for debugger to attach\n"
        << "    -das-profiler enable profiler\n"
        << "    -das-profiler-log-file <file> set profiler log file\n"
        << "    -das-profiler-manual manual profiler control\n"
        << "    -das-profiler-memory memory profiler\n"
        << "daslang -aot <in_script.das> <out_script.das.cpp> {-q} {-p}\n"
        << "    -project <path.das_project> path to project file\n"
        << "    -p          paranoid validation of CPP AOT\n"
        << "    -q          suppress all output\n"
        << "    -dry-run    no changes will be written\n"
        << "    -dasroot    set path to dascript root folder (with daslib)\n"
    ;
}

#ifndef MAIN_FUNC_NAME
  #define MAIN_FUNC_NAME main
#endif

#if DAS_SMART_PTR_TRACKER
#include <inttypes.h>
#endif

namespace das {
    extern AotListBase impl_aot_ast_boost;
    extern AotListBase impl_aot_printer_flags_visitor;
    extern AotListBase impl_aot_functional;
    extern AotListBase impl_aot_math_boost;
    extern AotListBase impl_aot_utf8_utils;
    extern AotListBase impl_aot_templates_boost;

}

int MAIN_FUNC_NAME ( int argc, char * argv[] ) {
    bool isArgAot = false;
    // // aot das-mode temporary disabled
    // force_aot_stub();
    if (argc > 1) {
        isArgAot = strcmp(argv[1],"-aot")==0;
        isAotLib = !isArgAot && strcmp(argv[1],"-aotlib")==0;
    }

    if ( argc>2 && (isArgAot || isAotLib) ) {
        return das_aot_main(argc, argv);
    }
    use_utf8();
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
            } else if ( cmd=="jit") {
                jitEnabled = true;
            } else if ( cmd=="log" ) {
                outputProgramCode = true;
            } else if ( cmd=="dry-run" ) {
                dryRun = true;
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
#if DAS_SMART_PTR_ID
                uint64_t id = 0;
                if ( sscanf(argv[i+1], "%" PRIx64, &id)!=1 ) {
                    printf("expecting smart pointer id, got %s\n", argv[i+1]);
                    return -1;
                }
                ptr_ref_count::ref_count_track = id;
                i += 1;
                printf("tracking %" PRIx64 " aka %" PRIu64 "\n", id, id);
#else
                printf("smart ptr id tracking is disabled\n");
                return -1;
#endif
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
    if (!Module::require("$")) {
        NEED_MODULE(Module_BuiltIn);
    }
    if (!Module::require("math")) {
        NEED_MODULE(Module_Math);
    }
    if (!Module::require("raster")) {
        NEED_MODULE(Module_Raster);
    }
    if (!Module::require("strings")) {
        NEED_MODULE(Module_Strings);
    }
    if (!Module::require("rtti")) {
        NEED_MODULE(Module_Rtti);
    }
    if (!Module::require("ast")) {
        NEED_MODULE(Module_Ast);
    }
    if (!Module::require("jit")) {
        NEED_MODULE(Module_Jit);
    }
    if (!Module::require("debugapi")) {
        NEED_MODULE(Module_Debugger);
    }
    NEED_MODULE(Module_Network);
    NEED_MODULE(Module_UriParser);
    NEED_MODULE(Module_JobQue);
    NEED_MODULE(Module_FIO);
    NEED_MODULE(Module_DASBIND);
    require_project_specific_modules();
    #include "modules/external_need.inc"
    Module::Initialize();

    if (formatter) {
        return format::run(formatter.value(), files);
    }
    // compile and run
    int failedFiles = 0;
    for ( auto & fn : files ) {
        replace(fn, "_dasroot_", getDasRoot());
        if (!compile_and_run(fn, mainName, outputProgramCode, dryRun)) {
            failedFiles++;
        }
    }
    // and done
    if ( pauseAfterDone ) getchar();
    Module::Shutdown();
#if DAS_SMART_PTR_TRACKER
    if ( g_smart_ptr_total!=0 ) {
        TextPrinter tp;
        tp << "smart pointers leaked: " << uint64_t(g_smart_ptr_total) << "\n";
#if DAS_SMART_PTR_ID
        tp << "leaked ids:";
        vector<uint64_t> ids;
        for ( auto it : ptr_ref_count::ref_count_ids ) {
            ids.push_back(it);
        }
        std::sort(ids.begin(), ids.end());
        for ( auto it : ids ) {
            tp << " " << HEX << it << DEC;
        }
        tp << "\n";
#endif
        exit(1);
    }
#endif
    return failedFiles;
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
