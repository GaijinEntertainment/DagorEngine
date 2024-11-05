// options log=true, print_var_access=true, print_ref=true

require math

// enable for test purposes only. this is slower
// options intern_strings = true

require testProfile
include ../config.das

def makeRandomSequence(var src:array<string>)
    let n = 500000
    let mod = uint(n)
    resize(src,n)
    for i in range(n)
        let num = (271828183u ^ uint(i*119))%mod
        src[i] = "{num}"

def dict(var tab:table<string;int>; src:array<string>)
    clear(tab)
    var maxOcc = 0
    for s in src
        maxOcc = max(++tab[s],maxOcc)
    return maxOcc

[export,no_jit,no_aot]
def main
    var tab : table<string;int>
    var src : array<string>
    makeRandomSequence(src)
	profile(20,"dictionary") <|
        dict(tab,src)
    if is_cpp_time()
        cpp_label()
        profile(20,"dictionary") <|
            testProfile::testDict(src)
        run_files([[TestFile
            luauFile = "dict.luau",
            luaFile = "dict.lua",
            luaJitFile = "dict.lua",
            nutFile = "dict.nut",
            nutModifiedFile = "dict.nut",
            // asFile = "dict.as",  // TODO: update AS to new version?
            jsFile = "dict.js",
            monoFile = "dict.cs",
            dotNetFile = "dict.cs",
        ]])
