// Macro Tutorial 5: Tag Function Macros
//
// This tutorial demonstrates the [tag_function] + [tag_function_macro]
// pattern — the standard way to build single-file macro modules.
//
// In tutorials 3 and 4, we used [function_macro], which requires the
// macro to live in a separate module compiled before the usage file.
// The tag_function pattern avoids that split: the tagged function and
// its macro class can coexist in the same module.
//
// We build a once() macro that executes a block only on the first call.
// Each call site gets its own auto-generated global bool flag, so
// multiple once() calls are independent.
//
// Covers:
//   - [tag_function(tag_name)] to mark a function with a tag
//   - [tag_function_macro(tag="tag_name")] to attach a macro via the tag
//   - transform() to replace calls with generated AST
//   - make_unique_private_name for per-call-site unique names
//   - add_global_private_var for creating flag variables
//   - clone_expression + move_unquote_block for block extraction
//   - qmacro_block with $i() and $b() splicing
//
// The macro is defined in tag_function_macro_mod.das.
//
// Run: daslang.exe tutorials/macros/05_tag_function_macro.das

options gen2

require tag_function_macro_mod

// ── once() in a loop ──
//
// The block executes only on the first iteration.
// Subsequent iterations skip it because the global flag is already true.

def test_loop() {
    for (i in range(3)) {
        once() {
            print("initialized (runs once)\n")
        }
        print("  iteration {i}\n")
    }
}

// ── Multiple independent once() calls ──
//
// Each call site has its own flag, so they fire independently.

def test_multiple() {
    for (i in range(2)) {
        once() {
            print("first once (runs once)\n")
        }
        once() {
            print("second once (runs once)\n")
        }
        print("  pass {i}\n")
    }
}

// ── once() in a function called multiple times ──
//
// The flag is global, not local — so calls across invocations still
// share the same flag.  The block runs only on the very first call.

def greet() {
    once() {
        print("welcome! (runs once)\n")
    }
    print("  greet called\n")
}

[export]
def main() {
    print("--- test_loop ---\n")
    test_loop()
    print("\n--- test_multiple ---\n")
    test_multiple()
    print("\n--- test_greet ---\n")
    greet()
    greet()
    greet()
}

// output:
// --- test_loop ---
// initialized (runs once)
//   iteration 0
//   iteration 1
//   iteration 2
//
// --- test_multiple ---
// first once (runs once)
// second once (runs once)
//   pass 0
//   pass 1
//
// --- test_greet ---
// welcome! (runs once)
//   greet called
//   greet called
//   greet called
