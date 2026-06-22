options gen2
require dastest/testing_boost public

require daslib/fio
require daslib/module_path

[test]
def test_real_project_local_path_returns_baked_dir(t : T?) {
    t |> run("real project-local path returns its dir_name") @(t : T?) {
        // This test file is project-local (sits in tests/fio/, no /modules/ segment).
        let real_baked = "{get_das_root()}/tests/fio/get_this_module_dir_resolver.das"
        let dir = __builtin_resolve_this_module_dir(real_baked)
        t |> equal(dir, "{get_das_root()}/tests/fio")
    }
}

[test]
def test_nonexistent_project_local_falls_back_to_exe_dir(t : T?) {
    t |> run("nonexistent project-local baked path falls back to an existing dir") @(t : T?) {
        // No /modules/ segment AND dir doesn't exist on disk → should fall
        // back to <exe_dir> rather than returning the dead path. Models the
        // relocated-bundle scenario where main.das's dev-time dir is gone.
        let dead = "/__no_such_dir_42__/__no_such_subdir__/main.das"
        let dir = __builtin_resolve_this_module_dir(dead)
        t |> success(dir != "/__no_such_dir_42__/__no_such_subdir__",
            "must not return the dead baked dir, got '{dir}'")
        t |> success(fexist(dir), "fallback dir '{dir}' must exist on disk")
    }
}

[test]
def test_nonexistent_modules_path_keeps_baked(t : T?) {
    t |> run("nonexistent /modules/ baked path returns baked dir unchanged") @(t : T?) {
        // When rel is non-empty the new fallback does NOT fire — preserves
        // existing tier-3 behavior for module paths so we don't accidentally
        // mask a missing module with the exe dir.
        let dead = "/__no_such_root__/modules/__no_such_module__/foo.das"
        let dir = __builtin_resolve_this_module_dir(dead)
        t |> equal(dir, "/__no_such_root__/modules/__no_such_module__")
    }
}

[test]
def test_empty_baked_returns_empty(t : T?) {
    t |> run("empty input returns empty") @(t : T?) {
        t |> equal(__builtin_resolve_this_module_dir(""), "")
    }
}
