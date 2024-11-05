// options log=true, log_nodes=true, print_var_access=true, print_ref=true
// options logCpp=true
options persistent_heap
require testProfile
require math

include ../config.das

def makeRandomSequence(var src:array<int>)
    let n = 100000
    let seed = 1u
    resize(src,n)
    for i in range(n)
        src[i] = int(uint_noise_1D(i, seed))

def sort_table(var tab:array<int>)
    sort ( tab, $(a,b) => a >= b )

def clone(var tab:array<int>; from:array<int>)
    let len = length(from)
    resize(tab, len)
    if len != 0
        unsafe
            memcpy(addr(tab[0]), addr(from[0]), len*typeinfo(sizeof type<int>))

[export,no_aot,no_jit]
def main
    var tab : array<int>
    makeRandomSequence(tab)
    profile(20, "sort")  <|
        var inscope tabb := tab
        sort_table(tabb)
    if is_cpp_time()
        cpp_label()
        profile(20, "sort")  <|
            var inscope tabb := tab
            testTableSort(tabb)
        run_files([[TestFile
            luaFile = "table-sort.lua",
            luaJitFile = "table-sort.lua",
            luauFile = "table-sort.luau",
            monoFile = "table-sort.cs",
            dotNetFile = "table-sort.cs",
            nutFile = "table-sort.nut",
            nutModifiedFile = "table-sort.nut"
        ]])
