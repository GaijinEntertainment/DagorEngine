// Tutorial 21 — Threading (C++ integration)
//
// Demonstrates two threading patterns for daslang host applications:
//
// Part A — Run on a worker thread
//   Compile a script on the main thread, clone the context, then run
//   the clone on a separate std::thread.  The daScript environment
//   (daScriptEnvironment) is thread-local, so it must be captured on
//   the main thread and re-bound on the worker.  A shared_ptr<Context>
//   ensures the clone outlives the thread.
//
// Part B — Compile on a worker thread
//   Create an entirely independent daScript environment on a new thread
//   (its own module registry, module initialization, compile, simulate,
//   execute, and shutdown).  This is the pattern used by test_threads.cpp
//   and is suitable for fully isolated scripting pipelines.
//
// Key concepts:
//   - daScriptEnvironment::getBound() / setBound() — TLS binding
//   - ReuseCacheGuard — thread-local free-list cache management
//   - ContextCategory::thread_clone — clone category for threading
//   - shared_ptr<Context> with sharedPtrContext = true
//   - NEED_ALL_DEFAULT_MODULES + Module::Initialize/Shutdown per thread

#include "daScript/daScript.h"
#include "daScript/misc/free_list.h"    // ReuseCacheGuard

#include <thread>
#include <cstdio>

using namespace das;

#define SCRIPT_NAME "/tutorials/integration/cpp/21_threading.das"

// ---------------------------------------------------------------------------
// compile_script — helper that compiles and simulates the tutorial script,
// returning a (program, context) pair.  Returns nullptr program on failure.
// ---------------------------------------------------------------------------
static ProgramPtr compile_script(TextPrinter & tout) {
    ModuleGroup dummyLibGroup;
    CodeOfPolicies policies;
    policies.threadlock_context = true;  // context will run on another thread — needs mutex
    auto fAccess = make_smart<FsFileAccess>();
    auto program = compileDaScript(getDasRoot() + SCRIPT_NAME,
                                   fAccess, tout, dummyLibGroup, policies);
    if (program->failed()) {
        tout << "Compilation failed:\n";
        for (auto & err : program->errors) {
            tout << reportError(err.at, err.what, err.extra,
                                err.fixme, err.cerr);
        }
        return nullptr;
    }
    return program;
}

// ===================================================================
// Part A — Run on a worker thread
//
// 1. Compile + simulate on the main thread.
// 2. Clone the context with ContextCategory::thread_clone.
// 3. Capture the current daScriptEnvironment.
// 4. Launch a std::thread that sets the environment and evaluates.
// 5. Join and print the result.
// ===================================================================
static void part_a_run_on_thread() {
    TextPrinter tout;
    tout << "=== Part A: Run on a worker thread ===\n";

    // 1. Compile & simulate on main thread
    auto program = compile_script(tout);
    if (!program)
        return;

    Context ctx(program->getContextStackSize());
    if (!program->simulate(ctx, tout)) {
        tout << "Simulation failed.\n";
        return;
    }

    // Find the function we want to call on the worker thread.
    auto fnCompute = ctx.findFunction("compute");
    if (!fnCompute) {
        tout << "Function 'compute' not found.\n";
        return;
    }

    // 2. Clone the context for the worker thread.
    //    ContextCategory::thread_clone marks it as a thread-owned clone.
    //    shared_ptr ensures stable lifetime across threads.
    shared_ptr<Context> threadCtx;
    threadCtx.reset(new Context(ctx, uint32_t(ContextCategory::thread_clone)));
    threadCtx->sharedPtrContext = true;

    // Find compute in the clone — same index, but we need the clone's SimFunction.
    auto fnComputeClone = threadCtx->findFunction("compute");

    // 3. Capture the current environment.
    //    daScriptEnvironment is stored in thread-local storage (TLS).
    //    Worker threads don't have one unless we explicitly set it.
    auto bound = daScriptEnvironment::getBound();

    // 4. Launch the worker thread.
    int32_t result = 0;
    std::thread worker([&result, threadCtx, fnComputeClone, bound]() mutable {
        // Bind the daScript environment on this thread.
        daScriptEnvironment::setBound(bound);
        // Evaluate the function.
        vec4f res = threadCtx->evalWithCatch(fnComputeClone, nullptr);
        result = cast<int32_t>::to(res);
    });

    // 5. Wait for completion.
    worker.join();

    tout << "  compute() on worker thread returned: " << result << "\n";
    if (result == 4950) {
        tout << "  PASS\n";
    } else {
        tout << "  FAIL (expected 4950)\n";
    }
}

// ===================================================================
// Part B — Compile on a worker thread
//
// Each thread gets its own daScript environment: module registry,
// compilation pipeline, context, and execution.  This is the pattern
// used in test_threads.cpp.
//
// Steps on the worker thread:
//   1. ReuseCacheGuard — initializes thread-local free-list cache.
//   2. NEED_ALL_DEFAULT_MODULES — register module factories.
//   3. Module::Initialize() — instantiate all needed modules.
//   4. compileDaScript → simulate → evalWithCatch.
//   5. Module::Shutdown() — tear down this thread's modules.
// ===================================================================
static void part_b_compile_on_thread() {
    TextPrinter tout;
    tout << "\n=== Part B: Compile on a worker thread ===\n";

    int32_t result = 0;
    std::thread worker([&result]() {
        // 1. Thread-local free-list cache — must be first.
        ReuseCacheGuard guard;

        // 2. Register module factories for this thread.
        NEED_ALL_DEFAULT_MODULES;

        // 3. Initialize the module system on this thread.
        //    This creates a fresh daScriptEnvironment in TLS.
        Module::Initialize();

        TextPrinter tout;
        ModuleGroup dummyLibGroup;
        CodeOfPolicies policies;
        policies.threadlock_context = true;  // context mutex for thread safety
        auto fAccess = make_smart<FsFileAccess>();

        // 4. Compile, simulate, and run — just like on the main thread.
        auto program = compileDaScript(getDasRoot() + SCRIPT_NAME,
                                       fAccess, tout, dummyLibGroup, policies);
        if (program->failed()) {
            tout << "  Worker compilation failed.\n";
            for (auto & err : program->errors) {
                tout << reportError(err.at, err.what, err.extra,
                                    err.fixme, err.cerr);
            }
        } else {
            Context ctx(program->getContextStackSize());
            if (!program->simulate(ctx, tout)) {
                tout << "  Worker simulation failed.\n";
            } else {
                auto fn = ctx.findFunction("compute");
                if (fn) {
                    vec4f res = ctx.evalWithCatch(fn, nullptr);
                    result = cast<int32_t>::to(res);
                } else {
                    tout << "  Function 'compute' not found.\n";
                }
            }
        }

        // Release the program before shutdown — context is on the stack
        // and will be destroyed first.
        program.reset();

        // 5. Shut down the module system for this thread.
        Module::Shutdown();
    });

    worker.join();

    tout << "  compute() compiled + run on worker thread returned: " << result << "\n";
    if (result == 4950) {
        tout << "  PASS\n";
    } else {
        tout << "  FAIL (expected 4950)\n";
    }
}

// ===================================================================
// main
// ===================================================================
int main(int, char * []) {
    // Initialize modules on the main thread — needed for Part A.
    // Part B does its own Initialize/Shutdown on the worker thread.
    NEED_ALL_DEFAULT_MODULES;
    Module::Initialize();

    part_a_run_on_thread();
    part_b_compile_on_thread();

    Module::Shutdown();
    return 0;
}
