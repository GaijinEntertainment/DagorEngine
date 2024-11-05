// options log=true, log_nodes=true, print_var_access=true, print_ref=true

require math
require testProfile

include ../config.das

[sideeffects]
def expLoop(n)
    var sum = 0.
    for i in range( n)
        sum += exp(rcp_est(float(i+1)))
    return sum

def verifyExp(f)
	let t = 1e+06
	let eps = 10.
	let q = abs(f-t)
	assert(q < eps)

[export,no_jit,no_aot]
def main
	var f1 = 0.
    profile(20,"exp loop") <|
        f1 = expLoop(1000000)
	verifyExp(f1)
    if is_cpp_time()
        var f4 = 0.
        cpp_label()
        profile(20,"exp loop") <|
            f4 = testProfile::testExpLoop(1000000)
        verifyExp(f4)
        run_files([[TestFile
            luauFile = "exp.luau",
            luaFile = "exp.lua",
            luaJitFile = "exp.lua",
            nutFile = "exp.nut",
            nutModifiedFile = "exp.nut",
            // asFile = "exp.as", // TODO: implement
            jsFile = "exp.js",
            monoFile = "exp.cs",
            dotNetFile = "exp.cs",
        ]])
