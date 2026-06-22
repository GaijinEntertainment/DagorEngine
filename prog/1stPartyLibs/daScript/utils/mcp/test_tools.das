options gen2
options rtti

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
require tools/cpp_grep_usage public
require tools/cpp_find_symbol public
require tools/cpp_outline public
require tools/cpp_goto_definition public
require tools/aot public
require tools/lint_tool public
require tools/program_log public
require tools/live public
require tools/detect_duplicates public
require tools/export_corpus public

// helper: parse tool result JSON, return (text, isError)
def parse_result(result : string; var text : string&; var is_error : bool&) : bool {
    var err : string
    var parsed = read_json(result, err)
    if (parsed == null) return false
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

// Guard for tests that go through ast-grep (grep_usage / outline / cpp_*).
// Windows CI doesn't always have @ast-grep/cli installed; without this guard
// every dependent assertion fails with `envelope: ast-grep not available`.
// Returns true if available; otherwise logs a single notice and returns false
// so the caller can early-out of the whole `[test]` body. The test then
// completes with zero sub-asserts and is reported as PASS — the alternative
// (`t->skip()`) would panic across the invoke_in_context boundary and resurface
// in the parent test as a failure.
def private require_ast_grep(t : T?) : bool {
    if (!empty(get_ast_grep_cmd())) return true
    t->log("ast-grep not installed — skipping test (install @ast-grep/cli to enable)")
    return false
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

// Locks the focused project-arg errors emitted by validate_project_arg so the
// pre-#2622 misleading "missing prerequisite ''" failure mode can't return.
[test]
def test_compile_check_invalid_project(t : T?) {
    t |> run("missing project file reports focused error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_compile_check(fixture_path("_fixture_valid.das"), "nonexistent_xyz.das_project"), text, is_error)
        t |> success(is_error, "should be error")
        t |> success(find(text, "'project' file not found") >= 0, "should mention project file not found")
    }
    t |> run("non-.das_project extension reports focused error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_compile_check(fixture_path("_fixture_valid.das"), fixture_path("_fixture_valid.das")), text, is_error)
        t |> success(is_error, "should be error")
        t |> success(find(text, "expected a path to a .das_project file") >= 0, "should mention expected extension")
    }
}

[test]
def test_run_script_invalid_project(t : T?) {
    t |> run("missing project file reports focused error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_run_script("", "print(\"hi\")", "", false, "nonexistent_xyz.das_project"), text, is_error)
        t |> success(is_error, "should be error")
        t |> success(find(text, "'project' file not found") >= 0, "should mention project file not found")
    }
    t |> run("non-.das_project extension reports focused error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_run_script("", "print(\"hi\")", "", false, fixture_path("_fixture_valid.das")), text, is_error)
        t |> success(is_error, "should be error")
        t |> success(find(text, "expected a path to a .das_project file") >= 0, "should mention expected extension")
    }
}

[test]
def test_eval_expression_invalid_project(t : T?) {
    t |> run("missing project file reports focused error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_eval_expression("1 + 1", "", "nonexistent_xyz.das_project"), text, is_error)
        t |> success(is_error, "should be error")
        t |> success(find(text, "'project' file not found") >= 0, "should mention project file not found")
    }
    t |> run("non-.das_project extension reports focused error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_eval_expression("1 + 1", "", fixture_path("_fixture_valid.das")), text, is_error)
        t |> success(is_error, "should be error")
        t |> success(find(text, "expected a path to a .das_project file") >= 0, "should mention expected extension")
    }
}

// ── parse_file_list (smoke) ──────────────────────────────────────────
// Canonical unit tests for `parse_file_list` and `expand_glob` live in
// `tests/fio/expand_glob_test.das` (the helpers moved to `daslib/fio`).
// Tools-side coverage of comma-list / glob / mixed inputs continues
// further down via `do_format_file` / `do_compile_check` / `do_lint`
// integration tests. This single subtest only confirms that the helper
// is reachable through the MCP `require` chain after the move.

def has_path(files : array<string>; suffix : string) : bool {
    for (f in files) {
        if (find(f, suffix) >= 0) return true
    }
    return false
}

[test]
def test_parse_file_list_smoke(t : T?) {
    t |> run("parse_file_list reachable through MCP require chain") <| @(t : T?) {
        var files : array<string>
        parse_file_list("{fixture_path("_fixture_valid.das")},utils/mcp/tests/_fixture_e*.das", files)
        t |> success(has_path(files, "_fixture_valid.das"), "plain entry preserved")
        t |> success(has_path(files, "_fixture_error.das"), "glob expanded")
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
    t |> run("rewrites unqualified daslib name") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_list_module_api("ast", "", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "[resolved 'ast' as 'daslib/ast']") >= 0, "should announce rewrite")
        t |> success(find(text, "AstVisitor") >= 0, "should list daslib/ast contents")
    }
    t |> run("no rewrite when name is already qualified") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_list_module_api("daslib/ast", "", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "[resolved") < 0, "should not announce rewrite")
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

def private parse_json_array(text : string; var parsed : JsonValue?&) : bool {
    var jerr : string
    parsed = read_json(text, jerr)
    return parsed != null && parsed.value is _array
}

def private find_format_result(arr : JsonValue?; file_substr : string; var status : string&; var message : string&) : bool {
    if (arr == null || !(arr.value is _array)) return false
    for (entry in arr.value as _array) {
        if (entry == null) continue
        let file_val = entry?["file"]
        if (file_val == null || !(file_val.value is _string)) continue
        let file = file_val.value as _string
        if (find(file, file_substr) < 0) continue
        let status_val = entry?["status"]
        let msg_val = entry?["message"]
        if (status_val != null && status_val.value is _string) {
            status = status_val.value as _string
        }
        if (msg_val != null && msg_val.value is _string) {
            message = msg_val.value as _string
        }
        return true
    }
    return false
}

[test]
def test_format_file(t : T?) {
    t |> run("single file returns JSON array with required fields") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_format_file(fixture_path("_fixture_valid.das")), text, is_error)
        t |> success(!is_error, "should not be error")

        var parsed : JsonValue?
        let ok = parse_json_array(text, parsed)
        t |> success(ok, "inner payload should be a JSON array")
        if (!ok) {
            unsafe { delete parsed; }
            return
        }

        t |> success(length(parsed.value as _array) >= 1, "array should contain at least one result")
        let first = (parsed.value as _array)[0]
        t |> success(first != null, "first result should exist")
        if (first != null) {
            t |> success(first?["file"] != null, "result has file")
            t |> success(first?["status"] != null, "result has status")
            t |> success(first?["message"] != null, "result has message")
            let st = first?["status"]
            if (st != null && st.value is _string) {
                let status = st.value as _string
                t |> success(status == "formatted" || status == "already_formatted", "status is expected")
            } else {
                t |> failure("status should be a string")
            }
        }
        unsafe { delete parsed; }
    }

    t |> run("comma-separated list returns one result per file") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_format_file("{fixture_path("_fixture_valid.das")},{fixture_path("_fixture_error.das")}"), text, is_error)
        t |> success(!is_error, "both fixture files should be processable")

        var parsed : JsonValue?
        let ok = parse_json_array(text, parsed)
        t |> success(ok, "inner payload should be a JSON array")
        if (!ok) {
            unsafe { delete parsed; }
            return
        }
        t |> equal(length(parsed.value as _array), 2, "comma-separated list should yield two results")

        var st1 = ""
        var msg1 = ""
        let has_valid = find_format_result(parsed, "_fixture_valid.das", st1, msg1)
        t |> success(has_valid, "valid fixture result present")
        t |> success(!empty(st1), "valid fixture has non-empty status")
        t |> success(find(msg1, "fixture_valid") >= 0, "valid fixture message references file")

        var st2 = ""
        var msg2 = ""
        let has_error_fixture = find_format_result(parsed, "_fixture_error.das", st2, msg2)
        t |> success(has_error_fixture, "error fixture result present")
        t |> success(!empty(st2), "error fixture has non-empty status")
        t |> success(find(msg2, "fixture_error") >= 0, "error fixture message references file")
        unsafe { delete parsed; }
    }

    t |> run("glob pattern expands and returns matching file entry") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_format_file("utils/mcp/tests/_fixture_v*.das"), text, is_error)
        t |> success(!is_error, "glob with matches should not be error")

        var parsed : JsonValue?
        let ok = parse_json_array(text, parsed)
        t |> success(ok, "inner payload should be a JSON array")
        if (!ok) {
            unsafe { delete parsed; }
            return
        }
        var st = ""
        var msg = ""
        let has_valid = find_format_result(parsed, "_fixture_valid.das", st, msg)
        t |> success(has_valid, "glob should include _fixture_valid.das")
        t |> success(st == "formatted" || st == "already_formatted", "glob result status is expected")
        unsafe { delete parsed; }
    }

    t |> run("mixed valid + missing sets isError and reports per-file statuses") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_format_file("{fixture_path("_fixture_valid.das")},utils/mcp/tests/_fixture_does_not_exist_123.das"), text, is_error)
        t |> success(is_error, "missing file in batch should set isError")

        var parsed : JsonValue?
        let ok = parse_json_array(text, parsed)
        t |> success(ok, "inner payload should be a JSON array")
        if (!ok) {
            unsafe { delete parsed; }
            return
        }
        t |> equal(length(parsed.value as _array), 2, "mixed batch should still return two result entries")

        var st_ok = ""
        var msg_ok = ""
        let has_valid = find_format_result(parsed, "_fixture_valid.das", st_ok, msg_ok)
        t |> success(has_valid, "valid file result present")
        t |> success(st_ok == "formatted" || st_ok == "already_formatted", "valid file status is expected")

        var st_missing = ""
        var msg_missing = ""
        let has_missing = find_format_result(parsed, "_fixture_does_not_exist_123.das", st_missing, msg_missing)
        t |> success(has_missing, "missing file result present")
        t |> equal(st_missing, "error")
        t |> success(find(msg_missing, "Cannot open file") >= 0, "missing file reports open error")
        unsafe { delete parsed; }
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
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "46", "14", "file", ""), text, is_error)
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
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "46", "25", "file", ""), text, is_error)
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
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "15", "22", "file", ""), text, is_error)
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
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "48", "30", "file", ""), text, is_error)
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
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "23", "12", "file", ""), text, is_error)
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
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "14", "5", "file", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "func") >= 0, "should identify as function")
        t |> success(find(text, "add_points") >= 0, "should name add_points")
        t |> success(find(text, "[call]") >= 0, "should find call reference")
    }
    t |> run("finds references from struct definition") <| @(t : T?) {
        var text : string
        var is_error = false
        // cursor on 'struct Point' at line 9
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "9", "1", "file", ""), text, is_error)
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
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "26", "1", "file", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "alias") >= 0, "should identify as alias")
        t |> success(find(text, "Flags") >= 0, "should name Flags")
        t |> success(find(text, "[alias ref]") >= 0, "should find alias references")
    }
    t |> run("finds alias references from variant usage") <| @(t : T?) {
        var text : string
        var is_error = false
        // cursor on Value(i = 10) at line 51 col 13
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "51", "13", "file", ""), text, is_error)
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
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "2", "1", "file", ""), text, is_error)
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
    t |> run("rewrites unqualified daslib require") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_eval_expression("1 + 2", "ast"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "[resolved unqualified modules under daslib/") >= 0, "should announce rewrite")
        t |> success(find(text, "3") >= 0, "should evaluate to 3")
    }
    t |> run("retry-also-fails surfaces retry error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_eval_expression("1 + 2", "nonexistent_module_xyz"), text, is_error)
        t |> success(is_error, "should be error")
        t |> success(find(text, "also retried with require 'daslib/nonexistent_module_xyz'") >= 0,
                "should mention retry attempt with rewritten name")
    }
    t |> run("retry skipped for non-module-resolution failures") <| @(t : T?) {
        var text : string
        var is_error = false
        // Builtin module 'math' resolves fine, but the expression fails (undefined symbol).
        // Retry must NOT fire — otherwise the user sees a misleading 'daslib/math' error
        // instead of the actual undefined-symbol error.
        parse_result(do_eval_expression("undefined_func_xyz()", "math"), text, is_error)
        t |> success(is_error, "should be error")
        t |> success(find(text, "daslib/math") < 0,
                "should NOT mention daslib/math — failure is not module-resolution")
        t |> success(find(text, "also retried") < 0, "should NOT announce a retry attempt")
    }
    t |> run("mixed list rewrites only the missing token") <| @(t : T?) {
        var text : string
        var is_error = false
        // 'math' is builtin (resolves), 'ast' needs daslib/. Retry must rewrite ONLY 'ast',
        // leaving 'math' verbatim. Otherwise the retry would fail on 'daslib/math' missing.
        parse_result(do_eval_expression("1 + 2", "math, ast"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "math, daslib/ast") >= 0, "should rewrite ast but keep math")
        t |> success(find(text, "daslib/math") < 0, "should NOT rewrite the builtin math")
        t |> success(find(text, "3") >= 0, "should evaluate to 3")
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
    t |> run("rewrites unqualified daslib module") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_describe_type("AstFunctionAnnotation", "ast"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "[resolved 'ast' as 'daslib/ast']") >= 0, "should announce rewrite")
        t |> success(find(text, "class AstFunctionAnnotation") >= 0, "should describe class")
    }
}

// ── grep_usage ──────────────────────────────────────────────────────

[test]
def test_grep_usage(t : T?) {
    if (!require_ast_grep(t)) return
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
    if (!require_ast_grep(t)) return
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
    if (!require_ast_grep(t)) return
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
    if (!require_ast_grep(t)) return
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
    if (!require_ast_grep(t)) return
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
    if (!require_ast_grep(t)) return
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
        if (bracket_pos < 0) return
        let name_start = bracket_pos + 1
        let name_end = find(text, "]", name_start)
        t |> success(name_end > name_start, "should have closing bracket")
        if (name_end <= name_start) return
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
        let ok = parse_result(do_compile_check(fixture_path("_fixture_valid.das"), "", "", true), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "\"success\":true") >= 0, "should contain success:true")
        t |> success(find(text, "\"file\":") >= 0, "should contain file field")
    }
    t |> run("json mode reports errors") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_compile_check(fixture_path("_fixture_error.das"), "", "", true), text, is_error)
        t |> success(find(text, "\"success\":false") >= 0, "should contain success:false")
        t |> success(find(text, "\"errors\":") >= 0, "should contain errors field")
    }
    t |> run("json batch returns array") <| @(t : T?) {
        var text : string
        var is_error = false
        let batch = fixture_path("_fixture_valid.das") + "," + fixture_path("_fixture_error.das")
        parse_result(do_compile_check(batch, "", "", true), text, is_error)
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
        let ok = parse_result(do_run_test(fixture_path("_fixture_test.das"), "", "", "", true), text, is_error)
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
        let ok = parse_result(do_list_requires(fixture_path("_fixture_valid.das"), "", "", true), text, is_error)
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
def test_lint_skip_expect(t : T?) {
    t |> run("file with `expect` directive is skipped via a VALID result envelope") <| @(t : T?) {
        var text : string
        var is_error = false
        // Regression: the single-file SKIP path used to return a raw "SKIP ..." string, not a
        // make_tool_result envelope — so the JSON-RPC `result` was invalid and the client hung.
        // parse_result returning false (unparseable) is the pre-fix signature.
        let ok = parse_result(do_lint(fixture_path("_fixture_expect.das")), text, is_error)
        t |> success(ok, "SKIP result must be a valid envelope (was a raw string => invalid JSON-RPC)")
        t |> success(!is_error, "skip is not an error")
        t |> success(find(text, "SKIP") >= 0, "should report SKIP")
        t |> success(find(text, "intentional compile errors") >= 0, "should explain the skip reason")
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

// Same focused-error contract as test_compile_check_invalid_project, but for
// run_lint's hoisted validate_project_arg call site.
[test]
def test_lint_invalid_project(t : T?) {
    t |> run("missing project file reports focused error") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_lint(fixture_path("_fixture_valid.das"), "nonexistent_xyz.das_project"), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(is_error, "should be error")
        t |> success(find(text, "'project' file not found") >= 0, "should mention project file not found")
    }
    t |> run("non-.das_project extension reports focused error") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_lint(fixture_path("_fixture_valid.das"), fixture_path("_fixture_valid.das")), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(is_error, "should be error")
        t |> success(find(text, "expected a path to a .das_project file") >= 0, "should mention expected extension")
    }
}

// ── detect_duplicates ────────────────────────────────────────────────


// Generic write-temp-file helper: creates a unique temp file with `prefix`
// and `ext`, writes `body` to it, returns the path. Returns "" and calls
// `t |> failure` on temp-file allocation failure.
def make_temp_text_file(t : T?; prefix : string; ext : string; body : string) : string {
    let r = create_temp_file_result(prefix, ext)
    if (!(r is value)) {
        t |> failure("create_temp_file_result failed")
        return ""
    }
    let path = unsafe(r.value)
    fopen(path, "wb") $(fw) {
        if (fw != null) {
            fw |> fprint(body)
        }
    }
    return path
}


def make_temp_corpus_file(t : T?; body : string) : string {
    return make_temp_text_file(t, "detect_dupe_mcp_test", ".json", body)
}


[test]
def test_detect_duplicates_missing_corpus(t : T?) {
    t |> run("missing 'corpus' arg returns error envelope") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_detect_duplicates("foo.das", "", "", "", "", ""), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(is_error, "should be error")
        t |> success(find(text, "corpus") >= 0, "should mention corpus: {text}")
    }
}


[test]
def test_detect_duplicates_missing_paths(t : T?) {
    t |> run("missing 'paths' arg returns error envelope") <| @(t : T?) {
        let path = make_temp_corpus_file(t, "\{\"schema_version\":2,\"functions\":[]\}")
        if (empty(path)) return
        var text : string
        var is_error = false
        let ok = parse_result(do_detect_duplicates("", path, "", "", "", ""), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(is_error, "should be error")
        t |> success(find(text, "paths") >= 0, "should mention paths: {text}")
        remove(path)
    }
}


[test]
def test_detect_duplicates_corpus_not_found(t : T?) {
    t |> run("nonexistent corpus path is reported") <| @(t : T?) {
        // Generate a portable temp path then immediately delete the file
        // — the path is now guaranteed nonexistent and unique to this run,
        // and works on Windows runners where /tmp doesn't exist.
        let bogus = make_temp_corpus_file(t, "")
        if (empty(bogus)) return
        remove(bogus)
        var text : string
        var is_error = false
        let ok = parse_result(do_detect_duplicates(fixture_path("_fixture_valid.das"),
            bogus, "", "", "", ""), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(is_error, "should be error")
    }
}


[test]
def test_detect_duplicates_empty_corpus(t : T?) {
    t |> run("empty corpus + valid candidate yields envelope with embedded report") <| @(t : T?) {
        let path = make_temp_corpus_file(t, "\{\"schema_version\":2,\"functions\":[]\}")
        if (empty(path)) return
        var text : string
        var is_error = false
        let ok = parse_result(do_detect_duplicates(fixture_path("_fixture_valid.das"),
            path, "", "", "", ""), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "should NOT be error: {text}")
        t |> success(find(text, "\"exit_code\":0") >= 0, "envelope has exit_code 0: {text}")
        t |> success(find(text, "\"report\"") >= 0, "envelope has nested report: {text}")
        t |> success(find(text, "candidates") >= 0, "report contains candidates array: {text}")
        remove(path)
    }
}


[test]
def test_detect_duplicates_bad_schema(t : T?) {
    t |> run("corpus with bad schema_version is rejected") <| @(t : T?) {
        let path = make_temp_corpus_file(t, "\{\"schema_version\":99,\"functions\":[]\}")
        if (empty(path)) return
        var text : string
        var is_error = false
        let ok = parse_result(do_detect_duplicates(fixture_path("_fixture_valid.das"),
            path, "", "", "", ""), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(is_error, "should be error")
        t |> success(find(text, "schema_version") >= 0, "mentions schema_version: {text}")
        remove(path)
    }
}


// ── export_corpus ────────────────────────────────────────────────────


[test]
def test_export_corpus_missing_paths(t : T?) {
    t |> run("missing 'paths' / 'paths_file' returns error envelope") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_export_corpus("", "", "export_corpus_test_out.json", "", ""), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(is_error, "should be error")
        t |> success(find(text, "paths") >= 0, "should mention paths: {text}")
    }
}


[test]
def test_export_corpus_missing_out(t : T?) {
    t |> run("missing 'out' arg returns error envelope") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_export_corpus("foo.das", "", "", "", ""), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(is_error, "should be error")
        t |> success(find(text, "out") >= 0, "should mention out: {text}")
    }
}


[test]
def test_export_corpus_no_match(t : T?) {
    t |> run("paths matching nothing yields error envelope") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_export_corpus("does_not_exist_xyzzy.das", "", "export_corpus_test_out.json", "", ""), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(is_error, "should be error: {text}")
    }
}


[test]
def test_export_corpus_writes_envelope(t : T?) {
    t |> run("valid fixture produces a corpus and a populated envelope") <| @(t : T?) {
        let r = create_temp_file_result("export_corpus_mcp_test", ".json")
        if (!(r is value)) {
            t |> failure("create_temp_file_result failed")
            return
        }
        let out_path = unsafe(r.value)
        // create_temp_file_result returns a path to a freshly-created temp
        // file. write_export reopens it with "wb" and overwrites, so leaving
        // the placeholder in place is the intended path.
        var text : string
        var is_error = false
        let ok = parse_result(do_export_corpus(fixture_path("_fixture_valid.das"), "", out_path, "", ""), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "should NOT be error: {text}")
        t |> success(find(text, "\"exit_code\":0") >= 0, "envelope has exit_code 0: {text}")
        t |> success(find(text, "\"out\":") >= 0, "envelope echoes out path: {text}")
        t |> success(find(text, "\"stdout\":") >= 0, "envelope has subprocess stdout: {text}")
        // Confirm the file actually exists and is non-empty.
        let body = fread(out_path)
        t |> success(!empty(body), "corpus file is non-empty")
        t |> success(find(body, "\"schema_version\"") >= 0, "corpus has schema_version: {body}")
        remove(out_path)
    }
}


// Helper: write `content` to a temp .txt and return its path. Used by
// the paths_file / workers integration tests. Thin wrapper around
// `make_temp_text_file` to keep the call sites readable.
def make_temp_paths_file(t : T?; content : string) : string {
    return make_temp_text_file(t, "export_corpus_paths", ".txt", content)
}


[test]
def test_export_corpus_paths_file(t : T?) {
    t |> run("paths_file alone produces a corpus") <| @(t : T?) {
        let paths_body = "{fixture_path("_fixture_valid.das")}\n"
        let paths_file = make_temp_paths_file(t, paths_body)
        if (empty(paths_file)) return
        let r = create_temp_file_result("export_corpus_paths_file", ".json")
        if (!(r is value)) {
            t |> failure("create_temp_file_result failed")
            remove(paths_file)
            return
        }
        let out_path = unsafe(r.value)
        var text : string
        var is_error = false
        let ok = parse_result(do_export_corpus("", paths_file, out_path, "", ""), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "should NOT be error: {text}")
        let body = fread(out_path)
        t |> success(!empty(body), "corpus file is non-empty")
        t |> success(find(body, "\"schema_version\"") >= 0, "corpus has schema_version")
        t |> success(find(body, "_fixture_valid.das") >= 0, "corpus references the fixture file")
        remove(paths_file)
        remove(out_path)
    }
}


[test]
def test_export_corpus_paths_and_paths_file_union(t : T?) {
    t |> run("paths and paths_file unioned (no duplicate records)") <| @(t : T?) {
        // paths and paths_file each contribute one fixture; the union
        // should compile both, with no (file, name, line) triple
        // appearing twice — verifies the input dedup, not just that
        // both filenames show up somewhere in the JSON text.
        let paths_body = "{fixture_path("_fixture_test.das")}\n"
        let paths_file = make_temp_paths_file(t, paths_body)
        if (empty(paths_file)) return
        let r = create_temp_file_result("export_corpus_union", ".json")
        if (!(r is value)) {
            t |> failure("create_temp_file_result failed")
            remove(paths_file)
            return
        }
        let out_path = unsafe(r.value)
        var text : string
        var is_error = false
        let ok = parse_result(
            do_export_corpus(fixture_path("_fixture_valid.das"), paths_file,
                             out_path, "", ""),
            text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "should NOT be error: {text}")
        let body = fread(out_path)
        var jerr : string
        var parsed = read_json(body, jerr)
        t |> success(parsed != null, "envelope parses: {jerr}")
        if (parsed == null) {
            remove(paths_file)
            remove(out_path)
            return
        }
        let funcs = parsed?["functions"]
        t |> success(funcs != null && funcs.value is _array, "functions array present")
        var seen : table<string; int>
        var have_valid = false
        var have_test = false
        if (funcs != null && funcs.value is _array) {
            for (ef in funcs.value as _array) {
                if (ef == null) continue
                var f = ""
                var n = ""
                var ln = 0
                let fv = ef?["file"]
                if (fv != null && fv.value is _string) {
                    f = fv.value as _string
                }
                let nv = ef?["name"]
                if (nv != null && nv.value is _string) {
                    n = nv.value as _string
                }
                let lv = ef?["line"]
                if (lv != null && lv.value is _number) {
                    ln = int(lv.value as _number)
                }
                let key = "{f}:{n}:{ln}"
                seen[key] = (seen?[key] ?? 0) + 1
                if (find(f, "_fixture_valid.das") >= 0) {
                    have_valid = true
                }
                if (find(f, "_fixture_test.das") >= 0) {
                    have_test = true
                }
            }
        }
        t |> success(have_valid, "valid fixture contributed records")
        t |> success(have_test, "test fixture contributed records")
        var dupes = 0
        for (count in values(seen)) {
            if (count > 1) {
                dupes++
            }
        }
        t |> equal(dupes, 0)
        unsafe { delete parsed; }
        remove(paths_file)
        remove(out_path)
    }
}


[test]
def test_export_corpus_workers_one(t : T?) {
    t |> run("workers param parses and runs the sequential path") <| @(t : T?) {
        // Argv-plumbing test only. The fixture is a single file —
        // far below `PARALLEL_THRESHOLD = 16` — so both `workers=""`
        // (default) and `workers="1"` go through the same in-process
        // sequential loop. There is no parallel-vs-sequential
        // equivalence to verify here; that's covered by the
        // multi-stage manual determinism check (sequential vs
        // parallel byte-equality on daslib / tests/language). What
        // we DO verify: the `--workers 1` argv reaches the child
        // and the child produces a well-formed envelope.
        let r = create_temp_file_result("export_corpus_w1", ".json")
        if (!(r is value)) {
            t |> failure("create_temp_file_result failed")
            return
        }
        let out_path = unsafe(r.value)
        var text : string
        var is_error = false
        let ok = parse_result(
            do_export_corpus(fixture_path("_fixture_valid.das"), "",
                             out_path, "", "1"),
            text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "should NOT be error: {text}")
        let body = fread(out_path)
        t |> success(!empty(body), "corpus non-empty")
        t |> success(find(body, "\"schema_version\"") >= 0, "schema_version present")
        remove(out_path)
    }
}


[test]
def test_export_corpus_workers_failure_propagates(t : T?) {
    t |> run("compile error in input fails the run, even with workers") <| @(t : T?) {
        // _fixture_error.das deliberately fails to compile. Whether the
        // run goes parallel (large list) or sequential (small list),
        // detect-dupe panics on compile failure → parent exit non-zero
        // → MCP envelope reports is_error.
        let paths_body = "{fixture_path("_fixture_error.das")}\n"
        let paths_file = make_temp_paths_file(t, paths_body)
        if (empty(paths_file)) return
        let r = create_temp_file_result("export_corpus_fail", ".json")
        if (!(r is value)) {
            t |> failure("create_temp_file_result failed")
            remove(paths_file)
            return
        }
        let out_path = unsafe(r.value)
        var text : string
        var is_error = false
        let ok = parse_result(
            do_export_corpus("", paths_file, out_path, "", "1"),
            text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(is_error, "should be error: {text}")
        t |> success(find(text, "exit code") >= 0, "envelope mentions exit code: {text}")
        remove(paths_file)
        remove(out_path)
    }
}

// ── cpp_grep_usage ───────────────────────────────────────────────────

// Diagnostic helper: when a cpp_* tool returns is_error=true unexpectedly the
// CI log only shows the assertion message, not the envelope text. Embed a
// truncated snapshot so a Windows-only failure surfaces what sg actually said.
def private cpp_diag(text : string) : string {
    let trimmed = length(text) > 800 ? slice(text, 0, 800) : text
    return "envelope: {trimmed}"
}

[test]
def test_cpp_grep_usage(t : T?) {
    if (!require_ast_grep(t)) return
    t |> run("finds known C++ symbol in src/ast") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_grep_usage("addExternFunc", "src/ast", "", ""), text, is_error)
        t |> success(!is_error, "should not be error -- {cpp_diag(text)}")
        t |> success(find(text, "ast_interop.cpp") >= 0, "should mention ast_interop.cpp -- {cpp_diag(text)}")
    }
    t |> run("missing dir reports cleanly") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_grep_usage("anything", "/no/such/dir/here", "", ""), text, is_error)
        t |> success(is_error || find(text, "No matches") >= 0, "should error or report no matches")
    }
    t |> run("missing symbol arg returns error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_grep_usage("", "src", "", ""), text, is_error)
        t |> success(is_error, "should be error")
    }
}

// ── cpp_grep_usage v2: multi-kind identifier matching ────────────────
//
// Each test below FAILS under the old `sg run -p Symbol` implementation
// because that pattern only matches bare identifier expressions. The new
// implementation generates a multi-kind YAML rule (identifier +
// type_identifier + namespace_identifier + field_identifier) so usages in
// type positions, qualified calls, base-class clauses, etc. are all caught.

[test]
def test_cpp_grep_usage_type_position(t : T?) {
    if (!require_ast_grep(t)) return
    t |> run("finds Outer used as a return type") <| @(t : T?) {
        var text : string
        var is_error = false
        // GrepDerived::makeOuter() returns Outer.
        parse_result(do_cpp_grep_usage("Outer", "utils/mcp/tests", "", ""), text, is_error)
        t |> success(!is_error, "should not be error -- {cpp_diag(text)}")
        t |> success(find(text, "Outer makeOuter()") >= 0,
            "must catch type_identifier in return position -- {cpp_diag(text)}")
    }
    t |> run("finds Outer used as a parameter type") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_grep_usage("Outer", "utils/mcp/tests", "", ""), text, is_error)
        t |> success(find(text, "void takeOuter(const Outer & o)") >= 0,
            "must catch type_identifier in parameter position -- {cpp_diag(text)}")
    }
    t |> run("finds Outer used as a base class") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_grep_usage("Outer", "utils/mcp/tests", "", ""), text, is_error)
        t |> success(find(text, "class GrepDerived : public Outer") >= 0,
            "must catch namespace_identifier in base_class_clause -- {cpp_diag(text)}")
    }
    t |> run("finds Outer used as qualified scope") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_grep_usage("Outer", "utils/mcp/tests", "", ""), text, is_error)
        t |> success(find(text, "Outer::Inner inner") >= 0,
            "must catch namespace_identifier in qualified type expression -- {cpp_diag(text)}")
    }
}

[test]
def test_cpp_grep_usage_dedup(t : T?) {
    if (!require_ast_grep(t)) return
    t |> run("a single source line appears at most once even if multiple AST nodes match") <| @(t : T?) {
        // The destructor signature `TextWriter::~TextWriter()` produces both a
        // namespace_identifier and a type_identifier hit; the line should
        // appear exactly once in the output.
        var text : string
        var is_error = false
        parse_result(do_cpp_grep_usage("TextWriter", "src/misc/string_writer.cpp", "", ""), text, is_error)
        t |> success(!is_error, "should not be error -- {cpp_diag(text)}")
        // count occurrences of the destructor line.
        var count = 0
        var pos = 0
        let needle = "TextWriter::~TextWriter()"
        while (true) {
            let i = find(text, needle, pos)
            if (i < 0) break
            count++
            pos = i + length(needle)
        }
        t |> equal(count, 1, "destructor line should appear exactly once after dedup")
    }
}

// ── cpp_find_symbol ──────────────────────────────────────────────────

[test]
def test_cpp_find_symbol(t : T?) {
    if (!require_ast_grep(t)) return
    t |> run("finds class in src/ast") <| @(t : T?) {
        var text : string
        var is_error = false
        // narrowed from "src" (~144 files) to "src/ast" (~34 files); Module lives in src/ast
        parse_result(do_cpp_find_symbol("=Module", "class", "src/ast", ""), text, is_error)
        t |> success(!is_error, "should not be error -- {cpp_diag(text)}")
        t |> success(find(text, "Module") >= 0, "should find Module -- {cpp_diag(text)}")
    }
    t |> run(".h languageGlobs regression — finds class in header") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_find_symbol("=FsFileSystem", "class", "include", ""), text, is_error)
        t |> success(!is_error, "should not be error -- {cpp_diag(text)}")
        t |> success(find(text, "fs_file_info.h") >= 0, "should find FsFileSystem in fs_file_info.h (proves .h is treated as cpp) -- {cpp_diag(text)}")
    }
    t |> run("no-match returns clean empty result") <| @(t : T?) {
        var text : string
        var is_error = false
        // narrowed scope; absence-of-symbol is structural, not scope-dependent
        parse_result(do_cpp_find_symbol("zzznotarealsymbolzzz_xyzzy", "", "src/ast", ""), text, is_error)
        t |> success(!is_error, "should not be error on no-match -- {cpp_diag(text)}")
        t |> success(find(text, "No matches") >= 0, "should report no matches -- {cpp_diag(text)}")
    }
    t |> run("kind filter rejects wrong kinds") <| @(t : T?) {
        var text : string
        var is_error = false
        // addFunction is a method (not a class) in src/ast/; narrowed scope still proves the kind filter
        parse_result(do_cpp_find_symbol("=addFunction", "class", "src/ast", ""), text, is_error)
        t |> success(!is_error, "should not be error -- {cpp_diag(text)}")
        t |> success(find(text, "No matches") >= 0, "kind=class should reject methods named addFunction -- {cpp_diag(text)}")
    }
    t |> run("missing query returns error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_find_symbol("", "", "src/ast", ""), text, is_error)
        t |> success(is_error, "should be error")
    }
}

// ── cpp_outline ──────────────────────────────────────────────────────

[test]
def test_cpp_outline(t : T?) {
    if (!require_ast_grep(t)) return
    t |> run("lists declarations in a known file") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_outline("src/ast/ast_interop.cpp", "", ""), text, is_error)
        t |> success(!is_error, "should not be error -- {cpp_diag(text)}")
        t |> success(find(text, "[function]") >= 0 || find(text, "[class]") >= 0 || find(text, "[struct]") >= 0, "should list at least one declaration -- {cpp_diag(text)}")
    }
    t |> run("missing both args returns error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_outline("", "", ""), text, is_error)
        t |> success(is_error, "should be error")
    }
    t |> run("invalid mode rejected") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_outline(fixture_path("_fixture_cpp_outline.h"), "", "verbose"), text, is_error)
        t |> success(is_error, "unknown mode should error")
    }
}

// ── cpp_outline v2 (signatures, nesting, anti-misparse, qualified names) ─

let private CPP_OUTLINE_FIXTURE = "_fixture_cpp_outline.h"

def cpp_outline_text(mode : string) : string {
    var text : string
    var is_error = false
    parse_result(do_cpp_outline(fixture_path(CPP_OUTLINE_FIXTURE), "", mode), text, is_error)
    return text
}

[test]
def test_cpp_outline_signatures(t : T?) {
    if (!require_ast_grep(t)) return
    t |> run("multi-line function declaration collapsed into a single signature") <| @(t : T?) {
        let text = cpp_outline_text("")
        // describe() is declared across 5 lines in the fixture; the renderer
        // must show the full param list with default values on one output line.
        t |> success(find(text, "void describe( const char * name, int count = 0, bool verbose = false ) const") >= 0,
            "expected collapsed signature")
    }
    t |> run("free function shows return type and params") <| @(t : T?) {
        let text = cpp_outline_text("")
        t |> success(find(text, "[function] void freeFunction(int x)") >= 0,
            "expected freeFunction signature")
    }
}

[test]
def test_cpp_outline_class_misparse_filtered(t : T?) {
    if (!require_ast_grep(t)) return
    t |> run("class with macro prefix is reported as the class, not as a function") <| @(t : T?) {
        let text = cpp_outline_text("")
        // The fixture has `class MY_EXPORT Outer { ... }`. tree-sitter-cpp emits
        // both a class_specifier and a bogus function_definition for that line;
        // the post-filter must drop the function. The class name must be Outer
        // (not MY_EXPORT).
        t |> success(find(text, "[class] Outer") >= 0, "expected [class] Outer")
        t |> success(find(text, "[function] class MY_EXPORT") < 0,
            "must NOT emit a function entry for the misparsed class line")
        t |> success(find(text, "[class] MY_EXPORT") < 0,
            "name extractor must not pick the leading macro as the class name")
    }
}

[test]
def test_cpp_outline_anonymous_filtered(t : T?) {
    if (!require_ast_grep(t)) return
    t |> run("anonymous union/struct/enum entries are dropped") <| @(t : T?) {
        let text = cpp_outline_text("")
        // The fixture has an anonymous union AND an anonymous enum inside Outer.
        // Neither should appear as an unnamed entry.
        t |> success(find(text, "[union] union \{") < 0, "anonymous union must be dropped")
        t |> success(find(text, "[union]\n") < 0, "no bare [union] entries")
        t |> success(find(text, "[enum] enum \{") < 0, "anonymous enum must be dropped")
        // kFlagA is an enumerator inside the anonymous enum — must NOT leak as a name.
        t |> success(find(text, "[enum] kFlagA") < 0, "anonymous enum's enumerators must not leak as the name")
    }
}

[test]
def test_cpp_outline_template_specializations(t : T?) {
    if (!require_ast_grep(t)) return
    t |> run("template specializations show distinct args") <| @(t : T?) {
        let text = cpp_outline_text("")
        t |> success(find(text, "[struct] Trait\n") >= 0, "primary Trait should appear")
        t |> success(find(text, "Trait<int>") >= 0, "Trait<int> should appear with args")
        t |> success(find(text, "Trait<float>") >= 0, "Trait<float> should appear with args")
    }
}

[test]
def test_cpp_outline_nesting(t : T?) {
    if (!require_ast_grep(t)) return
    t |> run("methods nest under their enclosing class in tree mode") <| @(t : T?) {
        let text = cpp_outline_text("tree")
        // Tree mode: Inner is a child of Outer (depth 2 from namespace).
        // Methods of Inner are at depth 3, indented 6 spaces past the line-num column.
        t |> success(find(text, "    [class] Inner") >= 0, "Inner should be indented under Outer")
        t |> success(find(text, "      [function] void method()") >= 0,
            "Inner's method should be indented under Inner")
    }
    t |> run("auto mode picks tree when nesting exists") <| @(t : T?) {
        let auto_text = cpp_outline_text("")
        let tree_text = cpp_outline_text("tree")
        t |> success(auto_text == tree_text, "auto mode should match tree mode for a nested file")
    }
}

[test]
def test_cpp_outline_qualified_names(t : T?) {
    if (!require_ast_grep(t)) return
    t |> run("flat mode prefixes in-class methods with their ancestor chain") <| @(t : T?) {
        let text = cpp_outline_text("flat")
        // In flat mode the indentation goes away; ancestors must show as :: prefix.
        t |> success(find(text, "fixns::Outer::describe") >= 0,
            "Outer::describe should be qualified with namespace + class")
        t |> success(find(text, "fixns::Outer::Inner::method") >= 0,
            "deeply-nested method should carry full qualifier chain")
    }
    t |> run("flat mode does not double-prefix already-qualified names") <| @(t : T?) {
        let text = cpp_outline_text("flat")
        // The out-of-class definition `void Outer::describe(...)` already has ::;
        // flat-mode injection must not produce `fixns::Outer::describe::Outer::describe`.
        t |> success(find(text, "fixns::Outer::describe::") < 0,
            "must not double-qualify out-of-class definition")
    }
}

// ── cpp_goto_definition ──────────────────────────────────────────────

[test]
def test_cpp_goto_definition_errors(t : T?) {
    t |> run("missing args returns error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_goto_definition("", "", ""), text, is_error)
        t |> success(is_error, "should be error")
    }
    t |> run("non-positive line/column returns error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_goto_definition("src/ast/ast_interop.cpp", "0", "0"), text, is_error)
        t |> success(is_error, "should be error on 0/0 line/col")
    }
    t |> run("nonexistent file returns error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_goto_definition("/no/such/file.cpp", "1", "1"), text, is_error)
        t |> success(is_error, "should be error")
    }
}

// ── with_cpp_source redirect on find_symbol / goto_definition ────────

[test]
def test_find_symbol_with_cpp_source(t : T?) {
    t |> run("redirect off (default) — no '→ cpp:' line") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_find_symbol("=print", "", "", "", "", false), text, is_error)
        t |> success(!is_error, "should not be error -- {cpp_diag(text)}")
        t |> success(find(text, "print") >= 0, "should still find print -- {cpp_diag(text)}")
        t |> success(find(text, "→ cpp:") < 0, "no redirect line when with_cpp_source=false -- {cpp_diag(text)}")
    }
    t |> run("redirect on — emits '→ cpp:' line for builtins") <| @(t : T?) {
        var text : string
        var is_error = false
        // cpp_dirs="src/builtin" narrows the C++ scan from full CPP_SEARCH_DIRS
        // (~947 files) to ~33 files where `print` lives. Drops this test from
        // ~19s to ~1s.
        parse_result(do_find_symbol("=print", "function", "", "", "", true, "src/builtin"), text, is_error)
        t |> success(!is_error, "should not be error -- {cpp_diag(text)}")
        // Either we located print's cpp source OR we report (not located).
        // Both prove the redirect path fired.
        t |> success(find(text, "→ cpp:") >= 0, "redirect line should appear when with_cpp_source=true -- {cpp_diag(text)}")
    }
}

[test]
def test_goto_definition_with_cpp_source(t : T?) {
    t |> run("redirect off (default) — no C++: line") <| @(t : T?) {
        var text : string
        var is_error = false
        // line 34 col 5 is the 'p' of print(...) on `print("{d}\n")` — resolves to a builtin
        parse_result(do_goto_definition(fixture_path("_fixture_goto.das"), "34", "5", "", "", "", false), text, is_error)
        t |> success(!is_error, "should not be error -- {cpp_diag(text)}")
        t |> success(find(text, "Kind: builtin") >= 0, "should resolve to a builtin -- {cpp_diag(text)}")
    }
    t |> run("redirect on — appends C++: line for builtin") <| @(t : T?) {
        var text : string
        var is_error = false
        // cpp_dirs="src/builtin" narrows the redirect lookup; drops this from ~17s to ~1s.
        parse_result(do_goto_definition(fixture_path("_fixture_goto.das"), "34", "5", "", "", "", true, "src/builtin"), text, is_error)
        t |> success(!is_error, "should not be error -- {cpp_diag(text)}")
        t |> success(find(text, "Kind: builtin") >= 0, "should still resolve to a builtin -- {cpp_diag(text)}")
        t |> success(find(text, "C++:") >= 0, "C++ line should appear when with_cpp_source=true -- {cpp_diag(text)}")
    }
}

// ── cpp_search_config + staleness signature ──────────────────────────

[test]
def test_cpp_search_config(t : T?) {
    t |> run("CPP_SEARCH_DIRS contains expected defaults") <| @(t : T?) {
        var saw_modules = false
        for (d in CPP_SEARCH_DIRS) {
            if (d == "modules") {
                saw_modules = true
            }
        }
        t |> success(saw_modules, "CPP_SEARCH_DIRS should include 'modules'")
    }
    t |> run("CPP_SEARCH_INCLUDE_GLOBS locks to *.cpp/*.h/*.hpp") <| @(t : T?) {
        let n = length(CPP_SEARCH_INCLUDE_GLOBS)
        t |> success(n >= 3, "should list at least 3 include globs")
    }
}

[test]
def test_cpp_index_signature(t : T?) {
    if (!require_ast_grep(t)) return
    t |> run("ensure_cpp_index produces a stable signature across consecutive calls (no edits)") <| @(t : T?) {
        ensure_cpp_index()
        let sig1 = cpp_index_signature
        ensure_cpp_index()
        let sig2 = cpp_index_signature
        t |> success(!empty(sig1), "first signature should not be empty after ensure_cpp_index()")
        t |> success(sig1 == sig2, "signature should be stable across consecutive ensure_cpp_index() calls (no fs changes)")
    }
}

// Direct unit tests for the filesystem-fallback walker live with the shared
// module they exercise: utils/common/tests/test_git_signature.das. Behavioral
// integration is covered by test_cpp_index_signature above.

[test]
def test_cpp_collect_git_excludes(t : T?) {
    t |> run("daspkg cache .git is auto-excluded if present") <| @(t : T?) {
        let modules_dir = path_join(get_das_root(), "modules")
        let cache_git = path_join(modules_dir, ".daspkg_cache/index/.git")
        if (!stat(cache_git).is_valid) {
            // skip silently if cache not present (e.g. fresh checkout)
            return
        }
        var search_dirs : array<string>
        search_dirs |> push(modules_dir)
        let no_overrides : array<string>
        let excludes <- cpp_collect_git_excludes(search_dirs, no_overrides)
        var found = false
        for (g in excludes) {
            if (find(g, ".daspkg_cache/index") >= 0) {
                found = true
            }
        }
        t |> success(found, "modules/.daspkg_cache/index should be in the auto-exclude list")
    }
}

[test]
def test_cpp_function_declarations_visible(t : T?) {
    if (!require_ast_grep(t)) return
    // Regression for Copilot review #1: function declarations in headers
    // (no body, e.g. `void foo();`) must be findable. Without the
    // cpp-outline-functions-decl rule + has:function_declarator constraint,
    // these were silently invisible.
    t |> run("finds function declaration in ast.h header") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_find_symbol("=pull_all_auto_registered_modules", "function", "include", ""), text, is_error)
        t |> success(!is_error, "should not be error -- {cpp_diag(text)}")
        t |> success(find(text, "ast.h") >= 0, "should locate declaration in include/daScript/ast/ast.h -- {cpp_diag(text)}")
    }
}

[test]
def test_cpp_using_alias_visible(t : T?) {
    if (!require_ast_grep(t)) return
    // Regression for Copilot review #2: `using X = Y;` aliases must surface
    // under kind=typedef. Without the cpp-outline-typedefs-using rule, only
    // the legacy `typedef X Y;` form was visible.
    t |> run("finds 'using' alias as typedef") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_find_symbol("module_pull_t", "typedef", "include", ""), text, is_error)
        t |> success(!is_error, "should not be error -- {cpp_diag(text)}")
        t |> success(find(text, "module_pull_t") >= 0, "should locate the using-alias -- {cpp_diag(text)}")
    }
}

// ── run_mcp_subtool wiring tests ──────────────────────────────────────
//
// The 270 tool-level tests above already cover the wiring end-to-end via
// `do_<tool>(...)`. These four pin down the four behaviors of the helper
// itself: argv passthrough, missing-subtool, timeout, and non-zero exit.
// Fixtures live alongside the real subtools at utils/mcp/subtools/_test_*.das
// so they share the same require-chain shape (no behavioral surprises).

[test]
def test_run_mcp_subtool_passthrough(t : T?) {
    t |> run("argv round-trips through `--` separator") <| @(t : T?) {
        let raw = run_mcp_subtool("_test_passthrough", ["alpha", "beta", "gamma"])
        var text : string
        var is_error = false
        parse_result(raw, text, is_error)
        t |> success(!is_error, "should not be error -- text={text}")
        t |> success(find(text, "PASSTHROUGH: alpha beta gamma") >= 0,
                     "args should round-trip verbatim -- text={text}")
    }
    t |> run("empty args list still works") <| @(t : T?) {
        let raw = run_mcp_subtool("_test_passthrough", [])
        var text : string
        var is_error = false
        parse_result(raw, text, is_error)
        t |> success(!is_error, "should not be error -- text={text}")
        t |> success(find(text, "PASSTHROUGH:") >= 0,
                     "marker present even with no args -- text={text}")
    }
}

[test]
def test_run_mcp_subtool_missing(t : T?) {
    t |> run("nonexistent subtool surfaces as error") <| @(t : T?) {
        let raw = run_mcp_subtool("_does_not_exist_xyzzy", [])
        var text : string
        var is_error = false
        parse_result(raw, text, is_error)
        t |> success(is_error, "should be error -- text={text}")
        t |> success(find(text, "_does_not_exist_xyzzy") >= 0,
                     "error should name the missing subtool -- text={text}")
    }
}

[test]
def test_run_mcp_subtool_timeout(t : T?) {
    t |> run("subtool exceeding timeout surfaces 'timed out'") <| @(t : T?) {
        // Sleep 5s, allow 0.5s — guaranteed timeout. Wall clock cost ~0.5s
        // (process is killed at the deadline, not after the full sleep).
        let raw = run_mcp_subtool("_test_sleep", ["5000"], [timeout_sec = 0.5])
        var text : string
        var is_error = false
        parse_result(raw, text, is_error)
        t |> success(is_error, "should be error -- text={text}")
        t |> success(find(text, "timed out") >= 0,
                     "error should mention 'timed out' -- text={text}")
    }
}

[test]
def test_run_mcp_subtool_nonzero_exit(t : T?) {
    t |> run("panicking subtool surfaces 'failed (exit ...)'") <| @(t : T?) {
        let raw = run_mcp_subtool("_test_panic", [])
        var text : string
        var is_error = false
        parse_result(raw, text, is_error)
        t |> success(is_error, "should be error -- text={text}")
        t |> success(find(text, "failed (exit") >= 0,
                     "error should report non-zero exit -- text={text}")
        t |> success(find(text, "STDOUT_BEFORE_PANIC") >= 0,
                     "captured stdout before the panic should be in the error payload -- text={text}")
    }
}

// ── project_root end-to-end coverage ────────────────────────────────
// Tier 1: every MCP tool that accepts `project_root` should resolve the
// pretend_mod fixture's `require pretend_mod/greet` chain when the arg is
// set, and fail with `missing prerequisite 'pretend_mod` when it isn't.
// Fixture: utils/mcp/tests/_pretend_root/{consumer.das, modules/pretend_mod/{.das_module, greet.das}}.

let PROOT_CONSUMER = "_pretend_root/consumer.das"
let PROOT = "_pretend_root"
let PROOT_MISSING = "missing prerequisite 'pretend_mod"

def assert_project_root_pair(t : T?; without_root, with_root : string) {
    var t1 : string; var e1 = false
    parse_result(without_root, t1, e1)
    t |> success(e1, "without project_root: should be error -- text={t1}")
    t |> success(find(t1, PROOT_MISSING) >= 0,
                 "without project_root: should mention missing prerequisite -- text={t1}")
    var t2 : string; var e2 = false
    parse_result(with_root, t2, e2)
    t |> success(!e2, "with project_root: should NOT be error -- text={t2}")
}

[test]
def test_project_root_compile_check(t : T?) {
    t |> run("compile_check honors project_root") <| @(t : T?) {
        assert_project_root_pair(t,
            do_compile_check(fixture_path(PROOT_CONSUMER)),
            do_compile_check(fixture_path(PROOT_CONSUMER), "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_lint(t : T?) {
    t |> run("lint honors project_root") <| @(t : T?) {
        assert_project_root_pair(t,
            do_lint(fixture_path(PROOT_CONSUMER)),
            do_lint(fixture_path(PROOT_CONSUMER), "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_aot(t : T?) {
    t |> run("aot honors project_root") <| @(t : T?) {
        assert_project_root_pair(t,
            do_aot(fixture_path(PROOT_CONSUMER), ""),
            do_aot(fixture_path(PROOT_CONSUMER), "", "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_list_functions(t : T?) {
    t |> run("list_functions honors project_root") <| @(t : T?) {
        assert_project_root_pair(t,
            do_list_functions(fixture_path(PROOT_CONSUMER)),
            do_list_functions(fixture_path(PROOT_CONSUMER), "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_list_types(t : T?) {
    t |> run("list_types honors project_root") <| @(t : T?) {
        assert_project_root_pair(t,
            do_list_types(fixture_path(PROOT_CONSUMER)),
            do_list_types(fixture_path(PROOT_CONSUMER), "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_list_requires(t : T?) {
    t |> run("list_requires honors project_root") <| @(t : T?) {
        assert_project_root_pair(t,
            do_list_requires(fixture_path(PROOT_CONSUMER)),
            do_list_requires(fixture_path(PROOT_CONSUMER), "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_program_log(t : T?) {
    t |> run("program_log honors project_root") <| @(t : T?) {
        assert_project_root_pair(t,
            do_program_log(fixture_path(PROOT_CONSUMER), "main"),
            do_program_log(fixture_path(PROOT_CONSUMER), "main", "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_ast_dump(t : T?) {
    t |> run("ast_dump honors project_root") <| @(t : T?) {
        assert_project_root_pair(t,
            do_ast_dump("", fixture_path(PROOT_CONSUMER), "main", "source", ""),
            do_ast_dump("", fixture_path(PROOT_CONSUMER), "main", "source", "", "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_find_references(t : T?) {
    t |> run("find_references honors project_root") <| @(t : T?) {
        assert_project_root_pair(t,
            do_find_references(fixture_path(PROOT_CONSUMER), "7", "17", "file", ""),
            do_find_references(fixture_path(PROOT_CONSUMER), "7", "17", "file", "", "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_goto_definition(t : T?) {
    t |> run("goto_definition honors project_root") <| @(t : T?) {
        assert_project_root_pair(t,
            do_goto_definition(fixture_path(PROOT_CONSUMER), "7", "17"),
            do_goto_definition(fixture_path(PROOT_CONSUMER), "7", "17", "", "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_type_of(t : T?) {
    t |> run("type_of honors project_root") <| @(t : T?) {
        assert_project_root_pair(t,
            do_type_of(fixture_path(PROOT_CONSUMER), "7", "17"),
            do_type_of(fixture_path(PROOT_CONSUMER), "7", "17", "", "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_find_symbol(t : T?) {
    t |> run("find_symbol honors project_root") <| @(t : T?) {
        assert_project_root_pair(t,
            do_find_symbol("hello", "", fixture_path(PROOT_CONSUMER)),
            do_find_symbol("hello", "", fixture_path(PROOT_CONSUMER), "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_list_module_api(t : T?) {
    t |> run("list_module_api honors project_root") <| @(t : T?) {
        // list_module_api compiles a stub `require <module_name>`, so the
        // missing-prereq error mentions the module key, not 'pretend_mod'.
        var t1 : string; var e1 = false
        parse_result(do_list_module_api("pretend_mod/greet"), t1, e1)
        t |> success(e1, "without project_root: should be error -- text={t1}")
        t |> success(find(t1, "missing prerequisite 'pretend_mod/greet") >= 0,
                     "without project_root: should mention missing prereq -- text={t1}")
        var t2 : string; var e2 = false
        parse_result(do_list_module_api("pretend_mod/greet", "", "", "", "", fixture_path(PROOT)), t2, e2)
        t |> success(!e2, "with project_root: should NOT be error -- text={t2}")
        t |> success(find(t2, "hello") >= 0,
                     "with project_root: should list hello() -- text={t2}")
    }
}

[test]
def test_project_root_describe_type(t : T?) {
    t |> run("describe_type honors project_root") <| @(t : T?) {
        // describe_type uses module_name in the stub require chain;
        // without project_root the require fails to resolve.
        var t1 : string; var e1 = false
        parse_result(do_describe_type("FooBar", "pretend_mod/greet"), t1, e1)
        t |> success(e1, "without project_root: should be error -- text={t1}")
        t |> success(find(t1, "missing prerequisite 'pretend_mod/greet") >= 0,
                     "without project_root: should mention missing prereq -- text={t1}")
        // FooBar doesn't exist; tool reports "Type 'FooBar' not found" but is_error=false
        // because the compile itself succeeded — that's the project_root flow we're proving.
        var t2 : string; var e2 = false
        parse_result(do_describe_type("FooBar", "pretend_mod/greet", "", fixture_path(PROOT)), t2, e2)
        t |> success(!e2, "with project_root: should NOT be error -- text={t2}")
    }
}

[test]
def test_project_root_eval_expression(t : T?) {
    t |> run("eval_expression honors project_root") <| @(t : T?) {
        // eval_expression has its own subprocess path (not via run_mcp_subtool),
        // so this test exercises that separate argv construction path too.
        var t1 : string; var e1 = false
        parse_result(do_eval_expression("hello()", "pretend_mod/greet"), t1, e1)
        t |> success(e1, "without project_root: should be error -- text={t1}")
        t |> success(find(t1, "missing prerequisite 'pretend_mod/greet") >= 0,
                     "without project_root: should mention missing prereq -- text={t1}")
        var t2 : string; var e2 = false
        parse_result(do_eval_expression("hello()", "pretend_mod/greet", "", fixture_path(PROOT)), t2, e2)
        t |> success(!e2, "with project_root: should NOT be error -- text={t2}")
    }
}

[test]
def test_project_root_run_script(t : T?) {
    t |> run("run_script honors project_root") <| @(t : T?) {
        // Use INLINE code (not file path). daslang.exe auto-deduces project_root
        // from files.front()'s directory (utils/daScript/main.cpp:390); when
        // run_script is given a file path, that's the consumer's own directory
        // which auto-resolves modules/. Inline code writes a temp stub at
        // get_das_root(), whose modules tree doesn't include pretend_mod.
        let inline_code = "options gen2\nrequire pretend_mod/greet\n[export]\ndef main() \{\n    print(\"\{hello()\}\\n\")\n\}\n"
        var t1 : string; var e1 = false
        parse_result(do_run_script("", inline_code), t1, e1)
        t |> success(e1, "without project_root: should be error -- text={t1}")
        t |> success(find(t1, PROOT_MISSING) >= 0,
                     "without project_root: should mention missing prereq -- text={t1}")
        var t2 : string; var e2 = false
        parse_result(do_run_script("", inline_code, "", false, "", fixture_path(PROOT)), t2, e2)
        t |> success(!e2, "with project_root: should NOT be error -- text={t2}")
    }
}

[test]
def test_project_root_run_test(t : T?) {
    t |> run("run_test honors project_root") <| @(t : T?) {
        // consumer.das has no [test] funcs, but the compile-failure vs
        // compile-succeed gate is what we're testing.
        var t1 : string; var e1 = false
        parse_result(do_run_test(fixture_path(PROOT_CONSUMER)), t1, e1)
        t |> success(e1, "without project_root: should be error -- text={t1}")
        t |> success(find(t1, PROOT_MISSING) >= 0,
                     "without project_root: should mention missing prereq -- text={t1}")
        var t2 : string; var e2 = false
        parse_result(do_run_test(fixture_path(PROOT_CONSUMER), "", fixture_path(PROOT)), t2, e2)
        // No [test] functions in consumer.das, so dastest reports "no tests"
        // but the compile succeeded. Some implementations may mark "no tests"
        // as is_error=true; assert based on actual observed behavior, not
        // is_error. The key check: no missing-prereq message.
        t |> success(find(t2, PROOT_MISSING) < 0,
                     "with project_root: should NOT mention missing prereq -- text={t2}")
    }
}

// ── load_module end-to-end coverage ──────────────────────────────────
// Tier-1 peer suite to the project_root suite above. Same pretend_mod
// fixture, but exercised via `-load_module <pretend_mod_path>` instead
// of `-project_root <_pretend_root>`. The two flags shadow each other
// by basename, so pointing load_module directly at the module folder
// (which contains `.das_module`) is the canonical alternative to the
// `<DummyRoot>/modules/<name>` junction trick.

let PRETEND_LM = "_pretend_root/modules/pretend_mod"

def make_lm_array(p : string) : array<string> {
    var arr : array<string>
    arr |> push(p)
    return <- arr
}

def assert_load_module_pair(t : T?; without_lm, with_lm : string) {
    var t1 : string; var e1 = false
    parse_result(without_lm, t1, e1)
    t |> success(e1, "without load_module: should be error -- text={t1}")
    t |> success(find(t1, PROOT_MISSING) >= 0,
                 "without load_module: should mention missing prerequisite -- text={t1}")
    var t2 : string; var e2 = false
    parse_result(with_lm, t2, e2)
    t |> success(!e2, "with load_module: should NOT be error -- text={t2}")
}

[test]
def test_load_module_compile_check(t : T?) {
    t |> run("compile_check honors load_module") <| @(t : T?) {
        assert_load_module_pair(t,
            do_compile_check(fixture_path(PROOT_CONSUMER)),
            do_compile_check(fixture_path(PROOT_CONSUMER), "", "", false, make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_lint(t : T?) {
    t |> run("lint honors load_module") <| @(t : T?) {
        assert_load_module_pair(t,
            do_lint(fixture_path(PROOT_CONSUMER)),
            do_lint(fixture_path(PROOT_CONSUMER), "", "", make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_aot(t : T?) {
    t |> run("aot honors load_module") <| @(t : T?) {
        assert_load_module_pair(t,
            do_aot(fixture_path(PROOT_CONSUMER), ""),
            do_aot(fixture_path(PROOT_CONSUMER), "", "", "", make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_list_functions(t : T?) {
    t |> run("list_functions honors load_module") <| @(t : T?) {
        assert_load_module_pair(t,
            do_list_functions(fixture_path(PROOT_CONSUMER)),
            do_list_functions(fixture_path(PROOT_CONSUMER), "", "", make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_list_types(t : T?) {
    t |> run("list_types honors load_module") <| @(t : T?) {
        assert_load_module_pair(t,
            do_list_types(fixture_path(PROOT_CONSUMER)),
            do_list_types(fixture_path(PROOT_CONSUMER), "", "", make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_list_requires(t : T?) {
    t |> run("list_requires honors load_module") <| @(t : T?) {
        assert_load_module_pair(t,
            do_list_requires(fixture_path(PROOT_CONSUMER)),
            do_list_requires(fixture_path(PROOT_CONSUMER), "", "", false, make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_program_log(t : T?) {
    t |> run("program_log honors load_module") <| @(t : T?) {
        assert_load_module_pair(t,
            do_program_log(fixture_path(PROOT_CONSUMER), "main"),
            do_program_log(fixture_path(PROOT_CONSUMER), "main", "", "", make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_ast_dump(t : T?) {
    t |> run("ast_dump honors load_module") <| @(t : T?) {
        assert_load_module_pair(t,
            do_ast_dump("", fixture_path(PROOT_CONSUMER), "main", "source", ""),
            do_ast_dump("", fixture_path(PROOT_CONSUMER), "main", "source", "", "", "", make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_find_references(t : T?) {
    t |> run("find_references honors load_module") <| @(t : T?) {
        assert_load_module_pair(t,
            do_find_references(fixture_path(PROOT_CONSUMER), "7", "17", "file", ""),
            do_find_references(fixture_path(PROOT_CONSUMER), "7", "17", "file", "", "", "", make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_goto_definition(t : T?) {
    t |> run("goto_definition honors load_module") <| @(t : T?) {
        assert_load_module_pair(t,
            do_goto_definition(fixture_path(PROOT_CONSUMER), "7", "17"),
            do_goto_definition(fixture_path(PROOT_CONSUMER), "7", "17", "", "", "", false, "", make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_type_of(t : T?) {
    t |> run("type_of honors load_module") <| @(t : T?) {
        assert_load_module_pair(t,
            do_type_of(fixture_path(PROOT_CONSUMER), "7", "17"),
            do_type_of(fixture_path(PROOT_CONSUMER), "7", "17", "", "", "", make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_find_symbol(t : T?) {
    t |> run("find_symbol honors load_module") <| @(t : T?) {
        assert_load_module_pair(t,
            do_find_symbol("hello", "", fixture_path(PROOT_CONSUMER)),
            do_find_symbol("hello", "", fixture_path(PROOT_CONSUMER), "", "", false, "", make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_list_module_api(t : T?) {
    t |> run("list_module_api honors load_module") <| @(t : T?) {
        var t1 : string; var e1 = false
        parse_result(do_list_module_api("pretend_mod/greet"), t1, e1)
        t |> success(e1, "without load_module: should be error -- text={t1}")
        t |> success(find(t1, "missing prerequisite 'pretend_mod/greet") >= 0,
                     "without load_module: should mention missing prereq -- text={t1}")
        var t2 : string; var e2 = false
        parse_result(do_list_module_api("pretend_mod/greet", "", "", "", "", "", make_lm_array(fixture_path(PRETEND_LM))), t2, e2)
        t |> success(!e2, "with load_module: should NOT be error -- text={t2}")
        t |> success(find(t2, "hello") >= 0,
                     "with load_module: should list hello() -- text={t2}")
    }
}

[test]
def test_load_module_describe_type(t : T?) {
    t |> run("describe_type honors load_module") <| @(t : T?) {
        var t1 : string; var e1 = false
        parse_result(do_describe_type("FooBar", "pretend_mod/greet"), t1, e1)
        t |> success(e1, "without load_module: should be error -- text={t1}")
        t |> success(find(t1, "missing prerequisite 'pretend_mod/greet") >= 0,
                     "without load_module: should mention missing prereq -- text={t1}")
        var t2 : string; var e2 = false
        parse_result(do_describe_type("FooBar", "pretend_mod/greet", "", "", make_lm_array(fixture_path(PRETEND_LM))), t2, e2)
        t |> success(!e2, "with load_module: should NOT be error -- text={t2}")
    }
}

[test]
def test_load_module_eval_expression(t : T?) {
    t |> run("eval_expression honors load_module") <| @(t : T?) {
        var t1 : string; var e1 = false
        parse_result(do_eval_expression("hello()", "pretend_mod/greet"), t1, e1)
        t |> success(e1, "without load_module: should be error -- text={t1}")
        t |> success(find(t1, "missing prerequisite 'pretend_mod/greet") >= 0,
                     "without load_module: should mention missing prereq -- text={t1}")
        var t2 : string; var e2 = false
        parse_result(do_eval_expression("hello()", "pretend_mod/greet", "", "", make_lm_array(fixture_path(PRETEND_LM))), t2, e2)
        t |> success(!e2, "with load_module: should NOT be error -- text={t2}")
    }
}

[test]
def test_load_module_run_script(t : T?) {
    t |> run("run_script honors load_module") <| @(t : T?) {
        let inline_code = "options gen2\nrequire pretend_mod/greet\n[export]\ndef main() \{\n    print(\"\{hello()\}\\n\")\n\}\n"
        var t1 : string; var e1 = false
        parse_result(do_run_script("", inline_code), t1, e1)
        t |> success(e1, "without load_module: should be error -- text={t1}")
        t |> success(find(t1, PROOT_MISSING) >= 0,
                     "without load_module: should mention missing prereq -- text={t1}")
        var t2 : string; var e2 = false
        parse_result(do_run_script("", inline_code, "", false, "", "", make_lm_array(fixture_path(PRETEND_LM))), t2, e2)
        t |> success(!e2, "with load_module: should NOT be error -- text={t2}")
    }
}

[test]
def test_load_module_run_test(t : T?) {
    t |> run("run_test honors load_module") <| @(t : T?) {
        var t1 : string; var e1 = false
        parse_result(do_run_test(fixture_path(PROOT_CONSUMER)), t1, e1)
        t |> success(e1, "without load_module: should be error -- text={t1}")
        t |> success(find(t1, PROOT_MISSING) >= 0,
                     "without load_module: should mention missing prereq -- text={t1}")
        var t2 : string; var e2 = false
        parse_result(do_run_test(fixture_path(PROOT_CONSUMER), "", "", "", false, false, make_lm_array(fixture_path(PRETEND_LM))), t2, e2)
        t |> success(find(t2, PROOT_MISSING) < 0,
                     "with load_module: should NOT mention missing prereq -- text={t2}")
    }
}

// ── build_live_script_args unit tests ────────────────────────────────
// Pins daslang-live script_args composition. Flag-ordering matters because
// when both `-project` and `-project_root` are set, `-project_root` must end
// up to the LEFT (prepended last by build_live_script_args). Slash-replace
// only happens when is_windows=true.

[test]
def test_live_args_neither(t : T?) {
    t |> run("no project, no project_root: just -cwd") <| @(t : T?) {
        let s = build_live_script_args("/path/to/foo.das", "", "", false)
        t |> equal(s, "-cwd \"/path/to/foo.das\"", "should be just -cwd")
    }
}

[test]
def test_live_args_project_only(t : T?) {
    t |> run("project set, project_root empty: -project before -cwd") <| @(t : T?) {
        let s = build_live_script_args("/path/foo.das", "/proj/P.das_project", "", false)
        t |> equal(s, "-project \"/proj/P.das_project\" -cwd \"/path/foo.das\"",
                   "should prepend -project before -cwd")
    }
}

[test]
def test_live_args_project_root_only(t : T?) {
    t |> run("project_root set, project empty: -project_root before -cwd") <| @(t : T?) {
        let s = build_live_script_args("/path/foo.das", "", "/proj_root", false)
        t |> equal(s, "-project_root \"/proj_root\" -cwd \"/path/foo.das\"",
                   "should prepend -project_root before -cwd")
    }
}

[test]
def test_live_args_both(t : T?) {
    t |> run("both set: -project_root before -project before -cwd") <| @(t : T?) {
        let s = build_live_script_args("/path/foo.das", "/p.das_project", "/proj_root", false)
        t |> equal(s, "-project_root \"/proj_root\" -project \"/p.das_project\" -cwd \"/path/foo.das\"",
                   "project_root prepended last ends up leftmost")
    }
}

[test]
def test_live_args_load_module_only(t : T?) {
    t |> run("single load_module: -load_module before -cwd") <| @(t : T?) {
        var lm : array<string>
        lm |> push("/p1")
        let s = build_live_script_args("/path/foo.das", "", "", false, lm)
        t |> equal(s, "-load_module \"/p1\" -cwd \"/path/foo.das\"",
                   "single load_module pair between project_root slot and -cwd")
    }
}

[test]
def test_live_args_load_module_multi(t : T?) {
    t |> run("two load_modules: both pairs preserved in source order") <| @(t : T?) {
        var lm : array<string>
        lm |> push("/p1")
        lm |> push("/p2")
        let s = build_live_script_args("/path/foo.das", "", "", false, lm)
        t |> equal(s, "-load_module \"/p1\" -load_module \"/p2\" -cwd \"/path/foo.das\"",
                   "two load_module pairs preserved in source order")
    }
}

[test]
def test_live_args_all_set(t : T?) {
    t |> run("project_root + load_modules + project + cwd: ordered project_root → load_module → project → cwd") <| @(t : T?) {
        var lm : array<string>
        lm |> push("/m1")
        lm |> push("/m2")
        let s = build_live_script_args("/path/foo.das", "/p.das_project", "/proj_root", false, lm)
        t |> equal(s,
                   "-project_root \"/proj_root\" -load_module \"/m1\" -load_module \"/m2\" -project \"/p.das_project\" -cwd \"/path/foo.das\"",
                   "full order: project_root, then load_modules, then project, then cwd")
    }
}

[test]
def test_live_args_load_module_windows_paths(t : T?) {
    t |> run("is_windows=true: load_module paths get slash-replaced too") <| @(t : T?) {
        var lm : array<string>
        lm |> push("D:/mods/dasImgui")
        let s = build_live_script_args("D:/path/foo.das", "", "", true, lm)
        t |> equal(s, "-load_module \"D:\\mods\\dasImgui\" -cwd \"D:\\path\\foo.das\"",
                   "windows: load_module paths slash-replaced like project_root")
    }
}

[test]
def test_live_args_windows_paths(t : T?) {
    t |> run("is_windows=true: forward slashes become backslashes") <| @(t : T?) {
        let s = build_live_script_args("D:/path/foo.das", "D:/p.das_project", "D:/root", true)
        t |> equal(s, "-project_root \"D:\\root\" -project \"D:\\p.das_project\" -cwd \"D:\\path\\foo.das\"",
                   "windows: slashes replaced in all three quoted paths")
    }
    t |> run("is_windows=false: forward slashes preserved") <| @(t : T?) {
        let s = build_live_script_args("D:/path/foo.das", "", "", false)
        t |> success(find(s, "/path/foo.das") >= 0, "non-windows: keep forward slashes -- s={s}")
        t |> success(find(s, "\\path") < 0, "non-windows: no backslash conversion -- s={s}")
    }
}

// ── resolve_load_modules unit tests ──────────────────────────────────
// Pins the abs-resolve invariant: every load_module path passed to
// daslang-live MUST be absolute, because daslang-live's `-cwd` chdir's
// before `require_dynamic_modules` runs (utils/daslang-live/main.cpp:696-707).
// A relative -load_module would re-resolve against the new cwd and fail to
// find the module folder — same hazard as -project_root.

[test]
def test_resolve_load_modules_empty(t : T?) {
    t |> run("empty input yields empty output") <| @(t : T?) {
        let src : array<string>
        let out <- resolve_load_modules(src)
        t |> equal(length(out), 0, "empty in, empty out")
    }
}

[test]
def test_resolve_load_modules_absolutizes_relative(t : T?) {
    t |> run("relative paths become absolute") <| @(t : T?) {
        var src : array<string>
        src |> push("foo")
        src |> push("./bar/baz")
        let out <- resolve_load_modules(src)
        t |> equal(length(out), 2, "preserves entry count")
        // resolve_path uses get_das_root() as the base for relative paths.
        // The exact prefix is platform-specific; the invariant is that the
        // output contains the das root as a prefix (or starts with a drive
        // letter / leading slash on POSIX).
        let root = get_das_root()
        t |> success(find(out[0], root) >= 0,
                     "out[0] should contain das root prefix -- out[0]={out[0]} root={root}")
        t |> success(find(out[1], root) >= 0,
                     "out[1] should contain das root prefix -- out[1]={out[1]} root={root}")
    }
}

[test]
def test_resolve_load_modules_preserves_absolute(t : T?) {
    t |> run("already-absolute paths pass through unchanged") <| @(t : T?) {
        var src : array<string>
        let is_windows = get_platform_name() == "windows"
        let abs1 = is_windows ? "D:/already/abs/one" : "/already/abs/one"
        let abs2 = is_windows ? "D:/already/abs/two" : "/already/abs/two"
        src |> push(abs1)
        src |> push(abs2)
        let out <- resolve_load_modules(src)
        t |> equal(length(out), 2, "preserves entry count")
        t |> equal(out[0], abs1, "absolute path passes through unchanged")
        t |> equal(out[1], abs2, "absolute path passes through unchanged")
    }
}

// ── build_subtool_argv unit tests ────────────────────────────────────
// Pins the `-project_root <P>` argv invariant: flag MUST be inserted BEFORE
// the subtool script path. Otherwise daslang treats it as a positional .das
// file to load, which silently breaks dyn-module discovery.

[test]
def test_build_subtool_argv_no_project_root(t : T?) {
    t |> run("argv without project_root: [exe, script, --, ...args]") <| @(t : T?) {
        let args <- ["alpha", "beta"]
        let argv <- build_subtool_argv("daslang.exe", "sub.das", args, "")
        t |> equal(length(argv), 5, "argv length should be 5")
        t |> equal(argv[0], "daslang.exe", "argv[0]=exe")
        t |> equal(argv[1], "sub.das", "argv[1]=script_path (no flag before it)")
        t |> equal(argv[2], "--", "argv[2]=--")
        t |> equal(argv[3], "alpha", "argv[3]=alpha")
        t |> equal(argv[4], "beta", "argv[4]=beta")
    }
}

[test]
def test_build_subtool_argv_with_project_root(t : T?) {
    t |> run("argv with project_root: flag inserted BEFORE script path") <| @(t : T?) {
        // build_subtool_argv does pure string composition — the path doesn't
        // need to exist on the test host, just be passed through verbatim.
        let args <- ["alpha"]
        let argv <- build_subtool_argv("daslang.exe", "sub.das", args, "/proj/root")
        t |> equal(length(argv), 6, "argv length should be 6")
        t |> equal(argv[0], "daslang.exe", "argv[0]=exe")
        t |> equal(argv[1], "-project_root", "argv[1]=-project_root flag")
        t |> equal(argv[2], "/proj/root", "argv[2]=project_root path")
        t |> equal(argv[3], "sub.das", "argv[3]=script_path (AFTER the flag)")
        t |> equal(argv[4], "--", "argv[4]=--")
        t |> equal(argv[5], "alpha", "argv[5]=alpha")
    }
}

[test]
def test_build_subtool_argv_empty_args(t : T?) {
    t |> run("empty args still yields valid argv ending in --") <| @(t : T?) {
        let args : array<string>
        let argv <- build_subtool_argv("daslang.exe", "sub.das", args, "")
        t |> equal(length(argv), 3, "argv length should be 3")
        t |> equal(argv[0], "daslang.exe", "argv[0]=exe")
        t |> equal(argv[1], "sub.das", "argv[1]=script")
        t |> equal(argv[2], "--", "argv[2]=--")
    }
    t |> run("empty args + project_root still yields valid argv") <| @(t : T?) {
        let args : array<string>
        let argv <- build_subtool_argv("daslang.exe", "sub.das", args, "/root")
        t |> equal(length(argv), 5, "argv length should be 5")
        t |> equal(argv[3], "sub.das", "script comes after flag")
        t |> equal(argv[4], "--", "trailing -- still present")
    }
}

[test]
def test_build_subtool_argv_with_load_module(t : T?) {
    t |> run("single load_module pair inserted before script path") <| @(t : T?) {
        let args <- ["alpha"]
        var lm : array<string>
        lm |> push("/mod1")
        let argv <- build_subtool_argv("daslang.exe", "sub.das", args, "", lm)
        t |> equal(length(argv), 6, "argv length should be 6")
        t |> equal(argv[0], "daslang.exe", "argv[0]=exe")
        t |> equal(argv[1], "-load_module", "argv[1]=-load_module flag")
        t |> equal(argv[2], "/mod1", "argv[2]=load_module path")
        t |> equal(argv[3], "sub.das", "argv[3]=script_path (AFTER load_module)")
        t |> equal(argv[4], "--", "argv[4]=--")
        t |> equal(argv[5], "alpha", "argv[5]=user arg")
    }
}

[test]
def test_build_subtool_argv_with_load_modules_multi(t : T?) {
    t |> run("two load_module pairs preserved in source order") <| @(t : T?) {
        let args : array<string>
        var lm : array<string>
        lm |> push("/m1")
        lm |> push("/m2")
        let argv <- build_subtool_argv("daslang.exe", "sub.das", args, "", lm)
        t |> equal(length(argv), 7, "argv length should be 7")
        t |> equal(argv[1], "-load_module", "argv[1]=first -load_module flag")
        t |> equal(argv[2], "/m1", "argv[2]=first path")
        t |> equal(argv[3], "-load_module", "argv[3]=second -load_module flag")
        t |> equal(argv[4], "/m2", "argv[4]=second path")
        t |> equal(argv[5], "sub.das", "argv[5]=script (AFTER both load_module pairs)")
    }
}

[test]
def test_build_subtool_argv_with_project_root_and_load_module(t : T?) {
    t |> run("project_root + load_module: project_root first, then load_module, then script") <| @(t : T?) {
        let args : array<string>
        var lm : array<string>
        lm |> push("/m1")
        let argv <- build_subtool_argv("daslang.exe", "sub.das", args, "/proj_root", lm)
        t |> equal(length(argv), 7, "argv length should be 7")
        t |> equal(argv[1], "-project_root", "argv[1]=-project_root flag")
        t |> equal(argv[2], "/proj_root", "argv[2]=project_root path")
        t |> equal(argv[3], "-load_module", "argv[3]=-load_module flag (AFTER project_root)")
        t |> equal(argv[4], "/m1", "argv[4]=load_module path")
        t |> equal(argv[5], "sub.das", "argv[5]=script_path (AFTER all flags)")
        t |> equal(argv[6], "--", "argv[6]=--")
    }
}

// ── program_log smoke tests ──────────────────────────────────────────
// Pre-existing gap (no tests existed for program_log). Locks basic behavior
// after the in-process-to-subprocess refactor in commit 8eb959ed5.

[test]
def test_program_log_basic(t : T?) {
    t |> run("program_log returns post-compile program text for a named function") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_program_log(fixture_path("_fixture_valid.das"), "main"), text, is_error)
        t |> success(!is_error, "should not be error -- text={text}")
        t |> success(find(text, "def public main") >= 0,
                     "should contain post-infer main signature -- text={text}")
    }
    t |> run("program_log with empty func dumps full program") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_program_log(fixture_path("_fixture_valid.das"), ""), text, is_error)
        t |> success(!is_error, "should not be error -- text={text}")
        t |> success(find(text, "def public greet") >= 0,
                     "should list greet -- text={text}")
        t |> success(find(text, "def public main") >= 0,
                     "should list main -- text={text}")
    }
}

[test]
def test_program_log_missing_func(t : T?) {
    t |> run("program_log reports unfound function") <| @(t : T?) {
        // program_log returns is_error=false but message text indicates failure.
        // This is the current contract — pin it.
        var text : string
        var is_error = false
        parse_result(do_program_log(fixture_path("_fixture_valid.das"),
                                    "no_such_function_xyzzy"),
                     text, is_error)
        t |> success(find(text, "not found") >= 0,
                     "should report 'not found' for bogus function -- text={text}")
    }
}

[test]
def test_project_root_invalid_dir(t : T?) {
    t |> run("invalid project_root: pins current daslang behavior") <| @(t : T?) {
        // Locks in whatever daslang.exe does when -project_root points at a
        // nonexistent directory: today it silently no-ops the dyn-modules scan
        // and the consumer compile fails with missing-prereq. If daslang ever
        // changes to error explicitly on bad -project_root, this test catches
        // the behavior shift.
        var text : string
        var is_error = false
        parse_result(do_compile_check(fixture_path(PROOT_CONSUMER), "",
                                      fixture_path("nonexistent_xyz_pretend_root")),
                     text, is_error)
        t |> success(is_error, "should be error -- text={text}")
        t |> success(find(text, PROOT_MISSING) >= 0,
                     "should still report missing prereq -- text={text}")
    }
}
