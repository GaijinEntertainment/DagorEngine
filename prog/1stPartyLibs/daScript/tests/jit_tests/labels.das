options gen2
require dastest/testing_boost

require jit

var g_label = -1

def test_label(a : int) {
    goto a
    label 0:
    g_label = 0
    return
    label 1:
    g_label = 1
    return
    label 2:
    g_label = 2
    return
    label 3:
    g_label = 3
}

[sideeffects]
def test_static_label(first : bool) {
    if (first) {
        goto 1
    } else {
        goto 2
    }
    label 1:
    g_label = 1
    return
    label 2:
    g_label = 2
    return
}

[test]
def test_labels(t : T?) {
    t |> run("regular label") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_label))
        for (i in range(4)) {
            g_label = -1
            test_label(i)
            t |> equal(i, g_label)
        }
    }
    t |> run("panic label") @(t : T?) {
        // var failed = false
        // try {
        //     test_label(13)
        // } recover {
        //     failed = true
        // }
        // t |> success(failed)
    }
    t |> run("static label") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_static_label))
        g_label = -1
        test_static_label(true)
        t |> equal(1, g_label)
        g_label = -1
        test_static_label(false)
        t |> equal(2, g_label)
    }
}
