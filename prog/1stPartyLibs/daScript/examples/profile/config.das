require fio
require strings

let TEST_JAVASCRIPT = true
let TEST_LUAU = true
let TEST_LUA = true
let TEST_LUA_JIT = true
let TEST_SQUIRREL = true
let TEST_CSHARP = true
let TEST_MONO_INTERPRETER = false

[no_jit,no_aot]
def cpp_label
    print("\"C++\", ")

[no_jit,no_aot]
def is_cpp_time
    return !jit_enabled() && !aot_enabled()

struct TestFile
    luaFile : string // lua
    luaJitFile : string // lua jit
    luauFile : string // luau
    nutFile : string // squirrel
    nutModifiedFile : string // squirrel modified
    asFile  : string // angel script
    jsFile  : string // javascript
    monoFile  : string // csharp mono
    dotNetFile  : string // csharp .net

let ENCODING = string([{auto 0xefu8; 0xbbu8; 0xbfu8;}])

[no_jit,no_aot]
def run_and_gun ( lang,cmdLine:string )
    unsafe
        print("\"{lang}\", ")
        let exitCode = popen(cmdLine) <| $ ( f )
            var skipping = true
            while !feof(f)
                var st = fgets(f)
                if skipping
                    if st |> starts_with(ENCODING)  // MONO outputs encoding, skipping
                        st = st |> slice(3)
                        skipping = false
                print(st)
        if exitCode != 0
            print("Error: {exitCode}\n")

[no_jit,no_aot]
def run_and_gun_silent ( cmdLine:string )
    unsafe
        var problem = ""
        var exitCode = popen(cmdLine) <| $ ( f )
            while !feof(f)
                let st = fgets(f)
                problem = "{problem}\n{st}"
        if exitCode != 0
            print("Error: {exitCode}\n{problem}\n")
            return

[no_jit,no_aot]
def run_files ( tf:TestFile )
    if TEST_LUAU && !empty(tf.luauFile)
        // lua
        let luaRoot = "{get_das_root()}/examples/profile/tests/luau"
        let luaCompiler = "{luaRoot}/luau.exe"
        let cwd = getcwd()
        chdir(luaRoot)
        run_and_gun("LUAU --codegen","{luaCompiler} -O2 --codegen {luaRoot}/{tf.luauFile}")
        run_and_gun("LUAU","{luaCompiler} -O2 {luaRoot}/{tf.luauFile}")
        chdir(cwd)
    if TEST_LUA && !empty(tf.luaFile)
        // lua
        let luaRoot = "{get_das_root()}/examples/profile/tests/lua"
        let luaCompiler = "{luaRoot}/lua.exe"
        let cwd = getcwd()
        chdir(luaRoot)
        run_and_gun("LUA","{luaCompiler} {luaRoot}/{tf.luaFile}")
        chdir(cwd)
    if TEST_LUA_JIT && !empty(tf.luaJitFile)
        // lua
        let luaRoot = "{get_das_root()}/examples/profile/tests/lua"
        let luaJitCompiler = "{luaRoot}/luajit.exe"
        let cwd = getcwd()
        chdir(luaRoot)
        run_and_gun("LUAJIT -joff","{luaJitCompiler} -O3 -joff {luaRoot}/{tf.luaJitFile}")
        run_and_gun("LUAJIT","{luaJitCompiler} -O3 {luaRoot}/{tf.luaJitFile}")
        chdir(cwd)
    if TEST_SQUIRREL && !empty(tf.nutFile)
        // squirrel
        let nutRoot = "{get_das_root()}/examples/profile/tests/squirrel"
        let nutCompiler = "{nutRoot}/sq3-64.exe"
        let cwd = getcwd()
        chdir(nutRoot)
        run_and_gun("SQUIRREL3","{nutCompiler} {nutRoot}/{tf.nutFile}")
        chdir(cwd)
    if TEST_SQUIRREL && !empty(tf.nutModifiedFile)
        // quirrel
        let nutRoot = "{get_das_root()}/examples/profile/tests/squirrel"
        let numModifiedCompiler = "{nutRoot}/sq-64.exe"
        let cwd = getcwd()
        chdir(nutRoot)
        run_and_gun("QUIRREL","{numModifiedCompiler} {nutRoot}/{tf.nutModifiedFile}")
        chdir(cwd)
    if TEST_JAVASCRIPT && !empty(tf.jsFile)
        // javascript
        let jsRoot = "{get_das_root()}/examples/profile/tests/js"
        let jsCompiler = "{jsRoot}/qjs.exe"
        let cwd = getcwd()
        chdir(jsRoot)
        run_and_gun("QUICKJS","{jsCompiler} {jsRoot}/{tf.jsFile}")
        chdir(cwd)
    if TEST_CSHARP && !empty(tf.monoFile)
        // csharp
        let monoRoot = "{get_das_root()}/examples/profile/tests/cs"
        let monoCompiler = "mcs"
        let monoRunner = "mono"
        let cwd = getcwd()
        chdir(monoRoot)
        run_and_gun_silent("{monoCompiler} -optimize+ {monoRoot}/{tf.monoFile} -out:test.exe")
        if TEST_MONO_INTERPRETER
            run_and_gun("MONO --interpreter","{monoRunner} --interpreter test.exe")
        run_and_gun("MONO","{monoRunner} test.exe")
    if TEST_CSHARP && !empty(tf.dotNetFile)
        let cscRoot = "{get_das_root()}/examples/profile/tests/cs"
        let cscCompiler = "csc"
        let cwd = getcwd()
        chdir(cscRoot)
        run_and_gun_silent("{cscCompiler} /optimize+ {cscRoot}/{tf.dotNetFile} /out:testdotnet.exe")
        run_and_gun(".NET","testdotnet.exe")
        chdir(cwd)
