struct Foo
    bar : int

def newFoo
    var t : Foo
    t.bar = 1
    return t

def newFooB(b:block<Foo>)
    return invoke(b)

def addBar(a,b:Foo)
    return a.bar + b.bar

def makeSequence(var base:auto(numT) -&;max)
    var a : array<numT>
    resize(a, max+1-base)
    for t in a
        t = base++
    return <- a

def joinSequences(l:array<auto(numT)>;r:array<numT>)
    var a : array<numT>
    reserve(a, length(l) + length(r))
    for t in l
        push(a, t)
    for t in r
        push(a, t)
    return <- a

def makeSequenceB(b:block< array<auto> >)
    return <- invoke(b)

[export]
def test
    // copy on return
    let t = addBar(newFoo(),newFoo())
    assert(t==2)
    let tt = addBar(newFooB({ var T : Foo; T.bar = 1; return T; }),
        newFooB({ var T : Foo; T.bar = 1; return T; }))
    assert(tt==2)
    // move on return
    let z <- joinSequences(makeSequence(0,4),makeSequence(5,9))
    assert(length(z)==10)
    for ttq in range(9)
        assert(z[ttq]==ttq)
    // move on return in block
    let zz <- joinSequences(makeSequenceB({ return <- makeSequence(0,4); }),
        makeSequenceB({ return <- makeSequence(5,9); }))
    assert(length(zz)==10)
    for ttt in range(9)
        assert(zz[ttt]==ttt)
    return true
