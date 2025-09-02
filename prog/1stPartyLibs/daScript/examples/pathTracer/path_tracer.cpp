#include "daScript/daScript.h"

using namespace das;

#define PATHTRACER_NAME   "/examples/pathTracer/toy_path_tracer_profile.das"

namespace das { vector<void *> force_aot_stub(); }

void pathtracer () {
    TextPrinter tout;                               // output stream for all compiler messages (stdout. for stringstream use TextWriter)
    ModuleGroup dummyLibGroup;                      // module group for compiled program
    CodeOfPolicies policies;                        // compiler setup
    auto fAccess = make_smart<FsFileAccess>();      // default file access
    policies.aot = true;
    policies.fail_on_no_aot = false;
    // compile program
    auto program = compileDaScript(getDasRoot() + PATHTRACER_NAME, fAccess, tout, dummyLibGroup, policies);
    if ( program->failed() ) {
        // if compilation failed, report errors
        tout << "failed to compile\n";
        for ( auto & err : program->errors ) {
            tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
        }
        return;
    }
    // create daScript context
    Context ctx(program->getContextStackSize());
    if ( !program->simulate(ctx, tout) ) {
        // if interpretation failed, report errors
        tout << "failed to simulate\n";
        for ( auto & err : program->errors ) {
            tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
        }
        return;
    }
    // find function 'test' in the context
    auto fnTest = ctx.findFunction("main");
    if ( !fnTest ) {
        tout << "function 'test' not found\n";
        return;
    }
    // verify if 'test' is a function, with the correct signature
    // note, this operation is slow, so don't do it every time for every call
    if ( !verifyCall<void>(fnTest->debugInfo, dummyLibGroup) ) {
        tout << "function 'test', call arguments do not match. expecting def test : void\n";
        return;
    }
    // evaluate 'test' function in the context
    ctx.eval(fnTest, nullptr);
    if ( auto ex = ctx.getException() ) {       // if function cased panic, report it
        tout << "exception: " << ex << "\n";
        return;
    }
}

namespace das { vector<void *> force_aot_stub(); }

int main( int, char * [] ) {
    // force libDaScriptAot linking
    force_aot_stub();
    // request all da-script built in modules
    NEED_ALL_DEFAULT_MODULES;
    // request external modules
    #include "modules/external_need.inc"
    // add job-que
    if (!Module::require("jobque")) {
        NEED_MODULE(Module_JobQue);
    }
    // Initialize modules
    Module::Initialize();
    // run the path-tracer
    pathtracer();
    // shut-down daScript, free all memory
    Module::Shutdown();
    return 0;
}
