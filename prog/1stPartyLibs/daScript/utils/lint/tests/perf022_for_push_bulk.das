options gen2
// PERF022: for (s in A) { B |> push(s) } — use bulk *_from
//
// Problem:
//   Hand-rolled element-at-a-time append is overloaded against the bulk form
//   on the same name (push / push_clone), and silently skips the capacity
//   reserve. The bulk `*_from` overloads make intent explicit and reserve up
//   front.
//
// Bad pattern:
//   for (s in A) {
//       B |> push(s)
//   }
//
// Good pattern:
//   B |> push_from(A)
//
// Compiler folds `B |> push(s)`, `B.push(s)`, and `push(B, s)` to the same
// ExprCall, so the single detector catches all three forms.
//
// Scope: push and push_clone only. `emplace` from a for-loop iter var
// doesn't compile (iter var is const-ref, emplace requires var ref), so the
// rule never has a chance to fire on emplace. The `emplace_from` bulk API
// still exists in daslib/builtin.das for direct bulk-move calls.

expect 31208:6

require daslib/perf_lint

struct Foo {
    bar : int
}

// --- Positives: 6 simple-shape sites (2 calls × 3 syntactic forms) ---

def bad_push_pipe(src : array<int>; var dst : array<int>) {
    for (s in src) {
        dst |> push(s)                                                  // nolint:PERF006
    }
}

def bad_push_dot(src : array<int>; var dst : array<int>) {
    for (s in src) {
        dst.push(s)                                                     // nolint:PERF006
    }
}

def bad_push_plain(src : array<int>; var dst : array<int>) {
    for (s in src) {
        push(dst, s)                                                    // nolint:PERF006
    }
}

def bad_push_clone_pipe(src : array<Foo>; var dst : array<Foo>) {
    for (s in src) {
        dst |> push_clone(s)                                            // nolint:PERF006
    }
}

def bad_push_clone_dot(src : array<Foo>; var dst : array<Foo>) {
    for (s in src) {
        dst.push_clone(s)                                               // nolint:PERF006
    }
}

def bad_push_clone_plain(src : array<Foo>; var dst : array<Foo>) {
    for (s in src) {
        push_clone(dst, s)                                              // nolint:PERF006
    }
}

// --- Negatives: same shape but should NOT warn PERF022 ---

// 1. if-guarded push — selective, can't bulk-fold. PERF006 also suppresses
//    on conditional pushes, so no nolint needed.
def good_if_guarded(src : array<int>; var dst : array<int>) {
    for (s in src) {
        if (s > 0) {
            dst |> push(s)
        }
    }
}

// 2. Multi-statement body — the loop does more than one push.
def good_multi_stmt(src : array<int>; var dst : array<int>; var counter : int&) {
    for (s in src) {
        dst |> push(s)                                                  // nolint:PERF006
        counter++
    }
}

// 3. Transform-then-push — value isn't the iter var.
def good_transform(src : array<int>; var dst : array<int>) {
    for (s in src) {
        dst |> push(s * 2)                                              // nolint:PERF006
    }
}

// 4. Multi-source for — single bulk_from can't represent the zip.
def good_multi_source(a : array<int>; b : array<int>; var dst : array<int>) {
    for (x, y in a, b) {
        dst |> push(x + y)                                              // nolint:PERF006
    }
}

// 5. Nested for — outer body isn't a single ExprCall; inner body multi-statement so
//    it isn't a PERF022 hit either.
def good_nested(src : array<array<int>>; var dst : array<int>; var n : int&) {
    for (row in src) {
        for (s in row) {
            dst |> push(s * 2)                                          // nolint:PERF006
            n++
        }
    }
}

// 6. Push of a different variable — not the loop iter.
def good_push_other(src : array<int>; var dst : array<int>; other : int) {
    for (s in src) {
        dst |> push(other)                                              // nolint:PERF006
    }
}

// 7. Already-good form — the bulk call itself must not trigger.
def good_baseline(src : array<int>; var dst : array<int>) {
    dst |> push_from(src)
}
