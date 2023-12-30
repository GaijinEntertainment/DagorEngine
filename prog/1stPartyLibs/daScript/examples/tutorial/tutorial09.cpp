#include "daScript/daScript.h"

using namespace das;

GcRootLambda rootLambda;

void setLambda ( Lambda lmb, Context * ctx ) {
    // store lambda in global variable, which is only visible from the C++ side
    printf("setting lambda from C++ side\n");
    rootLambda = GcRootLambda(lmb,ctx);
}

// making custom builtin moGcdule
class Module_Tutorial09 : public Module {
public:
    Module_Tutorial09() : Module("tutorial_09") {   // module name, when used from das file
        ModuleLibrary lib(this);
        lib.addBuiltInModule();
        // register function 'set_lambda' with the module
        addExtern<DAS_BIND_FUN(setLambda)>(*this,lib,"set_lambda",SideEffects::modifyExternal, "setLambda");
    }
};

// registering module, so that its available via 'NEED_MODULE' macro
REGISTER_MODULE(Module_Tutorial09);

#define TUTORIAL_NAME   "/examples/tutorial/tutorial09.das"

void tutorial () {
    TextPrinter tout;                               // output stream for all compiler messages (stdout. for stringstream use TextWriter)
    ModuleGroup dummyLibGroup;                      // module group for compiled program
    auto fAccess = make_smart<FsFileAccess>();      // default file access
    CodeOfPolicies policies;                        // default policies
    policies.persistent_heap = true;                // enable persistent heap for GC purposes
    // compile program
    auto program = compileDaScript(getDasRoot() + TUTORIAL_NAME, fAccess, tout, dummyLibGroup, policies);
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
    auto fnTest = ctx.findFunction("test");
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
    for ( int i=0; i!=3; ++i ) {
        // call garbage collector
        ctx.reportAnyHeap(nullptr,true,true,false,false);
        ctx.collectHeap(nullptr,true,true);
        // now call lambda, if its there
        if ( rootLambda.capture ) {
            tout << "calling lambda\n";
            das_invoke_lambda<void>::invoke(&ctx, nullptr, rootLambda);
        }
        // now done
        ctx.evalWithCatch(fnTest, nullptr);
        if ( auto ex = ctx.getException() ) {       // if function cased panic, report it
            tout << "exception: " << ex << "\n";
            return;
        }
    }
    // reset root lambda
    rootLambda.reset();
}

int main( int, char * [] ) {
    // request all da-script built in modules
    NEED_ALL_DEFAULT_MODULES;
    // request our custom module
    NEED_MODULE(Module_Tutorial09);
    // Initialize modules
    Module::Initialize();
    // run the tutorial
    tutorial();
    // shut-down daScript, free all memory
    Module::Shutdown();
    return 0;
}
