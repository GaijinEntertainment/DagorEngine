// Tutorial 01 — Hello World (C++ integration)
//
// The simplest C++ program that embeds and runs a daslang script.
// Demonstrates the full lifecycle:
//   1. Initialize the daslang runtime
//   2. Compile a script from a file
//   3. Create a context and simulate (link) the program
//   4. Find and call an exported function
//   5. Shut down and free all resources
//
// Compare with tutorials/integration/c/01_hello_world.c for the C API
// equivalent of the same steps.

#include "daScript/daScript.h"

using namespace das;

// Path to the script, relative to the daslang root directory.
#define SCRIPT_NAME "/tutorials/integration/cpp/01_hello_world.das"

void tutorial() {
    // --- Step 1: set up compilation infrastructure ---
    //
    // TextPrinter  — sends compiler/runtime messages to stdout.
    //                (Use TextWriter for an in-memory string buffer instead.)
    // ModuleGroup  — holds modules visible during compilation.
    // FsFileAccess — provides disk-based file I/O to the compiler.
    TextPrinter tout;
    ModuleGroup dummyLibGroup;
    auto fAccess = make_smart<FsFileAccess>();

    // --- Step 2: compile the script ---
    //
    // compileDaScript reads the .das file, parses it, type-checks it,
    // and returns a Program.  getDasRoot() returns the project root so
    // that we can build a full path to the script file.
    auto program = compileDaScript(getDasRoot() + SCRIPT_NAME,
                                   fAccess, tout, dummyLibGroup);
    if (program->failed()) {
        tout << "Compilation failed:\n";
        for (auto & err : program->errors) {
            tout << reportError(err.at, err.what, err.extra,
                                err.fixme, err.cerr);
        }
        return;
    }

    // --- Step 3: create a context and simulate the program ---
    //
    // A Context is the runtime environment: stack, globals, heap.
    // simulate() resolves function pointers, initializes globals,
    // and prepares everything for execution.
    Context ctx(program->getContextStackSize());
    if (!program->simulate(ctx, tout)) {
        tout << "Simulation failed:\n";
        for (auto & err : program->errors) {
            tout << reportError(err.at, err.what, err.extra,
                                err.fixme, err.cerr);
        }
        return;
    }

    // --- Step 4: find the "test" function ---
    //
    // Any function marked [export] in the script can be looked up by name.
    // The returned pointer is valid for the lifetime of the context.
    auto fnTest = ctx.findFunction("test");
    if (!fnTest) {
        tout << "Function 'test' not found\n";
        return;
    }

    // Optional: verify the function signature at runtime.
    // verifyCall<ReturnType, ArgTypes...> checks debug info against the
    // expected prototype.  This is slow, so only do it once — not on
    // every call in a hot loop.
    if (!verifyCall<void>(fnTest->debugInfo, dummyLibGroup)) {
        tout << "Function 'test' has wrong signature; "
                "expected def test() : void\n";
        return;
    }

    // --- Step 5: call the function ---
    //
    // evalWithCatch runs the function inside a try/catch so that a
    // daslang panic() does not crash the host process.
    // The second argument is an array of vec4f arguments (nullptr = none).
    ctx.evalWithCatch(fnTest, nullptr);
    if (auto ex = ctx.getException()) {
        tout << "Script exception: " << ex << "\n";
    }
}

int main(int, char * []) {
    // Request all default built-in modules (math, strings, etc.).
    NEED_ALL_DEFAULT_MODULES;
    // Initialize the module registry — must be called once, before
    // any compilation.
    Module::Initialize();

    // Run the tutorial.
    tutorial();

    // Shut down daslang and free all global state.
    // No daslang API calls are allowed after this.
    Module::Shutdown();
    return 0;
}
