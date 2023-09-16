#include "daScript/daScript.h"

using namespace das;

// function, which we are going to expose to daScript
float xmadd ( float a, float b, float c, float d ) {
    return a*b + c*d;
}

// making custom builtin module
class Module_Tutorial02 : public Module {
public:
    Module_Tutorial02() : Module("tutorial_02") {   // module name, when used from das file
        ModuleLibrary lib(this);
        lib.addBuiltInModule();
        // adding constant to the module
        addConstant(*this,"SQRT2",sqrtf(2.0));
        // adding function to the module
        addExtern<DAS_BIND_FUN(xmadd)>(*this, lib, "xmadd",SideEffects::none, "xmadd");
    }
};

// registering module, so that its available via 'NEED_MODULE' macro
REGISTER_MODULE(Module_Tutorial02);

#define TUTORIAL_NAME   "/examples/tutorial/tutorial02.das"

#include "tutorial.inc"

int main( int, char * [] ) {
    // request all da-script built in modules
    NEED_ALL_DEFAULT_MODULES;
    // request our custom module
    NEED_MODULE(Module_Tutorial02);
    // Initialize modules
    Module::Initialize();
    // run the tutorial
    tutorial();
    // shut-down daScript, free all memory
    Module::Shutdown();
    return 0;
}
