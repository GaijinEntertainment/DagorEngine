// options log=true, log_nodes=true, log_stack=true

def make_10
    var a : array<float>
    for i in range(10)
        a |> push(float(i))
    return <- a

struct Bar
    t : string = "blah"
    ta : array<float> <- make_10()

[safe_when_uninitialized]
struct Foo
    data_bool:bool=true
    data_int:int = 1
    data_float:float = 3.14
    data_bar:Bar <- Bar()
    data_uint_3 : uint[3] = [[ uint 1u; 2u; 3u ]]

[export]
def test
    let f0 <- Foo()
    var f1 : Foo
    binary_save(f0) <| $(data)
        binary_load(f1, data)
        assert(f1.data_bool==true)
        assert(f1.data_int==1)
        assert(f1.data_float==3.14)
        assert(f1.data_bar.t=="blah")
        assert(f1.data_uint_3[0]==1u && f1.data_uint_3[1]==2u && f1.data_uint_3[2]==3u)
        for i in range(10)
            assert(f1.data_bar.ta[i]==float(i))
    return true


