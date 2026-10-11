#pragma once

#include "daScript/misc/vectypes.h"
#include "daScript/misc/arraytype.h"
#include "daScript/misc/smart_ptr.h"

namespace das {

    class Context;
    struct LineInfo;
    struct LineInfoArg;
    class Function;
    struct ExprCallFunc;
    struct ExprStringBuilder;

    struct TypeDecl;
    typedef TypeDecl * TypeDeclPtr;

    float4 das_invoke_code ( void * pfun, vec4f anything, void * cmres, Context * context );
    bool das_is_jit_function ( const Func func );
    bool das_is_aot_function ( const Func func );
    bool das_has_jit_fastpath ( const Func func );
    bool das_remove_jit ( const Func func );
    bool das_instrument_jit ( void * pfun, const Func func, const LineInfo & info, Context & context );
    void * das_instrument_line_info ( const LineInfo & info, Context * context, LineInfoArg * at );
    void * das_get_builtin_function_address ( Function * fn, Context * context, LineInfoArg * at );
}
