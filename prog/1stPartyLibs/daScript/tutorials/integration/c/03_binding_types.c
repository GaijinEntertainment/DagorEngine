// Tutorial: Binding C Types to daslang
//
// This tutorial shows how to:
//   - Create a custom module
//   - Bind a C enumeration so daslang can use it
//   - Bind a C structure so daslang can access its fields
//   - Bind a type alias
//   - Bind interop functions that operate on custom types
//
// Key concept: type mangling.  Every type in the daslangC API is described
// by a compact mangled string.  Common manglings:
//   "i" = int,  "u" = uint,  "f" = float,  "d" = double,
//   "b" = bool, "s" = string, "v" = void,
//   "1<i>A" = array<int>,  "1<f>?" = float*,
//   "H<Name>" = handled type (C struct bound with das_structure_make)
//
// Build: link against libDaScript (C++ linker required).
// Run:   from the project root.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <math.h>
#include <assert.h>

#include "daScript/daScriptC.h"

#define SCRIPT_NAME "/tutorials/integration/c/03_binding_types.das"

// -----------------------------------------------------------------------
// C types that we will expose to daslang
// -----------------------------------------------------------------------

typedef enum {
    Color_red   = 0,
    Color_green = 1,
    Color_blue  = 2
} Color;

typedef struct {
    float x;
    float y;
} Point2D;

// -----------------------------------------------------------------------
// Interop functions — called from daslang, implemented in C
// -----------------------------------------------------------------------

// def point_distance(p : Point2D) : float
// Mangled signature: "f H<Point2D>"
//   "f" = returns float
//   "H<Point2D>" = takes a handled struct Point2D
vec4f c_point_distance(das_context * ctx, das_node * node, vec4f * args) {
    (void)ctx; (void)node;
    Point2D * p = (Point2D *)das_argument_ptr(args[0]);
    float dist = sqrtf(p->x * p->x + p->y * p->y);
    return das_result_float(dist);
}

// def point_to_string(p : Point2D) : string
// Mangled signature: "s H<Point2D>"
vec4f c_point_to_string(das_context * ctx, das_node * node, vec4f * args) {
    (void)node;
    Point2D * p = (Point2D *)das_argument_ptr(args[0]);
    char buf[128];
    snprintf(buf, sizeof(buf), "(%.2f, %.2f)", p->x, p->y);
    // Allocate the string in the daslang context so it survives the call.
    char * result = das_allocate_string(ctx, buf);
    return das_result_string(result);
}

// -----------------------------------------------------------------------
// Module registration
// -----------------------------------------------------------------------

das_module * register_module_tutorial_c_03(void) {
    // Create the module and a temporary module group for type resolution.
    das_module * mod = das_module_create("tutorial_c_03");
    das_module_group * lib = das_modulegroup_make();
    das_modulegroup_add_module(lib, mod);

    // --- Alias ---
    // "IntArray" is an alias for array<int>.  Mangled: "1<i>A"
    das_module_bind_alias(mod, lib, "IntArray", "1<i>A");

    // --- Enumeration ---
    // The third argument to das_enumeration_make is the underlying int type:
    //   0 = int8, 1 = int (32-bit), 2 = int16.
    das_enumeration * en = das_enumeration_make("Color", "Color", 1);
    das_enumeration_add_value(en, "red",   "Color_red",   Color_red);
    das_enumeration_add_value(en, "green", "Color_green", Color_green);
    das_enumeration_add_value(en, "blue",  "Color_blue",  Color_blue);
    das_module_bind_enumeration(mod, en);

    // --- Structure ---
    // Create a handled structure with explicit size and alignment.
    das_structure * st = das_structure_make(lib, "Point2D", "Point2D",
                                           sizeof(Point2D), _Alignof(Point2D));
    das_structure_add_field(st, mod, lib, "x", "x", offsetof(Point2D, x), "f");
    das_structure_add_field(st, mod, lib, "y", "y", offsetof(Point2D, y), "f");
    das_module_bind_structure(mod, st);

    // --- Functions ---
    das_module_bind_interop_function(mod, lib, &c_point_distance,
        "point_distance", "c_point_distance",
        SIDEEFFECTS_none, "f H<Point2D>");

    das_module_bind_interop_function(mod, lib, &c_point_to_string,
        "point_to_string", "c_point_to_string",
        SIDEEFFECTS_none, "s H<Point2D>");

    das_modulegroup_release(lib);
    return mod;
}

// -----------------------------------------------------------------------
// Main — lifecycle with custom module registration
// -----------------------------------------------------------------------

int main(int argc, char ** argv) {
    (void)argc; (void)argv;

    das_initialize();

    // Register our module BEFORE compiling the script that requires it.
    register_module_tutorial_c_03();

    das_text_writer  * tout         = das_text_make_printer();
    das_module_group * module_group = das_modulegroup_make();
    das_file_access  * file_access  = das_fileaccess_make_default();

    char script_path[512];
    das_get_root(script_path, sizeof(script_path));
    int chars_left = ((int)sizeof(script_path)) - ((int)strlen(script_path)) - 1;
    assert(chars_left >= 0);
    strncat(script_path, SCRIPT_NAME, (size_t)chars_left);

    das_program * program = das_program_compile(script_path, file_access, tout, module_group);
    int err_count = das_program_err_count(program);
    if (err_count) {
        printf("Compilation failed with %d error(s)\n", err_count);
        for (int i = 0; i < err_count; i++)
            das_error_output(das_program_get_error(program, i), tout);
        goto cleanup;
    }

    {
        das_context * ctx = das_context_make(das_program_context_stack_size(program));
        if (!das_program_simulate(program, ctx, tout)) {
            printf("Simulation failed\n");
            das_context_release(ctx);
            goto cleanup;
        }

        das_function * fn_test = das_context_find_function(ctx, "test");
        if (!fn_test) {
            printf("Function 'test' not found\n");
            das_context_release(ctx);
            goto cleanup;
        }

        das_context_eval_with_catch(ctx, fn_test, NULL);
        {
            char * ex = das_context_get_exception(ctx);
            if (ex) printf("Exception: %s\n", ex);
        }

        das_context_release(ctx);
    }

cleanup:
    das_program_release(program);
    das_fileaccess_release(file_access);
    das_modulegroup_release(module_group);
    das_text_release(tout);

    das_shutdown();
    return 0;
}
