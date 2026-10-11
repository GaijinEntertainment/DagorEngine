options gen2

require strings
require daslib/strings_boost
require daslib/fio
require dastest/review_gate

// The mechanical half of modules/dasLLVM/REVIEW.md (contract: REVIEW_COMMON.md at the repo
// root). Run from the repo root: bin/daslang modules/dasLLVM/REVIEW.das — exit 0 clean,
// 1 with findings.

let private TESTS_DIR = "modules/dasLLVM/tests"
let private DESCRIPTOR = "modules/dasLLVM/.das_module"

// Directories under TESTS_DIR whose contents git tracks. A suite run that creates,
// overwrites or deletes inside one leaves the tree dirty or destroys a checked-in fixture.
var private TRACKED_FIXTURE_DIRS <- {
    "llvm_tune_profiles_defaults"
}

// Filesystem calls that create, overwrite or delete; fopen joins them only on a write mode.
var private SINK_CALLS <- {
    "remove", "remove_result", "rename", "rename_result",
    "mkdir", "mkdir_rec", "mkdir_result", "mkdir_rec_result",
    "rmdir", "rmdir_rec", "rmdir_result", "rmdir_rec_result",
    "fwrite", "copy_file", "copy_file_result", "set_mtime", "set_mtime_result"
}

// The require category every register_native_path row in the descriptor carries, and the C++
// witness module's registration name (register_dynamic_module - a .shared_module, no .das file);
// each of the descriptor's other identifier-shaped quoted tokens names a module file.
var private DESCRIPTOR_CATEGORY_TOKENS <- { "llvm", "Module_LLVM" }

def private is_ident_byte(b : int) : bool {
    return is_alnum(b) || b == '_'
}

def private ident_prefix(s : string) : string {
    var n = 0
    peek_data(s) $(d) {
        while (n < length(d) && is_ident_byte(int(d[n]))) {
            n++
        }
    }
    return slice(s, 0, n)
}

// whole-word occurrence: `name` present with no identifier byte on either side
def private contains_word(text : string; name : string) : bool {
    var found = false
    peek_data(text) $(d) {
        let nl = length(name)
        var from = 0
        while (!found) {
            let at = find(d, name, from)
            break if (at < 0)
            let okl = at == 0 || !is_ident_byte(int(d[at - 1]))
            let okr = at + nl >= length(d) || !is_ident_byte(int(d[at + nl]))
            found = okl && okr
            from = at + 1
        }
    }
    return found
}

// `name(` with no identifier byte in front, so `remove(` does not match `fs_remove(`
def private calls(text : string; name : string) : bool {
    var found = false
    let needle = "{name}("
    peek_data(text) $(d) {
        var from = 0
        while (!found) {
            let at = find(d, needle, from)
            break if (at < 0)
            found = at == 0 || !is_ident_byte(int(d[at - 1]))
            from = at + 1
        }
    }
    return found
}

def private calls_any(text : string; names : table<string>) : bool {
    for (n in keys(names)) {
        if (calls(text, n)) {
            return true
        }
    }
    return false
}

def private opens_for_write(text : string) : bool {
    return false if (!calls(text, "fopen"))
    return find(text, "\"w") >= 0 || find(text, "\"a") >= 0 || find(text, "\"r+") >= 0
}

def private writes(text : string; sinks : table<string>) : bool {
    return calls_any(text, sinks) || opens_for_write(text)
}

def private def_name(line : string) : string {
    var rest = slice(line, length("def "))
    if (rest |> starts_with("private ")) {
        rest = slice(rest, length("private "))
    }
    return ident_prefix(rest)
}

// A helper in the same file that writes counts as a write at its call sites: the path is
// spelled at the call and the write happens one frame down.
def private local_sink_defs(lines : array<string>; var out : table<string>) {
    var fn = ""
    for (raw in lines) {
        let line = strip(raw)
        if (line |> starts_with("def ")) {
            fn = def_name(line)
        } elif (!empty(fn) && writes(line, SINK_CALLS)) {
            out |> insert(fn)
        }
    }
}

// name and initializer text of a `let`/`var` declaration; empty name when the line declares
// nothing or declares it without an initializer
def private decl_init(line : string) : tuple<name : string; init : string> {
    let none = (name = "", init = "")
    var rest = ""
    if (line |> starts_with("let ")) {
        rest = slice(line, length("let "))
    } elif (line |> starts_with("var ")) {
        rest = slice(line, length("var "))
    } else {
        return none
    }
    if (rest |> starts_with("private ")) {
        rest = slice(rest, length("private "))
    }
    if (rest |> starts_with("inscope ")) {
        rest = slice(rest, length("inscope "))
    }
    let name = ident_prefix(rest)
    return none if (empty(name))
    let eq = find(rest, "=")
    let mv = find(rest, "<-")
    if (eq >= 0 && (mv < 0 || eq < mv)) {
        return (name = name, init = slice(rest, eq + 1))
    }
    return mv >= 0 ? (name = name, init = slice(rest, mv + 2)) : none
}

// Every traced name a text mentions contributes its own initializer, so a path assembled
// across declarations is still read at the call that writes it.
def private expand(text : string; trace : table<string; string>) : string {
    return build_string() $(var b : StringBuilderWriter) {
        b |> write(text)
        for (name, init in keys(trace), values(trace)) {
            if (contains_word(text, name)) {
                b |> write(" ")
                b |> write(init)
            }
        }
    }
}

def private tracked_dir_in(text : string) : string {
    for (folder in keys(TRACKED_FIXTURE_DIRS)) {
        if (find(text, folder) >= 0) {
            return folder
        }
    }
    return ""
}

def private check_tracked_fixture_writes(path : string; lines : array<string>; sinks : table<string>) {
    var inscope trace : table<string; string>
    var line_no = 0
    for (raw in lines) {
        line_no++
        let line = strip(raw)
        continue if (empty(line))
        if (writes(line, sinks)) {
            let named = tracked_dir_in(expand(line, trace))
            if (!empty(named)) {
                gate_finding(path, line_no,
                    "writes or removes a path under {TESTS_DIR}/{named}, which git tracks - a test works on a temp copy it makes itself, so the tree is unchanged after the suite runs. This scan sees only the write line and the local declarations that line names, over the tracked directories listed here: a path a helper returns, a write a call level below the path, and a tracked file directly under {TESTS_DIR} pass it unreported")
            }
        }
        let decl = decl_init(line)
        if (!empty(decl.name)) {
            trace |> insert(decl.name, expand(decl.init, trace))
        }
    }
}

def private collect_das_files(folder : string; var out : array<string>) {
    dir(folder) $(name) {
        return if (name == "." || name == "..")
        let p = "{folder}/{name}"
        let st = stat(p)
        if (st.is_valid && st.is_dir) {
            collect_das_files(p, out)
        } elif (name |> ends_with(".das")) {
            out |> push(p)
        }
    }
}

def private check_tests_folder {
    var inscope files : array<string>
    collect_das_files(TESTS_DIR, files)
    for (p in files) {
        var inscope lines <- split(strip_line_comments(fread(p)), "\n")
        var inscope sinks <- clone_to_move(SINK_CALLS)
        local_sink_defs(lines, sinks)
        check_tracked_fixture_writes(p, lines, sinks)
    }
}

[export]
def main() : int {
    if (!fexist("modules/dasLLVM/REVIEW.das") || !fexist(DESCRIPTOR)) {
        to_log(LOG_ERROR, "modules/dasLLVM/REVIEW.das: run from the repo root\n")
        return 2
    }
    check_tests_folder()
    gate_descriptor_census(DESCRIPTOR, ["modules/dasLLVM/daslib", "modules/dasLLVM/bindings"],
        DESCRIPTOR_CATEGORY_TOKENS)
    return gate_verdict("dasLLVM")
}
