options gen2
require dastest/testing_boost public
require math

// Regression pin for the AST-serializer dangling-symbol crash: a ctor (or global) referenced
// ONLY from a struct field initializer (`w : WithInit = WithInit()`) gets freed by
// removeUnusedSymbols while the surviving structure's field-init expression keeps the raw
// pointer - ClearUnusedSymbols nulls dangling refs in field inits but missed
// ExprMakeStruct.constructor and ExprVar.variable, so dastest's --ser sweep read a dangling
// Function*/Variable* (nondeterministic SIGSEGV, all three extended_checks CI lanes). The
// structs below stay deliberately UNUSED by any function so the symbols' only anchor is the
// field init; the CI serialization sweep over tests/ is what exercises the pin.

struct PlainPod {
    a : int
    b : float
}

struct WithInit {
    x : int = 7
}

struct Nested {
    p : PlainPod
    w : WithInit = WithInit()
}

[safe_when_uninitialized]
struct OptedOut {
    x : int = 7
}

// mutable so const folding can't erase the ExprVar - the global's only anchor is the field init
var g_seed = 13

struct UsesGlobal {
    s : int = g_seed
}

// builtin-module global (folded addConstant, index=-2): builtin modules are never pruned, so
// the cleaner must LEAVE this ref alone (only thisModule's freed globals get nulled)
struct UsesBuiltinGlobal {
    p : float = PI
}

// default-argument initializers are the same story: MarkSymbolUse skips them too
// (canVisitArgumentInit is false there), so a helper whose only anchor is `x = arg_helper()`
// is freed while the surviving function keeps the raw pointer in arg->init. arg_helper writes
// a global so const folding can't erase the call, and g_arg_state rides along as the Variable
// half of the same pin
var private g_arg_state = 1

def private arg_helper() : int {
    g_arg_state ++
    return g_arg_state
}

[export]
def default_arg_anchor(x : int = arg_helper()) : int {
    return x
}

variant PodVariant {
    i : int
    f : float
}

variant InitVariant {
    i : int
    w : WithInit
}

[test]
def test_field_init_ctor_survives_marking(t : T?) {
    t |> run("the structs above exist and stay otherwise unused") @(t : T?) {
        t |> success(true, "the pin is the file itself under the --ser sweep")
    }
}
