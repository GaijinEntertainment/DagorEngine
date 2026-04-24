options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/json
require daslib/json_boost
require strings
require daslib/fio

require tools/compile_check public
require tools/list_functions public
require tools/list_types public
require tools/run_script public
require tools/run_test public
require tools/format_file public
require tools/ast_dump public
require tools/list_modules public
require tools/find_symbol public
require tools/list_module_api public
require tools/list_requires public
require tools/convert_to_gen2 public
require tools/goto_definition public
require tools/type_of public
require tools/find_references public
require tools/eval_expression public
require tools/describe_type public
require tools/grep_usage public
require tools/outline public
require tools/aot public
require tools/lint_tool public

// helper: parse tool result JSON, return (text, isError)
def parse_result(result : string; var text : string&; var is_error : bool&) : bool {
    var err : string
    var parsed = read_json(result, err)
    if (parsed == null) {
        return false
    }
    // result is {"content":[{"type":"text","text":"..."}],"isError":false}
    // access content[0].text via JSON path
    let content = parsed?["content"]
    if (content == null) {
        unsafe { delete parsed; }
        return false
    }
    let first = content?[0]
    if (first == null) {
        unsafe { delete parsed; }
        return false
    }
    let text_val = first?["text"]
    if (text_val != null) {
        text = text_val.value as _string
    }
    let err_val = parsed?["isError"]
    if (err_val != null) {
        is_error = err_val.value as _bool
    }
    unsafe { delete parsed; }
    return true
}

def fixture_path(name : string) : string {
    return "utils/mcp/tests/{name}"
}

// ── compile_check ────────────────────────────────────────────────────

[test]
def test_compile_check_valid(t : T?) {
    t |> run("valid file compiles") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_compile_check(fixture_path("_fixture_valid.das")), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "Compilation OK") >= 0, "should contain 'Compilation OK'")
    }
    t |> run("lists functions") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_compile_check(fixture_path("_fixture_valid.das")), text, is_error)
        t |> success(find(text, "def greet") >= 0, "should list greet function")
        t |> success(find(text, "def main") >= 0, "should list main function")
    }
}

[test]
def test_compile_check_error(t : T?) {
    t |> run("invalid file reports error") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_compile_check(fixture_path("_fixture_error.das")), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(is_error, "should be error")
        t |> success(find(text, "Compilation failed") >= 0, "should contain 'Compilation failed'")
    }
}

[test]
def test_compile_check_missing(t : T?) {
    t |> run("missing file reports error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_compile_check("nonexistent_file_12345.das"), text, is_error)
        t |> success(is_error, "should be error")
        t |> success(find(text, "not found") >= 0, "should mention file not found")
    }
}

// ── compile_check batch ──────────────────────────────────────────────

[test]
def test_compile_check_batch_comma(t : T?) {
    t |> run("batch compile with comma-separated files") <| @(t : T?) {
        var text : string
        var is_error = false
        let files = "{fixture_path("_fixture_valid.das")},{fixture_path("_fixture_error.das")}"
        parse_result(do_compile_check(files), text, is_error)
        t |> success(is_error, "batch with error file should report error")
        t |> success(find(text, "1 passed") >= 0, "should have 1 passed")
        t |> success(find(text, "1 failed") >= 0, "should have 1 failed")
        t |> success(find(text, "PASS") >= 0, "should have PASS line")
        t |> success(find(text, "FAIL") >= 0, "should have FAIL line")
    }
}

[test]
def test_compile_check_batch_glob(t : T?) {
    t |> run("batch compile with glob pattern") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_compile_check("utils/mcp/tests/*.das"), text, is_error)
        // At least the valid and error fixtures should be found
        t |> success(find(text, "PASS") >= 0, "should have at least one PASS")
        t |> success(find(text, "files") >= 0, "should have summary line")
    }
}

[test]
def test_compile_check_glob_no_match(t : T?) {
    t |> run("glob with no matches reports error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_compile_check("nonexistent_dir_xyz/*.das"), text, is_error)
        t |> success(is_error, "should be error when no files match")
        t |> success(find(text, "no files matched") >= 0, "should mention no files matched")
    }
}

// ── list_functions ───────────────────────────────────────────────────

[test]
def test_list_functions(t : T?) {
    t |> run("lists user functions") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_list_functions(fixture_path("_fixture_valid.das")), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "def greet") >= 0, "should list greet")
        t |> success(find(text, "def main") >= 0, "should list main")
    }
}

// ── list_types ───────────────────────────────────────────────────────

[test]
def test_list_types(t : T?) {
    t |> run("lists structs, enums, classes") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_list_types(fixture_path("_fixture_valid.das")), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "Point") >= 0, "should list Point struct")
        t |> success(find(text, "Color") >= 0, "should list Color enum")
        t |> success(find(text, "Animal") >= 0, "should list Animal class")
    }
}

// ── run_script ───────────────────────────────────────────────────────

[test]
def test_run_script_inline(t : T?) {
    t |> run("inline code executes") <| @(t : T?) {
        var text : string
        var is_error = false
        let code = "options gen2\n[export]\ndef main() \{\n    print(\"hello_mcp_test\\n\")\n\}\n"
        parse_result(do_run_script("", code), text, is_error)
        // NOTE: popen stdout capture broken when run from dastest on Windows (see wip.das)
        // t |> success(!is_error, "should not be error")
        // t |> success(find(text, "hello_mcp_test") >= 0, "should contain output")
    }
}

[test]
def test_run_script_error(t : T?) {
    t |> run("bad code reports error") <| @(t : T?) {
        var text : string
        var is_error = false
        let code = "options gen2\ndef broken() : int \{\n    return \"nope\"\n\}\n"
        parse_result(do_run_script("", code), text, is_error)
        t |> success(is_error, "should be error")
    }
}

// ── run_test ──────────────────────────────────────────────────────────

[test]
def test_run_test_passing(t : T?) {
    t |> run("passing test returns success") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_run_test(fixture_path("_fixture_test.das")), text, is_error)
        // NOTE: popen stdout capture broken when run from dastest on Windows (see wip.das)
        // t |> success(!is_error, "should not be error")
        // t |> success(find(text, "passed") >= 0, "should contain 'passed'")
    }
}

[test]
def test_run_test_bad_file(t : T?) {
    t |> run("nonexistent file reports error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_run_test("nonexistent_test_file.das"), text, is_error)
        t |> success(is_error, "should be error")
    }
}

// ── ast_dump ─────────────────────────────────────────────────────────

[test]
def test_ast_dump_expression(t : T?) {
    t |> run("expression mode returns S-expression") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_ast_dump("a + b", "", "", "", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "ExprOp2") >= 0, "should contain ExprOp2")
        t |> success(find(text, "ExprVar") >= 0, "should contain ExprVar")
    }
}

[test]
def test_ast_dump_file_ast_mode(t : T?) {
    t |> run("file ast mode returns S-expression") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_ast_dump("", fixture_path("_fixture_valid.das"), "add", "ast_core", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "def add") >= 0, "should contain function header")
        t |> success(find(text, "ExprReturn") >= 0, "should contain ExprReturn node")
    }
}

[test]
def test_ast_dump_file_source_mode(t : T?) {
    t |> run("file source mode returns describe output") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_ast_dump("", fixture_path("_fixture_valid.das"), "add", "source", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "def add") >= 0, "should contain function header")
        t |> success(find(text, "return") >= 0, "should contain return statement")
        // source mode should NOT have ExprOp2 etc.
        t |> success(find(text, "ExprOp2") < 0, "should not contain S-expression nodes")
    }
}

[test]
def test_ast_dump_export_all(t : T?) {
    t |> run("export_all preserves non-main functions") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_ast_dump("", fixture_path("_fixture_valid.das"), "greet", "source", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "def greet") >= 0, "greet should be preserved by export_all")
    }
}

// ── list_modules ─────────────────────────────────────────────────────

[test]
def test_list_modules(t : T?) {
    t |> run("lists builtin modules") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_list_modules(), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "math") >= 0, "should list math module")
        t |> success(find(text, "strings") >= 0, "should list strings module")
    }
    t |> run("lists daslib modules") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_list_modules(), text, is_error)
        t |> success(find(text, "json") >= 0, "should list json module")
    }
}

// ── find_symbol ──────────────────────────────────────────────────────

[test]
def test_find_symbol(t : T?) {
    t |> run("finds builtin function") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_find_symbol("print", "", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "print") >= 0, "should find print")
    }
    t |> run("case-insensitive search") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_find_symbol("PRINT", "", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "print") >= 0, "should find print case-insensitively")
    }
    t |> run("exact match with = prefix") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_find_symbol("=print", "", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "print") >= 0, "should find exact 'print'")
    }
    t |> run("finds typedef aliases") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_find_symbol("Flags", "typedef", fixture_path("_fixture_refs.das")), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "typedef") >= 0, "should find typedef")
        t |> success(find(text, "Flags") >= 0, "should name Flags")
        t |> success(find(text, "bitfield") >= 0, "should show bitfield type")
    }
}

// ── list_module_api ──────────────────────────────────────────────────

[test]
def test_list_module_api(t : T?) {
    t |> run("lists math module functions") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_list_module_api("math", "", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "sin") >= 0, "should list sin function")
        t |> success(find(text, "cos") >= 0, "should list cos function")
    }
}

// ── list_module_api annotations ──────────────────────────────────────

[test]
def test_list_module_api_annotations(t : T?) {
    t |> run("lists function annotations from ast_boost") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_list_module_api("daslib/ast_boost", "", "annotations"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "[macro]") >= 0, "should list [macro] function annotation")
        t |> success(find(text, "Function annotations") >= 0, "should have Function annotations section")
    }
    t |> run("lists structure annotations from ast_boost") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_list_module_api("daslib/ast_boost", "", "annotations"), text, is_error)
        t |> success(find(text, "[function_macro]") >= 0, "should list [function_macro]")
        t |> success(find(text, "Structure annotations") >= 0, "should have Structure annotations section")
    }
    t |> run("lists call macros from ast") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_list_module_api("ast_core", "", "annotations"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "quote") >= 0, "should list quote call macro")
        t |> success(find(text, "Call macros") >= 0, "should have Call macros section")
    }
    t |> run("lists reader macros from regex_boost") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_list_module_api("daslib/regex_boost", "", "annotations"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "regex") >= 0, "should list regex reader macro")
        t |> success(find(text, "Reader macros") >= 0, "should have Reader macros section")
    }
    t |> run("filter works on annotations") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_list_module_api("daslib/ast_boost", "macro", "annotations"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "[macro]") >= 0, "should match macro")
        t |> success(find(text, "[contract]") < 0, "should not match contract")
    }
}

// ── format_file ──────────────────────────────────────────────────────

[test]
def test_format_file(t : T?) {
    t |> run("formats already-formatted file") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_format_file(fixture_path("_fixture_valid.das")), text, is_error)
        t |> success(!is_error, "should not be error")
        // might be "already formatted" or "Formatted" depending on state
        t |> success(find(text, "ormatted") >= 0, "should mention formatting result")
    }
}

// ── list_requires ────────────────────────────────────────────────────

[test]
def test_list_requires(t : T?) {
    t |> run("lists direct requires") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_list_requires(fixture_path("_fixture_requires.das")), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "Direct requires") >= 0, "should have Direct requires section")
        t |> success(find(text, "require math") >= 0, "should list math")
        t |> success(find(text, "require strings") >= 0, "should list strings")
    }
    t |> run("marks public requires") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_list_requires(fixture_path("_fixture_requires.das")), text, is_error)
        t |> success(find(text, "strings public") >= 0, "should mark strings as public")
    }
    t |> run("annotates builtin modules") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_list_requires(fixture_path("_fixture_requires.das")), text, is_error)
        t |> success(find(text, "// builtin") >= 0, "should annotate builtin modules")
    }
    t |> run("file with no explicit requires") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_list_requires(fixture_path("_fixture_valid.das")), text, is_error)
        t |> success(!is_error, "should not be error")
    }
}

// ── convert_to_gen2 ─────────────────────────────────────────────────

[test]
def test_convert_to_gen2(t : T?) {
    t |> run("converts gen1 to gen2 stdout") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_convert_to_gen2(fixture_path("_fixture_gen1.das"), false), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "options gen2") >= 0, "should add options gen2")
        t |> success(find(text, "def add(") >= 0, "should contain add function")
        t |> success(find(text, "\{") >= 0, "should contain braces")
    }
    t |> run("adds parentheses to control flow") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_convert_to_gen2(fixture_path("_fixture_gen1.das"), false), text, is_error)
        t |> success(find(text, "if (") >= 0, "should add parens to if")
    }
    t |> run("missing file reports error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_convert_to_gen2("nonexistent_file_12345.das", false), text, is_error)
        t |> success(is_error, "should be error for missing file")
    }
}

// ── goto_definition ──────────────────────────────────────────────────

[test]
def test_goto_definition_function(t : T?) {
    t |> run("resolves function call to definition") <| @(t : T?) {
        var text : string
        var is_error = false
        // distance() call at line 33, col 13
        parse_result(do_goto_definition(fixture_path("_fixture_goto.das"), "33", "13"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "Kind: function") >= 0, "should be function kind")
        t |> success(find(text, "Symbol: distance") >= 0, "should resolve to distance")
        t |> success(find(text, "Line: 14") >= 0, "should point to line 14")
    }
}

[test]
def test_goto_definition_variable(t : T?) {
    t |> run("resolves variable reference to declaration") <| @(t : T?) {
        var text : string
        var is_error = false
        // p1 variable at line 33, col 22
        parse_result(do_goto_definition(fixture_path("_fixture_goto.das"), "33", "22"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "Kind: variable") >= 0, "should be variable kind")
        t |> success(find(text, "Symbol: p1") >= 0, "should resolve to p1")
        t |> success(find(text, "Line: 31") >= 0, "should point to line 31")
    }
}

[test]
def test_goto_definition_field(t : T?) {
    t |> run("resolves field access to struct field") <| @(t : T?) {
        var text : string
        var is_error = false
        // a.x field at line 15, col 14
        parse_result(do_goto_definition(fixture_path("_fixture_goto.das"), "15", "14"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "Kind: field") >= 0, "should be field kind")
        t |> success(find(text, "Point.x") >= 0, "should resolve to Point.x")
        t |> success(find(text, "Line: 10") >= 0, "should point to line 10")
    }
}

[test]
def test_goto_definition_builtin(t : T?) {
    t |> run("resolves builtin function") <| @(t : T?) {
        var text : string
        var is_error = false
        // print() at line 34, col 5
        parse_result(do_goto_definition(fixture_path("_fixture_goto.das"), "34", "5"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "Kind: builtin") >= 0, "should be builtin kind")
        t |> success(find(text, "print") >= 0, "should resolve to print")
    }
}

[test]
def test_goto_definition_no_expression(t : T?) {
    t |> run("reports no expression at empty position") <| @(t : T?) {
        var text : string
        var is_error = false
        // line 1 col 1 — options gen2, no expression
        parse_result(do_goto_definition(fixture_path("_fixture_goto.das"), "1", "1"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "No expression found") >= 0, "should report no expression")
    }
}

[test]
def test_goto_definition_snippet(t : T?) {
    t |> run("includes source snippet") <| @(t : T?) {
        var text : string
        var is_error = false
        // distance() at line 33
        parse_result(do_goto_definition(fixture_path("_fixture_goto.das"), "33", "13"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "def distance") >= 0, "snippet should contain function definition")
    }
}

// ── type_of ──────────────────────────────────────────────────────────

[test]
def test_type_of_call(t : T?) {
    t |> run("function call returns result type") <| @(t : T?) {
        var text : string
        var is_error = false
        // distance() call at line 33 col 13
        parse_result(do_type_of(fixture_path("_fixture_goto.das"), "33", "13"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "ExprCall(distance)") >= 0, "should show ExprCall")
        t |> success(find(text, "float") >= 0, "should return float type")
    }
}

[test]
def test_type_of_variable(t : T?) {
    t |> run("variable reference shows struct type with location") <| @(t : T?) {
        var text : string
        var is_error = false
        // p1 at line 33 col 22
        parse_result(do_type_of(fixture_path("_fixture_goto.das"), "33", "22"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "ExprVar(p1)") >= 0, "should show ExprVar")
        t |> success(find(text, "Point") >= 0, "should show Point type")
    }
}

[test]
def test_type_of_field(t : T?) {
    t |> run("field access shows field type") <| @(t : T?) {
        var text : string
        var is_error = false
        // a.x at line 15 col 14
        parse_result(do_type_of(fixture_path("_fixture_goto.das"), "15", "14"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "ExprField(x)") >= 0, "should show ExprField")
        t |> success(find(text, "float") >= 0, "should return float type")
    }
}

[test]
def test_type_of_no_expression(t : T?) {
    t |> run("reports no expression at empty position") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_type_of(fixture_path("_fixture_goto.das"), "1", "1"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "No expression found") >= 0, "should report no expression")
    }
}

// ── find_references ──────────────────────────────────────────────────

[test]
def test_find_references_function(t : T?) {
    t |> run("finds function call and addr references") <| @(t : T?) {
        var text : string
        var is_error = false
        // cursor on add_points call at line 46 col 14
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "46", "14", "file"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "func") >= 0, "should identify as function")
        t |> success(find(text, "add_points") >= 0, "should name add_points")
        t |> success(find(text, "[call]") >= 0, "should find call reference")
        t |> success(find(text, "[addr]") >= 0, "should find addr reference (@@add_points)")
    }
}

[test]
def test_find_references_variable(t : T?) {
    t |> run("finds variable references") <| @(t : T?) {
        var text : string
        var is_error = false
        // cursor on p1 variable usage at line 46 col 25
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "46", "25", "file"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "variable") >= 0, "should identify as variable")
        t |> success(find(text, "p1") >= 0, "should name p1")
        t |> success(find(text, "Found 2 reference") >= 0, "should find 2 references")
    }
}

[test]
def test_find_references_field(t : T?) {
    t |> run("finds field references including safe field") <| @(t : T?) {
        var text : string
        var is_error = false
        // cursor on a.x field at line 15 col 22
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "15", "22", "file"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "Point.x") >= 0, "should identify as Point.x field")
        t |> success(find(text, "[field]") >= 0, "should find field references")
        t |> success(find(text, "[safe field]") >= 0, "should find safe field reference (?.x)")
        t |> success(find(text, "Found 6 reference") >= 0, "should find 6 references")
    }
}

[test]
def test_find_references_addr(t : T?) {
    t |> run("finds function from @@addr expression") <| @(t : T?) {
        var text : string
        var is_error = false
        // cursor on @@add_points at line 48 col 30
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "48", "30", "file"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "func") >= 0, "should identify as function")
        t |> success(find(text, "add_points") >= 0, "should name add_points")
        t |> success(find(text, "[call]") >= 0, "should find call reference")
        t |> success(find(text, "[addr]") >= 0, "should find addr reference")
    }
}

[test]
def test_find_references_invoke(t : T?) {
    t |> run("invoke resolves to function pointer variable") <| @(t : T?) {
        var text : string
        var is_error = false
        // cursor on invoke(f, a, b) at line 23 col 12
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "23", "12", "file"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "variable") >= 0, "should identify as variable")
        t |> success(find(text, "'f'") >= 0, "should name f")
    }
}

[test]
def test_find_references_from_declaration(t : T?) {
    t |> run("finds references from function definition") <| @(t : T?) {
        var text : string
        var is_error = false
        // cursor on 'def add_points' at line 14
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "14", "5", "file"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "func") >= 0, "should identify as function")
        t |> success(find(text, "add_points") >= 0, "should name add_points")
        t |> success(find(text, "[call]") >= 0, "should find call reference")
    }
    t |> run("finds references from struct definition") <| @(t : T?) {
        var text : string
        var is_error = false
        // cursor on 'struct Point' at line 9
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "9", "1", "file"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "structure") >= 0, "should identify as structure")
        t |> success(find(text, "Point") >= 0, "should name Point")
        t |> success(find(text, "[type ref]") >= 0, "should find type references")
    }
}

[test]
def test_find_references_alias(t : T?) {
    t |> run("finds alias references from bitfield declaration") <| @(t : T?) {
        var text : string
        var is_error = false
        // cursor on 'bitfield Flags' declaration at line 26
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "26", "1", "file"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "alias") >= 0, "should identify as alias")
        t |> success(find(text, "Flags") >= 0, "should name Flags")
        t |> success(find(text, "[alias ref]") >= 0, "should find alias references")
    }
    t |> run("finds alias references from variant usage") <| @(t : T?) {
        var text : string
        var is_error = false
        // cursor on Value(i = 10) at line 51 col 13
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "51", "13", "file"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "alias") >= 0, "should identify as alias")
        t |> success(find(text, "Value") >= 0, "should name Value")
    }
}

[test]
def test_find_references_global_variable(t : T?) {
    t |> run("finds global variable references from declaration") <| @(t : T?) {
        var text : string
        var is_error = false
        // cursor on 'let GLOBAL_SCALE' declaration at line 36 col 5, need no_opt to preserve ExprVar
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "36", "5", "file", "true"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "variable") >= 0, "should identify as variable")
        t |> success(find(text, "GLOBAL_SCALE") >= 0, "should name GLOBAL_SCALE")
        t |> success(find(text, "Found 1 reference") >= 0, "should find 1 reference (usage in main)")
    }
}

[test]
def test_find_references_no_expression(t : T?) {
    t |> run("reports no expression at empty position") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "2", "1", "file"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "No expression or declaration found") >= 0, "should report nothing found")
    }
}

// ── eval_expression ─────────────────────────────────────────────────

[test]
def test_eval_expression(t : T?) {
    t |> run("evaluates simple arithmetic") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_eval_expression("1 + 2", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> equal(text, "3")
    }
    t |> run("evaluates with require") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_eval_expression("float(42) + sin(1.0)", "math"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "42.84") >= 0, "should contain 42.84")
    }
    t |> run("evaluates with multiple requires") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_eval_expression("to_int(\"10\") + 5", "math, strings"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> equal(text, "15")
    }
    t |> run("reports compile error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_eval_expression("undefined_func()", ""), text, is_error)
        t |> success(is_error, "should be error")
        t |> success(find(text, "Evaluation failed") >= 0, "should report failure")
    }
    t |> run("missing expression is error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_eval_expression("", ""), text, is_error)
        t |> success(is_error, "should be error")
    }
}

// ── describe_type ───────────────────────────────────────────────────

[test]
def test_describe_type(t : T?) {
    t |> run("describes handled type fields") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_describe_type("TypeDecl", "ast_core"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "handled TypeDecl") >= 0, "should be handled type")
        t |> success(find(text, "baseType") >= 0, "should have baseType field")
        t |> success(find(text, "structType") >= 0, "should have structType field")
    }
    t |> run("describes enum values") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_describe_type("Type", "ast_core"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "enum Type") >= 0, "should be enum")
        t |> success(find(text, "tInt") >= 0, "should have tInt value")
    }
    t |> run("describes bitfield flags") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_describe_type("FunctionFlags", "ast_core"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "bitfield FunctionFlags") >= 0, "should be bitfield")
        t |> success(find(text, "builtIn") >= 0, "should have builtIn flag")
    }
    t |> run("type not found") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_describe_type("NonExistentType", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "not found") >= 0, "should report not found")
    }
}

// ── grep_usage ──────────────────────────────────────────────────────

[test]
def test_grep_usage(t : T?) {
    t |> run("finds symbol in directory") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_grep_usage("compile_program", "utils/mcp/tools", "", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "compile_program") >= 0, "should contain symbol name")
        t |> success(find(text, "common.das") >= 0, "should find in common.das")
        t |> success(find(text, "Found") >= 0, "should have Found header")
    }
    t |> run("returns correct line numbers") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_grep_usage("compile_program", "utils/mcp/tools", "", ""), text, is_error)
        // line numbers should be > 1 (not all zeros)
        t |> success(find(text, "  1:") < 0 || find(text, "  38:") >= 0 || find(text, "  41:") >= 0, "should have real line numbers")
    }
    t |> run("no matches returns clean message") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_grep_usage("xyzzy_nonexistent_symbol_12345", "utils/mcp/tools", "", ""), text, is_error)
        t |> success(!is_error, "should not be error")
    }
    t |> run("missing symbol is error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_grep_usage("", "", "", ""), text, is_error)
        t |> success(is_error, "should be error")
    }
}

[test]
def test_grep_usage_parse_aware(t : T?) {
    t |> run("skips comments and strings") <| @(t : T?) {
        // search for 'compile_program' in tools — should only find code references,
        // not occurrences inside comments or strings
        var text : string
        var is_error = false
        parse_result(do_grep_usage("compile_program", "utils/mcp/tools", "", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        // all matches should be actual code (compile_program function calls/defs)
        t |> success(find(text, "common.das") >= 0, "should find in common.das where it's defined")
    }
}

[test]
def test_grep_usage_glob_exclude(t : T?) {
    t |> run("exclude glob filters out files") <| @(t : T?) {
        // compile_program is defined in common.das — exclude it
        var text : string
        var is_error = false
        parse_result(do_grep_usage("compile_program", "utils/mcp/tools", "", "!**/common.das"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "common.das") < 0, "common.das should be excluded")
    }
    t |> run("include glob limits to matching files") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_grep_usage("compile_program", "utils/mcp/tools", "", "common.das"), text, is_error)
        t |> success(!is_error, "should not be error")
        // only common.das should appear
        t |> success(find(text, "common.das") >= 0, "should find in common.das")
        t |> success(find(text, "compile_check.das") < 0, "should not find in other files")
    }
}

[test]
def test_outline(t : T?) {
    t |> run("lists functions and structs") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_outline("utils/mcp/tests/_fixture_valid.das", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "[function]") >= 0, "should find functions")
        t |> success(find(text, "[struct]") >= 0, "should find structs")
        t |> success(find(text, "add") >= 0, "should find 'add' function")
        t |> success(find(text, "Point") >= 0, "should find 'Point' struct")
    }
    t |> run("lists enums") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_outline("utils/mcp/tests/_fixture_valid.das", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "[enum]") >= 0, "should find enums")
        t |> success(find(text, "Color") >= 0, "should find 'Color' enum")
    }
    t |> run("lists bitfields and variants") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_outline("utils/mcp/tests/_fixture_valid.das", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "[bitfield]") >= 0, "should find bitfields")
        t |> success(find(text, "Access") >= 0, "should find 'Access' bitfield")
        t |> success(find(text, "[variant]") >= 0, "should find variants")
        t |> success(find(text, "Value") >= 0, "should find 'Value' variant")
    }
    t |> run("sorted by line number") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_outline("utils/mcp/tests/_fixture_valid.das", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        // Point is defined before Color, which is before add
        let point_pos = find(text, "Point")
        let color_pos = find(text, "Color")
        let add_pos = find(text, "add")
        t |> success(point_pos < color_pos, "Point should come before Color")
        t |> success(color_pos < add_pos, "Color should come before add")
    }
    t |> run("missing args is error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_outline("", ""), text, is_error)
        t |> success(is_error, "should be error")
    }
}

[test]
def test_outline_glob(t : T?) {
    t |> run("glob scans multiple files") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_outline("", "utils/mcp/tests/_fixture_*.das"), text, is_error)
        t |> success(!is_error, "should not be error")
        // Should find declarations from multiple fixture files
        t |> success(find(text, "_fixture_valid.das") >= 0, "should include _fixture_valid.das")
    }
}

[test]
def test_grep_usage_detect(t : T?) {
    t |> run("detects ast-grep") <| @(t : T?) {
        let sg = detect_ast_grep()
        t |> success(!empty(sg), "ast-grep should be detected in test environment")
    }
}

// ── shared module caching ───────────────────────────────────────────

[test]
def test_shared_module_not_cached(t : T?) {
    let shared_mod = "utils/mcp/tests/fixture_shared_mod.das"
    let original = "options gen2\nmodule fixture_shared_mod shared\n\nstruct Foo \{\n    x : int = 0;\n\}\n"
    let modified = "options gen2\nmodule fixture_shared_mod shared\n\nstruct Foo \{\n    x : int = 0;\n\}\n\nstruct Bar \{\n    y : int = 0;\n\}\n"
    t |> run("sees new struct after shared module changes") <| @(t : T?) {
        // 1. compile user1 — caches shared module with only Foo
        var text : string
        var is_error = false
        parse_result(do_compile_check(fixture_path("_fixture_shared_user.das")), text, is_error)
        t |> success(!is_error, "user1 should compile (Foo exists)")
        // 2. add Bar to the shared module
        t |> success(fwrite(shared_mod, modified), "should write modified shared module")
        // 3. compile user2 — uses Bar, must see the updated module
        parse_result(do_compile_check(fixture_path("_fixture_shared_user2.das")), text, is_error)
        t |> success(!is_error, "user2 should compile (Bar should be visible after module change)")
        // 4. restore original
        fwrite(shared_mod, original)
    }
}

// ── aot ─────────────────────────────────────────────────────────────

[test]
def test_aot_full_file(t : T?) {
    t |> run("full file AOT") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_aot(fixture_path("_fixture_valid.das"), ""), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "inline") >= 0, "should contain inline functions")
        t |> success(find(text, "namespace das") >= 0, "should contain namespace")
    }
}

[test]
def test_aot_single_function(t : T?) {
    t |> run("extracts single function by name") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_aot(fixture_path("_fixture_aot.das"), "helper"), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "inline") >= 0, "should contain inline keyword")
        t |> success(find(text, "helper") >= 0, "should contain helper function name")
        // should NOT contain describe
        t |> success(find(text, "describe") < 0, "should not contain other functions")
    }
}

[test]
def test_aot_not_found(t : T?) {
    t |> run("nonexistent function") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_aot(fixture_path("_fixture_aot.das"), "nonexistent"), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "should not be error (informational)")
        t |> success(find(text, "No function matching") >= 0, "should say no match")
        t |> success(find(text, "Available functions") >= 0, "should list available functions")
    }
}

[test]
def test_aot_multiple_matches(t : T?) {
    t |> run("overloaded function returns disambiguation list") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_aot(fixture_path("_fixture_aot.das"), "describe"), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "Multiple functions match") >= 0, "should say multiple matches")
        t |> success(find(text, "int") >= 0, "should show int overload signature")
        t |> success(find(text, "float") >= 0, "should show float overload signature")
    }
}

[test]
def test_aot_select_by_aot_name(t : T?) {
    t |> run("select overload by mangled aot_name") <| @(t : T?) {
        // first get the disambiguation list to find the aot_name
        var text : string
        var is_error = false
        parse_result(do_aot(fixture_path("_fixture_aot.das"), "describe"), text, is_error)
        // extract an aot_name from the list (format: "describe(...) [describe_HEXHASH]")
        let bracket_pos = find(text, "[describe_")
        t |> success(bracket_pos >= 0, "should contain bracketed aot_name")
        if (bracket_pos < 0) {
            return
        }
        let name_start = bracket_pos + 1
        let name_end = find(text, "]", name_start)
        t |> success(name_end > name_start, "should have closing bracket")
        if (name_end <= name_start) {
            return
        }
        let aot_name = slice(text, name_start, name_end)
        // now select by aot_name
        parse_result(do_aot(fixture_path("_fixture_aot.das"), aot_name), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "inline") >= 0, "should contain inline function")
        t |> success(find(text, aot_name) >= 0, "should contain the selected aot_name")
    }
}

[test]
def test_aot_missing_file(t : T?) {
    t |> run("missing file reports error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_aot("nonexistent_file_12345.das", ""), text, is_error)
        t |> success(is_error, "should be error")
    }
}

// ── JSON output mode tests ──────────────────────────────────────────

[test]
def test_compile_check_json(t : T?) {
    t |> run("json mode returns CompileResult array") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_compile_check(fixture_path("_fixture_valid.das"), "", true), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "\"success\":true") >= 0, "should contain success:true")
        t |> success(find(text, "\"file\":") >= 0, "should contain file field")
    }
    t |> run("json mode reports errors") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_compile_check(fixture_path("_fixture_error.das"), "", true), text, is_error)
        t |> success(find(text, "\"success\":false") >= 0, "should contain success:false")
        t |> success(find(text, "\"errors\":") >= 0, "should contain errors field")
    }
    t |> run("json batch returns array") <| @(t : T?) {
        var text : string
        var is_error = false
        let batch = fixture_path("_fixture_valid.das") + "," + fixture_path("_fixture_error.das")
        parse_result(do_compile_check(batch, "", true), text, is_error)
        // Should be a JSON array with 2 elements
        let arr_start = "[" + "\{"
        t |> success(find(text, arr_start) >= 0, "should start with array")
        t |> success(find(text, "\"success\":true") >= 0, "should have a passing result")
        t |> success(find(text, "\"success\":false") >= 0, "should have a failing result")
    }
}

[test]
def test_run_test_json(t : T?) {
    t |> run("json mode returns TestSummary") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_run_test(fixture_path("_fixture_test.das"), "", "", true), text, is_error)
        t |> success(ok, "parse result JSON")
        // NOTE: popen stdout capture broken when run from dastest on Windows (see wip.das)
        // t |> success(!is_error, "should not be error")
        // t |> success(find(text, "\"success\":true") >= 0, "should contain success:true")
        // t |> success(find(text, "\"total\":") >= 0, "should contain total field")
        // t |> success(find(text, "\"tests\":") >= 0, "should contain tests array")
    }
}

[test]
def test_list_modules_json(t : T?) {
    t |> run("json mode returns ModuleList") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_list_modules(true), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "\"builtin\":") >= 0, "should contain builtin field")
        t |> success(find(text, "\"daslib\":") >= 0, "should contain daslib field")
        t |> success(find(text, "\"math\"") >= 0, "should contain math module")
    }
}

[test]
def test_list_requires_json(t : T?) {
    t |> run("json mode returns RequiresResult") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_list_requires(fixture_path("_fixture_valid.das"), "", true), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "\"direct\":") >= 0, "should contain direct field")
        t |> success(find(text, "\"transitive\":") >= 0, "should contain transitive field")
    }
}

// ── perf_lint (merged into lint) ─────────────────────────────────────

[test]
def test_perf_lint_warnings(t : T?) {
    t |> run("detects PERF001 warning") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_lint(fixture_path("_fixture_perf_lint.das")), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "PERF001") >= 0, "should contain PERF001")
        t |> success(find(text, "lint issue") >= 0, "should report lint issues")
    }
}

[test]
def test_perf_lint_clean(t : T?) {
    t |> run("clean file has no warnings") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_lint(fixture_path("_fixture_valid.das")), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "No lint issues") >= 0, "should say no issues")
    }
}

[test]
def test_perf_lint_compile_error(t : T?) {
    t |> run("compile error reports failure") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_lint(fixture_path("_fixture_error.das")), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(is_error, "should be error")
        t |> success(find(text, "Compilation failed") >= 0, "should contain 'Compilation failed'")
    }
}

[test]
def test_perf_lint_no_match(t : T?) {
    t |> run("no files matched reports error") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_lint("nonexistent_dir_xyz/*.das"), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(is_error, "should be error")
        t |> success(find(text, "no files matched") >= 0, "should mention no files matched")
    }
}

// ── lint ─────────────────────────────────────────────────────────────

[test]
def test_lint_issues(t : T?) {
    t |> run("detects unused variable and unreachable code") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_lint(fixture_path("_fixture_lint.das")), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "unused variable") >= 0, "should detect unused variable")
        t |> success(find(text, "unreachable code") >= 0, "should detect unreachable code")
    }
}

[test]
def test_lint_clean(t : T?) {
    t |> run("clean file has no lint issues") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_lint(fixture_path("_fixture_valid.das")), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "No lint issues") >= 0, "should say no issues")
    }
}

[test]
def test_lint_compile_error(t : T?) {
    t |> run("compile error reports failure") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_lint(fixture_path("_fixture_error.das")), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(is_error, "should be error")
        t |> success(find(text, "Compilation failed") >= 0, "should contain 'Compilation failed'")
    }
}

[test]
def test_lint_no_match(t : T?) {
    t |> run("no files matched reports error") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_lint("nonexistent_dir_xyz/*.das"), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(is_error, "should be error")
        t |> success(find(text, "no files matched") >= 0, "should mention no files matched")
    }
}

