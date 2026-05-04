// Tutorial 17 — Coroutines (C++ integration)
//
// Shows how to consume a daslang generator (coroutine) from C++.
// A daslang function returns a generator<int> via "return <-".
// The C++ host receives a Sequence iterator and steps through it
// with builtin_iterator_iterate(), receiving one value per call.
//
// Key concepts:
//   - Sequence — the C++ type that wraps a daslang generator
//   - evalWithCatch with a third &Sequence parameter to capture generators
//   - builtin_iterator_iterate — single-step the generator
//   - builtin_iterator_close — clean up when done (or early-exit)
//
// Compare with tutorials/integration/c/ for the C API equivalent.

#include "daScript/daScript.h"

using namespace das;

#define SCRIPT_NAME "/tutorials/integration/cpp/17_coroutines.das"

void tutorial() {
    TextPrinter tout;
    ModuleGroup dummyLibGroup;
    auto fAccess = make_smart<FsFileAccess>();

    // --- Compile the script ---
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

    // --- Create context and simulate ---
    Context ctx(program->getContextStackSize());
    if (!program->simulate(ctx, tout)) {
        tout << "Simulation failed:\n";
        for (auto & err : program->errors) {
            tout << reportError(err.at, err.what, err.extra,
                                err.fixme, err.cerr);
        }
        return;
    }

    // --- Find the "test" function ---
    auto fnTest = ctx.findFunction("test");
    if (!fnTest) {
        tout << "Function 'test' not found\n";
        return;
    }

    // --- Call the function, capturing the returned generator ---
    //
    // When a daslang function returns a generator, we pass a pointer
    // to a Sequence as the third argument to evalWithCatch.
    // After the call, 'it' holds the iterator state.
    Sequence it;
    ctx.evalWithCatch(fnTest, nullptr, &it);
    if (auto ex = ctx.getException()) {
        tout << "Script exception: " << ex << "\n";
        return;
    }

    // --- Step through the generator one value at a time ---
    //
    // builtin_iterator_iterate() advances the generator to its next
    // yield point and writes the yielded value into the output buffer.
    // It returns true while there are more values to produce.
    //
    // The output buffer must be large enough to hold one value of the
    // generator's element type.  For generator<int> that is sizeof(int32_t).
    tout << "Stepping through the generator from C++:\n";
    int32_t value = 0;
    int step = 0;
    while (builtin_iterator_iterate(it, &value, &ctx)) {
        tout << "  [c++] step " << step << " => value " << value << "\n";
        step++;
    }

    // --- Clean up ---
    //
    // builtin_iterator_close releases any resources held by the Sequence.
    // Always call this, even if you iterated to completion — it is a no-op
    // if the generator already finished, but essential if you break out
    // of the loop early.
    // The second argument is a pointer to the output buffer (same as iterate).
    builtin_iterator_close(it, &value, &ctx);

    tout << "Generator produced " << step << " values total\n";
}

int main(int, char * []) {
    NEED_ALL_DEFAULT_MODULES;
    Module::Initialize();
    tutorial();
    Module::Shutdown();
    return 0;
}
