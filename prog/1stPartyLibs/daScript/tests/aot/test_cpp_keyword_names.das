options gen2
require dastest/testing_boost public

// Coverage: AOT codegen for daslang names that collide with C++ keywords.
// Includes function names (the 2026-05-14 baseline) plus the broader sites
// (struct/class/enum/field/variable) opened by the lint-drop in the same PR.

struct Foo {
    a : int
}

def float(h : Foo) : int {
    return h.a + 1
}

def int(h : Foo) : float {
    return float(h.a) * 2.0
}

def double(h : Foo) : int {
    return h.a + h.a
}

def do(h : Foo) : int {
    return h.a * 10
}

[test]
def test_cpp_keyword_function_names(t : T?) {
    let h = Foo(a = 21)
    t |> run("def float") @(t : T?) {
        t |> equal(float(Foo(a = 41)), 42)
    }
    t |> run("def int") @(t : T?) {
        t |> equal(int(Foo(a = 3)), 6.0)
    }
    t |> run("def double") @(t : T?) {
        t |> equal(double(h), 42)
    }
    t |> run("def do") @(t : T?) {
        t |> equal(do(Foo(a = 4)), 40)
    }
}

// --- struct fields named after C++ keywords ---
struct StFields {
    char : int
    register : float
    extern : bool
    mutable : string
}

// --- struct named after a C++ keyword ---
struct register {
    value : int
}

// --- class named after a C++ keyword ---
class extern {
    payload : int = 99
    def get_payload() : int {
        return payload
    }
}

// --- enum named after a C++ keyword ---
enum signed {
    plus
    minus
}

// --- enum values named after C++ keywords ---
enum Width {
    short
    long
    union
}

// --- type alias named after a C++ keyword ---
typedef short = int

// --- function arg named after a C++ keyword ---
def double_it(char : int) : int {
    return char * 2
}

// --- function type with a keyword arg name ---
def call_with_int(f : function<(char : int) : int>; v : int) : int {
    return invoke(f, v)
}

// --- global var named after a C++ keyword ---
var g_char = 100

[test]
def test_for_loop_iter(t : T?) {
    t |> run("aot for-loop iter named char") @(t : T?) {
        var sum = 0
        for (char in [1, 2, 3]) {
            sum += char
        }
        t |> equal(sum, 6)
    }
}

[test]
def test_let_var(t : T?) {
    t |> run("aot let var named char") @(t : T?) {
        let char = 42
        t |> equal(char, 42)
    }
    t |> run("aot let with var named volatile") @(t : T?) {
        var volatile = 0
        volatile += 5
        volatile += 7
        t |> equal(volatile, 12)
    }
}

[test]
def test_global_var(t : T?) {
    t |> run("aot global var named g_char") @(t : T?) {
        t |> equal(g_char, 100)
    }
}

[test]
def test_struct_fields(t : T?) {
    t |> run("aot struct fields named after C++ keywords") @(t : T?) {
        let s = StFields(char = 5, register = 3.14, extern = true, mutable = "ok")
        t |> equal(s.char, 5)
        t |> equal(int(s.register * 100.0), 314)
        t |> equal(s.extern, true)
        t |> equal(s.mutable, "ok")
    }
}

[test]
def test_struct_name(t : T?) {
    t |> run("aot struct name 'register'") @(t : T?) {
        let r = register(value = 17)
        t |> equal(r.value, 17)
    }
}

[test]
def test_class_name(t : T?) {
    t |> run("aot class name 'extern'") @(t : T?) {
        var c = new extern()
        t |> equal(c->get_payload(), 99)
        unsafe { delete c; }
    }
}

[test]
def test_enum_name(t : T?) {
    t |> run("aot enum name 'signed'") @(t : T?) {
        let s = signed.plus
        t |> equal(s == signed.plus, true)
        t |> equal(s == signed.minus, false)
    }
}

[test]
def test_enum_values(t : T?) {
    t |> run("aot enum values short/long/union") @(t : T?) {
        let w = Width.short
        t |> equal(w == Width.short, true)
        t |> equal(w == Width.long, false)
        t |> equal(w == Width.union, false)
    }
}

[test]
def test_typedef_alias(t : T?) {
    t |> run("aot typedef alias named short") @(t : T?) {
        let x : short = 5
        t |> equal(x, 5)
    }
}

[test]
def test_function_arg(t : T?) {
    t |> run("aot function arg 'char'") @(t : T?) {
        t |> equal(double_it(5), 10)
    }
    t |> run("aot function type with keyword arg name") @(t : T?) {
        t |> equal(call_with_int(@@double_it, 7), 14)
    }
}

[test]
def test_lambda_arg(t : T?) {
    t |> run("aot lambda arg 'char'") @(t : T?) {
        var f <- @(char : int) => char * 3
        t |> equal(invoke(f, 4), 12)
        unsafe { delete f; }
    }
}
