options gen2
require dastest/testing_boost public

// Tests that { } table initialization and { for } table comprehensions work
// as sub-data in composite types: struct fields, array elements, function args,
// tuples, variants, move semantics, bare blocks.

struct Config {
    name : string
    tags : table<string; int>
}

struct Nested {
    lookup : table<int; table<string; int>>
}

variant Setting {
    as_int : int
    as_map : table<string; int>
}

// --- helpers ---

def count_keys(tab : table<string; int>) : int {
    return length(tab)
}

def sum_values(tab : table<string; int>) : int {
    var total = 0
    for (v in values(tab)) {
        total += v
    }
    return total
}

def make_table() : table<string; int> {
    return {"hello" => 1, "world" => 2}
}

def make_nested_table() : table<int; table<string; int>> {
    return {1 => {"a" => 10}, 2 => {"b" => 20}}
}

def accept_table(tab : table<string; int>) : int {
    return length(tab)
}

// --- tests ---

[test]
def test_table_literal_in_struct(t : T?) {
    t |> run("struct field assigned with braces") @(t : T?) {
        var cfg = Config(name = "test", tags = {"a" => 1, "b" => 2})
        t |> equal(cfg.name, "test")
        t |> equal(length(cfg.tags), 2)
        t |> success(key_exists(cfg.tags, "a"))
        t |> success(key_exists(cfg.tags, "b"))
    }
    t |> run("nested braces in struct field") @(t : T?) {
        var n = Nested(lookup = {1 => {"x" => 10}, 2 => {"y" => 20}})
        t |> equal(length(n.lookup), 2)
        t |> success(key_exists(n.lookup, 1))
        t |> success(key_exists(n.lookup, 2))
    }
}

[test]
def test_table_literal_in_array(t : T?) {
    t |> run("array of tables [braces, braces]") @(t : T?) {
        var arr = [{"a" => 1}, {"b" => 2, "c" => 3}]
        t |> equal(length(arr), 2)
        t |> equal(length(arr[0]), 1)
        t |> equal(length(arr[1]), 2)
        t |> success(key_exists(arr[0], "a"))
        t |> success(key_exists(arr[1], "b"))
        t |> success(key_exists(arr[1], "c"))
    }
    t |> run("array of tables [braces, braces, braces]") @(t : T?) {
        var arr = [{"x" => 10}, {"y" => 20}, {"z" => 30}]
        t |> equal(length(arr), 3)
        t |> success(key_exists(arr[0], "x"))
        t |> success(key_exists(arr[1], "y"))
        t |> success(key_exists(arr[2], "z"))
    }
}

[test]
def test_table_literal_in_function_arg(t : T?) {
    t |> run("braces as function argument") @(t : T?) {
        let n = count_keys({"x" => 1, "y" => 2, "z" => 3})
        t |> equal(n, 3)
    }
    t |> run("braces summed via function") @(t : T?) {
        let s = sum_values({"a" => 10, "b" => 20})
        t |> equal(s, 30)
    }
}

[test]
def test_table_literal_return(t : T?) {
    t |> run("return braces from function") @(t : T?) {
        let tab = make_table()
        t |> equal(length(tab), 2)
        t |> success(key_exists(tab, "hello"))
        t |> success(key_exists(tab, "world"))
    }
    t |> run("return nested braces from function") @(t : T?) {
        let tab = make_nested_table()
        t |> equal(length(tab), 2)
        t |> success(key_exists(tab, 1))
        t |> success(key_exists(tab, 2))
    }
}

[test]
def test_table_literal_in_tuple(t : T?) {
    t |> run("braces as tuple element") @(t : T?) {
        var tup = "config" => {"k" => 42}
        t |> equal(tup._0, "config")
        t |> equal(length(tup._1), 1)
        t |> success(key_exists(tup._1, "k"))
    }
}

[test]
def test_table_literal_in_variant(t : T?) {
    t |> run("braces in variant alternative") @(t : T?) {
        var s = Setting(as_map = {"x" => 5, "y" => 10})
        t |> success(s is as_map)
        t |> equal(length(s as as_map), 2)
        t |> success(key_exists(s as as_map, "x"))
    }
}

[test]
def test_nested_table_literals(t : T?) {
    t |> run("braces of braces tables") @(t : T?) {
        var tab = {1 => {"A", "B"}, 3 => {"C", "D"}}
        t |> equal(length(tab), 2)
        t |> success(key_exists(tab, 1))
        t |> success(key_exists(tab, 3))
    }
    t |> run("nested braces => braces => value") @(t : T?) {
        var tab = {"x" => {"a" => 1, "b" => 2}, "y" => {"c" => 3}}
        t |> equal(length(tab), 2)
        t |> success(key_exists(tab, "x"))
        t |> success(key_exists(tab, "y"))
    }
}

[test]
def test_table_move_semantics(t : T?) {
    t |> run("var <- table literal") @(t : T?) {
        var tab <- {"a" => 1, "b" => 2}
        t |> equal(length(tab), 2)
        t |> success(key_exists(tab, "a"))
        t |> success(key_exists(tab, "b"))
    }
    t |> run("var <- nested table literal") @(t : T?) {
        var tab <- {1 => {"x" => 100}, 2 => {"y" => 200}}
        t |> equal(length(tab), 2)
        t |> success(key_exists(tab, 1))
        t |> success(key_exists(tab, 2))
    }
    t |> run("var <- table comprehension") @(t : T?) {
        var tab <- {for (i in range(4)); i => i * 10}
        t |> equal(length(tab), 4)
        t |> success(key_exists(tab, 0))
        t |> success(key_exists(tab, 3))
    }
    t |> run("struct field <- table literal") @(t : T?) {
        var cfg = Config(name = "moved", tags <- {"p" => 10, "q" => 20})
        t |> equal(cfg.name, "moved")
        t |> equal(length(cfg.tags), 2)
        t |> success(key_exists(cfg.tags, "p"))
        t |> success(key_exists(cfg.tags, "q"))
    }
    t |> run("struct nested field <- table literal") @(t : T?) {
        var n = Nested(lookup <- {1 => {"a" => 10}, 2 => {"b" => 20}})
        t |> equal(length(n.lookup), 2)
        t |> success(key_exists(n.lookup, 1))
        t |> success(key_exists(n.lookup, 2))
    }
    t |> run("assign <- table literal to existing var") @(t : T?) {
        var tab : table<string; int>
        tab <- {"a" => 1, "b" => 2}
        t |> equal(length(tab), 2)
        t |> success(key_exists(tab, "a"))
        t |> success(key_exists(tab, "b"))
    }
    t |> run("assign <- nested table literal to existing var") @(t : T?) {
        var tab : table<int; table<string; int>>
        tab <- {1 => {"x" => 10}, 2 => {"y" => 20}}
        t |> equal(length(tab), 2)
        t |> success(key_exists(tab, 1))
        t |> success(key_exists(tab, 2))
    }
    t |> run("assign <- table comprehension to existing var") @(t : T?) {
        var tab : table<int; int>
        tab <- {for (i in range(5)); i => i * 100}
        t |> equal(length(tab), 5)
        t |> success(key_exists(tab, 0))
        t |> success(key_exists(tab, 4))
    }
}

[test]
def test_table_comprehension(t : T?) {
    t |> run("brace-for creates table") @(t : T?) {
        var tab <- {for (i in range(3)); i => i * 10}
        t |> equal(length(tab), 3)
        t |> success(key_exists(tab, 0))
        t |> success(key_exists(tab, 1))
        t |> success(key_exists(tab, 2))
    }
    t |> run("brace-for with string keys") @(t : T?) {
        var tab <- {for (i in range(3)); "k{i}" => i}
        t |> equal(length(tab), 3)
        t |> success(key_exists(tab, "k0"))
        t |> success(key_exists(tab, "k1"))
        t |> success(key_exists(tab, "k2"))
    }
    t |> run("brace-for with expression values") @(t : T?) {
        var tab <- {for (i in range(4)); i => i * i}
        t |> equal(length(tab), 4)
        t |> success(key_exists(tab, 0))
        t |> success(key_exists(tab, 3))
    }
    t |> run("var <- brace-for with nested table value") @(t : T?) {
        var tab <- {for (i in range(2)); i => {"v{i}" => i}}
        t |> equal(length(tab), 2)
        t |> success(key_exists(tab, 0))
        t |> success(key_exists(tab, 1))
    }
}

[test]
def test_table_literal_in_bare_block(t : T?) {
    t |> run("braces inside bare block") @(t : T?) {
        var found = false
        {
            var tab = {"needle" => 1}
            found = key_exists(tab, "needle")
        }
        t |> success(found)
    }
    t |> run("nested braces inside bare block") @(t : T?) {
        var n = 0
        {
            var tab = {1 => {"a" => 42}}
            n = length(tab)
        }
        t |> equal(n, 1)
    }
    t |> run("struct with braces field inside bare block") @(t : T?) {
        var tag_count = 0
        {
            var cfg = Config(name = "block", tags = {"x" => 1, "y" => 2})
            tag_count = length(cfg.tags)
        }
        t |> equal(tag_count, 2)
    }
    t |> run("move braces inside bare block") @(t : T?) {
        var n = 0
        {
            var tab <- {"a" => 1, "b" => 2, "c" => 3}
            n = length(tab)
        }
        t |> equal(n, 3)
    }
    t |> run("brace-for comprehension inside bare block") @(t : T?) {
        var n = 0
        {
            var tab <- {for (i in range(4)); i => i}
            n = length(tab)
        }
        t |> equal(n, 4)
    }
}
