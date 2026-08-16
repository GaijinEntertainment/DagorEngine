options gen2

require dastest/testing_boost
require daslib/lint_config
require daslib/fio

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
