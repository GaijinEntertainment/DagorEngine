require daslib/strings_boost
require peg/peg
require fio
require rtti
require debugapi
require math

var LOG_TEST_COMPILATION_TIME = false
var ENABLE_AOT = true
var ENABLE_JIT = true
var ENABLE_INTERPRETER = true


var failed = 0

def compile_and_run ( fileName:string; useAot:bool; useJit:bool )
    var t0 = ref_time_ticks()
    var inscope access <- make_file_access("")
    using <| $(var mg:ModuleGroup)
        using <| $(var cop:CodeOfPolicies)
            cop.aot_module = true
            cop.threadlock_context = true
            if useJit
                cop.jit = jit_enabled()
                cop.jit_module := "{get_das_root()}/daslib/just_in_time.das"
            elif useAot
                cop.aot = true
            else
                cop.jit = false
                cop.aot = false
            compile_file(fileName,access,unsafe(addr(mg)),cop) <| $(ok,program,issues)
                if !ok
                    print("failed to compile {fileName}\n{issues}\n")
                    failed ++
                    return
                simulate(program) <| $ ( sok; context; serrors ) // no aot for now
                    if !sok
                        print("failed to simulate {fileName}\n{serrors}\n")
                        failed ++
                        return
                    var dt = get_time_usec(t0)
                    var sec = double(dt)/1000000.0lf
                    if LOG_TEST_COMPILATION_TIME
                        print("compiled in {sec} sec\n")
                    try
                        unsafe(invoke_in_context(context,"main"))
                    recover
                        let ctx = unsafe(reinterpret<Context?> context)
                        print("failed to run {fileName}\n{ctx.exceptionAt}: {ctx.exception}\n")
                        failed ++


def test_single_file ( fileName:string )
    if ENABLE_AOT
        print("\"DAS AOT\", ")
        compile_and_run(fileName, true, false)
    if ENABLE_JIT
        print("\"DAS JIT\", ")
        compile_and_run(fileName, false, true)
    if ENABLE_INTERPRETER
        print("\"DAS INTERPRETER\", ")
        compile_and_run(fileName, false, false)
    if ENABLE_AOT || ENABLE_JIT || ENABLE_INTERPRETER
        print("\n\n")

def run_dir ( appDir : string )
    dir(appDir) <| $ ( fileName )
        if (!fileName |> starts_with("_")) && fileName |> ends_with(".das")
            let appfile = "{appDir}/{fileName}"
            test_single_file(appfile)

def get_cpu_name
    var cpuName = ""
    unsafe
        popen("wmic cpu get name") <| $ ( f )
            var prevName = false
            while !feof(f)
                let st = fgets(f)
                if st |> find("Name") != -1
                    prevName = true
                elif prevName
                    cpuName = st |> trim()
                    prevName = false
    return cpuName

struct ProfileEntry
    language : string
    category : string
    time     : double
    count    : int

def parse_profile_entry ( invocation:string; blk:block<(var res : ProfileEntry; errors:array<ParsingError>):void> )
    parse invocation
        var profile_entry : ProfileEntry
        rule(WS, string_ as language, WS, ",", WS, string_ as category, WS, ",", WS, double_ as time, WS, ",", WS, double_ as count) <|
            return <- [[ProfileEntry language=language, category=category, time=time, count=int(count)]]

def parse_short_profile_entry ( invocation:string; blk:block<(var res : ProfileEntry; errors:array<ParsingError>):void> )
    parse invocation
        var profile_entry : ProfileEntry
        rule(WS, string_ as category, WS, ",", WS, double_ as time, WS, ",", WS, double_ as count) <|
            return <- [[ProfileEntry category=category, time=time, count=int(count)]]

def make_table ( tablename:string; entries:array<ProfileEntry>; languages:table<string> )
    var categories : table<string; double>
    for e in entries
        if languages |> key_exists(e.language)
            if categories |> key_exists(e.category)
                categories[e.category] = min(e.time, categories[e.category])
            else
                categories[e.category] = e.time
    var slanguages <- [{for l in keys(languages); l}]
    slanguages |> sort
    var scategories <- [{for c in keys(categories); c}]
    scategories |> sort
    print("<h2>{tablename}</h2>\n")
    print("<table border=\"1\">\n")
    print("<tr><th>Test</th>\n")
    for l in slanguages
        print("<th>{l}</th>")
    print("</tr>\n")
    for c in scategories
        var cattime : table<string; double>
        for e in entries
            if e.category==c
                cattime[e.language] = e.time
        print("<tr><td>{c}</td>\n")
        for l in slanguages
            if cattime |> key_exists(l)
                let time = cattime[l]
                let mintime = categories[c]
                let factor = time / mintime
                let ts = fmt(":.4f", float(time))
                let fs = fmt(":.2f", float(factor))
                if int(factor*100.0lf) == 100
                    print("<td><b>{fs} ({ts})</b></td>")
                else
                    print("<td>{fs} ({ts})</td>")
            else
                print("<td>N/A</td>")
        print("</tr>\n")
    print("</table>\n")

def make_entries ( extra:string = "" )
    let exe = get_command_line_arguments()[0]
    let cmdLine = "{exe}{extra}"
    var entries : array<ProfileEntry>
    var prevLanguage = ""
    unsafe
        var exitCode = popen(cmdLine) <| $ ( f )
            while !feof(f)
                let st = fgets(f)
                var parsed = false
                parse_profile_entry(st) <| $ ( res, errors )
                    if errors |> empty
                        prevLanguage = res.language
                        entries |> push(res)
                        parsed = true
                        print("{res}\n")
                if !parsed
                    parse_short_profile_entry(st) <| $ ( res, errors )
                        if errors |> empty
                            res.language = prevLanguage
                            entries |> push(res)
                            parsed = true
                            print("{res}\n")
                if !parsed
                    print("{st}")
        if exitCode != 0
            print("Error: {exitCode}\n")
    return <- entries

def make_html
    var entries <- make_entries()
    print("\n<!DOCTYPE html><html>\n")
    print("<body>\n")
    print("<small>Tested on {get_cpu_name()} at {get_clock()}</small><br>\n")
    make_table("Interpreted", entries, {{
        "DAS INTERPRETER";
        "MONO --interpreter";
        "LUAU";
        "LUA";
        "LUAJIT -joff";
        "SQUIRREL3";
        "QUIRREL";
        "QUICKJS";
    }})
    make_table("AOT or JIT", entries, {{
        "DAS AOT";
        "DAS JIT";
        "C++";
        "MONO";
        ".NET";
        "LUAJIT";
        "LUAU --codegen";
    }})
    print("</body>\n")
    print("</html>\n\n")

def make_extra_html
    var entries <- make_entries(" -nomain -extra")
    print("\n<!DOCTYPE html><html>\n")
    print("<body>\n")
    print("<small>Tested on {get_cpu_name()}</small><br>\n")
    make_table("ExtraTests", entries, {{
        "DAS INTERPRETER";
        "DAS AOT";
        "DAS JIT"
    }})
    print("</body>\n")
    print("</html>\n\n")

[export]
def main
    var cpuName = get_cpu_name()
    print("Testing on {cpuName} at {get_clock()}\n")
    var singleTest = ""
    var mainTests = true
    var extraTests = false
    var hashTests = false
    print("{get_command_line_arguments()}\n")
    let nArgs = get_command_line_arguments() |> length()
    var i = 0
    while i < nArgs
        let arg = get_command_line_arguments()[i]
        if arg=="-test" && i < nArgs-1
            singleTest = get_command_line_arguments()[i+1]
            i += 2
            continue
        elif arg=="-html"
            make_html()
            // make_extra_html()
            return
        elif arg=="-log"
            LOG_TEST_COMPILATION_TIME = true
        elif arg=="-extra"
            extraTests = true
        elif arg=="-hash"
            hashTests = true
        elif arg=="-nomain"
            mainTests = false
        elif arg=="-help"
            print("Usage: profile_tests [-test <testname>] [-log] [-extra] [-hash]\n")
            print("  -test testname  runs /examples/profile/tests/test.das and exits immediately\n")
            print("  -log            logs compilation time\n")
            print("  -extra          runs extra tests\n")
            print("  -hash           runs hash tests\n")
            print("  -nomain         disables main tests\n")
            return
        i += 1
    if !empty(singleTest)
        let appfile = "{get_das_root()}/examples/profile/tests/{singleTest}.das"
        test_single_file(appfile)
        return
    if mainTests
        run_dir("{get_das_root()}/examples/profile/tests")
    if extraTests
        run_dir("{get_das_root()}/examples/profile/extra_test")
    if hashTests
        run_dir("{get_das_root()}/examples/profile/hash_test")
    if failed != 0
        print("TOTAL FAILURES: {failed}\n")


