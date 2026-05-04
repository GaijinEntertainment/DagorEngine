#pragma once
#include "daScript/misc/platform.h" // DAS_THREAD_LOCAL

namespace das
{
    inline DAS_THREAD_LOCAL(unsigned) ModuleKarma;
    class Module;

    // This function registers all builtin modules.
    // Builtin modules are modules listed in /src/builtin/
    // Note: this is similar to NEED_ALL_DEFAULT_MODULES
    // but more safe, since it allows such modules to be
    // already registered.
    DAS_API void register_builtin_modules();

};

#define NEED_MODULE(ClassName) \
    extern DAS_API das::Module * register_##ClassName (); \
    *das::ModuleKarma += unsigned(intptr_t(register_##ClassName()));

#define NEED_ALL_DEFAULT_MODULES \
    NEED_MODULE(Module_BuiltIn); \
    NEED_MODULE(Module_Math); \
    NEED_MODULE(Module_Strings); \
    NEED_MODULE(Module_Rtti); \
    NEED_MODULE(Module_Ast); \
    NEED_MODULE(Module_Debugger); \
    NEED_MODULE(Module_Jit); \
    NEED_MODULE(Module_FIO); \
    NEED_MODULE(Module_DASBIND); \
    NEED_MODULE(Module_Network);

// DECLARE_MODULE / PULL_MODULE — namespace-safe alternatives to NEED_MODULE.
//
// NEED_MODULE places an `extern` declaration at the current scope, which
// fails inside a C++ namespace (the linker looks for Namespace::register_…
// instead of the global-scope function).
//
// Use DECLARE_MODULE at global/file scope to forward-declare the register
// function, then PULL_MODULE inside any namespace or function body to call it.
//
// Example:
//   DECLARE_ALL_DEFAULT_MODULES;               // file scope
//   namespace MyApp {
//       void init() {
//           PULL_ALL_DEFAULT_MODULES;           // OK — works inside namespace
//           das::Module::Initialize();
//       }
//   }

#define DECLARE_MODULE(ClassName) \
    extern DAS_API das::Module * register_##ClassName ()

#define PULL_MODULE(ClassName) \
    *das::ModuleKarma += unsigned(intptr_t(::register_##ClassName()))

#define DECLARE_ALL_DEFAULT_MODULES \
    DECLARE_MODULE(Module_BuiltIn); \
    DECLARE_MODULE(Module_Math); \
    DECLARE_MODULE(Module_Strings); \
    DECLARE_MODULE(Module_Rtti); \
    DECLARE_MODULE(Module_Ast); \
    DECLARE_MODULE(Module_Debugger); \
    DECLARE_MODULE(Module_Jit); \
    DECLARE_MODULE(Module_FIO); \
    DECLARE_MODULE(Module_DASBIND); \
    DECLARE_MODULE(Module_Network)

#define PULL_ALL_DEFAULT_MODULES \
    PULL_MODULE(Module_BuiltIn); \
    PULL_MODULE(Module_Math); \
    PULL_MODULE(Module_Strings); \
    PULL_MODULE(Module_Rtti); \
    PULL_MODULE(Module_Ast); \
    PULL_MODULE(Module_Debugger); \
    PULL_MODULE(Module_Jit); \
    PULL_MODULE(Module_FIO); \
    PULL_MODULE(Module_DASBIND); \
    PULL_MODULE(Module_Network)

