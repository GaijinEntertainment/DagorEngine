options gen2

require daslib/ast
require daslib/ast_boost
require daslib/fio
require daslib/strings_boost
require strings
require canonical
require cluster
require exchange
require patterns
require ../common/own_functions.das


def public read_path_lines(f : FILE const?; var paths : array<string>) {
    if (f == null) return
    while (!feof(f)) {
        let line = fgets(f) |> strip
        if (empty(line) || line |> starts_with("#")) continue
        paths |> push(line)
    }
}


def public read_path_lines_from_file(path : string; var paths : array<string>;
                                     var error : string&) : bool {
    var ok = false
    fopen(path, "rb") $(f) {
        if (f == null) return
        ok = true
        read_path_lines(f, paths)
    }
    if (!ok) {
        error = "could not open paths-from file: {path}"
    }
    return ok
}


// Files no scan should parse: builtin.das and the ast-fuzz selftest victims
// (deliberately broken; test_ast_fuzz.das owns them).
def public is_skip_file(path : string; basename : string) : bool {
    return basename == "builtin.das" || normalize_path(path) |> contains("ast-fuzz/selftest/")
}


def public is_skip_dir(name : string) : bool {
    return name |> starts_with(".")
}


def public has_expect_directive(file : string) : bool {
    let text = fread(file)
    if (empty(text)) return false
    let lines <- text |> split("\n")
    for (line in lines) {
        let trimmed = line |> strip
        if (trimmed |> starts_with("expect ")) return true
    }
    return false
}


def public scan_das_files(path : string; var files : array<string>; var cache : table<string; void?>) {
    let st = stat(path)
    if (!st.is_valid) return
    if (st.is_reg) {
        if (path |> ends_with(".das")) {
            let np = normalize_path(path)
            if (!cache |> key_exists(np)) {
                let fname = base_name(np)
                if (!is_skip_file(np, fname)) {
                    cache |> insert(np, null)
                    files |> push(np)
                }
            }
        }
        return
    }
    if (!st.is_dir) return
    fio::dir(path) $(name) {
        if (name == "." || name == ".." || name |> starts_with("_") || is_skip_dir(name)) return
        let full = path_join(path, name)
        let fst = stat(full)
        if (!fst.is_valid) return
        if (fst.is_dir) {
            scan_das_files(full, files, cache)
        } elif (fst.is_reg && name |> ends_with(".das") && !is_skip_file(full, name)) {
            let np = normalize_path(full)
            if (!cache |> key_exists(np)) {
                cache |> insert(np, null)
                files |> push(np)
            }
        }
    }
}


// `loc` is the record's identity for the per-run dedup - `file:line` for a function, `file:line:column`
// for a lambda, since one line can open several.
def private push_record(var records : array<FuncRecord>; var seen_loc : table<string; void?>;
                        loc : string; ref : MemberRef; canon : string;
                        want_sigs : bool; min_tokens : int) {
    if (empty(canon) || key_exists(seen_loc, loc)) return
    seen_loc |> insert(loc, null)
    records |> emplace(make_record(ref, canon, want_sigs, min_tokens))
}


def private collect_one_function(func : FunctionPtr;
                                 var records : array<FuncRecord>;
                                 var seen_loc : table<string; void?>;
                                 want_sigs : bool;
                                 min_tokens : int) {
    let fn_file = string(func.at.fileInfo.name)
    let line = int(func.at.line)
    let ref = MemberRef(file = fn_file, name = string(func.name), line = line,
        end_line = int((func.body != null) ? func.body.at.last_line : func.at.last_line), is_lambda = false)
    push_record(records, seen_loc, "{fn_file}:{line}", ref, canonicalize(func), want_sigs, min_tokens)
}


class LambdaSiteCollector : AstVisitor {
    sites : array<ExprMakeBlock?>

    def override visitExprMakeBlock(var expr : ExprMakeBlock?) : ExpressionPtr {
        sites |> push(expr)
        return expr
    }
}


// Every `@(...) { ... }` / `$(...) { ... }` in the function, nested ones included, as its
// own record named `<function>\`lambda@<column>` at the block's line - the column keeps two
// blocks on one line apart in every identity the record has.
def private collect_lambdas(func : FunctionPtr;
                            var records : array<FuncRecord>;
                            var seen_loc : table<string; void?>;
                            want_sigs : bool;
                            min_tokens : int) {
    let fn_file = string(func.at.fileInfo.name)
    var collector = new LambdaSiteCollector()
    make_visitor(*collector) $(adapter) {
        visit(func, adapter)
    }
    for (mk in collector.sites) {
        let line = int(mk.at.line)
        let column = int(mk.at.column)
        let ref = MemberRef(file = fn_file, name = "{func.name}`lambda@{column}", line = line,
            end_line = int((mk._block != null) ? mk._block.at.last_line : mk.at.last_line), is_lambda = true)
        let canon = canonicalize(mk)
        push_record(records, seen_loc, "{fn_file}:{line}:{column}", ref, canon, want_sigs, min_tokens)
    }
    unsafe { delete collector; }
}


def public collect_from_program(program : ProgramPtr; source_file : string; var records : array<FuncRecord>; var seen_loc : table<string; void?>; lambdas_only : bool; want_sigs : bool; min_tokens : int) {
    for_each_own_function(program, source_file) $(func) {
        if (lambdas_only) {
            collect_lambdas(func, records, seen_loc, want_sigs, min_tokens)
        } else {
            collect_one_function(func, records, seen_loc, want_sigs, min_tokens)
        }
    }
}


def public drop_records_under_files(var records : array<FuncRecord>;
                                    against_files : table<string; void?>) : int {
    var kept : array<FuncRecord>
    kept |> reserve(length(records))
    var dropped = 0
    for (r in records) {
        if (key_exists(against_files, normalize_path(r.ref.file))) {
            dropped++
            continue
        }
        kept |> push_clone(r)
    }
    delete records
    records <- kept
    return dropped
}


def public apply_pattern_filter(var records : array<FuncRecord>;
                                kept : table<string; void?>;
                                keep_all : bool;
                                verbose : bool) : table<string; int> {
    var skip_counts : table<string; int>
    if (keep_all) return <- skip_counts
    var kept_records : array<FuncRecord>
    kept_records |> reserve(length(records))
    for (r in records) {
        let hit = classify(r.ref.name, r.canonical)
        if (empty(hit.name) || key_exists(kept, hit.name)) {
            kept_records |> push_clone(r)
            continue
        }
        if (verbose) {
            to_log(LOG_INFO, "  pattern-skip [{hit.name}]: {r.ref.file}:{r.ref.line} {r.ref.name} ({hit.note})\n")
        }
        if (key_exists(skip_counts, hit.name)) {
            skip_counts[hit.name] = skip_counts[hit.name] + 1
        } else {
            skip_counts |> insert(hit.name, 1)
        }
    }
    delete records
    records <- kept_records
    return <- skip_counts
}


def public collect_candidate_refs(records : array<FuncRecord>) : array<MemberRef> {
    return <- [for (r in records); r.ref; where r.is_candidate]
}


// The grammar reports its abort as `invalid_expression` with a `syntax error, ...` text, the
// lexer's as `syntax_error`; every other code is a recorded diagnostic the parse walked past.
def private has_syntax_error(program : ProgramPtr) : bool {
    for (e in program.errors) {
        if (e.cerr == CompilationError.syntax_error || e.what |> starts_with("syntax error")) return true
    }
    return false
}


// One file, parsed alone: no prerequisite walk, no infer, no macro runs. A require,
// annotation, parent class or reader macro the parse cannot resolve is recorded and ignored -
// the shape survives either way. A syntax error ends the file's parse and returns false; the
// functions parsed before it are still collected.
def public parse_and_collect(file : string; var records : array<FuncRecord>;
                             var seen_loc : table<string; void?>;
                             verbose : bool; lambdas_only : bool;
                             want_sigs : bool; min_tokens : int;
                             quiet : bool = false;
                             project : string = "") : bool {
    var inscope access <- make_file_access(project)
    var ok_parse = true
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.version_2_syntax = true
            parse_file_no_prerequisites(file, access, unsafe(addr(mg)), cop) $(ok; program; issues) {
                if (!ok && has_syntax_error(program)) {
                    ok_parse = false
                    if (!quiet) {
                        if (verbose) {
                            to_log(LOG_ERROR, "FAIL {file}\n  {issues}\n")
                        } else {
                            to_log(LOG_ERROR, "FAIL {file}\n")
                        }
                    }
                }
                collect_from_program(program, file, records, seen_loc, lambdas_only, want_sigs, min_tokens)
            }
        }
    }
    return ok_parse
}
