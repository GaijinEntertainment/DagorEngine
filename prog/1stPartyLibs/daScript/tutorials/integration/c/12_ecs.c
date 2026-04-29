// Tutorial: Mock ECS — C Host with daScript Macros
//
// This tutorial demonstrates a pattern for integrating daslang with a
// native Entity Component System (ECS).  The C side owns component data
// as flat arrays (SOA layout).  daslang scripts define "systems" — small
// update functions annotated with [es] — and a daslang macro module
// handles all the registration plumbing.
//
// The C host:
//   1. Registers a module exposing ecs_register / ecs_register_global
//   2. Compiles the user script (macro adds arguments + [init] registration)
//   3. Simulates — [init] functions call ecs_register / ecs_register_global
//   4. Runs a game loop: sets globals, calls ES functions per entity
//
// Key concepts:
//   - C owns data, daslang owns logic
//   - [es] macro injects struct fields as function arguments
//   - @required globals are registered via macro-generated [init]
//   - C matches argument names to component arrays at runtime
//   - TypeInfo passed from daslang lets C introspect types
//
// Build: link against libDaScript (C++ linker required).
// Run:   from the project root.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "daScript/daScriptC.h"

#define SCRIPT_NAME "/tutorials/integration/c/12_ecs.das"

// -----------------------------------------------------------------------
// Mock ECS — hardcoded component arrays (SOA layout)
// -----------------------------------------------------------------------

#define NUM_ENTITIES 4

// Component arrays.  Field names match the daslang struct field names.
// float3 in daslang is 12 bytes (3 floats).
static float comp_position[NUM_ENTITIES][3] = {
    { 0.0f, 10.0f,  0.0f},
    { 5.0f, 20.0f,  0.0f},
    {-3.0f,  5.0f,  2.0f},
    { 0.0f,  0.0f,  0.0f}
};

static float comp_velocity[NUM_ENTITIES][3] = {
    { 1.0f,  0.0f,  0.0f},
    { 0.0f,  2.0f, -1.0f},
    { 0.0f,  0.0f,  0.0f},
    { 3.0f,  1.0f,  0.0f}
};

// -----------------------------------------------------------------------
// Component registry — maps names to data arrays
// -----------------------------------------------------------------------

typedef struct {
    const char * name;
    void *       data;
    int          elem_size;   // bytes per element (12 for float3)
} ComponentArray;

#define MAX_COMPONENTS 16
static ComponentArray g_components[MAX_COMPONENTS];
static int g_component_count = 0;

static void register_component(const char * name, void * data, int elem_size) {
    assert(g_component_count < MAX_COMPONENTS);
    g_components[g_component_count].name      = name;
    g_components[g_component_count].data      = data;
    g_components[g_component_count].elem_size = elem_size;
    g_component_count++;
}

static ComponentArray * find_component(const char * name) {
    for (int i = 0; i < g_component_count; i++) {
        if (strcmp(g_components[i].name, name) == 0)
            return &g_components[i];
    }
    return NULL;
}

// -----------------------------------------------------------------------
// ES function registry — populated by daslang [init] via ecs_register
// -----------------------------------------------------------------------

typedef struct {
    char           name[64];
    das_function * fn;        // resolved after simulation
    das_type_info* ti;        // TypeInfo for the component struct
} EsEntry;

#define MAX_ES 16
static EsEntry g_es[MAX_ES];
static int g_es_count = 0;

// -----------------------------------------------------------------------
// Global variable registry — populated by daslang [init]
// -----------------------------------------------------------------------

typedef struct {
    char           name[64];
    void *         ptr;       // pointer into daslang context memory
    das_type_info* ti;        // TypeInfo for the variable type
} GlobalEntry;

#define MAX_GLOBALS 16
static GlobalEntry g_globals[MAX_GLOBALS];
static int g_global_count = 0;

// -----------------------------------------------------------------------
// Interop: ecs_register(fn : void?; name : string; ti : TypeInfo const)
//
// Called from daslang [init] to register an ES function.
// The function pointer is passed directly — no name-based resolution needed.
// TypeInfo describes the component struct so C can introspect fields.
//
// Mangled: "v 1<v>? s CH<rtti_core::TypeInfo>"
// -----------------------------------------------------------------------
vec4f c_ecs_register(das_context * ctx, das_node * node, vec4f * args) {
    (void)ctx; (void)node;
    das_function * fn  = (das_function *)das_argument_ptr(args[0]);
    char * name        = das_argument_string(args[1]);
    das_type_info * ti = (das_type_info *)das_argument_ptr(args[2]);

    assert(g_es_count < MAX_ES);
    strncpy(g_es[g_es_count].name, name, sizeof(g_es[0].name) - 1);
    g_es[g_es_count].name[sizeof(g_es[0].name) - 1] = '\0';
    g_es[g_es_count].fn = fn;
    g_es[g_es_count].ti = ti;
    g_es_count++;

    printf("[C] ecs_register: '%s'", name);
    das_struct_info * si = ti ? das_type_info_get_struct(ti) : NULL;
    if (si) {
        int nfields = das_struct_info_get_field_count(si);
        printf(" (struct '%s', %d fields:", das_struct_info_get_name(si), nfields);
        for (int i = 0; i < nfields; i++) {
            das_type_info * ft = das_struct_info_get_field_type(si, i);
            printf(" %s:%s", das_struct_info_get_field_name(si, i),
                   das_type_info_get_description(ft));
        }
        printf(")");
    }
    printf("\n");

    return das_result_void();
}

// -----------------------------------------------------------------------
// Interop: ecs_register_global(name : string; ptr : void?; ti : TypeInfo const)
//
// Called from daslang [init] to register a @required global variable.
// C host writes into the pointer before each tick.
//
// Mangled: "v s 1<v>? CH<rtti_core::TypeInfo>"
// -----------------------------------------------------------------------
vec4f c_ecs_register_global(das_context * ctx, das_node * node, vec4f * args) {
    (void)ctx; (void)node;
    char * name        = das_argument_string(args[0]);
    void * ptr         = das_argument_ptr(args[1]);
    das_type_info * ti = (das_type_info *)das_argument_ptr(args[2]);

    assert(g_global_count < MAX_GLOBALS);
    strncpy(g_globals[g_global_count].name, name, sizeof(g_globals[0].name) - 1);
    g_globals[g_global_count].name[sizeof(g_globals[0].name) - 1] = '\0';
    g_globals[g_global_count].ptr = ptr;
    g_globals[g_global_count].ti  = ti;
    g_global_count++;

    if (ti) {
        printf("[C] ecs_register_global: '%s' (%s, %d bytes)\n",
               name, das_type_info_get_description(ti), das_type_info_get_size(ti));
    } else {
        printf("[C] ecs_register_global: '%s'\n", name);
    }

    return das_result_void();
}

// -----------------------------------------------------------------------
// Module registration
// -----------------------------------------------------------------------

das_module * register_module_tutorial_c_12(void) {
    das_module * mod = das_module_create("tutorial_c_12");
    das_module_group * lib = das_modulegroup_make();
    das_modulegroup_add_module(lib, mod);
    // Add rtti_core so the mangled name parser can resolve CH<rtti_core::TypeInfo>
    das_modulegroup_add_module(lib, das_module_find("rtti_core"));

    das_module_bind_interop_function(mod, lib, &c_ecs_register,
        "ecs_register", "c_ecs_register",
        SIDEEFFECTS_modifyExternal, "v 1<v>? s CH<rtti_core::TypeInfo>");

    das_module_bind_interop_function(mod, lib, &c_ecs_register_global,
        "ecs_register_global", "c_ecs_register_global",
        SIDEEFFECTS_modifyExternal, "v s 1<v>? CH<rtti_core::TypeInfo>");

    das_modulegroup_release(lib);
    return mod;
}

// -----------------------------------------------------------------------
// ECS update — the game loop tick
// -----------------------------------------------------------------------

static void set_global_float(const char * name, float value) {
    for (int i = 0; i < g_global_count; i++) {
        if (strcmp(g_globals[i].name, name) == 0) {
            if (g_globals[i].ptr) {
                *(float *)g_globals[i].ptr = value;
            }
            return;
        }
    }
}

static void ecs_update(das_context * ctx) {
    for (int es = 0; es < g_es_count; es++) {
        das_function * fn = g_es[es].fn;
        if (!fn) continue;

        das_func_info * fi = das_function_get_info(fn);
        if (!fi) continue;
        int argc = das_func_info_get_arg_count(fi);

        // Resolve component arrays for each argument once.
        // In a real engine this would be done at registration time, not every update.
        ComponentArray * arg_comp[16];
        int valid = 1;
        for (int a = 0; a < argc; a++) {
            const char * arg_name = das_func_info_get_arg_name(fi, a);
            arg_comp[a] = find_component(arg_name);
            if (!arg_comp[a]) {
                printf("[C] ERROR: ES '%s' requires unknown component '%s'\n",
                       g_es[es].name, arg_name);
                valid = 0;
            }
        }
        if (!valid) continue;

        // Call the ES function for each entity
        for (int ent = 0; ent < NUM_ENTITIES; ent++) {
            vec4f args[16];
            for (int a = 0; a < argc; a++) {
                char * base = (char *)arg_comp[a]->data + ent * arg_comp[a]->elem_size;
                args[a] = das_result_ptr(base);
            }
            das_context_eval_with_catch(ctx, fn, args);
            char * ex = das_context_get_exception(ctx);
            if (ex) {
                printf("[C] Exception in '%s': %s\n", g_es[es].name, ex);
                return;
            }
        }
    }
}

static void print_entities(void) {
    for (int i = 0; i < NUM_ENTITIES; i++) {
        printf("  [%d] pos=(%.2f, %.2f, %.2f) vel=(%.2f, %.2f, %.2f)\n", i,
               comp_position[i][0], comp_position[i][1], comp_position[i][2],
               comp_velocity[i][0], comp_velocity[i][1], comp_velocity[i][2]);
    }
}

// -----------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------

int main(int argc, char ** argv) {
    (void)argc; (void)argv;

    das_initialize();

    // Register component arrays — names match daslang struct field names
    register_component("position", comp_position, 3 * sizeof(float));
    register_component("velocity", comp_velocity, 3 * sizeof(float));

    // Register our C module
    register_module_tutorial_c_12();

    das_text_writer  * tout       = das_text_make_printer();
    das_module_group * libgroup   = das_modulegroup_make();
    das_file_access  * file_access = das_fileaccess_make_default();

    char script_path[512];
    das_get_root(script_path, sizeof(script_path));
    strncat(script_path, SCRIPT_NAME,
            sizeof(script_path) - strlen(script_path) - 1);

    // Compile the user script.
    // The [es] macro fires during compilation, adding arguments to ES functions.
    das_program * program = das_program_compile(script_path, file_access, tout, libgroup);
    int err_count = das_program_err_count(program);
    if (err_count) {
        printf("Compilation failed with %d error(s)\n", err_count);
        for (int i = 0; i < err_count; i++)
            das_error_output(das_program_get_error(program, i), tout);
        goto cleanup;
    }

    {
        das_context * ctx = das_context_make(das_program_context_stack_size(program));

        // Simulate — runs [init] which calls ecs_register / ecs_register_global
        if (!das_program_simulate(program, ctx, tout)) {
            printf("Simulation failed\n");
            das_context_release(ctx);
            goto cleanup;
        }

        // Call test() if present
        {
            das_function * fn_test = das_context_find_function(ctx, "test");
            if (fn_test) {
                das_context_eval_with_catch(ctx, fn_test, NULL);
                char * ex = das_context_get_exception(ctx);
                if (ex) printf("[C] test() exception: %s\n", ex);
            }
        }

        printf("\nInitial state:\n");
        print_entities();

        // Game loop — 3 ticks at 60 fps
        float dt = 1.0f / 60.0f;
        for (int tick = 0; tick < 3; tick++) {
            set_global_float("dt", dt);
            ecs_update(ctx);
        }

        printf("\nAfter 3 ticks (dt=%.4f):\n", dt);
        print_entities();

        das_context_release(ctx);
    }

cleanup:
    das_program_release(program);
    das_fileaccess_release(file_access);
    das_modulegroup_release(libgroup);
    das_text_release(tout);

    das_shutdown();
    return 0;
}
