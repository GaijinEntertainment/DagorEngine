// options log=true, log_nodes=true, print_var_access=true, print_ref=true
// options logCpp=true
require testProfile

include ../config.das

[sideeffects]
def testAdds()
    var count = 0
    for i in range( 10000000)
        count = AddOne(count)
    return count

[export,no_aot,no_jit]
def main
	var count = 0
    profile(20,"native loop") <|
        count = testAdds()
	assert(10000000==count)
    if is_cpp_time()
        run_files([[TestFile
            asFile = "native.as",
            luaJitFile = "native.lua",
            nutModifiedFile = "native.nut",
            monoFile = "native.cs",
            dotNetFile = "native.cs"
        ]])
