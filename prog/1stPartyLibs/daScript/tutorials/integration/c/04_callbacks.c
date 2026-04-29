// Tutorial: Callbacks and Closures
//
// This tutorial shows how to:
//   - Bind C interop functions that receive daslang callable types
//   - Call a daslang function pointer (@@function) from C
//   - Call a daslang lambda from C
//   - Call a daslang block from C
//
// Key concept: mangled name encoding for callable types
//   "0<args>@@" = function pointer (no capture)
//   "0<args>@"  = lambda (captures variables)
//   "0<args>$"  = block (stack-allocated closure)
//
//   Example: "s 0<i;f>@@ i f" means:
//     returns string, takes function<(int,float):string>, int, float
//
// Build: link against libDaScript (C++ linker required).
// Run:   from the project root.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "daScript/daScriptC.h"

#define SCRIPT_NAME "/tutorials/integration/c/04_callbacks.das"

// -----------------------------------------------------------------------
// Interop: c_call_function — receives a function pointer and calls it
//
// daslang signature:
//   def c_call_function(callback : function<(a:int; b:float) : string>;
//                       a : int; b : float) : string
//
// Mangled: "s 0<i;f>@@ i f"
//   return "s" (string)
//   arg 0: "0<i;f>@@" — function pointer taking (int, float) returning string
//   arg 1: "i" (int)
//   arg 2: "f" (float)
// -----------------------------------------------------------------------
vec4f c_call_function_impl(das_context * ctx, das_node * node, vec4f * args) {
    (void)node;
    das_function * callback = das_argument_function(args[0]);
    int    a = das_argument_int(args[1]);
    float  b = das_argument_float(args[2]);

    printf("[C] calling function pointer with (%d, %.2f)\n", a, b);

    // Build arguments for the callback.
    vec4f cb_args[2];
    cb_args[0] = das_result_int(a);
    cb_args[1] = das_result_float(b);

    // Call the daslang function.
    vec4f ret = das_context_eval_with_catch(ctx, callback, cb_args);
    char * ex = das_context_get_exception(ctx);
    if (ex) {
        printf("[C] exception in callback: %s\n", ex);
        return das_result_string(NULL);
    }

    char * str = das_argument_string(ret);
    printf("[C] callback returned: \"%s\"\n", str);
    return ret;
}

// -----------------------------------------------------------------------
// Interop: c_call_lambda — receives a lambda and calls it
//
// daslang signature:
//   def c_call_lambda(callback : lambda<(a:int; b:float) : string>;
//                     a : int; b : float) : string
//
// Mangled: "s 0<i;f>@ i f"
//   "0<i;f>@" — lambda (single @, captures variables)
//
// Important: when calling a lambda, the FIRST argument must be the lambda
// itself (its capture block).  So the args array is 3 elements:
//   [0] = lambda, [1] = int a, [2] = float b
// -----------------------------------------------------------------------
vec4f c_call_lambda_impl(das_context * ctx, das_node * node, vec4f * args) {
    (void)node;
    das_lambda * lambda = das_argument_lambda(args[0]);
    int    a = das_argument_int(args[1]);
    float  b = das_argument_float(args[2]);

    printf("[C] calling lambda with (%d, %.2f)\n", a, b);

    // Lambda arguments: first is the lambda itself, then the actual args.
    vec4f lmb_args[3];
    lmb_args[0] = das_result_lambda(lambda);    // capture context
    lmb_args[1] = das_result_int(a);
    lmb_args[2] = das_result_float(b);

    vec4f ret = das_context_eval_lambda(ctx, lambda, lmb_args);
    char * ex = das_context_get_exception(ctx);
    if (ex) {
        printf("[C] exception in lambda: %s\n", ex);
        return das_result_string(NULL);
    }

    char * str = das_argument_string(ret);
    printf("[C] lambda returned: \"%s\"\n", str);
    return ret;
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// Interop: c_call_block — receives a block and calls it
//
// daslang signature:
//   def c_call_block(callback : block<(a:int; b:float) : string>;
//                    a : int; b : float) : string
//
// Mangled: "s 0<i;f>$ i f"
//   "0<i;f>$" — block (stack-allocated, $ suffix)
//   "i" — int argument
//   "f" — float argument
//
// Unlike lambdas, block arguments do NOT include the block itself as
// the first argument.  Just pass the actual arguments.
// -----------------------------------------------------------------------
vec4f c_call_block_impl(das_context * ctx, das_node * node, vec4f * args) {
    (void)node;
    das_block * block = das_argument_block(args[0]);
    int    a = das_argument_int(args[1]);
    float  b = das_argument_float(args[2]);

    printf("[C] calling block with (%d, %.2f)\n", a, b);

    vec4f blk_args[2];
    blk_args[0] = das_result_int(a);
    blk_args[1] = das_result_float(b);

    vec4f ret = das_context_eval_block(ctx, block, blk_args);
    char * ex = das_context_get_exception(ctx);
    if (ex) {
        printf("[C] exception in block: %s\n", ex);
        return das_result_string(NULL);
    }

    char * str = das_argument_string(ret);
    printf("[C] block returned: \"%s\"\n", str);
    return ret;
}

// -----------------------------------------------------------------------
// Module registration
// -----------------------------------------------------------------------

das_module * register_module_tutorial_c_04(void) {
    das_module * mod = das_module_create("tutorial_c_04");
    das_module_group * lib = das_modulegroup_make();
    das_modulegroup_add_module(lib, mod);

    // c_call_function: takes function<(int,float):string>, int, float — returns string
    das_module_bind_interop_function(mod, lib, &c_call_function_impl,
        "c_call_function", "c_call_function_impl",
        SIDEEFFECTS_modifyExternal, "s 0<i;f>@@ i f");

    // c_call_lambda: takes lambda<(int,float):string>, int, float — returns string
    das_module_bind_interop_function(mod, lib, &c_call_lambda_impl,
        "c_call_lambda", "c_call_lambda_impl",
        SIDEEFFECTS_modifyExternal, "s 0<i;f>@ i f");

    // c_call_block: takes block<(int,float):string>, int, float — returns string
    das_module_bind_interop_function(mod, lib, &c_call_block_impl,
        "c_call_block", "c_call_block_impl",
        SIDEEFFECTS_modifyExternal, "s 0<i;f>$ i f");

    das_modulegroup_release(lib);
    return mod;
}

// -----------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------

int main(int argc, char ** argv) {
    (void)argc; (void)argv;

    das_initialize();
    register_module_tutorial_c_04();

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
