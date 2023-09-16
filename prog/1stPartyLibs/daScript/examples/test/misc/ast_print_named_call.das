options log=true, log_infer_passes=true
require daslib/contracts

def test(a:int=1; b:int)
    print("{a},{b}\n")

[export]
def main
    test([a=1, b=2])
    test(3, 4, [a=1, b=2])
