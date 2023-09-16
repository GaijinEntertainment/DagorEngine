require daslib/rst_comment

struct Foo
    /* this is a C comment in structure */
    a : int // this is a C++ comment in the field a
    no_comment : float
    b,c : int // this is a C++ comment for the fields b, c

struct Bar
    // this is a C++ comment in structure
    a : int

let some_const = 1234 // this is a C++ comment for a constant

[export]
def foo ( a, b : int )
    // this is a C++ comment
    pass

[export]
def bar ( a, b : int )
    /* this is a C comment */
    pass

class MyClass
    /* this is a C comment in class */
    a : int // this is a C++ comment in the class field a
    no_comment : float
    b,c : int // this is a C++ comment for the class fields b, c
    def abstract abstract_method ( t : int ) : void // this is comment for an abstract method
    def no_comment_method
        pass
    def foo ( t : int ) : int       /* this is comment for the method foo */
        // this is a C++ comment, for the method foo
        return 1

[export]
def main
    let c = new MyClass()
    print("hello, world!\n")
