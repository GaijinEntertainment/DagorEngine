require function_annotation_macro

[unique_transaction]
def add(a,b:int)
    print("a={a}, b={b}\n")
    return a + b

[export]
def test
    add(1,2)
    return true
