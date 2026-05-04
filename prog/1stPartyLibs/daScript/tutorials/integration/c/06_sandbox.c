// Tutorial: Sandbox — Secure Script Execution
//
// This tutorial covers:
//   1. Filesystem sandboxing — lock file access so only pre-introduced
//      files are available to the compiler.
//   2. Policy enforcement via .das_project — whitelist modules, options,
//      and annotations; forbid unsafe blocks.
//   3. Combining both layers for a complete sandboxed scripting environment.
//   4. CodeOfPolicies via the C API — enforce policies like no_unsafe
//      without a .das_project file.
//
// The two layers are independent and complementary:
//   - Filesystem lock:  controls WHICH FILES exist (physical layer)
//   - .das_project:     controls WHAT IS ALLOWED even among existing files
//                       (policy layer)
//
// Build: link against libDaScript (C++ linker required).
// Run:   requires 06_sandbox.das_project on disk (policy file);
//        all script source is embedded as C string constants.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "daScript/daScriptC.h"

// -----------------------------------------------------------------------
// Inline script sources
//
// All daslang code is embedded as C string constants — the host controls
// exactly what the compiler can see.
// -----------------------------------------------------------------------

// A small utility module that works within the sandbox.
// It only uses basic language features — no unsafe, no banned modules.
static const char * HELPER_MODULE =
    "options gen2\n"
    "module sandbox_helpers shared\n"
    "\n"
    "[export]\n"
    "def clamp_value(val, lo, hi : int) : int {\n"
    "    if (val < lo) {\n"
    "        return lo\n"
    "    }\n"
    "    if (val > hi) {\n"
    "        return hi\n"
    "    }\n"
    "    return val\n"
    "}\n"
    "\n"
    "[export]\n"
    "def greet(name : string) : string {\n"
    "    return \"Hello, {name}!\"\n"
    "}\n"
;

// A valid script that stays within sandbox rules.
// It requires only whitelisted modules and the user helper module.
static const char * VALID_SCRIPT =
    "options gen2\n"
    "require sandbox_helpers\n"
    "require daslib/strings_boost\n"
    "\n"
    "[export]\n"
    "def test() {\n"
    "    // Use the helper module\n"
    "    let clamped = clamp_value(150, 0, 100)\n"
    "    print(\"clamped: {clamped}\\n\")\n"
    "\n"
    "    let msg = greet(\"sandbox\")\n"
    "    print(\"{msg}\\n\")\n"
    "\n"
    "    // Use the strings module (whitelisted native module)\n"
    "    let parts <- split(\"a-b-c\", \"-\")\n"
    "    print(\"split: {parts}\\n\")\n"
    "}\n"
;

// A script that uses a banned option.
static const char * VIOLATES_OPTION =
    "options gen2\n"
    "options persistent_heap\n"       // blocked by option_allowed
    "\n"
    "[export]\n"
    "def test() {\n"
    "    print(\"this should not compile\\n\")\n"
    "}\n"
;

// A script that uses an unsafe block.
static const char * VIOLATES_UNSAFE =
    "options gen2\n"
    "\n"
    "[export]\n"
    "def test() {\n"
    "    unsafe {\n"                  // blocked by module_allowed_unsafe
    "        print(\"this should not compile\\n\")\n"
    "    }\n"
    "}\n"
;

// A script that tries to require a banned module.
static const char * VIOLATES_MODULE =
    "options gen2\n"
    "require daslib/fio\n"                   // blocked by module_allowed (not whitelisted)
    "\n"
    "[export]\n"
    "def test() {\n"
    "    print(\"this should not compile\\n\")\n"
    "}\n"
;

// -----------------------------------------------------------------------
// Part 1: Compile and run a valid script in the sandbox
// -----------------------------------------------------------------------
static void run_valid_script(void) {
    printf("=== Part 1: Valid script in sandbox ===\n\n");

    das_text_writer  * tout         = das_text_make_printer();
    das_module_group * module_group = das_modulegroup_make();

    // Build the path to the .das_project file.
    char project_path[512];
    das_get_root(project_path, sizeof(project_path));
    strncat(project_path, "/tutorials/integration/c/06_sandbox.das_project",
            sizeof(project_path) - strlen(project_path) - 1);

    // Create file access with the .das_project loaded.
    // This compiles the project file and sets up all policy callbacks.
    das_file_access * fa = das_fileaccess_make_project(project_path);

    // Pre-cache all daslib files so the compiler can find them.
    // The policy layer (module_allowed) still controls which modules
    // are actually permitted — having the files cached is necessary
    // but not sufficient.
    das_fileaccess_introduce_daslib(fa);

    // Introduce the user module and script as virtual files.
    das_fileaccess_introduce_file(fa, "sandbox_helpers.das", HELPER_MODULE, 0);
    das_fileaccess_introduce_file(fa, "user_script.das", VALID_SCRIPT, 0);

    // Lock the filesystem — from now on, only pre-introduced files
    // are accessible. Any attempt to load uncached files returns null.
    das_fileaccess_lock(fa);

    // Compile the script.  The compiler resolves `require` paths through
    // the project's module_get callback and checks each module against
    // module_allowed before loading it.
    das_program * program = das_program_compile("user_script.das",
                                                 fa, tout, module_group);

    int err_count = das_program_err_count(program);
    if (err_count) {
        printf("Unexpected compilation errors (%d):\n", err_count);
        for (int i = 0; i < err_count; i++) {
            das_error * error = das_program_get_error(program, i);
            char buf[1024];
            das_error_report(error, buf, sizeof(buf));
            printf("  %s\n", buf);
        }
        goto cleanup;
    }

    {
        das_context * ctx = das_context_make(das_program_context_stack_size(program));
        if (!das_program_simulate(program, ctx, tout)) {
            printf("Simulation failed\n");
            das_context_release(ctx);
            goto cleanup;
        }

        das_function * fn = das_context_find_function(ctx, "test");
        if (fn) {
            das_context_eval_with_catch(ctx, fn, NULL);
            char * ex = das_context_get_exception(ctx);
            if (ex) {
                printf("Exception: %s\n", ex);
            }
        }

        das_context_release(ctx);
    }

cleanup:
    das_program_release(program);
    das_fileaccess_release(fa);
    das_modulegroup_release(module_group);
    das_text_release(tout);
}

// -----------------------------------------------------------------------
// Shared helper: compile a script under sandbox policy, print errors.
// -----------------------------------------------------------------------
static void compile_and_report(const char * label, const char * script_source) {
    printf("\n=== %s ===\n\n", label);

    das_text_writer  * tout         = das_text_make_writer();
    das_module_group * module_group = das_modulegroup_make();

    char project_path[512];
    das_get_root(project_path, sizeof(project_path));
    strncat(project_path, "/tutorials/integration/c/06_sandbox.das_project",
            sizeof(project_path) - strlen(project_path) - 1);

    das_file_access * fa = das_fileaccess_make_project(project_path);
    das_fileaccess_introduce_daslib(fa);
    das_fileaccess_introduce_file(fa, "bad_script.das", script_source, 0);
    das_fileaccess_lock(fa);

    das_program * program = das_program_compile("bad_script.das",
                                                 fa, tout, module_group);

    int err_count = das_program_err_count(program);
    printf("Compilation produced %d error(s):\n", err_count);
    for (int i = 0; i < err_count; i++) {
        das_error * error = das_program_get_error(program, i);
        char buf[1024];
        das_error_report(error, buf, sizeof(buf));
        printf("  error %d: %s\n", i, buf);
    }

    das_program_release(program);
    das_fileaccess_release(fa);
    das_modulegroup_release(module_group);
    das_text_release(tout);
}

// -----------------------------------------------------------------------
// Part 5: CodeOfPolicies — enforce no_unsafe from the C API
//
// Instead of (or in addition to) using a .das_project file, you can
// set compilation policies directly via the C API.
// -----------------------------------------------------------------------
static void compile_with_policies(void) {
    printf("\n=== Part 5: CodeOfPolicies via C API ===\n\n");

    // A simple script that uses an unsafe block.
    static const char * UNSAFE_SCRIPT =
        "options gen2\n"
        "\n"
        "[export]\n"
        "def test() {\n"
        "    unsafe {\n"
        "        print(\"this should not compile\\n\")\n"
        "    }\n"
        "}\n"
    ;

    das_text_writer  * tout   = das_text_make_writer();
    das_module_group * libgrp = das_modulegroup_make();
    das_file_access  * fa     = das_fileaccess_make_default();

    das_fileaccess_introduce_file(fa, "policy_test.das", UNSAFE_SCRIPT, 0);

    // Create policies and set no_unsafe = true.
    das_policies * pol = das_policies_make();
    das_policies_set_bool(pol, DAS_POLICY_NO_UNSAFE, 1);

    // Compile with policies — unsafe blocks will be rejected.
    das_program * program = das_program_compile_policies(
        "policy_test.das", fa, tout, libgrp, pol);

    das_policies_release(pol);

    int err_count = das_program_err_count(program);
    printf("Compilation with DAS_POLICY_NO_UNSAFE produced %d error(s):\n", err_count);
    for (int i = 0; i < err_count; i++) {
        das_error * error = das_program_get_error(program, i);
        char buf[1024];
        das_error_report(error, buf, sizeof(buf));
        printf("  error %d: %s\n", i, buf);
    }

    das_program_release(program);
    das_fileaccess_release(fa);
    das_modulegroup_release(libgrp);
    das_text_release(tout);
}

// -----------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------
int main(int argc, char ** argv) {
    (void)argc; (void)argv;

    das_initialize();

    run_valid_script();
    compile_and_report("Part 2: Banned option", VIOLATES_OPTION);
    compile_and_report("Part 3: Unsafe block forbidden", VIOLATES_UNSAFE);
    compile_and_report("Part 4: Banned module", VIOLATES_MODULE);
    compile_with_policies();

    das_shutdown();
    return 0;
}
