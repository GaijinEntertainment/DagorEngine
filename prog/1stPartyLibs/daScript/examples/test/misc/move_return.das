def iliketomoveitmoveit ( var a )
    return <- a // TODO: should kaboom here

def takeit ( var a )
    print("takit it {a} LC = {lock_count(a)}\n")

[export]
def main
    var a : array<array<int>>
    var b : array<array<int>>
    resize(a, 10)
    a[0] |> push(1)
    for t in a[0]
        takeit(iliketomoveitmoveit(a))
        // b <- a
    print("a = {a} LC = {lock_count(a)}\n")


options log
