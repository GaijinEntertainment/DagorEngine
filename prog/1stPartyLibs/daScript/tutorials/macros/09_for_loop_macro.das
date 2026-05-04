options gen2

// Tutorial 09 — For-loop macro
//
// Demonstrates AstForLoopMacro (registered via [for_loop_macro]).
// The macro module defines a for_loop_macro that enables
//   for ((k,v) in tab)
// syntax on tables, rewriting it to
//   for (k, v in keys(tab), values(tab))

require for_loop_macro_mod

// ---- Section 1: without the macro (the verbose way) -------------------------

def section1() {
    print("--- Section 1: table iteration without the macro ---\n")
    var tab <- { "one" => 1, "two" => 2, "three" => 3 }
    for (k, v in keys(tab), values(tab)) {
        print("  {k} => {v}\n")
    }
}

// ---- Section 2: with the for_loop_macro -------------------------------------

def section2() {
    print("\n--- Section 2: for ((k,v) in tab) with the macro ---\n")
    var tab <- { "one" => 1, "two" => 2, "three" => 3 }
    for ((k, v) in tab) {
        print("  {k} => {v}\n")
    }
}

// ---- Section 3: mixed sources -----------------------------------------------

def section3() {
    print("\n--- Section 3: mixed sources ---\n")
    var tab <- { "apple" => 10, "banana" => 20, "cherry" => 30 }
    for ((k, v), idx in tab, range(100)) {
        print("  [{idx}] {k} => {v}\n")
    }
}

[export]
def main() {
    section1()
    section2()
    section3()
}
