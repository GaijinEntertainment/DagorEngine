// Tutorial 03 — Binding C++ Functions (C++ integration)
//
// Demonstrates how to expose C++ functions to daslang:
//   - addExtern with DAS_BIND_FUN — automatic binding of free functions
//   - SideEffects flags — how the optimizer uses purity information
//   - addConstant — exposing C++ constants to scripts
//   - SimNode_ExtFuncCallAndCopyOrMove — for functions returning ref types
//   - Functions that receive Context* or LineInfoArg* automatically
//
// This is the C++ equivalent of creating a "custom module" that scripts
// can require.

#include "daScript/daScript.h"

#include <cstdio>
#include <cmath>

using namespace das;

// -----------------------------------------------------------------------
// C++ functions that we want to expose to daslang
// -----------------------------------------------------------------------

// A pure function — no side effects; the optimizer may fold or eliminate
// calls to it if the result is unused.
float xmadd(float a, float b, float c, float d) {
    return a * b + c * d;
}

// A pure function that takes an int and returns an int.
int factorial(int n) {
    int result = 1;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

// A function with side effects — it prints to stdout, so the optimizer
// must never eliminate calls to it.
void greet(const char * name) {
    printf("Hello from C++, %s!\n", name);
}

// A function that modifies its argument (passed by reference).
// SideEffects::modifyArgument tells the compiler that the argument
// is changed, so it won't be optimized away.
void double_it(int32_t & value) {
    value *= 2;
}

// A function that needs access to the daslang evaluation context.
// When a C++ function takes Context* as its first argument (or
// LineInfoArg* as its last argument), daslang passes them automatically
// — the script caller does not see these parameters.
void print_stack_info(Context * ctx) {
    printf("Context stack size: %d bytes\n", ctx->stack.size());
}

// -----------------------------------------------------------------------
// Packaging functions into a Module
// -----------------------------------------------------------------------

class Module_Tutorial03 : public Module {
public:
    Module_Tutorial03() : Module("tutorial_03_cpp") {
        ModuleLibrary lib(this);
        lib.addBuiltInModule();

        // --- Constants ---
        // addConstant binds a compile-time constant visible in scripts.
        addConstant(*this, "PI",    3.14159265358979323846f);
        addConstant(*this, "SQRT2", sqrtf(2.0f));

        // --- Pure functions (SideEffects::none) ---
        // The optimizer knows these have no observable effects, so it can
        // eliminate calls whose results are unused.
        addExtern<DAS_BIND_FUN(xmadd)>(*this, lib, "xmadd",
            SideEffects::none, "xmadd");

        addExtern<DAS_BIND_FUN(factorial)>(*this, lib, "factorial",
            SideEffects::none, "factorial");

        // --- Functions with external side effects ---
        // greet prints to stdout, so it modifies external state.
        addExtern<DAS_BIND_FUN(greet)>(*this, lib, "greet",
            SideEffects::modifyExternal, "greet");

        // --- Functions that modify arguments ---
        // double_it changes its argument in place.
        addExtern<DAS_BIND_FUN(double_it)>(*this, lib, "double_it",
            SideEffects::modifyArgument, "double_it");

        // --- Context-aware functions ---
        // print_stack_info takes Context* — daslang injects it
        // automatically; the script signature has zero parameters.
        addExtern<DAS_BIND_FUN(print_stack_info)>(*this, lib, "print_stack_info",
            SideEffects::modifyExternal, "print_stack_info");
    }
};

// Register the module so it can be activated with NEED_MODULE.
REGISTER_MODULE(Module_Tutorial03);

// -----------------------------------------------------------------------
// Host program
// -----------------------------------------------------------------------

#define SCRIPT_NAME "/tutorials/integration/cpp/03_binding_functions.das"

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

    auto fnTest = ctx.findFunction("test");
    if (!fnTest) {
        tout << "Function 'test' not found\n";
        return;
    }

    ctx.evalWithCatch(fnTest, nullptr);
    if (auto ex = ctx.getException()) {
        tout << "Exception: " << ex << "\n";
    }
}

int main(int, char * []) {
    NEED_ALL_DEFAULT_MODULES;
    NEED_MODULE(Module_Tutorial03);
    Module::Initialize();
    tutorial();
    Module::Shutdown();
    return 0;
}
