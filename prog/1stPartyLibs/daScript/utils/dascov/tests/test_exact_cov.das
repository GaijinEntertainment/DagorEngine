options gen2

options no_coverage

require daslib/ast_boost
require daslib/coverage
require daslib/strings_boost
require dastest/testing_boost

require daslib/debugger
require daslib/fio
require daslib/rtti
require strings

def compile_and_simulate(input : string; var access; var mg : ModuleGroup?; cop; blk) {
    compile_file(input, access, mg, cop) $(ok; var program : smart_ptr<Program>; issues) {
        if (!ok) {
            print("failed to compile {input}\n{issues}\n")
            return
        }
        simulate(program) $(sok; var pctx : smart_ptr<Context>; serrors) {
            if (!sok) {
                panic("Failed to simulate {serrors}")
            }
            blk(program, pctx)
        }
    }
}

def get_coverage_report(input_file : string) : string {
    var report : string
    using <| $(var cop : CodeOfPolicies) {
        using() $(var mg : ModuleGroup) {
            var inscope access <- make_file_access("")
            cop.threadlock_context = true
            access |> add_extra_module("coverage", "{get_das_root()}/daslib/coverage.das")
            compile_and_simulate(input_file, access, unsafe(addr(mg)), cop) $(program : ProgramPtr; var pctx : smart_ptr<Context>) {
                with_argv(array<string>()) {
                    unsafe(invoke_in_context(pctx, "main"))
                }
                var rep = ""
                unsafe(invoke_in_context(pctx, "fill_coverage_report", unsafe(addr(rep)), ""))
                // Clone to current context.
                report = clone_string(rep)
            }
        }
    }
    return report
}

def has_line(report : string; line : string) : bool {
    return find(report, line) >= 0
}

def normalize_report(report : string) : string {
    // Replace absolute path prefix with relative "tests/" prefix
    let root = "{get_das_root()}/utils/dascov/"
    return replace(report, root, "")
}

[test]
def test_simple_branch(t : T?) {
    let input_file = "{get_das_root()}/utils/dascov/tests/simple_branch.das"
    let report = normalize_report(get_coverage_report(input_file))
    t |> run("source file") <| @(t : T?) {
        t |> success(report |> has_line("SF:tests/simple_branch.das"))
    }
    t |> run("functions registered") <| @(t : T?) {
        t |> success(report |> has_line("FN:5,foo"))
        t |> success(report |> has_line("FN:10,main"))
    }
    t |> run("foo not called, main called") <| @(t : T?) {
        t |> success(report |> has_line("FNDA:0,foo"))
        t |> success(report |> has_line("FNDA:1,main"))
    }
    t |> run("function counts") <| @(t : T?) {
        t |> success(report |> has_line("FNF:2"))
        t |> success(report |> has_line("FNH:1"))
    }
    t |> run("branches") <| @(t : T?) {
        // if (x > 1) on line 12: true branch taken, false branch not taken
        t |> success(report |> has_line("BRDA:12,0,0,1"))
        t |> success(report |> has_line("BRDA:12,0,1,-"))
        t |> success(report |> has_line("BRF:2"))
        t |> success(report |> has_line("BRH:1"))
    }
    t |> run("line coverage") <| @(t : T?) {
        // main body lines are hit
        t |> success(report |> has_line("DA:11,1"))
        t |> success(report |> has_line("DA:12,1"))
        t |> success(report |> has_line("DA:13,1"))
        // else branch line not hit
        t |> success(report |> has_line("DA:15,0"))
        // foo body not hit
        t |> success(report |> has_line("DA:6,0"))
    }
}

[test]
def test_simple_linear(t : T?) {
    let input_file = "{get_das_root()}/utils/dascov/tests/simple_linear.das"
    let report = normalize_report(get_coverage_report(input_file))
    t |> run("source file") <| @(t : T?) {
        t |> success(report |> has_line("SF:tests/simple_linear.das"))
    }
    t |> run("main called") <| @(t : T?) {
        t |> success(report |> has_line("FNDA:1,main"))
        t |> success(report |> has_line("FNF:1"))
        t |> success(report |> has_line("FNH:1"))
    }
    t |> run("no branches") <| @(t : T?) {
        t |> success(report |> has_line("BRF:0"))
        t |> success(report |> has_line("BRH:0"))
    }
    t |> run("all lines hit") <| @(t : T?) {
        t |> success(report |> has_line("DA:5,1"))
        t |> success(report |> has_line("DA:6,1"))
        t |> success(report |> has_line("DA:7,1"))
    }
}

[test]
def test_multi_func(t : T?) {
    let input_file = "{get_das_root()}/utils/dascov/tests/multi_func.das"
    let report = normalize_report(get_coverage_report(input_file))
    t |> run("all functions registered") <| @(t : T?) {
        t |> success(report |> has_line("FN:3,called_func"))
        t |> success(report |> has_line("FN:7,uncalled_func"))
        t |> success(report |> has_line("FN:12,main"))
    }
    t |> run("called_func and main hit, uncalled_func not") <| @(t : T?) {
        t |> success(report |> has_line("FNDA:1,called_func"))
        t |> success(report |> has_line("FNDA:0,uncalled_func"))
        t |> success(report |> has_line("FNDA:1,main"))
    }
    t |> run("function counts") <| @(t : T?) {
        t |> success(report |> has_line("FNF:3"))
        t |> success(report |> has_line("FNH:2"))
    }
    t |> run("uncalled function lines are 0") <| @(t : T?) {
        t |> success(report |> has_line("DA:8,0"))
    }
    t |> run("called function lines are hit") <| @(t : T?) {
        t |> success(report |> has_line("DA:4,1"))
        t |> success(report |> has_line("DA:13,1"))
    }
}

[test]
def test_nested_branch(t : T?) {
    let input_file = "{get_das_root()}/utils/dascov/tests/nested_branch.das"
    let report = normalize_report(get_coverage_report(input_file))
    t |> run("main registered and called") <| @(t : T?) {
        t |> success(report |> has_line("FN:4,main"))
        t |> success(report |> has_line("FNDA:1,main"))
    }
    t |> run("outer branch: true taken, false not") <| @(t : T?) {
        // if (x > 5) on line 6, block 1
        t |> success(report |> has_line("BRDA:6,1,0,1"))
        t |> success(report |> has_line("BRDA:6,1,1,-"))
    }
    t |> run("inner branch: true not taken, false taken") <| @(t : T?) {
        // if (x > 20) on line 7, block 0
        t |> success(report |> has_line("BRDA:7,0,0,-"))
        t |> success(report |> has_line("BRDA:7,0,1,1"))
    }
    t |> run("branch counts") <| @(t : T?) {
        // 2 if statements = 4 branches
        t |> success(report |> has_line("BRF:4"))
        // outer true + inner false = 2 hit
        t |> success(report |> has_line("BRH:2"))
    }
    t |> run("covered lines") <| @(t : T?) {
        t |> success(report |> has_line("DA:5,1"))  // var x = 10
        t |> success(report |> has_line("DA:6,1"))  // if (x > 5)
        t |> success(report |> has_line("DA:7,1"))  // if (x > 20)
        t |> success(report |> has_line("DA:10,1")) // feint("shallow")
    }
    t |> run("uncovered lines") <| @(t : T?) {
        t |> success(report |> has_line("DA:8,0"))  // feint("deep")
        t |> success(report |> has_line("DA:13,0")) // feint("low")
    }
}
