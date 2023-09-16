expect 40102

[local_only (arg)]
def take_local(arg)
    print("arg={arg}\n")

struct Foo
    bar : int

[export]
def test
    take_local(1)                   // 40102: call annotated by local_only failed, expecting [[...]]
    take_local([[Foo bar=1]])       // ok
    take_local([[auto 1,2.0,"3"]])  // ok, tuple [[ ]]
    return true

