// Tutorial: Context Variables — Accessing daslang Globals from C
//
// This tutorial covers:
//   1. Reading scalar global variables (int, float, string, bool) from C
//   2. Writing global variables from C
//   3. Calling a daslang function that verifies the C-side writes
//   4. Enumerating all global variables by index
//
// Build: link against libDaScript (C++ linker required).
// Run:   requires 07_context_variables.das on disk.

#include <stdio.h>
#include <string.h>

#include "daScript/daScriptC.h"

int main(int argc, char ** argv) {
    (void)argc; (void)argv;

    das_initialize();

    das_text_writer  * tout   = das_text_make_printer();
    das_module_group * libgrp = das_modulegroup_make();
    das_file_access  * fa     = das_fileaccess_make_default();

    // Build the script path relative to getDasRoot().
    char script[512];
    das_get_root(script, sizeof(script));
    strncat(script, "/tutorials/integration/c/07_context_variables.das",
            sizeof(script) - strlen(script) - 1);

    // Compile.
    das_program * program = das_program_compile(script, fa, tout, libgrp);
    int err_count = das_program_err_count(program);
    if (err_count) {
        printf("Compilation failed with %d error(s):\n", err_count);
        for (int i = 0; i < err_count; i++) {
            char buf[1024];
            das_error_report(das_program_get_error(program, i), buf, sizeof(buf));
            printf("  %s\n", buf);
        }
        goto cleanup;
    }

    {
        // Create context and simulate.
        das_context * ctx = das_context_make(das_program_context_stack_size(program));
        if (!das_program_simulate(program, ctx, tout)) {
            printf("Simulation failed\n");
            das_context_release(ctx);
            goto cleanup;
        }

        // ---------------------------------------------------------------
        // 1. Reading scalar globals by name
        // ---------------------------------------------------------------
        printf("=== Reading globals from C ===\n");

        int idx_score = das_context_find_variable(ctx, "score");
        if (idx_score >= 0) {
            int * p = (int *)das_context_get_variable(ctx, idx_score);
            printf("score = %d\n", *p);
        }

        int idx_speed = das_context_find_variable(ctx, "speed");
        if (idx_speed >= 0) {
            float * p = (float *)das_context_get_variable(ctx, idx_speed);
            printf("speed = %.2f\n", *p);
        }

        int idx_name = das_context_find_variable(ctx, "player_name");
        if (idx_name >= 0) {
            // String globals are stored as char* in the context.
            char ** p = (char **)das_context_get_variable(ctx, idx_name);
            printf("player_name = %s\n", *p);
        }

        int idx_alive = das_context_find_variable(ctx, "alive");
        if (idx_alive >= 0) {
            // Bool is stored as a single byte.
            char * p = (char *)das_context_get_variable(ctx, idx_alive);
            printf("alive = %s\n", *p ? "true" : "false");
        }

        // ---------------------------------------------------------------
        // 2. Writing globals from C
        // ---------------------------------------------------------------
        printf("\n=== Writing globals from C ===\n");

        if (idx_score >= 0) {
            int * p = (int *)das_context_get_variable(ctx, idx_score);
            *p = 9999;
            printf("C set score = 9999\n");
        }

        if (idx_speed >= 0) {
            float * p = (float *)das_context_get_variable(ctx, idx_speed);
            *p = 99.5f;
            printf("C set speed = 99.5\n");
        }

        // ---------------------------------------------------------------
        // 3. Call a daslang function to verify the C-side changes
        // ---------------------------------------------------------------
        printf("\n=== Calling daslang to verify ===\n");
        das_function * fn = das_context_find_function(ctx, "print_globals");
        if (fn) {
            das_context_eval_with_catch(ctx, fn, NULL);
            char * ex = das_context_get_exception(ctx);
            if (ex) {
                printf("Exception: %s\n", ex);
            }
        }

        // ---------------------------------------------------------------
        // 4. Enumerate all global variables
        // ---------------------------------------------------------------
        printf("\n=== All global variables ===\n");
        int total = das_context_get_total_variables(ctx);
        for (int i = 0; i < total; i++) {
            const char * name = das_context_get_variable_name(ctx, i);
            int sz = das_context_get_variable_size(ctx, i);
            if (name) {
                printf("  [%d] %s (size=%d)\n", i, name, sz);
            }
        }

        das_context_release(ctx);
    }

cleanup:
    das_program_release(program);
    das_fileaccess_release(fa);
    das_modulegroup_release(libgrp);
    das_text_release(tout);

    das_shutdown();
    return 0;
}
