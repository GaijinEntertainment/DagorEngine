// options persisten_heap   // this option is not necessary, we are setting this up from the C++ side

require tutorial_09

var count = 0

class Foo
    def Foo
        print("creating new Foo\n")
    def bar
        print("from Foo::bar\n")

[export]
def test
    count ++
    if count == 1
        print("FIRST CALL:\n")
        // first time call - we create lambda
        var f = new Foo()
        set_lambda() <| @   // this lambda captures f, so it stays on heap. we have no other references to f
            f->bar()
    elif count == 2
        pass
    else
        pass


