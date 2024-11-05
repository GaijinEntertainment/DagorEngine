// options log=true, print_var_access=true, print_ref=true

require testProfile

include ../config.das

[sideeffects]
def fibI(n)
    var last = 1
    var cur = 0
    for i in range(n)
        let tmp = cur
        cur += last
        last = tmp
    return cur

[export,no_jit,no_aot]
def main
	var f1 = 0
    profile(20,"fibonacci loop") <|
        f1 = fibI(6511134)
	assert(f1==1781508648)
    if is_cpp_time()
        cpp_label()
        var f4 = 0
        profile(20,"fibonacci loop") <|
            f4 = testProfile::testFibI(6511134)
        assert(f4==1781508648)
        run_files([[TestFile
            luauFile = "fib_loop.luau",
            luaFile = "fib_loop.lua",
            luaJitFile = "fib_loop.lua",
            nutFile = "fib_loop.nut",
            nutModifiedFile = "fib_loop.nut",
            asFile = "fib_loop.as",
            jsFile = "fib_loop.js",
            monoFile = "fib_loop.cs",
            dotNetFile = "fib_loop.cs",
        ]])
