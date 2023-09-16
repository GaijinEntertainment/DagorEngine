expect 10006  // integer constant is out of range

[export]
def mainFn()
    let a = 2147483648   // 10006: integer constant is out of range
    if (a < 0)
        print("FAIL: a = {a}")
    else
        print("SUCCESS")
