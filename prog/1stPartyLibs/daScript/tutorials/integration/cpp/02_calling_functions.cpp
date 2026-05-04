// Tutorial 02 — Calling daslang Functions (C++ integration)
//
// Demonstrates TWO ways to call daslang functions from C++:
//
// Part A — Low-level: manual cast<T>::from/to + evalWithCatch
//   The raw approach — you build a vec4f array, call evalWithCatch,
//   and unpack the result.  Good for understanding the internals and
//   for maximum control (e.g. avoiding copies, reusing arg buffers).
//
// Part B — High-level: das_invoke_function / das_invoke_function_by_name
//   The idiomatic C++ approach — type-safe variadic templates that
//   handle cast<> and AOT dispatch automatically.  Prefer this in
//   production code.
//
// Both parts call the same daslang functions so you can compare.
//
// Compare with tutorials/integration/c/02_calling_functions.c for the
// C API equivalent.

#include "daScript/daScript.h"
#include "daScript/simulate/aot.h"       // das_invoke_function

#include <cstdio>

using namespace das;

#define SCRIPT_NAME "/tutorials/integration/cpp/02_calling_functions.das"

// Structure layout must exactly match the daslang struct Vec2.
struct Vec2 {
    float x;
    float y;
};

void tutorial() {
    TextPrinter tout;
    ModuleGroup dummyLibGroup;
    auto fAccess = make_smart<FsFileAccess>();

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

    Context ctx(program->getContextStackSize());
    if (!program->simulate(ctx, tout)) {
        tout << "Simulation failed:\n";
        for (auto & err : program->errors) {
            tout << reportError(err.at, err.what, err.extra,
                                err.fixme, err.cerr);
        }
        return;
    }

    // ===================================================================
    // PART A — Low-level: cast<T> + evalWithCatch
    //
    // This is the manual approach.  You pack arguments into a vec4f
    // array using cast<T>::from, call evalWithCatch, and unpack the
    // return value with cast<T>::to.
    // ===================================================================
    printf("=== Part A: Low-level (cast + evalWithCatch) ===\n");

    // -------------------------------------------------------------------
    // A1. int arguments, int return value
    // -------------------------------------------------------------------
    {
        auto fnAdd = ctx.findFunction("add");
        if (!fnAdd) { tout << "'add' not found\n"; return; }

        // Optional: verify the signature at runtime.
        // verifyCall<ReturnType, ArgType1, ArgType2, ...> checks that the
        // function's debug info matches the expected C++ types.
        // This is slow (walks debug info), so do it once — not in a loop.
        if (!verifyCall<int32_t, int32_t, int32_t>(fnAdd->debugInfo, dummyLibGroup)) {
            tout << "'add' has wrong signature\n";
            return;
        }

        vec4f args[2];
        args[0] = cast<int32_t>::from(17);
        args[1] = cast<int32_t>::from(25);

        vec4f ret = ctx.evalWithCatch(fnAdd, args);
        if (auto ex = ctx.getException()) {
            tout << "exception in add: " << ex << "\n";
            return;
        }

        int32_t result = cast<int32_t>::to(ret);
        printf("  add(17, 25) = %d\n", result);
    }

    // -------------------------------------------------------------------
    // A2. float argument, float return value
    // -------------------------------------------------------------------
    {
        auto fnSquare = ctx.findFunction("square");
        if (!fnSquare) { tout << "'square' not found\n"; return; }

        vec4f args[1];
        args[0] = cast<float>::from(3.5f);

        vec4f ret = ctx.evalWithCatch(fnSquare, args);
        if (auto ex = ctx.getException()) {
            tout << "exception in square: " << ex << "\n";
            return;
        }

        float result = cast<float>::to(ret);
        printf("  square(3.5) = %.2f\n", result);
    }

    // -------------------------------------------------------------------
    // A3. string argument, string return value
    //
    // Strings are passed as char* via cast<char*>::from.
    // Returned strings live on the context heap and remain valid until
    // the context is destroyed or garbage-collected.
    // -------------------------------------------------------------------
    {
        auto fnGreet = ctx.findFunction("greet");
        if (!fnGreet) { tout << "'greet' not found\n"; return; }

        vec4f args[1];
        args[0] = cast<char *>::from((char *)"World");

        vec4f ret = ctx.evalWithCatch(fnGreet, args);
        if (auto ex = ctx.getException()) {
            tout << "exception in greet: " << ex << "\n";
            return;
        }

        const char * result = cast<char *>::to(ret);
        printf("  greet(\"World\") = \"%s\"\n", result);
    }

    // -------------------------------------------------------------------
    // A4. Multiple argument types (int, float, string, bool)
    // -------------------------------------------------------------------
    {
        auto fnShow = ctx.findFunction("show_types");
        if (!fnShow) { tout << "'show_types' not found\n"; return; }

        vec4f args[4];
        args[0] = cast<int32_t>::from(42);
        args[1] = cast<float>::from(3.14f);
        args[2] = cast<char *>::from((char *)"test");
        args[3] = cast<bool>::from(true);

        ctx.evalWithCatch(fnShow, args);
        if (auto ex = ctx.getException()) {
            tout << "exception in show_types: " << ex << "\n";
            return;
        }
    }

    // -------------------------------------------------------------------
    // A5. Function returning a struct (cmres)
    //
    // When a function returns a struct, the caller supplies a result
    // buffer.  Use the three-argument evalWithCatch(fn, args, &result).
    // -------------------------------------------------------------------
    {
        auto fnMakeVec2 = ctx.findFunction("make_vec2");
        if (!fnMakeVec2) { tout << "'make_vec2' not found\n"; return; }

        vec4f args[2];
        args[0] = cast<float>::from(1.5f);
        args[1] = cast<float>::from(2.5f);

        Vec2 v;
        ctx.evalWithCatch(fnMakeVec2, args, &v);
        if (auto ex = ctx.getException()) {
            tout << "exception in make_vec2: " << ex << "\n";
            return;
        }

        printf("  make_vec2(1.5, 2.5) = { x=%.1f, y=%.1f }\n", v.x, v.y);
    }

    // -------------------------------------------------------------------
    // A6. Handling exceptions
    //
    // If the script calls panic(), evalWithCatch catches it.
    // -------------------------------------------------------------------
    {
        auto fnFail = ctx.findFunction("will_fail");
        if (!fnFail) { tout << "'will_fail' not found\n"; return; }

        ctx.evalWithCatch(fnFail, nullptr);
        if (auto ex = ctx.getException()) {
            printf("  Caught expected exception: %s\n", ex);
        } else {
            printf("  Expected an exception but got none\n");
        }
    }

    // ===================================================================
    // PART B — High-level: das_invoke_function
    //
    // das_invoke_function<ReturnType>::invoke(ctx, lineinfo, func, args...)
    //   - Handles cast<> automatically for all arguments
    //   - Dispatches to AOT when available, falls back to interpreter
    //   - Type-safe at compile time
    //
    // das_invoke_function_by_name<ReturnType>::invoke(ctx, lineinfo, name, args...)
    //   - Same, but looks up the function by name each time
    //   - Validates uniqueness and safety (not cmres, not unsafe)
    //
    // For cmres (struct returns):
    //   das_invoke_function<ReturnType>::invoke_cmres(ctx, lineinfo, func, args...)
    //
    // The Func struct is a lightweight wrapper around SimFunction*.
    // ===================================================================
    printf("\n=== Part B: High-level (das_invoke_function) ===\n");

    // -------------------------------------------------------------------
    // B1. int arguments, int return value
    // -------------------------------------------------------------------
    {
        Func fnAdd = ctx.findFunction("add");

        int32_t result = das_invoke_function<int32_t>::invoke(
            &ctx, nullptr, fnAdd, 17, 25);
        printf("  add(17, 25) = %d\n", result);
    }

    // -------------------------------------------------------------------
    // B2. float argument, float return value
    // -------------------------------------------------------------------
    {
        Func fnSquare = ctx.findFunction("square");

        float result = das_invoke_function<float>::invoke(
            &ctx, nullptr, fnSquare, 3.5f);
        printf("  square(3.5) = %.2f\n", result);
    }

    // -------------------------------------------------------------------
    // B3. string argument, string return value
    // -------------------------------------------------------------------
    {
        Func fnGreet = ctx.findFunction("greet");

        const char * result = das_invoke_function<const char *>::invoke(
            &ctx, nullptr, fnGreet, (char *)"World");
        printf("  greet(\"World\") = \"%s\"\n", result);
    }

    // -------------------------------------------------------------------
    // B4. void return, multiple argument types
    // -------------------------------------------------------------------
    {
        Func fnShow = ctx.findFunction("show_types");

        das_invoke_function<void>::invoke(
            &ctx, nullptr, fnShow,
            42, 3.14f, (char *)"test", true);
    }

    // -------------------------------------------------------------------
    // B5. Function returning a struct — use invoke_cmres
    // -------------------------------------------------------------------
    {
        Func fnMakeVec2 = ctx.findFunction("make_vec2");

        Vec2 v = das_invoke_function<Vec2>::invoke_cmres(
            &ctx, nullptr, fnMakeVec2, 1.5f, 2.5f);
        printf("  make_vec2(1.5, 2.5) = { x=%.1f, y=%.1f }\n", v.x, v.y);
    }

    // -------------------------------------------------------------------
    // B6. Invoke by name — no need to find the function first
    //
    // das_invoke_function_by_name looks up the function, checks that it
    // is unique and not cmres/unsafe, then calls it.  Convenient for
    // one-off calls; for hot loops, cache the Func instead.
    // -------------------------------------------------------------------
    {
        int32_t result = das_invoke_function_by_name<int32_t>::invoke(
            &ctx, nullptr, "add", 100, 200);
        printf("  add(100, 200) by name = %d\n", result);
    }
}

int main(int, char * []) {
    NEED_ALL_DEFAULT_MODULES;
    Module::Initialize();
    tutorial();
    Module::Shutdown();
    return 0;
}
