// reserved identifier names
// verifies compiler rejects using reserved words (like 'register', 'do')
// as enum names, enum values, struct names, struct fields, function names,
// function args, local variables, and lambda parameters
options gen2
expect 30116:9

enum do {// 30116: invalid name
    foo
    bar
}

enum foo {
    register = 5    // 30116: invalid name
    noregister
}

struct Foo {
    register : string   // 30116: invalid name
}

struct register {// 30116: invalid name
    foo : int
}

def do(a : int) {// 30116: invalid name
    return a
}

def foo(register : int) {// 30116: invalid name
    return register
}

def bar(a : int) {
    let register = a        // 30116: invalid name
    return register
}

def any(x) {
    invoke(x, 12)
}

def lbm(a : int) {
    any() <| $(register : int) {// 30116: invalid name
        return a + register
    }
}

[export]
def test {
    return true
}
