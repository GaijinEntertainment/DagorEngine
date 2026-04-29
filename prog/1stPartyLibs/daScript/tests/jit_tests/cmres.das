options gen2
require dastest/testing_boost

require UnitTest

def cmres_via_copy(var a : int[10]) {
    for (i in range(10)) {
        a[i] = 13
    }
    return a
}

def cmres_alias {
    var a : int[10]
    for (i in range(10)) {
        a[i] = i
    }
    return a
}

def cmres_var_init_via_call {
    var a = cmres_alias()
    return a
}

def cmres_copy_local(which : bool) {
    var a : int[10]
    var b : int[10]
    a = cmres_alias()
    return which ? a : b
}

def cmres_alias_no_jit {
    var a : int[10]
    for (i in range(10)) {
        a[i] = i
    }
    return a
}

def cmres_var_init_via_call_with_cmres {
    var a = cmres_alias_no_jit()    // calling non-jit function
    return a
}

def cmres_call_native {
    var a = testCMRES()
    return a
}

def cmres_return_call_cmres {
    return testCMRES()
}

def cmres_passthrough(var a : int[10]) {
    return cmres_via_copy(a)
}

struct ReturnFromFinally {
    values : table<string; int>
}

[export]
def ReturnFromFinally(str : string) {
    var res : ReturnFromFinally
    if (empty(str)) {
        return <- res
    }
    var inscope bytes : array<uint8>
    return <- res
}

[test]
def test_cmres(t : T?) {
    t |> success(!jit_enabled() || is_jit_function(@@ReturnFromFinally))
    t |> run("cmres via copy") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@cmres_via_copy))
        var x : int[10]
        var b = cmres_via_copy(x)
        for (xx in x) {
            t |> equal(13, xx)
        }
        for (bb in b) {
            t  |> equal(13, bb)
        }
    }
    t |> run("cmres passthrough via copy") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@cmres_passthrough))
        var x : int[10]
        var b = cmres_passthrough(x)
        for (xx in x) {
            t |> equal(13, xx)
        }
        for (bb in b) {
            t  |> equal(13, bb)
        }
    }
    t |> run("cmres alias") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@cmres_alias))
        var x = cmres_alias()
        for (xx, i in x, count()) {
            t |> equal(i, xx)
        }
    }
    t |> run("cmres var init via call") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@cmres_var_init_via_call))
        var x = cmres_var_init_via_call()
        for (xx, i in x, count()) {
            t |> equal(i, xx)
        }
    }
    t |> run("cmres copy local") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@cmres_copy_local))
        var x = cmres_copy_local(true)
        for (xx, i in x, count()) {
            t |> equal(i, xx)
        }
    }
    t |> run("cmres var init via call with cmres") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@cmres_var_init_via_call_with_cmres))
        var x = cmres_var_init_via_call_with_cmres()
        for (xx, i in x, count()) {
            t |> equal(i, xx)
        }
    }
    t |> run("cmres call native") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@cmres_call_native))
        var b = cmres_call_native()
        for (bb in b) {
            t  |> equal(13, bb)
        }
    }
    t |> run("cmres return call cmres") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@cmres_return_call_cmres))
        var b = cmres_return_call_cmres()
        for (bb in b) {
            t  |> equal(13, bb)
        }
    }
}