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
    // Lives in libDaScript / libDaScriptDyn (compiler lib, DAS_CC_EXPORTS),
    // NOT the runtime lib — so the export tag must be DAS_CC_API, not DAS_API.
    // MSVC happens to link this even with the wrong tag; clang-mingw is strict
    // and reports an undefined symbol at exe link time.
    DAS_CC_API void register_builtin_modules();

};

// Registration entry points are part of the static-module ABI. Keep their
// language linkage explicit so non-C++ hosts (including D) can call the
// unmangled register_Module_* names. These declarations also make the
// built-in modules directly available to NEED_MODULE.
extern "C" {
    DAS_API das::Module * register_Module_BuiltIn ();
    DAS_API das::Module * register_Module_Math ();
    DAS_API das::Module * register_Module_Strings ();
    DAS_API das::Module * register_Module_Rtti ();
    DAS_API das::Module * register_Module_Ast ();
    DAS_API das::Module * register_Module_Debugger ();
    DAS_API das::Module * register_Module_Jit ();
    DAS_API das::Module * register_Module_FIO ();
    DAS_API das::Module * register_Module_DASBIND ();
    DAS_API das::Module * register_Module_Network ();
    DAS_API das::Module * register_Module_UriParser ();
    DAS_API das::Module * register_Module_JobQue ();
}

// Fusion is not a module registration entry point, but it is pulled alongside
// the default modules and likewise needs a global declaration.
DAS_CC_API void register_fusion ();

// Pull a module through its global C-linkage registration entry point. Built-in
// modules are declared above. A separately compiled custom module must first be
// declared with DECLARE_MODULE at file scope.
#define NEED_MODULE(ClassName) \
    extern DAS_API das::Module * das_pull_##ClassName (); \
    *das::ModuleKarma += unsigned(intptr_t(das_pull_##ClassName()));

#define NEED_FUSION \
    ::register_fusion()

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
    NEED_MODULE(Module_Network); \
    NEED_FUSION;

// DECLARE_MODULE / PULL_MODULE split declaration from registration. Use
// DECLARE_MODULE at global/file scope to forward-declare a separately compiled
// module, then PULL_MODULE (or NEED_MODULE) inside any namespace or function.
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
    extern "C" DAS_API das::Module * register_##ClassName ()

#define PULL_MODULE(ClassName) \
    NEED_MODULE(ClassName)

#define DECLARE_FUSION \
    extern DAS_CC_API void register_fusion()

#define PULL_FUSION \
    ::register_fusion()

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
    DECLARE_MODULE(Module_Network); \
    DECLARE_FUSION

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
    PULL_MODULE(Module_Network); \
    PULL_FUSION

