// Shared git-aware staleness signature for utils/* index caches.
//
// Used by `utils/mcp/tools/cpp_common.das` (cpp source-search index).
// Consumers plug in their own path predicate and search-dirs scope; this
// module owns the git plumbing, the filesystem fallback, and the
// determinism guarantees (sorted fs walk, per-tree HEAD hash).

options gen2

require strings public
require daslib/fio public
require daslib/strings_boost public

// Repo-relative path with `/`-separators; falls back to `path` on error
// (mirrors `make_relative_path` from utils/mcp/tools/common.das, kept
// local so this module has no cross-tool dependencies).
def private to_relative(path, base : string) : string {
    var err : string
    let rel = relative(path, base, err)
    return to_generic_path(empty(rel) ? path : rel)
}

// Strip `git status --porcelain` 3-char status prefix; return the new
// name on rename lines (`R  old -> new`).
def public extract_status_path(line : string) : string {
    if (length(line) <= 3) return ""
    var path = slice(line, 3)
    let arrow = find(path, " -> ")
    if (arrow >= 0) {
        path = slice(path, arrow + 4)
    }
    return path
}

// stat() wrapper that returns 0 on missing/error so signature compute
// doesn't crash on a deleted/inaccessible file.
def public stat_mtime_or_zero(path : string) : int64 {
    let st = stat(path)
    return st.is_valid ? int64(st.mtime) : 0l
}

def private is_directory(path : string) : bool {
    let s = stat(path)
    return s.is_valid && s.is_dir
}

// Filesystem-only fallback signature. Walks each `search_dirs_abs`
// recursively, collects (repo_rel_path, mtime) for files passing
// `predicate`, **sorts the collection by path**, then hashes. The sort
// makes the signature deterministic across platforms — `_findfirst` on
// Windows and `readdir()` on Unix don't guarantee stable order, so the
// raw walk order would otherwise produce different signatures for the
// same on-disk state.
//
// Returns "fs:HASH" on success, "" if every search_dir is missing.
def public compute_filesystem_signature(
                                        root : string;
                                        predicate : lambda<(repo_rel_path : string) : bool>;
                                        search_dirs_abs : array<string>
                                        ) : string {
    var collected : array<tuple<string; int64>>
    var any_dir = false
    for (sd in search_dirs_abs) {
        if (!is_directory(sd)) continue
        any_dir = true
        var sd_rel = to_relative(sd, root)
        if (sd_rel == ".") {
            sd_rel = ""
        }
        dir_rec(sd) $(filename, is_dir) {
            if (is_dir) return
            let leaf = to_generic_path(filename)
            let rel = empty(sd_rel) ? leaf : "{sd_rel}/{leaf}"
            if (!invoke(predicate, rel)) return
            collected |> push((rel, stat_mtime_or_zero(path_join(sd, filename))))
        }
    }
    if (!any_dir) return ""
    sort(collected) <| $(a, b) => a._0 < b._0
    let combined = build_string() $(var w) {
        for (item in collected) {
            w |> write("{item._0}:{item._1}\n")
        }
    }
    return "fs:{hash(combined)}"
}

// Compute a git-aware staleness signature for the index covering
// `search_dirs_abs`, considering only paths matching `predicate`.
//
// `root` is any directory inside the repo; the module discovers the git
// toplevel via `git -C root rev-parse --show-toplevel`.
//
// `predicate(repo_rel_path)` decides whether a path is part of the index
// (e.g. ends_with(".md") for docs, ends_with(".cpp"|".h"|".hpp") for cpp).
// Status output is pre-narrowed to paths under any `search_dirs_abs`
// entry, so the predicate only needs the file-class filter.
//
// Strategy:
//   1. Discover toplevel; on failure → compute_filesystem_signature.
//   2. For each search_dir, `git rev-parse HEAD:<dir_rel>` → tree hash.
//      Per-tree hashing fixes the global-HEAD over-invalidation: only
//      commits that change a tracked tree under an indexed dir trigger
//      a rebuild. (Branch switches that don't touch indexed trees
//      become no-ops; `git pull` of unrelated subtrees stops re-churning
//      the index.) On rev-parse error for a dir (untracked at HEAD), use
//      a placeholder so the next status line catches new files.
//   3. `git status --porcelain --untracked-files=normal`. Per line:
//      extract path; reject if not under any search_dir; reject if
//      predicate is false; otherwise fold "<line>:<mtime>" into hash.
//   4. Return "git:HASH". Any git step error → fall back to fs.
//
// Returns "" only if both git and the filesystem walk fail (caller
// should treat empty as "do not invalidate blindly").
def public compute_signature(
                             root : string;
                             predicate : lambda<(repo_rel_path : string) : bool>;
                             search_dirs_abs : array<string>
                             ) : string {
    var top_buf : string
    if (run_and_capture(["git", "-C", root, "rev-parse", "--show-toplevel"], top_buf, 5.0) != 0
        || empty(strip(top_buf))) return compute_filesystem_signature(root, predicate, search_dirs_abs)
    let toplevel = strip(top_buf)

    var dirs_rel : array<string>
    dirs_rel |> reserve(length(search_dirs_abs))
    for (sd in search_dirs_abs) {
        var rel = to_relative(sd, toplevel)
        if (rel == ".") {
            rel = ""
        }
        dirs_rel |> push(rel)
    }
    // Sort so the tree-hash fold below produces an order-independent
    // signature regardless of how the caller ordered `search_dirs_abs`.
    sort(dirs_rel) <| $(a, b) => a < b

    var tree_parts : array<string>
    for (rel in dirs_rel) {
        var tb : string
        // Empty rel means search_dir == toplevel; rev-parse HEAD: errors,
        // so use bare HEAD in that case.
        let spec = empty(rel) ? "HEAD" : "HEAD:{rel}"
        if (run_and_capture(["git", "-C", toplevel, "rev-parse", spec], tb, 5.0) == 0
            && !empty(strip(tb))) {
            tree_parts |> push("{rel}={strip(tb)}")
        } else {
            tree_parts |> push("{rel}=untracked")
        }
    }

    var status_buf : string
    if (run_and_capture(["git", "-C", toplevel, "status", "--porcelain", "--untracked-files=normal"],
                        status_buf, 5.0) != 0) return compute_filesystem_signature(root, predicate, search_dirs_abs)
    let lines <- split(status_buf, "\n")
    let combined = build_string() $(var w) {
        for (t in tree_parts) {
            w |> write("{t}\n")
        }
        for (line in lines) {
            let p = extract_status_path(line)
            if (empty(p)) continue
            var under = false
            for (rel in dirs_rel) {
                if (empty(rel) || p == rel || starts_with(p, "{rel}/")) {
                    under = true
                    break
                }
            }
            if (!under || !invoke(predicate, p)) continue
            w |> write("{line}:{stat_mtime_or_zero(path_join(toplevel, p))}\n")
        }
    }
    return "git:{hash(combined)}"
}
