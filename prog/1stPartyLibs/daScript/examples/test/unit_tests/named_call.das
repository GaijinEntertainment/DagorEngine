// options log = true, log_infer_passes = true

def boo(a:int=1;b:int)
    return a + b

def boo(a,b,c:int)
    return a + b + c

def test_function_infer
    let t1 = boo([a=2,b=3])
    assert(t1==5)
    let t2 = boo([b=3])
    assert(t2==4)
    let t3 = boo([a=1,b=2,c=3])
    assert(t3==6)

def hoo(a=1;b)
    return a + b

def hoo(a,b,c)
    return a + b + c

def test_generic_infer
    let t1 = hoo([a=2,b=3])
    assert(t1==5)
    let t2 = hoo([b=3])
    assert(t2==4)
    let t3 = hoo([a=1,b=2,c=3])
    assert(t3==6)

[export]
def test
    test_function_infer()
    test_generic_infer()
    return true


