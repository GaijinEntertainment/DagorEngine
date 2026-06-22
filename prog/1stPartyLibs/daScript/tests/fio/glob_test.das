options gen2

require dastest/testing_boost
require daslib/fio
require strings

[test]
def test_match_glob_literal(t : T?) {
    t |> run("exact literal matches") @(t : T?) {
        t |> success(match_glob("foo", "foo"))
        t |> success(match_glob("foo/bar", "foo/bar"))
        t |> success(!match_glob("foo", "bar"))
        t |> success(!match_glob("foo", "foob"))
        t |> success(!match_glob("foo", "fo"))
        t |> success(!match_glob("foo/bar", "foo/baz"))
    }
}

[test]
def test_match_glob_star(t : T?) {
    t |> run("single star within segment") @(t : T?) {
        t |> success(match_glob("*.txt", "hello.txt"))
        t |> success(match_glob("*.txt", ".txt"))
        t |> success(!match_glob("*.txt", "hello.md"))
        t |> success(match_glob("a*c", "abc"))
        t |> success(match_glob("a*c", "ac"))
        t |> success(match_glob("a*c", "axxxc"))
        t |> success(!match_glob("a*c", "abd"))
    }
    t |> run("single star does not cross /") @(t : T?) {
        t |> success(!match_glob("*.txt", "dir/hello.txt"))
        t |> success(!match_glob("a*c", "a/c"))
        t |> success(match_glob("dir/*.txt", "dir/hello.txt"))
        t |> success(!match_glob("dir/*.txt", "dir/sub/hello.txt"))
    }
}

[test]
def test_match_glob_double_star(t : T?) {
    t |> run("** matches across /") @(t : T?) {
        t |> success(match_glob("**", "anything"))
        t |> success(match_glob("**", "a/b/c"))
        t |> success(match_glob("**/foo.txt", "foo.txt"))
        t |> success(match_glob("**/foo.txt", "a/foo.txt"))
        t |> success(match_glob("**/foo.txt", "a/b/c/foo.txt"))
        t |> success(match_glob("a/**/b", "a/b"))
        t |> success(match_glob("a/**/b", "a/x/b"))
        t |> success(match_glob("a/**/b", "a/x/y/z/b"))
        t |> success(!match_glob("a/**/b", "x/a/b"))
    }
    t |> run("trailing /** requires at least one segment under root") @(t : T?) {
        t |> success(match_glob("a/**", "a/b"))
        t |> success(match_glob("a/**", "a/b/c"))
        t |> success(!match_glob("a/**", "a"))   // shell glob convention: a/** does NOT match a itself
    }
    t |> run("** combined with extension filter") @(t : T?) {
        t |> success(match_glob("assets/**/*.png", "assets/icon.png"))
        t |> success(match_glob("assets/**/*.png", "assets/sub/icon.png"))
        t |> success(match_glob("assets/**/*.png", "assets/a/b/c/icon.png"))
        t |> success(!match_glob("assets/**/*.png", "assets/icon.jpg"))
        t |> success(!match_glob("assets/**/*.png", "other/icon.png"))
    }
    t |> run("**/ requires a / boundary when ** consumes characters") @(t : T?) {
        // Round-5 regression: the `/` after `**` is optional only when `**`
        // matches empty. Pattern `a/**/b` must NOT match `a/xb` — there is
        // no `/` separator between the `**` consumption and `b`.
        t |> success(!match_glob("a/**/b", "a/xb"))
        t |> success(!match_glob("**/foo.txt", "afoo.txt"))
        t |> success(!match_glob("assets/**/*.png", "assetsicon.png"))
        // Zero-segment path still works (empty `**` match).
        t |> success(match_glob("**/foo.txt", "foo.txt"))
        t |> success(match_glob("a/**/b", "a/b"))
    }
}

[test]
def test_match_glob_question(t : T?) {
    t |> run("? matches exactly one non-/ char") @(t : T?) {
        t |> success(match_glob("?ello", "hello"))
        t |> success(match_glob("h?llo", "hello"))
        t |> success(match_glob("hell?", "hello"))
        t |> success(!match_glob("?ello", "ello"))         // zero chars
        t |> success(!match_glob("?ello", "hhello"))       // two chars
        t |> success(!match_glob("?", "/"))                // ? does not match /
    }
}

[test]
def test_match_glob_charclass(t : T?) {
    t |> run("[abc] matches any listed char") @(t : T?) {
        t |> success(match_glob("[abc]ar", "bar"))
        t |> success(match_glob("[abc]ar", "aar"))
        t |> success(match_glob("[abc]ar", "car"))
        t |> success(!match_glob("[abc]ar", "dar"))
    }
    t |> run("[a-z] matches a range") @(t : T?) {
        t |> success(match_glob("[a-z]", "m"))
        t |> success(match_glob("[a-z]", "a"))
        t |> success(match_glob("[a-z]", "z"))
        t |> success(!match_glob("[a-z]", "A"))
        t |> success(!match_glob("[a-z]", "0"))
        t |> success(match_glob("[0-9][0-9]", "42"))
    }
    t |> run("[!abc] / [^abc] negates") @(t : T?) {
        t |> success(match_glob("[!abc]", "x"))
        t |> success(!match_glob("[!abc]", "a"))
        t |> success(match_glob("[^abc]", "x"))
        t |> success(!match_glob("[^abc]", "b"))
    }
    t |> run("unterminated [ matches as a literal (POSIX fnmatch)") @(t : T?) {
        // POSIX: a `[` with no closing `]` is treated as a literal `[`.
        t |> success(match_glob("[abc", "[abc"))
        t |> success(match_glob("[", "["))
        t |> success(!match_glob("[", "x"))
        t |> success(!match_glob("[abc", "abc"))
    }
    t |> run("] right after [ or [! is a literal class member") @(t : T?) {
        // POSIX: the first `]` after `[` (or `[!` / `[^`) is in the class,
        // not the terminator.
        t |> success(match_glob("[]ab]", "]"))
        t |> success(match_glob("[]ab]", "a"))
        t |> success(!match_glob("[]ab]", "c"))
        t |> success(match_glob("[!]ab]", "x"))
        t |> success(!match_glob("[!]ab]", "]"))
    }
    t |> run("class can hold metacharacters as literals") @(t : T?) {
        // No backslash escape mechanism — `[*]` is the documented way to
        // match a literal `*` (and likewise for `?` and `[`).
        t |> success(match_glob("[*]", "*"))
        t |> success(!match_glob("[*]", "a"))
        t |> success(match_glob("[?]", "?"))
        t |> success(!match_glob("[?]", "a"))
    }
    t |> run("- at start or end of class is a literal") @(t : T?) {
        // POSIX: a `-` adjacent to `[` / `[!` / `]` is a literal `-`,
        // not a range marker.
        t |> success(match_glob("[a-]", "a"))
        t |> success(match_glob("[a-]", "-"))
        t |> success(!match_glob("[a-]", "b"))
        t |> success(match_glob("[-a]", "-"))
        t |> success(match_glob("[-a]", "a"))
    }
}

[test]
def test_match_glob_edge_cases(t : T?) {
    t |> run("empty patterns and paths") @(t : T?) {
        t |> success(match_glob("", ""))
        t |> success(!match_glob("", "x"))
        t |> success(!match_glob("x", ""))
        t |> success(match_glob("*", ""))
        t |> success(match_glob("**", ""))
    }
}

[test]
def test_is_glob_pattern(t : T?) {
    t |> run("detects glob metacharacters") @(t : T?) {
        t |> success(is_glob_pattern("*.txt"))
        t |> success(is_glob_pattern("**/foo"))
        t |> success(is_glob_pattern("file?.txt"))
        t |> success(is_glob_pattern("[abc].das"))
        t |> success(!is_glob_pattern("plain.txt"))
        t |> success(!is_glob_pattern(""))
        t |> success(!is_glob_pattern("a/b/c.das"))
    }
}

def make_fixture_tree(t : T?; root : string) {
    mkdir_rec("{root}/assets/sub")
    mkdir_rec("{root}/src")
    mkdir_rec("{root}/tests/inner")
    t |> success(fwrite("{root}/main.das", "// main\n"))
    t |> success(fwrite("{root}/README.md", "# readme\n"))
    t |> success(fwrite("{root}/assets/icon.png", "PNG"))
    t |> success(fwrite("{root}/assets/banner.jpg", "JPG"))
    t |> success(fwrite("{root}/assets/sub/deep.png", "PNG"))
    t |> success(fwrite("{root}/src/util.das", "// util\n"))
    t |> success(fwrite("{root}/tests/test_main.das", "// test\n"))
    t |> success(fwrite("{root}/tests/inner/test_inner.das", "// inner\n"))
}

def cleanup_tree(root : string) {
    var error : string
    rmdir_rec(root, error)
}

[test]
def test_glob_filesystem(t : T?) {
    let tmp = create_temp_directory_result("glob_test_")
    if (!(tmp is value)) {
        t |> failure("could not create temp directory: {tmp as error}")
        return
    }
    let root = tmp as value
    make_fixture_tree(t, root)
    t |> run("glob picks PNG files anywhere") @(t : T?) {
        var hits : array<string>
        glob(root, "**/*.png") $(filename, is_dir) {
            if (!is_dir) {
                hits |> push_clone(filename)
            }
        }
        t |> equal(length(hits), 2)
    }
    t |> run("glob picks .das files in src only") @(t : T?) {
        var hits : array<string>
        glob(root, "src/*.das") $(filename, is_dir) {
            if (!is_dir) {
                hits |> push_clone(filename)
            }
        }
        t |> equal(length(hits), 1)
    }
    t |> run("glob with ** picks all .das files") @(t : T?) {
        var hits : array<string>
        glob(root, "**/*.das") $(filename, is_dir) {
            if (!is_dir) {
                hits |> push_clone(filename)
            }
        }
        t |> equal(length(hits), 4)
    }
    t |> run("glob_filtered honors excludes") @(t : T?) {
        var hits : array<string>
        var includes : array<string>
        var excludes : array<string>
        includes |> push_clone("**/*.das")
        excludes |> push_clone("tests/**")
        glob_filtered(root, includes, excludes) $(filename, is_dir) {
            if (!is_dir) {
                hits |> push_clone(filename)
            }
        }
        t |> equal(length(hits), 2)
    }
    t |> run("glob callback always yields /-separated paths") @(t : T?) {
        // Round-2 fix: glob must invoke `blk` with the normalized path,
        // not the OS-native one. On macOS/Linux this is a tautology; on
        // Windows it's the load-bearing assertion.
        glob(root, "**/*.das") $(filename, is_dir) {
            t |> success(find(filename, "\\") < 0,
                "callback path '{filename}' must not contain backslashes")
        }
    }
    t |> run("glob_filtered callback always yields /-separated paths") @(t : T?) {
        var includes : array<string>
        let excludes : array<string>
        includes |> push_clone("**/*.das")
        glob_filtered(root, includes, excludes) $(filename, is_dir) {
            t |> success(find(filename, "\\") < 0,
                "callback path '{filename}' must not contain backslashes")
        }
    }
    t |> run("glob on non-existent root yields nothing, no error") @(t : T?) {
        // dir_rec on a missing path silently no-ops via the non-erroring
        // overload; glob/glob_filtered piggyback on that. Verify the contract:
        // no panic, no callback invocations.
        var hits : array<string>
        glob("{root}/_no_such_dir_{get_clock()}", "**/*") $(filename, is_dir) {
            hits |> push_clone(filename)
        }
        t |> equal(length(hits), 0)
    }
    t |> run("glob_filtered with empty includes yields nothing") @(t : T?) {
        // An empty include list means "match nothing", so the callback must
        // not fire even when entries exist under root.
        var hits : array<string>
        let includes : array<string>
        let excludes : array<string>
        glob_filtered(root, includes, excludes) $(filename, is_dir) {
            hits |> push_clone(filename)
        }
        t |> equal(length(hits), 0)
    }
    cleanup_tree(root)
}

[test]
def test_to_generic_path(t : T?) {
    t |> run("backslashes become forward slashes") @(t : T?) {
        t |> equal(to_generic_path("a\\b\\c"), "a/b/c")
        t |> equal(to_generic_path("a/b\\c/d"), "a/b/c/d")
    }
    t |> run("already-generic paths pass through") @(t : T?) {
        t |> equal(to_generic_path("a/b/c"), "a/b/c")
        t |> equal(to_generic_path("plain"), "plain")
        t |> equal(to_generic_path(""), "")
    }
}
