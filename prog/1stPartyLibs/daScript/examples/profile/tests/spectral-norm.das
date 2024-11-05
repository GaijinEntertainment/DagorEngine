// options log=true, print_var_access=true, print_ref=true

require testProfile

options stack = 1_000_000
options solid_context

include ../config.das

require math

let N = 500

def A ( i,j : int )
    return double( (((i+j) * (i+j+1)) >> 1) + i + 1)

def dot ( v,u : double[N] ) : double
    var sum = 0.0lf
    for V,U in v,u
        sum += V * U
    return sum

def mult_Av ( v : double[N]; var out : double[N]  )
    for i in range(N)
        var sum = 0.0lf
        for j in range(N)
            sum += v[j] / A(i,j)
        out[i] = sum

def mult_Atv ( v : double[N]; var out : double[N] )
    for i in range(N)
        var sum = 0.0lf
        for j in range(N)
            sum += v[j] / A(j,i)
        out[i] = sum

var tmp : double[N]
def mult_AtAv ( v : double[N]; var out : double[N] )
    mult_Av(v,tmp)
    mult_Atv(tmp,out)

[sideeffects]
def test
    var u,v : double[N]
    for i in range(N)
        u[i] = 1.0lf
    for i in range(10)
        mult_AtAv(u,v)
        mult_AtAv(v,u)
    var vBv = dot(u,v)
    var vv = dot(v,v)
    return sqrt(vBv / vv)

[export,no_jit,no_aot]
def main
    let ref = 1.274224153lf
    var f1 : double
    profile(10,"spectral norm") <|
        f1 = test()
    assert( abs(f1 - ref) < 1e-5lf, "spectral norm" )
    if is_cpp_time()
        cpp_label()
        profile(10,"spectral norm") <|
            testSnorm()
        run_files([[TestFile
            luaFile = "spectral-norm.lua",
            luauFile = "spectral-norm.luau",
            luaJitFile = "spectral-norm.lua",
            monoFile = "spectral-norm.cs",
            nutFile = "spectral-norm.nut",
            nutModifiedFile = "spectral-norm.nut",
            dotNetFile = "spectral-norm.cs",
            jsFile = "spectral-norm.js",
        ]])
