module foo_bar

[export]
def getValue
    return 13

[export]
def getDefaultValue(a:int;b:int=5;c:int)
    return a + b + c

[export]
def test
    // this is here because we compile all tests
    return true
