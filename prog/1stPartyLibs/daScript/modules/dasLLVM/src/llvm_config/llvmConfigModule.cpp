#include "daScript/misc/platform.h"

#include "daScript/misc/performance_time.h"
#include "daScript/simulate/aot.h"
#include "daScript/simulate/aot_builtin_jit.h"
#include "daScript/simulate/aot_builtin_ast.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/misc/fpe.h"

extern bool llvm_jit_enabled ();
extern bool llvm_jit_all_functions ();

extern bool llvm_debug_result ();
extern bool llvm_debug_everything ();

extern bool llvm_jit_always_solid ();
extern bool llvm_jit_allow_unaligned_vector_read_out_of_bounds ();

namespace das {

    class Module_LlvmConfig : public Module {
    public:
        Module_LlvmConfig() : Module("llvm_config") {
            DAS_PROFILE_SECTION("Module_LlvmConfig");
            ModuleLibrary lib(this);
            lib.addBuiltInModule();

            addExtern<DAS_BIND_FUN(llvm_jit_enabled)>(*this, lib,  "llvm_jit_enabled",
                SideEffects::none, "llvm_jit_enabled");
            addExtern<DAS_BIND_FUN(llvm_jit_all_functions)>(*this, lib,  "llvm_jit_all_functions",
                SideEffects::none, "llvm_jit_all_functions");

            addExtern<DAS_BIND_FUN(llvm_debug_result)>(*this, lib,  "llvm_debug_result",
                SideEffects::none, "llvm_debug_result");
            addExtern<DAS_BIND_FUN(llvm_debug_everything)>(*this, lib,  "llvm_debug_everything",
                SideEffects::none, "llvm_debug_everything");

            addExtern<DAS_BIND_FUN(llvm_jit_always_solid)>(*this, lib,  "llvm_jit_always_solid",
                SideEffects::none, "llvm_jit_always_solid");
            addExtern<DAS_BIND_FUN(llvm_jit_allow_unaligned_vector_read_out_of_bounds)>(*this,
                lib,  "llvm_jit_allow_unaligned_vector_read_out_of_bounds",
                SideEffects::none, "llvm_jit_allow_unaligned_vector_read_out_of_bounds");

            // lets make sure its all aot ready
            verifyAotReady();
        }
    };
}

REGISTER_MODULE_IN_NAMESPACE(Module_LlvmConfig,das);
