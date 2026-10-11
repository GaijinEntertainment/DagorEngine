options gen2
options rtti
options no_aot
options no_coverage
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/ast
require daslib/rtti
require daslib/ast_boost
require daslib/ast_print
require strings
require daslib/strings_boost

var g_seed = 1

[never_inline]
def apply_ro(v : int; blk : block<(x : int) : int>) : int {
    return invoke(blk, v)
}

[never_inline]
def apply_rw(v : int; blk : block<(var y : int) : int>) : int {
    return invoke(blk, v)
}

[never_inline]
def fixture_shape() : int {
    let a = g_seed + 1
    var b = a * 2
    b += apply_ro(b) $(x : int) : int {
        return x + 1
    }
    b += apply_rw(b) $(var y : int) : int {
        y ++
        return y
    }
    return b
}

[never_inline]
def fixture_args(var out : int; spare : int) : int { // nolint:LINT012 - spare is unused on purpose: it is the fixture for the printer's unused-argument marker
    out ++
    return out
}

[never_inline]
def fixture_raw_assign(var out : int; src : int) : int {
    var tmp : int
    out !== src
    tmp !<- out
    out !:= tmp
    return out
}

def printed_fixture(t : T?; name : string) : string {
    var txt = ""
    compiling_module() |> for_each_function(name) $(var func) {
        if (empty(txt)) {
            txt = build_string() $(var w) {
                printFunc(func, unsafe(addr(w)))
            }
        }
    }
    t |> success(!empty(txt), "{name} not found in the compiling module")
    return txt
}

[test]
def test_ast_print_let_var(t : T?) {
    g_seed = 1
    t |> equal(fixture_shape(), 19)
    let golden = join(fixed_array<string>(
        "[never_inline]",
        "def fixture_shape : int const \{",
        "    let a : int const = (g_seed + 1)",
        "    var b : int = (a * 2)",
        "    (b += apply_ro(b,$(x : int const):int \{",
        "        return (x + 1)",
        "    \}))",
        "    (b += apply_rw(b,$(var y : int -const):int \{",
        "        ++(y)",
        "        return y",
        "    \}))",
        "    return b",
        "\}",
        "",
        ""), "\n")
    t |> run("locals and closure arguments print their own mutability") @(t : T?) {
        t |> equal(printed_fixture(t, "fixture_shape"), golden)
    }
}

[test]
def test_ast_print_raw_assignments(t : T?) {
    t |> run("the raw copy, move and clone print their own spelling") @(t : T?) {
        let out = 0
        t |> equal(fixture_raw_assign(out, 5), 5)
        let txt = printed_fixture(t, "fixture_raw_assign")
        t |> success(find(txt, " !== ") >= 0, "raw copy printed: {txt}")
        t |> success(find(txt, " !<- ") >= 0, "raw move printed: {txt}")
        t |> success(find(txt, "out !== tmp") >= 0, "a raw clone of a copyable type lowers to a raw copy and prints as one: {txt}")
    }
}

[test]
def test_ast_print_function_arguments(t : T?) {
    g_seed = 1
    t |> equal(fixture_args(g_seed, 0), 2)
    let golden = join(fixed_array<string>(
        "[never_inline]",
        "def fixture_args ( var out : int -const;  /*unused*/ spare : int const )  : int const \{",
        "    ++(out)",
        "    return out",
        "\}",
        "",
        ""), "\n")
    t |> run("function arguments print their mutability and the unused marker") @(t : T?) {
        t |> equal(printed_fixture(t, "fixture_args"), golden)
    }
}
