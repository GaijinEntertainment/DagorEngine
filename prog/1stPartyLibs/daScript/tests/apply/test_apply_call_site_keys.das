options gen2
options indenting = 4

require dastest/testing_boost public
require daslib/apply

//! An `apply` whose block uses `return` to skip a field falls back to generated per-field
//! helpers whose names key the call site. Two applies sharing a line must each get their own
//! instantiation and their own value; a key without a column component gives them one name.

struct KeyFoo {
    a : int
    b : string
}

struct OtherFoo {
    a : int
    b : string
    c : int
}

[test]
def test_two_applies_on_one_line_stay_separate(t : T?) {
    t |> run("same struct twice") @(t : T?) {
        var left = ""
        var right = ""
        apply(KeyFoo(a = 1, b = "x")) $(name, field) { if (name == "b") { return }; left += "{name}={field};" }; apply(KeyFoo(a = 2, b = "y")) $(name, field) { if (name == "a") { return }; right += "{name}={field};" }
        t |> equal(left, "a=1;")
        t |> equal(right, "b=y;")
    }
    t |> run("different structs") @(t : T?) {
        var left = ""
        var right = ""
        apply(KeyFoo(a = 3, b = "p")) $(name, field) { if (name == "a") { return }; left += "{name};" }; apply(OtherFoo(a = 4, b = "q", c = 5)) $(name, field) { if (name == "b") { return }; right += "{name};" }
        t |> equal(left, "b;")
        t |> equal(right, "a;c;")
    }
    t |> run("const and mutable sources") @(t : T?) {
        let fixed = KeyFoo(a = 6, b = "r")
        var movable = KeyFoo(a = 7, b = "s")
        var left = ""
        var right = ""
        apply(fixed) $(name, field) { if (name == "b") { return }; left += "{field};" }; apply(movable) $(name, field) { if (name == "a") { return }; right += "{field};" }
        t |> equal(left, "6;")
        t |> equal(right, "s;")
    }
}
