// llvm - the witness that this build was CONFIGURED with dasLLVM. The package itself is pure
// das (dasbind over LLVM.dll), so nothing else in it can answer "is LLVM part of this build" -
// its files sit on disk in every checkout, whatever DAS_LLVM_DISABLED says. This module is
// compiled in only when the package is, so `require ?llvm <target>` and
// `builtin_module_exists(llvm)` follow the configure, never the filesystem: a wasm or console
// build without LLVM loads no tune framework, no sidecar, no IR generator.

#include "daScript/daScript.h"

namespace das {

class Module_LLVM : public Module {
public:
    Module_LLVM() : Module("llvm") {
        ModuleLibrary lib(this);
        lib.addBuiltInModule();
        registerModuleGroupMember("tune_framework", "llvm/daslib/llvm_tune");
    }
    virtual ModuleAotType aotRequire ( TextWriter & ) const override {
        return ModuleAotType::cpp;
    }
};

REGISTER_DYN_MODULE(Module_LLVM, Module_LLVM);

} // namespace das

REGISTER_MODULE_IN_NAMESPACE(Module_LLVM, das);
