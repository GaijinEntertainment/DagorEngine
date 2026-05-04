// Tutorial 24: Pattern Matching
//
// This tutorial covers:
//   - match macro for structured matching
//   - Matching integers, enums, structs
//   - Variable binding with $v(name)
//   - Wildcards (_) and guards (&&)
//   - OR patterns (||)
//   - Tuple and array matching
//   - Variant matching
//   - Null/pointer matching
//   - match_expr for computed patterns
//   - Bool matching
//   - multi_match (all arms)
//   - static_match (generic-safe)
//
// Run: daslang.exe tutorials/language/24_pattern_matching.das

options gen2

require daslib/match

// === Enums and structs for examples ===

enum Color {
    red
    green
    blue
    black
}

struct Point {
    x : int
    y : int
}

struct Shape {
    kind : string
    size : int
}

variant Value {
    i : int
    f : float
    s : string
}

// === Basic integer matching ===

def describe_number(n : int) : string {
    match (n) {
        if (0) {
            return "zero"
        }
        if (1) {
            return "one"
        }
        if (2) {
            return "two"
        }
        if (_) {                // wildcard — matches anything
            return "other"
        }
    }
    return "unreachable"
}

// === Enum matching ===

def color_code(c : Color) : int {
    match (c) {
        if (Color.red) {        // gen2 dot syntax for enum values
            return 1
        }
        if (Color.green) {
            return 2
        }
        if (Color.blue) {
            return 3
        }
        if (_) {
            return 0
        }
    }
    return -1
}

// === Struct matching + variable binding ===

def describe_point(p : Point) : string {
    match (p) {
        if (Point(x = 0, y = 0)) {                 // exact values
            return "origin"
        }
        if (Point(x = 0, y = $v(y))) {             // bind y, x must be 0
            return "on y-axis at {y}"
        }
        if (Point(x = $v(x), y = 0)) {             // bind x, y must be 0
            return "on x-axis at {x}"
        }
        if (Point(x = $v(x), y = $v(y))) {         // bind both
            return "at ({x}, {y})"
        }
    }
    return "unreachable"
}

// === Guards ===

def classify_shape(s : Shape) : string {
    match (s) {
        if (Shape(kind = "circle", size = $v(sz)) && sz > 100) {
            return "large circle ({sz})"
        }
        if (Shape(kind = "circle", size = $v(sz))) {
            return "small circle ({sz})"
        }
        if (Shape(kind = $v(k), size = $v(sz))) {
            return "{k} of size {sz}"
        }
    }
    return "unknown"
}

// === OR patterns ===

def is_primary(c : Color) : bool {
    match (c) {
        if (Color.red || Color.green || Color.blue) {
            return true
        }
        if (_) {
            return false
        }
    }
    return false
}

// === Tuple matching ===

def classify_pair(t : tuple<int; string>) : string {
    match (t) {
        if ((0, "zero")) {                  // exact match
            return "zero-zero"
        }
        if ((1, _)) {                       // _ matches any one element
            return "starts with one"
        }
        if (($v(n), "hello")) {             // bind first, match second
            return "hello #{n}"
        }
        if (_) {
            return "other"
        }
    }
    return "unreachable"
}

// === Variant matching ===

def describe_value(v : Value) : string {
    match (v) {
        if ($v(i) as i) {                   // match variant alternative by name
            return "integer: {i}"
        }
        if ($v(f) as f) {
            return "float: {f}"
        }
        if ($v(s) as s) {
            return "string: {s}"
        }
    }
    return "unknown"
}

// === Null / pointer matching ===

def describe_point_ptr(p : Point?) : string {
    match (p) {
        if (null) {                         // null check
            return "null"
        }
        if (Point(x = 0, y = 0)) {         // auto-dereferences
            return "origin"
        }
        if (Point(x = $v(x), y = $v(y))) {
            return "({x}, {y})"
        }
    }
    return "unreachable"
}

// === multi_match — all matching arms execute ===

def tag_number(n : int) : string {
    var tags = "{n}:"
    multi_match(n) {
        if (0) {
            tags += " zero"
        }
        if (1) {
            tags += " one"
        }
        if ($v(a) && a > 0) {
            tags += " positive"
        }
        if ($v(a) && a < 0) {
            tags += " negative"
        }
        if ($v(a) && (a % 2 == 0) && (a != 0)) {
            tags += " even"
        }
    }
    return tags
}

// === static_match — safe in generics ===
// Arms with type mismatches are silently dropped,
// so this function works regardless of argument type.

def static_type_name(what) : string {
    static_match(what) {
        if (match_type(type<int>, $v(v))) {
            return "int({v})"
        }
        if (match_type(type<float>, $v(v))) {
            return "float({v})"
        }
        if (match_type(type<string>, $v(v))) {
            return "string({v})"
        }
        if (_) {
            return "unknown"
        }
    }
    return "unreachable"
}

// === Array matching ===
// Static arrays match element-by-element.
// Dynamic arrays match head elements; tail is ignored.

def classify_static_arr(a : int[3]) : string {
    match (a) {
        if (fixed_array<int>(0, 0, 0)) {        // all zeros
            return "zeros"
        }
        if (fixed_array<int>(1, $v(b), $v(c))) { // first is 1, bind rest
            return "1,{b},{c}"
        }
        if (_) {
            return "other"
        }
    }
    return "unreachable"
}

def classify_dynamic_arr(a : array<int>) : string {
    match (a) {
        if (array<int>(0, 0, ...)) {             // starts with 0, 0
            return "starts with 0,0"
        }
        if (array<int>($v(x), $v(y), ...)) {     // bind first two (any length)
            return "starts with {x},{y}"
        }
        if (_) {
            return "other"
        }
    }
    return "unreachable"
}

// === match_expr — computed patterns ===
// match_expr(expression) evaluates at runtime instead of matching literally.
// Useful when the expected value depends on captured variables.

def is_consecutive_triple(t : tuple<int; int; int>) : bool {
    match (t) {
        if (($v(a), match_expr(a + 1), match_expr(a + 2))) {
            return true
        }
        if (_) {
            return false
        }
    }
    return false
}

// === Bool matching ===

def describe_bool(b : bool) : string {
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

// === Main ===

[export]
def main {

    // Integer matching
    print("integer matching:\n")
    print("  0 -> {describe_number(0)}\n")
    print("  1 -> {describe_number(1)}\n")
    print("  42 -> {describe_number(42)}\n")

    // Enum matching
    print("enum matching:\n")
    print("  red -> {color_code(Color.red)}\n")
    print("  green -> {color_code(Color.green)}\n")
    print("  black -> {color_code(Color.black)}\n")

    // Struct matching
    print("struct matching:\n")
    print("  (0,0) -> {describe_point(Point(x=0, y=0))}\n")
    print("  (0,5) -> {describe_point(Point(x=0, y=5))}\n")
    print("  (3,0) -> {describe_point(Point(x=3, y=0))}\n")
    print("  (2,7) -> {describe_point(Point(x=2, y=7))}\n")

    // Guards
    print("guards:\n")
    var s1 = classify_shape(Shape(kind = "circle", size = 200))
    print("  circle 200 -> {s1}\n")
    var s2 = classify_shape(Shape(kind = "circle", size = 50))
    print("  circle 50 -> {s2}\n")
    var s3 = classify_shape(Shape(kind = "square", size = 30))
    print("  square 30 -> {s3}\n")

    // OR patterns
    print("OR patterns:\n")
    print("  red primary? {is_primary(Color.red)}\n")
    print("  black primary? {is_primary(Color.black)}\n")

    // Tuple matching
    print("tuple matching:\n")
    var t0 : tuple<int; string>
    t0 = (0, "zero")
    print("  (0,zero) -> {classify_pair(t0)}\n")
    t0 = (1, "xyz")
    print("  (1,xyz) -> {classify_pair(t0)}\n")
    t0 = (5, "hello")
    print("  (5,hello) -> {classify_pair(t0)}\n")
    t0 = (9, "bye")
    print("  (9,bye) -> {classify_pair(t0)}\n")

    // Variant matching
    print("variant matching:\n")
    var v : Value
    v = Value(i = 42)
    print("  int -> {describe_value(v)}\n")
    v = Value(f = 3.14)
    print("  float -> {describe_value(v)}\n")

    // Null matching
    print("null matching:\n")
    var p = new Point(x = 10, y = 20)
    print("  point -> {describe_point_ptr(p)}\n")
    var np : Point? = null
    print("  null -> {describe_point_ptr(np)}\n")
    unsafe { delete p; }

    // multi_match
    print("multi_match:\n")
    print("  {tag_number(0)}\n")
    print("  {tag_number(1)}\n")
    print("  {tag_number(4)}\n")
    print("  {tag_number(-3)}\n")

    // static_match
    print("static_match:\n")
    print("  {static_type_name(42)}\n")
    print("  {static_type_name(3.14)}\n")
    print("  {static_type_name("hi")}\n")

    // Array matching
    print("array matching:\n")
    var sa : int[3]
    sa = fixed_array<int>(0, 0, 0)
    print("  [0,0,0] -> {classify_static_arr(sa)}\n")
    sa = fixed_array<int>(1, 5, 9)
    print("  [1,5,9] -> {classify_static_arr(sa)}\n")
    sa = fixed_array<int>(7, 7, 7)
    print("  [7,7,7] -> {classify_static_arr(sa)}\n")
    var da : array<int>
    da |> push(0); da |> push(0); da |> push(3); da |> push(4)
    print("  [0,0,3,4] -> {classify_dynamic_arr(da)}\n")
    var da2 : array<int>
    da2 |> push(5); da2 |> push(6)
    print("  [5,6] -> {classify_dynamic_arr(da2)}\n")

    // match_expr
    print("match_expr:\n")
    print("  (1,2,3) consecutive? {is_consecutive_triple((1, 2, 3))}\n")
    print("  (5,6,7) consecutive? {is_consecutive_triple((5, 6, 7))}\n")
    print("  (1,3,5) consecutive? {is_consecutive_triple((1, 3, 5))}\n")

    // Bool matching
    print("bool matching:\n")
    print("  true -> {describe_bool(true)}\n")
    print("  false -> {describe_bool(false)}\n")

    // === Summary ===
    // - require daslib/match
    // - match(expr) { if (pattern) { body } }
    // - match works on functions that return values
    // - $v(name) — bind matched value to a variable
    // - _ — wildcard, matches anything
    // - Guards: if (pattern && condition) { ... }
    // - OR: if (pattern1 || pattern2) { ... }
    // - Structs: if (Type(field = value_or_binding)) { ... }
    // - Enums: if (Enum.value) { ... }
    // - Variants: if ($v(x) as alternative_name) { ... }
    // - Tuples: if ((val, _, $v(x))) { ... }
    // - Arrays: if (fixed_array<int>(1, $v(b), $v(c))) { ... }
    // - match_expr: if (($v(a), match_expr(a+1))) — computed patterns
    // - Bool: if (true) / if (false)
    // - Null: if (null) { ... } for pointer types
    // - multi_match — all matching arms execute
    // - static_match — silently drops type-mismatched arms (generics)

    print("done\n")
}

// output:
// integer matching:
//   0 -> zero
//   1 -> one
//   42 -> other
// enum matching:
//   red -> 1
//   green -> 2
//   black -> 0
// struct matching:
//   (0,0) -> origin
//   (0,5) -> on y-axis at 5
//   (3,0) -> on x-axis at 3
//   (2,7) -> at (2, 7)
// guards:
//   circle 200 -> large circle (200)
//   circle 50 -> small circle (50)
//   square 30 -> square of size 30
// OR patterns:
//   red primary? true
//   black primary? false
// tuple matching:
//   (0,zero) -> zero-zero
//   (1,xyz) -> starts with one
//   (5,hello) -> hello #5
//   (9,bye) -> other
// variant matching:
//   int -> integer: 42
//   float -> float: 3.14
// null matching:
//   point -> (10, 20)
//   null -> null
// multi_match:
//   0: zero
//   1: one positive
//   4: positive even
//   -3: negative
// static_match:
//   int(42)
//   float(3.14)
//   string(hi)
// array matching:
//   [0,0,0] -> zeros
//   [1,5,9] -> 1,5,9
//   [7,7,7] -> other
//   [0,0,3,4] -> starts with 0,0
//   [5,6] -> starts with 5,6
// match_expr:
//   (1,2,3) consecutive? true
//   (5,6,7) consecutive? true
//   (1,3,5) consecutive? false
// bool matching:
//   true -> yes
//   false -> no
// done
