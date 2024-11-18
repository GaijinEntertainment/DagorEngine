var
    some_a <- [{ auto[] "test"}]

def test_a()
    assert(length(some_a) == 1)
    return <-clone(some_a)
let
    some_0 <- test_a()

[export]
def test_0
    return <-clone(some_0)

[export]
def test
    return true
