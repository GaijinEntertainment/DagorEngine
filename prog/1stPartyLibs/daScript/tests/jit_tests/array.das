options gen2
require dastest/testing_boost

def makeAll13(var a : array<int>) {
    for (i in range(length(a))) {
        a[i] = 13
    }
}

def addAll(var a : array<int>) {
    var c = 0
    for (i in range(length(a))) {
        c += a[i]
    }
    return c
}

def resizeLocked(var a : array<int>) {
    for (_a, i in a, count()) {
        if (i == 3) {
            a |> resize(20)
        }
    }
}

def outOfRange(var a : array<int>) {
    return a[13]
}

def check_array_float3(t : T?; a : array<float3>) {
    for (v, i in a, count()) {
        t |> equal(float3(i + 1, i + 2, i + 3), v)
    }
}

def check_dim_float3(t : T?; a : float3[3]) {
    for (v, i in a, count()) {
        t |> equal(float3(i + 1, i + 2, i + 3), v)
    }
}

[test]
def test_array(t : T?) {
    t |> run("array float3") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@check_array_float3))
        var a <- array<float3>(
            float3(1, 2, 3),
            float3(2, 3, 4),
            float3(3, 4, 5)
        )
        check_array_float3(t, a)
    }
    t |> run("dim float3") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@check_dim_float3))
        var a <- fixed_array<float3>(
            float3(1, 2, 3),
            float3(2, 3, 4),
            float3(3, 4, 5)
        )
        check_dim_float3(t, a)
    }
    t |> run("array []") @(t : T?) {
        var x : array<int>
        resize(x, 10)
        t |> success(!jit_enabled() || is_jit_function(@@makeAll13))
        t |> success(!jit_enabled() || is_jit_function(@@addAll))
        makeAll13(x)
        for (i in x) {
            t |> equal(i, 13)
        }
        t |> equal(addAll(x), 130)
    }
    t |> run("out of range") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@outOfRange))
        var x : array<int>
        resize(x, 10)
        var failed = false
        try {
            var r = outOfRange(x)
            t |> success(r == 123456)// code never gets here
        } recover {
            failed = true
        }
        t |> success(failed, "out of range caused panic")
    }
    t |> run("resize locked") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@resizeLocked))
        var x : array<int>
        resize(x, 10)
        var failed = false
        try {
            resizeLocked(x)
        } recover {
            failed = true
        }
        t |> success(failed, "resize caused panic")
    }
}
