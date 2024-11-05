#include "daScript/daScript.h"
#include "daScript/ast/ast_serializer.h"

using namespace das;

// function, which we are going to expose to daScript
float xmadd ( float a, float b, float c, float d ) {
    return a*b + c*d;
}

// making custom builtin module
class Module_Tutorial08 : public Module {
public:
    Module_Tutorial08() : Module("tutorial_08") {   // module name, when used from das file
        ModuleLibrary lib;
        lib.addModule(this);
        lib.addBuiltInModule();
        // adding constant to the module
        addConstant(*this,"SQRT2",sqrtf(2.0));
        // adding function to the module
        addExtern<DAS_BIND_FUN(xmadd)>(*this, lib, "xmadd",SideEffects::none, "xmadd");
    }
};

// registering module, so that its available via 'NEED_MODULE' macro
REGISTER_MODULE(Module_Tutorial08);

// #define TUTORIAL_NAME   "/examples/tutorial/tutorial08.das"
// #define TUTORIAL_NAME   "/examples/test/unit_tests/abc.das"
// #define TUTORIAL_NAME   "/examples/test/unit_tests/access_private_from_lambda.das"
// #define TUTORIAL_NAME   "/examples/test/unit_tests/aka.das"
#define TUTORIAL_NAME   "/examples/test/unit_tests/aonce.das"

void tutorial () {
    TextPrinter tout;                               // output stream for all compiler messages (stdout. for stringstream use TextWriter)
    ModuleGroup dummyLibGroup;                      // module group for compiled program
    auto fAccess = make_smart<FsFileAccess>();      // default file access
    // compile program
    auto program = compileDaScript(getDasRoot() + TUTORIAL_NAME, fAccess, tout, dummyLibGroup);
    if ( program->failed() ) {
        // if compilation failed, report errors
        tout << "failed to compile\n";
        for ( auto & err : program->errors ) {
            tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
        }
        return;
    }
// serialize
    AstSerializer ser;
    program->serialize(ser);
    program.reset();
// deserialize
    AstSerializer deser ( ForReading{});
    deser.buffer = das::move(ser.buffer);
    auto new_program = make_smart<Program>();
    new_program->serialize(deser);
    program = new_program;

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
    if ( !verifyCall<bool>(fnTest->debugInfo, dummyLibGroup) ) {
        tout << "function 'test', call arguments do not match. expecting def test : void\n";
        return;
    }
    ctx.restart();
    bool result = cast<bool>::to(ctx.eval(fnTest, nullptr));
    if ( auto ex = ctx.getException() ) {
        tout << "exception: " << ex << "\n";
        return;
    }
    if ( !result ) {
        tout << "failed\n";
        return;
    }
    tout << "ok\n";
}

int main( int, char * [] ) {
    // request all da-script built in modules
    NEED_ALL_DEFAULT_MODULES;
    // request our custom module
    NEED_MODULE(Module_Tutorial08);
    // Initialize modules
    Module::Initialize();
    // run the tutorial
    tutorial();
    // shut-down daScript, free all memory
    Module::Shutdown();
#if DAS_SMART_PTR_TRACKER
    if ( g_smart_ptr_total ) {
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
    }
#endif
    return 0;
}
