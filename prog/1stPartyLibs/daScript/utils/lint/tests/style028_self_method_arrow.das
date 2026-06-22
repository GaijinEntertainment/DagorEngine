options gen2
// STYLE028: 'self->method(...)' inside a class method is optional.
//
// Problem: `self->foo(args)` writes the implicit receiver twice — once via
// `self`, once via the `->` invoke. The compiler auto-promotes bare
// `foo(args)` inside a class method to the same `self->foo(args)` invoke
// (ast_infer_type.cpp:5612), so the prefix is pure noise.
//
// Bad pattern:
//   class C {
//     def m1() { self->m2() }
//   }
//
// Good pattern:
//   class C {
//     def m1() { m2() }            // implicit self
//   }
//   // or, when you genuinely want an explicit receiver: self.m2()

expect 31209:4

require daslib/style_lint

// --- Bad patterns ---

class Bad {
    val : int = 0

    def helper() : int {
        return val
    }

    def do_thing(x : int) : void {
        val = x
    }

    def caller_arrow() : int {
        self->do_thing(5)                              // STYLE028
        return self->helper()                          // STYLE028
    }

    def chained() : int {
        return self->helper() + self->helper()         // STYLE028 x2 (distinct columns)
    }
}

// --- Good patterns (no warnings) ---

class Good {
    val : int = 0

    def helper() : int {
        return val
    }

    def do_thing(x : int) : void {
        val = x
    }

    def implicit_self() : int {
        do_thing(7)                                    // OK: implicit self promotion
        return helper()                                // OK
    }

    def dot_receiver() : int {
        return self.helper()                           // OK: dot form, not '->'
    }
}

// Free function — not a class method, no self at all.
def free_fn(var g : Good?) : int {
    return g->helper()                                 // OK: receiver is not 'self'
}
