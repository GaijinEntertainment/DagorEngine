options gen2
require dastest/testing_boost public

require daslib/fio
require daslib/module_path

// The resolver's second argument is is_standalone_exe(), folded at the call
// site in a real build. These tests call the builtin directly with an explicit
// flag so they are deterministic under any execution tier.

[test]
def test_real_project_local_path_returns_baked_dir(t : T?) {
    t |> run("dev (not standalone): real project-local path returns its dir_name") @(t : T?) {
        // This test file is project-local (sits in tests/fio/, no /modules/ segment).
        let real_baked = "{get_das_root()}/tests/fio/get_this_module_dir_resolver.das"
        let dir = __builtin_resolve_this_module_dir(real_baked, false)
        t |> equal(dir, "{get_das_root()}/tests/fio")
    }
}

[test]
def test_standalone_project_local_prefers_exe_dir(t : T?) {
    t |> run("standalone: project-local path prefers exe dir over the (existing) baked dir") @(t : T?) {
        // The dictation-bot release-config bug: on the author's own machine the
        // dev source dir still exists, so a standalone exe must NOT read from it —
        // it must resolve next to the exe, where the release ships its config.
        let real_baked = "{get_das_root()}/tests/fio/get_this_module_dir_resolver.das"
        let dir = __builtin_resolve_this_module_dir(real_baked, true)
        t |> success(dir != "{get_das_root()}/tests/fio",
            "standalone must not return the baked source dir, got '{dir}'")
        t |> success(fexist(dir), "exe dir '{dir}' must exist on disk")
    }
}

[test]
def test_nonexistent_project_local_falls_back_to_exe_dir(t : T?) {
    t |> run("dev (not standalone): nonexistent project-local baked path falls back to an existing dir") @(t : T?) {
        // No /modules/ segment AND dir doesn't exist on disk → should fall
        // back to <exe_dir> rather than returning the dead path. Models the
        // relocated-bundle scenario where main.das's dev-time dir is gone.
        let dead = "/__no_such_dir_42__/__no_such_subdir__/main.das"
        let dir = __builtin_resolve_this_module_dir(dead, false)
        t |> success(dir != "/__no_such_dir_42__/__no_such_subdir__",
            "must not return the dead baked dir, got '{dir}'")
        t |> success(fexist(dir), "fallback dir '{dir}' must exist on disk")
    }
}

[test]
def test_nonexistent_modules_path_keeps_baked(t : T?) {
    t |> run("dev (not standalone): nonexistent /modules/ baked path returns baked dir unchanged") @(t : T?) {
        // When rel is non-empty and not standalone the fallback does NOT fire —
        // preserves existing tier-3 behavior for module paths so we don't
        // accidentally mask a missing module with the exe dir.
        let dead = "/__no_such_root__/modules/__no_such_module__/foo.das"
        let dir = __builtin_resolve_this_module_dir(dead, false)
        t |> equal(dir, "/__no_such_root__/modules/__no_such_module__")
    }
}

[test]
def test_standalone_modules_path_prefers_exe_dir(t : T?) {
    t |> run("standalone: nonexistent /modules/ baked path still refuses the dev dir") @(t : T?) {
        // rel non-empty, but neither <exe_dir>/<rel> nor <das_root>/<rel> exists;
        // a standalone exe must fall back to <exe_dir>, never the baked dev path.
        let dead = "/__no_such_root__/modules/__no_such_module__/foo.das"
        let dir = __builtin_resolve_this_module_dir(dead, true)
        t |> success(dir != "/__no_such_root__/modules/__no_such_module__",
            "standalone must not return the dead baked dir, got '{dir}'")
        t |> success(fexist(dir), "fallback dir '{dir}' must exist on disk")
    }
}

[test]
def test_empty_baked_returns_empty(t : T?) {
    t |> run("empty input returns empty") @(t : T?) {
        t |> equal(__builtin_resolve_this_module_dir("", false), "")
        t |> equal(__builtin_resolve_this_module_dir("", true), "")
    }
}
