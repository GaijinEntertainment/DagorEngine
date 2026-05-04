// Tutorial 10 — Threading (C integration)
//
// Demonstrates two threading patterns using the daScriptC.h API:
//
// Part A — Run on a worker thread
//   Compile on the main thread, clone the context with
//   das_context_clone(), capture the environment with
//   das_environment_get_bound(), and evaluate on a worker thread.
//
// Part B — Compile on a worker thread
//   Create a fully independent daScript environment on a new thread
//   by calling das_reuse_cache_guard_create() + das_initialize() at
//   thread start and das_shutdown() + das_reuse_cache_guard_release()
//   before exit.
//
// Key C API functions used:
//   das_environment_get_bound / das_environment_set_bound
//   das_reuse_cache_guard_create / das_reuse_cache_guard_release
//   das_context_clone / DAS_CONTEXT_CATEGORY_THREAD_CLONE
//   das_program_compile_policies / DAS_POLICY_THREADLOCK_CONTEXT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "daScript/daScriptC.h"

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <process.h>
#else
    #include <pthread.h>
#endif

#define SCRIPT_NAME "/tutorials/integration/c/10_threading.das"

// --------------------------------------------------------------------------
// Portable thread helpers
// --------------------------------------------------------------------------
#ifdef _WIN32
typedef unsigned (__stdcall *thread_func_t)(void *);
typedef HANDLE thread_handle_t;

static thread_handle_t thread_start(thread_func_t func, void * arg) {
    return (HANDLE)_beginthreadex(NULL, 0, func, arg, 0, NULL);
}
static void thread_join(thread_handle_t h) {
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
}
#else
typedef void *(*thread_func_t)(void *);
typedef pthread_t thread_handle_t;

static thread_handle_t thread_start(thread_func_t func, void * arg) {
    pthread_t t;
    pthread_create(&t, NULL, func, arg);
    return t;
}
static void thread_join(thread_handle_t h) {
    pthread_join(h, NULL);
}
#endif

// =======================================================================
// Part A — Run on a worker thread
// =======================================================================

typedef struct {
    das_context *   ctx;            // cloned context
    das_environment * env;          // environment captured from main thread
    int             result;         // output: compute() return value
} part_a_args;

#ifdef _WIN32
static unsigned __stdcall part_a_worker(void * raw)
#else
static void * part_a_worker(void * raw)
#endif
{
    part_a_args * args = (part_a_args *)raw;

    // Bind the main thread's environment on this worker thread.
    das_environment_set_bound(args->env);

    // Look up the function in the *cloned* context.
    das_function * fn = das_context_find_function(args->ctx, "compute");
    if (fn) {
        vec4f res = das_context_eval_with_catch(args->ctx, fn, NULL);
        args->result = das_argument_int(res);
    }

#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

static void part_a_run_on_thread(void) {
    printf("=== Part A: Run on a worker thread ===\n");

    das_text_writer  * tout  = das_text_make_printer();
    das_module_group * lib   = das_modulegroup_make();
    das_file_access  * fac   = das_fileaccess_make_default();

    // Build script path
    char path[512];
    das_get_root(path, sizeof(path));
    strncat(path, SCRIPT_NAME, sizeof(path) - strlen(path) - 1);

    // Compile with threadlock_context = true
    das_policies * pol = das_policies_make();
    das_policies_set_bool(pol, DAS_POLICY_THREADLOCK_CONTEXT, 1);
    das_program * prog = das_program_compile_policies(path, fac, tout, lib, pol);
    das_policies_release(pol);

    if (das_program_err_count(prog)) {
        printf("  Compilation failed.\n");
        goto cleanup_a;
    }

    {
        int stack = das_program_context_stack_size(prog);
        das_context * ctx = das_context_make(stack);
        if (!das_program_simulate(prog, ctx, tout)) {
            printf("  Simulation failed.\n");
            das_context_release(ctx);
            goto cleanup_a;
        }

        // Clone the context for the worker thread
        das_context * clone = das_context_clone(ctx, DAS_CONTEXT_CATEGORY_THREAD_CLONE);

        // Capture the environment
        das_environment * env = das_environment_get_bound();

        part_a_args args;
        args.ctx    = clone;
        args.env    = env;
        args.result = 0;

        thread_handle_t t = thread_start(part_a_worker, &args);
        thread_join(t);

        printf("  compute() on worker thread returned: %d\n", args.result);
        printf("  %s\n", args.result == 4950 ? "PASS" : "FAIL (expected 4950)");

        das_context_release(clone);
        das_context_release(ctx);
    }

cleanup_a:
    das_program_release(prog);
    das_fileaccess_release(fac);
    das_modulegroup_release(lib);
    das_text_release(tout);
}

// =======================================================================
// Part B — Compile on a worker thread
// =======================================================================

typedef struct {
    int result;
} part_b_args;

#ifdef _WIN32
static unsigned __stdcall part_b_worker(void * raw)
#else
static void * part_b_worker(void * raw)
#endif
{
    part_b_args * args = (part_b_args *)raw;

    // 1. Thread-local free-list cache — must be first.
    das_reuse_cache_guard * guard = das_reuse_cache_guard_create();

    // 2+3. Register module factories + create environment in TLS.
    das_initialize();

    das_text_writer  * tout = das_text_make_printer();
    das_module_group * lib  = das_modulegroup_make();
    das_file_access  * fac  = das_fileaccess_make_default();

    char path[512];
    das_get_root(path, sizeof(path));
    strncat(path, SCRIPT_NAME, sizeof(path) - strlen(path) - 1);

    // 4. Compile with threadlock_context
    das_policies * pol = das_policies_make();
    das_policies_set_bool(pol, DAS_POLICY_THREADLOCK_CONTEXT, 1);
    das_program * prog = das_program_compile_policies(path, fac, tout, lib, pol);
    das_policies_release(pol);

    if (das_program_err_count(prog)) {
        printf("  Worker compilation failed.\n");
    } else {
        int stack = das_program_context_stack_size(prog);
        das_context * ctx = das_context_make(stack);
        if (!das_program_simulate(prog, ctx, tout)) {
            printf("  Worker simulation failed.\n");
        } else {
            das_function * fn = das_context_find_function(ctx, "compute");
            if (fn) {
                vec4f res = das_context_eval_with_catch(ctx, fn, NULL);
                args->result = das_argument_int(res);
            }
        }
        das_context_release(ctx);
    }

    das_program_release(prog);
    das_fileaccess_release(fac);
    das_modulegroup_release(lib);
    das_text_release(tout);

    // 5. Shut down this thread's module system.
    das_shutdown();
    das_reuse_cache_guard_release(guard);

#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

static void part_b_compile_on_thread(void) {
    printf("\n=== Part B: Compile on a worker thread ===\n");

    part_b_args args;
    args.result = 0;

    thread_handle_t t = thread_start(part_b_worker, &args);
    thread_join(t);

    printf("  compute() compiled + run on worker thread returned: %d\n", args.result);
    printf("  %s\n", args.result == 4950 ? "PASS" : "FAIL (expected 4950)");
}

// =======================================================================
// main
// =======================================================================
int main(int argc, char ** argv) {
    (void)argc; (void)argv;

    // Initialize on the main thread — needed for Part A.
    // Part B does its own das_initialize/das_shutdown on the worker.
    das_initialize();

    part_a_run_on_thread();
    part_b_compile_on_thread();

    das_shutdown();
    return 0;
}
