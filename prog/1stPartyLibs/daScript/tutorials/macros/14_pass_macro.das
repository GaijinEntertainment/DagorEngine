options gen2

// Tutorial 14 — Pass macro
//
// Demonstrates AstPassMacro — whole-program macros invoked at different
// compilation passes.  Two annotations are shown:
//
// [lint_macro]  (Section 1) — invoked for each module compiled after the
//   macro module.  The lint pass is read-only: it can inspect the
//   fully-typed AST and report diagnostics but cannot modify code.
//   The macro uses compiling_module() to inspect the current module
//   and warns about functions whose body exceeds a statement threshold.
//
// [infer_macro] (Section 2) — runs during the inference pass.  The macro
//   can walk and modify the AST.  Returning true from apply() tells the
//   compiler to re-infer; the loop continues until every macro returns
//   false.  The macro in pass_macro_mod inserts a _trace_enter() call at
//   every function entry, following the pattern used by daslib/heartbeat.

require pass_macro_mod

// ---- helper functions for demonstration ------------------------------------

def greet(name : string) {
    print("Hello, {name}!\n")
}

def sum_to(n : int) : int {
    var total = 0
    for (i in range(n)) {
        total += i
    }
    return total
}

def countdown(n : int) {
    var i = n
    while (i > 0) {
        i--
    }
    print("counted down from {n}\n")
}

[export]
def main() {
    greet("world")
    let s = sum_to(5)
    print("sum = {s}\n")
    countdown(3)
}

// Expected output:
// [lint] 'main' has 5 top-level statements (>4)
// >>> main
// >>> greet
// Hello, world!
// >>> sum_to
// sum = 10
// >>> countdown
// counted down from 3
