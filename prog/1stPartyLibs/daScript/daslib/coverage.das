/*
 * Code coverage macro for daslang.
 * To make it work you should add coverage module:
 * `require daslib/coverage`
 * After executing tests/anything else add call to
 * `fill_coverage_report(filename) : string`
 * to get coverage report. To create html from it use:
 * `genhtml cov.lcov -o coverage-report  --synthesize-missing --ignore-errors source`
 */
options gen2
// `inscope` is necessary, so all modules will include it.
module coverage shared !inscope

//! Code coverage instrumentation macro.
//!
//! Automatically inserts coverage tracking calls at every block entry point,
//! function entry, and branch (if/else) decision point.
//! After execution, call `fill_coverage_report` to produce output in LCOV format,
//! which can be converted to HTML via ``genhtml``.

require daslib/ast
require daslib/fio
require daslib/rtti
require strings

require daslib/ast_boost
require daslib/strings_boost
require daslib/templates_boost


let private builtin_files = fixed_array("fio.das", "ast.das", "builtin.das", "debugger.das", "network.das", "rtti.das")

def private resolve_file_path(name : string) : string {
    for (bf in builtin_files) {
        if (name == bf) {
            return "{get_das_root()}/src/builtin/{name}"
        }
    }
    return name
}

struct LineCov {
    //! Per-file coverage data storing line hit counts.
    lines : table<uint; uint>

    def insert(x : uint) {
        //! Increments the hit count for the given line number.
        unsafe(lines[x]) += 1u
    }
}

struct FuncCov {
    //! Per-function coverage entry storing line number, hit count, and per-line coverage.
    line : uint
    hits : uint
    line_cov = LineCov()
}

struct BranchCov {
    //! Per-branch coverage entry storing line, block id, branch id and hit count.
    line : uint
    block_num : uint
    branch_num : uint
    hits : uint
}

struct CoverageData {
    line_cov = LineCov() // Line coverage.
    func_cov : table<string; FuncCov> // Function name -> Function coverage.
    branch_cov : table<uint; BranchCov> // branch_idx -> Branch coverage.
}

var public coverageData = table<string; CoverageData>()

def private single_file_report(name : string) {
    var fnFound = 0
    var fnHit = 0
    let fnData = build_string() <| $(writer) {
        coverageData |> get(name) $(funcs) {
            for (fname, entry in keys(funcs.func_cov), values(funcs.func_cov)) {
                writer |> write("FN:{entry.line |> int},{fname}\n")
            }
            for (fname, entry in keys(funcs.func_cov), values(funcs.func_cov)) {
                writer |> write("FNDA:{entry.hits |> int},{fname}\n")
                fnFound += 1
                if (entry.hits > 0u) {
                    fnHit += 1
                }
            }
        }
    }
    var brFound = 0
    var brHit = 0
    let brData = build_string() <| $(writer) {
        coverageData |> get(name) $(branches) {
            for (br in values(branches.branch_cov)) {
                var taken : string
                if (br.hits == 0u) {
                    taken = "-"
                } else {
                    taken = "{br.hits |> int}"
                }
                writer |> write("BRDA:{br.line |> int},{br.block_num |> int},{br.branch_num |> int},{taken}\n")
                brFound += 1
                if (br.hits > 0u) {
                    brHit += 1
                }
            }
        }
    }
    let data = build_string() <| $(writer) {
        coverageData |> get(name) $(x) {
            for (k, v in keys(x.line_cov.lines), values(x.line_cov.lines)) {
                writer |> write("DA:{k |> int},{v |> int}\n")
            }
        }
    }
    return "\nTN:\nSF:{name}\n{fnData}FNF:{fnFound}\nFNH:{fnHit}\n{brData}BRF:{brFound}\nBRH:{brHit}\n{data}end_of_record\n"
}

def get_report(name : string = "") : string {
    //! Returns code coverage report in lcov format for the specified file, or all files if name is empty.
    if (name |> empty) {
        return build_string() <| $(var w) {
            for (k in keys(coverageData)) {
                w |> write(single_file_report(k))
            }
        }
    } else {
        return single_file_report(name)
    }
}

struct FileSummary {
    file : string
    lineHits : uint
    lineMissed : uint
    funcHits : uint
    funcMissed : uint
    branchHits : uint
    branchMissed : uint
}

def get_summary(cov_data) : array<FileSummary> {
    //! Returns per-file coverage summary with hit/missed counts for lines, functions, and branches.
    var res : array<FileSummary>
    for (file, data in keys(cov_data), values(cov_data)) {
        var s = FileSummary(file = string(file))
        for (v in values(data.line_cov.lines)) {
            if (v > 0u) {
                s.lineHits += 1u
            } else {
                s.lineMissed += 1u
            }
        }
        for (fc in values(data.func_cov)) {
            if (fc.hits > 0u) {
                s.funcHits += 1u
            } else {
                s.funcMissed += 1u
            }
        }
        for (br in values(data.branch_cov)) {
            if (br.hits > 0u) {
                s.branchHits += 1u
            } else {
                s.branchMissed += 1u
            }
        }
        res |> push(s)
    }
    return <- res
}

def private pct_string(hits, total : uint) : string {
    if (total == 0u) {
        return "-"
    }
    let p = (hits * 1000u / total + 5u) / 10u
    return "{p:d}%"
}

def private summary_line(var writer : StringBuilderWriter; name : string; nameWidth : int; hits, missed : uint) {
    writer |> write(name)
    writer |> write_chars(' ', nameWidth - name |> length)
    writer |> write("{hits:d}")
    writer |> write_chars(' ', 10 - "{hits:d}" |> length)
    writer |> write("{missed:d}")
    writer |> write_chars(' ', 10 - "{missed:d}" |> length)
    let total = hits + missed
    let pct = pct_string(hits, total)
    writer |> write(pct)
    writer |> write("\n")
}

def public register_coverage_lines(file : string; lines : array<uint>) {
    //! Registers lines as instrumented with 0 hit count (for DA:line,0 in LCOV).
    coverageData |> get_with_default(file) <| $(var cov) {
        for (line in lines) {
            cov.line_cov.lines |> insert(line, 0u)
        }
    }
}

def public register_coverage_func_lines(file : string; fname : string; lines : array<uint>) {
    //! Registers per-function lines with 0 hit count so uncalled functions show missed lines.
    coverageData |> get_with_default(file) <| $(var cov) {
        cov.func_cov |> get_with_default(fname) <| $(var fc) {
            for (line in lines) {
                fc.line_cov.lines |> insert(line, 0u)
            }
        }
    }
}

def public register_coverage_functions(file : string; names : array<string>; lines : array<uint>) {
    //! Registers functions with 0 hit count for FN/FNDA records in LCOV.
    coverageData |> get_with_default(file) <| $(var funcs) {
        for (i in range(names |> length)) {
            funcs.func_cov |> get_with_default(names[i]) <| $(var fc) {
                fc.line = lines[i]
            }
        }
    }
}

def public register_coverage_branches(file : string; blines : array<uint>; block_nums : array<uint>; branch_nums : array<uint>) {
    //! Registers branches with 0 hit count for BRDA records in LCOV.
    coverageData |> get_with_default(file) <| $(var branches) {
        for (i in range(blines |> length)) {
            branches.branch_cov |> insert(i |> uint, BranchCov(line = blines[i], block_num = block_nums[i], branch_num = branch_nums[i], hits = 0u))
        }
    }
}

def public add_line_coverage(file : string; fname : string; line : uint) {
    //! Records a coverage hit for the given file, function, and line number.
    coverageData |> get_with_default(file) <| $(var cov) {
        cov.line_cov.insert(line)
        cov.func_cov |> get_with_default(fname) <| $(var fc) {
            fc.line_cov.insert(line)
        }
    }
}

def public add_func_coverage(file : string; name : string) {
    //! Records a coverage hit for the given file and function name.
    coverageData |> get_with_default(file) <| $(var funcs) {
        funcs.func_cov |> get_with_default(name) <| $(var fc) {
            fc.hits += 1u
        }
    }
}

def public add_branch_coverage(file : string; branch_idx : uint) {
    //! Records a coverage hit for the given file and branch index.
    coverageData |> get_with_default(file) <| $(var branches) {
        branches.branch_cov |> modify(uint(branch_idx)) <| $(value) {
            var res = value
            res.hits += 1u
            return res
        }
    }
}

// compile-time data for function and branch registration
struct RegisterData {
    allFunctions : table<string; uint>
    allBranches : array<tuple<uint; uint; uint>>
    branchCounter : uint
    funcLines : table<string; table<uint>>  // fname -> set of lines
}

var registration_data = table<string; RegisterData>()

class CoverageMacro : AstVisitor {
    //! AST visitor that instruments expression blocks, function entries, and branches with coverage tracking calls.
    astChanged : bool = false

    @do_not_delete func : Function?
    def override preVisitFunction(var fun : FunctionPtr) {
        //! Stores a pointer to the current function being visited and records function info for coverage.
        func = get_ptr(fun)
        if (fun.flags.generated || fun.flags.init || fun.flags.builtIn) {
            return
        }
        if (fun.at.fileInfo == null) {
            return
        }
        let file = resolve_file_path(string(fun.at.fileInfo.name))
        let fname = string(fun.name)
        let line = fun.at.line
        registration_data |> get_with_default(file) <| $(var file_reg) {
            if (!(file_reg.allFunctions |> key_exists(fname))) {
                file_reg.allFunctions |> insert(fname, line)
            }
        }
    }

    def override canVisitFunction(fun : Function?) : bool {
        if (fun.flags.init || fun.flags.builtIn) {
            return false
        }
        for (ann in fun.annotations) {
            if (ann.annotation.name != "marker") {
                continue
            }
            for (arg in ann.arguments) {
                if (arg.name == "no_coverage") {
                    return false
                }
            }
        }
        return true
    }

    def override preVisitFunctionBody(fun : FunctionPtr; expr : ExpressionPtr) {
        //! Inserts function coverage call at the start of each function body.
        if (fun.flags.generated || fun.flags.init || fun.flags.builtIn) {
            return
        }
        if (fun.at.fileInfo == null) {
            return
        }
        if (!(expr is ExprBlock)) {
            return
        }
        let file = resolve_file_path(string(fun.at.fileInfo.name))
        let fname = string(fun.name)
        var blk = unsafe(reinterpret<ExprBlock?> expr)
        var inscope prev : array<ExpressionPtr>
        for (ex in blk.list) {
            prev.emplace(ex)
        }
        blk.list |> clear()
        blk.list |> emplace_new <| qmacro(_::add_func_coverage($v(file), $v(fname)))
        for (el in range(prev |> length)) {
            blk.list |> emplace <| prev[el]
        }
        astChanged = true
        func.not_inferred()
    }

    def override visitExprIfThenElse(var ifte : smart_ptr<ExprIfThenElse>) : ExpressionPtr {
        //! Instruments if/else branches with branch coverage tracking calls.
        // if (ifte.isStatic) {
        //     return <- ifte
        // }
        if (ifte.at.fileInfo == null) {
            return <- ifte
        }
        let file = resolve_file_path(string(ifte.at.fileInfo.name))
        if (file |> ends_with("coverage.das")) {
            return <- ifte
        }
        let line = ifte.at.line
        var block_num : uint
        registration_data |> get_with_default(file) <| $(var file_reg) {
            block_num = file_reg.branchCounter
            file_reg.branchCounter += 1u
        }
        var true_idx : uint
        var false_idx : uint
        registration_data |> get_with_default(file) <| $(var file_reg) {
            true_idx = uint(file_reg.allBranches |> length)
            file_reg.allBranches |> push(tuple(line, block_num, 0u))

            false_idx = uint(file_reg.allBranches |> length)
            file_reg.allBranches |> push(tuple(line, block_num, 1u))
        }
        // instrument true branch
        if (ifte.if_true != null && ifte.if_true is ExprBlock) {
            var true_blk = unsafe(reinterpret<ExprBlock?> ifte.if_true)
            var inscope prevTrue : array<ExpressionPtr>
            for (ex in true_blk.list) {
                prevTrue.emplace(ex)
            }
            true_blk.list |> clear()
            true_blk.list |> emplace_new <| qmacro(_::add_branch_coverage($v(file), $v(true_idx)))
            for (el in range(prevTrue |> length)) {
                true_blk.list |> emplace <| prevTrue[el]
            }
        }
        // instrument false branch
        if (ifte.if_false != null && ifte.if_false is ExprBlock) {
            var false_blk = unsafe(reinterpret<ExprBlock?> ifte.if_false)
            var inscope prevFalse : array<ExpressionPtr>
            for (ex in false_blk.list) {
                prevFalse.emplace(ex)
            }
            false_blk.list |> clear()
            false_blk.list |> emplace_new <| qmacro(_::add_branch_coverage($v(file), $v(false_idx)))
            for (el in range(prevFalse |> length)) {
                false_blk.list |> emplace <| prevFalse[el]
            }
        } elif (ifte.if_false == null) {
            // no else branch — create one with just the branch coverage call
            var inscope elseBlk <- new ExprBlock(at = ifte.at)
            elseBlk.list |> emplace_new <| qmacro(_::add_branch_coverage($v(file), $v(false_idx)))
            move(ifte.if_false) <| elseBlk
        }
        astChanged = true
        func.not_inferred()
        return <- ifte
    }

    def override visitExprBlock(var blk : smart_ptr<ExprBlock>) : ExpressionPtr {
        //! Instruments expression blocks with coverage tracking calls.
        if (blk.at.fileInfo == null) {
            return <- blk
        }
        astChanged = true
        func.not_inferred()
        var inscope prev : array<ExpressionPtr>
        for (ex in blk.list) {
            prev.emplace(ex)
        }
        blk.list |> clear()
        for (el in range(prev |> length)) {
            // Todo: add linter phase to check fileInfo is never null.
            if (prev[el].at.fileInfo != null) {
                let file = resolve_file_path(string(prev[el].at.fileInfo.name))
                if (file |> ends_with("coverage.das")) {
                    blk.list |> emplace <| prev[el]
                    continue
                }
                let fname = string(func.name)
                let line = prev[el].at.line
                coverageData |> get_with_default(file) <| $(var data) {
                    data.line_cov.insert(line)
                }
                registration_data |> get_with_default(file) <| $(var reg) {
                    reg.funcLines |> get_with_default(fname) <| $(var flines) {
                        flines |> insert(line)
                    }
                }
                blk.list |> emplace_new <| qmacro(_::add_line_coverage($v(file), $v(fname), $v(line)))
                blk.list |> emplace <| prev[el]
            }
        }
        return <- blk
    }

    def override preVisitExprAssert(var expr : smart_ptr<ExprAssert>) {
        // Once coverage added all assertions should become verify
        // since side-effects added when calling coverage tools.
        expr.isVerify = true
        astChanged = true
        func.not_inferred()
    }

}

//! Infer pass macro that applies coverage instrumentation to the program.
[infer_macro]
class CoveragePass : AstPassMacro {
    //! Pass macro that instruments modules with code coverage.
    def override apply(prog : ProgramPtr; mod : Module?) : bool {
        //! Applies the coverage instrumentation visitor to the program.
        if (mod.name == "coverage" || (prog._options |> find_arg("no_coverage") ?as tBool ?? false)) {
            return false
        }
        var found = false
        mod |> for_each_function("__coverage_init") <| $(fn) {
            found = true
        }
        if (found) {
            return false
        }
        var covModule : Module?
        program_for_each_module(prog) <| $(m) {
            if (m.name == "coverage") {
                covModule = m
            }
        }
        if (covModule == null) {
            return false
        }
        add_module_require(mod, covModule, false)
        var astVisitor = new CoverageMacro()
        var inscope astVisitorAdapter <- make_visitor(*astVisitor)
        visit_module(prog, astVisitorAdapter, mod)
        var result = astVisitor.astChanged
        if (result) {
            // generate [init] function that registers all instrumented lines with 0 hits
            var inscope initFn <- qmacro_function("__coverage_init") <| $() {
                pass
            }
            var blk = unsafe(reinterpret<ExprBlock?> initFn.body)
            blk.list |> clear()
            for (file, values in keys(coverageData), values(coverageData)) {
                // register lines
                var lineArr : array<uint>
                for (line in keys(values.line_cov.lines)) {
                    lineArr |> push(line)
                }
                blk.list |> emplace_new <| qmacro(_::register_coverage_lines($v(string(file)), $v(lineArr)))
            }
            for (file, reg_data in keys(registration_data), values(registration_data)) {
                // register functions
                var nameArr : array<string>
                var lineArr : array<uint>
                for (fname, fline in keys(reg_data.allFunctions), values(reg_data.allFunctions)) {
                    nameArr |> push(string(fname))
                    lineArr |> push(fline)
                }
                blk.list |> emplace_new <| qmacro(coverage::register_coverage_functions($v(string(file)), $v(nameArr), $v(lineArr)))

                // register per-function lines
                for (fname, flines in keys(reg_data.funcLines), values(reg_data.funcLines)) {
                    var flineArr : array<uint>
                    for (fl in keys(flines)) {
                        flineArr |> push(fl)
                    }
                    blk.list |> emplace_new <| qmacro(coverage::register_coverage_func_lines($v(string(file)), $v(string(fname)), $v(flineArr)))
                }

                // register branches
                let nbranches = reg_data.allBranches |> length()
                var blines : array<uint>
                var block_nums : array<uint>
                var branch_nums : array<uint>
                blines |> reserve(nbranches)
                block_nums |> reserve(nbranches)
                branch_nums |> reserve(nbranches)
                for (i in range(nbranches)) {
                    blines |> push(reg_data.allBranches[i]._0)
                    block_nums |> push(reg_data.allBranches[i]._1)
                    branch_nums |> push(reg_data.allBranches[i]._2)
                }
                blk.list |> emplace_new <| qmacro(coverage::register_coverage_branches($v(string(file)), $v(blines), $v(block_nums), $v(branch_nums)))

            }
            initFn.flags |= FunctionFlags.generated | FunctionFlags.init | FunctionFlags.privateFunction
            compiling_module() |> add_function(initFn)
        }
        unsafe {
            delete astVisitor
        }
        return result
    }
}

def get_coverage_summary(cov_data : table<string; CoverageData>;
                         per_function : bool) {
    //! Returns a human-readable coverage summary table string (line coverage).
    //! When per_function is true, also lists each function with its hit count.
    var summary <- get_summary(cov_data)
    var total_hits : uint
    var total_missed : uint
    var max_len = 4
    for (s in summary) {
        if (s.file |> length > max_len) {
            max_len = s.file |> length
        }
    }
    if (per_function) {
        for (_file, data in keys(cov_data), values(cov_data)) {
            for (fname in keys(data.func_cov)) {
                if (fname |> length > max_len) {
                    max_len = fname |> length
                }
            }
        }
    }
    let w = max_len + 2
    return build_string() <| $(writer) {
        writer |> write("\nCoverage Summary\n")
        writer |> write_chars('=', w + 30)
        writer |> write("\n")
        writer |> write("File")
        writer |> write_chars(' ', w - 4)
        writer |> write("Hits")
        writer |> write_chars(' ', 6)
        writer |> write("Missed")
        writer |> write_chars(' ', 4)
        writer |> write("Coverage\n")
        writer |> write_chars('-', w + 30)
        writer |> write("\n")
        for (s in summary) {
            total_hits += s.lineHits
            total_missed += s.lineMissed
            summary_line(writer, s.file, w, s.lineHits, s.lineMissed)
            if (per_function) {
                cov_data |> get(s.file) $(data) {
                    for (fname, fc in keys(data.func_cov), values(data.func_cov)) {
                        var fLineHits : uint
                        var fLineMissed : uint
                        for (v in values(fc.line_cov.lines)) {
                            if (v > 0u) {
                                fLineHits += 1u
                            } else {
                                fLineMissed += 1u
                            }
                        }
                        if (fLineHits + fLineMissed > 0u) {
                            summary_line(writer, "  {fname}", w, fLineHits, fLineMissed)
                        }
                    }
                }
            }
        }
        writer |> write_chars('-', w + 30)
        writer |> write("\n")
        summary_line(writer, "TOTAL", w, total_hits, total_missed)
    }
}

def is_excluded(file : string; exclude_prefixes : array<string>) : bool {
    for (prefix in exclude_prefixes) {
        if (file |> starts_with(prefix)) {
            return true
        }
    }
    return false
}


def parse_lcov(lcov_path : string; exclude_prefixes : array<string>) : table<string; CoverageData> {
    // Parses an LCOV file (possibly containing multiple records for the same
    // source file), populates coverageData, and calls get_coverage_summary.
    var currentFile = ""
    var skipFile = false
    // Track function start lines per file for assigning DA lines to functions.
    var funcStarts : table<string; array<tuple<uint; string>>>
    var result = table<string; CoverageData>()
    fopen(lcov_path, "rb") <| $(f) {
        if (f == null) {
            print("Failed to open {lcov_path}\n")
            return
        }
        while (!feof(f)) {
            let line = fgets(f) |> strip()
            if (line |> empty() || line == "end_of_record" || line == "TN:") {
                continue
            }
            if (line |> starts_with("SF:")) {
                currentFile = slice(line, 3)
                skipFile = is_excluded(currentFile, exclude_prefixes)
                continue
            }
            if (skipFile) {
                continue
            }
            result |> get_with_default(currentFile) <| $(var cov) {
                if (line |> starts_with("FN:")) {
                    // FN:line,name
                    let rest = slice(line, 3)
                    let comma = find(rest, ",")
                    if (comma >= 0) {
                        let fline = uint(to_int(slice(rest, 0, comma)))
                        let fname = slice(rest, comma + 1)
                        cov.func_cov |> get_with_default(fname) <| $(var fc) {
                            fc.line = fline
                        }
                        funcStarts |> get_with_default(currentFile) <| $(var starts) {
                            starts |> push(tuple(fline, string(fname)))
                        }
                    }
                } elif (line |> starts_with("FNDA:")) {
                    // FNDA:hit_count,function_name
                    let rest = slice(line, 5)
                    let comma = find(rest, ",")
                    if (comma >= 0) {
                        let hits = uint(to_int(slice(rest, 0, comma)))
                        let fname = slice(rest, comma + 1)
                        cov.func_cov |> get_with_default(fname) <| $(var fc) {
                            fc.hits += hits
                        }
                    }
                } elif (line |> starts_with("DA:")) {
                    // DA:line_number,hit_count
                    let rest = slice(line, 3)
                    let comma = find(rest, ",")
                    if (comma >= 0) {
                        let ln = uint(to_int(slice(rest, 0, comma)))
                        let hits = uint(to_int(slice(rest, comma + 1)))
                            if (cov.line_cov.lines |> key_exists(ln)) {
                                unsafe(cov.line_cov.lines[ln]) += hits
                            } else {
                                cov.line_cov.lines |> insert(ln, hits)
                            }
                    }
                } elif (line |> starts_with("BRDA:")) {
                    // BRDA:line,block,branch,taken
                    let rest = slice(line, 5)
                    let last_comma = rfind(rest, ",")
                    if (last_comma >= 0) {
                        let taken = slice(rest, last_comma + 1)
                        let hits = taken == "-" ? 0u : uint(to_int(taken))
                        // parse line,block,branch from the key part
                        let key_part = slice(rest, 0, last_comma)
                        let c1 = find(key_part, ",")
                        if (c1 >= 0) {
                            let br_line = uint(to_int(slice(key_part, 0, c1)))
                            let rest2 = slice(key_part, c1 + 1)
                            let c2 = find(rest2, ",")
                            if (c2 >= 0) {
                                let block_num = uint(to_int(slice(rest2, 0, c2)))
                                let branch_num = uint(to_int(slice(rest2, c2 + 1)))
                                let idx = uint(cov.branch_cov |> length)
                                cov.branch_cov |> insert(idx, BranchCov(
                                    line = br_line,
                                    block_num = block_num,
                                    branch_num = branch_num,
                                    hits = hits
                                ))
                            }
                        }
                    }
                }
            }
        }
    }
    // Sort function starts and assign DA lines to functions for per-function line coverage.
    for (file, starts in keys(funcStarts), values(funcStarts)) {
        sort(starts) <| $(a, b) {
            return a._0 < b._0
        }
        result |> get(file) <| $(var cov) {
            for (ln, hits in keys(cov.line_cov.lines), values(cov.line_cov.lines)) {
                // Find function with largest start line <= ln.
                var bestFunc = ""
                for (s in starts) {
                    if (s._0 <= ln) {
                        bestFunc = s._1
                    } else {
                        break
                    }
                }
                if (!bestFunc |> empty()) {
                    cov.func_cov |> get(bestFunc) <| $(var fc) {
                        if (fc.line_cov.lines |> key_exists(ln)) {
                            unsafe(fc.line_cov.lines[ln]) += hits
                        } else {
                            fc.line_cov.lines |> insert(ln, hits)
                        }
                    }
                }
            }
        }
    }
    return <- result
}

// ==========================================================================
// Public API
// ==========================================================================

// Assigns to str coverage summary
[export]
def public fill_coverage_summary(var str : string?; per_function : bool = false) {
    *str = get_coverage_summary(coverageData, per_function)
}


// Assigns to `dest` coverage in `lcov` format
[export]
def public fill_coverage_report(var dest : string?; name : string = "") {
    //! Fills the string at dest with the coverage report. Used for cross-context data transfer.
    *dest = get_report(name)
}

// Returns all recorded coverage table to this moment.
[export]
def public get_coverage_details(var cov : table<string; CoverageData>?) {
    return <- clone(coverageData)
}

[export]
def public summary_from_lcov_file(filename : string; exclude : array<string>) {
    let parsed_cov = parse_lcov(filename, exclude)
    return get_coverage_summary(parsed_cov, true)
}
