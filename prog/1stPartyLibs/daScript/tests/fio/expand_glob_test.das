options gen2

require dastest/testing_boost
require daslib/fio
require strings

def make_fixture_tree(t : T?; root : string) {
    mkdir_rec("{root}/sub")
    mkdir_rec("{root}/sub/deep")
    mkdir_rec("{root}/empty_subdir")
    mkdir_rec("{root}/other")
    t |> success(fwrite("{root}/foo.das", "// foo\n"))
    t |> success(fwrite("{root}/bar.das", "// bar\n"))
    t |> success(fwrite("{root}/baz.txt", "baz"))
    t |> success(fwrite("{root}/sub/inner.das", "// inner\n"))
    t |> success(fwrite("{root}/sub/deep/deeper.das", "// deeper\n"))
    t |> success(fwrite("{root}/other/o1.das", "// o1\n"))
}

def cleanup_tree(root : string) {
    var error : string
    rmdir_rec(root, error)
}

def has_path(files : array<string>; suffix : string) : bool {
    for (f in files) {
        if (find(f, suffix) >= 0) {
            return true
        }
    }
    return false
}

[test]
def test_parse_file_list_single_plain(t : T?) {
    t |> run("single plain file passes through unchanged") @(t : T?) {
        var files : array<string>
        parse_file_list("path/to/file.das", files)
        t |> equal(length(files), 1)
        t |> equal(files[0], "path/to/file.das")
    }
}

[test]
def test_parse_file_list_single_glob(t : T?) {
    let tmp = create_temp_directory_result("expand_glob_single_")
    if (!(tmp is value)) {
        t |> failure("could not create temp directory: {tmp as error}")
        return
    }
    let root = tmp as value
    make_fixture_tree(t, root)
    t |> run("single glob expands to matches") @(t : T?) {
        var files : array<string>
        parse_file_list("{root}/*.das", files)
        t |> success(length(files) >= 2, "at least foo.das and bar.das")
        t |> success(has_path(files, "foo.das"), "foo.das matched")
        t |> success(has_path(files, "bar.das"), "bar.das matched")
    }
    cleanup_tree(root)
}

[test]
def test_parse_file_list_comma_plain(t : T?) {
    t |> run("comma list of plain files") @(t : T?) {
        var files : array<string>
        parse_file_list("a.das,b.das,c.das", files)
        t |> equal(length(files), 3)
        t |> equal(files[0], "a.das")
        t |> equal(files[1], "b.das")
        t |> equal(files[2], "c.das")
    }
}

[test]
def test_parse_file_list_comma_globs(t : T?) {
    let tmp = create_temp_directory_result("expand_glob_comma_")
    if (!(tmp is value)) {
        t |> failure("could not create temp directory: {tmp as error}")
        return
    }
    let root = tmp as value
    make_fixture_tree(t, root)
    t |> run("comma list of globs (issue #2498 repro)") @(t : T?) {
        var files : array<string>
        parse_file_list("{root}/*.das,{root}/other/*.das", files)
        t |> success(has_path(files, "foo.das"), "first glob expanded")
        t |> success(has_path(files, "o1.das"), "second glob expanded")
    }
    cleanup_tree(root)
}

[test]
def test_parse_file_list_mixed(t : T?) {
    let tmp = create_temp_directory_result("expand_glob_mixed_")
    if (!(tmp is value)) {
        t |> failure("could not create temp directory: {tmp as error}")
        return
    }
    let root = tmp as value
    make_fixture_tree(t, root)
    t |> run("mixed plain + glob") @(t : T?) {
        var files : array<string>
        parse_file_list("literal.das,{root}/foo.das,{root}/sub/*.das", files)
        t |> success(has_path(files, "literal.das"), "plain entry preserved")
        t |> success(has_path(files, "foo.das"), "explicit file resolved")
        t |> success(has_path(files, "inner.das"), "glob expanded")
    }
    cleanup_tree(root)
}

[test]
def test_parse_file_list_newline(t : T?) {
    t |> run("newline-delimited") @(t : T?) {
        var files : array<string>
        parse_file_list("a.das\nb.das\nc.das", files)
        t |> equal(length(files), 3)
        t |> equal(files[0], "a.das")
        t |> equal(files[1], "b.das")
        t |> equal(files[2], "c.das")
    }
}

[test]
def test_parse_file_list_whitespace(t : T?) {
    t |> run("mixed comma + newline + whitespace") @(t : T?) {
        var files : array<string>
        parse_file_list("  a.das , b.das \n c.das ", files)
        t |> equal(length(files), 3)
        t |> equal(files[0], "a.das")
        t |> equal(files[2], "c.das")
    }
    t |> run("single fast-path strips whitespace") @(t : T?) {
        var files : array<string>
        parse_file_list("  a.das  ", files)
        t |> equal(length(files), 1)
        t |> equal(files[0], "a.das")
    }
}

[test]
def test_parse_file_list_empty_parts(t : T?) {
    t |> run("empty parts skipped") @(t : T?) {
        var files : array<string>
        parse_file_list("a.das,,\n,b.das", files)
        t |> equal(length(files), 2, "2 non-empty files")
    }
    t |> run("single fast-path skips empty/whitespace-only") @(t : T?) {
        var files : array<string>
        parse_file_list("   ", files)
        t |> equal(length(files), 0)
    }
}

[test]
def test_parse_file_list_order_preserved(t : T?) {
    let tmp = create_temp_directory_result("expand_glob_order_")
    if (!(tmp is value)) {
        t |> failure("could not create temp directory: {tmp as error}")
        return
    }
    let root = tmp as value
    make_fixture_tree(t, root)
    t |> run("expand_glob preserves prior entries (no global re-sort)") @(t : T?) {
        // Regression Copilot caught: expand_glob's sort() once scrambled
        // entries pushed before it. Order must hold across (plain, glob, plain).
        var files : array<string>
        parse_file_list("zzz.das,{root}/*.das,aaa.das", files)
        t |> equal(files[0], "zzz.das", "first plain stays first")
        t |> equal(files[length(files) - 1], "aaa.das", "last plain stays last")
    }
    cleanup_tree(root)
}

[test]
def test_expand_glob_recursive(t : T?) {
    let tmp = create_temp_directory_result("expand_glob_recursive_")
    if (!(tmp is value)) {
        t |> failure("could not create temp directory: {tmp as error}")
        return
    }
    let root = tmp as value
    make_fixture_tree(t, root)
    t |> run("** in middle of pattern walks from literal prefix") @(t : T?) {
        // Pattern with `**` must walk from the literal prefix, not from a
        // literal `**` directory that does not exist.
        var files : array<string>
        parse_file_list("{root}/**/inner.das", files)
        t |> success(has_path(files, "inner.das"))
    }
    cleanup_tree(root)
}

[test]
def test_expand_glob_shallow(t : T?) {
    let tmp = create_temp_directory_result("expand_glob_shallow_")
    if (!(tmp is value)) {
        t |> failure("could not create temp directory: {tmp as error}")
        return
    }
    let root = tmp as value
    make_fixture_tree(t, root)
    t |> run("shallow pattern does not descend into subdirectories") @(t : T?) {
        // Patterns without `**` use the non-recursive walker. `{root}/*.das`
        // must NOT pick up files under `{root}/sub/` or `{root}/other/`.
        var files : array<string>
        parse_file_list("{root}/*.das", files)
        for (f in files) {
            t |> success(find(f, "/sub/") < 0, "shallow * did not descend into sub/")
            t |> success(find(f, "/other/") < 0, "shallow * did not descend into other/")
        }
    }
    cleanup_tree(root)
}

[test]
def test_expand_glob_multi_segment(t : T?) {
    let tmp = create_temp_directory_result("expand_glob_multiseg_")
    if (!(tmp is value)) {
        t |> failure("could not create temp directory: {tmp as error}")
        return
    }
    let root = tmp as value
    make_fixture_tree(t, root)
    t |> run("multi-segment pattern without ** still recurses") @(t : T?) {
        // A pattern like `{root}/*/inner.das` has no `**` but it crosses
        // a `/`, so the recursive walk must be used.
        var files : array<string>
        parse_file_list("{root}/*/inner.das", files)
        t |> success(has_path(files, "inner.das"),
            "multi-segment pattern matched sub/inner.das")
    }
    cleanup_tree(root)
}

[test]
def test_expand_glob_backslash(t : T?) {
    let tmp = create_temp_directory_result("expand_glob_backslash_")
    if (!(tmp is value)) {
        t |> failure("could not create temp directory: {tmp as error}")
        return
    }
    let root = tmp as value
    make_fixture_tree(t, root)
    t |> run("backslash-style pattern matches the same as forward-slash") @(t : T?) {
        // The pattern is normalized to `/` inside expand_glob, so backslash
        // patterns produce the same result as POSIX `/` patterns. Build the
        // backslash pattern by replacing `/` with `\` in the same input.
        let fwd = "{root}/sub/*.das"
        let back = replace(fwd, "/", "\\")
        var slash : array<string>
        var bs : array<string>
        parse_file_list(fwd, slash)
        parse_file_list(back, bs)
        sort(slash)
        sort(bs)
        t |> equal(length(slash), length(bs), "same number of matches")
        for (i in range(length(slash))) {
            t |> equal(slash[i], bs[i], "entry {i} matches")
        }
    }
    cleanup_tree(root)
}

[test]
def test_expand_glob_absolute(t : T?) {
    let tmp = create_temp_directory_result("expand_glob_abs_")
    if (!(tmp is value)) {
        t |> failure("could not create temp directory: {tmp as error}")
        return
    }
    let root = tmp as value
    make_fixture_tree(t, root)
    t |> run("absolute path patterns walk the root, not cwd") @(t : T?) {
        // _split_literal_prefix keeps the separator in dir_part when the
        // pattern starts at the filesystem root, so glob walks the right
        // place rather than an empty path.
        var files : array<string>
        parse_file_list("{root}/*.das", files)
        t |> success(length(files) > 0, "absolute pattern matched at least one file")
        t |> success(has_path(files, "foo.das"), "foo.das present")
    }
    t |> run("absolute root prefix produces clean paths (no double slash)") @(t : T?) {
        // When dir_part ends with `/` (POSIX root or Windows drive root),
        // expand_glob must not insert another `/` when concatenating with
        // the matched entry.
        var files : array<string>
        parse_file_list("{root}/*.das", files)
        for (f in files) {
            t |> success(find(f, "//") < 0, "no double slashes in '{f}'")
        }
    }
    cleanup_tree(root)
}

[test]
def test_expand_glob_charclass(t : T?) {
    let tmp = create_temp_directory_result("expand_glob_charclass_")
    if (!(tmp is value)) {
        t |> failure("could not create temp directory: {tmp as error}")
        return
    }
    let root = tmp as value
    make_fixture_tree(t, root)
    t |> run("character class triggers glob expansion") @(t : T?) {
        // parse_file_list dispatches via fio.is_glob_pattern, which
        // recognizes `[` (not just `*` / `?`).
        var files : array<string>
        parse_file_list("{root}/[fb]*.das", files)
        t |> success(has_path(files, "foo.das"), "[fb]* matched foo.das")
        t |> success(has_path(files, "bar.das"), "[fb]* matched bar.das")
    }
    cleanup_tree(root)
}

[test]
def test_expand_glob_filters_dirs(t : T?) {
    let tmp = create_temp_directory_result("expand_glob_dirs_")
    if (!(tmp is value)) {
        t |> failure("could not create temp directory: {tmp as error}")
        return
    }
    let root = tmp as value
    make_fixture_tree(t, root)
    t |> run("shallow expand_glob filters out directories") @(t : T?) {
        // The shallow `dir(...)` branch must stat each match and skip dirs
        // — downstream tools (format/lint/compile_check) take regular files.
        var files : array<string>
        parse_file_list("{root}/*", files)
        for (f in files) {
            let st = stat(f)
            t |> success(!(st.is_valid && st.is_dir),
                "expand_glob must not emit directory '{f}'")
        }
    }
    cleanup_tree(root)
}
