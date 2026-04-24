// Tutorial 07 — Callbacks: Blocks, Lambdas, and Functions (C++ integration)
//
// Demonstrates how to receive and invoke daslang closures in C++:
//   - Blocks (stack-bound closures) via `const Block &` / `TBlock<>`
//   - Function pointers via `Func`
//   - Lambdas via `Lambda`
//   - The `das_invoke*` high-level templates
//   - Low-level `context->invoke()` for untyped invocation

#include "daScript/daScript.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/simulate/aot.h"

#include <cstdio>
#include <cmath>

using namespace das;

// -----------------------------------------------------------------------
// 1. Receiving a Block — `TBlock<RetType, ArgTypes...>`
//
// Blocks are stack-bound closures.  They are only valid during the call
// that receives them — you must NOT store them for later use.
// Use `TBlock<>` for typed blocks (the compiler checks argument types).
// -----------------------------------------------------------------------

// Apply a transform function to each element of an array.
// The block takes an int and returns an int.
void apply_transform(const TArray<int32_t> & arr, int32_t * results,
                     const TBlock<int32_t, int32_t> & blk,
                     Context * context, LineInfoArg * at) {
    for (uint32_t i = 0; i < arr.size; ++i) {
        // das_invoke<RetType>::invoke(ctx, lineinfo, block, args...)
        results[i] = das_invoke<int32_t>::invoke(context, at, blk, arr[i]);
    }
}

// A simpler example: call a void block with two arguments.
void with_values(int32_t a, int32_t b,
                 const TBlock<void, int32_t, int32_t> & blk,
                 Context * context, LineInfoArg * at) {
    das_invoke<void>::invoke(context, at, blk, a, b);
}

// Predicate filter: call block for each element, collect those that pass.
int32_t count_matching(const TArray<int32_t> & arr,
                       const TBlock<bool, int32_t> & pred,
                       Context * context, LineInfoArg * at) {
    int32_t count = 0;
    for (uint32_t i = 0; i < arr.size; ++i) {
        if (das_invoke<bool>::invoke(context, at, pred, arr[i])) {
            count++;
        }
    }
    return count;
}

// -----------------------------------------------------------------------
// 2. Receiving a Function pointer — `Func`
//
// `Func` is a reference to a daslang function.  Function pointers
// can be stored and invoked later (within the same context).
// Use `das_invoke_function<RetType>::invoke(ctx, li, fn, args...)`.
// -----------------------------------------------------------------------

void call_function_twice(Func fn, int32_t value,
                         Context * context, LineInfoArg * at) {
    das_invoke_function<void>::invoke(context, at, fn, value);
    das_invoke_function<void>::invoke(context, at, fn, value * 2);
}

// -----------------------------------------------------------------------
// 3. Receiving a Lambda — `Lambda` / `TLambda<>`
//
// Lambdas are heap-allocated closures that can capture variables.
// Use `das_invoke_lambda<RetType>::invoke(ctx, li, lambda, args...)`.
//
// NOTE: The untyped `Lambda` maps to `lambda<>` in daslang, which does not
// match typed lambdas like `lambda<(x:int):int>`.  For type-safe lambda
// acceptance, use `TLambda<RetType, ArgTypes...>` (similar to TBlock/TFunc).
// However, TLambda may require manual addExtern registration.
//
// Example (not registered — see tutorial 06 for addInterop approach):
//
//   int32_t invoke_lambda_with(TLambda<int32_t, int32_t> lambda,
//                              int32_t value,
//                              Context * context, LineInfoArg * at) {
//       return das_invoke_lambda<int32_t>::invoke(context, at, lambda, value);
//   }
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// 4. Practical pattern: C++ iterates, script processes
//
// A common embed pattern is having C++ own the iteration logic while
// the script provides processing via a block callback.
// -----------------------------------------------------------------------

void for_each_fibonacci(int32_t count,
                        const TBlock<void, int32_t, int32_t> & blk,
                        Context * context, LineInfoArg * at) {
    int32_t a = 0, b = 1;
    for (int32_t i = 0; i < count; ++i) {
        das_invoke<void>::invoke(context, at, blk, i, a);
        int32_t next = a + b;
        a = b;
        b = next;
    }
}

// Accumulator pattern: the block returns a running total.
int32_t reduce_range(int32_t from, int32_t to, int32_t init,
                     const TBlock<int32_t, int32_t, int32_t> & blk,
                     Context * context, LineInfoArg * at) {
    int32_t acc = init;
    for (int32_t i = from; i < to; ++i) {
        acc = das_invoke<int32_t>::invoke(context, at, blk, acc, i);
    }
    return acc;
}

// -----------------------------------------------------------------------
// Module
// -----------------------------------------------------------------------

class Module_Tutorial07 : public Module {
public:
    Module_Tutorial07() : Module("tutorial_07_cpp") {
        ModuleLibrary lib(this);
        lib.addBuiltInModule();

        // --- Block callbacks ---
        addExtern<DAS_BIND_FUN(apply_transform)>(*this, lib, "apply_transform",
            SideEffects::modifyArgument, "apply_transform")
                ->args({"arr", "results", "transform", "context", "at"});

        addExtern<DAS_BIND_FUN(with_values)>(*this, lib, "with_values",
            SideEffects::invoke, "with_values")
                ->args({"a", "b", "blk", "context", "at"});

        addExtern<DAS_BIND_FUN(count_matching)>(*this, lib, "count_matching",
            SideEffects::invoke, "count_matching")
                ->args({"arr", "pred", "context", "at"});

        // --- Function pointer callback ---
        addExtern<DAS_BIND_FUN(call_function_twice)>(*this, lib, "call_function_twice",
            SideEffects::invoke, "call_function_twice")
                ->args({"fn", "value", "context", "at"});

        // --- Practical patterns ---
        addExtern<DAS_BIND_FUN(for_each_fibonacci)>(*this, lib, "for_each_fibonacci",
            SideEffects::invoke, "for_each_fibonacci")
                ->args({"count", "blk", "context", "at"});

        addExtern<DAS_BIND_FUN(reduce_range)>(*this, lib, "reduce_range",
            SideEffects::invoke, "reduce_range")
                ->args({"from", "to", "init", "blk", "context", "at"});
    }
};

REGISTER_MODULE(Module_Tutorial07);

// -----------------------------------------------------------------------
// Host program
// -----------------------------------------------------------------------

#define SCRIPT_NAME "/tutorials/integration/cpp/07_callbacks.das"

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
    NEED_MODULE(Module_Tutorial07);
    Module::Initialize();
    tutorial();
    Module::Shutdown();
    return 0;
}
