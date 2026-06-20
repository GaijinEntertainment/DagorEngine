options gen2
// STYLE012: array<T> variable initialized via push/emplace instead of literal
//
// Problem: Declaring an empty `array<T>` and then populating it with a
// contiguous run of TWO OR MORE `push`/`emplace` calls is a common
// "I-forgot-how-to-init-arrays" bug — and it's both harder to read and slower
// than using an array literal `var a <- [x, <-y]`. A single push stays silent
// because it is often more readable that way.
//
// Bad pattern:
//   var a : array<int>
//   a |> push(1)
//   a |> push(2)
//
// Good patterns:
//   var a <- [1, 2]                          // inferred element type
//   var a <- array<Shape?>(new Circle(),     // explicit element type,
//                          new Square())     //   useful for polymorphic upcasts
//
// `push_clone` is intentionally NOT flagged — no clean array-literal equivalent.

expect 31209:6

require daslib/style_lint

struct Foo {
    x : int
}

struct Inner {
    v : int
}

struct RegStruct {
    inner : Inner
    tag : int
    nums : array<int>
}

struct Bar {
    far : array<int>
}

struct Holder {
    bar : Bar
}

// bad: two pushes after empty array decl (1 warning)
def bad_push_sequence() {
    var a : array<int>
    a |> push(1)
    a |> push(2)
}

// good: single push — readability is fine, stays silent
def good_single_push() {
    var a : array<int>
    a |> push(42)
}

// bad: emplace sequence (1 warning)
def bad_emplace_sequence() {
    var f1 = Foo(x = 1)
    var f2 = Foo(x = 2)
    var a : array<Foo>
    a |> emplace(f1)
    a |> emplace(f2)
}

// bad: mixed push + emplace (1 warning)
def bad_mixed() {
    var f = Foo(x = 1)
    var a : array<Foo>
    a |> push(Foo(x = 0))
    a |> emplace(f)
}

// good: array literal
def good_literal() {
    var a <- [1, 2, 3]
    a |> push(4)  // the literal used initialization; this push is acceptable
}

// good: push_clone is intentionally ignored
def good_push_clone() {
    var s = "hi"
    var a : array<string>
    a |> push_clone(s)
}

// good: push inside a loop is fine *for STYLE012* (no contiguous push run after the
// let). The shape DOES match STYLE027 (use a comprehension), so suppress that here.
def good_loop_push() {
    var a : array<int>  // nolint:STYLE027
    for (i in range(5)) {
        a |> push(i)
    }
}

// good: push inside conditional is fine
def good_conditional_push(cond : bool) {
    var a : array<int>
    if (cond) {
        a |> push(1)
    }
}

// good: inscope variable (matches STYLE011 exclusion)
def good_inscope() {
    var inscope a : array<int>
    a |> push(1)
}

// good: var decl followed by other statement, then push (push not contiguous)
def good_interleaved() : int {
    var a : array<int>
    let x = 42
    a |> push(x)
    return x
}

// bad: push run into a field of a freshly constructed struct → fold into the
// constructor `var rs <- RegStruct(nums <- [10, 20], ...)` (1 warning)
def bad_ctor_field_pushes() {
    var rs <- RegStruct(inner = Inner(v = 7), tag = 3)
    rs.nums |> push(10)
    rs.nums |> push(20)
}

// bad: same fold for a default-initialized struct var (1 warning)
def bad_uninit_struct_field_pushes() {
    var rs : RegStruct
    rs.nums |> push(1)
    rs.nums |> push(2)
}

// bad: push run into a deep chain (1 warning, general literal suggestion)
def bad_chain_pushes(var fo : array<Holder>) {
    fo[13].bar.far |> push(1)
    fo[13].bar.far |> push(2)
}

// good: constructor already initializes the field — pushes are a deliberate append
def good_ctor_append() {
    var rs <- RegStruct(tag = 3, nums <- [1])
    rs.nums |> push(10)
    rs.nums |> push(20)
}

// good: single push into a chain
def good_single_chain_push(var rs : RegStruct) {
    rs.nums |> push(10)
}

// good: interleaved pushes to different chains — no contiguous run per chain
def good_interleaved_chains(var a : Bar; var b : Bar) {
    a.far |> push(1)
    b.far |> push(2)
    a.far |> push(3)
}

def some_index() : int {
    return 13
}

// good: computed (impure) index in the chain — receiver identity not provable
def good_impure_chain(var fo : array<Holder>) {
    fo[some_index()].bar.far |> push(1)
    fo[some_index()].bar.far |> push(2)
}

// good: 3-arg positional-insert push must NOT collapse into a literal
def good_positional_insert(var rs : RegStruct) {
    rs.nums |> push(1, 0)
    rs.nums |> push(2, 0)
}
