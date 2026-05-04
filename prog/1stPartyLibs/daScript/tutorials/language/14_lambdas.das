// Tutorial 14: Lambdas and Closures
//
// This tutorial covers:
//   - Lambda declaration with @() syntax
//   - How lambdas differ from blocks and functions
//   - Call syntax: fn(x) — invoke as an alternative
//   - Capture: by copy (default), by move, by clone
//   - Storing lambdas in variables (move semantics)
//   - Simplified => syntax
//   - Lambda vs block vs function
//   - Finally blocks (cleanup on lambda destruction)
//   - Lambda lifecycle: capture → invocations → destruction (finally + field cleanup)
//
// Run: daslang.exe tutorials/language/14_lambdas.das

options gen2

[export]
def main {

    // === Lambda basics ===
    // Lambdas are declared with @ instead of $ (blocks).
    // Unlike blocks, lambdas can be stored, moved, and outlive the scope.
    // Unlike anonymous functions (@@), lambdas capture variables.
    // Lambdas capture variables by COPY by default.

    var multiplier = 10
    var mul_lambda <- @(x : int) : int {
        return x * multiplier
    }
    print("mul_lambda(5) = {mul_lambda(5)}\n")   // 50

    // Changing the original variable does NOT affect the lambda (captured by copy).
    multiplier = 999
    print("mul_lambda(5) still = {mul_lambda(5)}\n")  // still 50

    // === Simplified => syntax ===
    // For single-expression lambdas.
    var doubler <- @(x : int) : int => x * 2
    print("doubler(7) = {doubler(7)}\n")

    // === Call syntax ===
    // Lambda variables support call syntax — call them like regular functions.
    // invoke() is the explicit alternative — same behavior.
    print("call syntax: {doubler(3)}\n")
    print("invoke:      {invoke(doubler, 3)}\n")

    // === Storing lambdas ===
    // Lambdas MUST be moved with <- (they cannot be copied).
    // var copy = doubler      // ERROR: lambdas can't be copied
    // var moved <- doubler    // OK but doubler becomes null

    // === Storing lambdas in arrays ===
    // Lambdas can be stored in arrays using emplace (which moves them in).
    // Blocks CANNOT be stored in arrays — they are stack-bound.
    var callbacks : array<lambda<() : void>>
    var greet <- @() {
        print("hello from callback\n")
    }
    callbacks |> emplace(greet)
    var farewell <- @() {
        print("goodbye from callback\n")
    }
    callbacks |> emplace(farewell)
    for (cb in callbacks) {
        invoke(cb)
    }
    print("stored {length(callbacks)} lambdas in array\n")
    delete callbacks

    // === Passing lambdas to functions ===
    // Functions that take lambda<> accept only @ lambdas (not @@ functions).
    print("apply(doubler, 21) = {apply_lambda(doubler, 21)}\n")

    // === Capture by copy (default) ===
    // Each lambda gets its own copy of captured variables.
    var counter = 0
    var inc <- @() : int {
        return ++counter    // this is the lambda's OWN copy of counter
    }
    print("inc() = {inc()}\n")    // 1
    print("inc() = {inc()}\n")    // 2
    print("inc() = {inc()}\n")    // 3
    print("original counter = {counter}\n")  // still 0

    // === Capture by move ===
    // Use capture(move(var)) to transfer ownership into the lambda.
    // The original variable becomes empty/zeroed after the move.
    var data <- [10, 20, 30]
    unsafe {
        var summer <- @capture(move(data)) () : int {
            var s = 0
            for (v in data) {
                s += v
            }
            return s
        }
        print("sum of moved data = {summer()}\n")     // 60
    }
    print("data length after move = {length(data)}\n") // 0

    // === Capture by clone ===
    // Use capture(clone(var)) to deep-copy a container into the lambda.
    // The original remains intact.
    var items <- [1, 2, 3]
    var count_items <- @capture(clone(items)) () : int {
        return length(items)
    }
    print("cloned items count = {count_items()}\n")  // 3
    print("original items still = {length(items)}\n") // 3

    // === Lambda vs Block vs Function ===
    //
    // FUNCTION (@@):
    //   - No capture at all. Same as a named function.
    //   - Type: function<>
    //   - See tutorial 12 for details.
    //
    // BLOCK ($):
    //   - Stack-allocated, very fast
    //   - Captures by reference automatically
    //   - Cannot be stored, returned, or put in arrays — only passed as argument
    //   - Type: block<>
    //
    // LAMBDA (@):
    //   - Heap-allocated, captures by copy (default), move, clone, or ref
    //   - Can be stored in variables (moved), passed around, stored in arrays
    //   - Type: lambda<>
    //
    // Each type has its own parameter type:
    //   function<(x:int):int>  — accepts only @@
    //   lambda<(x:int):int>    — accepts only @
    //   block<(x:int):int>     — accepts all three (most flexible)

    // === Practical example: stateful counter factory ===
    var make_counter <- @() : lambda<() : int> {
        var n = 0
        return <- @() : int => ++n
    }
    var c1 <- invoke(make_counter)
    print("c1: {c1()}, {c1()}, {c1()}\n")   // 1, 2, 3

    // === Lambda lifecycle ===
    // A lambda is a heap-allocated struct with fields for each captured variable.
    // The full lifecycle is:
    //   1. CAPTURE  — variables are copied/moved/cloned into the lambda struct
    //   2. INVOKE   — the lambda body runs (may be called many times)
    //   3. DESTROY  — when the lambda is deleted or GC'd:
    //                 a) finally{} block runs (user cleanup code)
    //                 b) captured fields are finalized (compiler-generated delete)
    //                 c) the struct memory is freed
    //
    // NOTE: lambda finally{} runs during DESTRUCTION, not after each call.
    //       This is different from block finally{}, which runs after each invocation.

    // === Captured field cleanup ===
    // Captured fields are automatically deleted when the lambda is destroyed,
    // UNLESS:
    //   - The field was captured by reference (pointer to external data — not owned)
    //   - The field was captured by move or clone (doNotDelete flag is set)
    //   - The field type is POD (int, float, etc. — no cleanup needed)
    //
    // Example: an array captured by clone. The original stays intact;
    // the cloned copy inside the lambda is cleaned up on destruction.
    print("\n--- captured field cleanup ---\n")
    var nums <- [100, 200, 300]
    var sum_nums <- @capture(clone(nums)) () : int {
        var s = 0
        for (v in nums) {
            s += v
        }
        return s
    }
    print("sum = {sum_nums()}\n")      // 600
    print("original nums = {nums}\n")  // still [100, 200, 300]
    // When sum_nums is deleted or goes out of scope, the cloned array
    // inside the lambda struct is automatically finalized.

    // === Finally blocks on lambdas ===
    // A lambda's finally{} block runs ONCE when the lambda is DESTROYED
    // (deleted or garbage collected) — NOT after each invocation.
    //
    // This is the opposite of block finally{}, which runs after every call.
    //
    // Use lambda finally{} for one-time destruction cleanup, such as
    // releasing resources that the lambda owns.
    print("\n--- finally on destruction ---\n")
    var data2 <- [10, 20]
    var demo <- @capture(clone(data2)) () {
        print("  body: data2 has {length(data2)} items\n")
    } finally {
        // This runs once, when 'demo' is destroyed.
        print("  finally: lambda destroyed\n")
    }
    demo()
    demo()
    print("  about to delete demo...\n")
    unsafe { delete demo; }
    // Output shows: body twice, then "about to delete", then finally once.
}

def apply_lambda(fn : lambda<(x : int) : int>; value : int) : int {
    return fn(value)
}

// output:
// mul_lambda(5) = 50
// mul_lambda(5) still = 50
// doubler(7) = 14
// call syntax: 6
// invoke:      6
// hello from callback
// goodbye from callback
// stored 2 lambdas in array
// apply(doubler, 21) = 42
// inc() = 1
// inc() = 2
// inc() = 3
// original counter = 0
// sum of moved data = 60
// data length after move = 0
// cloned items count = 3
// original items still = 3
// c1: 1, 2, 3
//
// --- captured field cleanup ---
// sum = 600
// original nums = [[ 100; 200; 300]]
//
// --- finally on destruction ---
//   body: data2 has 2 items
//   body: data2 has 2 items
//   about to delete demo...
//   finally: lambda destroyed
