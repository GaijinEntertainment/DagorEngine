options gen2

require dastest/testing_boost public
require tools/common
require daslib/fio
require strings

// Unit tests for the MCP cross-tree guard (tools/common.das): a file whose absolute path is
// outside the server root must warn; in-tree absolute + relative + empty must stay silent.
// Portable — the root is getcwd(), the cross-tree path is a synthetic sibling outside it.

[test]
def test_crosstree_guard(t : T?) {
    let root = getcwd()

    t |> run("in-tree absolute path → silent") @(t : T?) {
        let w = crosstree_warning(path_join(root, "utils/mcp/main.das"), root)
        t |> success(empty(w), "no warning for a file under the server root")
    }

    t |> run("cross-tree absolute path → warns") @(t : T?) {
        // a sibling of the root that isn't under it → relative() yields a leading ".."
        let outside = path_join(dir_name(root), "some-other-worktree/x.das")
        let w = crosstree_warning(outside, root)
        t |> success(!empty(w), "warns for a file outside the server root")
        t |> success(find(w, "CROSS-TREE WARNING") >= 0, "warning is labelled")
    }

    t |> run("relative path → silent") @(t : T?) {
        let w = crosstree_warning("utils/mcp/main.das", root)
        t |> success(empty(w), "relative paths resolve against the server cwd (in-tree)")
    }

    t |> run("empty file arg → silent") @(t : T?) {
        t |> success(empty(crosstree_warning("", root)), "no file, no warning")
    }

    t |> run("a dir named \"..foo\" under the root → silent (not an escape)") @(t : T?) {
        let w = crosstree_warning(path_join(root, "..foo/x.das"), root)
        t |> success(empty(w), "a first segment merely beginning with .. stays in-tree")
    }

    t |> run("relative project_root is resolved to absolute → cross-tree still detected") @(t : T?) {
        let outside = path_join(dir_name(root), "some-other-worktree/x.das")
        // project_root "." resolves to the cwd; the outside file still escapes it (regression: a
        // relative root used to make relative_result error and silence the warning)
        t |> success(!empty(crosstree_warning(outside, ".")), "relative root resolved, still warns")
    }

    t |> run("comma list with a cross-tree first entry → warns") @(t : T?) {
        let outside = path_join(dir_name(root), "some-other-worktree/a.das")
        let w = crosstree_warning("{outside},{path_join(root, "b.das")}", root)
        t |> success(!empty(w), "checks the first path of a comma list")
    }

    t |> run("multi-path parsing: whitespace / non-first / newline entries all checked") @(t : T?) {
        let outside = path_join(dir_name(root), "some-other-worktree/x.das")
        let intree = path_join(root, "a.das")
        t |> success(!empty(crosstree_warning(" {outside}", root)), "leading whitespace is stripped")
        t |> success(!empty(crosstree_warning("{intree},{outside}", root)), "a cross-tree entry that isn't first still warns")
        t |> success(!empty(crosstree_warning("{intree}\n{outside}", root)), "newline-separated lists are split")
    }

    t |> run("guard_crosstree splices a leading content block for cross-tree") @(t : T?) {
        let outside = path_join(dir_name(root), "some-other-worktree/x.das")
        let base = make_tool_result("original output")
        let guarded = guard_crosstree(base, outside, root)
        t |> success(guarded != base, "result changed")
        t |> success(find(guarded, "CROSS-TREE WARNING") >= 0, "warning present")
        t |> success(find(guarded, "original output") >= 0, "original text preserved")
        // in-tree → unchanged
        let g2 = guard_crosstree(base, path_join(root, "x.das"), root)
        t |> success(g2 == base, "in-tree result untouched")
    }

    t |> run("guard_crosstree keeps JSON valid for an empty content array") @(t : T?) {
        let outside = path_join(dir_name(root), "some-other-worktree/x.das")
        let guarded = guard_crosstree("\{\"content\":[],\"isError\":false}", outside, root)
        t |> success(find(guarded, "CROSS-TREE WARNING") >= 0, "warning inserted")
        t |> success(find(guarded, ",]") < 0, "no trailing comma before the array close")
        var err : string
        t |> success(read_json(guarded, err) != null, "result still parses as JSON")
    }
}
