options gen2

// Companion script for the "Binding C++ Functions" tutorial.
// The module "tutorial_03_cpp" is defined in 03_binding_functions.cpp and
// exposes C++ functions and constants to this script.

require tutorial_03_cpp

[export]
def test() {
    // --- Using bound constants ---
    print("PI    = {PI}\n")
    print("SQRT2 = {SQRT2}\n")

    // --- Calling a pure function (SideEffects::none) ---
    let xma = xmadd(SQRT2, SQRT2, 1.0, 1.0)   // sqrt(2)*sqrt(2) + 1*1 = 3
    print("xmadd(SQRT2, SQRT2, 1.0, 1.0) = {xma}\n")

    // --- Calling another pure function ---
    print("factorial(10) = {factorial(10)}\n")

    // --- Calling a function with external side effects ---
    // This prints directly to stdout from C++.
    greet("daslang")

    // --- Calling a function that modifies its argument ---
    var val = 21
    double_it(val)
    print("double_it(21) = {val}\n")

    // --- Calling a context-aware function ---
    // print_stack_info takes no arguments in daslang —
    // the Context* is injected automatically by the runtime.
    print_stack_info()
}
