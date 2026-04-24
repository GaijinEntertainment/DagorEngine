options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/match
require daslib/ast_boost
require daslib/defer

// ===========================
// Edge case types
// ===========================

variant IntOrStr {
    i : int
    s : string
}

struct Nested {
    inner : Point
    tag : string
}

struct Point {
    x : int
    y : int
}

struct Empty {}

variant ABC {
    a : int
    b : float
    c : string
}

// ===========================
// Nested struct matching
// ===========================

[sideeffects]
def nested_match(n : Nested) {
    match (n) {
        if (Nested(inner = Point(x = 0, y = 0), tag = "origin")) {
            return "origin labeled"
        }
        if (Nested(inner = Point(x = $v(x), y = $v(y)))) {
            return "({x},{y})"
        }
        if (_) {
            return "other"
        }
    }
}

// ===========================
// Match on empty struct (no fields)
// ===========================

[sideeffects]
def empty_struct_match(e : Empty) {
    match (e) {
        if (Empty()) {
            return "empty"
        }
        if (_) {
            return "wildcard"
        }
    }
}

// ===========================
// Match string values
// ===========================

[sideeffects]
def string_match(s : string) {
    match (s) {
        if ("hello") {
            return 1
        }
        if ("world") {
            return 2
        }
        if (_) {
            return 0
        }
    }
}

// ===========================
// Boolean match — tests that if(true)/if(false) inside match block
// are treated as pattern match arms, not constant-folded.
// ===========================

[sideeffects]
def bool_match(b : bool) {
    match (b) {
        if (true) {
            return "yes"
        }
        if (false) {
            return "no"
        }
    }
    return "unreachable"
}

// ===========================
// Multiple variant alternatives (3-way)
// ===========================

[sideeffects]
def three_variant_match(v : ABC) {
    match (v) {
        if ($v(a) as a) {
            return "a={a}"
        }
        if ($v(b) as b) {
            return "b={b}"
        }
        if ($v(c) as c) {
            return "c={c}"
        }
    }
    return "unreachable"
}

// ===========================
// Variant match with guard on captured value
// ===========================

[sideeffects]
def variant_guard_match(v : IntOrStr) {
    match (v) {
        if ($v(i) as i && i > 0) {
            return "positive int"
        }
        if ($v(i) as i) {
            return "non-positive int"
        }
        if ($v(s) as s) {
            return "string: {s}"
        }
    }
    return "unreachable"
}

// ===========================
// Tuple with ... (spread)
// ===========================

[sideeffects]
def tuple4_match(t : tuple<int; int; int; int>) {
    match (t) {
        if ((1, ...)) {
            return "starts with 1"
        }
        if ((..., 9)) {
            return "ends with 9"
        }
        if ((0, ..., 0)) {
            return "bookends 0"
        }
        if (_) {
            return "other"
        }
    }
}

// ===========================
// match_expr with captured variable
// ===========================

[sideeffects]
def match_expr_captured(p : Point) {
    match (p) {
        if (Point(x = $v(x), y = match_expr(x))) {   // y == x
            return "diagonal"
        }
        if (Point(x = $v(x), y = match_expr(x * 2))) { // y == x*2
            return "double"
        }
        if (_) {
            return "other"
        }
    }
}

// ===========================
// Dynamic array with spread and literal values
// ===========================

[sideeffects]
def dynamic_array_tail_literal(A : array<int>) {
    match (A) {
        if (array<int>(..., 42)) {
            return "ends with 42"
        }
        if (array<int>(1, ...)) {
            return "starts with 1"
        }
        if (_) {
            return "other"
        }
    }
}

// ===========================
// OR pattern with int values
// ===========================

[sideeffects]
def or_pattern_simple(i : int) {
    match (i) {
        if (1 || 2 || 3) {
            return "low"
        }
        if (10 || 20) {
            return "round"
        }
        if (_) {
            return "other"
        }
    }
}

// ===========================
// Struct match with all fields wildcarded (no field constraints)
// ===========================

[sideeffects]
def struct_all_wildcard(p : Point) {
    match (p) {
        if (Point(x = 0, y = 0)) {
            return "origin"
        }
        if (Point()) {
            return "any point"
        }
    }
}

// ===========================
// Struct match: field ordering shouldn't matter
// ===========================

[sideeffects]
def struct_field_order(p : Point) {
    match (p) {
        if (Point(y = 0, x = $v(x))) {
            return x
        }
        if (_) {
            return -1
        }
    }
}

// ===========================
// Variant match using Variant(field = ...) syntax
// ===========================

[sideeffects]
def variant_field_syntax(v : IntOrStr) {
    match (v) {
        if (IntOrStr(i = 42)) {
            return "the answer"
        }
        if (IntOrStr(i = $v(i))) {
            return "int {i}"
        }
        if (IntOrStr(s = $v(s))) {
            return "str {s}"
        }
        if (_) {
            return "?"
        }
    }
}

// ===========================
// Static array match with exact values
// ===========================

[sideeffects]
def static_array_exact(A : int[3]) {
    match (A) {
        if (fixed_array<int>(1, 2, 3)) {
            return "123"
        }
        if (fixed_array<int>(0, ...)) {
            return "starts 0"
        }
        if (_) {
            return "other"
        }
    }
}

// ===========================
// Nested variant inside struct
// ===========================

struct HasVariant {
    name : string
    value : IntOrStr
}

[sideeffects]
def nested_variant_in_struct(h : HasVariant) {
    match (h) {
        if (HasVariant(name = "x", value = $v(i) as i)) {
            return "x int {i}"
        }
        if (HasVariant(value = $v(s) as s)) {
            return "string {s}"
        }
        if (_) {
            return "other"
        }
    }
}

// ===========================
// multi_match — all matching arms execute
// ===========================

[sideeffects]
def multi_match_categories(n : int) {
    var result = ""
    multi_match(n) {
        if (0) {
            result += "zero "
        }
        if (1) {
            result += "one "
        }
    }
    return result
}

// ===========================
// static_match — fallthrough on type mismatch
// ===========================

[sideeffects]
def static_match_int_or_float(what) {
    static_match(what) {
        if (match_type(type<int>, $v(v))) {
            return "int:{v}"
        }
        if (match_type(type<float>, $v(v))) {
            return "float:{v}"
        }
        if (_) {
            return "other"
        }
    }
}

// ===========================
// match on tuple with capture
// ===========================

[sideeffects]
def tuple_capture(t : tuple<int; string>) {
    match (t) {
        if (($v(n), "hello")) {
            return n * 10
        }
        if (($v(n), _)) {
            return n
        }
    }
}

// ===========================
// deep nesting: struct -> struct -> value
// ===========================

struct Outer {
    mid : Nested
    id : int
}

[sideeffects]
def deep_nested_match(o : Outer) {
    match (o) {
        if (Outer(mid = Nested(inner = Point(x = 0, y = 0), tag = "root"), id = 1)) {
            return "root-1"
        }
        if (Outer(mid = Nested(inner = Point(x = $v(x))))) {
            return "x={x}"
        }
        if (_) {
            return "?"
        }
    }
}

// ===========================
// match_expr in struct: captured var used in expression
// (tests the apply_template fix for root ExprVar replacement)
// ===========================

[sideeffects]
def match_expr_struct_capture(p : Point) {
    match (p) {
        if (Point(x = $v(x), y = match_expr(x))) {
            return "diagonal x={x}"
        }
        if (Point(x = $v(x), y = match_expr(x * 2))) {
            return "double x={x}"
        }
        if (_) {
            return "other"
        }
    }
}

// ===========================
// match_expr in tuple: captured var in expression
// ===========================

[sideeffects]
def match_expr_tuple_capture(t : tuple<int; int>) {
    match (t) {
        if (($v(a), match_expr(a))) {
            return "equal={a}"
        }
        if (_) {
            return "different"
        }
    }
}

// ===========================
// Null pointer matching
// ===========================

[sideeffects]
def null_ptr_match(p : Point?) {
    match (p) {
        if (null) {
            return "null"
        }
        if (Point(x = 0, y = 0)) {
            return "origin"
        }
        if (Point(x = $v(x))) {
            return "x={x}"
        }
    }
    return "unreachable"
}

// ===========================
// Guard on nested struct pattern
// ===========================

[sideeffects]
def guard_nested(n : Nested) {
    match (n) {
        if (Nested(inner = Point(x = $v(x), y = $v(y))) && x == y) {
            return "diagonal {x}"
        }
        if (Nested(inner = Point(x = $v(x))) && x > 10) {
            return "big x={x}"
        }
        if (_) {
            return "other"
        }
    }
}

// ===========================
// OR pattern with variant as-syntax
// ===========================

// OR pattern on variant — no capture, just type check
// NOTE: match_or requires matching access expressions for captured vars,
// so using wildcards instead of captures when patterns access different fields
[sideeffects]
def or_variant_match(v : ABC) {
    match (v) {
        if ((_ as a) || (_ as c)) {
            return "a or c"
        }
        if (_) {
            return "b"
        }
    }
}

// ===========================
// enum match via match macro
// ===========================

enum Color {
    Red
    Green
    Blue
}

[sideeffects]
def enum_match(c : Color) {
    match (c) {
        if (Color.Red) {
            return "red"
        }
        if (Color.Blue) {
            return "blue"
        }
        if (_) {
            return "other"
        }
    }
}

// ===========================
// int literal match via match macro
// ===========================

[sideeffects]
def int_literal_match(n : int) {
    match (n) {
        if (0) {
            return "zero"
        }
        if (1) {
            return "one"
        }
        if (42) {
            return "answer"
        }
        if (_) {
            return "other"
        }
    }
}

// ===========================
// float literal match
// ===========================

[sideeffects]
def float_match(f : float) {
    match (f) {
        if (0.0) {
            return "zero"
        }
        if (1.0) {
            return "one"
        }
        if (_) {
            return "other"
        }
    }
}

// ===========================
// TESTS
// ===========================

[test]
def test_match_edge_cases(t : T?) {

    t |> run("nested struct match") @(t : T?) {
        t |> equal("origin labeled", nested_match(Nested(inner = Point(x = 0, y = 0), tag = "origin")))
        t |> equal("(3,4)", nested_match(Nested(inner = Point(x = 3, y = 4), tag = "test")))
    }

    t |> run("empty struct match") @(t : T?) {
        t |> equal("empty", empty_struct_match(Empty()))
    }

    t |> run("string match") @(t : T?) {
        t |> equal(1, string_match("hello"))
        t |> equal(2, string_match("world"))
        t |> equal(0, string_match("xyz"))
        t |> equal(0, string_match(""))
    }

    t |> run("bool match") @(t : T?) {
        t |> equal("yes", bool_match(true))
        t |> equal("no", bool_match(false))
    }

    t |> run("three-way variant match") @(t : T?) {
        t |> equal("a=42", three_variant_match(ABC(a = 42)))
        t |> equal("b=3.14", three_variant_match(ABC(b = 3.14)))
        t |> equal("c=hi", three_variant_match(ABC(c = "hi")))
    }

    t |> run("variant match with guard") @(t : T?) {
        t |> equal("positive int", variant_guard_match(IntOrStr(i = 5)))
        t |> equal("non-positive int", variant_guard_match(IntOrStr(i = -3)))
        t |> equal("non-positive int", variant_guard_match(IntOrStr(i = 0)))
        t |> equal("string: hello", variant_guard_match(IntOrStr(s = "hello")))
    }

    t |> run("tuple with spread") @(t : T?) {
        t |> equal("starts with 1", tuple4_match((1, 2, 3, 4)))
        t |> equal("ends with 9", tuple4_match((2, 3, 4, 9)))
        t |> equal("bookends 0", tuple4_match((0, 3, 4, 0)))
        t |> equal("other", tuple4_match((2, 3, 4, 5)))
    }

    t |> run("match_expr with captured variable") @(t : T?) {
        t |> equal("diagonal", match_expr_captured(Point(x = 5, y = 5)))
        t |> equal("double", match_expr_captured(Point(x = 3, y = 6)))
        t |> equal("other", match_expr_captured(Point(x = 1, y = 7)))
    }

    t |> run("dynamic array tail literal") @(t : T?) {
        t |> equal("ends with 42", dynamic_array_tail_literal(array<int>(1, 2, 42)))
        t |> equal("starts with 1", dynamic_array_tail_literal(array<int>(1, 2, 3)))
        t |> equal("other", dynamic_array_tail_literal(array<int>(2, 3, 4)))
        t |> equal("other", dynamic_array_tail_literal(array<int>()))
    }

    t |> run("OR pattern simple") @(t : T?) {
        t |> equal("low", or_pattern_simple(1))
        t |> equal("low", or_pattern_simple(2))
        t |> equal("low", or_pattern_simple(3))
        t |> equal("round", or_pattern_simple(10))
        t |> equal("round", or_pattern_simple(20))
        t |> equal("other", or_pattern_simple(5))
    }

    t |> run("struct all wildcard") @(t : T?) {
        t |> equal("origin", struct_all_wildcard(Point(x = 0, y = 0)))
        t |> equal("any point", struct_all_wildcard(Point(x = 1, y = 2)))
    }

    t |> run("struct field order") @(t : T?) {
        t |> equal(5, struct_field_order(Point(x = 5, y = 0)))
        t |> equal(-1, struct_field_order(Point(x = 5, y = 1)))
    }

    t |> run("variant field syntax") @(t : T?) {
        t |> equal("the answer", variant_field_syntax(IntOrStr(i = 42)))
        t |> equal("int 7", variant_field_syntax(IntOrStr(i = 7)))
        t |> equal("str hello", variant_field_syntax(IntOrStr(s = "hello")))
    }

    t |> run("static array exact") @(t : T?) {
        t |> equal("123", static_array_exact(fixed_array<int>(1, 2, 3)))
        t |> equal("starts 0", static_array_exact(fixed_array<int>(0, 5, 5)))
        t |> equal("other", static_array_exact(fixed_array<int>(5, 5, 5)))
    }

    t |> run("nested variant in struct") @(t : T?) {
        t |> equal("x int 10", nested_variant_in_struct(HasVariant(name = "x", value = IntOrStr(i = 10))))
        t |> equal("string world", nested_variant_in_struct(HasVariant(name = "y", value = IntOrStr(s = "world"))))
        t |> equal("other", nested_variant_in_struct(HasVariant(name = "z", value = IntOrStr(i = 1))))
    }

    t |> run("multi_match categories") @(t : T?) {
        t |> equal("zero ", multi_match_categories(0))
        t |> equal("one ", multi_match_categories(1))
        t |> equal("", multi_match_categories(5))
    }

    t |> run("static_match int or float") @(t : T?) {
        t |> equal("int:42", static_match_int_or_float(42))
        t |> equal("float:3.14", static_match_int_or_float(3.14))
        t |> equal("other", static_match_int_or_float("hello"))
    }

    t |> run("tuple capture") @(t : T?) {
        t |> equal(50, tuple_capture((5, "hello")))
        t |> equal(3, tuple_capture((3, "bye")))
    }

    t |> run("deep nested match") @(t : T?) {
        t |> equal("root-1", deep_nested_match(Outer(mid = Nested(inner = Point(x = 0, y = 0), tag = "root"), id = 1)))
        t |> equal("x=7", deep_nested_match(Outer(mid = Nested(inner = Point(x = 7, y = 3), tag = "foo"), id = 2)))
    }

    t |> run("match_expr struct capture") @(t : T?) {
        t |> equal("diagonal x=5", match_expr_struct_capture(Point(x = 5, y = 5)))
        t |> equal("double x=3", match_expr_struct_capture(Point(x = 3, y = 6)))
        t |> equal("other", match_expr_struct_capture(Point(x = 1, y = 7)))
    }

    t |> run("match_expr tuple capture") @(t : T?) {
        t |> equal("equal=5", match_expr_tuple_capture((5, 5)))
        t |> equal("different", match_expr_tuple_capture((1, 2)))
    }

    t |> run("null pointer match") @(t : T?) {
        t |> equal("null", null_ptr_match(null))
        unsafe {
            var inscope p = new Point(x = 0, y = 0)
            t |> equal("origin", null_ptr_match(p))
            var inscope q = new Point(x = 7, y = 3)
            t |> equal("x=7", null_ptr_match(q))
        }
    }

    t |> run("guard on nested struct") @(t : T?) {
        t |> equal("diagonal 3", guard_nested(Nested(inner = Point(x = 3, y = 3), tag = "")))
        t |> equal("big x=20", guard_nested(Nested(inner = Point(x = 20, y = 1), tag = "")))
        t |> equal("other", guard_nested(Nested(inner = Point(x = 5, y = 6), tag = "")))
    }

    t |> run("OR variant match") @(t : T?) {
        t |> equal("a or c", or_variant_match(ABC(a = 1)))
        t |> equal("a or c", or_variant_match(ABC(c = "hi")))
        t |> equal("b", or_variant_match(ABC(b = 1.0)))
    }

    t |> run("enum match") @(t : T?) {
        t |> equal("red", enum_match(Color.Red))
        t |> equal("blue", enum_match(Color.Blue))
        t |> equal("other", enum_match(Color.Green))
    }

    t |> run("int literal match") @(t : T?) {
        t |> equal("zero", int_literal_match(0))
        t |> equal("one", int_literal_match(1))
        t |> equal("answer", int_literal_match(42))
        t |> equal("other", int_literal_match(99))
    }

    t |> run("float match") @(t : T?) {
        t |> equal("zero", float_match(0.0))
        t |> equal("one", float_match(1.0))
        t |> equal("other", float_match(3.14))
    }
}
