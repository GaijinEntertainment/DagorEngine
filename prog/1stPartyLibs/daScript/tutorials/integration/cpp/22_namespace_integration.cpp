// Tutorial 22 — Namespace Integration
//
// Demonstrates how to initialize daslang modules inside a C++ namespace.
//
// The standard NEED_MODULE / NEED_ALL_DEFAULT_MODULES macros place an
// `extern` declaration at the current scope.  When that scope is a C++
// namespace, the linker looks for MyNamespace::register_Module_BuiltIn()
// instead of the global ::register_Module_BuiltIn(), causing link errors.
//
// The solution is the DECLARE_MODULE / PULL_MODULE macro pair:
//   • DECLARE_MODULE  — forward-declares the register function (must be at
//                        file/global scope so it refers to the global symbol)
//   • PULL_MODULE     — calls the register function using ::qualified name
//                        (safe inside any namespace, class, or function body)
//
// Convenience wrappers DECLARE_ALL_DEFAULT_MODULES and PULL_ALL_DEFAULT_MODULES
// cover all built-in modules at once.

#include "daScript/daScript.h"

using namespace das;

#define SCRIPT_NAME "/tutorials/integration/cpp/01_hello_world.das"

// Step 1: Forward-declare module functions at global/file scope.
// This ensures the extern declarations refer to the global-scope symbols
// defined by REGISTER_MODULE / REGISTER_MODULE_IN_NAMESPACE in the library.
DECLARE_ALL_DEFAULT_MODULES;

// For external (plugin) modules, include the generated file:
//   #include "modules/external_declare.inc"
// (mirrors external_need.inc but uses DECLARE_MODULE instead of NEED_MODULE)

// ------------------------------------------------------------------
// Everything below lives inside a namespace — PULL_MODULE works here.
// ------------------------------------------------------------------
namespace MyApp {

void runScript() {
    TextPrinter tout;
    ModuleGroup dummyLibGroup;
    auto fAccess = make_smart<FsFileAccess>();
    auto program = compileDaScript(getDasRoot() + SCRIPT_NAME,
                                   fAccess, tout, dummyLibGroup);
    if (program->failed()) {
        tout << "Compilation failed:\n";
        for (auto & err : program->errors) {
            tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
        }
        return;
    }
    Context ctx(program->getContextStackSize());
    if (!program->simulate(ctx, tout)) {
        tout << "Simulation failed:\n";
        for (auto & err : program->errors) {
            tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
        }
        return;
    }
    auto fnTest = ctx.findFunction("test");
    if (!fnTest) {
        tout << "Function 'test' not found\n";
        return;
    }
    ctx.evalWithCatch(fnTest, nullptr);
    if (auto ex = ctx.getException()) {
        tout << "Script exception: " << ex << "\n";
    }
}

void initialize() {
    // Step 2: Pull (register) the modules.  PULL_ALL_DEFAULT_MODULES uses
    // ::register_Module_*() — the :: prefix ensures the call resolves to
    // the global-scope function regardless of the enclosing namespace.
    PULL_ALL_DEFAULT_MODULES;

    // For external (plugin) modules, include the generated file:
    //   #include "modules/external_pull.inc"
    // (mirrors external_need.inc but uses PULL_MODULE instead of NEED_MODULE)

    Module::Initialize();
}

void shutdown() {
    Module::Shutdown();
}

} // namespace MyApp

// main() delegates to the namespaced functions.
int main(int, char * []) {
    MyApp::initialize();
    MyApp::runScript();
    MyApp::shutdown();
    return 0;
}
