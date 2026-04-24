// Tutorial: Unaligned ABI & Advanced Topics
//
// This tutorial covers:
//   1. The unaligned calling convention (vec4f_unaligned) — portable, no
//      SSE alignment requirements, works on all platforms.
//   2. Inline script source — compile daslang from a C string instead of
//      a file on disk (das_fileaccess_introduce_file).
//   3. Detailed error reporting — iterating compilation errors.
//
// Build: link against libDaScript (C++ linker required).
// Run:   this tutorial does NOT need a .das file on disk — the script
//        source is embedded in the C code below.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "daScript/daScriptC.h"

// -----------------------------------------------------------------------
// Inline script source
//
// Instead of loading a .das file from disk, we embed the script as a
// C string constant and use das_fileaccess_introduce_file to register it.
// -----------------------------------------------------------------------
static const char * INLINE_SCRIPT =
    "options gen2\n"
    "require tutorial_c_05\n"
    "\n"
    "[export]\n"
    "def test() {\n"
    "    print(\"Hello from inline script!\\n\")\n"
    "    c_greet(42)\n"
    "}\n"
    "\n"
    "[export]\n"
    "def add(a : int; b : int) : int {\n"
    "    return a + b\n"
    "}\n"
;

// Deliberately broken script for demonstrating error reporting.
static const char * BAD_SCRIPT =
    "options gen2\n"
    "[export]\n"
    "def test() {\n"
    "    let x : int = \"not an int\"\n"  // type error
    "}\n"
;

// -----------------------------------------------------------------------
// Unaligned interop function
//
// Signature: void (das_context*, das_node*, vec4f_unaligned*, vec4f_unaligned*)
// Unlike the aligned variant, arguments and result use vec4f_unaligned.
//
// daslang signature: def c_greet(n : int) : void
// Mangled: "v i"
// -----------------------------------------------------------------------
void c_greet_unaligned(das_context * ctx, das_node * node,
                       vec4f_unaligned * args, vec4f_unaligned * result) {
    (void)ctx; (void)node;
    int n = das_argument_int_unaligned(args + 0);
    printf("[C unaligned] greet called with n=%d\n", n);
    das_result_void_unaligned(result);
}

// -----------------------------------------------------------------------
// Module registration — using unaligned binding
// -----------------------------------------------------------------------
das_module * register_module_tutorial_c_05(void) {
    das_module * mod = das_module_create("tutorial_c_05");
    das_module_group * lib = das_modulegroup_make();
    das_modulegroup_add_module(lib, mod);

    // Bind with the _unaligned variant.
    das_module_bind_interop_function_unaligned(mod, lib, &c_greet_unaligned,
        "c_greet", "c_greet_unaligned",
        SIDEEFFECTS_modifyExternal, "v i");

    das_modulegroup_release(lib);
    return mod;
}

// -----------------------------------------------------------------------
// Part 1: Compile and run an inline script using the unaligned ABI
// -----------------------------------------------------------------------
static void run_inline_script(void) {
    printf("=== Part 1: Inline script with unaligned ABI ===\n\n");

    das_text_writer  * tout         = das_text_make_printer();
    das_module_group * module_group = das_modulegroup_make();
    das_file_access  * file_access  = das_fileaccess_make_default();

    // Register the inline script as a virtual file.
    das_fileaccess_introduce_file(file_access, "inline_script.das", INLINE_SCRIPT, 0);

    // Compile from the virtual file name — no disk access needed.
    das_program * program = das_program_compile("inline_script.das",
                                                 file_access, tout, module_group);
    int err_count = das_program_err_count(program);
    if (err_count) {
        printf("Compilation failed\n");
        goto cleanup1;
    }

    {
        das_context * ctx = das_context_make(das_program_context_stack_size(program));
        if (!das_program_simulate(program, ctx, tout)) {
            printf("Simulation failed\n");
            das_context_release(ctx);
            goto cleanup1;
        }

        // Call "test" using the unaligned eval function.
        das_function * fn_test = das_context_find_function(ctx, "test");
        if (fn_test) {
            vec4f_unaligned result;
            das_context_eval_with_catch_unaligned(ctx, fn_test, NULL, 0, &result);
            char * ex = das_context_get_exception(ctx);
            if (ex) printf("Exception: %s\n", ex);
        }

        // Call "add(17, 25)" using unaligned arguments and result.
        das_function * fn_add = das_context_find_function(ctx, "add");
        if (fn_add) {
            vec4f_unaligned args[2];
            das_result_int_unaligned(args + 0, 17);
            das_result_float_unaligned(args + 1, 25);

            // For int args we use das_result_int_unaligned even for the second arg.
            das_result_int_unaligned(args + 1, 25);

            vec4f_unaligned result;
            das_context_eval_with_catch_unaligned(ctx, fn_add, args, 2, &result);
            char * ex = das_context_get_exception(ctx);
            if (ex) {
                printf("Exception: %s\n", ex);
            } else {
                int sum = das_argument_int_unaligned(&result);
                printf("add(17, 25) = %d\n", sum);
            }
        }

        das_context_release(ctx);
    }

cleanup1:
    das_program_release(program);
    das_fileaccess_release(file_access);
    das_modulegroup_release(module_group);
    das_text_release(tout);
}

// -----------------------------------------------------------------------
// Part 2: Demonstrate detailed error reporting
// -----------------------------------------------------------------------
static void demonstrate_error_reporting(void) {
    printf("\n=== Part 2: Error reporting ===\n\n");

    das_text_writer  * tout         = das_text_make_writer();   // string writer, not stdout
    das_module_group * module_group = das_modulegroup_make();
    das_file_access  * file_access  = das_fileaccess_make_default();

    das_fileaccess_introduce_file(file_access, "bad_script.das", BAD_SCRIPT, 0);
    das_program * program = das_program_compile("bad_script.das",
                                                 file_access, tout, module_group);

    int err_count = das_program_err_count(program);
    printf("Compilation produced %d error(s):\n", err_count);

    // Iterate errors and extract them into a C buffer.
    for (int i = 0; i < err_count; i++) {
        das_error * error = das_program_get_error(program, i);
        char buf[1024];
        das_error_report(error, buf, sizeof(buf));
        printf("  error %d: %s\n", i, buf);
    }

    das_program_release(program);
    das_fileaccess_release(file_access);
    das_modulegroup_release(module_group);
    das_text_release(tout);
}

// -----------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------
int main(int argc, char ** argv) {
    (void)argc; (void)argv;

    das_initialize();
    register_module_tutorial_c_05();

    run_inline_script();
    demonstrate_error_reporting();

    das_shutdown();
    return 0;
}
