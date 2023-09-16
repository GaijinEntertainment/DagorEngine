expect 30116:9

enum do // 30116: invalid name
    foo
    bar

enum foo
    template = 5    // 30116: invalid name
    notemplate

struct Foo
    template : string   // 30116: invalid name

struct template // 30116: invalid name
    foo : int

def do(a:int)   // 30116: invalid name
    return a

def foo(template:int)   // 30116: invalid name
    return template

def bar(a:int)
    let template = a        // 30116: invalid name
    return template

def any(x)
    invoke(x,12)

def lbm(a:int)
    any() <| $ ( template : int )   // 30116: invalid name
        return a + template

[export]
def test
    return true
