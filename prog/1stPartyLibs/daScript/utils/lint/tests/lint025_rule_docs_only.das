options gen2
options indenting = 4
options no_unused_function_arguments = false

require dastest/testing_boost public

require strings
require daslib/fio

// LINT025 is a FOLDER rule the runner enforces with its own tree walk, not a compile pass,
// so it cannot ride the `expect 50503:N` fixture form the rule fixtures here use - the
// check is driven through the CLI over a planted tree.
//
// The tree carries three folders: `tagged` (rule_docs_only = true, one stray .md beside the
// five allowed names), `off` (rule_docs_only = false, one stray) and `plain` (no config, one
// stray). Exactly one finding may come back, naming `tagged/notes.md`.

var private _tmp_dir : string
var private _tmp_dir_inited = false


def private tmp_root() : string {
    if (!_tmp_dir_inited) {
        let r = create_temp_directory_result("lint025_rule_docs_only")
        // Fail-fast: an empty path would resolve against the working directory and plant
        // the fixture tree in the worktree.
        if (!(r is value)) {
            panic("could not create temp dir for the LINT025 test: {unsafe(r.error)}")
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


// One folder: its own .lint_config (skipped when `config` is empty), a lint-clean .das so
// the run has something to lint, and the .md files the folder carries.
def private plant_folder(root, name, config : string; docs : array<string>) : string {
    let dir = path_join(root, name)
    mkdir_rec(dir)
    if (!empty(config)) {
        plant(dir, ".lint_config", config)
    }
    plant(dir, "ok.das", "options gen2\n\n[export]\ndef main \{\n    print(\"ok\\n\")\n\}\n")
    for (doc in docs) {
        plant(dir, doc, "# {doc}\n")
    }
    return dir
}


def private build_tree() : string {
    let root = path_join(tmp_root(), "tree")
    mkdir_rec(root)
    plant_folder(root, "tagged", "[docs]\nrule_docs_only = true\n",
        ["notes.md", "REVIEW.md", "REVIEW_LINQ.md", "ARCHITECTURE.md", "ARCHITECTURE_DEDUP.md"])
    plant_folder(root, "off", "[docs]\nrule_docs_only = false\n", ["notes.md"])
    plant_folder(root, "plain", "", ["notes.md"])
    return root
}


// argv[0] is the running interpreter - dastest itself is spawned as
// `daslang(.exe) dastest/dastest.das ...`, so it names the binary to spawn.
def private das_exe() : string {
    let args <- get_command_line_arguments()
    return empty(args) ? "" : args[0]
}


def private run_lint(root : string; var output : string&) : int {
    let argv <- [das_exe(), "-dasroot", get_das_root(), "{get_das_root()}/utils/lint/main.das", "--", "-q", root]
    return unsafe(popen_argv(argv, 0.0, $(f) {
        if (f != null) {
            output = fread(f)
        }
    }))
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
def test_rule_docs_only_folder(t : T?) {
    t |> run("a stray .md is reported once, and only in the tagged folder") @(t : T?) {
        let root = build_tree()
        var out : string
        let rc = run_lint(root, out)
        t |> equal(count_of(out, "LINT025"), 1, "exactly one stray .md in the planted tree\n{out}")
        t |> success(find(out, "tagged/notes.md: LINT025: stray .md beside shipped code") >= 0,
            "the finding names the tagged folder's stray\n{out}")
        t |> success(find(out, "off/notes.md") < 0, "rule_docs_only = false reports nothing\n{out}")
        t |> success(find(out, "plain/notes.md") < 0, "an untagged folder reports nothing\n{out}")
        t |> equal(rc, 2, "a finding sets the issue exit code\n{out}")
    }
    t |> run("the rule documents themselves are never findings") @(t : T?) {
        let root = build_tree()
        var out : string
        run_lint(root, out)
        for (doc in ["REVIEW.md", "REVIEW_LINQ.md", "ARCHITECTURE.md", "ARCHITECTURE_DEDUP.md"]) {
            t |> success(find(out, "/{doc}: LINT025") < 0, "{doc} is a rule document\n{out}")
        }
    }
    t |> run("a file positional arms its folder - the PR lint shape") @(t : T?) {
        let root = build_tree()
        var out : string
        let rc = run_lint(path_join(path_join(root, "tagged"), "ok.das"), out)
        t |> equal(count_of(out, "LINT025"), 1, "linting one file still checks its folder\n{out}")
        t |> success(find(out, "tagged/notes.md: LINT025") >= 0, "the finding names the stray\n{out}")
        t |> equal(rc, 2, "a finding sets the issue exit code\n{out}")
    }
    t |> run("the tag is a folder property - a subfolder does not inherit it") @(t : T?) {
        let root = build_tree()
        let nested = path_join(path_join(root, "tagged"), "nested")
        mkdir_rec(nested)
        plant(nested, "stray.md", "# stray\n")
        var out : string
        run_lint(root, out)
        t |> success(find(out, "nested/stray.md") < 0, "no cascade into an untagged subfolder\n{out}")
    }
    t |> run("a tagged folder with no .das still gets its verdict") @(t : T?) {
        let root = build_tree()
        let dir = path_join(root, "mdonly")
        mkdir_rec(dir)
        plant(dir, ".lint_config", "[docs]\nrule_docs_only = true\n")
        plant(dir, "stray.md", "# stray\n")
        var out : string
        let rc = run_lint(dir, out)
        t |> success(find(out, "no .das files found") < 0, "an armed folder is not an empty input\n{out}")
        t |> success(find(out, "mdonly/stray.md: LINT025") >= 0, "the finding names the stray\n{out}")
        t |> equal(rc, 2, "a finding sets the issue exit code\n{out}")
    }
}


[test]
def test_rule_docs_only_modes(t : T?) {
    t |> run("overlapping scan roots report each stray once") @(t : T?) {
        let root = build_tree()
        var out : string
        let argv <- [das_exe(), "-dasroot", get_das_root(), "{get_das_root()}/utils/lint/main.das",
                     "--", "-q", root, path_join(root, "tagged")]
        unsafe(popen_argv(argv, 0.0, $(f) {
            if (f != null) {
                out = fread(f)
            }
        }))
        t |> equal(count_of(out, "tagged/notes.md"), 1, "the seen set dedups overlapping roots\n{out}")
    }
    t |> run("a tagged folder under a skipped directory is not scanned") @(t : T?) {
        let root = build_tree()
        let hidden = path_join(path_join(root, "_gen"), "inner")
        mkdir_rec(hidden)
        plant(hidden, ".lint_config", "[docs]\nrule_docs_only = true\n")
        plant(hidden, "stray.md", "# stray\n")
        var out : string
        run_lint(root, out)
        t |> success(find(out, "inner/stray.md") < 0, "the walk prunes _-prefixed dirs like the das scan\n{out}")
    }
    t |> run("a non-bool rule_docs_only value reads as false") @(t : T?) {
        let root = build_tree()
        let dir = path_join(root, "strval")
        mkdir_rec(dir)
        plant(dir, ".lint_config", "[docs]\nrule_docs_only = \"true\"\n")
        plant(dir, "stray.md", "# stray\n")
        var out : string
        run_lint(root, out)
        t |> success(find(out, "strval/stray.md") < 0, "only a bool true arms the folder\n{out}")
    }
    t |> run("--enable whitelist mode silences an unlisted LINT025") @(t : T?) {
        let root = build_tree()
        var out : string
        let argv <- [das_exe(), "-dasroot", get_das_root(), "{get_das_root()}/utils/lint/main.das",
                     "--", "-q", "--enable", "LINT001", root]
        unsafe(popen_argv(argv, 0.0, $(f) {
            if (f != null) {
                out = fread(f)
            }
        }))
        t |> equal(count_of(out, "LINT025"), 0, "whitelist mode without LINT025 reports nothing\n{out}")
    }
    t |> run("--disable LINT025 silences the pass") @(t : T?) {
        let root = build_tree()
        var out : string
        let argv <- [das_exe(), "-dasroot", get_das_root(), "{get_das_root()}/utils/lint/main.das",
                     "--", "-q", "--disable", "LINT025", root]
        let rc = unsafe(popen_argv(argv, 0.0, $(f) {
            if (f != null) {
                out = fread(f)
            }
        }))
        t |> equal(count_of(out, "LINT025"), 0, "the disabled rule reports nothing\n{out}")
        t |> equal(rc, 0, "and the run is clean\n{out}")
    }
}
