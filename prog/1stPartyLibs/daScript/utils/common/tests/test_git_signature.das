options gen2

require dastest/testing_boost public
require strings
require daslib/fio
require ../git_signature.das

// ─── helpers ─────────────────────────────────────────────────────────

def make_temp_root(t : T?) : string {
    let r = create_temp_directory_result("gitsig_test")
    if (!(r is value)) {
        t |> failure("could not create temp dir: {unsafe(r.error)}")
        return ""
    }
    return unsafe(r.value)
}

def cleanup_root(root : string) {
    if (!empty(root)) {
        rmdir_rec(root)
    }
}

def md_predicate(p : string) : bool {
    return ends_with(p, ".md")
}

// ─── extract_status_path ─────────────────────────────────────────────

[test]
def test_extract_status_path_basic(t : T?) {
    t |> run("modified file: ' M file.txt' -> 'file.txt'") <| @(t : T?) {
        t |> equal(extract_status_path(" M file.txt"), "file.txt")
    }
    t |> run("untracked file: '?? new.md' -> 'new.md'") <| @(t : T?) {
        t |> equal(extract_status_path("?? new.md"), "new.md")
    }
}

[test]
def test_extract_status_path_rename(t : T?) {
    t |> run("rename: 'R  old.txt -> new.txt' returns the new name") <| @(t : T?) {
        t |> equal(extract_status_path("R  old.txt -> new.txt"), "new.txt")
    }
}

[test]
def test_extract_status_path_short(t : T?) {
    t |> run("line of length <= 3 returns empty") <| @(t : T?) {
        t |> equal(extract_status_path(""), "")
        t |> equal(extract_status_path("M"), "")
        t |> equal(extract_status_path(" M "), "")
    }
}

// ─── compute_filesystem_signature ────────────────────────────────────

[test]
def test_filesystem_signature_deterministic(t : T?) {
    let root = make_temp_root(t)
    if (empty(root)) return
    t |> success(fwrite("{root}/a.md", "alpha"))
    t |> success(fwrite("{root}/b.md", "beta"))
    t |> success(fwrite("{root}/c.md", "gamma"))
    t |> run("two invocations on the same on-disk state produce the same signature") <| @(t : T?) {
        let sig1 = compute_filesystem_signature(root, @(p : string) => md_predicate(p), [root])
        let sig2 = compute_filesystem_signature(root, @(p : string) => md_predicate(p), [root])
        t |> success(!empty(sig1), "first signature should not be empty")
        t |> equal(sig1, sig2)
    }
    cleanup_root(root)
}

[test]
def test_filesystem_signature_predicate_filters(t : T?) {
    let root_a = make_temp_root(t)
    let root_b = make_temp_root(t)
    if (empty(root_a) || empty(root_b)) return
    // root_a: just .md files. root_b: same .md files plus .txt that should
    // be filtered by the predicate. Signatures must be equal.
    t |> success(fwrite("{root_a}/a.md", "alpha"))
    t |> success(fwrite("{root_a}/b.md", "beta"))
    t |> success(fwrite("{root_b}/a.md", "alpha"))
    t |> success(fwrite("{root_b}/b.md", "beta"))
    t |> success(fwrite("{root_b}/skip.txt", "should be filtered"))
    t |> success(fwrite("{root_b}/skip.cmake", "also filtered"))
    t |> run("predicate(.md only) makes .txt/.cmake invisible to the signature") <| @(t : T?) {
        // Signatures hash relative paths; since temp dirs differ, we can't
        // compare across roots. Compute against a SINGLE root with and
        // without the txt files.
        let with_txt = compute_filesystem_signature(root_b, @(p : string) => md_predicate(p), [root_b])
        // Delete .txt files; signature should not change.
        var rm_err : string
        let _drop_txt = remove(path_join(root_b, "skip.txt"), rm_err)
        let _drop_cmake = remove(path_join(root_b, "skip.cmake"), rm_err)
        let after_drop = compute_filesystem_signature(root_b, @(p : string) => md_predicate(p), [root_b])
        t |> success(!empty(with_txt), "should produce a non-empty sig")
        t |> equal(with_txt, after_drop)
    }
    cleanup_root(root_a)
    cleanup_root(root_b)
}

[test]
def test_filesystem_signature_returns_empty_when_dirs_missing(t : T?) {
    t |> run("all search_dirs missing -> empty signature") <| @(t : T?) {
        let sig = compute_filesystem_signature("/", @(p : string) => md_predicate(p), ["/__definitely_does_not_exist_42__"])
        t |> equal(sig, "")
    }
}

// ─── compute_signature ───────────────────────────────────────────────

[test]
def test_compute_signature_returns_non_empty(t : T?) {
    let root = make_temp_root(t)
    if (empty(root)) return
    t |> success(fwrite("{root}/a.md", "alpha"))
    t |> run("returns a non-empty signature with either git: or fs: prefix") <| @(t : T?) {
        let sig = compute_signature(root, @(p : string) => md_predicate(p), [root])
        t |> success(!empty(sig), "compute_signature should not return empty for a populated dir")
        let is_git = starts_with(sig, "git:")
        let is_fs = starts_with(sig, "fs:")
        t |> success(is_git || is_fs, "expected 'git:' or 'fs:' prefix, got: {sig}")
    }
    cleanup_root(root)
}

[test]
def test_compute_signature_no_git_falls_back(t : T?) {
    // create_temp_directory creates dirs in the system temp area (e.g.
    // /var/folders/... on macOS, /tmp on Linux), which is not inside a
    // git checkout. compute_signature should fall back to filesystem-only.
    let root = make_temp_root(t)
    if (empty(root)) return
    t |> success(fwrite("{root}/a.md", "alpha"))
    t |> run("system temp dir (not in git) -> fs: prefix") <| @(t : T?) {
        let sig = compute_signature(root, @(p : string) => md_predicate(p), [root])
        t |> success(starts_with(sig, "fs:"), "expected fs: prefix outside git, got: {sig}")
    }
    cleanup_root(root)
}
