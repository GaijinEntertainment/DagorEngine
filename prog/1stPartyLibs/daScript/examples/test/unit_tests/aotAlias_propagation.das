// mainly to test, if it compiles and runs correctly with AOT compilation

struct MyStruct {
    value: int;
}

var a = 1

def my_func(t: type<auto(TT)>; func: lambda<(arg:TT?):void>) {
    invoke(func, null);
}

[export]
def test() {
    my_func(type<MyStruct>, @(arg: MyStruct?){
        assert(arg==null);
        assert(a == 1);
        a ++;
    });
    assert(a == 2);
    return true;
}
