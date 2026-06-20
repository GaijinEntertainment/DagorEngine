// TARGET SPEC for the tFixedArray rework (FIXED_ARRAY_REWORK.md) — Part B.
// Natural generic-alias semantics:
//   auto(TT)   <- int[4]       =>  TT = int[4]      (whole array, like array<int>)
//   auto(TT)[] <- float[4][4]  =>  TT = float[4]    (peel one level)
//   auto(TT)[] <- M4[10]       =>  TT = M4          (alias preserved)
//   default<TT> after FA binding => zeroed FA
options gen2
require dastest/testing_boost public

typedef M4 = float[4][4]

def plain_name(a : auto(TT)) : string {
    return typeinfo typename(type<TT>)
}

def dim_name(a : auto(TT)[]) : string {
    return typeinfo typename(type<TT>)
}

def dim_name_var(var a : auto(TT)[]) : string {
    a[0] = a[1] // exercise the var-ness: mutable row copy through the peeled binding
    return typeinfo typename(type<TT>)
}

def peel_name(a : auto(TT)) : string {
    return typeinfo typename(type<TT - []>)
}

def zeroed_count(a : auto(TT)) : int {
    var z = default<TT>
    var n = 0
    for (_x in z) {
        n ++
    }
    return n
}

def pick(a : auto(TT)) : string {
    return "plain"
}

def pick(a : auto(TT)[]) : string {
    return "dim"
}

[test]
def test_whole_array_binding(t : T?) {
    t |> run("auto(TT) binds the whole fixed array") @(t : T?) {
        let i4 : int[4]
        t |> equal(plain_name(i4), "int const[4]")
    }
    t |> run("default<TT> is a zeroed FA") @(t : T?) {
        let i4 : int[4]
        t |> equal(zeroed_count(i4), 4)
    }
}

[test]
def test_peel_one_level(t : T?) {
    t |> run("auto(TT)[] peels one level off multi-dim") @(t : T?) {
        // decoration matches the plain form (settled at Stage 2 review): the peeled
        // binding inherits the param's constness — non-var binds const, var binds bare
        let m : float[4][4]
        t |> equal(dim_name(m), "float const[4]")
        var mv : float[4][4]
        t |> equal(dim_name_var(mv), "float[4]")
    }
    t |> run("multi-dim now prefers the dim overload") @(t : T?) {
        // characterized on master as "plain" (test_generics_current.das documents
        // the old behavior); the rework makes auto[] match and win
        let m : float[2][3]
        t |> equal(pick(m), "dim")
    }
    t |> run("1D keeps preferring the dim overload") @(t : T?) {
        let i4 : int[4]
        t |> equal(pick(i4), "dim")
    }
    t |> run("TT -[] unwraps one level") @(t : T?) {
        let i24 : int[2][4]
        t |> equal(peel_name(i24), "int const[4]")
    }
}

[test]
def test_get_key_fixed(t : T?) {
    t |> run("get_key on table<K;V[N]> resolves") @(t : T?) {
        // master's dedicated FA overload (old builtin.das:1407) failed to resolve;
        // deleted in 1g — the base get_key generic covers FA values naturally.
        // get_key is address-based: the value must be a reference INTO the table slot
        var tab : table<string; int[4]>
        var v : int[4]
        for (i in range(4)) {
            v[i] = i + 1
        }
        tab |> insert("a", v)
        var found = ""
        for (vv in values(tab)) {
            found = get_key(tab, vv)
        }
        t |> equal(found, "a")
        delete tab
    }
}
