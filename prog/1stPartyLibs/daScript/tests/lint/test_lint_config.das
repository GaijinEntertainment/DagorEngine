options gen2

require dastest/testing_boost
require daslib/lint_config
require daslib/fio
require strings

// Per-process temp directory. Created on first call, removed by [finalize].
// Avoids two problems:
//   1. Earlier version wrote `_lint_config_fixture.toml` into the worktree's
//      `tests/lint/` and never deleted it — left as untracked-modified state
//      across runs.
//   2. Parallel `dastest` invocations would race on that shared path. With
//      a process-private temp dir each invocation is hermetic.
var private _tmp_dir : string
var private _tmp_dir_inited = false

def fixture_path() : string {
    if (!_tmp_dir_inited) {
        let r = create_temp_directory_result("lint_config_test")
        // Fail-fast: silently writing to "" would resolve to the current
        // working directory and defeat the hermetic / parallel-safe goal.
        if (!(r is value)) {
            panic("could not create temp dir for lint_config test: {unsafe(r.error)}")
        }
        _tmp_dir = unsafe(r.value)
        _tmp_dir_inited = true
    }
    return path_join(_tmp_dir, "fixture.toml")
}

def write_fixture(content : string) {
    fopen(fixture_path(), "wb") $(f) {
        if (f != null) {
            fwrite(f, content)
        }
    }
}

[finalize]
def cleanup_temp_dir {
    if (_tmp_dir_inited && !empty(_tmp_dir)) {
        rmdir_rec(_tmp_dir)
    }
}

[test]
def test_missing_file_no_op(t : T?) {
    t |> run("missing file leaves disabled_codes untouched") @@(t : T?) {
        var d : table<string>
        load_lint_config_from_path("/no/such/lint_config_path_xyz", d)
        t |> equal(length(d), 0)
    }
}

[test]
def test_rule_false_inserts(t : T?) {
    t |> run("'STYLE100 = false' inserts the code (off)") @@(t : T?) {
        write_fixture("[rules]\nSTYLE100 = false\n")
        var d : table<string>
        load_lint_config_from_path(fixture_path(), d)
        t |> equal(length(d), 1)
        t |> success(key_exists(d, "STYLE100"))
    }
}

[test]
def test_rule_true_erases_default(t : T?) {
    t |> run("'STYLE005 = true' erases a preloaded default (on)") @@(t : T?) {
        write_fixture("[rules]\nSTYLE005 = true\n")
        var d : table<string>
        d |> insert("STYLE005")
        load_lint_config_from_path(fixture_path(), d)
        t |> equal(length(d), 0)
        t |> success(!key_exists(d, "STYLE005"))
    }
}

[test]
def test_comments_and_blanks_skipped(t : T?) {
    t |> run("# comments and blank lines inside [rules] are tolerated") @@(t : T?) {
        write_fixture("# header line\n\n[rules]\n# inline comment\nSTYLE100 = false\n")
        var d : table<string>
        load_lint_config_from_path(fixture_path(), d)
        t |> equal(length(d), 1)
        t |> success(key_exists(d, "STYLE100"))
    }
}

[test]
def test_non_bool_under_rules_ignored(t : T?) {
    t |> run("non-bool entries under [rules] are silently ignored") @@(t : T?) {
        write_fixture("[rules]\nSTYLE100 = \"on\"\nSTYLE200 = 1\nSTYLE300 = false\n")
        var d : table<string>
        load_lint_config_from_path(fixture_path(), d)
        // Only STYLE300 (the actual bool) flips state.
        t |> equal(length(d), 1)
        t |> success(key_exists(d, "STYLE300"))
    }
}

[test]
def test_malformed_toml_no_op(t : T?) {
    t |> run("malformed TOML returns null → defaults stand") @@(t : T?) {
        write_fixture("[rules\nSTYLE100 = false\n")        // unclosed header
        var d : table<string>
        d |> insert("STYLE005")
        load_lint_config_from_path(fixture_path(), d)
        // Original disabled-set unchanged.
        t |> equal(length(d), 1)
        t |> success(key_exists(d, "STYLE005"))
    }
}

[test]
def test_no_rules_section_no_op(t : T?) {
    t |> run("file without [rules] section is harmless") @@(t : T?) {
        write_fixture("name = \"unrelated\"\nversion = 1\n")
        var d : table<string>
        d |> insert("STYLE005")
        load_lint_config_from_path(fixture_path(), d)
        // No [rules] → no toggles.
        t |> equal(length(d), 1)
        t |> success(key_exists(d, "STYLE005"))
    }
}

[test]
def test_rule_true_for_unset_code_is_noop(t : T?) {
    t |> run("'X = true' when X is not in the set is harmless") @@(t : T?) {
        write_fixture("[rules]\nSTYLE999 = true\n")
        var d : table<string>
        load_lint_config_from_path(fixture_path(), d)
        t |> equal(length(d), 0)
    }
}

[test]
def test_path_excludes_parse(t : T?) {
    t |> run("[paths] exclude array parses; non-strings ignored") @@(t : T?) {
        write_fixture("[paths]\nexclude = [\"tests/**\", 42, \"benchmarks/*.das\", \"\"]\n[rules]\nSTYLE100 = false\n")
        var ex : array<string>
        load_path_excludes_from_path(fixture_path(), ex)
        t |> equal(length(ex), 2)
        t |> equal(ex[0], "tests/**")
        t |> equal(ex[1], "benchmarks/*.das")
    }
    t |> run("missing [paths] section yields no excludes") @@(t : T?) {
        write_fixture("[rules]\nSTYLE100 = false\n")
        var ex : array<string>
        load_path_excludes_from_path(fixture_path(), ex)
        t |> equal(length(ex), 0)
    }
    t |> run("missing file / malformed TOML yield no excludes") @@(t : T?) {
        var ex : array<string>
        load_path_excludes_from_path("/no/such/lint_config_path_xyz", ex)
        t |> equal(length(ex), 0)
        write_fixture("[paths\nexclude = [\"tests/**\"]\n")       // unclosed header
        load_path_excludes_from_path(fixture_path(), ex)
        t |> equal(length(ex), 0)
    }
}

[test]
def test_path_excludes_match(t : T?) {
    t |> run("root-relative ** glob matches nested paths, either spelling") @@(t : T?) {
        let ex <- ["tests/**"]
        t |> success(matches_path_excludes("tests/lint/foo.das", ex))
        t |> success(matches_path_excludes("{get_das_root()}/tests/lint/foo.das", ex))
        t |> success(matches_path_excludes("tests\\lint\\foo.das", ex))
        t |> success(!matches_path_excludes("utils/tests_helper.das", ex))
        t |> success(!matches_path_excludes("daslib/lint.das", ex))
    }
    t |> run("single * does not cross a separator") @@(t : T?) {
        let ex <- ["tests/*.das"]
        t |> success(matches_path_excludes("tests/a.das", ex))
        t |> success(!matches_path_excludes("tests/sub/a.das", ex))
    }
    t |> run("empty exclude list matches nothing") @@(t : T?) {
        let ex : array<string>
        t |> success(!matches_path_excludes("tests/a.das", ex))
    }
}

def private plant(dir, name, content : string) {
    fopen(path_join(dir, name), "wb") $(f) {
        if (f != null) {
            fwrite(f, content)
        }
    }
}

def private cascade_root() : string {
    let root = path_join(_tmp_dir, "cascade")
    mkdir_rec(path_join(root, "a/b"))
    mkdir(path_join(root, ".git"))
    plant(root, ".lint_config", "[format]\nforce_clean_comments = true\n")
    plant(path_join(root, "a"), ".lint_config", "[format]\nformat_enabled = false\n")
    plant(path_join(root, "a/b"), ".lint_config", "[format]\nforce_clean_comments = false\n")
    return root
}

[test]
def test_format_policy_cascade(t : T?) {
    fixture_path()
    t |> run("no config anywhere: format on, keep comments") @@(t : T?) {
        let bare = path_join(_tmp_dir, "bare")
        mkdir_rec(bare)
        mkdir(path_join(bare, ".git"))
        let pol = format_policy_for(path_join(bare, "x.das"))
        t |> success(pol.format_enabled)
        t |> success(!pol.force_clean_comments)
    }
    t |> run("root config applies to a root file, .git read inclusively") @@(t : T?) {
        let root = cascade_root()
        let pol = format_policy_for(path_join(root, "x.das"))
        t |> success(pol.format_enabled)
        t |> success(pol.force_clean_comments)
    }
    t |> run("mid-level file: own key from a/, missing key cascades to root") @@(t : T?) {
        let root = cascade_root()
        let pol = format_policy_for(path_join(root, "a/x.das"))
        t |> success(!pol.format_enabled)
        t |> success(pol.force_clean_comments)
    }
    t |> run("deep file: each key from its own nearest level") @@(t : T?) {
        let root = cascade_root()
        let pol = format_policy_for(path_join(root, "a/b/x.das"))
        t |> success(!pol.format_enabled)
        t |> success(!pol.force_clean_comments)
    }
    t |> run("malformed config level falls through to its parent") @@(t : T?) {
        let root = cascade_root()
        let bad = path_join(root, "bad")
        mkdir_rec(bad)
        plant(bad, ".lint_config", "[format\nforce_clean_comments = false\n")
        let pol = format_policy_for(path_join(bad, "x.das"))
        t |> success(pol.format_enabled)
        t |> success(pol.force_clean_comments)
    }
    t |> run("non-bool values are ignored, key cascades") @@(t : T?) {
        let root = cascade_root()
        let odd = path_join(root, "odd")
        mkdir_rec(odd)
        plant(odd, ".lint_config", "[format]\nforce_clean_comments = \"nope\"\n")
        let pol = format_policy_for(path_join(odd, "x.das"))
        t |> success(pol.force_clean_comments)
    }
}

[test]
def test_format_policy_per_key_cascade(t : T?) {
    fixture_path()
    t |> run("nearest level wins its key; the other key still cascades from above") @@(t : T?) {
        let root = path_join(_tmp_dir, "perkey")
        mkdir_rec(path_join(root, "a"))
        mkdir(path_join(root, ".git"))
        plant(root, ".lint_config", "[format]\nforce_clean_comments = true\nformat_enabled = true\n")
        plant(path_join(root, "a"), ".lint_config", "[format]\nforce_clean_comments = false\n")
        let pol = format_policy_for(path_join(root, "a/x.das"))
        t |> success(!pol.force_clean_comments, "nearest a/ wins force_clean_comments")
        t |> success(pol.format_enabled, "format_enabled cascades from root")
    }
    t |> run("a decided key is immune to an ancestor's redefinition") @@(t : T?) {
        let root = path_join(_tmp_dir, "immune")
        mkdir_rec(path_join(root, "a/b"))
        mkdir(path_join(root, ".git"))
        plant(root, ".lint_config", "[format]\nforce_clean_comments = true\n")
        plant(path_join(root, "a"), ".lint_config", "[format]\nforce_clean_comments = false\n")
        plant(path_join(root, "a/b"), ".lint_config", "[format]\nforce_clean_comments = true\n")
        let pol = format_policy_for(path_join(root, "a/b/x.das"))
        t |> success(pol.force_clean_comments, "b's own value stands against a's")
    }
    t |> run("a config with no [format] table falls through to its parent") @@(t : T?) {
        let root = path_join(_tmp_dir, "noformat")
        mkdir_rec(path_join(root, "a"))
        mkdir(path_join(root, ".git"))
        plant(root, ".lint_config", "[format]\nforce_clean_comments = true\n")
        plant(path_join(root, "a"), ".lint_config", "[rules]\n")
        let pol = format_policy_for(path_join(root, "a/x.das"))
        t |> success(pol.force_clean_comments, "child without [format] inherits the parent's key")
    }
    t |> run("a bare filename resolves against the working directory's cascade") @@(t : T?) {
        let pol = format_policy_for("x.das")
        t |> success(pol.format_enabled)
        t |> success(!pol.force_clean_comments)
    }
    t |> run("a path outside any repository terminates at the filesystem root") @@(t : T?) {
        let pol = format_policy_for(path_join(_tmp_dir, "no_git_here/x.das"))
        t |> success(pol.format_enabled)
    }
}

[test]
def test_partition_by_format_policy(t : T?) {
    fixture_path()
    t |> run("skips disabled trees, jobs carry their folder's policy") @@(t : T?) {
        let root = path_join(_tmp_dir, "partition")
        mkdir_rec(path_join(root, "swept"))
        mkdir_rec(path_join(root, "corpus"))
        mkdir_rec(path_join(root, "plain"))
        mkdir(path_join(root, ".git"))
        plant(path_join(root, "swept"), ".lint_config", "[format]\nforce_clean_comments = true\n")
        plant(path_join(root, "corpus"), ".lint_config", "[format]\nformat_enabled = false\n")
        var files <- [path_join(root, "swept/a.das"), path_join(root, "swept/a2.das"), path_join(root, "corpus/b.das"), path_join(root, "plain/c.das")]
        var skipped : array<string>
        let jobs <- partition_by_format_policy(files, skipped)
        t |> equal(length(jobs), 3)
        t |> equal(length(skipped), 1)
        t |> success(skipped[0] |> ends_with("b.das"), "disabled tree file skipped")
        t |> success(jobs[0].file |> ends_with("a.das") && jobs[0].strip_comments, "swept survivor strips")
        t |> success(jobs[1].file |> ends_with("a2.das") && jobs[1].strip_comments, "memoized sibling gets the same policy")
        t |> success(jobs[2].file |> ends_with("c.das") && !jobs[2].strip_comments, "plain survivor keeps comments")
    }
}

[test]
def test_daslib_policy_comments_harvested(t : T?) {
    t |> run("daslib ships with force_clean_comments off") @@(t : T?) {
        let pol = format_policy_for(path_join(get_das_root(), "daslib/lint.das"))
        t |> success(pol.format_enabled)
        t |> success(!pol.force_clean_comments, "daslib comments are harvested, not formatter-stripped; re-arming the strip is a decision, not drift")
    }
}
