options gen2
require dastest/testing_boost

def jit_not(a : bool) {
    return !a
}

def jit_xorxor(a, b : bool) {
    return a ^^ b
}

def jit_set_xorxor(var a : bool; b : bool) {
    a ^^= b
    return a
}

var monad_test = 0

[sideeffects]
def getA_true {
    monad_test |= 1
    return true
}

[sideeffects]
def getA_false {
    return false
}

[sideeffects]
def getB_true {
    monad_test |= 2
    return true
}

[sideeffects]
def getB_false {
    monad_test |= 4
    return false
}

def jit_andand_true_true {
    return getA_true() && getB_true()
}

def jit_andand_true_false {
    return getA_true() && getB_false()
}

def jit_andand_false_true {
    return getA_false() && getB_true()
}

def jit_andand_false_false {
    return getA_false() && getB_false()
}

def jit_set_andand_true_true {
    var a = getA_true()
    a &&= getB_true()
    return a
}

def jit_set_andand_true_false {
    var a = getA_true()
    a &&= getB_false()
    return a
}

def jit_set_andand_false_true {
    var a = getA_false()
    a &&= getB_true()
    return a
}

def jit_set_andand_false_false {
    var a = getA_false()
    a &&= getB_false()
    return a
}


def jit_oror_true_true {
    return getA_true() || getB_true()
}

def jit_oror_true_false {
    return getA_true() || getB_false()
}

def jit_oror_false_true {
    return getA_false() || getB_true()
}

def jit_oror_false_false {
    return getA_false() || getB_false()
}

def jit_set_oror_true_true {
    var a = getA_true()
    a ||= getB_true()
    return a
}

def jit_set_oror_true_false {
    var a = getA_true()
    a ||= getB_false()
    return a
}

def jit_set_oror_false_true {
    var a = getA_false()
    a ||= getB_true()
    return a
}

def jit_set_oror_false_false {
    var a = getA_false()
    a ||= getB_false()
    return a
}

[test]
def test_bool_arithmetics(t : T?) {
    t |> run("!") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@jit_not))
        t |> equal(true, jit_not(false))
        t |> equal(false, jit_not(true))
    }
    t |> run("^^ ^^=") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@jit_xorxor))
        t |> equal(false, jit_xorxor(false, false))
        t |> equal(true,  jit_xorxor(true, false))
        t |> equal(true,  jit_xorxor(false, true))
        t |> equal(false, jit_xorxor(true, true))
        t |> success(!jit_enabled() || is_jit_function(@@jit_set_xorxor))
        t |> equal(false, jit_set_xorxor(false, false))
        t |> equal(true,  jit_set_xorxor(true, false))
        t |> equal(true,  jit_set_xorxor(false, true))
        t |> equal(false, jit_set_xorxor(true, true))
    }
    t |> run("&&") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@jit_andand_true_true))
        monad_test = 0
        var tt = jit_andand_true_true()
        t |> success(tt == true)
        t |> success(monad_test == 3)
        t |> success(!jit_enabled() || is_jit_function(@@jit_andand_true_false))
        monad_test = 0
        var tf = jit_andand_true_false()
        t |> success(tf == false)
        t |> success(monad_test == 5)
        t |> success(!jit_enabled() || is_jit_function(@@jit_andand_false_true))
        monad_test = 0
        var ft = jit_andand_false_true()
        t |> success(ft == false)
        t |> success(monad_test == 0)
        t |> success(!jit_enabled() || is_jit_function(@@jit_andand_false_false))
        monad_test = 0
        var ff = jit_andand_false_false()
        t |> success(ff == false)
        t |> success(monad_test == 0)
    }
    t |> run("&&=") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@jit_set_andand_true_true))
        monad_test = 0
        var tt = jit_set_andand_true_true()
        t |> success(tt == true)
        t |> success(monad_test == 3)
        t |> success(!jit_enabled() || is_jit_function(@@jit_set_andand_true_false))
        monad_test = 0
        var tf = jit_set_andand_true_false()
        t |> success(tf == false)
        t |> success(monad_test == 5)
        t |> success(!jit_enabled() || is_jit_function(@@jit_set_andand_false_true))
        monad_test = 0
        var ft = jit_set_andand_false_true()
        t |> success(ft == false)
        t |> success(monad_test == 0)
        t |> success(!jit_enabled() || is_jit_function(@@jit_set_andand_false_false))
        monad_test = 0
        var ff = jit_set_andand_false_false()
        t |> success(ff == false)
        t |> success(monad_test == 0)
    }
    t |> run("||") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@jit_oror_true_true))
        monad_test = 0
        var tt = jit_oror_true_true()
        t |> success(tt == true)
        t |> success(monad_test == 1)
        t |> success(!jit_enabled() || is_jit_function(@@jit_oror_true_false))
        monad_test = 0
        var tf = jit_oror_true_false()
        t |> success(tf == true)
        t |> success(monad_test == 1)
        t |> success(!jit_enabled() || is_jit_function(@@jit_oror_false_true))
        monad_test = 0
        var ft = jit_oror_false_true()
        t |> success(ft == true)
        t |> success(monad_test == 2)
        t |> success(!jit_enabled() || is_jit_function(@@jit_oror_false_false))
        monad_test = 0
        var ff = jit_oror_false_false()
        t |> success(ff == false)
        t |> success(monad_test == 4)
    }
    t |> run("||=") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@jit_set_oror_true_true))
        monad_test = 0
        var tt = jit_set_oror_true_true()
        t |> success(tt == true)
        t |> success(monad_test == 1)
        t |> success(!jit_enabled() || is_jit_function(@@jit_set_oror_true_false))
        monad_test = 0
        var tf = jit_set_oror_true_false()
        t |> success(tf == true)
        t |> success(monad_test == 1)
        t |> success(!jit_enabled() || is_jit_function(@@jit_set_oror_false_true))
        monad_test = 0
        var ft = jit_set_oror_false_true()
        t |> success(ft == true)
        t |> success(monad_test == 2)
        t |> success(!jit_enabled() || is_jit_function(@@jit_set_oror_false_false))
        monad_test = 0
        var ff = jit_set_oror_false_false()
        t |> success(ff == false)
        t |> success(monad_test == 4)
    }
}
