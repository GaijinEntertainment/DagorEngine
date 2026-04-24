// Tutorial: AOT — Ahead-of-Time Compilation (C API)
//
// This tutorial covers:
//   1. Compiling a daslang program in interpreter mode (no AOT)
//   2. Compiling the same program with AOT enabled via das_policies
//   3. Checking whether functions are AOT-linked with das_function_is_aot
//
// AOT workflow:
//   1. daslang.exe generates C++ source from the .das script
//   2. The generated .cpp is compiled into this executable (handled by CMake)
//   3. At runtime, policies.aot = true causes simulate() to link
//      interpreter simulation nodes to the generated native C++ functions
//
// Build: link against libDaScript (C++ linker required).
//        The AOT-generated .cpp is compiled into this target by CMake.
// Run:   requires the C++ tutorial 13_aot.das on disk.

#include <stdio.h>
#include <string.h>

#include "daScript/daScriptC.h"

int main(int argc, char ** argv) {
    (void)argc; (void)argv;

    das_initialize();

    das_text_writer  * tout   = das_text_make_printer();
    das_module_group * libgrp = das_modulegroup_make();
    das_file_access  * fa     = das_fileaccess_make_default();

    // The AOT tutorial reuses the C++ tutorial 13 script.
    char script[512];
    das_get_root(script, sizeof(script));
    strncat(script, "/tutorials/integration/cpp/13_aot.das",
            sizeof(script) - strlen(script) - 1);

    // ================================================================
    // Step 1: Compile WITHOUT AOT (interpreter mode)
    // ================================================================
    printf("=== Interpreter mode ===\n");
    {
        das_program * program = das_program_compile(script, fa, tout, libgrp);
        int err_count = das_program_err_count(program);
        if (err_count) {
            printf("Compilation failed with %d error(s)\n", err_count);
            das_program_release(program);
            goto done;
        }

        das_context * ctx = das_context_make(das_program_context_stack_size(program));
        if (!das_program_simulate(program, ctx, tout)) {
            printf("Simulation failed\n");
            das_context_release(ctx);
            das_program_release(program);
            goto done;
        }

        das_function * fn = das_context_find_function(ctx, "test");
        if (fn) {
            printf("  test() is AOT: %s\n", das_function_is_aot(fn) ? "yes" : "no");
            das_context_eval_with_catch(ctx, fn, NULL);
            {
                char * ex = das_context_get_exception(ctx);
                if (ex) printf("Exception: %s\n", ex);
            }
        }

        das_context_release(ctx);
        das_program_release(program);
    }

    // ================================================================
    // Step 2: Compile WITH AOT policies enabled
    // ================================================================
    printf("\n=== AOT mode (policies.aot = true) ===\n");
    {
        // Create policies and enable AOT.
        das_policies * pol = das_policies_make();
        das_policies_set_bool(pol, DAS_POLICY_AOT, 1);
        das_policies_set_bool(pol, DAS_POLICY_FAIL_ON_NO_AOT, 1);

        das_program * program = das_program_compile_policies(script, fa, tout, libgrp, pol);
        das_policies_release(pol);

        int err_count = das_program_err_count(program);
        if (err_count) {
            printf("Compilation failed with %d error(s)\n", err_count);
            das_program_release(program);
            goto done;
        }

        das_context * ctx = das_context_make(das_program_context_stack_size(program));
        if (!das_program_simulate(program, ctx, tout)) {
            printf("Simulation failed\n");
            das_context_release(ctx);
            das_program_release(program);
            goto done;
        }

        das_function * fn = das_context_find_function(ctx, "test");
        if (fn) {
            printf("  test() is AOT: %s\n", das_function_is_aot(fn) ? "yes" : "no");
            das_context_eval_with_catch(ctx, fn, NULL);
            {
                char * ex = das_context_get_exception(ctx);
                if (ex) printf("Exception: %s\n", ex);
            }
        }

        das_context_release(ctx);
        das_program_release(program);
    }

    // ================================================================
    // Summary
    // ================================================================
    printf("\n=== AOT workflow summary ===\n");
    printf("1. daslang.exe -aot script.das script.das.cpp\n");
    printf("   -> generates C++ source from daslang functions\n");
    printf("2. Compile the .cpp into your executable\n");
    printf("   -> functions self-register via static AotListBase\n");
    printf("3. Create policies with DAS_POLICY_AOT enabled\n");
    printf("   -> simulate() links AOT functions automatically\n");
    printf("4. Functions run as native C++ — no interpreter overhead\n");

done:
    das_fileaccess_release(fa);
    das_modulegroup_release(libgrp);
    das_text_release(tout);

    das_shutdown();
    return 0;
}
