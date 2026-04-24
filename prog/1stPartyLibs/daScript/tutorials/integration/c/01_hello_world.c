// Tutorial: Hello World from C
//
// This is the simplest possible C program that embeds and runs a daslang file.
// It demonstrates the full lifecycle:
//   1. Initialize daslang
//   2. Compile a script from a file
//   3. Simulate (prepare for execution)
//   4. Find and call a function
//   5. Clean up and shut down
//
// Build: link against libDaScript (C++ linker required).
// Run:   from the project root so that the .das file path resolves correctly.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "daScript/daScriptC.h"

#define SCRIPT_NAME "/tutorials/integration/c/01_hello_world.das"

int main(int argc, char ** argv) {
    (void)argc; (void)argv;

    // -----------------------------------------------------------------------
    // Step 1: Initialize daslang
    // This must be called once before any other daslang API call.
    // It registers all built-in modules (math, strings, etc.).
    // -----------------------------------------------------------------------
    das_initialize();

    // -----------------------------------------------------------------------
    // Step 2: Create the supporting objects
    //
    // - Text writer:   captures compiler/runtime messages (sends to stdout)
    // - Module group:  holds modules used during compilation
    // - File access:   provides the compiler with file I/O (default = disk)
    // -----------------------------------------------------------------------
    das_text_writer * tout         = das_text_make_printer();
    das_module_group * module_group = das_modulegroup_make();
    das_file_access * file_access  = das_fileaccess_make_default();

    // Build the full path to the script relative to the daslang root.
    char script_path[512];
    das_get_root(script_path, sizeof(script_path));
    int chars_left = ((int)sizeof(script_path)) - ((int)strlen(script_path)) - 1;
    assert(chars_left >= 0 && "script_path buffer is too small");
    strncat(script_path, SCRIPT_NAME, (size_t)chars_left);

    // -----------------------------------------------------------------------
    // Step 3: Compile the script
    //
    // das_program_compile reads the .das file, parses it, type-checks it,
    // and produces a program object.  Any errors are written to `tout`.
    // -----------------------------------------------------------------------
    das_program * program = das_program_compile(script_path, file_access, tout, module_group);

    int err_count = das_program_err_count(program);
    if (err_count) {
        printf("Compilation failed with %d error(s):\n", err_count);
        for (int i = 0; i < err_count; i++) {
            das_error * error = das_program_get_error(program, i);
            das_error_output(error, tout);
        }
        goto cleanup;
    }

    // -----------------------------------------------------------------------
    // Step 4: Create a context and simulate the program
    //
    // A context is the runtime environment: it owns the stack, global
    // variables, and heap.  Simulation resolves all functions, initializes
    // globals, and prepares the program for execution.
    // -----------------------------------------------------------------------
    das_context * ctx = das_context_make(das_program_context_stack_size(program));

    if (!das_program_simulate(program, ctx, tout)) {
        printf("Simulation failed:\n");
        err_count = das_program_err_count(program);
        for (int i = 0; i < err_count; i++) {
            das_error * error = das_program_get_error(program, i);
            das_error_output(error, tout);
        }
        das_context_release(ctx);
        goto cleanup;
    }

    // -----------------------------------------------------------------------
    // Step 5: Find the "test" function
    //
    // Any function marked [export] in the script can be looked up by name.
    // The returned handle is valid until the context is released.
    // -----------------------------------------------------------------------
    das_function * fn_test = das_context_find_function(ctx, "test");
    if (!fn_test) {
        printf("Function 'test' not found in the script.\n");
        das_context_release(ctx);
        goto cleanup;
    }

    // -----------------------------------------------------------------------
    // Step 6: Call the function
    //
    // das_context_eval_with_catch runs the function inside a try/catch so
    // that daslang exceptions don't crash the host.  The second argument
    // is an array of vec4f arguments — NULL because "test" takes none.
    // -----------------------------------------------------------------------
    das_context_eval_with_catch(ctx, fn_test, NULL);

    // Check whether the script threw an exception.
    {
        char * exception = das_context_get_exception(ctx);
        if (exception != NULL) {
            printf("Script exception: %s\n", exception);
        }
    }

    // -----------------------------------------------------------------------
    // Step 7: Clean up
    //
    // Release everything in reverse order of creation.
    // -----------------------------------------------------------------------
    das_context_release(ctx);

cleanup:
    das_program_release(program);
    das_fileaccess_release(file_access);
    das_modulegroup_release(module_group);
    das_text_release(tout);

    // -----------------------------------------------------------------------
    // Step 8: Shut down daslang
    //
    // Frees all global state.  No daslang API calls are allowed after this.
    // -----------------------------------------------------------------------
    das_shutdown();

    return 0;
}
