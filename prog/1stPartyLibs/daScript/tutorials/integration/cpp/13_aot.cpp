// Tutorial 13 — Ahead-of-Time Compilation (C++ integration)
//
// Demonstrates the AOT workflow:
//   - Compiling a daslang program in interpreter mode vs AOT mode
//   - The two-stage build: generate C++ from script, then link it in
//   - CodeOfPolicies::aot — enabling AOT linking during simulate
//   - How AOT functions self-register via AotListBase
//
// AOT generates C++ source code from daslang functions.  When linked
// into the host and policies.aot is true, simulate() replaces the
// interpreter's simulation nodes with direct calls to the generated
// native C++ functions, resulting in near-C++ performance.
//
// Build stages (handled by CMake in this project):
//   1. Compile daslang.exe (the compiler tool)
//   2. Run: daslang.exe utils/aot/main.das -- -aot script.das script.das.cpp
//      This transpiles daslang → C++ source
//   3. Compile the generated .cpp into the host executable
//   4. At runtime, call compileDaScript with policies.aot = true
//      simulate() links the AOT functions automatically
//
// The generated C++ file is compiled into this executable, so the
// AOT path below will show "test() is AOT: yes".

#include "daScript/daScript.h"

using namespace das;

// -----------------------------------------------------------------------
// Host program — AOT compilation demo
// -----------------------------------------------------------------------

#define SCRIPT_NAME "/tutorials/integration/cpp/13_aot.das"

void tutorial() {
    TextPrinter tout;
    ModuleGroup dummyLibGroup;
    auto fAccess = make_smart<FsFileAccess>();

    // --- Step 1: Compile WITHOUT AOT (interpreter mode) ---
    tout << "=== Interpreter mode ===\n";
    {
        auto program = compileDaScript(getDasRoot() + SCRIPT_NAME,
                                       fAccess, tout, dummyLibGroup);
        if (program->failed()) {
            for (auto & err : program->errors) {
                tout << reportError(err.at, err.what, err.extra,
                                    err.fixme, err.cerr);
            }
            return;
        }

        Context ctx(program->getContextStackSize());
        if (!program->simulate(ctx, tout)) {
            tout << "Simulation failed\n";
            return;
        }

        auto fnTest = ctx.findFunction("test");
        if (!fnTest) {
            tout << "Function 'test' not found\n";
            return;
        }

        // Check if function is AOT-linked
        tout << "  test() is AOT: "
             << (fnTest->aot ? "yes" : "no") << "\n";

        ctx.evalWithCatch(fnTest, nullptr);
        if (auto ex = ctx.getException()) {
            tout << "Exception: " << ex << "\n";
        }
    }

    // --- Step 2: Compile WITH AOT policies enabled ---
    //
    // The generated AOT C++ is compiled into this executable.
    // With policies.aot = true, simulate() links functions to their
    // native C++ implementations via the static AotListBase registry.
    tout << "\n=== AOT mode (policies.aot = true) ===\n";
    {
        CodeOfPolicies policies;
        policies.aot = true;
        // fail_on_no_aot = true — error if any function lacks AOT
        policies.fail_on_no_aot = true;

        auto program = compileDaScript(getDasRoot() + SCRIPT_NAME,
                                       fAccess, tout, dummyLibGroup,
                                       policies);
        if (program->failed()) {
            for (auto & err : program->errors) {
                tout << reportError(err.at, err.what, err.extra,
                                    err.fixme, err.cerr);
            }
            return;
        }

        Context ctx(program->getContextStackSize());
        if (!program->simulate(ctx, tout)) {
            tout << "Simulation failed\n";
            return;
        }

        auto fnTest = ctx.findFunction("test");
        if (!fnTest) {
            tout << "Function 'test' not found\n";
            return;
        }

        tout << "  test() is AOT: "
             << (fnTest->aot ? "yes" : "no") << "\n";

        ctx.evalWithCatch(fnTest, nullptr);
        if (auto ex = ctx.getException()) {
            tout << "Exception: " << ex << "\n";
        }
    }

    // --- Summary ---
    tout << "\n=== AOT workflow summary ===\n";
    tout << "1. daslang.exe -aot script.das script.das.cpp\n";
    tout << "   -> generates C++ source from daslang functions\n";
    tout << "2. Compile the .cpp into your executable\n";
    tout << "   -> functions self-register via static AotListBase\n";
    tout << "3. Set CodeOfPolicies::aot = true before compileDaScript\n";
    tout << "   -> simulate() links AOT functions automatically\n";
    tout << "4. Functions run as native C++ — no interpreter overhead\n";
}

int main(int, char * []) {
    NEED_ALL_DEFAULT_MODULES;
    Module::Initialize();
    tutorial();
    Module::Shutdown();
    return 0;
}
