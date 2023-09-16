#include "daScript/daScript.h"

#include "tutorial02aot.h"

using namespace das;

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
        // and verify
        verifyAotReady();
    }
    virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
        // specifying which include files are required for this module
        tw << "#include \"tutorial02aot.h\"\n";
        // specifying AOT type, in this case direct cpp mode (and not hybrid mode)
        return ModuleAotType::cpp;
    }
};

// registering module, so that its available via 'NEED_MODULE' macro
REGISTER_MODULE(Module_Tutorial02);
