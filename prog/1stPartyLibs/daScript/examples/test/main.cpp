#include "daScript/daScript.h"
#include "daScript/ast/ast_serializer.h"
#include "daScript/simulate/fs_file_info.h"
#include "daScript/misc/performance_time.h"
#include "daScript/misc/fpe.h"
#include "daScript/misc/sysos.h"

#ifdef _MSC_VER
#include <io.h>
#else
#include <dirent.h>
#endif

#if DAS_SMART_PTR_TRACKER
#include <inttypes.h>
#endif

using namespace das;

bool g_reportCompilationFailErrors = false;
bool g_collectSharedModules = true;
bool g_failOnSmartPtrLeaks = true;

TextPrinter tout;

template <typename F>
bool with_program_serialized ( F callback, ProgramPtr program ) {
// serialize
    AstSerializer ser;
    program->serialize(ser);
    program.reset();
// deserialize
    AstSerializer deser ( ForReading{}, move(ser.buffer) );
    auto new_program = make_smart<Program>();
    new_program->serialize(deser);
    program = new_program;
// run it
    return callback(program);
}

StandaloneContextNode * StandaloneContextNode::head = nullptr;

bool run_all_standalone_context_tests () {
    vector<Context *> contexts;
    while ( auto h = StandaloneContextNode::head ) {
        StandaloneContextNode::head = h->tail;
        contexts.push_back(h->regFn());
    }
    bool res = true;
    for ( auto context : contexts ) {
        auto fn_tests = context->findFunctions("test");
        DAS_ASSERTF(!fn_tests.empty(), "expected to see test inside a testing context");
        for ( auto fn_test: fn_tests ) {
            bool result = cast<bool>::to(context->eval(fn_test, nullptr));
            if ( auto ex = context->getException() ) {
                tout << "exception: " << ex << "\n";
                res = false;
            }
            if ( !result ) {
                tout << "failed\n";
                res = false;
            }
        }

    }
    return res;
}

bool compilation_fail_test ( const string & fn, bool, bool ) {
    uint64_t timeStamp = ref_time_ticks();
    tout << fn << " ";
    auto fAccess = make_smart<FsFileAccess>();
    ModuleGroup dummyLibGroup;
    if ( auto program = compileDaScript(fn, fAccess, tout, dummyLibGroup) ) {
        if ( program->failed() ) {
            // we allow circular module dependency to fly through
            if ( program->errors.size()==1 ) {
                if ( program->errors[0].cerr==CompilationError::module_not_found ) {
                    if ( fn.find("circular_module_dependency")!=string::npos ) {
                        tout << "ok\n";
                        return true;
                    }
                }
            }
            // regular error processing
            bool failed = false;
            auto errors = program->expectErrors;
            for ( auto err : program->errors ) {
                int count = -- errors[err.cerr];
                if ( g_reportCompilationFailErrors || count<0 ) {
                    tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
                }
                if ( count <0 ) {
                    failed = true;
                }
            }
            bool any_errors = false;
            for ( auto err : errors ) {
                if ( err.second > 0 ) {
                    any_errors = true;
                    break;
                }
            }
            if ( any_errors || failed ) {
                tout << "failed";
                if ( any_errors ) {
                    tout << ", expecting errors";
                    for ( auto terr : errors  ) {
                        if (terr.second > 0 ) {
                            tout << " " << int(terr.first) << ":" << terr.second;
                        }
                    }
                }
                tout << "\n";
                return false;
            }
            int usec = get_time_usec(timeStamp);
            tout << "ok " << ((usec/1000)/1000.0) << "\n";
            return true;
        } else {
            tout << "failed, compiled without errors\n";
            return false;
        }
    } else {
        tout << "failed, not expected to compile\n";
        return false;
    }
}

bool unit_test ( const string & fn, bool useAot, bool useSer ) {
    uint64_t timeStamp = ref_time_ticks();
    tout << fn << " ";
    auto fAccess = make_smart<FsFileAccess>();
    ModuleGroup dummyLibGroup;
    CodeOfPolicies policies;
    policies.aot = useAot;
    policies.fail_on_no_aot = false;
    // policies.intern_strings = true;
    // policies.intern_const_strings = true;
    // policies.no_unsafe = true;
    if ( auto program = compileDaScript(fn, fAccess, tout, dummyLibGroup, policies) ) {
        if ( program->failed() ) {
            tout << "failed to compile\n";
            for ( auto & err : program->errors ) {
                tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
            }
            return false;
        } else {
            auto F = [ &dummyLibGroup, timeStamp, useAot ] ( ProgramPtr program ) {
                if (program->unsafe) tout << "[unsafe] ";
                Context ctx(program->getContextStackSize());
                if ( !program->simulate(ctx, tout) ) {
                    tout << "failed to simulate\n";
                    for ( auto & err : program->errors ) {
                        tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
                    }
                    return false;
                }
                if ( auto fnTest = ctx.findFunction("test") ) {
                    if ( !verifyCall<bool>(fnTest->debugInfo, dummyLibGroup) ) {
                        tout << "function 'test', call arguments do not match\n";
                        return false;
                    }
                    ctx.restart();
                    bool result = cast<bool>::to(ctx.eval(fnTest, nullptr));
                    if ( auto ex = ctx.getException() ) {
                        tout << "exception: " << ex << "\n";
                        return false;
                    }
                    if ( !result ) {
                        tout << "failed\n";
                        return false;
                    }
                    int usec = get_time_usec(timeStamp);
                    tout << (useAot ? "ok AOT " : "ok ") << ((usec/1000)/1000.0) << "\n";
                    return true;
                } else {
                    tout << "function 'test' not found\n";
                    return false;
                }
            };
            if ( useSer ) {
                return with_program_serialized(F, program);
            } else {
                return F(program);
            }
        }
    } else {
        return false;
    }
}

bool exception_test ( const string & fn, bool useAot, bool ) {
    tout << fn << " ";
    auto fAccess = make_smart<FsFileAccess>();
    ModuleGroup dummyLibGroup;
    CodeOfPolicies policies;
    policies.aot = useAot;
    if ( auto program = compileDaScript(fn, fAccess, tout, dummyLibGroup, policies) ) {
        if ( program->failed() ) {
            tout << "failed to compile\n";
            for ( auto & err : program->errors ) {
                tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
            }
            return false;
        } else {
            Context ctx(program->getContextStackSize());
            if ( !program->simulate(ctx, tout) ) {
                tout << "failed to simulate\n";
                for ( auto & err : program->errors ) {
                    tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
                }
                return false;
            }
            if ( auto fnTest = ctx.findFunction("test") ) {
                if ( !verifyCall<bool>(fnTest->debugInfo, dummyLibGroup) ) {
                    tout << "function 'test', call arguments do not match\n";
                    return false;
                }
                ctx.restart();
                ctx.evalWithCatch(fnTest, nullptr);
                if ( auto ex = ctx.getException() ) {
                    tout << "with exception " << ex << ", " << (useAot ? "ok AOT\n" : "ok\n");
                    return true;
                }
                tout << "failed, finished without exception\n";
                return false;
            } else {
                tout << "function 'test' not found\n";
                return false;
            }
        }
    } else {
        return false;
    }
}

bool performance_test ( const string & fn, bool useAot ) {
    // tout << fn << "\n";
    auto fAccess = make_smart<FsFileAccess>();
    ModuleGroup dummyLibGroup;
    CodeOfPolicies policies;
    policies.aot = useAot;
    policies.fail_on_no_aot = true;
    // policies.intern_strings = true;
    // policies.intern_const_strings = true;
    // policies.no_unsafe = true;
    if ( auto program = compileDaScript(fn, fAccess, tout, dummyLibGroup, policies) ) {
        if ( program->failed() ) {
            tout << fn << " failed to compile\n";
            for ( auto & err : program->errors ) {
                tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
            }
            return false;
        } else {
            Context ctx(program->getContextStackSize());
            if ( !program->simulate(ctx, tout) ) {
                tout << fn << " failed to simulate\n";
                for ( auto & err : program->errors ) {
                    tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
                }
                return false;
            }
            return true;
        }
    } else {
        return false;
    }
}


bool run_tests( const string & path, bool (*test_fn)(const string &, bool useAot, bool useSer), bool useAot, bool useSer = false ) {
#ifdef _MSC_VER
    bool ok = true;
    _finddata_t c_file;
    intptr_t hFile;
    string findPath = path + "/*.das";
    if ((hFile = _findfirst(findPath.c_str(), &c_file)) != -1L) {
        do {
            const char * atDas = strstr(c_file.name,".das");
            if ( atDas && strcmp(atDas,".das")==0 && c_file.name[0]!='_' ) {
                ok = test_fn(path + "/" + c_file.name, useAot, useSer) && ok;
                if ( g_collectSharedModules ) Module::CollectSharedModules();
            }
        } while (_findnext(hFile, &c_file) == 0);
    }
    _findclose(hFile);
    return ok;
#else
    bool ok = true;
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (path.c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            const char * atDas = strstr(ent->d_name,".das");
            if ( atDas && strcmp(atDas,".das")==0 && ent->d_name[0]!='_' ) {
                ok = test_fn(path + "/" + ent->d_name, useAot, useSer) && ok;
            }
        }
        closedir (dir);
    }
    return ok;
#endif
}

bool run_unit_tests( const string & path, bool check_aot = false, bool useSer = false ) {
    if ( check_aot ) {
        return run_tests(path, unit_test, false, useSer ) && run_tests(path, unit_test, true, useSer);
    } else {
        return run_tests(path, unit_test, false, useSer);
    }
}

bool isolated_unit_test ( const string & fn, bool useAot, bool useSer ) {
    // register modules
    g_collectSharedModules = false;
    NEED_MODULE(Module_BuiltIn);
    NEED_MODULE(Module_Math);
    NEED_MODULE(Module_Raster);
    NEED_MODULE(Module_Strings);
    NEED_MODULE(Module_UnitTest);
    NEED_MODULE(Module_Rtti);
    NEED_MODULE(Module_Ast);
    NEED_MODULE(Module_Debugger);
    NEED_MODULE(Module_Network);
    NEED_MODULE(Module_UriParser);
    NEED_MODULE(Module_JobQue);
    NEED_MODULE(Module_FIO);
    NEED_MODULE(Module_DASBIND);
    Module::Initialize();
    bool result = unit_test(fn,useAot, useSer);
    // shutdown
    Module::Shutdown();
#if DAS_SMART_PTR_TRACKER
    if ( g_smart_ptr_total ) {
        if ( g_failOnSmartPtrLeaks ) {
            result = false;
        }
        TextPrinter tp;
        tp << "smart pointers leaked: " << uint64_t(g_smart_ptr_total) << "\n";
    }
#endif
    return result;
}

bool run_isolated_unit_tests( const string & path, bool check_aot = false, bool use_ser = false ) {
    if ( check_aot ) {
        return run_tests(path, isolated_unit_test, false, use_ser) && run_tests(path, unit_test, true, use_ser);
    } else {
        return run_tests(path, isolated_unit_test, false, use_ser);
    }
}

bool run_compilation_fail_tests( const string & path ) {
    return run_tests(path, compilation_fail_test, false, false);
}

bool run_exception_tests( const string & path ) {
    return run_tests(path, exception_test, false, false) && run_tests(path, exception_test, true, false);
}


bool run_module_test ( const string & path, const string & main, bool usePak, bool useSer ) {
    tout << "testing MODULE at " << path << " ";
    auto fAccess = usePak ?
            make_smart<FsFileAccess>( path + "/project.das_project", make_smart<FsFileAccess>()) :
            make_smart<FsFileAccess>();
    ModuleGroup dummyLibGroup;
    if ( auto program = compileDaScript(path + "/" + main, fAccess, tout, dummyLibGroup) ) {
        if ( program->failed() ) {
            tout << "failed to compile\n";
            for ( auto & err : program->errors ) {
                tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
            }
            return false;
        } else {
            auto F = [&dummyLibGroup] ( ProgramPtr program ) {
                Context ctx(program->getContextStackSize());
                if ( !program->simulate(ctx, tout) ) {
                    tout << "failed to simulate\n";
                    for ( auto & err : program->errors ) {
                        tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
                    }
                    return false;
                }
                if ( auto fnTest = ctx.findFunction("test") ) {
                    if ( !verifyCall<bool>(fnTest->debugInfo, dummyLibGroup) ) {
                        tout << "function 'test', call arguments do not match\n";
                        return false;
                    }
                    ctx.restart();
                    bool result = cast<bool>::to(ctx.eval(fnTest, nullptr));
                    if ( auto ex = ctx.getException() ) {
                        tout << "exception: " << ex << "\n";
                        return false;
                    }
                    if ( !result ) {
                        tout << "failed\n";
                        return false;
                    }
                    tout << "ok\n";
                    return true;
                } else {
                    tout << "function 'test' not found\n";
                    return false;
                }
            };
            if ( useSer ) {
                return with_program_serialized(F, program);
            } else {
                return F(program);
            }
        }
    } else {
        return false;
    }
}

int main( int argc, char * argv[] ) {
    if ( argc>2 ) {
        tout << "daScriptTest [pathToDasRoot]\n";
        return -1;
    }  else if ( argc==2 ) {
        setDasRoot(argv[1]);
    }
    setCommandLineArguments(argc,argv);
    // ptr_ref_count::ref_count_track = 0x1242c;
    // das_track_string_breakpoint(189);
    // das_track_breakpoint(8);
    // _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    // _CrtSetBreakAlloc(6836533);
#if defined(_MSC_VER) || defined(__i386__)
    _mm_setcsr((_mm_getcsr()&~_MM_ROUND_MASK) | _MM_FLUSH_ZERO_MASK | _MM_ROUND_NEAREST | 0x40);//0x40
    FPE_ENABLE_ALL;
#endif
    // isolated uint tests
    // those are to test memory leaks on individual tests
#if 0
    run_isolated_unit_tests(getDasRoot() +  "/examples/test/unit_tests", false);
    return 0;
#endif
    // register modules
    NEED_MODULE(Module_BuiltIn);
    NEED_MODULE(Module_Math);
    NEED_MODULE(Module_Raster);
    NEED_MODULE(Module_Strings);
    NEED_MODULE(Module_UnitTest);
    NEED_MODULE(Module_Rtti);
    NEED_MODULE(Module_Ast);
    NEED_MODULE(Module_Debugger);
    NEED_MODULE(Module_Network);
    NEED_MODULE(Module_UriParser);
    NEED_MODULE(Module_JobQue);
    NEED_MODULE(Module_FIO);
    NEED_MODULE(Module_DASBIND);
    Module::Initialize();
    // aot library
#if 0 // Debug this one test
    compilation_fail_test(getDasRoot() + "/examples/test/compilation_fail_tests/smart_ptr.das",true);
    Module::Shutdown();
    getchar();
    return 0;
#endif
#if 0 // Debug this one test
// generators
    // #define TEST_NAME   "/dasgen/gen_bind.das"
    // #define TEST_NAME   "/doc/reflections/das2rst.das"
// examples
    // #define TEST_NAME   "/examples/test/dict_pg.das"
    // #define TEST_NAME   "/examples/test/hello_world.das"
    // #define TEST_NAME   "/examples/test/ast_print.das"
    // #define TEST_NAME   "/examples/test/base64.das"
    // #define TEST_NAME   "/examples/test/regex_lite.das"
    // #define TEST_NAME   "/examples/test/hello_world.das"
    // #define TEST_NAME   "/examples/test/json_example.das"
    // #define TEST_NAME   "/examples/test/ast_print.das"
    // #define TEST_NAME   "/examples/test/apply_example.das"
    // #define TEST_NAME   "/examples/test/unit_tests/hint_macros_example.das"
    // #define TEST_NAME   "/examples/test/unit_tests/aonce.das"
    // #define TEST_NAME   "/examples/test/unit_tests/check_defer.das"
    #define TEST_NAME   "/examples/test/unit_tests/return_reference.das"
    unit_test(getDasRoot() +  TEST_NAME,false);
    unit_test(getDasRoot() +  TEST_NAME,true);
    // extra
    //  #define TEST_NAME   "/examples/test/unit_tests/apply_macro_example.das"
    //  unit_test(getDasRoot() +  TEST_NAME,false);
    Module::Shutdown();
#if DAS_ENABLE_SMART_PTR_TRACKING
    dumpTrackingLeaks();
#endif
    getchar();
    return 0;
#endif
#if 0 // Module test
    run_module_test(getDasRoot() +  "/examples/test/module", "main_inc.das", true);
    Module::Shutdown();
    getchar();
    return 0;
#endif
#if 0 // COMPILER PERFORMANCE TESTS
    {
        uint64_t timeStamp = ref_time_ticks();
        int tmin = INT_MAX;
        for ( int passes=0; passes!=20; ++passes ) {
            uint64_t timeStampM = ref_time_ticks();
            if ( !run_tests(getDasRoot() +  "/examples/test/unit_tests", performance_test, true) ) {
                tout << "TESTS FAILED\n";
                break;
            }
            int usecM = get_time_usec(timeStampM);
            tmin = min(tmin, usecM);
        }
        // shutdown
        int usec = get_time_usec(timeStamp);
        tout << "tests took " << ((usec/1000)/1000.0) << ", min pass " << ((tmin / 1000) / 1000.0) << "\n";
        Module::Shutdown();
    }
    return 0;
#endif
#if 0 // COMPILER PERFORMANCE SINGLE TEST
    #define TEST_NAME   "/examples/test/unit_tests/check_defer.das"
    {
        uint64_t timeStamp = ref_time_ticks();
        for ( int passes=0; passes!=5; ++passes ) {
            if ( !unit_test(getDasRoot() +  TEST_NAME,false) ) {
                tout << "TESTS FAILED\n";
                break;
            }
        }
        // shutdown
        int usec = get_time_usec(timeStamp);
        tout << "tests took " << ((usec/1000)/1000.0) << "\n";
        Module::Shutdown();
        getchar();
    }
    return 0;
#endif
    uint64_t timeStamp = ref_time_ticks();
    bool ok = true;
    ok = run_compilation_fail_tests(getDasRoot() + "/examples/test/compilation_fail_tests") && ok;
    ok = run_unit_tests(getDasRoot() +  "/examples/test/unit_tests",    true,  false) && ok;
    ok = run_unit_tests(getDasRoot() +  "/examples/test/optimizations", false, false) && ok;
    ok = run_exception_tests(getDasRoot() +  "/examples/test/runtime_errors") && ok;
    ok = run_module_test(getDasRoot() +  "/examples/test/module", "main.das",        true, false) && ok;
    ok = run_module_test(getDasRoot() +  "/examples/test/module", "main_inc.das",    true, false)  && ok;
    ok = run_module_test(getDasRoot() +  "/examples/test/module", "main_default.das", false, false) && ok;
    ok = run_module_test(getDasRoot() +  "/examples/test/module/alias",  "main.das", true, false) && ok;
    ok = run_module_test(getDasRoot() +  "/examples/test/module/cdp",    "main.das", true, false) && ok;
    ok = run_module_test(getDasRoot() +  "/examples/test/module/unsafe", "main.das", true, false) && ok;
    int usec = get_time_usec(timeStamp);
    tout << "TESTS " << (ok ? "PASSED " : "FAILED!!! ") << ((usec/1000)/1000.0) << "\n";
    // shutdown
    Module::Shutdown();
#if DAS_SMART_PTR_TRACKER
    if ( g_failOnSmartPtrLeaks ) {
        ok = ok && (g_smart_ptr_total==0);
    }
    TextPrinter tp;
    tp << "smart pointers leaked: " << uint64_t(g_smart_ptr_total) << "\n";
#endif
    return ok ? 0 : -1;
}
