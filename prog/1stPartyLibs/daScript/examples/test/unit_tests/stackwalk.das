options aot_prologue=true

def fnBottom
    stackwalk()
    return true

[no_aot]
def fnMid
    fnBottom()
    return true

def fnTop
    fnMid()
    return true

[export]
def test
    fnTop()
    return true

