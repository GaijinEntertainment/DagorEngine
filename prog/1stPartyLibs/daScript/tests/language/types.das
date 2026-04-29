options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

var {
    ty_g_var_i : int = 1
    ty_g_var_f : float = 3.14
    ty_g_var_s : string = "blah"
}

def ty_test_global_var(t : T?) {
    t |> equal(ty_g_var_i, 1)
    ty_g_var_i = ty_g_var_i + 1
    t |> equal(ty_g_var_i, 2)
    t |> equal(ty_g_var_f, 3.14)
    ty_g_var_f = ty_g_var_f * 2.0
    t |> equal(ty_g_var_f, 6.28)
    t |> equal(ty_g_var_s, "blah")
    ty_g_var_s += "_blah"
    t |> equal(ty_g_var_s, "blah_blah")
}

def ty_test_local_var(t : T?) {
    var l_var_i : int = 1
    var l_var_f : float = 3.14
    var l_var_s : string = "blah"
    t |> equal(l_var_i, 1)
    l_var_i = l_var_i + 1
    t |> equal(l_var_i, 2)
    t |> equal(l_var_f, 3.14)
    l_var_f = l_var_f * 2.0
    t |> equal(l_var_f, 6.28)
    t |> equal(l_var_s, "blah")
    l_var_s += "_blah"
    t |> equal(l_var_s, "blah_blah")
}

def ty_test_arg(t : T?; var a_var_i : int = 1; var a_var_f : float = 3.14; var a_var_s : string = "blah") {
    t |> equal(a_var_i, 1)
    a_var_i = a_var_i + 1
    t |> equal(a_var_i, 2)
    t |> equal(a_var_f, 3.14)
    a_var_f = a_var_f * 2.0
    t |> equal(a_var_f, 6.28)
    t |> equal(a_var_s, "blah")
    a_var_s += "_blah"
    t |> equal(a_var_s, "blah_blah")
}

def ty_test_block_invoke(b : block<(var b_var_i : int; var b_var_f : float; var b_var_s : string) : void>) {
    invoke(b, 1, 3.14, "blah")
}

def ty_test_block_var(t : T?) {
    ty_test_block_invoke() <| $(var b_var_i : int; var b_var_f : float; var b_var_s : string) {
        t |> equal(b_var_i, 1)
        b_var_i = b_var_i + 1
        t |> equal(b_var_i, 2)
        t |> equal(b_var_f, 3.14)
        b_var_f = b_var_f * 2.0
        t |> equal(b_var_f, 6.28)
        t |> equal(b_var_s, "blah")
        b_var_s += "_blah"
        t |> equal(b_var_s, "blah_blah")
    }
}

[test]
def test_types(t : T?) {
    // reset globals for each test run
    ty_g_var_i = 1
    ty_g_var_f = 3.14
    ty_g_var_s = "blah"
    t |> run("global variables") @(t : T?) {
        ty_test_global_var(t)
    }
    t |> run("local variables") @(t : T?) {
        ty_test_local_var(t)
    }
    t |> run("argument variables") @(t : T?) {
        ty_test_arg(t)
    }
    t |> run("block variables") @(t : T?) {
        ty_test_block_var(t)
    }
}
