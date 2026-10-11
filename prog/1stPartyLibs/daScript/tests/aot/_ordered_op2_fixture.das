options gen2
options disable_auto_inline

var g_order = 0

def fa() : float {
    g_order = g_order * 10 + 1
    return 2.0
}

def fv() : float3 {
    g_order = g_order * 10 + 2
    return float3(1.0, 2.0, 3.0)
}

def fb() : bool {
    g_order ++
    return true
}

def fb2() : bool {
    g_order += 2
    return false
}

def fi() : int {
    g_order ++
    return 3
}

[export]
def pure_mul(a, b : float3) : float3 {
    return a * b
}

[export]
def cases() {
    let a = float3(fa()) * fv()
    let s = fi() + fi()
    let b = fb() && fb2()
    let c = fb() || fb2()
    let x = fb() ^^ fb2()
    print("{a} {s} {b} {c} {x}\n")
}
