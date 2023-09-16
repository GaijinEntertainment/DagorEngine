require UnitTest

[export]
def test
    testFooArray() <| $(arr)
        for i in range(arr |> length)
            if arr[i].fooData != i
                panic("bad")
    return true
