// Tutorial: Type Introspection — Querying Type Layout from C
//
// This tutorial covers:
//   1. Getting type info for global variables (struct, array, enum)
//   2. Inspecting struct fields (name, type, offset, size)
//   3. Navigating nested struct types
//   4. Enumerating enum values
//   5. Inspecting function signatures (argument names and types)
//
// Build: link against libDaScript (C++ linker required).
// Run:   requires 11_type_introspection.das on disk.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "daScript/daScriptC.h"

// Helper: print a human-readable name for a base type.
static const char * base_type_name(das_base_type t) {
    switch (t) {
        case DAS_TYPE_VOID:          return "void";
        case DAS_TYPE_BOOL:          return "bool";
        case DAS_TYPE_INT8:          return "int8";
        case DAS_TYPE_UINT8:         return "uint8";
        case DAS_TYPE_INT16:         return "int16";
        case DAS_TYPE_UINT16:        return "uint16";
        case DAS_TYPE_INT64:         return "int64";
        case DAS_TYPE_UINT64:        return "uint64";
        case DAS_TYPE_INT:           return "int";
        case DAS_TYPE_INT2:          return "int2";
        case DAS_TYPE_INT3:          return "int3";
        case DAS_TYPE_INT4:          return "int4";
        case DAS_TYPE_UINT:          return "uint";
        case DAS_TYPE_UINT2:         return "uint2";
        case DAS_TYPE_UINT3:         return "uint3";
        case DAS_TYPE_UINT4:         return "uint4";
        case DAS_TYPE_FLOAT:         return "float";
        case DAS_TYPE_FLOAT2:        return "float2";
        case DAS_TYPE_FLOAT3:        return "float3";
        case DAS_TYPE_FLOAT4:        return "float4";
        case DAS_TYPE_DOUBLE:        return "double";
        case DAS_TYPE_RANGE:         return "range";
        case DAS_TYPE_URANGE:        return "urange";
        case DAS_TYPE_RANGE64:       return "range64";
        case DAS_TYPE_URANGE64:      return "urange64";
        case DAS_TYPE_STRING:        return "string";
        case DAS_TYPE_STRUCTURE:     return "struct";
        case DAS_TYPE_HANDLE:        return "handle";
        case DAS_TYPE_ENUMERATION:   return "enum";
        case DAS_TYPE_ENUMERATION8:  return "enum8";
        case DAS_TYPE_ENUMERATION16: return "enum16";
        case DAS_TYPE_ENUMERATION64: return "enum64";
        case DAS_TYPE_BITFIELD:      return "bitfield";
        case DAS_TYPE_BITFIELD8:     return "bitfield8";
        case DAS_TYPE_BITFIELD16:    return "bitfield16";
        case DAS_TYPE_BITFIELD64:    return "bitfield64";
        case DAS_TYPE_POINTER:       return "pointer";
        case DAS_TYPE_FUNCTION:      return "function";
        case DAS_TYPE_LAMBDA:        return "lambda";
        case DAS_TYPE_ITERATOR:      return "iterator";
        case DAS_TYPE_ARRAY:         return "array";
        case DAS_TYPE_TABLE:         return "table";
        case DAS_TYPE_BLOCK:         return "block";
        case DAS_TYPE_TUPLE:         return "tuple";
        case DAS_TYPE_VARIANT:       return "variant";
        default:                     return "unknown";
    }
}

// Helper: print struct layout recursively with indentation.
static void print_struct_layout(das_struct_info * si, int indent) {
    int nfields = das_struct_info_get_field_count(si);
    int total_size = das_struct_info_get_size(si);
    printf("%*s%s (%d bytes, %d fields):\n", indent, "",
           das_struct_info_get_name(si), total_size, nfields);
    for (int f = 0; f < nfields; f++) {
        const char * fname = das_struct_info_get_field_name(si, f);
        int offset = das_struct_info_get_field_offset(si, f);
        das_type_info * ftype = das_struct_info_get_field_type(si, f);
        das_base_type bt = das_type_info_get_type(ftype);
        int fsz = das_type_info_get_size(ftype);
        printf("%*s  +%-3d %-20s %-10s %d bytes\n", indent, "",
               offset, fname, base_type_name(bt), fsz);
        // Recurse into nested structs.
        if (bt == DAS_TYPE_STRUCTURE) {
            das_struct_info * nested = das_type_info_get_struct(ftype);
            if (nested) {
                print_struct_layout(nested, indent + 6);
            }
        }
    }
}

int main(int argc, char ** argv) {
    (void)argc; (void)argv;

    das_initialize();

    das_text_writer  * tout   = das_text_make_printer();
    das_module_group * libgrp = das_modulegroup_make();
    das_file_access  * fa     = das_fileaccess_make_default();

    // Build the script path relative to getDasRoot().
    char script[512];
    das_get_root(script, sizeof(script));
    strncat(script, "/tutorials/integration/c/11_type_introspection.das",
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
        // 1. Struct introspection — navigate array<Entity> to Entity
        // ---------------------------------------------------------------
        printf("=== Struct introspection ===\n");

        int idx = das_context_find_variable(ctx, "entities");
        if (idx >= 0) {
            das_type_info * ti = das_context_get_variable_type(ctx, idx);
            printf("'entities' type: %s\n", das_type_info_get_description(ti));
            printf("'entities' base type: %s\n", base_type_name(das_type_info_get_type(ti)));

            // Navigate: array<Entity> -> Entity
            das_type_info * elem = das_type_info_get_first_type(ti);
            if (elem && das_type_info_get_type(elem) == DAS_TYPE_STRUCTURE) {
                das_struct_info * si = das_type_info_get_struct(elem);
                if (si) {
                    printf("\nEntity struct layout (with nested Transform):\n");
                    print_struct_layout(si, 2);
                }
            }
        }

        // ---------------------------------------------------------------
        // 2. Enum introspection — find the Direction enum
        // ---------------------------------------------------------------
        printf("\n=== Enum introspection ===\n");

        if (idx >= 0) {
            das_type_info * ti = das_context_get_variable_type(ctx, idx);
            das_type_info * elem = das_type_info_get_first_type(ti);
            if (elem) {
                das_struct_info * si = das_type_info_get_struct(elem);
                if (si) {
                    // Find the 'direction' field (last field of Entity).
                    int nfields = das_struct_info_get_field_count(si);
                    for (int f = 0; f < nfields; f++) {
                        das_type_info * ftype = das_struct_info_get_field_type(si, f);
                        das_enum_info * ei = das_type_info_get_enum(ftype);
                        if (ei) {
                            const char * emod = das_enum_info_get_module(ei);
                            printf("Enum '%s' (module: %s):\n",
                                   das_enum_info_get_name(ei),
                                   emod ? emod : "<script>");
                            int nvals = das_enum_info_get_count(ei);
                            for (int v = 0; v < nvals; v++) {
                                printf("  %s = %lld\n",
                                       das_enum_info_get_value_name(ei, v),
                                       (long long)das_enum_info_get_value(ei, v));
                            }
                        }
                    }
                }
            }
        }

        // ---------------------------------------------------------------
        // 3. Function introspection — enumerate exported functions
        // ---------------------------------------------------------------
        printf("\n=== Function introspection ===\n");

        int total_fns = das_context_get_total_functions(ctx);
        for (int i = 0; i < total_fns; i++) {
            das_function * fn = das_context_get_function(ctx, i);
            if (!fn) continue;
            das_func_info * fi = das_function_get_info(fn);
            if (!fi) continue;

            // Skip private and built-in functions.
            uint32_t flags = das_func_info_get_flags(fi);
            if (flags & DAS_FUNCINFO_FLAG_BUILTIN) continue;
            if (flags & DAS_FUNCINFO_FLAG_PRIVATE) continue;

            const char * name = das_func_info_get_name(fi);
            das_type_info * ret = das_func_info_get_result(fi);
            printf("def %s(", name);

            int nargs = das_func_info_get_arg_count(fi);
            for (int a = 0; a < nargs; a++) {
                if (a > 0) printf("; ");
                const char * aname = das_func_info_get_arg_name(fi, a);
                das_type_info * atype = das_func_info_get_arg_type(fi, a);
                printf("%s : %s", aname, das_type_info_get_description(atype));
            }

            printf(") : %s", ret ? das_type_info_get_description(ret) : "void");
            if (das_function_is_aot(fn))
                printf("  [AOT]");
            printf("\n");
        }

        // ---------------------------------------------------------------
        // 4. Engine-side allocation using discovered layout
        // ---------------------------------------------------------------
        printf("\n=== Engine-side allocation ===\n");

        if (idx >= 0) {
            das_type_info * ti = das_context_get_variable_type(ctx, idx);
            das_type_info * elem = das_type_info_get_first_type(ti);
            if (elem) {
                das_struct_info * si = das_type_info_get_struct(elem);
                if (si) {
                    int struct_size = das_struct_info_get_size(si);
                    int count = 3;
                    char * buf = (char *)calloc(count, struct_size);
                    printf("Allocated %d entities (%d bytes each, %d total)\n",
                           count, struct_size, count * struct_size);

                    // Demonstrate writing a field at a known offset.
                    // Find the 'health' field offset.
                    int nfields = das_struct_info_get_field_count(si);
                    for (int f = 0; f < nfields; f++) {
                        if (strcmp(das_struct_info_get_field_name(si, f), "health") == 0) {
                            int off = das_struct_info_get_field_offset(si, f);
                            for (int e = 0; e < count; e++) {
                                int * hp = (int *)(buf + e * struct_size + off);
                                *hp = 100 + e;
                                printf("  entity[%d].health = %d (at byte offset %d)\n",
                                       e, *hp, e * struct_size + off);
                            }
                            break;
                        }
                    }
                    free(buf);
                }
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
