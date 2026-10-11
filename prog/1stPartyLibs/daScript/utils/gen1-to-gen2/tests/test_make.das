options gen2
options indenting = 4
options rtti = true
options strict_smart_pointers = false

require dastest/testing_boost public
require daslib/fio
require strings
require daslib/strings_boost
require ../patch.das
require ../convert.das

//! Make-syntax conversion, on the shapes the v1 grammar allows.
//!
//! The expectations are byte-exact, including where comments land: a comment written before the
//! type stays before it, one written after the type lands after the `(`, and one written inside
//! a `Foo()` initializer joins the leading run — there is nowhere else for it to go once the
//! parens are gone.

let FIXTURE = "{get_das_root()}/utils/gen1-to-gen2/tests/_fixture_make_syntax.das"


def private converted() : string {
    let res <- convert_file(FIXTURE, ConvertOptions(v2 = true))
    return res.output
}


def private body_line(text : string; needle : string) : string {
    for (ln in text |> split("\n")) {
        if (ln |> find(needle) >= 0) return ln |> strip
    }
    return "<{needle} not found>"
}


[test]
def test_make_struct(t : T?) {
    t |> run("every v1 make-struct shape") @(t : T?) {
        let out = converted()
        t |> equal(body_line(out, "v1 ="), "var v1 = Foo( a=1, b=2.0)", "fields become call arguments")
        t |> equal(body_line(out, "v2 ="), "var v2 = /*a*/Foo(/*b*/ a=1)", "comments keep their side of the type")
        t |> equal(body_line(out, "v3 ="), "var v3 = fixed_array(Foo( a=1), Foo( a=2))",
            "';' between instances becomes a second call")
        t |> equal(body_line(out, "v4 ="), "var v4 = Foo()", "no fields at all")
        t |> equal(body_line(out, "v8 ="), "var v8 = Foo( /*c*/ a=1 /*d*/)", "interior comments ride along")
    }

    t |> run("nested make-structs compose in one pass") @(t : T?) {
        t |> equal(body_line(converted(), "v7 ="), "var v7 = Bar( a=Foo( b=2.0))", "inner and outer")
    }
}


[test]
def test_make_array(t : T?) {
    t |> run("array and tuple forms") @(t : T?) {
        let out = converted()
        t |> equal(body_line(out, "v5 ="), "var v5 = fixed_array( 1, 2, 3)", "';' separators become ','")
        t |> equal(body_line(out, "v6 ="), "var v6 =  (4, 5)", "a single tuple value is just parens")
        t |> equal(body_line(out, "v9 ="), "var v9 = fixed_array( (1, 2), (3, 4))", "an array of tuples")
        t |> equal(body_line(out, "v10 ="), "var v10 = struct<Foo#>()", "a temporary struct is constructed by name")
        t |> equal(body_line(out, "v11 ="), "var v11 = fixed_array( 7)", "the dimension goes with the `auto`")
    }

    t |> run("an element type moves outside the call") @(t : T?) {
        let out = converted()
        t |> equal(body_line(out, "v12 ="), "var v12 = fixed_array<int>( 1, 2)", "a typed make-array")
        t |> equal(body_line(out, "v13 ="),
            "var v13 = fixed_array<tuple<string; float>>( (\"a\", 1.0), (\"b\", 2.0))",
            "a typed array of tuples, trailing separator dropped")
    }
}


[test]
def test_make_wrappers(t : T?) {
    t |> run("the array-of and table-of bracket forms") @(t : T?) {
        let out = converted()
        t |> equal(body_line(out, "w1 ="), "var w1 = \{ 1 => \"a\", 2 => \"b\" \}", "table literal")
        t |> equal(body_line(out, "w2 ="), "var w2 = [Foo( a = 1), Foo( a = 2)]", "array of structs")
        t |> equal(body_line(out, "w3 ="), "var w3 = [ 1, 2, 3]", "array of values")
    }

    t |> run("comprehensions keep their body; brackets and for-head change") @(t : T?) {
        let out = converted()
        t |> equal(body_line(out, "w4 ="), "var w4 = [for (x in 0..10); x * x; where x % 2 == 0]", "array form")
        t |> equal(body_line(out, "w5 ="), "var w5 = [iterator for (x in 0..10); x * x]", "generator form")
    }
}


[test]
def test_make_compiles(t : T?) {
    t |> run("the converted make-syntax compiles as gen2") @(t : T?) {
        var issues = ""
        t |> success(compiles_as_gen2(FIXTURE, converted(), issues), "compiles: {issues}")
    }
}


[test]
def test_make_comments_conserved(t : T?) {
    t |> run("no comment is lost by any make rule") @(t : T?) {
        let res <- convert_file(FIXTURE, ConvertOptions(v2 = true))
        t |> equal(res.failed, "", "conversion did not fail the conservation gate")
        t |> equal(length(res.refused), 0, "no rule was refused")
        t |> success(check_comments_preserved(fread(FIXTURE), res.output).ok, "comments conserved")
    }
}
