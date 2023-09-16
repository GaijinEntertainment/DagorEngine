// options log=true, logCpp=true

require UnitTest

[export]
def test
    test_das_string() <| $(var s,s2:das_string)
        s2 := "test_das_string"
        assert("{s}"=="test_das_string")
        assert(string(s)=="test_das_string")
        peek(s) <| $(st)
            assert(st=="test_das_string")
        assert(s=="test_das_string")
        s := "out_of_it"
        assert("{s}"=="out_of_it")
        assert(string(s)=="out_of_it")
    return true

