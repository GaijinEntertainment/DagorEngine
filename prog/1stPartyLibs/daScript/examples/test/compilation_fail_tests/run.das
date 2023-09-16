expect 40101

var
    g = 5

[run]
def add13(n)
    g = n
    return n + 13

[export]
def test
    print("{add13(3)}\n")       //  40101: function did not run at compilation time because it has side-effects
    return true


