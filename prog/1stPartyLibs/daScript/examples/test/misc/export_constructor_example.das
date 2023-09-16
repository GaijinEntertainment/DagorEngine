require daslib/export_constructor

[export_constructor]
class FooBar
    i : int = 10
    def FooBar
        print("c-tor {i}\n")


[export]
def main
    print("hello, world\n")
    var fb = make`FooBar()
    print("fb = {fb}\n")

options log
