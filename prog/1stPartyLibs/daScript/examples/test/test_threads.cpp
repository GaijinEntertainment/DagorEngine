#include "daScript/daScript.h"
#include "daScript/simulate/fs_file_info.h"
#include "daScript/misc/performance_time.h"
#include "daScript/misc/fpe.h"
#include "daScript/misc/sysos.h"

#ifdef _MSC_VER
#include <io.h>
#else
#include <dirent.h>
#endif

using namespace das;

#include <sstream>
#include <thread>

bool VerboseTests = false;
bool AnyNoiseInTests = true;

unique_ptr<FsReadOnlyCachedFileSystem> cache_fs;

string this_thread_id() {
    stringstream ss;
    ss << this_thread::get_id();
    return ss.str();
}

bool g_reportCompilationFailErrors = false;

bool performance_test ( const string & fn, bool useAot ) {
    TextPrinter tout;
    if ( VerboseTests )
        tout << "[" << this_thread_id() << "]";
    // tout << fn << "\n";
    auto fAccess = make_smart<FsFileAccess>();
    fAccess->addFileSystem(cache_fs.get(), true, true);
    ModuleGroup dummyLibGroup;
    CodeOfPolicies policies;
    policies.aot = useAot;
    policies.fail_on_no_aot = true;
    // policies.intern_strings = true;
    // policies.intern_const_strings = true;
    // policies.no_unsafe = true;
    if ( auto program = compileDaScript(fn, fAccess, tout, dummyLibGroup, policies) ) {
        if ( program->failed() ) {
            tout << fn << " failed to compile (" << this_thread_id() << ")\n";
            for ( auto & err : program->errors ) {
                tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
            }
            return false;
        } else {
            Context ctx(program->getContextStackSize());
            if ( !program->simulate(ctx, tout) ) {
                tout << fn << " failed to simulate(" << this_thread_id() << ")\n";
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


bool run_tests( const string & path, bool (*test_fn)(const string &, bool useAot), bool useAot ) {
#ifdef _MSC_VER
    bool ok = true;
    _finddata_t c_file;
    intptr_t hFile;
    string findPath = path + "/*.das";
    if ((hFile = _findfirst(findPath.c_str(), &c_file)) != -1L) {
        do {
            const char * atDas = strstr(c_file.name,".das");
            if ( atDas && strcmp(atDas,".das")==0 && c_file.name[0]!='_' ) {
                ok = test_fn(path + "/" + c_file.name, useAot) && ok;
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
                ok = test_fn(path + "/" + ent->d_name, useAot) && ok;
            }
        }
        closedir (dir);
    }
    return ok;
#endif
}


void require_project_specific_modules();

void test_thread(bool useAot) {
    ReuseCacheGuard guard;
    TextPrinter tout;
    if ( AnyNoiseInTests )
        tout << "test_thread: " << this_thread_id() << "\n";
    if ( VerboseTests )
        tout << "NEED MODULE (" << this_thread_id() << ")\n";
    uint64_t timeStamp0 = ref_time_ticks();
    // register modules
    NEED_MODULE(Module_BuiltIn);
    NEED_MODULE(Module_Math);
    NEED_MODULE(Module_Raster);
    NEED_MODULE(Module_Strings);
    NEED_MODULE(Module_Rtti);
    NEED_MODULE(Module_Ast);
    NEED_MODULE(Module_Debugger);
    NEED_MODULE(Module_Network);
    NEED_MODULE(Module_UriParser);
    NEED_MODULE(Module_JobQue);
    NEED_MODULE(Module_FIO);
    NEED_MODULE(Module_DASBIND);
    require_project_specific_modules();
    #include "modules/external_need.inc"
    if ( VerboseTests )
        tout << "Module::Initialize (" << this_thread_id() << ")\n";
    Module::Initialize();
    int usec0 = get_time_usec(timeStamp0);
    if ( AnyNoiseInTests )
        tout << "Initialized in " << ((usec0/1000)/1000.0) << " (" << this_thread_id() << ")\n";
    // run em
    uint64_t timeStamp = ref_time_ticks();
#if 1
    performance_test(getDasRoot() +  "/modules/dasImgui/greyprint/greyprint.das", useAot );
    int usec = get_time_usec(timeStamp);
    if ( AnyNoiseInTests )
        tout << "Compiled in " << ((usec/1000)/1000.0) << " (" << this_thread_id() << ")\n";
#else
    if ( !run_tests(getDasRoot() +  "/examples/test/unit_tests", performance_test, useAot) ) {
        if ( VerboseTests )
            tout << "TESTS FAILED (" << this_thread_id() << ")\n";
    } else {
        int usec = get_time_usec(timeStamp);
        if ( AnyNoiseInTests )
            tout << "Passed in " << ((usec/1000)/1000.0) << " (" << this_thread_id() << ")\n";
    }
#endif
    if ( VerboseTests )
        tout << "Module::Shutdown (" << this_thread_id() << ")\n";
    Module::Shutdown();
}

#include <thread>

int main( int argc, char * argv[] ) {
    TextPrinter tout;
    if ( argc>2 ) {
        tout << "daScriptTestThreads [pathToDasRoot]\n";
        return -1;
    }  else if ( argc==2 ) {
        setDasRoot(argv[1]);
    }
    setCommandLineArguments(argc,argv);

    cache_fs.reset(new FsReadOnlyCachedFileSystem());

    for ( int use_aot=0; use_aot!=1; use_aot++ ) {
        #if 1   // for verbose version
            if ( AnyNoiseInTests )
                tout << (use_aot ? "AOT " : "") << "Baseline:\n";
            test_thread(use_aot!=0);
        #endif
        vector<thread> THREADS;
        auto total_threads = max(1, int(thread::hardware_concurrency()-2));
        if ( AnyNoiseInTests )
            tout << (use_aot ? "AOT " : "") << "Threaded on " << total_threads << " threads:\n";
        for ( int i=0; i<total_threads; ++i ) {
            THREADS.emplace_back(thread([=](){
                for (int j = 0; j != 4; ++j) {
                    test_thread(use_aot != 0);
                }
            }));
        }
        test_thread(use_aot!=0);
        for ( auto & th : THREADS ) {
            th.join();
        }
    }

    cache_fs.release();

    return 0;
}

