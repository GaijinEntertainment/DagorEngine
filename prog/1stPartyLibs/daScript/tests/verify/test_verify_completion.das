options gen2
expect 40104:8

// Verify that [verify_completion] rejects unbounded loops and recursive calls.

require daslib/validate_code

[export, verify_completion]
def bad_loop(var all : iterator<int>; cond_const : bool) {
    for (t in all) {                    // 40104: unbounded iterator loop
        feint("t = {t}\n")
    }
    for (t, r in all, range(100500)) {  // ok — has explicit range
        feint("t = {t}\n")
    }
    var cond = true
    while (cond) {                      // 40104: unbounded while loop
        feint("nop\n")
        cond = false
    }
    var tab <- {1 => 1, 2 => 2, 3 => 3}
    for (x in tab |> keys()) {          // 40104: keys() is unbounded
        feint("x = {x}\n")
    }
}

[export, verify_completion]
def simple_recursive() {
    feint("simple\n")
    simple_recursive()                  // 40104: direct recursion
}

def rec_a() {
    feint("a\n")
    rec_b()                             // 40104: mutual recursion a->b->a
}

def rec_b() {
    feint("b\n")
    rec_a()                             // 40104: mutual recursion b->a->b
}

def rec_c() {
    rec_a()                             // 40104: transitive recursion
}

[export, verify_completion]
def rec_d() {
    rec_c()                             // 40104: transitive recursion c->a->b->a
}

[export]
def main() {
    pass
}
