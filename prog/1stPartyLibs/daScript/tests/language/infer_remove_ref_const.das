options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

def irc_logT(x : auto(numT)&) : string {
    return typeinfo typename(type<numT>)
}

def irc_logTNoRef(x : auto(numT) -&) : string {
    return typeinfo typename(type<numT>)
}

def irc_logTNoRefNoConst(x : auto(numT) -const -&) : string {
    return typeinfo typename(type<numT>)
}

def irc_aliasLogT(x : auto(numT)&; y : numT) : string {
    return typeinfo typename(type<numT>)
}

def irc_aliasLogTNoRef(x : auto(numT); y : numT -&) : string {
    let z : auto(passT) = y
    return typeinfo typename(type<passT>)
}

def irc_aliasLogTNoRefNoConst(x : auto(numT); y : numT -const -&) : string {
    var z : auto(passT) = y
    return typeinfo typename(type<passT>)
}

[test]
def test_infer_remove_ref_const(t : T?) {
    t |> run("auto ref inference") @(t : T?) {
        let x : auto const = 1
        t |> equal(irc_logT(x), "int const&")
        t |> equal(irc_logTNoRef(x), "int const")
        t |> equal(irc_logTNoRefNoConst(x), "int")
    }
    t |> run("alias ref inference") @(t : T?) {
        let x : auto const = 1
        t |> equal(irc_aliasLogT(x, x), "int const&")
        t |> equal(irc_aliasLogTNoRef(x, x), "int const")
        t |> equal(irc_aliasLogTNoRefNoConst(x, x), "int")
    }
}
