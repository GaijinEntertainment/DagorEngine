#pragma once

#include "daScript/misc/platform.h"

// Keep this list short. Every entry is a call out of jitted code into the runtime, which costs a
// call boundary the optimizer cannot see through. An intrinsic emitted as LLVM IR inlines and
// folds with its neighbours, so implement one in IR whenever the operation can be expressed there,
// and reach for a runtime entry point only when it cannot.

namespace das {

    class Context;
    struct LineInfoArg;
    struct TypeAnnotation;

    // The JIT runtime entry points live in jit_runtime.cpp; these hand their addresses to the
    // emitter, and module_jit.cpp binds them. They run in the compiling process only, so they
    // are not in aot_builtin_jit.h, which Module_Jit::aotRequire injects into generated AOT C++.
    void * das_get_jit_exception ();
    void * das_get_jit_call_or_fastcall ();
    void * das_get_jit_call_with_cmres ();
    void * das_get_jit_invoke_block ();
    void * das_get_jit_invoke_block_with_cmres ();
    void * das_get_jit_string_builder ();
    void * das_get_jit_string_builder_temp ();
    void * das_get_jit_get_global_mnh ();
    void * das_get_jit_get_shared_mnh ();
    void * das_get_jit_get_handled_field_offset ();
    void * das_get_jit_check_handled_type_size ();
    void * das_get_jit_check_handled_field_offset ();
    void * das_get_jit_handled_abi_check_report ();
    void * das_get_jit_alloc_heap ();
    void * das_get_jit_alloc_persistent ();
    void * das_get_jit_free_heap ();
    void * das_get_jit_free_persistent ();
    void * das_get_jit_array_resize ();
    void * das_get_jit_table_at ( int32_t baseType, Context * context, LineInfoArg * at );
    void * das_get_jit_table_erase ( int32_t baseType, Context * context, LineInfoArg * at );
    void * das_get_jit_table_find ( int32_t baseType, Context * context, LineInfoArg * at );
    void * das_get_jit_string_table_at_with_hash ();
    void * das_get_jit_string_table_at_after_packed_miss ();
    void * das_get_jit_str_cat ();
    void * das_get_jit_ast_typedecl ();
    void * das_get_jit_make_block ();
    void * das_get_jit_try_recover ();
    void * das_get_jit_ad_by_sid ();
    void * das_get_jit_debug ();
    void * das_get_jit_debug_enter ();
    void * das_get_jit_debug_exit ();
    void * das_get_jit_debug_line ();
    void * das_get_jit_iterator_iterate ();
    void * das_get_jit_iterator_delete ();
    void * das_get_jit_iterator_close ();
    void * das_get_jit_iterator_first ();
    void * das_get_jit_iterator_next ();
    void * das_get_jit_initialize_fileinfo ();
    void * das_get_jit_free_fileinfo ();

    // llvm_exe.das declares these into the emitted module by name (LLVMAddFunctionWithType),
    // so a standalone exe links them undecorated - C linkage, not C++ mangling.
    extern "C" {
        DAS_API void * das_get_jit_simnode_interop ();
        DAS_API void * das_get_jit_free_simnode_interop ();
        DAS_API void * das_get_jit_init_extern_function ();
        DAS_API void * das_get_jit_new ( TypeAnnotation * annotation );
        DAS_API void * das_get_jit_delete ( TypeAnnotation * annotation );
        DAS_API void * das_get_jit_clone ( TypeAnnotation * annotation );
        DAS_API void * das_get_jit_each ( TypeAnnotation * annotation );
        DAS_API void * das_get_jit_at ( TypeAnnotation * annotation, int32_t indexType );
    }
}
