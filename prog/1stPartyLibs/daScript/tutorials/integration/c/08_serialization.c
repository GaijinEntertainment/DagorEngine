// Tutorial: Serialization — Save and Restore Compiled Programs (C API)
//
// This tutorial covers:
//   1. Compiling a daslang program normally
//   2. Serializing the compiled program to an in-memory binary blob
//   3. Releasing the original program
//   4. Deserializing from the blob — no parsing, no type inference
//   5. Simulating and running the deserialized program
//
// This is useful for:
//   - Faster startup (skip compilation on subsequent runs)
//   - Distributing pre-compiled scripts
//   - Caching compiled programs in editors / build pipelines
//
// Build: link against libDaScript (C++ linker required).
// Run:   requires the C++ tutorial 14_serialization.das on disk.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "daScript/daScriptC.h"

int main(int argc, char ** argv) {
    (void)argc; (void)argv;

    das_initialize();

    das_text_writer  * tout   = das_text_make_printer();
    das_module_group * libgrp = das_modulegroup_make();
    das_file_access  * fa     = das_fileaccess_make_default();

    // The serialization tutorial reuses the C++ tutorial 14 script.
    char script[512];
    das_get_root(script, sizeof(script));
    strncat(script, "/tutorials/integration/cpp/14_serialization.das",
            sizeof(script) - strlen(script) - 1);

    // ================================================================
    // Stage 1: Compile from source
    // ================================================================
    printf("=== Stage 1: Compile from source ===\n");

    das_program * program = das_program_compile(script, fa, tout, libgrp);
    int err_count = das_program_err_count(program);
    if (err_count) {
        printf("Compilation failed with %d error(s):\n", err_count);
        for (int i = 0; i < err_count; i++) {
            char buf[1024];
            das_error_report(das_program_get_error(program, i), buf, sizeof(buf));
            printf("  %s\n", buf);
        }
        goto cleanup_early;
    }
    printf("  Compiled successfully.\n");

    // Simulate before serializing — simulation resolves function
    // pointers and initializes the program fully.
    {
        das_context * ctx = das_context_make(das_program_context_stack_size(program));
        if (!das_program_simulate(program, ctx, tout)) {
            printf("Simulation failed.\n");
            das_context_release(ctx);
            goto cleanup_early;
        }
        printf("  Simulated successfully.\n");
        das_context_release(ctx);
    }

    // ================================================================
    // Stage 2: Serialize to an in-memory buffer
    // ================================================================
    printf("\n=== Stage 2: Serialize ===\n");

    {
        const void * blob_data = NULL;
        int64_t blob_size = 0;
        das_serialized_data * blob = das_program_serialize(program, &blob_data, &blob_size);

        printf("  Serialized size: %lld bytes\n", (long long)blob_size);

        // Release the original program — we no longer need it.
        das_program_release(program);
        program = NULL;
        printf("  Original program released.\n");

        // ============================================================
        // Stage 3: Deserialize from the buffer
        // ============================================================
        printf("\n=== Stage 3: Deserialize ===\n");

        program = das_program_deserialize(blob_data, blob_size);
        printf("  Deserialized successfully.\n");

        // Release the serialized blob — the deserialized program is independent.
        das_serialized_data_release(blob);
    }

    // ================================================================
    // Stage 4: Simulate and run (same as a freshly compiled program)
    // ================================================================
    printf("\n=== Stage 4: Simulate and run ===\n");

    {
        das_context * ctx = das_context_make(das_program_context_stack_size(program));
        if (!das_program_simulate(program, ctx, tout)) {
            printf("Simulation of deserialized program failed.\n");
            das_context_release(ctx);
            goto cleanup_early;
        }

        das_function * fn = das_context_find_function(ctx, "test");
        if (!fn) {
            printf("Function 'test' not found.\n");
            das_context_release(ctx);
            goto cleanup_early;
        }

        das_context_eval_with_catch(ctx, fn, NULL);
        {
            char * ex = das_context_get_exception(ctx);
            if (ex) {
                printf("Exception: %s\n", ex);
            }
        }
        das_context_release(ctx);
    }

cleanup_early:
    if (program) das_program_release(program);
    das_fileaccess_release(fa);
    das_modulegroup_release(libgrp);
    das_text_release(tout);

    das_shutdown();
    return 0;
}
