// Tutorial: Calling daslang Functions from C
//
// This tutorial shows how to:
//   - Pass arguments of various types (int, float, string, bool) to daslang
//   - Read return values (int, float, string)
//   - Call functions that return complex results (structures)
//
// Build: link against libDaScript (C++ linker required).
// Run:   from the project root so that the .das file path resolves correctly.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "daScript/daScriptC.h"

#define SCRIPT_NAME "/tutorials/integration/c/02_calling_functions.das"

// Structure layout must exactly match the daslang struct Vec2.
typedef struct {
    float x;
    float y;
} Vec2;

// Helper: compile and simulate a script, returning the context.
// Returns NULL on failure (errors are printed to tout).
static das_context * compile_and_simulate(
    const char * script_path,
    das_file_access * file_access,
    das_text_writer * tout,
    das_module_group * module_group,
    das_program ** out_program
) {
    *out_program = das_program_compile((char*)script_path, file_access, tout, module_group);
    int err_count = das_program_err_count(*out_program);
    if (err_count) {
        printf("Compilation failed with %d error(s)\n", err_count);
        for (int i = 0; i < err_count; i++)
            das_error_output(das_program_get_error(*out_program, i), tout);
        return NULL;
    }
    das_context * ctx = das_context_make(das_program_context_stack_size(*out_program));
    if (!das_program_simulate(*out_program, ctx, tout)) {
        printf("Simulation failed\n");
        das_context_release(ctx);
        return NULL;
    }
    return ctx;
}

int main(int argc, char ** argv) {
    (void)argc; (void)argv;

    das_initialize();

    das_text_writer  * tout         = das_text_make_printer();
    das_module_group * module_group = das_modulegroup_make();
    das_file_access  * file_access  = das_fileaccess_make_default();

    char script_path[512];
    das_get_root(script_path, sizeof(script_path));
    int chars_left = ((int)sizeof(script_path)) - ((int)strlen(script_path)) - 1;
    assert(chars_left >= 0);
    strncat(script_path, SCRIPT_NAME, (size_t)chars_left);

    das_program * program = NULL;
    das_context * ctx = compile_and_simulate(script_path, file_access, tout, module_group, &program);
    if (!ctx) goto cleanup;

    // -----------------------------------------------------------------------
    // 1. Calling a function with int arguments and int return value
    //
    // To pass arguments, fill a vec4f array using das_result_* helpers.
    // Each argument occupies one vec4f slot.
    // The return value is also a vec4f — extract it with das_argument_*.
    // -----------------------------------------------------------------------
    {
        das_function * fn_add = das_context_find_function(ctx, "add");
        if (!fn_add) { printf("'add' not found\n"); goto done; }

        vec4f args[2];
        args[0] = das_result_int(17);       // first argument: int a = 17
        args[1] = das_result_int(25);       // second argument: int b = 25

        vec4f ret = das_context_eval_with_catch(ctx, fn_add, args);
        if (das_context_get_exception(ctx)) { printf("exception in add\n"); goto done; }

        int result = das_argument_int(ret);
        printf("add(17, 25) = %d\n", result);
    }

    // -----------------------------------------------------------------------
    // 2. Calling a function with float argument and float return value
    // -----------------------------------------------------------------------
    {
        das_function * fn_square = das_context_find_function(ctx, "square");
        if (!fn_square) { printf("'square' not found\n"); goto done; }

        vec4f args[1];
        args[0] = das_result_float(3.5f);   // float x = 3.5

        vec4f ret = das_context_eval_with_catch(ctx, fn_square, args);
        if (das_context_get_exception(ctx)) { printf("exception in square\n"); goto done; }

        float result = das_argument_float(ret);
        printf("square(3.5) = %.2f\n", result);
    }

    // -----------------------------------------------------------------------
    // 3. Calling a function with string argument and string return value
    //
    // Strings returned from daslang live in the context's heap.
    // They remain valid until the context is released or GC'd.
    // -----------------------------------------------------------------------
    {
        das_function * fn_greet = das_context_find_function(ctx, "greet");
        if (!fn_greet) { printf("'greet' not found\n"); goto done; }

        vec4f args[1];
        args[0] = das_result_string("World");

        vec4f ret = das_context_eval_with_catch(ctx, fn_greet, args);
        if (das_context_get_exception(ctx)) { printf("exception in greet\n"); goto done; }

        char * result = das_argument_string(ret);
        printf("greet(\"World\") = \"%s\"\n", result);
    }

    // -----------------------------------------------------------------------
    // 4. Calling a function with multiple argument types
    //
    // bool is passed as int (0 = false, non-zero = true).
    // -----------------------------------------------------------------------
    {
        das_function * fn_show = das_context_find_function(ctx, "show_types");
        if (!fn_show) { printf("'show_types' not found\n"); goto done; }

        vec4f args[4];
        args[0] = das_result_int(42);
        args[1] = das_result_float(3.14f);
        args[2] = das_result_string("test");
        args[3] = das_result_bool(1);       // true

        das_context_eval_with_catch(ctx, fn_show, args);
        if (das_context_get_exception(ctx)) { printf("exception in show_types\n"); goto done; }
    }

    // -----------------------------------------------------------------------
    // 5. Calling a function that returns a struct (complex result / cmres)
    //
    // When a function returns a struct, use das_context_eval_with_catch_cmres.
    // You pass a pointer to a C struct with the same layout.
    // -----------------------------------------------------------------------
    {
        das_function * fn_make_vec2 = das_context_find_function(ctx, "make_vec2");
        if (!fn_make_vec2) { printf("'make_vec2' not found\n"); goto done; }

        vec4f args[2];
        args[0] = das_result_float(1.5f);
        args[1] = das_result_float(2.5f);

        Vec2 v;
        das_context_eval_with_catch_cmres(ctx, fn_make_vec2, args, &v);
        if (das_context_get_exception(ctx)) { printf("exception in make_vec2\n"); goto done; }

        printf("make_vec2(1.5, 2.5) = { x=%.1f, y=%.1f }\n", v.x, v.y);
    }

done:
    das_context_release(ctx);

cleanup:
    das_program_release(program);
    das_fileaccess_release(file_access);
    das_modulegroup_release(module_group);
    das_text_release(tout);

    das_shutdown();
    return 0;
}
