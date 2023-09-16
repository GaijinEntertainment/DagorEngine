#pragma once

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_visitor.h"

namespace das {
    float4 das_invoke_code ( void * pfun, vec4f anything, void * cmres, Context * context );
    bool das_is_jit_function ( const Func func );
    bool das_remove_jit ( const Func func );
    bool das_instrument_jit ( void * pfun, const Func func, Context * context );
    void * das_get_jit_exception ();
    void * das_get_jit_call_or_fastcall ();
    void * das_get_jit_call_with_cmres ( );
    void * das_get_jit_invoke_block ( );
    void * das_get_jit_invoke_block_with_cmres ( );
    void * das_get_jit_string_builder ();
    void * das_get_jit_get_global_mnh ();
    void * das_get_jit_alloc_heap ();
    void * das_get_jit_alloc_persistent ();
    void * das_get_jit_free_heap ();
    void * das_get_jit_free_persistent ();
    void * das_get_jit_array_lock ();
    void * das_get_jit_array_unlock ();
    void * das_get_jit_table_at ( int32_t baseType, Context * context, LineInfoArg * at );
    void * das_get_jit_table_erase ( int32_t baseType, Context * context, LineInfoArg * at );
    void * das_get_jit_table_find ( int32_t baseType, Context * context, LineInfoArg * at );
    void * das_get_jit_str_cmp ();
    void * das_get_jit_prologue ();
    void * das_get_jit_epilogue ();
    void * das_get_jit_make_block ();
    void * das_get_jit_debug ();
    void * das_get_jit_iterator_iterate();
    void * das_get_jit_iterator_delete();
    void * das_get_jit_iterator_close();
    void * das_get_builtin_function_address ( Function * fn, Context * context, LineInfoArg * at );
    void * das_make_interop_node ( Context & ctx, ExprCallFunc * call, Context * context, LineInfoArg * at );
    void * das_sb_make_interop_node ( Context & ctx, ExprStringBuilder * call, Context * context, LineInfoArg * at );
}
