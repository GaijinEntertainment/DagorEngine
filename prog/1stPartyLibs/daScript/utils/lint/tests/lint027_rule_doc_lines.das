options gen2
options indenting = 4
options no_unused_function_arguments = false

require dastest/testing_boost public

require strings
require daslib/fio

// LINT027 is a FOLDER rule the runner enforces with its own tree walk, not a compile pass, so
// like LINT025 it is driven through the CLI over a planted tree rather than an `expect` header.
//
// The tree carries four folders: `tagged` (rule_docs_only = true), `arch` (no config, a routed
// checklist), `off` (rule_docs_only = false) and `plain` (no config). Four findings come back -
// one per oversized rule document, in whatever folder holds it: no tag arms the gate.

var private _tmp_dir : string
var private _tmp_dir_inited = false


def private tmp_root() : string {
    if (!_tmp_dir_inited) {
        let r = create_temp_directory_result("lint027_rule_doc_lines")
        // Fail-fast: an empty path would resolve against the working directory and plant
        // the fixture tree in the worktree.
        if (!(r is value)) {
            panic("could not create temp dir for the LINT027 test: {unsafe(r.error)}")
        }
        _tmp_dir = unsafe(r.value)
        _tmp_dir_inited = true
    }
    return _tmp_dir
}


[finalize]
def cleanup_temp_dir {
    if (_tmp_dir_inited && !empty(_tmp_dir)) {
        rmdir_rec(_tmp_dir)
    }
}


def private plant(dir, name, content : string) {
    fopen(path_join(dir, name), "wb") $(f) {
        if (f != null) {
            fwrite(f, content)
        }
    }
}


// A markdown body of exactly `n` lines, each newline-terminated.
def private md_of_lines(n : int) : string {
    return build_string() $(var w) {
        for (i in range(n)) {
            w |> write("line {i}\n")
        }
    }
}


// One folder: its own .lint_config (skipped when `config` is empty), a lint-clean .das so the
// run has something to lint, and the .md files the folder carries, each at its own length.
def private plant_folder(root, name, config : string; docs : array<string>; lengths : array<int>) : string {
    let dir = path_join(root, name)
    mkdir_rec(dir)
    if (!empty(config)) {
        plant(dir, ".lint_config", config)
    }
    plant(dir, "ok.das", "options gen2\n\n[export]\ndef main \{\n    print(\"ok\\n\")\n\}\n")
    for (doc, n in docs, lengths) {
        plant(dir, doc, md_of_lines(n))
    }
    return dir
}


def private build_tree() : string {
    let root = path_join(tmp_root(), "tree")
    mkdir_rec(root)
    // 301 fires, 300 is at the cap
    plant_folder(root, "tagged", "[docs]\nrule_docs_only = true\n",
        ["ARCHITECTURE.md", "REVIEW.md"], [301, 300])
    plant_folder(root, "arch", "", ["REVIEW_GPU.md"], [301])
    plant_folder(root, "off", "[docs]\nrule_docs_only = false\n", ["ARCHITECTURE.md"], [400])
    plant_folder(root, "plain", "", ["ARCHITECTURE.md"], [400])
    return root
}


// argv[0] is the running interpreter - dastest itself is spawned as
// `daslang(.exe) dastest/dastest.das ...`, so it names the binary to spawn.
def private das_exe() : string {
    let args <- get_command_line_arguments()
    return empty(args) ? "" : args[0]
}


def private run_lint_argv(extra : array<string>; root : string; var output : string&) : int {
    var argv <- [das_exe(), "-dasroot", get_das_root(), "{get_das_root()}/utils/lint/main.das", "--", "-q"]
    argv |> push_from(extra, [root])
    return unsafe(popen_argv(argv, 0.0, $(f) {
        if (f != null) {
            output = fread(f)
        }
    }))
}


def private run_lint(root : string; var output : string&) : int {
    let none : array<string>
    return run_lint_argv(none, root, output)
}


def private count_of(text, sub : string) : int {
    var n = 0
    var from = 0
    while (true) {
        let at = find(text, sub, from)
        break if (at < 0)
        n ++
        from = at + length(sub)
    }
    return n
}


[test]
def test_rule_doc_line_gate(t : T?) {
    t |> run("an oversized rule document fires, once per document, in any folder") @(t : T?) {
        let root = build_tree()
        var out : string
        let rc = run_lint(root, out)
        t |> equal(count_of(out, "LINT027"), 4, "one per oversized rule document, in any folder\n{out}")
        t |> success(find(out, "tagged/ARCHITECTURE.md: LINT027: rule document exceeds 300 lines (301)") >= 0,
            "the finding names the document and its size\n{out}")
        t |> success(find(out, "arch/REVIEW_GPU.md: LINT027") >= 0,
            "a folder holding a rule document is gated with no tag\n{out}")
        t |> equal(rc, 2, "a finding sets the issue exit code\n{out}")
    }
    t |> run("--enable LINT027 alone still arms the folder walk") @(t : T?) {
        let root = build_tree()
        var out : string
        let rc = run_lint_argv(["--enable", "LINT027"], root, out)
        t |> equal(count_of(out, "LINT027"), 4, "the line gate runs with LINT025/026 whitelisted off\n{out}")
        t |> equal(rc, 2, "a finding sets the issue exit code\n{out}")
    }
    t |> run("the cap itself is silent") @(t : T?) {
        let root = build_tree()
        var out : string
        run_lint(root, out)
        t |> success(find(out, "REVIEW.md: LINT027") < 0, "300 lines exactly is at the cap\n{out}")
    }
    t |> run("an untagged folder is gated like a tagged one") @(t : T?) {
        let root = build_tree()
        var out : string
        run_lint(root, out)
        t |> success(find(out, "off/ARCHITECTURE.md") >= 0, "rule_docs_only = false does not disarm the line gate\n{out}")
        t |> success(find(out, "plain/ARCHITECTURE.md") >= 0, "no .lint_config does not disarm it either\n{out}")
    }
    t |> run("a nested folder holding a rule document is gated on its own") @(t : T?) {
        let root = build_tree()
        let nested = path_join(path_join(root, "tagged"), "nested")
        mkdir_rec(nested)
        plant(nested, "ARCHITECTURE.md", md_of_lines(400))
        var out : string
        run_lint(root, out)
        t |> success(find(out, "nested/ARCHITECTURE.md") >= 0, "the subfolder's own rule document is gated\n{out}")
    }
    t |> run("--disable LINT027 silences the pass") @(t : T?) {
        let root = build_tree()
        var out : string
        let rc = run_lint_argv(["--disable", "LINT027"], root, out)
        t |> equal(count_of(out, "LINT027"), 0, "the disabled rule reports nothing\n{out}")
        t |> equal(rc, 0, "and the run is clean\n{out}")
    }
    t |> run("--enable whitelist mode silences an unlisted LINT027") @(t : T?) {
        let root = build_tree()
        var out : string
        run_lint_argv(["--enable", "LINT001"], root, out)
        t |> equal(count_of(out, "LINT027"), 0, "whitelist mode without LINT027 reports nothing\n{out}")
    }
}
