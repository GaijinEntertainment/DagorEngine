// options log=true, log_generics=false, optimize=true

options no_unused_function_arguments = false

def logT(x:auto(numT)&)
    return typeinfo(typename type<numT>)

def logTNoRef(x:auto(numT) -&)
    return typeinfo(typename type<numT>)

def logTNoRefNoConst(x:auto(numT) -const -&)
    return typeinfo(typename type<numT>)

def aliasLogT(x:auto(numT)&;y:numT)
    return typeinfo(typename type<numT>)

def aliasLogTNoRef(x:auto(numT);y:numT -&)
    let z : auto(passT) = y
    return typeinfo(typename type<passT>)

def aliasLogTNoRefNoConst(x:auto(numT);y:numT -const -&)
    var z : auto(passT) = y
    return typeinfo(typename type<passT>)

[export]
def test
    let x : auto const = 1
    assert(logT(x)=="int const&")
    assert(logTNoRef(x)=="int const")
    assert(logTNoRefNoConst(x)=="int")
    assert(aliasLogT(x,x)=="int const&")
    assert(aliasLogTNoRef(x,x)=="int const")
    assert(aliasLogTNoRefNoConst(x,x)=="int")
    return true
