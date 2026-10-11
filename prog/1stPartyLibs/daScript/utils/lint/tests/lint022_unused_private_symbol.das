options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// LINT022: a private declaration that is never used
//
// Problem: `private` limits a symbol to its own module, so if nothing in that
// module references it the declaration is dead code no other rule looks for.
// It compiles, formats and lints clean while nobody outside can ever reach it.
// Applies to functions, structures, classes and enumerations.
//
// Bad pattern:
//   def private rollback(var st : State) { ... }   // no caller anywhere
//   struct private Unused { x : int }              // no type mentions it
//
// Good pattern:
//   remove it, or drop `private` when another module needs it.

expect 50503:5

require daslib/lint

// bad: private and referenced nowhere (1 warning)
def private dead_helper(x : int) : int {
    return x * 2
}

// good: private and called
def private live_helper(x : int) : int {
    return x + 1
}

// good: private, referenced only through a @@ function pointer
def private pointer_helper(x : int) : int {
    return x - 1
}

// good: an annotated private function is an entry point nothing calls from source
[init]
def private register_something() {
    pass
}

// good: public and unused - another module may call it
def public_unused(x : int) : int {
    return x * 3
}

// bad: no type in the module mentions it (1 warning)
struct private DeadStruct {
    x : int
}

// bad: a field of its own type keeps a linked list alive, which is not a use (1 warning)
struct private DeadList {
    next : DeadList?
    v : int
}

// bad: a class is referenced by its own method fields and generated constructor (1 warning)
class private DeadClass {
    def get() : int {
        return 1
    }
}

// good: reachable only as another structure's field type
struct private LiveField {
    y : int
}

// good: reachable only as a container element type
struct private LiveElement {
    w : int
}

// good: reachable only through a private function's signature
struct private LiveSig {
    s : int
}

// good: reachable only from inside a lambda body, which compiles to a generated function
struct private LiveInLambda {
    q : int
}

// good: named by a string literal, the only trace a find_structure-style lookup leaves
struct private LookedUpByName {
    n : int
}

// bad: no value of it is ever used (1 warning)
enum private DeadEnum {
    A
    B
}

// good: one of its values is used
enum private LiveEnum {
    C
    D
}

// good: globals are NOT checked. Const folding and static_if elide the very references the check
// would need - a `static_if (log_enabled)` body leaves no trace of the flag it tests
var private unread_global = 7
let private DEAD_CONST = 9

// good: read below
var private live_global = 8

struct Holder {
    f : LiveField
}

def private takes(a : LiveSig) : int => a.s

[export]
def main {
    let fp = @@pointer_helper
    let loc : array<LiveElement>
    let h = Holder(f = LiveField(y = 2))
    print("{live_helper(1)} {invoke(fp, 2)} {length(loc)} {h.f.y}\n")
    print("{LiveEnum.C} {live_global} {takes(LiveSig(s = 3))} {public_unused(1)}\n")
    let looked_up = find_structure_name("LookedUpByName")
    print("{looked_up}\n")
    let lam <- @@() {
        let inner = LiveInLambda(q = 4)
        print("{inner.q}\n")
    }
    invoke(lam)
}

def private find_structure_name(nm : string) : string => nm
