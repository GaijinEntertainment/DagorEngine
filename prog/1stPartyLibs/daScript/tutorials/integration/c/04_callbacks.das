options gen2

// Companion script for the "Callbacks and Closures" C integration tutorial.
// This script uses interop functions from the C module "tutorial_c_04".

require tutorial_c_04

// A regular function used as a callback via function pointer.
def format_result(a : int; b : float) : string {
    return "formatted: {a} + {b}"
}

[export]
def test() {
    // 1. Call a bound C function with a function-pointer callback.
    //    @@format_result passes a pointer to format_result.
    print("--- function pointer callback ---\n")
    let r1 = c_call_function(@@format_result, 10, 2.5)
    print("C returned: {r1}\n")

    // 2. Call a bound C function with a lambda callback.
    //    Lambdas can capture variables from the enclosing scope.
    print("\n--- lambda callback ---\n")
    var counter = 0
    var lmb <- @(a : int; b : float) : string {
        counter += a
        return "lambda: a={a} b={b} counter={counter}"
    }
    let r2 = c_call_lambda(lmb, 5, 1.5)
    print("C returned: {r2}\n")
    let r3 = c_call_lambda(lmb, 3, 0.5)
    print("C returned: {r3}\n")

    // 3. Call a bound C function with a block callback.
    //    Blocks are stack-allocated closures — lighter than lambdas.
    //    Like lambdas they can capture variables from the enclosing scope.
    print("\n--- block callback ---\n")
    var blk <- $(a : int; b : float) : string {
        counter += a
        return "block: a={a} b={b} counter={counter}"
    }
    let r4 = c_call_block(blk, 7, 0.5)
    print("C returned: {r4}\n")
    let r5 = c_call_block(blk, 3, 1.5)
    print("C returned: {r5}\n")
}
