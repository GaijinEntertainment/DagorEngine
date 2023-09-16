require UnitTest

[export]
def test
    testFooArray() <| $(arr)
        verify(length(arr) == 10)
        verify(capacity(arr) == 16)
        let index = find_index_if(each(arr)) <| $ ( foo )
            return foo.fooData == 5
        assert(index==5)
    return true
