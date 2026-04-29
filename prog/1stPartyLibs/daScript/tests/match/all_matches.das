options gen2
require dastest/testing_boost public
require daslib/match
require daslib/ast_boost
require daslib/defer

enum Color {
    black
    red
    green
    blue
}

variant IF {
    i : int
    f : float
}

struct Foo {
    a : int
    v : IF
}

struct Bar {
    a : int
    b : float
}

struct AB {
    a, b : int
}

[sideeffects]
def enum_match(color : Color) {
    match (color) {
        if (Color.black) {
            return 0
        }
        if (Color.red) {
            return 1
        }
        if (_) {
            return -1
        }
    }
}

[sideeffects]
def enum_static_match(color, blah) {
    static_match(color) {
        if (Color.black) {
            return 0
        }
        if (match_expr(blah)) {
            return 1
        }
        if (_) {
            return -1
        }
    }
}

[sideeffects]
def static_match_by_type(what) {
    static_match(what) {
        if (match_type(type<int>, $v(expr))) {
            return expr
        }
        if (_) {
            return -1
        }
    }
}

[sideeffects]
def struct_match(f : Foo) {
    match (f) {
        if (Foo(a = 13)) {
            return 0
        }
        if (Foo(a = $v(anyA))) {
            return anyA
        }
    }
}

[sideeffects]
def struct_match(b : Bar) {
    match (b) {
        if (Bar(a = 13)) {
            return 0
        }
        if (Bar(a = $v(anyA), b = 13.0)) {
            return anyA
        }
        if (_) {
            return -1
        }
    }
}

[sideeffects]
def guards_match(ab : AB) {
    match (ab) {
        if (AB(a = $v(a), b = $v(b)) && (b > a)) {
            return "{b} > {a}"
        }
        if (AB(a = $v(a), b = $v(b))) {
            return "{b} <= {a}"
        }
    }
}

[sideeffects]
def variant_as_match(v : IF) {
    match (v) {
        if ($v(i) as i) {
            return "int"
        }
        if ($v(f) as f) {
            return "float"
        }
        if (_) {
            return "anything"
        }
    }
}

[sideeffects]
def nested_struct_match(v : Foo) {
    match (v) {
        if (Foo(a = 1, v = $v(i) as i)) {
            return 1
        }
        if (Foo(v = $v(f) as f)) {
            return 2
        }
        if (_) {
            return 0
        }
    }
}

[sideeffects]
def variant_match(v : IF) {
    match (v) {
        if (IF(i = $v(i))) {
            return 1
        }
        if (IF(f = $v(f))) {
            return 2
        }
        if (_) {
            return 0
        }
    }
}

[sideeffects]
def static_array_match(A : int[3]) {
    match (A) {
        if (fixed_array($v(a), $v(b), $v(c)) && (a + b + c) == 6) { // total of 3 elements, sum is 6
            return 1
        }
        if (fixed_array<int>(0, ...)) { // starts with 0
            return 0
        }
        if (fixed_array<int>(..., 13)) { // ends with 13
            return 2
        }
        if (fixed_array<int>(12, ..., 12)) { // starts and ends with 12
            return 3
        }
        if (_) {
            return -1
        }
    }
}

[sideeffects]
def dynamic_array_match(A : array<int>) {
    match (A) {
        if ([ $v(a), $v(b), $v(c)] && (a + b + c) == 6) {// total of 3 elements, sum is 6
            return 1
        }
        if (array<int>(0, 0, 0, ...)) { // first 3 are 0
            return 0
        }
        if (array<int>(..., 1, 2)) { // ends with 1,2
            return 2
        }
        if (array<int>(0, 1, ..., 2, 3)) { // starts with 0,1, ends with 2,3
            return 3
        }
        if (_) {
            return -1
        }
    }
}


[sideeffects]
def ascending_array_match(A : int[3]) {
    match (A) {
        if (fixed_array<int>($v(x), match_expr(x + 1), match_expr(x + 2))) {
            return true
        }
        if (_) {
            return false
        }
    }
}

[sideeffects]
def tuple_match(A : tuple<int; float; string>) {
    match (A) {
        if ((1, _, "3")) {
            return 1
        }
        if ((13, ...)) { // starts with 13
            return 2
        }
        if ((..., "13")) { // ends with "13"
            return 3
        }
        if ((2, ..., "2")) { // starts with 2, ends with "2"
            return 4
        }
        if (_) {
            return 0
        }
    }
}

[sideeffects]
def or_match(i : int) {
    match (i) {
        if (1 || 2) {
            return true
        }
        if (_) {
            return false
        }
    }
}

[sideeffects]
def or_match_v(B : Bar) {
    match (B) {
        if (Bar(a = 1, b = $v(b)) || Bar(a = 2, b = $v(b))) {
            return b
        }
        if (_) {
            return 0.0
        }
    }
}

[sideeffects]
def struct_ptr_match(f : Foo?) {
    match (f) {
        if (null) {
            return -1
        }
        if (Foo(a = 13)) {
            return 0
        }
        if (Foo(a = $v(anyA))) {
            return anyA
        }
        if (_) {
            return -2
        }
    }
}

[sideeffects]
def match_handle_ptr(T : TypeDeclPtr) {
    match (T) {
        if (null) {
            return "null"
        }
        if (TypeDecl(baseType = Type.tPointer)) {
            return "pointer"
        }
        if (_) {
            return "anything"
        }
    }
}

[sideeffects]
def match_ast_expr_ptr(E : ExpressionPtr) {
    match (E) {
        if (null) {
            return "null"
        }
        if (ExprConstInt(value = $v(v))) {
            return "int {v}"
        }
        if (_) {
            return "anything"
        }
    }
}

struct Cmd {
    rtti = "Cmd"
}

[match_as_is]
struct CmdMove : Cmd {
    override rtti = "CmdMove"
    x : float
    y : float
}

def operator is CmdMove(cmd : Cmd) {
    return cmd.rtti == "CmdMove"
}

def operator is CmdMove(anything) {
    return false
}

def operator as CmdMove(cmd : Cmd ==const) : CmdMove const& {
    assert(cmd.rtti == "CmdMove")
    unsafe {
        return reinterpret<CmdMove const&> cmd
    }
}

def operator as CmdMove(var cmd : Cmd ==const) : CmdMove& {
    assert(cmd.rtti == "CmdMove")
    unsafe {
        return reinterpret<CmdMove&> cmd
    }
}

def operator as CmdMove(anything) {
    panic("Cannot cast to CmdMove")
    return CmdMove()
}

[sideeffects]
def matching_as_and_is(cmd : Cmd) {
    match (cmd) {
        if (CmdMove(x = $v(x), y = $v(y))) {
            return x + y
        }
        if (_) {
            return 0.
        }
    }
}

[sideeffects]
def matching_type_as_is(cmd : Cmd) {
    match (cmd) {
        if (match_type(type<CmdMove>, $v(cmdm))) {
            return cmdm.x + cmdm.y
        }
        if (_) {
            return 0.
        }
    }
}

[match_copy, safe_when_uninitialized]
struct CmdLocate : Cmd {
    override rtti = "CmdLocate"
    x : float
    y : float
    z : float
}

def match_copy(var cmdm : CmdLocate; cmd : Cmd) {
    if (cmd.rtti != "CmdLocate") {
        return false
    }
    unsafe {
        cmdm = reinterpret<CmdLocate const&> cmd
    }
    return true
}

[sideeffects]
def matching_copy(cmd : Cmd) {
    match (cmd) {
        if (CmdLocate(x = $v(x), y = $v(y), z = $v(z))) {
            return x + y + z
        }
        if (_) {
            return 0.
        }
    }
}

[sideeffects]
def matching_type_copy(cmd : Cmd) {
    match (cmd) {
        if (match_type(type<CmdLocate>, $v(cmdl))) {
            return cmdl.x + cmdl.y + cmdl.z
        }
        if (_) {
            return 0.
        }
    }
}

[sideeffects]
def multi_match_test(a : int) {
    var text = "{a}"
    multi_match(a) {
        if (0) {
            text += " zero"
        }
        if (1) {
            text += " one"
        }
        if (2) {
            text += " two"
        }
        if ($v(a) && (a % 2 == 0) && (a != 0)) {
            text += " even"
        }
        if ($v(a) && (a % 2 == 1)) {
            text += " odd"
        }
    }
    return text
}

[test]
def test_all_match(t : T?) {
    t |> run("enum match") @@(t : T?) {
        t |> equal(0, enum_match(Color.black))
        t |> equal(1, enum_match(Color.red))
        t |> equal(-1, enum_match(Color.green))
        t |> equal(-1, enum_match(Color.blue))
    }
    t |> run("enum static match") @@(t : T?) {
        t |> equal(0, enum_static_match(Color.black, null))
        t |> equal(1, enum_static_match(1234, 1234))
        t |> equal(-1, enum_static_match(5678, 1234))
        t |> equal(-1, enum_static_match("a", 1234))
    }
    t |> run("enum static match by type") @@(t : T?) {
        t |> equal(13, static_match_by_type(13))
        t |> equal(-1, static_match_by_type(13.))
    }
    t |> run("struct match") @@(t : T?) {
        t |> equal(0, struct_match(Foo(a = 13)))
        t |> equal(23, struct_match(Foo(a = 23)))
        t |> equal(0, struct_match(Bar(a = 13)))
        t |> equal(23, struct_match(Bar(a = 23, b = 13.0)))
        t |> equal(-1, struct_match(Bar(a = 23, b = 14.0)))
    }
    t |> run("guards match") @@(t : T?) {
        t |> equal("2 > 1", guards_match(AB(a = 1, b = 2)))
        t |> equal("1 <= 2", guards_match(AB(a = 2, b = 1)))
    }
    t |> run("variant match") @@(t : T?) {
        t |> equal(1, variant_match(IF(i = 1)))
        t |> equal(2, variant_match(IF(f = 1.0)))
    }
    t |> run("variant as match") @@(t : T?) {
        t |> equal("int", variant_as_match(IF(i = 1)))
        t |> equal("float", variant_as_match(IF(f = 1.0)))
    }
    t |> run("nested struct + variant as match") @@(t : T?) {
        t |> equal(1, nested_struct_match(Foo(a = 1, v = IF(i = 1))))
        t |> equal(2, nested_struct_match(Foo(v = IF(f = 1.))))
        t |> equal(0, nested_struct_match(Foo(a = 13, v = IF(i = 1))))
    }
    t |> run("static array match") @@(t : T?) {
        t |> equal(1, static_array_match(fixed_array<int>(1, 2, 3)))
        t |> equal(0, static_array_match(fixed_array<int>(0, 1, 2)))   // strts with 0
        t |> equal(0, static_array_match(fixed_array<int>(0, 0, 0)))   // strts with 0
        t |> equal(-1, static_array_match(fixed_array<int>(3, 3, 3)))
        t |> equal(2, static_array_match(fixed_array<int>(3, 3, 13))) // ends with 12
        t |> equal(3, static_array_match(fixed_array<int>(12, 3, 12))) // starts and ends with 12
    }
    t |> run("dynamic array match") @@(t : T?) {
        t |> equal(1, dynamic_array_match(array<int>(1, 2, 3)))          // summ is 6, 3 total
        t |> equal(0, dynamic_array_match(array<int>(0, 0, 0)))          // first 3 are 0
        t |> equal(0, dynamic_array_match(array<int>(0, 0, 0, 1, 2, 3))) // first 3 are 0
        t |> equal(-1, dynamic_array_match(array<int>(3, 3, 3)))
        t |> equal(-1, dynamic_array_match(array<int>(1, 2, 3, 4)))
        t |> equal(-1, dynamic_array_match(array<int>(2, 2)))
        t |> equal(2, dynamic_array_match(array<int>(1, 2)))     // ends with 1,2
        t |> equal(2, dynamic_array_match(array<int>(2, 1, 2)))  // ends with 1,2
        t |> equal(3, dynamic_array_match(array<int>(0, 1, 2, 3))) // starts with 0,1, ends with 2,3
        t |> equal(3, dynamic_array_match(array<int>(0, 1, 12, 11, 10, 2, 3))) // starts with 0,1, ends with 2,3
    }
    t |> run("match_expr") @@(t : T?) {
        t |> equal(true, ascending_array_match(fixed_array<int>(1, 2, 3)))
        t |> equal(true, ascending_array_match(fixed_array<int>(0, 1, 2)))
        t |> equal(false, ascending_array_match(fixed_array<int>(1, 1, 2)))
    }
    t |> run("match tuple") @@(t : T?) {
        t |> equal(1, tuple_match((1, 2.0, "3")))
        t |> equal(0, tuple_match((1, 2.0, "4")))
        t |> equal(2, tuple_match((13, 2.0, "4")))
        t |> equal(3, tuple_match((11, 2.0, "13")))
        t |> equal(4, tuple_match((2, 2.0, "2")))
    }
    t |> run("or match") @@(t : T?) {
        t |> equal(true, or_match(1))
        t |> equal(true, or_match(2))
        t |> equal(false, or_match(3))
    }
    t |> run("or match with variable") @@(t : T?) {
        t |> equal(1., or_match_v(Bar(a = 1, b = 1.)))
        t |> equal(1., or_match_v(Bar(a = 2, b = 1.)))
        t |> equal(0., or_match_v(Bar(a = 3, b = 1.)))
    }
    t |> run("struct ptr match") @@(t : T?) {
        t |> equal(-1, struct_ptr_match(null))
        t |> equal(0, struct_ptr_match(new Foo(a = 13)))
        t |> equal(23, struct_ptr_match(new Foo(a = 23)))
    }
    t |> run("match handle ptr") @@(t : T?) {
        t |> equal("null", match_handle_ptr(default<TypeDeclPtr>))
        var inscope tp <- new TypeDecl(baseType = Type.tPointer)
        t |> equal("pointer", match_handle_ptr(tp))
        var inscope tnp <- new TypeDecl(baseType = Type.tInt)
        t |> equal("anything", match_handle_ptr(tnp))
    }
    t |> run("match ast expression ptr") @@(t : T?) {
        t |> equal("null", match_ast_expr_ptr(default<ExpressionPtr>))
        var inscope ep <- new ExprConstInt(value = 13)
        t |> equal("int 13", match_ast_expr_ptr(ep))
        var inscope fp <- new ExprConstFloat(value = 13.0)
        t |> equal("anything", match_ast_expr_ptr(fp))
    }
    t |> run("match as is") @@(t : T?) {
        t |> equal(0., matching_as_and_is(Cmd()))
        t |> equal(3., matching_as_and_is(CmdMove(x = 1., y = 2.)))
    }
    t |> run("match type is") @@(t : T?) {
        t |> equal(0., matching_type_as_is(Cmd()))
        t |> equal(3., matching_type_as_is(CmdMove(x = 1., y = 2.)))
    }
    t |> run("match copy") @@(t : T?) {
        t |> equal(0., matching_copy(Cmd()))
        t |> equal(6., matching_copy(CmdLocate(x = 1., y = 2., z = 3.)))
    }
    t |> run("match type copy") @@(t : T?) {
        t |> equal(0., matching_type_copy(Cmd()))
        t |> equal(6., matching_type_copy(CmdLocate(x = 1., y = 2., z = 3.)))
    }
    t |> run("multi-match test") @@(t : T?) {
        t |> equal("0 zero", multi_match_test(0))
        t |> equal("1 one odd", multi_match_test(1))
        t |> equal("2 two even", multi_match_test(2))
        t |> equal("3 odd", multi_match_test(3))
    }
}


