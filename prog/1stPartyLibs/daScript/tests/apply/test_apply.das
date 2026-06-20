options gen2
require daslib/apply
require dastest/testing_boost public

variant FooBar {
    foo : int
    bar : string
}

[test]
def test_apply_variant(t : T?) {
    t |> run("contents") @(t : T?) {
        apply(FooBar(foo = 1)) $(name, field) {
            if (name != "foo") {
                t->fatal("unknown field")
            }
            static_if (typeinfo stripped_typename(field) == "int") {
                t |> equal(field, 1)
            } else {
                t->fatal("we got unsupported field here")
            }
        }
        apply(FooBar(bar = "hello")) $(name, field) {
            if (name != "bar") {
                t->fatal("unknown field")
            }
            static_if (typeinfo stripped_typename(field) == "string") {
                t |> equal(field, "hello")
            } else {
                t->fatal("we got unsupported field here")
            }
        }
    }
}

struct Foo {
    a : int
    b : string
}

[test]
def test_apply_struct(t : T?) {
    t |> run("contents") @(t : T?) {
        var foo = Foo(a = 1, b = "hello")
        apply(foo) $(name, field) {
            if (name != "a" && name != "b") {
                t->fatal("unknown field")
            }
            static_if (typeinfo stripped_typename(field) == "int") {
                t |> equal(field, 1)
            } static_elif (typeinfo stripped_typename(field) == "string") {
                t |> equal(field, "hello")
            }
        }
    }
    t |> run("assign") @(t : T?) {
        var foo = Foo(a = 1, b = "hello")
        apply(foo) $(name, field) {
            static_if (typeinfo stripped_typename(field) == "int") {
                field = 13
            } static_elif (typeinfo stripped_typename(field) == "string") {
                field = "13"
            }
        }
        apply(foo) $(name, field) {
            t |> equal("{field}", "13")
        }
    }
}

[test]
def test_apply_tuple(t : T?) {
    t |> run("contents") @(t : T?) {
        var foo = (1, "hello")
        apply(foo) $(name, field) {
            if (name != "_0" && name != "_1") {
                t->fatal("unknown field")
            }
            static_if (typeinfo stripped_typename(field) == "int") {
                t |> equal(field, 1)
            } static_elif (typeinfo stripped_typename(field) == "string") {
                t |> equal(field, "hello")
            }
        }
    }
    t |> run("assign") @(t : T?) {
        var foo = (1, "hello")
        apply(foo) $(name, field) {
            static_if (typeinfo stripped_typename(field) == "int") {
                field = 13
            } static_elif (typeinfo stripped_typename(field) == "string") {
                field = "13"
            }
        }
        apply(foo) $(name, field) {
            t |> equal("{field}", "13")
        }
    }
}

[test]
def test_apply_named_tuple(t : T?) {
    var foo = (a = 1, b = "hello")
    apply(foo) $(name, field) {
        static_if (name == "a") {
            t |> equal(field, 1)
        } static_elif (name == "b") {
            t |> equal(field, "hello")
        } else {
            t->fatal("unknown field")
        }
    }
}

[test]
def test_apply_imm_struct(t : T?) {
    t |> run("contents") @(t : T?) {
        let foo = Foo(a = 1, b = "hello")
        apply_imm(foo) $(name, field) {
            if (name != "a" && name != "b") {
                t->fatal("unknown field")
            }
            static_if (typeinfo stripped_typename(field) == "int") {
                t |> equal(field, 1)
            } static_elif (typeinfo stripped_typename(field) == "string") {
                t |> equal(field, "hello")
            }
        }
    }
    t |> run("assign") @(t : T?) {
        var foo = Foo(a = 1, b = "hello")
        apply_imm(foo) $(name, field) {
            static_if (typeinfo stripped_typename(field) == "int") {
                field = 13
            } static_elif (typeinfo stripped_typename(field) == "string") {
                field = "13"
            }
        }
        apply_imm(foo) $(name, field) {
            t |> equal("{field}", "13")
        }
    }
}

struct FooAnn {
    @min = 0 @max = 3 a : int = 1
    b : string
}

[test]
def test_apply_imm_annotations(t : T?) {
    let foo = FooAnn(a = 2, b = "hi")
    apply_imm(foo) $(name; field; annotations) {
        static_if (name == "a") {
            t |> equal(field, 2)
            t |> equal(length(annotations), 2)
            for (ann in annotations) {
                if (ann.name == "min" && ann.data is tInt) {
                    t |> equal(ann.data as tInt, 0)
                } elif (ann.name == "max" && ann.data is tInt) {
                    t |> equal(ann.data as tInt, 3)
                }
            }
        } else {
            t |> equal(field, "hi")
            t |> equal(length(annotations), 0)
        }
    }
}

struct Foos {
    a : int
    b : float
}

tuple Foot {
    a : int
    b : float
}

variant Foov {
    a : int
    b : float
}

[test]
def test_static_assert_on_name(_t : T?) {
    var f = Foos(a = 1, b = 2.0)
    apply(f) $(name, value) {
        static_if (name == "a") {
            assert(value == 1)
        } else {
            assert(value == 2.0)
        }
    }
    var g  = tuple(1, 2.0)
    apply(g) $(name, value) {
        static_if (name == "_0") {
            assert(value == 1)
        } else {
            assert(value == 2.0)
        }
    }
    var gg = Foot(a = 1, b = 2.0)
    apply(gg) $(name, value) {
        static_if (name == "a") {
            assert(value == 1)
        } else {
            assert(value == 2.0)
        }
    }
    var h = Foov(a = 1)
    apply(h) $(name, value) {
        static_if (name == "a") {
            assert(value == 1)
        } else {
            assert(value == 0.0)
        }
    }
    var h2 = Foov(b = 2.0)
    apply(h2) $(name, value) {
        static_if (name == "b") {
            assert(value == 2.0)
        } else {
            assert(value == 0)
        }
    }
}

// ===== Regular (non-workhorse) field types: nested struct + array =====

struct Inner {
    v : int
}

struct RegStruct {
    inner : Inner
    nums  : array<int>
    tag   : int
}

[test]
def test_apply_regular_struct_read(t : T?) {
    var rs <- RegStruct(inner = Inner(v = 7), tag = 3, nums <- [10, 20])
    var seen = 0
    apply(rs) $(name : string; field) {
        static_if (typeinfo stripped_typename(field) == "Inner") {
            t |> equal(field.v, 7)
            seen++
        } static_elif (typeinfo is_array(field)) {
            t |> equal(length(field), 2)
            seen++
        } static_elif (typeinfo stripped_typename(field) == "int") {
            t |> equal(field, 3)
            seen++
        }
    }
    t |> equal(seen, 3)
}

[test]
def test_apply_regular_struct_mutate(t : T?) {
    var rs : RegStruct
    apply(rs) $(name : string; var field) {
        static_if (typeinfo stripped_typename(field) == "Inner") {
            field.v = 99
        } static_elif (typeinfo is_array(field)) {
            field |> push(5)
        } static_elif (typeinfo stripped_typename(field) == "int") {
            field = 42
        }
    }
    t |> equal(rs.inner.v, 99)   // nested-struct field mutation propagates
    t |> equal(length(rs.nums), 1)
    t |> equal(rs.tag, 42)
}

[test]
def test_apply_regular_tuple(t : T?) {
    var tu : tuple<Inner; int>
    tu._0 = Inner(v = 8)
    tu._1 = 5
    apply(tu) $(name : string; var field) {
        static_if (typeinfo stripped_typename(field) == "Inner") {
            field.v = 80
        } static_elif (typeinfo stripped_typename(field) == "int") {
            field = 50
        }
    }
    t |> equal(tu._0.v, 80)
    t |> equal(tu._1, 50)
}

variant RegVar {
    sm : Inner
    n  : int
}

[test]
def test_apply_regular_variant(t : T?) {
    var rv = RegVar(sm = Inner(v = 11))
    apply(rv) $(name : string; var field) {
        static_if (typeinfo stripped_typename(field) == "Inner") {
            field.v = 110
        }
    }
    t |> equal(unsafe(rv.sm).v, 110)   // active-arm struct mutation propagates
}

// ===== const self (read-only) =====

[test]
def test_apply_const_self(t : T?) {
    let cs = Foo(a = 5, b = "world")
    var seen = 0
    apply(cs) $(name : string; field) {
        static_if (typeinfo stripped_typename(field) == "int") {
            t |> equal(field, 5)
            seen++
        } static_elif (typeinfo stripped_typename(field) == "string") {
            t |> equal(field, "world")
            seen++
        }
    }
    t |> equal(seen, 2)
    // mutating a const-self field does not compile (field binds as `let &`) — by design.
}

// ===== variant: mutation through the active arm =====

[test]
def test_apply_variant_mutate(t : T?) {
    var v = FooBar(foo = 1)
    apply(v) $(name : string; var field) {
        static_if (typeinfo stripped_typename(field) == "int") {
            field = 42
        }
    }
    t |> equal(unsafe(v.foo), 42)
    var v2 = FooBar(bar = "hi")
    apply(v2) $(name : string; var field) {
        static_if (typeinfo stripped_typename(field) == "string") {
            field = "bye"
        }
    }
    t |> equal(unsafe(v2.bar), "bye")
}

// ===== annotations (struct, 3-arg block) =====

[test]
def test_apply_struct_annotations(t : T?) {
    let foo = FooAnn(a = 2, b = "hi")
    apply(foo) $(name : string; field; annotations) {
        static_if (name == "a") {
            t |> equal(length(annotations), 2)
            for (ann in annotations) {
                if (ann.name == "min" && ann.data is tInt) {
                    t |> equal(ann.data as tInt, 0)
                } elif (ann.name == "max" && ann.data is tInt) {
                    t |> equal(ann.data as tInt, 3)
                }
            }
        } else {
            t |> equal(length(annotations), 0)
        }
    }
}

// ===== rvalue self is evaluated exactly once (single-bind, not per-field) =====

var g_apply_make_count = 0

def make_counted_foo() : Foo {
    g_apply_make_count++
    return Foo(a = 1, b = "x")
}

[test]
def test_apply_rvalue_single_eval(t : T?) {
    g_apply_make_count = 0
    var seen = 0
    apply(make_counted_foo()) $(name : string; field) {
        seen++
    }
    t |> equal(g_apply_make_count, 1)   // self materialized once, not re-evaluated per field
    t |> equal(seen, 2)
}

// ===== a nested block in the body captures + mutates the field ref =====

def take_int_block(blk : block<(v : int) : void>) {
    invoke(blk, 77)
}

[test]
def test_apply_nested_capture(t : T?) {
    var foo = Foo(a = 1, b = "x")
    apply(foo) $(name : string; var field) {
        static_if (typeinfo stripped_typename(field) == "int") {
            take_int_block() $(v) {
                field = v   // capture the field ref into a nested block, mutate through it
            }
        }
    }
    t |> equal(foo.a, 77)
}

// ===== arbitrary field count (> DAS_MAX_FUNCTION_ARGUMENTS = 32): inline needs no subrange split =====

struct Big36 {
    f00, f01, f02, f03, f04, f05, f06, f07, f08, f09 : int
    f10, f11, f12, f13, f14, f15, f16, f17, f18, f19 : int
    f20, f21, f22, f23, f24, f25, f26, f27, f28, f29 : int
    f30, f31, f32, f33, f34, f35 : int
}

[test]
def test_apply_large_arity(t : T?) {
    var b : Big36
    var count = 0
    apply(b) $(name : string; var field) {
        field = count
        count++
    }
    t |> equal(count, 36)   // every field visited despite exceeding the old per-helper arg limit
    t |> equal(b.f00, 0)
    t |> equal(b.f35, 35)
}

// ===== invoke fallback: a block that uses `return` to skip a field routes to the generated helper =====
//
// With the inline path a bare `return` would escape the enclosing function, so a block that uses
// `return` to skip a field falls back to the per-field invoke codegen, where `return` keeps its
// original block-local "skip this field" meaning. That is also why these compile inside an
// int-returning function — an inlined `return;` would be a return-type mismatch.

struct Triple {
    a : int
    b : int
    c : int
}

def sum_skip_b(tr : Triple) : int {
    var sum = 0
    apply(tr) $(name : string; field) {
        static_if (name == "b") {
            return     // block-local skip; routes apply to the invoke fallback
        }
        sum += field
    }
    return sum
}

[test]
def test_apply_invoke_fallback_struct(t : T?) {
    t |> equal(sum_skip_b(Triple(a = 1, b = 2, c = 3)), 4)   // a + c; b skipped, function returns normally
}

def sum_skip_first(tu : tuple<int; int; int>) : int {
    var sum = 0
    apply(tu) $(name : string; field) {
        static_if (name == "_0") {
            return
        }
        sum += field
    }
    return sum
}

[test]
def test_apply_invoke_fallback_tuple(t : T?) {
    t |> equal(sum_skip_first((1, 2, 3)), 5)   // _1 + _2; _0 skipped
}

variant Triv {
    x : int
    y : int
}

def read_skip_x(v : Triv) : int {
    var got = -1
    apply(v) $(name : string; field) {
        static_if (name == "x") {
            return    // skip the active arm when it is x
        }
        got = field
    }
    return got
}

[test]
def test_apply_invoke_fallback_variant(t : T?) {
    t |> equal(read_skip_x(Triv(x = 7)), -1)   // active arm x is skipped -> got stays -1
    t |> equal(read_skip_x(Triv(y = 9)), 9)    // active arm y is processed
}
