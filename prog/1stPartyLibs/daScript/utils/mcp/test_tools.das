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
require daslib/das_source_formatter
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
require tools/common public
require tools/cpp_common public
require tools/outline public
require tools/cpp_grep_usage public
require tools/cpp_find_symbol public
require tools/cpp_outline public
require tools/cpp_goto_definition public
require tools/cpp_compile_check public
require tools/cpp_format_file public
require tools/cpp_status public
require tools/aot public
require tools/lint_tool public
require tools/program_log public
require tools/live public
require tools/detect_duplicates public
require tools/export_corpus public
require tools/arch_of public
require tools/arch_sites public
require tools/discover public

def parse_result(result : string; var text : string&; var is_error : bool&) : bool {
    var err : string
    var parsed = read_json(result, err)
    if (parsed == null) return false
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

def private require_ast_grep(t : T?) : bool {
    if (!empty(get_ast_grep_cmd())) return true
    t->log("ast-grep not installed — skipping test (install @ast-grep/cli to enable)")
    return false
}


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


[test]
def test_run_script_inline(t : T?) {
    t |> run("inline code executes") <| @(t : T?) {
        var text : string
        var is_error = false
        let code = "options gen2\n[export]\ndef main() \{\n    print(\"hello_mcp_test\\n\")\n\}\n"
        parse_result(do_run_script("", code), text, is_error)
        t->log("output assertions disabled: popen stdout capture is broken under dastest on Windows")
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


[test]
def test_run_test_passing(t : T?) {
    t |> run("passing test returns success") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_run_test(fixture_path("_fixture_test.das")), text, is_error)
        t->log("result assertions disabled: popen stdout capture is broken under dastest on Windows")
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
    t |> run("a module this program compiled from das is not a builtin, and the registered paths are a section") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_list_modules(true), text, is_error)
        var err : string
        var parsed = read_json(text, err)
        t |> success(parsed != null, "json parses: {err}")
        return if (parsed == null)
        t |> success(json_array_has(parsed?["builtin"], "math"), "math is a builtin")
        t |> success(!json_array_has(parsed?["builtin"], "json_boost"), "json_boost, which this program compiled, is not listed as a builtin")
        t |> success(json_array_has(parsed?["daslib"], "json_boost"), "json_boost is a daslib module")
        t |> success(parsed?["registered"] != null, "the registered require paths are listed")
        unsafe { delete parsed; }
    }
}

def private json_array_has(arr : JsonValue?; wanted : string) : bool {
    if (arr == null || !(arr.value is _array)) return false
    for (m in arr.value as _array) {
        if (m != null && m.value is _string && (m.value as _string) == wanted) return true
    }
    return false
}

def private project_root_fixture() : string {
    return path_join(get_das_root(), "utils/mcp/tests/_pretend_root")
}

//! the pretend root's second module folder: its descriptor registers the tree's UnitTest C++ module
//! and a require path of its own, and every tool asked with project_root sees both - as does a plain run
[test]
def test_project_root_fixture(t : T?) {
    let root = project_root_fixture()
    let script = path_join(root, "probe_consumer.das")
    t |> run("list_modules with project_root lists the descriptor's C++ module and require path") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_list_modules(true, root), text, is_error)
        t |> success(!is_error, "should not be error: {text}")
        var err : string
        var parsed = read_json(text, err)
        t |> success(parsed != null, "json parses: {err}")
        return if (parsed == null)
        t |> success(json_array_has(parsed?["builtin"], "UnitTest"), "the C++ module the descriptor registered is a builtin")
        t |> success(json_array_has(parsed?["registered"], "unit_probe/hello"), "the require path the descriptor registered is listed")
        unsafe { delete parsed; }
    }
    t |> run("find_symbol with project_root finds a symbol from the project's own require path") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_find_symbol("probe_hello_value", "", script, "", root), text, is_error)
        t |> success(!is_error, "should not be error: {text}")
        t |> success(find(text, "probe_hello_value") >= 0, "should find the project's function")
    }
    t |> run("compile_check with project_root compiles a file requiring both") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_compile_check(script, "", root), text, is_error)
        t |> success(!is_error, "should not be error: {text}")
        t |> success(find(text, "Compilation OK") >= 0, "should compile: {text}")
    }
    t |> run("a plain run and an eager run of the file both reach the C++ module and the require path") <| @(t : T?) {
        let exe = get_daslang_exe()
        t |> success(!empty(exe), "daslang executable is known")
        return if (empty(exe))
        var plain_out : string
        let plain_rc = run_and_capture([exe, "-project_root", root, script], plain_out, 120.0)
        t |> equal(plain_rc, 0, "plain run exits 0: {plain_out}")
        t |> success(find(plain_out, "probe 4242 1234") >= 0, "plain run prints the probe: {plain_out}")
        var eager_out : string
        let eager_rc = run_and_capture([exe, "-ignore-manifest", "-project_root", root, script], eager_out, 120.0)
        t |> equal(eager_rc, 0, "eager run exits 0: {eager_out}")
        t |> success(find(eager_out, "probe 4242 1234") >= 0, "eager run prints the probe: {eager_out}")
    }
}


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

        t |> success(!empty(parsed.value as _array), "array should contain at least one result")
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

}

[test]
def test_format_file_batches(t : T?) {
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


[test]
def test_format_strip_comments_source(t : T?) {
    t |> run("keeps header, docs, fmt directives, nolint; strips the rest") <| @(t : T?) {
        let src = fread(fixture_path("_fixture_comments.das"))
        t |> success(!empty(src), "fixture readable")
        let stripped = format_source_string(src, true)
        t |> success(find(stripped, "// header block stays") >= 0, "header block kept")
        t |> success(find(stripped, "//! doc comment survives") >= 0, "doc comment kept")
        t |> success(find(stripped, "nolint:STYLE005") >= 0, "nolint suppression kept with its why")
        t |> success(find(stripped, "@nolint") >= 0, "@nolint form kept")
        t |> success(find(stripped, "narration") < 0, "narration stripped")
        t |> success(find(stripped, "trailing dies") < 0, "trailing comment stripped")
        t |> success(find(stripped, "block dies") < 0, "block comment stripped")
        t |> success(find(stripped, "let z = x + y") >= 0, "multi-line inline comment joins its expression")
    }
    t |> run("license comments survive the strip anywhere in the file") <| @(t : T?) {
        let src = "options gen2\n\ndef mid \{\n    return 1\n\}\n\n// Copyright (c) 2008 Bjoern Hoehrmann\n// SPDX-License-Identifier: MIT\n// COPYRIGHT 2026 SHOUTING NOTICES TOO\ndef tail \{\n    return 2\n\}\n"
        let stripped = format_source_string(src, true)
        t |> success(find(stripped, "Copyright (c) 2008") >= 0, "Copyright line kept mid-file")
        t |> success(find(stripped, "SPDX-License-Identifier: MIT") >= 0, "SPDX line kept mid-file")
        t |> success(find(stripped, "COPYRIGHT 2026") >= 0, "match is case-insensitive")
    }
    t |> run("a fmt directive ends a kept continuation chain") <| @(t : T?) {
        let src = "options gen2\n\ndef head \{\n    return 1\n\}\n\n//! documented contract\n//fmt:keep-blank-lines\n// narration dies here\ndef tail \{\n    return 2\n\}\n"
        let stripped = format_source_string(src, true)
        t |> success(find(stripped, "documented contract") >= 0, "doc kept")
        t |> success(find(stripped, "fmt:keep-blank-lines") >= 0, "directive kept")
        t |> success(find(stripped, "narration dies") < 0, "narration after the directive is not a continuation")
    }
    t |> run("strip-disabled path leaves every comment in place") <| @(t : T?) {
        let src = fread(fixture_path("_fixture_comments.das"))
        let formatted = format_source_string(src, false)
        t |> success(find(formatted, "narration dies") >= 0, "narration kept when not stripping")
        t |> success(find(formatted, "block dies") >= 0, "block comment kept when not stripping")
    }
    t |> run("//fmt:ignore-file opts out of stripping too") <| @(t : T?) {
        let src = "//fmt:ignore-file\nlet X = 1\n// narration survives ignore\nlet Y = 2\n"
        let out = format_source_string(src, true)
        t |> success(find(out, "narration survives ignore") >= 0, "ignore-file blocks the strip")
    }
    t |> run("header under options survives; a stripped banner leaves no blank-line scar") <| @(t : T?) {
        let src = "options gen2\n\n// header under options stays\n\nrequire strings // trailing narration dies\n\ndef a() \{\n    return\n\}\n\n\n// banner dies\n\n\ndef b() \{\n    return\n\}\n"
        let out = format_source_string(src, true)
        t |> success(find(out, "header under options stays") >= 0, "header kept below the options line")
        t |> success(find(out, "trailing narration") < 0, "trailing narration on a require line is not header")
        t |> success(find(out, "banner") < 0, "banner stripped")
        t |> success(find(out, "\n\n\n\n") < 0, "no 3+ blank run at the strip site")
    }
    t |> run("//fmt: directives inside code survive a strip") <| @(t : T?) {
        let src = "options gen2\n\ndef a() \{\n    //fmt:keep-alignment\n    // narration dies\n    return\n\}\n"
        let out = format_source_string(src, true)
        t |> success(find(out, "//fmt:keep-alignment") >= 0, "fmt directive kept")
        t |> success(find(out, "narration") < 0, "narration stripped")
    }
    t |> run("a kept comment keeps its attached continuation lines, whole or not at all") <| @(t : T?) {
        let src = "options gen2\n\ndef a() \{\n    // nolint:STYLE014 — the why starts here and\n    // continues on a second line without truncation\n    return\n\}\n\ndef b() \{\n    //! doc head\n    // doc continuation line\n    return\n\}\n"
        let out = format_source_string(src, true)
        t |> success(find(out, "continues on a second line") >= 0, "nolint why kept whole")
        t |> success(find(out, "doc continuation line") >= 0, "//! continuation kept")
    }
    t |> run("the chain extends through a third line, and a block comment never continues it") <| @(t : T?) {
        let src = "options gen2\n\ndef a() \{\n    // nolint:STYLE014 — line one of the why,\n    // line two of the why,\n    // line three of the why\n    return\n\}\n\ndef b() \{\n    //! doc head\n    /* block narration dies */\n    return\n\}\n"
        let out = format_source_string(src, true)
        t |> success(find(out, "line three of the why") >= 0, "third continuation line kept")
        t |> success(find(out, "block narration") < 0, "a /* */ under a kept head is not a continuation")
    }
    t |> run("a blank line or code breaks the continuation chain") <| @(t : T?) {
        let src = "options gen2\n\ndef a() \{\n    // nolint:STYLE014 — one-line why\n\n    // separate narration dies\n    return  // nolint:STYLE014 — trailing why\n    // not a continuation of a trailing head, dies\n\}\n"
        let out = format_source_string(src, true)
        t |> success(find(out, "one-line why") >= 0, "head kept")
        t |> success(find(out, "separate narration") < 0, "blank line breaks the chain")
        t |> success(find(out, "trailing why") >= 0, "trailing nolint kept")
        t |> success(find(out, "not a continuation") < 0, "trailing head opens no chain")
    }
    t |> run("!is-prefixed identifiers survive formatting untouched") <| @(t : T?) {
        let src = "options gen2\n\ndef check(f : bool) : bool \{\n    return !is_unary(f)\n\}\n"
        let out = format_source_string(src, false)
        t |> success(find(out, "!is_unary(f)") >= 0,
            "template matching has no word boundary: a '!is'/'!as' entry in greedy_match_safe_tokens would eat the prefix of !is_unary-style unary-not calls - those two tokens must stay OUT of the list")
    }
}

def private make_policy_dir(name, config : string; var err : string&) : string {
    let dir = create_temp_directory(name, err)
    return "" if (empty(dir))
    mkdir(path_join(dir, ".git"))
    if (!empty(config)) {
        fwrite(path_join(dir, ".lint_config"), config)
    }
    return dir
}

[test]
def test_format_file_comment_policy(t : T?) {
    t |> run("force_clean_comments folder strips, and the compile verify passes") <| @(t : T?) {
        var terr : string
        let dir = make_policy_dir("mcp_strip_test", "[format]\nforce_clean_comments = true\n", terr)
        if (empty(dir)) {
            t |> failure("temp dir: {terr}")
            return
        }
        let dst = path_join(dir, "victim.das")
        let original = fread(fixture_path("_fixture_comments.das"))
        t |> success(fwrite(dst, original), "fixture copied")
        var text : string
        var is_error = false
        let ok = parse_result(do_format_file(dst), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(find(text, "comments stripped") >= 0, "opted-in folder stripped -- text={text}")
        t |> success(find(text, "rescue_advisory") >= 0, "strip batch appends the rescue advisory -- text={text}")
        t |> success(find(text, "harvester") >= 0, "advisory names the harvester agent")
        let after = fread(dst)
        t |> success(find(after, "narration") < 0, "comments removed on disk")
        t |> success(find(after, "//! doc comment survives") >= 0, "docs kept on disk")
        rmdir_rec(dir)
    }
    t |> run("a reformat that strips no comments raises no advisory") <| @(t : T?) {
        var terr : string
        let dir = make_policy_dir("mcp_nostrip_test", "[format]\nforce_clean_comments = true\n", terr)
        if (empty(dir)) {
            t |> failure("temp dir: {terr}")
            return
        }
        let dst = path_join(dir, "spacing.das")
        fwrite(dst, "options gen2\n\ndef f() \{\n    let x=1\n    return x\n\}\n")
        var text : string
        var is_error = false
        parse_result(do_format_file(dst), text, is_error)
        t |> success(find(text, "\"status\":\"formatted\"") >= 0, "spacing-only file was reformatted -- text={text}")
        t |> success(find(text, "rescue_advisory") < 0, "no comments deleted, no advisory")
        rmdir_rec(dir)
    }
    t |> run("fail-closed: a strip whose compile_check fails restores the original bytes") <| @(t : T?) {
        var terr : string
        let dir = make_policy_dir("mcp_revert_test", "[format]\nforce_clean_comments = true\n", terr)
        if (empty(dir)) {
            t |> failure("temp dir: {terr}")
            return
        }
        let dst = path_join(dir, "broken.das")
        let src = "options gen2\n\ndef broken() \{\n    // narration to strip\n    return undefined_symbol_xyz\n\}\n"
        t |> success(fwrite(dst, src), "broken source written")
        var text : string
        var is_error = false
        let ok = parse_result(do_format_file(dst), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(is_error, "reverted outcome reports as error")
        t |> success(find(text, "reverted") >= 0, "status is reverted -- text={text}")
        t |> success(find(text, "rescue_advisory") < 0, "a reverted strip keeps its comments — no rescue advisory")
        t |> equal(fread(dst), src, "original bytes restored")
        rmdir_rec(dir)
    }
    t |> run("no config: formats, comments stay") <| @(t : T?) {
        var terr : string
        let dir = make_policy_dir("mcp_keep_test", "", terr)
        if (empty(dir)) {
            t |> failure("temp dir: {terr}")
            return
        }
        let dst = path_join(dir, "victim.das")
        fwrite(dst, fread(fixture_path("_fixture_comments.das")))
        var text : string
        var is_error = false
        parse_result(do_format_file(dst), text, is_error)
        t |> success(!is_error, "default run should not error")
        t |> success(find(text, "rescue_advisory") < 0, "nothing stripped — no rescue advisory")
        let after = fread(dst)
        t |> success(find(after, "narration dies") >= 0, "comments intact on disk")
        rmdir_rec(dir)
    }
}

[test]
def test_format_file_policy_scoping(t : T?) {
    t |> run("format_enabled = false skips the file untouched") <| @(t : T?) {
        var terr : string
        let dir = make_policy_dir("mcp_skip_test", "[format]\nformat_enabled = false\n", terr)
        if (empty(dir)) {
            t |> failure("temp dir: {terr}")
            return
        }
        let dst = path_join(dir, "corpus.das")
        let src = "options gen2\ndef f() \{\n    let x=1\n\}\n"
        fwrite(dst, src)
        var text : string
        var is_error = false
        parse_result(do_format_file(dst), text, is_error)
        t |> success(find(text, "skipped") >= 0, "status is skipped -- text={text}")
        t |> equal(fread(dst), src, "bytes untouched")
        rmdir_rec(dir)
    }
    t |> run("nearest .lint_config wins per key") <| @(t : T?) {
        var terr : string
        let dir = make_policy_dir("mcp_cascade_test", "[format]\nforce_clean_comments = true\n", terr)
        if (empty(dir)) {
            t |> failure("temp dir: {terr}")
            return
        }
        let sub = path_join(dir, "teaching")
        mkdir(sub)
        fwrite(path_join(sub, ".lint_config"), "[format]\nforce_clean_comments = false\n")
        let dst = path_join(sub, "victim.das")
        fwrite(dst, fread(fixture_path("_fixture_comments.das")))
        var text : string
        var is_error = false
        parse_result(do_format_file(dst), text, is_error)
        t |> success(!is_error, "cascade run should not error")
        let after = fread(dst)
        t |> success(find(after, "narration dies") >= 0, "child override kept comments")
        rmdir_rec(dir)
    }
}


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


[test]
def test_goto_definition_function(t : T?) {
    t |> run("resolves function call to definition") <| @(t : T?) {
        var text : string
        var is_error = false
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
        parse_result(do_goto_definition(fixture_path("_fixture_goto.das"), "15", "16"), text, is_error)
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
        parse_result(do_goto_definition(fixture_path("_fixture_goto.das"), "34", "5"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "Kind: builtin") >= 0, "should be builtin kind")
        t |> success(find(text, "print") >= 0, "should resolve to print")
    }
}

[test]
def test_goto_definition_declarations(t : T?) {
    t |> run("a class name is the class, never the finalizer the compiler puts on its line") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_goto_definition(fixture_path("_fixture_refs.das"), "46", "8"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "Symbol: Square") >= 0 && find(text, "Kind: struct") >= 0, "the class: {text}")
        t |> success(find(text, "__finalize") < 0, "not the synthesized finalizer: {text}")
    }
    t |> run("the parent named on a derived class line is the parent") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_goto_definition(fixture_path("_fixture_refs.das"), "46", "16"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "Symbol: Shape") >= 0 && find(text, "Line: 42") >= 0, "the parent's declaration: {text}")
    }
    t |> run("a for loop variable's use navigates to its header") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_goto_definition(fixture_path("_fixture_refs.das"), "69", "17"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "Symbol: i") >= 0 && find(text, "Line: 68") >= 0, "the header: {text}")
    }
    t |> run("a type name in an expression, a parameter and a constructor is the type") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_goto_definition(fixture_path("_fixture_refs.das"), "66", "21"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "Symbol: Square") >= 0 && find(text, "Line: 46") >= 0 && find(text, "Column: 6") >= 0, "new Square() names the class: {text}")
        parse_result(do_goto_definition(fixture_path("_fixture_refs.das"), "14", "24"), text, is_error)
        t |> success(find(text, "Symbol: Point") >= 0 && find(text, "Line: 9") >= 0, "a parameter's type: {text}")
        parse_result(do_goto_definition(fixture_path("_fixture_refs.das"), "62", "14"), text, is_error)
        t |> success(find(text, "Symbol: Value") >= 0 && find(text, "Kind: typedef") >= 0 && find(text, "Line: 31") >= 0, "a variant constructor's type lands on the typedef: {text}")
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "66", "21", "file", ""), text, is_error)
        t |> success(find(text, "structure 'Square'") >= 0 && find(text, ":66:20") >= 0, "references from the type name in new: {text}")
        parse_result(do_type_of(fixture_path("_fixture_refs.das"), "66", "21"), text, is_error)
        t |> success(find(text, "TypeDecl(Square) : Square") >= 0, "type_of on a type name: {text}")
    }
    t |> run("a function's own name is its definition") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_goto_definition(fixture_path("_fixture_refs.das"), "14", "6"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "Symbol: add_points") >= 0 && find(text, "Kind: function") >= 0 && find(text, "Line: 14") >= 0, "the definition: {text}")
    }
    t |> run("a global's own name, a for variable's header and a class line report the cursor's column") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_goto_definition(fixture_path("_fixture_refs.das"), "36", "5"), text, is_error)
        t |> success(find(text, "Symbol: GLOBAL_SCALE") >= 0 && find(text, "Kind: variable") >= 0 && find(text, "Line: 36") >= 0, "the global: {text}")
        parse_result(do_goto_definition(fixture_path("_fixture_refs.das"), "68", "10"), text, is_error)
        t |> success(find(text, "Symbol: i") >= 0 && find(text, "Kind: variable") >= 0 && find(text, "Line: 68") >= 0, "the for variable on its header: {text}")
        parse_result(do_goto_definition(fixture_path("_fixture_refs.das"), "46", "8"), text, is_error)
        t |> success(find(text, "Column: 6") >= 0, "the class name's column on its own line: {text}")
        parse_result(do_type_of(fixture_path("_fixture_refs.das"), "68", "10"), text, is_error)
        t |> success(find(text, "Variable(i) : int") >= 0, "type_of on a for variable's header: {text}")
        t |> equal(source_line(fixture_path("_fixture_refs.das"), 68), "    for (i in range(2)) \{", "the subtools' line reader is 1-based")
        t |> equal(source_line(fixture_path("_fixture_refs.das"), 100000), "", "past the end of the file the line is empty")
    }
    t |> run("a CRLF file's lines come back without the carriage return") <| @(t : T?) {
        var terr : string
        let dir = create_temp_directory("mcp_source_line_crlf", terr)
        if (empty(dir)) {
            t |> failure("temp dir: {terr}")
            return
        }
        let file = path_join(dir, "crlf.das")
        fwrite(file, "let a = 1\r\nlet b = 2\r\nlet c = 3\r\n")
        t |> equal(source_line(file, 2), "let b = 2", "the line, not the line plus \\r")
        t |> equal(length(source_line(file, 3)), length("let c = 3"), "the last line too")
        t |> equal(source_line(file, 5), "", "past the end of the file the line is empty")
        rmdir_rec(dir)
    }
}

[test]
def test_goto_definition_members_and_types(t : T?) {
    t |> run("a method call lands on the method the receiver's class runs, a field's own name is the field") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_goto_definition(fixture_path("_fixture_refs.das"), "78", "15"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "Symbol: Square`area") >= 0 && find(text, "Kind: function") >= 0 && find(text, "Line: 47") >= 0, "s->area() is the override, not the class: {text}")
        parse_result(do_goto_definition(fixture_path("_fixture_refs.das"), "67", "20"), text, is_error)
        t |> success(find(text, "Symbol: Square`area") >= 0 && find(text, "Line: 47") >= 0, "shape->area() too: {text}")
        parse_result(do_goto_definition(fixture_path("_fixture_refs.das"), "10", "5"), text, is_error)
        t |> success(find(text, "Symbol: x") >= 0 && find(text, "Kind: field") >= 0 && find(text, "Line: 10") >= 0, "the field's declaration: {text}")
        parse_result(do_type_of(fixture_path("_fixture_refs.das"), "78", "15"), text, is_error)
        t |> success(text |> starts_with("ExprField(area)") && find(text, "ExprRef2Value") < 0, "type_of on a method call: the method field first, never the compiler's ref-to-value conversion: {text}")
    }
    t |> run("an enum type name in a parameter is the enum") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_goto_definition(fixture_path("_fixture_refs.das"), "73", "15"), text, is_error)
        t |> success(find(text, "Symbol: Color") >= 0 && find(text, "Kind: enum") >= 0 && find(text, "Line: 3") >= 0, "the enum: {text}")
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "73", "15", "file", "true"), text, is_error)
        t |> success(find(text, "enumeration 'Color'") >= 0 && find(text, ":73:") >= 0 && find(text, ":60:") >= 0, "references from the parameter's type: the parameter and the constant's use (unoptimized, the folded let keeps its site): {text}")
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "3", "6", "file", "true"), text, is_error)
        t |> success(find(text, "enumeration 'Color'") >= 0 && find(text, ":73:") >= 0 && find(text, ":60:") >= 0 && find(text, ":3:") < 0, "from the declaration's name: the same sites, never the declaration: {text}")
        parse_result(do_type_of(fixture_path("_fixture_refs.das"), "73", "15"), text, is_error)
        t |> success(find(text, "TypeDecl(Color) : Color") >= 0, "type_of on the enum type name: {text}")
    }
}

[test]
def test_goto_definition_no_expression(t : T?) {
    t |> run("reports no expression at empty position") <| @(t : T?) {
        var text : string
        var is_error = false
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
        parse_result(do_goto_definition(fixture_path("_fixture_goto.das"), "33", "13"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "def distance") >= 0, "snippet should contain function definition")
    }
}


[test]
def test_type_of_call(t : T?) {
    t |> run("function call returns result type") <| @(t : T?) {
        var text : string
        var is_error = false
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
        parse_result(do_type_of(fixture_path("_fixture_goto.das"), "15", "16"), text, is_error)
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


[test]
def test_find_references_function(t : T?) {
    t |> run("finds function call and addr references") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "57", "14", "file", ""), text, is_error)
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
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "57", "25", "file", ""), text, is_error)
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
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "15", "24", "file", ""), text, is_error)
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
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "59", "30", "file", ""), text, is_error)
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
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "14", "5", "file", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "func") >= 0, "should identify as function")
        t |> success(find(text, "add_points") >= 0, "should name add_points")
        t |> success(find(text, "[call]") >= 0, "should find call reference")
    }
    t |> run("finds references from struct definition") <| @(t : T?) {
        var text : string
        var is_error = false
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
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "26", "1", "file", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "alias") >= 0, "should identify as alias")
        t |> success(find(text, "Flags") >= 0, "should name Flags")
        t |> success(find(text, "[alias ref]") >= 0, "should find alias references")
        t |> success(find(text, ":26:") < 0, "the declaration's own line is its binding, not a reference: {text}")
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "26", "10", "file", ""), text, is_error)
        t |> success(find(text, ":38:") >= 0 && find(text, ":26:") < 0, "from the name on the declaration: the parameter's type, not the declaration: {text}")
    }
    t |> run("finds alias references from variant usage") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "62", "13", "file", ""), text, is_error)
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
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "36", "5", "file", "true"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "variable") >= 0, "should identify as variable")
        t |> success(find(text, "GLOBAL_SCALE") >= 0, "should name GLOBAL_SCALE")
        t |> success(find(text, "Found 1 reference") >= 0, "should find 1 reference (usage in main)")
    }
}

[test]
def test_find_references_class(t : T?) {
    t |> run("a class name resolves the class, not the finalizer the compiler puts on its line") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "46", "8", "file", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "structure 'Square'") >= 0, "should identify the class: {text}")
        t |> success(find(text, "__finalize") < 0, "never the synthesized finalizer: {text}")
        t |> success(find(text, "Found 2 reference") >= 0, "the new site and the parameter type, no implicit self: {text}")
        t |> success(find(text, ":66:20 in main [type ref]") >= 0 && find(text, ":77:") >= 0, "the new site and the parameter's type: {text}")
    }
    t |> run("a method's sites reference its function, through -> and the implicit self; an override is its own method") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "47", "18", "file", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "func 'Square`area'") >= 0, "the override's own function: {text}")
        t |> success(find(text, ":67:") >= 0 && find(text, ":78:") >= 0 && find(text, "[method call]") >= 0, "shape->area() and s->area() call the override: {text}")
        t |> success(find(text, ":47:") < 0, "never the declaration's own line: {text}")
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "43", "9", "file", ""), text, is_error)
        t |> success(find(text, "func 'Shape`area'") >= 0 && find(text, "Found 0 reference") >= 0, "the base method has no site of its own - every receiver is a Square: {text}")
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "78", "15", "file", ""), text, is_error)
        t |> success(find(text, "func 'Square`area'") >= 0 && find(text, ":67:") >= 0, "from a call site the same set: {text}")
    }
    t |> run("a field's declaration name references its accesses") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "10", "5", "file", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "field 'Point.x'") >= 0, "the field: {text}")
        t |> success(find(text, ":15:") >= 0 && find(text, ":19:") >= 0 && find(text, ":64:") >= 0, "a.x, p?.x and p3.x: {text}")
        t |> success(find(text, ":10:") < 0, "never the declaration's own line: {text}")
        t |> success(find(text, ":86:") < 0, "Other.x is a different field with the same name: {text}")
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "86", "14", "file", ""), text, is_error)
        t |> success(find(text, "field 'Other.x'") >= 0 && find(text, ":86:") >= 0 && find(text, ":15:") < 0, "from an access, the other field's own sites: {text}")
    }
    t |> run("a parent named on a derived class line references the parent") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "42", "8", "file", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "structure 'Shape'") >= 0, "should identify the base class: {text}")
        t |> success(find(text, "Found 1 reference") >= 0, "the derived class line only, no implicit self of its method: {text}")
        t |> success(find(text, ":46:15 [parent]") >= 0, "the parent mention on the derived class line: {text}")
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "46", "16", "file", ""), text, is_error)
        t |> success(find(text, "structure 'Shape'") >= 0, "the cursor on the parent name resolves the parent: {text}")
    }
    t |> run("a user-written finalize is itself, with no synthesized caller") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "48", "11", "file", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "func 'finalize'") >= 0, "should identify the user finalizer: {text}")
        t |> success(find(text, "Found 0 reference") >= 0, "the synthesized finalizer chain is not a caller: {text}")
    }
}

[test]
def test_find_references_for_variable(t : T?) {
    t |> run("a for loop variable in its header pairs with its body use") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "68", "10", "file", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "variable 'i'") >= 0, "should identify the loop variable: {text}")
        t |> success(find(text, ":69:16 in main [variable]") >= 0, "the body use: {text}")
        parse_result(do_find_references(fixture_path("_fixture_refs.das"), "69", "17", "file", ""), text, is_error)
        t |> success(find(text, "variable 'i'") >= 0 && find(text, ":69:16") >= 0, "from the use, the same variable: {text}")
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
        t |> success(find(text, ":2:1") >= 0, "the message echoes the caller's 1-based position: {text}")
    }
}


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
        parse_result(do_eval_expression("undefined_func_xyz()", "math"), text, is_error)
        t |> success(is_error, "should be error")
        t |> success(find(text, "daslib/math") < 0,
                "should NOT mention daslib/math — failure is not module-resolution")
        t |> success(find(text, "also retried") < 0, "should NOT announce a retry attempt")
    }
    t |> run("mixed list rewrites only the missing token") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_eval_expression("1 + 2", "math, ast"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "math, daslib/ast") >= 0, "should rewrite ast but keep math")
        t |> success(find(text, "daslib/math") < 0, "should NOT rewrite the builtin math")
        t |> success(find(text, "3") >= 0, "should evaluate to 3")
    }
}


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
    t |> run("slash-anchored glob filters rather than zeroing") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_grep_usage("compile_program", "", "", "utils/mcp/tools/*.das"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "common.das") >= 0, "root-anchored glob should match")
        t |> success(find(text, "Found 0 match") < 0, "glob must not silently zero the result")
    }
    t |> run("explicit '.' directory behaves like the default root") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_grep_usage("compile_program", ".", "", "utils/mcp/tools/*.das"), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "common.das") >= 0, "'.' root must not append a trailing /.")
        t |> success(find(text, "Found 0 match") < 0, "glob must not silently zero the result")
    }
}

[test]
def test_grep_usage_parse_aware(t : T?) {
    if (!require_ast_grep(t)) return
    t |> run("skips comments and strings") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_grep_usage("compile_program", "utils/mcp/tools", "", ""), text, is_error)
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "common.das") >= 0, "should find in common.das where it's defined")
    }
}

[test]
def test_grep_usage_glob_exclude(t : T?) {
    if (!require_ast_grep(t)) return
    t |> run("exclude glob filters out files") <| @(t : T?) {
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


[test]
def test_shared_module_not_cached(t : T?) {
    let shared_mod = "utils/mcp/tests/fixture_shared_mod.das"
    let original = "options gen2\nmodule fixture_shared_mod shared\n\nstruct Foo \{\n    x : int = 0;\n\}\n"
    let modified = "options gen2\nmodule fixture_shared_mod shared\n\nstruct Foo \{\n    x : int = 0;\n\}\n\nstruct Bar \{\n    y : int = 0;\n\}\n"
    t |> run("sees new struct after shared module changes") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_compile_check(fixture_path("_fixture_shared_user.das")), text, is_error)
        t |> success(!is_error, "user1 should compile (Foo exists)")
        t |> success(fwrite(shared_mod, modified), "should write modified shared module")
        parse_result(do_compile_check(fixture_path("_fixture_shared_user2.das")), text, is_error)
        t |> success(!is_error, "user2 should compile (Bar should be visible after module change)")
        fwrite(shared_mod, original)
    }
}


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
        var text : string
        var is_error = false
        parse_result(do_aot(fixture_path("_fixture_aot.das"), "describe"), text, is_error)
        let bracket_pos = find(text, "[describe_")
        t |> success(bracket_pos >= 0, "should contain bracketed aot_name")
        if (bracket_pos < 0) return
        let name_start = bracket_pos + 1
        let name_end = find(text, "]", name_start)
        t |> success(name_end > name_start, "should have closing bracket")
        if (name_end <= name_start) return
        let aot_name = slice(text, name_start, name_end)
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
def test_lint_stale_nolint(t : T?) {
    t |> run("stale nolint reported via LINT019") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_lint(fixture_path("_fixture_stale_nolint.das")), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "should not be error")
        t |> success(find(text, "LINT019") >= 0, "should report the stale directive")
        t |> success(find(text, "PERF006") >= 0, "should name the stale code")
    }
    t |> run("stale nolint reported in the multi-file path too") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_lint("{fixture_path("_fixture_stale_nolint.das")},{fixture_path("_fixture_valid.das")}"), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(find(text, "LINT019") >= 0, "batch path runs the stale scan too")
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
        let ok = parse_result(do_lint(fixture_path("_fixture_expect.das")), text, is_error)
        t |> success(ok, "SKIP result must be a valid envelope (was a raw string => invalid JSON-RPC)")
        t |> success(!is_error, "skip is not an error")
        t |> success(find(text, "SKIP") >= 0, "should report SKIP")
        t |> success(find(text, "intentional compile errors") >= 0, "should explain the skip reason")
    }
    t |> run("a `lint-skip-file` header skips instead of hard-failing on policy-conflicted files") <| @(t : T?) {
        var text : string
        var is_error = false
        let ok = parse_result(do_lint("{get_das_root()}/tests/language/static_if.das"), text, is_error)
        t |> success(ok, "skip result is a valid envelope")
        t |> success(!is_error, "skip is not an error")
        t |> success(find(text, "SKIP") >= 0, "should report SKIP: {text}")
        t |> success(find(text, "lint-skip-file") >= 0, "should carry the directive reason: {text}")
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
        let path = make_temp_corpus_file(t, "\{\"schema_version\":3,\"functions\":[]\}")
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
        let path = make_temp_corpus_file(t, "\{\"schema_version\":3,\"functions\":[]\}")
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
        var text : string
        var is_error = false
        let ok = parse_result(do_export_corpus(fixture_path("_fixture_valid.das"), "", out_path, "", ""), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "should NOT be error: {text}")
        t |> success(find(text, "\"exit_code\":0") >= 0, "envelope has exit_code 0: {text}")
        t |> success(find(text, "\"out\":") >= 0, "envelope echoes out path: {text}")
        t |> success(find(text, "\"stdout\":") >= 0, "envelope has subprocess stdout: {text}")
        let body = fread(out_path)
        t |> success(!empty(body), "corpus file is non-empty")
        t |> success(find(body, "\"schema_version\"") >= 0, "corpus has schema_version: {body}")
        remove(out_path)
    }
}


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
def test_export_corpus_workers_parse_failure_still_exports(t : T?) {
    t |> run("syntax error in input still exports the survivors, even with workers") <| @(t : T?) {
        let paths_body = "{fixture_path("_fixture_syntax_error.das")}\n"
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
        t |> success(!is_error, "a parse failure is not a failed run: {text}")
        t |> success(find(text, "failed to parse") >= 0, "the envelope's stdout carries the failure count: {text}")
        t |> success(stat(out_path).is_valid, "the corpus file is written")
        remove(paths_file)
        remove(out_path)
    }
}


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


[test]
def test_cpp_grep_usage_type_position(t : T?) {
    if (!require_ast_grep(t)) return
    t |> run("finds Outer used as a return type") <| @(t : T?) {
        var text : string
        var is_error = false
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
        var text : string
        var is_error = false
        parse_result(do_cpp_grep_usage("TextWriter", "src/misc/string_writer.cpp", "", ""), text, is_error)
        t |> success(!is_error, "should not be error -- {cpp_diag(text)}")
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


[test]
def test_cpp_find_symbol(t : T?) {
    if (!require_ast_grep(t)) return
    t |> run("finds class in src/ast") <| @(t : T?) {
        var text : string
        var is_error = false
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
        parse_result(do_cpp_find_symbol("zzznotarealsymbolzzz_xyzzy", "", "src/ast", ""), text, is_error)
        t |> success(!is_error, "should not be error on no-match -- {cpp_diag(text)}")
        t |> success(find(text, "No matches") >= 0, "should report no matches -- {cpp_diag(text)}")
    }
    t |> run("kind filter rejects wrong kinds") <| @(t : T?) {
        var text : string
        var is_error = false
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
        t |> success(find(text, "[union] union \{") < 0, "anonymous union must be dropped")
        t |> success(find(text, "[union]\n") < 0, "no bare [union] entries")
        t |> success(find(text, "[enum] enum \{") < 0, "anonymous enum must be dropped")
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
        t |> success(find(text, "fixns::Outer::describe") >= 0,
            "Outer::describe should be qualified with namespace + class")
        t |> success(find(text, "fixns::Outer::Inner::method") >= 0,
            "deeply-nested method should carry full qualifier chain")
    }
    t |> run("flat mode does not double-prefix already-qualified names") <| @(t : T?) {
        let text = cpp_outline_text("flat")
        t |> success(find(text, "fixns::Outer::describe::") < 0,
            "must not double-qualify out-of-class definition")
    }
}


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
        parse_result(do_find_symbol("=print", "function", "", "", "", true, "src/builtin"), text, is_error)
        t |> success(!is_error, "should not be error -- {cpp_diag(text)}")
        t |> success(find(text, "→ cpp:") >= 0, "redirect line should appear when with_cpp_source=true -- {cpp_diag(text)}")
    }
}

[test]
def test_goto_definition_with_cpp_source(t : T?) {
    t |> run("redirect off (default) — no C++: line") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_goto_definition(fixture_path("_fixture_goto.das"), "34", "5", "", "", "", false), text, is_error)
        t |> success(!is_error, "should not be error -- {cpp_diag(text)}")
        t |> success(find(text, "Kind: builtin") >= 0, "should resolve to a builtin -- {cpp_diag(text)}")
    }
    t |> run("redirect on — appends C++: line for builtin") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_goto_definition(fixture_path("_fixture_goto.das"), "34", "5", "", "", "", true, "src/builtin"), text, is_error)
        t |> success(!is_error, "should not be error -- {cpp_diag(text)}")
        t |> success(find(text, "Kind: builtin") >= 0, "should still resolve to a builtin -- {cpp_diag(text)}")
        t |> success(find(text, "C++:") >= 0, "C++ line should appear when with_cpp_source=true -- {cpp_diag(text)}")
    }
}


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


[test]
def test_cpp_collect_git_excludes(t : T?) {
    t |> run("daspkg cache .git is auto-excluded if present") <| @(t : T?) {
        let modules_dir = path_join(get_das_root(), "modules")
        let cache_git = path_join(modules_dir, ".daspkg_cache/index/.git")
        if (!stat(cache_git).is_valid) {
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
    t |> run("finds 'using' alias as typedef") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_find_symbol("module_pull_t", "typedef", "include", ""), text, is_error)
        t |> success(!is_error, "should not be error -- {cpp_diag(text)}")
        t |> success(find(text, "module_pull_t") >= 0, "should locate the using-alias -- {cpp_diag(text)}")
    }
}


def private has_compile_db(text : string) : bool {
    return find(text, "No compile_commands.json") < 0
}

[test]
def test_cpp_compile_check(t : T?) {
    t |> run("missing file is an error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_compile_check("src/zzz_nonexistent_xyz.cpp", "", false), text, is_error)
        t |> success(is_error, "missing file -> error -- text={text}")
        t |> success(find(text, "file not found") >= 0, "reports file not found -- text={text}")
    }
    t |> run("real TU syntax-checks (or graceful no-DB)") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_compile_check("src/misc/sysos.cpp", "", false), text, is_error)
        if (!has_compile_db(text)) {
            t->log("no compile_commands.json — skipping cpp_compile_check assertions")
            return
        }
        t |> success(find(text, "Compile check") >= 0 || find(text, "no compile DB entry") >= 0,
                     "ran a check or reported no entry -- text={text}")
    }
    t |> run("header is rejected with guidance") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_compile_check("src/misc/sysos.cpp", "", false), text, is_error)
        if (!has_compile_db(text)) return
        parse_result(do_cpp_compile_check("include/daScript/daScript.h", "", false), text, is_error)
        t |> success(find(text, "header") >= 0, "header gets a clear 'pass a .cpp' message -- text={text}")
    }
    t |> run("json mode returns a CppCompileResult shape") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_compile_check("src/misc/sysos.cpp", "", true), text, is_error)
        if (!has_compile_db(text) || find(text, "no compile DB entry") >= 0) return
        t |> success(find(text, "\"success\"") >= 0, "json has success field -- text={text}")
        t |> success(find(text, "\"file\"") >= 0, "json has file field -- text={text}")
    }
}

[test]
def test_cpp_build_info(t : T?) {
    t |> run("known TU shows compiler + syntax-only command (or graceful no-DB)") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_build_info("src/misc/sysos.cpp", ""), text, is_error)
        if (!has_compile_db(text)) {
            t->log("no compile_commands.json — skipping cpp_build_info assertions")
            return
        }
        if (find(text, "no compile DB entry") >= 0) return
        t |> success(find(text, "compiler:") >= 0, "shows compiler -- text={text}")
        t |> success(find(text, "syntax-only command") >= 0, "shows derived syntax-only command -- text={text}")
        t |> success(find(text, "/Zs") >= 0 || find(text, "-fsyntax-only") >= 0, "appends a syntax-only flag -- text={text}")
    }
    t |> run("file not in the DB reports no entry") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_build_info("src/misc/sysos.cpp", ""), text, is_error)
        if (!has_compile_db(text)) return
        parse_result(do_cpp_build_info("utils/mcp/protocol_core.das", ""), text, is_error)
        t |> success(is_error, "non-cpp file -> error -- text={text}")
        t |> success(find(text, "no compile DB entry") >= 0, "reports no entry -- text={text}")
    }
}

[test]
def test_cpp_format_file(t : T?) {
    t |> run("daScript tree has no .clang-format -> no_config") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_format_file("src/misc/sysos.cpp"), text, is_error)
        t |> success(!is_error, "no_config is not an error -- text={text}")
        t |> success(find(text, "no_config") >= 0, "reports no_config -- text={text}")
    }
    t |> run("missing file is an error") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_format_file("src/zzz_nonexistent_xyz.cpp"), text, is_error)
        t |> success(is_error, "missing file -> error -- text={text}")
        t |> success(find(text, "file not found") >= 0, "reports file not found -- text={text}")
    }
}

[test]
def test_cpp_status(t : T?) {
    t |> run("text report names each diagnosed dependency") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_status("", false), text, is_error)
        t |> success(!is_error, "status is not an error -- text={text}")
        t |> success(find(text, "cpp-mcp server status") >= 0, "has report header -- text={text}")
        t |> success(find(text, "ast-grep") >= 0, "reports ast-grep -- text={text}")
        t |> success(find(text, "compile DB") >= 0, "reports compile DB -- text={text}")
        t |> success(find(text, "clang-format") >= 0, "reports clang-format -- text={text}")
    }
    t |> run("json mode returns structured fields") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_status("", true), text, is_error)
        t |> success(!is_error, "json status is not an error -- text={text}")
        t |> success(find(text, "das_root") >= 0, "json has das_root -- text={text}")
        t |> success(find(text, "ast_grep_available") >= 0, "json has ast_grep_available -- text={text}")
    }
    t |> run("explicit build_dir is honored for the compile-DB probe") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_cpp_status("build-ninja", true), text, is_error)
        t |> success(!is_error, "build_dir status is not an error -- text={text}")
        t |> success(find(text, "compile_db") >= 0, "json reports compile_db field -- text={text}")
    }
}


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


let PROOT_CONSUMER = "_pretend_root/consumer.das"
let PROOT = "_pretend_root"
let PROOT_MISSING = "missing prerequisite 'pretend_mod"

def assert_prereq_pair(t : T?; what : string; without_it, with_it : string) {
    var t1 : string; var e1 = false
    parse_result(without_it, t1, e1)
    t |> success(e1, "without {what}: should be error -- text={t1}")
    t |> success(find(t1, PROOT_MISSING) >= 0,
                 "without {what}: should mention missing prerequisite -- text={t1}")
    var t2 : string; var e2 = false
    parse_result(with_it, t2, e2)
    t |> success(!e2, "with {what}: should NOT be error -- text={t2}")
}

[test]
def test_project_root_compile_check(t : T?) {
    t |> run("compile_check honors project_root") <| @(t : T?) {
        assert_prereq_pair(t, "project_root",
            do_compile_check(fixture_path(PROOT_CONSUMER)),
            do_compile_check(fixture_path(PROOT_CONSUMER), "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_lint(t : T?) {
    t |> run("lint honors project_root") <| @(t : T?) {
        assert_prereq_pair(t, "project_root",
            do_lint(fixture_path(PROOT_CONSUMER)),
            do_lint(fixture_path(PROOT_CONSUMER), "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_aot(t : T?) {
    t |> run("aot honors project_root") <| @(t : T?) {
        assert_prereq_pair(t, "project_root",
            do_aot(fixture_path(PROOT_CONSUMER), ""),
            do_aot(fixture_path(PROOT_CONSUMER), "", "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_list_functions(t : T?) {
    t |> run("list_functions honors project_root") <| @(t : T?) {
        assert_prereq_pair(t, "project_root",
            do_list_functions(fixture_path(PROOT_CONSUMER)),
            do_list_functions(fixture_path(PROOT_CONSUMER), "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_list_types(t : T?) {
    t |> run("list_types honors project_root") <| @(t : T?) {
        assert_prereq_pair(t, "project_root",
            do_list_types(fixture_path(PROOT_CONSUMER)),
            do_list_types(fixture_path(PROOT_CONSUMER), "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_list_requires(t : T?) {
    t |> run("list_requires honors project_root") <| @(t : T?) {
        assert_prereq_pair(t, "project_root",
            do_list_requires(fixture_path(PROOT_CONSUMER)),
            do_list_requires(fixture_path(PROOT_CONSUMER), "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_program_log(t : T?) {
    t |> run("program_log honors project_root") <| @(t : T?) {
        assert_prereq_pair(t, "project_root",
            do_program_log(fixture_path(PROOT_CONSUMER), "main"),
            do_program_log(fixture_path(PROOT_CONSUMER), "main", "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_ast_dump(t : T?) {
    t |> run("ast_dump honors project_root") <| @(t : T?) {
        assert_prereq_pair(t, "project_root",
            do_ast_dump("", fixture_path(PROOT_CONSUMER), "main", "source", ""),
            do_ast_dump("", fixture_path(PROOT_CONSUMER), "main", "source", "", "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_find_references(t : T?) {
    t |> run("find_references honors project_root") <| @(t : T?) {
        assert_prereq_pair(t, "project_root",
            do_find_references(fixture_path(PROOT_CONSUMER), "7", "17", "file", ""),
            do_find_references(fixture_path(PROOT_CONSUMER), "7", "17", "file", "", "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_goto_definition(t : T?) {
    t |> run("goto_definition honors project_root") <| @(t : T?) {
        assert_prereq_pair(t, "project_root",
            do_goto_definition(fixture_path(PROOT_CONSUMER), "7", "17"),
            do_goto_definition(fixture_path(PROOT_CONSUMER), "7", "17", "", "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_type_of(t : T?) {
    t |> run("type_of honors project_root") <| @(t : T?) {
        assert_prereq_pair(t, "project_root",
            do_type_of(fixture_path(PROOT_CONSUMER), "7", "17"),
            do_type_of(fixture_path(PROOT_CONSUMER), "7", "17", "", "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_find_symbol(t : T?) {
    t |> run("find_symbol honors project_root") <| @(t : T?) {
        assert_prereq_pair(t, "project_root",
            do_find_symbol("hello", "", fixture_path(PROOT_CONSUMER)),
            do_find_symbol("hello", "", fixture_path(PROOT_CONSUMER), "", fixture_path(PROOT)))
    }
}

[test]
def test_project_root_list_module_api(t : T?) {
    t |> run("list_module_api honors project_root") <| @(t : T?) {
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
        var t1 : string; var e1 = false
        parse_result(do_describe_type("FooBar", "pretend_mod/greet"), t1, e1)
        t |> success(e1, "without project_root: should be error -- text={t1}")
        t |> success(find(t1, "missing prerequisite 'pretend_mod/greet") >= 0,
                     "without project_root: should mention missing prereq -- text={t1}")
        var t2 : string; var e2 = false
        parse_result(do_describe_type("FooBar", "pretend_mod/greet", "", fixture_path(PROOT)), t2, e2)
        t |> success(!e2, "with project_root: should NOT be error -- text={t2}")
    }
}

[test]
def test_project_root_eval_expression(t : T?) {
    t |> run("eval_expression honors project_root") <| @(t : T?) {
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
        var t1 : string; var e1 = false
        parse_result(do_run_test(fixture_path(PROOT_CONSUMER)), t1, e1)
        t |> success(e1, "without project_root: should be error -- text={t1}")
        t |> success(find(t1, PROOT_MISSING) >= 0,
                     "without project_root: should mention missing prereq -- text={t1}")
        var t2 : string; var e2 = false
        parse_result(do_run_test(fixture_path(PROOT_CONSUMER), "", fixture_path(PROOT)), t2, e2)
        t |> success(find(t2, PROOT_MISSING) < 0,
                     "with project_root: should NOT mention missing prereq -- text={t2}")
    }
}


let PRETEND_LM = "_pretend_root/modules/pretend_mod"

def make_lm_array(p : string) : array<string> {
    var arr : array<string>
    arr |> push(p)
    return <- arr
}

[test]
def test_load_module_compile_check(t : T?) {
    t |> run("compile_check honors load_module") <| @(t : T?) {
        assert_prereq_pair(t, "load_module",
            do_compile_check(fixture_path(PROOT_CONSUMER)),
            do_compile_check(fixture_path(PROOT_CONSUMER), "", "", false, make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_lint(t : T?) {
    t |> run("lint honors load_module") <| @(t : T?) {
        assert_prereq_pair(t, "load_module",
            do_lint(fixture_path(PROOT_CONSUMER)),
            do_lint(fixture_path(PROOT_CONSUMER), "", "", make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_aot(t : T?) {
    t |> run("aot honors load_module") <| @(t : T?) {
        assert_prereq_pair(t, "load_module",
            do_aot(fixture_path(PROOT_CONSUMER), ""),
            do_aot(fixture_path(PROOT_CONSUMER), "", "", "", make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_list_functions(t : T?) {
    t |> run("list_functions honors load_module") <| @(t : T?) {
        assert_prereq_pair(t, "load_module",
            do_list_functions(fixture_path(PROOT_CONSUMER)),
            do_list_functions(fixture_path(PROOT_CONSUMER), "", "", make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_list_types(t : T?) {
    t |> run("list_types honors load_module") <| @(t : T?) {
        assert_prereq_pair(t, "load_module",
            do_list_types(fixture_path(PROOT_CONSUMER)),
            do_list_types(fixture_path(PROOT_CONSUMER), "", "", make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_list_requires(t : T?) {
    t |> run("list_requires honors load_module") <| @(t : T?) {
        assert_prereq_pair(t, "load_module",
            do_list_requires(fixture_path(PROOT_CONSUMER)),
            do_list_requires(fixture_path(PROOT_CONSUMER), "", "", false, make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_program_log(t : T?) {
    t |> run("program_log honors load_module") <| @(t : T?) {
        assert_prereq_pair(t, "load_module",
            do_program_log(fixture_path(PROOT_CONSUMER), "main"),
            do_program_log(fixture_path(PROOT_CONSUMER), "main", "", "", make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_ast_dump(t : T?) {
    t |> run("ast_dump honors load_module") <| @(t : T?) {
        assert_prereq_pair(t, "load_module",
            do_ast_dump("", fixture_path(PROOT_CONSUMER), "main", "source", ""),
            do_ast_dump("", fixture_path(PROOT_CONSUMER), "main", "source", "", "", "", make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_find_references(t : T?) {
    t |> run("find_references honors load_module") <| @(t : T?) {
        assert_prereq_pair(t, "load_module",
            do_find_references(fixture_path(PROOT_CONSUMER), "7", "17", "file", ""),
            do_find_references(fixture_path(PROOT_CONSUMER), "7", "17", "file", "", "", "", make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_goto_definition(t : T?) {
    t |> run("goto_definition honors load_module") <| @(t : T?) {
        assert_prereq_pair(t, "load_module",
            do_goto_definition(fixture_path(PROOT_CONSUMER), "7", "17"),
            do_goto_definition(fixture_path(PROOT_CONSUMER), "7", "17", "", "", "", false, "", make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_type_of(t : T?) {
    t |> run("type_of honors load_module") <| @(t : T?) {
        assert_prereq_pair(t, "load_module",
            do_type_of(fixture_path(PROOT_CONSUMER), "7", "17"),
            do_type_of(fixture_path(PROOT_CONSUMER), "7", "17", "", "", "", make_lm_array(fixture_path(PRETEND_LM))))
    }
}

[test]
def test_load_module_find_symbol(t : T?) {
    t |> run("find_symbol honors load_module") <| @(t : T?) {
        assert_prereq_pair(t, "load_module",
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
        let lm <- ["/p1", "/p2"]
        let s = build_live_script_args("/path/foo.das", "", "", false, lm)
        t |> equal(s, "-load_module \"/p1\" -load_module \"/p2\" -cwd \"/path/foo.das\"",
                   "two load_module pairs preserved in source order")
    }
}

[test]
def test_live_args_all_set(t : T?) {
    t |> run("project_root + load_modules + project + cwd: ordered project_root → load_module → project → cwd") <| @(t : T?) {
        let lm <- ["/m1", "/m2"]
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
        let src <- ["foo", "./bar/baz"]
        let out <- resolve_load_modules(src)
        t |> equal(length(out), 2, "preserves entry count")
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
        let lm <- ["/m1", "/m2"]
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

// Every citation these fixtures plant is assembled from ARCH_ANN rather than spelled out: the
// LINT026 forward pass reads source and skips only `//` comments, so a citation spelled out in
// one of the string literals below would be a citation made by THIS file, against a document
// utils/mcp does not carry.
let ARCH_ANN = "arch"

// A temp folder holding a document and the code citing it. Both sides have to live under one
// root because arch_sites walks the subtree the DOCUMENT owns. `code.das` cites twice: once
// into a section the document declares, once into an anchor it does not.
def private plant_arch_fixture(name : string; var err : string&) : string {
    let dir = create_temp_directory(name, err)
    return "" if (empty(dir))
    fwrite(path_join(dir, "ARCHITECTURE.md"), build_string() $(var w) {
        w |> write("# Doc\n\n## Tokenizer \{#tokenizer\}\n\nThe tokenizer section body.\n\n")
        w |> write("### Details \{#details\}\n\nA subsection stays inside its parent.\n\n")
        w |> write("## Buffers \{#buffers\}\n\nThe buffers section body.\n")
    })
    fwrite(path_join(dir, "code.das"), build_string() $(var w) {
        w |> write("options gen2\n\n")
        w |> write("[{ARCH_ANN}(at=\"ARCHITECTURE.md#tokenizer\")]\ndef scan_tokens \{\n\}\n\n")
        w |> write("[{ARCH_ANN}(at=\"ARCHITECTURE.md#gone\")]\ndef dangling \{\n\}\n")
    })
    return dir
}

[test]
def test_arch_of(t : T?) {
    t |> run("a resolving citation carries the cited section text") <| @(t : T?) {
        var terr : string
        let dir = plant_arch_fixture("mcp_arch_of_test", terr)
        if (empty(dir)) {
            t |> failure("temp dir: {terr}")
            return
        }
        var text : string
        var is_error = false
        let ok = parse_result(do_arch_of(path_join(dir, "code.das")), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "a readable file is not a tool failure -- text={text}")
        t |> success(find(text, "code.das:3  scan_tokens  ->  ") >= 0,
            "the citation is a locator: its own line, the annotated function, then where it lands -- text={text}")
        t |> success(find(text, "ARCHITECTURE.md:3-9  #tokenizer") >= 0,
            "the section is a line range from its heading through its last line - the deeper heading at 7 does not end it, the next same-level one at 11 does -- text={text}")
        t |> success(find(text, "The tokenizer section body.") < 0,
            "the text itself is the document's to show; the tool gives coordinates -- text={text}")
        t |> success(find(text, "#buffers") < 0, "a section nothing cites is not in the answer -- text={text}")
        rmdir_rec(dir)
    }
    t |> run("a dangling citation reports LINT026's reason") <| @(t : T?) {
        var terr : string
        let dir = plant_arch_fixture("mcp_arch_dangling_test", terr)
        if (empty(dir)) {
            t |> failure("temp dir: {terr}")
            return
        }
        var text : string
        var is_error = false
        parse_result(do_arch_of(path_join(dir, "code.das")), text, is_error)
        t |> success(find(text, "does not resolve - no such anchor") >= 0,
            "lint's own wording, so one tool's answer reads as the other's -- text={text}")
        t |> success(find(text, "code.das:7  dangling  ->  does not resolve") >= 0,
            "the dangling citation still names its line and symbol -- text={text}")
        rmdir_rec(dir)
    }
    t |> run("the symbol filter keeps one citation") <| @(t : T?) {
        var terr : string
        let dir = plant_arch_fixture("mcp_arch_symbol_test", terr)
        if (empty(dir)) {
            t |> failure("temp dir: {terr}")
            return
        }
        var text : string
        var is_error = false
        parse_result(do_arch_of(path_join(dir, "code.das"), "scan_tokens"), text, is_error)
        t |> success(find(text, "scan_tokens  ->  ") >= 0, "the named symbol's citation is kept -- text={text}")
        t |> success(find(text, "dangling") < 0, "the other symbol's citation is dropped -- text={text}")
        t |> success(find(text, "1 citation(s) shown.") >= 0, "and the tally says one -- text={text}")
        rmdir_rec(dir)
    }
    t |> run("a missing file is a tool failure") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_arch_of("nonexistent_arch_file_12345.das"), text, is_error)
        t |> success(is_error, "should be error -- text={text}")
        t |> success(find(text, "not found") >= 0, "should mention file not found -- text={text}")
    }
    t |> run("a file citing nothing says so") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_arch_of(fixture_path("_fixture_valid.das")), text, is_error)
        t |> success(!is_error, "no citations is not a failure -- text={text}")
        t |> success(find(text, "No [arch] citations") >= 0, "should report an empty scan -- text={text}")
    }
}

// The C++ arm of arch_of: a `// <doc>#<anchor>` pointer is read from a C++ source and located in
// the same shape a .das citation is, with no symbol - the tool does not parse C++ declarations.
[test]
def test_arch_of_cpp(t : T?) {
    t |> run("a C++ source is read for its pointers, and lands in the same shape") <| @(t : T?) {
        var terr : string
        let dir = plant_arch_fixture("mcp_arch_cpp_test", terr)
        if (empty(dir)) {
            t |> failure("temp dir: {terr}")
            return
        }
        fwrite(path_join(dir, "code.cpp"), "// ARCHITECTURE.md#tokenizer - the pointer form\nvoid f() \{\}\n")
        var text : string
        var is_error = false
        parse_result(do_arch_of(path_join(dir, "code.cpp")), text, is_error)
        t |> success(!is_error, "a C++ file is a readable source -- text={text}")
        t |> success(find(text, "code.cpp:1  ->  ") >= 0,
            "the pointer is located at its line, with no symbol -- text={text}")
        t |> success(find(text, "ARCHITECTURE.md:3-9  #tokenizer") >= 0, "and lands on the same section -- text={text}")
        rmdir_rec(dir)
    }
}

[test]
def test_arch_sites(t : T?) {
    t |> run("the census finds the citations planted in the document's subtree") <| @(t : T?) {
        var terr : string
        let dir = plant_arch_fixture("mcp_arch_sites_test", terr)
        if (empty(dir)) {
            t |> failure("temp dir: {terr}")
            return
        }
        var text : string
        var is_error = false
        let ok = parse_result(do_arch_sites(path_join(dir, "ARCHITECTURE.md")), text, is_error)
        t |> success(ok, "parse result JSON")
        t |> success(!is_error, "an existing document is not a tool failure -- text={text}")
        t |> success(find(text, "\{#tokenizer\} (lines 3-9) - 1 citation(s)") >= 0,
            "the cited anchor is grouped with its section's line range -- text={text}")
        t |> success(find(text, "code.das:3  scan_tokens") >= 0,
            "the site names file, line and symbol -- text={text}")
        t |> success(find(text, "\{#buffers\} (lines 11-13) - no citations") >= 0,
            "an uncited anchor is part of the census -- text={text}")
        t |> success(find(text, "searched: ") >= 0 && find(text, "searched: ") < find(text, "src"),
            "the header names the document's own folder first, then the C++ homes -- text={text}")
        t |> success(find(text, "\{#gone\}") >= 0,
            "a citation naming an undeclared anchor is surfaced, not dropped -- text={text}")
        t |> success(find(text, "2 citation(s) in 1 file(s).") >= 0, "the tally counts both -- text={text}")
        rmdir_rec(dir)
    }
    t |> run("an anchor query narrows the census to that section") <| @(t : T?) {
        var terr : string
        let dir = plant_arch_fixture("mcp_arch_anchor_test", terr)
        if (empty(dir)) {
            t |> failure("temp dir: {terr}")
            return
        }
        var text : string
        var is_error = false
        let doc = path_join(dir, "ARCHITECTURE.md")
        parse_result(do_arch_sites("{doc}#tokenizer"), text, is_error)
        t |> success(find(text, "code.das:3  scan_tokens") >= 0, "the section's own site is listed -- text={text}")
        t |> success(find(text, "1 citation(s) in 1 file(s).") >= 0, "and only it -- text={text}")
        t |> success(find(text, "\{#buffers\}") < 0, "other anchors stay out -- text={text}")
        rmdir_rec(dir)
    }
    t |> run("a missing document is a tool failure") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_arch_sites("nonexistent_arch_doc_12345.md"), text, is_error)
        t |> success(is_error, "should be error -- text={text}")
        t |> success(find(text, "Document not found") >= 0, "should mention the missing document -- text={text}")
    }
}

def private discover_lines(query, limit, module_filter : string; var is_error : bool&) : array<string> {
    //! The tool's text split into lines: line 0 is the header, the rows follow in rank order.
    var text : string
    parse_result(do_discover(query, limit, module_filter), text, is_error)
    return <- split(text, "\n")
}

def private top_symbol(lines : array<string>) : string {
    //! The `module::name` of the top row - the text before its ` - description` or `  [group]` tail.
    return "" if (length(lines) < 2)
    var cut = length(lines[1])
    for (tail in [" - ", "  ["]) {
        let at = find(lines[1], tail)
        if (at >= 0 && at < cut) {
            cut = at
        }
    }
    return trim(slice(lines[1], 0, cut))
}

[test]
def test_discover(t : T?) {
    t |> run("query word order ranks a name over its mirror, and the group tail names the manual's heading") <| @(t : T?) {
        var is_error = false
        let lines <- discover_lines("convert uri to file name", "5", "", is_error)
        t |> success(!is_error, "should not be error -- {lines}")
        t |> success(top_symbol(lines) == "uriparser::uri_to_file_name", "uri_to_file_name ranks first, its mirror file_name_to_uri does not -- {lines}")
        t |> success(find(lines[1], "[File name conversions]") >= 0, "the row carries its group heading -- {lines}")
        t |> success(find(lines[1], " - Converts a URI string") >= 0, "the row carries its description -- {lines}")
    }
    t |> run("a word found only in descriptions still ranks its symbol first") <| @(t : T?) {
        var is_error = false
        let lines <- discover_lines("percent-encodes reserved characters", "3", "", is_error)
        t |> success(top_symbol(lines) == "uriparser::escape_uri", "description words rank escape_uri first -- {lines}")
    }
    t |> run("synonyms bridge the query's word to the name") <| @(t : T?) {
        var is_error = false
        let lines <- discover_lines("url to path", "8", "", is_error)
        let text = join(lines, "\n")
        t |> success(find(text, "uri_to_file_name") >= 0 || find(text, "uri_to_unix_file_name") >= 0, "url reaches the uri family -- {lines}")
    }
    t |> run("fts5 syntax filters before ranking") <| @(t : T?) {
        var is_error = false
        let lines <- discover_lines("uri* NOT windows", "50", "uriparser", is_error)
        let text = join(lines, "\n")
        t |> success(!is_error, "should not be error -- {lines}")
        t |> success(find(text, "uri_to_file_name") >= 0, "prefix match survives -- {lines}")
        t |> success(find(text, "uri_to_windows_file_name") < 0, "NOT excludes the windows twin -- {lines}")
    }
    t |> run("an exact name ranks first, and the module filter keeps other modules out") <| @(t : T?) {
        var is_error = false
        let in_fio <- discover_lines("base_name", "3", "fio", is_error)
        t |> success(top_symbol(in_fio) == "fio::base_name", "exact name first in the named module -- {in_fio}")
        let elsewhere <- discover_lines("base_name", "50", "uriparser", is_error)
        t |> success(find(join(elsewhere, "\n"), "fio::") < 0, "another module's rows never appear -- {elsewhere}")
    }
    t |> run("limit defaults to 12 on an empty or unparsable value and is honored otherwise") <| @(t : T?) {
        var is_error = false
        t |> success(find(discover_lines("string", "", "", is_error)[0], "showing 12") >= 0, "empty limit shows 12")
        t |> success(find(discover_lines("string", "abc", "", is_error)[0], "showing 12") >= 0, "garbage limit shows 12")
        t |> success(find(discover_lines("string", "3", "", is_error)[0], "showing 3") >= 0, "a numeric limit is honored")
    }
    t |> run("a query no symbol matches says so without an error") <| @(t : T?) {
        var is_error = false
        let lines <- discover_lines("zzqqxxyy", "5", "", is_error)
        t |> success(!is_error, "no match is not a tool failure -- {lines}")
        t |> success(find(lines[0], "no symbol matches") >= 0, "names the outcome -- {lines}")
    }
    t |> run("an empty query and a bad fts5 query are tool failures") <| @(t : T?) {
        var text : string
        var is_error = false
        parse_result(do_discover("  ", "", ""), text, is_error)
        t |> success(is_error, "empty query should be error -- text={text}")
        parse_result(do_discover("(uri", "", ""), text, is_error)
        t |> success(is_error, "unbalanced fts5 query should be error -- text={text}")
        t |> success(find(text, "invalid FTS5 query") >= 0, "names the cause -- text={text}")
    }
}

[test]
def test_served_tree_paths(t : T?) {
    let here = getcwd()
    let dir = path_join(get_das_root(), "utils/mcp/tests/_served_tree_{ref_time_ticks()}")
    mkdir(dir)
    t |> success(chdir(dir), "enter the served tree")
    let served = to_generic_path(getcwd())
    t |> run("a relative path joins the served tree, not the daslang root") @(t : T?) {
        t |> equal(resolve_path("tests/x.das"), path_join(served, "tests/x.das"))
        t |> equal(resolve_path(""), served)
        t |> equal(resolve_path("."), served)
        let abs = path_join(to_generic_path(get_das_root()), "abs/x.das")   //! a rooted path on every platform (a bare `/abs` is drive-relative on Windows and resolves to `D:/abs`)
        t |> equal(resolve_path(abs), abs)
    }
    t |> run("a served tree with no project marker keeps the daslang C++ index roots") @(t : T?) {
        var inscope dirs <- cpp_default_search_dirs()
        t |> equal(length(dirs), length(CPP_SEARCH_DIRS))
        t |> equal(dirs[0], path_join(get_das_root(), CPP_SEARCH_DIRS[0]))
    }
    t |> run("the C++ index roots are the served tree once it carries a project marker") @(t : T?) {
        fwrite(path_join(served, "CMakeLists.txt"), "")
        var inscope dirs <- cpp_default_search_dirs()
        t |> equal(length(dirs), 1)
        t |> equal(dirs[0], served)
    }
    t |> run("the compile database is looked up under the served tree") @(t : T?) {
        mkdir(path_join(served, "build"))
        let db = path_join(served, "build/compile_commands.json")
        fwrite(db, "[]")
        t |> equal(cpp_locate_compile_db(""), db)
    }
    t |> success(chdir(here), "leave the served tree")
    rmdir_rec(dir)
    t |> run("at the daslang root the C++ index roots are CPP_SEARCH_DIRS") @(t : T?) {
        if (to_generic_path(getcwd()) != get_das_root()) {
            t |> skip("the suite runs outside the daslang root")
            return
        }
        var inscope dirs <- cpp_default_search_dirs()
        t |> equal(length(dirs), length(CPP_SEARCH_DIRS))
        t |> equal(dirs[0], path_join(get_das_root(), CPP_SEARCH_DIRS[0]))
    }
}
