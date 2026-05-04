// Tutorial 12: Function Pointers and Anonymous Functions
//
// This tutorial covers:
//   - Getting a pointer to a named function with @@
//   - The function<> type
//   - Calling function pointers (call syntax and invoke)
//   - Disambiguation with @@<signature>
//   - Anonymous functions (nameless @@)
//   - Passing functions as arguments
//   - function<> vs lambda<> vs block<> — distinct types
//
// Run: daslang.exe tutorials/language/12_function_pointers.das

options gen2

// === Named functions ===

def double_it(x : int) : int {
    return x * 2
}

def triple_it(x : int) : int {
    return x * 3
}

def negate(x : int) : int {
    return -x
}

// === Overloaded function — same name, different signatures ===

def add(a, b : int) : int {
    return a + b
}

def add(a, b : float) : float {
    return a + b
}

// === Function that accepts a function<> parameter ===

def apply(f : function<(x : int) : int>; value : int) : int {
    return f(value)    // call syntax — just like calling a regular function
}

def transform_array(arr : array<int>; f : function<(x : int) : int>) {
    for (i, v in count(), arr) {
        print("  {v} -> {f(v)}\n")
    }
}

[export]
def main {

    // === Getting function pointers ===

    // Use @@ to get a pointer to a named function.
    // The result is a value of type function<(x:int):int>.
    let fn_double = @@double_it
    let fn_triple = @@triple_it

    // Call them directly — function pointers support call syntax
    print("fn_double(5) = {fn_double(5)}\n")
    print("fn_triple(5) = {fn_triple(5)}\n")

    // invoke() is the explicit alternative — same result
    print("invoke: {invoke(fn_double, 5)}\n")

    // === Passing functions as arguments ===

    print("apply(double, 7) = {apply(fn_double, 7)}\n")
    print("apply(triple, 7) = {apply(fn_triple, 7)}\n")
    print("apply(negate, 7) = {apply(@@negate, 7)}\n")   // inline @@

    // === Transform an array ===
    var nums <- [10, 20, 30]
    print("double:\n")
    transform_array(nums, @@double_it)
    print("negate:\n")
    transform_array(nums, @@negate)

    // === Disambiguating overloads ===

    // When multiple functions share a name, specify the signature:
    let fn_add_int = @@ < (a : int; b : int) : int > add
    let fn_add_flt = @@ < (a : float; b : float) : float > add
    print("add_int(3,4) = {fn_add_int(3, 4)}\n")
    print("add_flt(1.5,2.5) = {fn_add_flt(1.5, 2.5)}\n")

    // === Anonymous functions (nameless @@) ===

    // @@ can also create an anonymous function inline.
    // Like a lambda, but it's a function<> — no capture, no heap allocation.
    let square <- @@(x : int) : int => x * x
    print("square(6) = {square(6)}\n")

    // Anonymous functions CANNOT capture outer variables:
    //   var outer = 10
    //   let bad <- @@(x : int) : int => x + outer   // ERROR: can't locate variable

    // Multi-line anonymous function
    let clamp <- @@(x : int) : int {
        if (x < 0) {
            return 0
        }
        if (x > 100) {
            return 100
        }
        return x
    }
    print("clamp(-5) = {clamp(-5)}\n")
    print("clamp(50) = {clamp(50)}\n")
    print("clamp(200) = {clamp(200)}\n")

    // Anonymous functions can be passed directly to function<> parameters
    print("apply(@@, 8) = {apply(@@(x : int) : int => x * x + 1, 8)}\n")

    // === function<> vs lambda<> vs block<> ===

    // These are three DISTINCT callable types:
    //
    //   function<>  — pointer to a named or anonymous function (@@)
    //                  No capture. No heap allocation.
    //                  Created with: @@name or @@(args) { ... }
    //
    //   lambda<>    — closure that captures variables (@)
    //                  Heap-allocated. Captured by copy (default).
    //                  Created with: @(args) { ... }
    //
    //   block<>     — stack-allocated callback ($)
    //                  Captures by reference. Cannot be stored.
    //                  Created with: $(args) { ... }
    //
    // A function<> parameter accepts only @@, not @ or $.
    // A lambda<>   parameter accepts only @, not @@ or $.
    // A block<>    parameter accepts any of them (most flexible).
    //
    // Use function<> when you don't need capture (pure transforms).
    // Use lambda<>  when you need to capture state.
    // Use block<>   when you want a temporary callback parameter.

    print("done\n")
}

// output:
// fn_double(5) = 10
// fn_triple(5) = 15
// invoke: 10
// apply(double, 7) = 14
// apply(triple, 7) = 21
// apply(negate, 7) = -7
// double:
//   10 -> 20
//   20 -> 40
//   30 -> 60
// negate:
//   10 -> -10
//   20 -> -20
//   30 -> -30
// add_int(3,4) = 7
// add_flt(1.5,2.5) = 4
// square(6) = 36
// clamp(-5) = 0
// clamp(50) = 50
// clamp(200) = 100
// apply(@@, 8) = 65
// done
