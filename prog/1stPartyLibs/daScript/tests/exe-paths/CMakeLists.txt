# Integration test for daslang -exe's 3-tier dynamic-module resolution.
# See run_layouts.cmake for the per-scenario logic.

if(NOT TARGET daslang OR NOT TARGET dasModuleSQLITE OR NOT DAS_LLVM_INCLUDED)
    # daslang -exe (the codegen exercised by this integration test) requires
    # LLVM/JIT support; skip when LLVM is disabled.
    return()
endif()

# Locate the dasSQLITE shared module artifact for the active build configuration.
# In multi-config generators (MSVC, Xcode) the path includes $<CONFIG>; in
# single-config (Make/Ninja) it's flat. Use a generator expression to evaluate
# at test-run time.
set(_sqlite_mod_path "$<TARGET_FILE:dasModuleSQLITE>")

add_test(
    NAME exe_paths_module_resolve
    COMMAND ${CMAKE_COMMAND}
        -DDASLANG=$<TARGET_FILE:daslang>
        -DSRC_DAS=${CMAKE_CURRENT_SOURCE_DIR}/_uses_sqlite.das
        -DSQLITE_MOD=${_sqlite_mod_path}
        -DWORKDIR=${CMAKE_CURRENT_BINARY_DIR}/wk
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_layouts.cmake
)
# Label `small` is what CI's `ctest -L small` step picks up (.github/workflows/build.yml).
# `exe-paths` / `integration` are descriptive — useful for local `ctest -L exe-paths`.
set_tests_properties(exe_paths_module_resolve PROPERTIES LABELS "small;exe-paths;integration")
