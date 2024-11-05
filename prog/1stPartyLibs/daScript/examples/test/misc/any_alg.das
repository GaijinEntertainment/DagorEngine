require daslib/algorithm

[export]
def main
    var a <- [{for x in range(10); x}]
    var b : int[10]
    for A,B in a,b
        B = A
    print("a={a} b={b}\n")

    let lba = lower_bound ( a, 5 )
    let lbb = lower_bound ( b, 5 )

    print("lba(5)={lba} lbb(5)={lbb}\n")

    let lba2 = lower_bound ( b, 5 ) <| $(x, y: int) { return x < y; }
    print("lba2(5)={lba2}\n")
