options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

struct CmrFoo {
    bar : int
}

def cmr_newFoo() : CmrFoo {
    var tf : CmrFoo
    tf.bar = 1
    return tf
}

def cmr_newFooB(b : block<CmrFoo>) : CmrFoo {
    return invoke(b)
}

def cmr_addBar(a, b : CmrFoo) : int {
    return a.bar + b.bar
}

def cmr_makeSequence(var base : auto(numT) -&; max : numT) : array<numT> {
    var a : array<numT>
    resize(a, int(max) + 1 - int(base))
    for (tf in a) {
        tf = base++
    }
    return <- a
}

def cmr_joinSequences(l : array<auto(numT)>; r : array<numT>) : array<numT> {
    var a : array<numT>
    reserve(a, length(l) + length(r))
    for (tf in l) {
        push(a, tf)
    }
    for (tf in r) {
        push(a, tf)
    }
    return <- a
}

def cmr_makeSequenceB(b : block<array<auto>>) : auto {
    return <- invoke(b)
}

[test]
def test_copy_and_move_on_return(t : T?) {
    t |> run("copy on return") @(t : T?) {
        let val = cmr_addBar(cmr_newFoo(), cmr_newFoo())
        t |> equal(val, 2)
        let val2 = cmr_addBar(
            cmr_newFooB() ${ var tf : CmrFoo; tf.bar = 1; return tf; },
            cmr_newFooB() ${ var tf : CmrFoo; tf.bar = 1; return tf; }
        )
        t |> equal(val2, 2)
    }
    t |> run("move on return") @(t : T?) {
        let z <- cmr_joinSequences(cmr_makeSequence(0, 4), cmr_makeSequence(5, 9))
        t |> equal(length(z), 10)
        for (i in range(9)) {
            t |> equal(z[i], i)
        }
    }
    t |> run("move on return in block") @(t : T?) {
        let z <- cmr_joinSequences(
            cmr_makeSequenceB() ${ return <- cmr_makeSequence(0, 4); },
            cmr_makeSequenceB() ${ return <- cmr_makeSequence(5, 9); }
        )
        t |> equal(length(z), 10)
        for (i in range(9)) {
            t |> equal(z[i], i)
        }
    }
}
