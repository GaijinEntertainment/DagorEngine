#include "daScript/daScript.h"
#include "daScript/simulate/fs_file_info.h"
#include "daScript/misc/sysos.h"

#ifdef _MSC_VER
#include <io.h>
#else
#include <dirent.h>
#endif

using namespace das;

#include "test_profile.h"

MAKE_TYPE_FACTORY(Object, Object)

namespace das {

template <>
struct typeName<ObjectArray> {
    constexpr static const char * name() {
        return "ObjectArray";
    }
};

}

TextPrinter tout;

bool unit_test ( const string & fn, bool useAOT ) {
    // make sure there is no stack
    CodeOfPolicies policies;
    policies.aot = useAOT;
    policies.stack = 0;
    auto access = make_smart<FsFileAccess>();
    ModuleGroup dummyGroup;
    if ( auto program = compileDaScript(fn,access,tout,dummyGroup,policies) ) {
        if ( program->failed() ) {
            tout << fn << " failed to compile\n";
            for ( auto & err : program->errors ) {
                tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
            }
            return false;
        } else {
            // tout << *program << "\n";
            Context ctx(program->getContextStackSize());
            if ( !program->simulate(ctx, tout) ) {
                tout << "failed to simulate\n";
                for ( auto & err : program->errors ) {
                    tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
                }
                return false;
            }
            // run the test
            if ( auto fnTest = ctx.findFunction("test") ) {
                StackAllocator sharedStack(8*1024);
                SharedStackGuard guard(ctx, sharedStack);
                bool result;
                if ( verifyCall<bool>(fnTest->debugInfo, dummyGroup) ) {
                    result = cast<bool>::to(ctx.eval(fnTest, nullptr));
                } else if ( verifyCall<bool,vector<Object>>(fnTest->debugInfo, dummyGroup) ) {
                    vector<Object> objects;
                    objects.resize(10000);
                    vec4f args[1] = { cast<vector<Object> *>::from(&objects) };
                    result = cast<bool>::to(ctx.eval(fnTest, args));
                } else {
                    tout << "function 'test', call arguments do not match\n";
                    return false;
                }
                if ( auto ex = ctx.getException() ) {
                    tout << fn << ", exception: " << ex << "\n";
                    return false;
                }
                if ( !result ) {
                    tout << fn << ", failed\n";
                    return false;
                }
                // tout << "ok\n";
                return true;
            } else {
                tout << fn << ", function 'test' not found\n";
                return false;
            }
        }
    } else {
        return false;
    }
}

bool run_tests( const string & path, bool (*test_fn)(const string &, bool aot), bool useAot ) {
    vector<string> files;
#ifdef _MSC_VER
    _finddata_t c_file;
    intptr_t hFile;
    string findPath = path + "/*.das";
    if ((hFile = _findfirst(findPath.c_str(), &c_file)) != -1L) {
        do {
            files.push_back(path + "/" + c_file.name);
        } while (_findnext(hFile, &c_file) == 0);
    }
    _findclose(hFile);
#else
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (path.c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            const char * atDas = strstr(ent->d_name,".das");
            if ( atDas && strcmp(atDas,".das")==0 ) {
                files.push_back(path + "/" + ent->d_name);
            }
        }
        closedir (dir);
    }
#endif
    sort(files.begin(),files.end());
    bool ok = true;
    for ( auto & fn : files ) {
        ok = test_fn(fn,useAot) && ok;
    }
    return ok;
}

int main( int argc, char * argv[] ) {
    if ( argc>2 ) {
        tout << "daScriptTest [pathToDasRoot]\n";
        return -1;
    }  else if ( argc==2 ) {
        setDasRoot(argv[1]);
    }
    setCommandLineArguments(argc,argv);
    _mm_setcsr((_mm_getcsr()&~_MM_ROUND_MASK) | _MM_FLUSH_ZERO_MASK | _MM_ROUND_NEAREST | 0x40);//0x40
    // register modules
    NEED_MODULE(Module_BuiltIn);
    NEED_MODULE(Module_Math);
    NEED_MODULE(Module_Raster);
    NEED_MODULE(Module_Strings);
    NEED_MODULE(Module_TestProfile);
    Module::Initialize();
#if 0
    auto TEST_NAME = getDasRoot() +  "/examples/profile/tests/nbodies.das";
    tout << "\nINTERPRETED:\n";
    unit_test(TEST_NAME,false);
    tout << "\nAOT:\n";
    unit_test(TEST_NAME,true);
    Module::Shutdown();
    return 0;
#endif
    // run tests
    if (argc == 1) {
        tout << "\nINTERPRETED:\n";
        run_tests(getDasRoot() + "/examples/profile/tests", unit_test, false);
        tout << "\nAOT:\n";
        run_tests(getDasRoot() + "/examples/profile/tests", unit_test, true);
    }
    for ( int i=1; i!=argc; ++i ) {
        string path=argv[i];
        unit_test(path,false);
    }
    // and done
    Module::Shutdown();
    return 0;
}
