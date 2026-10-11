options gen2 = false

def classify(n : int)
    var total = 0
    for i in range(n)
        if i % 2 == 0
            total += i // counted
        else
            total -= i
    while total > 100
        total -= 10
    return total

def risky()
    try
        classify(3) // may fail
    recover
        return -1
    return 0

def commented(n : int) // what this one is for
    return n + 1 // and what it gives back

def take_block(blk : block<> const)
    invoke(blk)

def piped(n : int)
    var acc = 0
    take_block <|
        acc += n
    return acc

def indexed(a : array<array<int>>)
    var s = 0
    for tt in a[0]
        s += tt
    return s

def take_arg(blk : block<(a : int)> const)
    invoke(blk, 1)

def annotated()
    take_arg() <| $([[optional]] a : int)
        return a
    return 0

def traits(a : int)
    var [[lo, hi]] = (1, 2)
    return typeinfo(sizeof a) + lo + hi

enum Color
    red
    green

bitfield Flags2
    one
    two

variant Var1
    i : int
    s : string

struct Holder
    b : int

def spellings(var h : Holder; v : Var1)
    let c = Color green
    let fl = Flags2 one
    var p = 0
    unsafe
        p = reinterpret<int&> h.b
    if v is i && c == Color green
        p += int(fl)
    return p

def pair()
    return 1, "2"

def piped_literal()
    var blk = $ <| (a, b : int)
        return a + b
    return invoke(blk, 1, 2)

let GATE = false

def more_spellings()
    let braced_lambda = @ <| () {}
    var sum = 1
    static_if GATE
        sum += 1
    return sum
