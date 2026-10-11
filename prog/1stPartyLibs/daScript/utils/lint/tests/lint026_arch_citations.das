options gen2
options indenting = 4
options no_unused_function_arguments = false

require dastest/testing_boost public

require strings
require daslib/fio

// LINT026 is a FOLDER rule the runner enforces with its own tree walk, not a compile pass, so it
// cannot ride the `expect 50503:N` fixture form the rule fixtures here use - the check is driven
// through the CLI over a planted tree.
//
// Every citation this file PLANTS is assembled from `ANN` rather than spelled out. The forward
// pass reads source and skips only `//` comments, so a citation spelled out inside one of the
// string literals below would be a citation made by this file, against a document
// utils/lint/tests does not carry.

let ANN = "arch"

var private _tmp_dir : string
var private _tmp_dir_inited = false


def private tmp_root() : string {
    if (!_tmp_dir_inited) {
        let r = create_temp_directory_result("lint026_arch_citations")
        // Fail-fast: an empty path would resolve against the working directory and plant the
        // fixture tree in the worktree.
        if (!(r is value)) {
            panic("could not create temp dir for the LINT026 test: {unsafe(r.error)}")
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


// One `[arch]` citation to plant, and the function that carries it.
struct private Cite {
    doc : string
    anchor : string
    fn : string
}


def private plant(dir, name, content : string) {
    fopen(path_join(dir, name), "wb") $(f) {
        if (f != null) {
            fwrite(f, content)
        }
    }
}


// A lint-clean .das carrying `cites`, one per function, every function called from main so none
// of them reads as unused. Citation N lands on line 4N - 1.
def private plant_code(dir, name : string; cites : array<Cite>) {
    plant(dir, name, build_string() $(var w) {
        w |> write("options gen2\n\n")
        for (c in cites) {
            w |> write("[{ANN}(at=\"{c.doc}#{c.anchor}\")]\ndef {c.fn} \{\n\}\n\n")
        }
        w |> write("[export]\ndef main \{\n")
        for (c in cites) {
            w |> write("    {c.fn}();\n")
        }
        w |> write("\}\n")
    })
}


// A .md declaring `anchors`, one heading each - repeat a name to declare it twice. Anchor N
// lands on line 3N.
def private plant_doc(dir, name : string; anchors : array<string>) {
    plant(dir, name, build_string() $(var w) {
        w |> write("# {name}\n\n")
        for (a in anchors) {
            w |> write("## Section {a} \{#{a}\}\ntext\n\n")
        }
    })
}


// argv[0] is the running interpreter - dastest itself is spawned as
// `daslang(.exe) dastest/dastest.das ...`, so it names the binary to spawn.
def private das_exe() : string {
    let args <- get_command_line_arguments()
    return empty(args) ? "" : args[0]
}


def private run_lint_argv(extra : array<string>; root : string; var output : string&) : int {
    var argv <- [das_exe(), "-dasroot", get_das_root(), "{get_das_root()}/utils/lint/main.das", "--", "-q"]
    argv |> push_from(extra)
    argv |> push(root)
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


// One document, and code citing it four ways: resolving, dangling anchor, dangling document,
// and an anchor the document declares twice.
def private build_forward(name : string) : string {
    let dir = path_join(tmp_root(), name)
    mkdir_rec(dir)
    plant_doc(dir, "ARCHITECTURE.md", ["alpha", "beta", "beta"])
    plant_code(dir, "ok.das", [Cite(doc = "ARCHITECTURE.md", anchor = "alpha", fn = "good"),
                               Cite(doc = "ARCHITECTURE.md", anchor = "nope", fn = "bad_anchor"),
                               Cite(doc = "MISSING.md", anchor = "alpha", fn = "bad_file"),
                               Cite(doc = "ARCHITECTURE.md", anchor = "beta", fn = "dup_anchor")])
    return dir
}


// A folder whose document declares one cited anchor and one orphan, beside a sibling folder
// whose NOTES.md carries a loose anchor - every anchor is owed a citation, no tag arms it.
def private build_reverse(name : string) : string {
    let root = path_join(tmp_root(), name)
    let armed = path_join(root, "armed")
    mkdir_rec(armed)
    plant_doc(armed, "ARCHITECTURE.md", ["cited", "orphan"])
    plant_code(armed, "ok.das", [Cite(doc = "ARCHITECTURE.md", anchor = "cited", fn = "one"),
                                 Cite(doc = "ARCHITECTURE.md", anchor = "cited", fn = "two")])
    let plain = path_join(root, "plain")
    mkdir_rec(plain)
    plant_doc(plain, "NOTES.md", ["loose"])
    plant(plain, "ok.das", "options gen2\n\n[export]\ndef main \{\n    print(\"ok\\n\")\n\}\n")
    return root
}


[test]
def test_arch_forward(t : T?) {
    t |> run("each unresolved citation fires once, and the resolving one is silent") @(t : T?) {
        let dir = build_forward("fwd")
        var out : string
        let rc = run_lint(dir, out)
        t |> equal(count_of(out, "LINT026"), 3, "one finding per unresolved citation\n{out}")
        t |> success(find(out, "ok.das:7: LINT026: [arch] citation does not resolve - no such anchor") >= 0,
            "the dangling anchor is named at its own line\n{out}")
        t |> success(find(out, "ok.das:11: LINT026: [arch] citation does not resolve - no such file") >= 0,
            "the dangling document is named at its own line\n{out}")
        t |> success(find(out, "ok.das:15: LINT026: [arch] citation does not resolve - anchor appears 2 times") >= 0,
            "a twice-declared anchor is not a resolution\n{out}")
        t |> success(find(out, "#alpha\")") < 0 || count_of(out, "ARCHITECTURE.md#alpha") == 0,
            "the citation that resolves says nothing\n{out}")
        t |> equal(rc, 2, "a finding sets the issue exit code\n{out}")
    }
    t |> run("a citation resolves against the CITING file's folder, not the run root") @(t : T?) {
        let root = path_join(tmp_root(), "nested")
        let sub = path_join(root, "sub")
        mkdir_rec(sub)
        plant_doc(root, "ARCHITECTURE.md", ["up"])
        plant_code(sub, "ok.das", [Cite(doc = "../ARCHITECTURE.md", anchor = "up", fn = "only")])
        var out : string
        let rc = run_lint(root, out)
        t |> equal(count_of(out, "LINT026"), 0, "the relative path walks up from sub/\n{out}")
        t |> equal(rc, 0, "and the run is clean\n{out}")
    }
}


[test]
def test_arch_forward_control(t : T?) {
    t |> run("negative control - the forward finding follows the citation") @(t : T?) {
        let dir = path_join(tmp_root(), "fwdctl")
        mkdir_rec(dir)
        plant_doc(dir, "ARCHITECTURE.md", ["alpha"])
        plant_code(dir, "ok.das", [Cite(doc = "ARCHITECTURE.md", anchor = "nope", fn = "only")])
        var out : string
        run_lint(dir, out)
        t |> equal(count_of(out, "does not resolve"), 1, "the dangling citation fires\n{out}")
        t |> equal(count_of(out, "has no citation"), 1, "and the anchor it missed is uncited\n{out}")
        // mutate: the same citation, pointed at a section the document declares
        plant_code(dir, "ok.das", [Cite(doc = "ARCHITECTURE.md", anchor = "alpha", fn = "only")])
        var fixed : string
        let rc = run_lint(dir, fixed)
        t |> equal(count_of(fixed, "LINT026"), 0, "a resolving citation is silent both ways\n{fixed}")
        t |> equal(rc, 0, "and the run is clean\n{fixed}")
        // restore
        plant_code(dir, "ok.das", [Cite(doc = "ARCHITECTURE.md", anchor = "nope", fn = "only")])
        var again : string
        run_lint(dir, again)
        t |> equal(count_of(again, "LINT026"), 2, "restoring the citation brings both findings back\n{again}")
    }
    t |> run("a commented-out citation is not a citation") @(t : T?) {
        let dir = path_join(tmp_root(), "commented")
        mkdir_rec(dir)
        plant_doc(dir, "ARCHITECTURE.md", ["alpha"])
        plant(dir, "ok.das", build_string() $(var w) {
            w |> write("options gen2\n\n")
            w |> write("// [{ANN}(at=\"GONE.md#vanished\")]\n")
            w |> write("[export]\ndef main \{\n    print(\"ok\\n\")\n\}\n")
        })
        var out : string
        let rc = run_lint(dir, out)
        t |> equal(count_of(out, "does not resolve"), 0, "a `//` line is prose, not a citation\n{out}")
        t |> equal(count_of(out, "has no citation"), 1, "so the document's anchor has no citer\n{out}")
        t |> equal(rc, 2, "the uncited anchor sets the issue exit code\n{out}")
    }
    test_arch_forward_control_tail(t)
}


def private test_arch_forward_control_tail(t : T?) {
    t |> run("a rule document's `<doc>.md#<anchor>` is a citation: it keeps the anchor alive, and a dangling one is a finding") @(t : T?) {
        let dir = path_join(tmp_root(), "mdcite")
        mkdir_rec(dir)
        plant_doc(dir, "ARCHITECTURE.md", ["alpha", "beta"])
        plant(dir, "ok.das", "options gen2\n\n[export]\ndef main \{\n    print(\"ok\\n\")\n\}\n")
        plant(dir, "REVIEW.md", "# rules\n\nA rule leaning on `ARCHITECTURE.md#alpha` and on `ARCHITECTURE.md#gamma`.\n")
        var out : string
        let rc = run_lint(dir, out)
        t |> equal(count_of(out, "REVIEW.md:3: LINT026: citation does not resolve - no such anchor"), 1, "the dangling markdown citation fires at its line\n{out}")
        t |> equal(count_of(out, "#alpha"), 0, "the cited anchor is alive\n{out}")
        t |> equal(count_of(out, "#beta"), 1, "the uncited one is the reverse finding\n{out}")
        t |> equal(count_of(out, "LINT026"), 2, "two findings, no more\n{out}")
        t |> equal(rc, 2, "a finding sets the issue exit code\n{out}")
        // mutate: the dangling citation fixed
        plant(dir, "REVIEW.md", "# rules\n\nA rule leaning on `ARCHITECTURE.md#alpha` and on `ARCHITECTURE.md#beta`.\n")
        var fixed : string
        let rc2 = run_lint(dir, fixed)
        t |> equal(count_of(fixed, "LINT026"), 0, "both anchors cited from prose, nothing dangling\n{fixed}")
        t |> equal(rc2, 0, "and the run is clean\n{fixed}")
    }
    t |> run("a markdown citation led by `../` or `./` resolves against the citing file's folder: it keeps the anchor alive, and a dangling one is a finding") @(t : T?) {
        let dir = path_join(tmp_root(), "mdrel")
        let sub = path_join(dir, "sub")
        mkdir_rec(sub)
        plant_doc(dir, "ARCHITECTURE.md", ["alpha", "beta"])
        plant(dir, "ok.das", "options gen2\n\n[export]\ndef main \{\n    print(\"ok\\n\")\n\}\n")
        plant(sub, "REVIEW.md", "# rules\n\nA rule leaning on `../ARCHITECTURE.md#alpha` and on `../ARCHITECTURE.md#gamma`.\n")
        plant(dir, "NOTES.md", "# notes\n\nA note leaning on `./ARCHITECTURE.md#beta`.\n")
        var out : string
        let rc = run_lint(dir, out)
        t |> equal(count_of(out, "REVIEW.md:3: LINT026: citation does not resolve - no such anchor"), 1, "the dangling `../` citation fires at its line\n{out}")
        t |> equal(count_of(out, "#alpha"), 0, "the `../` cited anchor is alive\n{out}")
        t |> equal(count_of(out, "#beta"), 0, "the `./` cited anchor is alive\n{out}")
        t |> equal(count_of(out, "LINT026"), 1, "one finding, no more\n{out}")
        t |> equal(rc, 2, "a finding sets the issue exit code\n{out}")
    }
    t |> run("a structure's citation is shape-checked by lint - the compiler never sees it") @(t : T?) {
        let dir = path_join(tmp_root(), "malformed")
        mkdir_rec(dir)
        plant_doc(dir, "ARCHITECTURE.md", ["alpha"])
        // a structure takes the marker unvalidated (one annotation per NAME, so `arch` is the
        // function annotation), which is why lint owns the shape on this side
        plant(dir, "ok.das", build_string() $(var w) {
            w |> write("options gen2\n\n")
            w |> write("[{ANN}(at=\"ARCHITECTURE.md\")]\nstruct S \{\n    a : int;\n\}\n\n")
            w |> write("[export]\ndef main \{\n    var s : S;\n    s.a = 1;\n    print(\"\{s.a\}\\n\");\n\}\n")
        })
        var out : string
        let rc = run_lint(dir, out)
        t |> equal(count_of(out, "does not resolve"), 1, "the anchor-less citation fires\n{out}")
        t |> success(find(out, "ok.das:3: LINT026: [arch] citation does not resolve - malformed citation") >= 0,
            "reported at the annotation, as malformed\n{out}")
        t |> equal(rc, 2, "a finding, not a compile error\n{out}")
    }
    t |> run("a citation stays in the document's folder tree - an ancestor's document resolves, a sibling's does not") @(t : T?) {
        let root = path_join(tmp_root(), "scope")
        let a = path_join(root, "a")
        let b = path_join(root, "b")
        mkdir_rec(a)
        mkdir_rec(b)
        plant_doc(root, "ARCHITECTURE.md", ["alpha"])
        plant_doc(a, "ARCHITECTURE.md", ["beta"])
        plant_code(b, "ok.das", [Cite(doc = "../ARCHITECTURE.md", anchor = "alpha", fn = "near"),
                                 Cite(doc = "../a/ARCHITECTURE.md", anchor = "beta", fn = "far")])
        var out : string
        let rc = run_lint(root, out)
        t |> equal(count_of(out, "outside this file's folder tree"), 1, "the sibling-folder citation is the finding\n{out}")
        t |> equal(count_of(out, "LINT026"), 1, "the ancestor citation resolves, and the far citation still counts as beta's citer\n{out}")
        t |> equal(rc, 2, "a finding sets the issue exit code\n{out}")
    }
}


[test]
def test_arch_roots(t : T?) {
    t |> run("a CMake build tree under a root is not source - its documents and files are skipped") @(t : T?) {
        let root = path_join(tmp_root(), "buildtree")
        let gen = path_join(root, "gen")
        mkdir_rec(gen)
        plant(gen, "CMakeCache.txt", "CMAKE_HOME_DIRECTORY:INTERNAL=x\n")
        plant_doc(gen, "vendored.md", ["build_include"])
        plant_code(gen, "ok.das", [Cite(doc = "missing.md", anchor = "nope", fn = "only")])
        plant_code(root, "ok.das", [])
        var out : string
        let rc = run_lint(root, out)
        t |> equal(count_of(out, "LINT026"), 0, "nothing under the build tree is reported\n{out}")
        t |> equal(rc, 0, "and the run is clean\n{out}")
    }
    t |> run("another repository's checkout under a root is not source either - a folder with a .git is skipped") @(t : T?) {
        let root = path_join(tmp_root(), "nestedrepo")
        let vendored = path_join(root, "vendored")
        mkdir_rec(path_join(vendored, ".git"))
        plant_doc(vendored, "theirs.md", ["their_anchor"])
        plant_code(vendored, "theirs.das", [Cite(doc = "BestPractices.md", anchor = "nope", fn = "only")])
        plant_code(root, "ok.das", [])
        var out : string
        let rc = run_lint(root, out)
        t |> equal(count_of(out, "LINT026"), 0, "nothing under the checkout is reported\n{out}")
        t |> equal(rc, 0, "and the run is clean\n{out}")
    }
    t |> run("a .md positional arms its folder with no .das compiled - the changed-document PR shape") @(t : T?) {
        let root = path_join(tmp_root(), "mdpos")
        mkdir_rec(root)
        plant_doc(root, "ARCHITECTURE.md", ["alpha", "orphan"])
        plant_code(root, "ok.das", [Cite(doc = "ARCHITECTURE.md", anchor = "alpha", fn = "only")])
        var out : string
        let rc = run_lint(path_join(root, "ARCHITECTURE.md"), out)
        t |> success(find(out, "0 files") >= 0, "no .das is compiled for a document positional\n{out}")
        t |> equal(count_of(out, "has no citation"), 1, "the folder's orphan anchor is found through the document\n{out}")
        t |> equal(rc, 2, "a finding sets the issue exit code\n{out}")
    }
    t |> run("nested roots fold into one walk - a subfolder's citer satisfies the parent's anchor, and nothing reports twice") @(t : T?) {
        let root = path_join(tmp_root(), "nested")
        let sub = path_join(root, "sub")
        mkdir_rec(sub)
        plant_doc(root, "ARCHITECTURE.md", ["alpha", "orphan"])
        plant_code(sub, "ok.das", [Cite(doc = "../ARCHITECTURE.md", anchor = "alpha", fn = "only")])
        plant_doc(sub, "NOTES.md", ["loose"])
        var out : string
        let rc = run_lint_argv([sub], root, out)
        t |> equal(count_of(out, "\{#orphan\}"), 1, "the parent's orphan is found once\n{out}")
        t |> equal(count_of(out, "\{#loose\}"), 1, "the subfolder given twice reports its loose anchor once\n{out}")
        t |> equal(count_of(out, "\{#alpha\}"), 0, "the subfolder's citer satisfies the parent's anchor\n{out}")
        t |> equal(rc, 2, "the two findings set the issue exit code\n{out}")
    }
}


[test]
def test_arch_reverse(t : T?) {
    t |> run("an uncited anchor fires in every folder - no tag arms the reverse pass") @(t : T?) {
        let root = build_reverse("rev")
        var out : string
        let rc = run_lint(root, out)
        t |> equal(count_of(out, "LINT026"), 2, "the orphan and the sibling's loose anchor are the findings\n{out}")
        t |> success(find(out, "armed/ARCHITECTURE.md:6: LINT026: anchor \{#orphan\} has no citation") >= 0,
            "the finding names the anchor at its heading line\n{out}")
        t |> success(find(out, "NOTES.md") >= 0, "a sibling folder's loose anchor is owed too\n{out}")
        t |> success(find(out, "\{#cited\}") < 0, "two citations satisfy one anchor\n{out}")
        t |> equal(rc, 2, "a finding sets the issue exit code\n{out}")
    }
    t |> run("negative control - dropping the citation orphans the anchor it covered") @(t : T?) {
        let root = build_reverse("revctl")
        let armed = path_join(root, "armed")
        var before : string
        run_lint(root, before)
        t |> equal(count_of(before, "LINT026"), 2, "the orphan and the loose anchor\n{before}")
        // mutate: the code stops citing `cited`, so both anchors are owed
        plant_code(armed, "ok.das", [Cite(doc = "ARCHITECTURE.md", anchor = "orphan", fn = "one")])
        var swapped : string
        run_lint(root, swapped)
        t |> equal(count_of(swapped, "\{#cited\}"), 1, "the anchor it stopped citing is now owed\n{swapped}")
        t |> equal(count_of(swapped, "\{#orphan\}"), 0, "and the one it took up is covered\n{swapped}")
        // restore
        plant_code(armed, "ok.das", [Cite(doc = "ARCHITECTURE.md", anchor = "cited", fn = "one"),
                                     Cite(doc = "ARCHITECTURE.md", anchor = "cited", fn = "two")])
        var again : string
        run_lint(root, again)
        t |> equal(count_of(again, "\{#orphan\}"), 1, "restoring the citations restores the finding\n{again}")
    }
}


// A C++ source carrying `pointers`, one per line inside a `//` comment, plus an `#include`, a
// block comment and two web URLs that must read as nothing. Pointer N lands on line 3N + 3.
def private plant_cpp(dir, name : string; pointers : array<string>) {
    plant(dir, name, build_string() $(var w) {
        w |> write("#include \"ARCHITECTURE.md#not-a-pointer.h\"\n")
        w |> write("/* ARCHITECTURE.md#alpha inside a block comment is not read */\n")
        w |> write("// see https://github.com/example/repo/blob/main/docs/Usage.md#the-web-not-the-tree\n")
        w |> write("// and github.com/example/repo/blob/main/docs/FAQ.md#a-host-first-segment\n")
        for (p in pointers) {
            w |> write("\n// {p} - the mechanism this function implements\nvoid f() \{\}\n")
        }
    })
}


[test]
def test_arch_cpp_pointers(t : T?) {
    t |> run("a C++ `// <doc>#<anchor>` pointer is a citation: it satisfies the anchor, and a dangling one fires at its line") @(t : T?) {
        let dir = path_join(tmp_root(), "cpp")
        mkdir_rec(dir)
        plant_doc(dir, "ARCHITECTURE.md", ["alpha", "orphan"])
        plant_cpp(dir, "ok.cpp", ["ARCHITECTURE.md#alpha", "ARCHITECTURE.md#nope"])
        var out : string
        let rc = run_lint(dir, out)
        t |> equal(count_of(out, "\{#alpha\}"), 0, "the pointer counts as alpha's citer\n{out}")
        t |> equal(count_of(out, "\{#orphan\}"), 1, "the anchor nothing points at is still owed\n{out}")
        t |> success(find(out, "ok.cpp:9: LINT026: [arch] citation does not resolve - no such anchor") >= 0,
            "the dangling pointer is named at its own line\n{out}")
        t |> equal(count_of(out, "not-a-pointer"), 0, "an #include is not a comment, so not a pointer\n{out}")
        t |> equal(count_of(out, "the-web-not-the-tree"), 0, "a URL with a scheme is a document on the web, not a pointer\n{out}")
        t |> equal(count_of(out, "a-host-first-segment"), 0, "a bare host.tld/... path is a document on the web, not a pointer\n{out}")
        t |> equal(count_of(out, "LINT026"), 2, "the block-comment mention reads as nothing\n{out}")
        t |> equal(rc, 2, "a finding sets the issue exit code\n{out}")
    }
    t |> run("a pointer's path is root-relative, walked up from the citing file, and the folder-tree rule does not bind C++") @(t : T?) {
        let root = path_join(tmp_root(), "cpproot")
        let inc = path_join(path_join(root, "include"), "deep")
        let src = path_join(root, "src")
        mkdir_rec(inc)
        mkdir_rec(src)
        plant_doc(src, "ARCHITECTURE.md", ["far"])
        plant_cpp(inc, "deep.h", ["src/ARCHITECTURE.md#far"])
        var out : string
        let rc = run_lint(root, out)
        t |> equal(count_of(out, "LINT026"), 0, "include/deep/deep.h reaches src/ARCHITECTURE.md through the root above both\n{out}")
        t |> equal(rc, 0, "and the run is clean\n{out}")
    }
    t |> run("negative control - the pointer follows its anchor") @(t : T?) {
        let dir = path_join(tmp_root(), "cppctl")
        mkdir_rec(dir)
        plant_doc(dir, "ARCHITECTURE.md", ["alpha"])
        plant_cpp(dir, "ok.cpp", ["ARCHITECTURE.md#alpha"])
        var before : string
        let rc = run_lint(dir, before)
        t |> equal(count_of(before, "LINT026"), 0, "a resolving pointer is silent both ways\n{before}")
        t |> equal(rc, 0, "and the run is clean\n{before}")
        // mutate: the pointer names a section the document does not declare
        plant_cpp(dir, "ok.cpp", ["ARCHITECTURE.md#gone"])
        var swapped : string
        run_lint(dir, swapped)
        t |> equal(count_of(swapped, "does not resolve"), 1, "the dangling pointer fires\n{swapped}")
        t |> equal(count_of(swapped, "\{#alpha\}"), 1, "and the anchor it left is owed\n{swapped}")
        // restore
        plant_cpp(dir, "ok.cpp", ["ARCHITECTURE.md#alpha"])
        var again : string
        run_lint(dir, again)
        t |> equal(count_of(again, "LINT026"), 0, "restoring the pointer restores silence\n{again}")
    }
}


[test]
def test_arch_modes(t : T?) {
    t |> run("--disable LINT026 silences the forward direction") @(t : T?) {
        let dir = build_forward("fwddis")
        var out : string
        let rc = run_lint_argv(["--disable", "LINT026"], dir, out)
        t |> equal(count_of(out, "LINT026"), 0, "the disabled rule reports nothing\n{out}")
        t |> equal(rc, 0, "and the run is clean\n{out}")
    }
    t |> run("--disable LINT026 silences the reverse direction") @(t : T?) {
        let root = build_reverse("revdis")
        var out : string
        let rc = run_lint_argv(["--disable", "LINT026"], root, out)
        t |> equal(count_of(out, "LINT026"), 0, "the reverse pass reports nothing\n{out}")
        t |> equal(rc, 0, "and the run is clean\n{out}")
    }
    t |> run("--enable whitelist mode without LINT026 silences it") @(t : T?) {
        let dir = build_forward("fwdwl")
        var out : string
        run_lint_argv(["--enable", "LINT001"], dir, out)
        t |> equal(count_of(out, "LINT026"), 0, "whitelist mode without LINT026 reports nothing\n{out}")
    }
    t |> run("a file positional arms its folder - the PR lint shape") @(t : T?) {
        let dir = build_forward("fwdfile")
        var out : string
        let rc = run_lint(path_join(dir, "ok.das"), out)
        t |> equal(count_of(out, "LINT026"), 3, "linting one file still checks its folder\n{out}")
        t |> equal(rc, 2, "a finding sets the issue exit code\n{out}")
    }
}
