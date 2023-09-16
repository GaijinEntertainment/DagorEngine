class A
    private onLog: lambda
    private y : int
    def A
        pass
    def install
        onLog <- @ <|
            y --
    def bang
        if onLog != null
            onLog |> invoke()
    def operator delete
        y ++

[export]
def test
    var a = new A()
    a->install()
    a->bang()
    return true




