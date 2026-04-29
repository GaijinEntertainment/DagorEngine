options gen2
require dastest/testing_boost public
require daslib/strings_boost

// Test print format for gen2 syntax output

enum TestColor {
    red
    green
    blue
}

struct TestFoo {
    a : int
    b : float
}

struct TestBar {
    name : string
    items : array<int>
}

variant TestVariant {
    i : int
    f : float
}

variant TestComplexVariant {
    simple : int
    nested : array<int>
}

[test]
def test_struct_format(t : T?) {
    t |> run("struct with names") @@(t : T?) {
        var f = TestFoo(a = 1, b = 2.23)
        var s = sprint(f, print_flags.escapeString | print_flags.namesAndDimensions | print_flags.singleLine)
        t |> equal(s, "TestFoo = TestFoo( a = 1, b = 2.23)")
    }
    t |> run("struct without names") @@(t : T?) {
        var f = TestFoo(a = 1, b = 2.23)
        var s = sprint(f, print_flags.singleLine)
        t |> equal(s, "TestFoo = ( 1, 2.23)")
    }
    t |> run("struct string interpolation") @@(t : T?) {
        var f = TestFoo(a = 42, b = 3.14)
        t |> equal("{f}", "( 42, 3.14)")
    }
}

[test]
def test_tuple_format(t : T?) {
    t |> run("tuple default") @@(t : T?) {
        var tup = (3, 0.14)
        t |> equal("{tup}", "( 3, 0.14)")
    }
    t |> run("tuple with fullTypeInfo") @@(t : T?) {
        var tup = (3, 0.14)
        var s = sprint(tup, print_flags.fullTypeInfo | print_flags.singleLine)
        // fullTypeInfo adds the type prefix
        t |> success(s |> starts_with("tuple<"))
        t |> success(s |> find("( 3, 0.14)") != -1)
    }
}

[test]
def test_dynamic_array_format(t : T?) {
    t |> run("array default") @@(t : T?) {
        var arr <- [1, 2, 3]
        t |> equal("{arr}", "[ 1, 2, 3]")
    }
    t |> run("array of strings") @@(t : T?) {
        var arr <- ["a", "b", "c"]
        // without escapeString, strings are raw (no quotes) - space before each element
        var s = sprint(arr, print_flags.singleLine)
        t |> success(s |> find("[") != -1, "should start with [")
        t |> success(s |> find("a,") != -1 || s |> find("a, ") != -1, "should contain a")
    }
    t |> run("array with fullTypeInfo") @@(t : T?) {
        var arr <- [1, 2, 3]
        var s = sprint(arr, print_flags.fullTypeInfo | print_flags.escapeString | print_flags.singleLine)
        t |> success(s |> starts_with("array<int>"))
        t |> success(s |> find("( 1, 2, 3)") != -1)
    }
}

[test]
def test_fixed_array_format(t : T?) {
    t |> run("fixed_array default") @@(t : T?) {
        var a = fixed_array(1, 2, 3, 4, 5, 6)
        t |> equal("{a}", "[ 1, 2, 3, 4, 5, 6]")
    }
    t |> run("fixed_array with fullTypeInfo") @@(t : T?) {
        var a = fixed_array(1, 2, 3)
        var s = sprint(a, print_flags.fullTypeInfo | print_flags.singleLine)
        t |> success(s |> find("fixed_array<int>( 1, 2, 3)") != -1)
    }
}

[test]
def test_table_format(t : T?) {
    t |> run("table default") @@(t : T?) {
        var tab <- {1 => "one"}
        var s = sprint(tab, print_flags.singleLine)
        t |> success(s |> find("1 => one") != -1, "should contain 1 => one")
        // tables use curly braces without fullTypeInfo
        t |> success(s |> find("(") == -1, "should not use parens without fullTypeInfo")
    }
    t |> run("table with escape") @@(t : T?) {
        var tab <- {1 => "one"}
        var s = sprint(tab, print_flags.escapeString | print_flags.singleLine)
        t |> success(s |> find("1 => ") != -1, "should use => separator")
    }
    t |> run("table with fullTypeInfo") @@(t : T?) {
        var tab <- {1 => "one"}
        var s = sprint(tab, print_flags.fullTypeInfo | print_flags.escapeString | print_flags.singleLine)
        t |> success(s |> starts_with("table<int"), "should start with table type")
        t |> success(s |> find("(") != -1, "should use parens with fullTypeInfo")
        t |> success(s |> find("1 => ") != -1, "should use => separator")
    }
}

[test]
def test_enum_format(t : T?) {
    t |> run("enum default") @@(t : T?) {
        var c = TestColor.green
        t |> equal("{c}", "green")
    }
    t |> run("enum with fullTypeInfo") @@(t : T?) {
        var c = TestColor.blue
        var s = sprint(c, print_flags.fullTypeInfo | print_flags.singleLine)
        t |> success(s |> find("TestColor.blue") != -1)
    }
}

[test]
def test_separator_format(t : T?) {
    t |> run("struct uses commas not semicolons") @@(t : T?) {
        var f = TestFoo(a = 1, b = 2.0)
        var s = sprint(f, print_flags.namesAndDimensions | print_flags.singleLine)
        t |> success(s |> find(",") != -1, "should have comma separator")
        t |> success(s |> find(";") == -1, "should not have semicolon separator")
    }
    t |> run("table uses => not :") @@(t : T?) {
        var tab <- {1 => "one"}
        var s = sprint(tab, print_flags.escapeString | print_flags.singleLine)
        t |> success(s |> find("=>") != -1, "should use => separator")
    }
}

[test]
def test_handle_format(t : T?) {
    t |> run("handle uses parens") @@(t : T?) {
        // Handles print with Type(...) when namesAndDimensions is set
        // We can't easily construct a handle here, but we can verify
        // that basic types don't emit [[ ]]
        var f = TestFoo(a = 1, b = 2.0)
        var s = sprint(f, print_flags.namesAndDimensions | print_flags.singleLine)
        t |> success(s |> find("[[") == -1, "should not contain [[")
        t |> success(s |> find("]]") == -1, "should not contain ]]")
    }
}

[test]
def test_variant_format(t : T?) {
    t |> run("variant simple single line") @@(t : T?) {
        var v = TestVariant(i = 42)
        var s = sprint(v, print_flags.namesAndDimensions | print_flags.singleLine)
        t |> success(s |> find("variant(") != -1, "should use variant() syntax")
        t |> success(s |> find("i = 42") != -1, "should contain field name and value")
        t |> success(s |> find("\n") == -1, "simple variant should be single line")
    }
    t |> run("variant with fullTypeInfo") @@(t : T?) {
        var v = TestVariant(f = 3.14)
        var s = sprint(v, print_flags.namesAndDimensions | print_flags.singleLine | print_flags.fullTypeInfo)
        t |> success(s |> find("variant<") != -1, "should show variant type info")
        t |> success(s |> find("variant(") != -1, "should use variant() syntax")
    }
}
