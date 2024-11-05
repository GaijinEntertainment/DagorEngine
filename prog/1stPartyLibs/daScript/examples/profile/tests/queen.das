require testProfile

include ../config.das

options solid_context

let N = 8

def isplaceok ( a:int[8]; n,c,cmn,cpn:int )
    for i in range(n)
        let ai = a[i]
        if ai == c || ai == cmn + i || ai  == cpn - i
            return false
    return true

var solutions : int

def addqueen ( var a:int[8]; n:int )
    for c in range(N)
        if isplaceok(a, n, c, c-n, c+n)
            a[n] = c
            if n == (N-1)
                solutions++
            else
                addqueen(a, n + 1)

def test()
    solutions = 0
    var a : int[8]
    addqueen(a,0)
    assert(solutions == 92)

[export,no_jit,no_aot]
def main
    profile(20, "queen")  <|
        test()
    if is_cpp_time()
        cpp_label()
        profile(20, "queen")  <|
            let size = testQueens()
            assert(size == 92)
        run_files([[TestFile
            luauFile = "queen.luau",
            luaFile = "queen.lua",
            luaJitFile = "queen.lua",
            nutFile = "queen.nut",
            nutModifiedFile = "queen.nut",
            // monoFile = "queen.cs",
            dotNetFile = "queen.cs",
        ]])

