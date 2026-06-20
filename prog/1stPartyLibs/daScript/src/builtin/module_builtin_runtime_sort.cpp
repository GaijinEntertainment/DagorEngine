#include "daScript/misc/platform.h"

#include "module_builtin.h"

#include "daScript/ast/ast_interop.h"
#include "daScript/simulate/aot_builtin.h"
#include "daScript/simulate/sim_policy.h"
#include "daScript/simulate/das_qsort_r.h"

namespace das
{
    struct AnySortContext {
        vec4f *     bargs;
        SimNode *   node;
        Context *   context;
    };

    // unspecified

    void builtin_sort_any_cblock ( void * anyData, int32_t elementSize, int32_t length, const Block & cmp, Context * context, LineInfoArg * at ) {
        if ( length<=1 ) return;
        vec4f bargs[2];
        context->invokeEx(cmp, bargs, nullptr, [&](SimNode * code) {
            das_qsort_r(anyData, length, elementSize, [&](const void * x, const void * y){
              bargs[0] = cast<void *>::from(x);
              bargs[1] = cast<void *>::from(y);
              return code->evalBool(*context);
            });
        }, at);
    }

    void builtin_sort_any_ref_cblock ( void * anyData, int32_t elementSize, int32_t length, const Block & cmp, Context * context, LineInfoArg * at ) {
        if ( length<=1 ) return;
        vec4f bargs[2];
        if ( elementSize <= 4 ) {
            context->invokeEx(cmp, bargs, nullptr, [&](SimNode * code) {
                das_qsort_r(anyData, length, elementSize, [&](const void * x, const void * y){
                    bargs[0] = v_ldu_x((const float *)x);
                    bargs[1] = v_ldu_x((const float *)y);
                    return code->evalBool(*context);
                });
            },at);
        } else if ( elementSize <= 8 ) {
            context->invokeEx(cmp, bargs, nullptr, [&](SimNode * code) {
                das_qsort_r(anyData, length, elementSize, [&](const void * x, const void * y){
                    bargs[0] = v_ldu_half(x);
                    bargs[1] = v_ldu_half(y);
                    return code->evalBool(*context);
                });
            }, at);
        } else {
            context->invokeEx(cmp, bargs, nullptr, [&](SimNode * code) {
                das_qsort_r(anyData, length, elementSize, [&](const void * x, const void * y){
                    bargs[0] = v_ldu((const float *)x);
                    bargs[1] = v_ldu((const float *)y);
                    return code->evalBool(*context);
                });
            }, at);
        }
    }

    void builtin_sort_dim_any_cblock ( vec4f anything, int32_t elementSize, int32_t length, const Block & cmp, Context * context, LineInfoArg * at ) {
        auto anyData = cast<void *>::to(anything);
        builtin_sort_any_cblock(anyData, elementSize, length, cmp, context, at);
    }

    void builtin_sort_dim_any_ref_cblock ( vec4f anything, int32_t elementSize, int32_t length, const Block & cmp, Context * context, LineInfoArg * at ) {
        auto anyData = cast<void *>::to(anything);
        builtin_sort_any_ref_cblock(anyData, elementSize, length, cmp, context, at);
    }

    // The sort wrappers below pass arr.size (uint64_t) into inner sort functions taking
    // int32_t length. For arrays > INT_MAX elements this would silently truncate and
    // sort only a prefix. Panic up-front instead; the int-sized sort surface simply
    // does not support arrays past INT_MAX, and there is no `long_sort` companion yet.
    static __forceinline int32_t sort_array_size_or_panic ( const Array & arr, Context * context, LineInfoArg * at, const char * op ) {
        if ( arr.size > uint64_t(INT32_MAX) ) context->throw_error_at(at, "%s: array size %llu exceeds INT_MAX; sort is not supported on arrays this large", op, (unsigned long long)arr.size);
        return int32_t(arr.size);
    }

    void builtin_sort_array_any_cblock ( Array & arr, int32_t elementSize, int32_t, const Block & cmp, Context * context, LineInfoArg * at ) {
        int32_t len = sort_array_size_or_panic(arr, context, at, "sort");
        array_lock(*context,arr,at);
        builtin_sort_any_cblock(arr.data, elementSize, len, cmp, context, at);
        array_unlock(*context,arr,at);
    }

    void builtin_sort_array_any_ref_cblock ( Array & arr, int32_t elementSize, int32_t, const Block & cmp, Context * context, LineInfoArg * at ) {
        int32_t len = sort_array_size_or_panic(arr, context, at, "sort");
        array_lock(*context,arr,at);
        builtin_sort_any_ref_cblock(arr.data, elementSize, len, cmp, context, at);
        array_unlock(*context,arr,at);
    }

    void builtin_sort_string ( void * data, int32_t length ) {
        if ( length<=1 ) return;
        const char ** pdata = (const char **) data;
        das_sort ( pdata, pdata + length, [&](const char * a, const char * b){
            return strcmp(to_rts(a), to_rts(b))<0;
        });
    }

    // ==========================================================================
    // stable_sort any-path runtime wrappers — mirror of the builtin_sort_*_any_*
    // surface above, routing through the byte-pointer das_stable_sort_r (adaptive
    // natural-run merge, stable). Interp path only; AOT uses the typed _T
    // templates (das_stable_sort<T>) in aot.h.
    // ==========================================================================

    void builtin_stable_sort_any_cblock ( void * anyData, int32_t elementSize, int32_t length, const Block & cmp, Context * context, LineInfoArg * at ) {
        if ( length<=1 ) return;
        vec4f bargs[2];
        context->invokeEx(cmp, bargs, nullptr, [&](SimNode * code) {
            das_stable_sort_r(anyData, length, elementSize, [&](const void * x, const void * y){
              bargs[0] = cast<void *>::from(x);
              bargs[1] = cast<void *>::from(y);
              return code->evalBool(*context);
            });
        }, at);
    }

    void builtin_stable_sort_any_ref_cblock ( void * anyData, int32_t elementSize, int32_t length, const Block & cmp, Context * context, LineInfoArg * at ) {
        if ( length<=1 ) return;
        vec4f bargs[2];
        if ( elementSize <= 4 ) {
            context->invokeEx(cmp, bargs, nullptr, [&](SimNode * code) {
                das_stable_sort_r(anyData, length, elementSize, [&](const void * x, const void * y){
                    bargs[0] = v_ldu_x((const float *)x);
                    bargs[1] = v_ldu_x((const float *)y);
                    return code->evalBool(*context);
                });
            },at);
        } else if ( elementSize <= 8 ) {
            context->invokeEx(cmp, bargs, nullptr, [&](SimNode * code) {
                das_stable_sort_r(anyData, length, elementSize, [&](const void * x, const void * y){
                    bargs[0] = v_ldu_half(x);
                    bargs[1] = v_ldu_half(y);
                    return code->evalBool(*context);
                });
            }, at);
        } else {
            context->invokeEx(cmp, bargs, nullptr, [&](SimNode * code) {
                das_stable_sort_r(anyData, length, elementSize, [&](const void * x, const void * y){
                    bargs[0] = v_ldu((const float *)x);
                    bargs[1] = v_ldu((const float *)y);
                    return code->evalBool(*context);
                });
            }, at);
        }
    }

    void builtin_stable_sort_dim_any_cblock ( vec4f anything, int32_t elementSize, int32_t length, const Block & cmp, Context * context, LineInfoArg * at ) {
        builtin_stable_sort_any_cblock(cast<void *>::to(anything), elementSize, length, cmp, context, at);
    }

    void builtin_stable_sort_dim_any_ref_cblock ( vec4f anything, int32_t elementSize, int32_t length, const Block & cmp, Context * context, LineInfoArg * at ) {
        builtin_stable_sort_any_ref_cblock(cast<void *>::to(anything), elementSize, length, cmp, context, at);
    }

    void builtin_stable_sort_array_any_cblock ( Array & arr, int32_t elementSize, int32_t, const Block & cmp, Context * context, LineInfoArg * at ) {
        int32_t len = sort_array_size_or_panic(arr, context, at, "stable_sort");
        array_lock(*context,arr,at);
        builtin_stable_sort_any_cblock(arr.data, elementSize, len, cmp, context, at);
        array_unlock(*context,arr,at);
    }

    void builtin_stable_sort_array_any_ref_cblock ( Array & arr, int32_t elementSize, int32_t, const Block & cmp, Context * context, LineInfoArg * at ) {
        int32_t len = sort_array_size_or_panic(arr, context, at, "stable_sort");
        array_lock(*context,arr,at);
        builtin_stable_sort_any_ref_cblock(arr.data, elementSize, len, cmp, context, at);
        array_unlock(*context,arr,at);
    }

    // ==========================================================================
    // partial_sort / nth_element / heap-op any-path runtime wrappers.
    //
    // Mirrors the builtin_sort_*_any_cblock surface but routes through the
    // byte-pointer das_<op>_r templates in das_qsort_r.h. Each pair is the
    // "cblock" (workhorse-cast bargs) and "ref_cblock" (raw-load bargs)
    // variant of the same algorithm.
    // ==========================================================================

    // ---- partial_sort any path ----

    void builtin_partial_sort_any_cblock ( void * anyData, int32_t elementSize, int32_t length, int32_t n, const Block & cmp, Context * context, LineInfoArg * at ) {
        if ( length<=1 || n<=0 ) return;
        if ( n>length ) n = length;
        vec4f bargs[2];
        context->invokeEx(cmp, bargs, nullptr, [&](SimNode * code) {
            das_partial_sort_r(anyData, length, n, elementSize, [&](const void * x, const void * y){
                bargs[0] = cast<void *>::from(x);
                bargs[1] = cast<void *>::from(y);
                return code->evalBool(*context);
            });
        }, at);
    }

    void builtin_partial_sort_any_ref_cblock ( void * anyData, int32_t elementSize, int32_t length, int32_t n, const Block & cmp, Context * context, LineInfoArg * at ) {
        if ( length<=1 || n<=0 ) return;
        if ( n>length ) n = length;
        vec4f bargs[2];
        if ( elementSize <= 4 ) {
            context->invokeEx(cmp, bargs, nullptr, [&](SimNode * code) {
                das_partial_sort_r(anyData, length, n, elementSize, [&](const void * x, const void * y){
                    bargs[0] = v_ldu_x((const float *)x);
                    bargs[1] = v_ldu_x((const float *)y);
                    return code->evalBool(*context);
                });
            }, at);
        } else if ( elementSize <= 8 ) {
            context->invokeEx(cmp, bargs, nullptr, [&](SimNode * code) {
                das_partial_sort_r(anyData, length, n, elementSize, [&](const void * x, const void * y){
                    bargs[0] = v_ldu_half(x);
                    bargs[1] = v_ldu_half(y);
                    return code->evalBool(*context);
                });
            }, at);
        } else {
            context->invokeEx(cmp, bargs, nullptr, [&](SimNode * code) {
                das_partial_sort_r(anyData, length, n, elementSize, [&](const void * x, const void * y){
                    bargs[0] = v_ldu((const float *)x);
                    bargs[1] = v_ldu((const float *)y);
                    return code->evalBool(*context);
                });
            }, at);
        }
    }

    void builtin_partial_sort_dim_any_cblock ( vec4f anything, int32_t elementSize, int32_t length, int32_t n, const Block & cmp, Context * context, LineInfoArg * at ) {
        builtin_partial_sort_any_cblock(cast<void *>::to(anything), elementSize, length, n, cmp, context, at);
    }
    void builtin_partial_sort_dim_any_ref_cblock ( vec4f anything, int32_t elementSize, int32_t length, int32_t n, const Block & cmp, Context * context, LineInfoArg * at ) {
        builtin_partial_sort_any_ref_cblock(cast<void *>::to(anything), elementSize, length, n, cmp, context, at);
    }
    void builtin_partial_sort_array_any_cblock ( Array & arr, int32_t elementSize, int32_t, int32_t n, const Block & cmp, Context * context, LineInfoArg * at ) {
        int32_t len = sort_array_size_or_panic(arr, context, at, "partial_sort");
        array_lock(*context, arr, at);
        builtin_partial_sort_any_cblock(arr.data, elementSize, len, n, cmp, context, at);
        array_unlock(*context, arr, at);
    }
    void builtin_partial_sort_array_any_ref_cblock ( Array & arr, int32_t elementSize, int32_t, int32_t n, const Block & cmp, Context * context, LineInfoArg * at ) {
        int32_t len = sort_array_size_or_panic(arr, context, at, "partial_sort");
        array_lock(*context, arr, at);
        builtin_partial_sort_any_ref_cblock(arr.data, elementSize, len, n, cmp, context, at);
        array_unlock(*context, arr, at);
    }

    // ---- nth_element any path ----

    void builtin_nth_element_any_cblock ( void * anyData, int32_t elementSize, int32_t length, int32_t n, const Block & cmp, Context * context, LineInfoArg * at ) {
        if ( length<=1 || n<0 || n>=length ) return;
        vec4f bargs[2];
        context->invokeEx(cmp, bargs, nullptr, [&](SimNode * code) {
            das_nth_element_r(anyData, length, n, elementSize, [&](const void * x, const void * y){
                bargs[0] = cast<void *>::from(x);
                bargs[1] = cast<void *>::from(y);
                return code->evalBool(*context);
            });
        }, at);
    }

    void builtin_nth_element_any_ref_cblock ( void * anyData, int32_t elementSize, int32_t length, int32_t n, const Block & cmp, Context * context, LineInfoArg * at ) {
        if ( length<=1 || n<0 || n>=length ) return;
        vec4f bargs[2];
        if ( elementSize <= 4 ) {
            context->invokeEx(cmp, bargs, nullptr, [&](SimNode * code) {
                das_nth_element_r(anyData, length, n, elementSize, [&](const void * x, const void * y){
                    bargs[0] = v_ldu_x((const float *)x);
                    bargs[1] = v_ldu_x((const float *)y);
                    return code->evalBool(*context);
                });
            }, at);
        } else if ( elementSize <= 8 ) {
            context->invokeEx(cmp, bargs, nullptr, [&](SimNode * code) {
                das_nth_element_r(anyData, length, n, elementSize, [&](const void * x, const void * y){
                    bargs[0] = v_ldu_half(x);
                    bargs[1] = v_ldu_half(y);
                    return code->evalBool(*context);
                });
            }, at);
        } else {
            context->invokeEx(cmp, bargs, nullptr, [&](SimNode * code) {
                das_nth_element_r(anyData, length, n, elementSize, [&](const void * x, const void * y){
                    bargs[0] = v_ldu((const float *)x);
                    bargs[1] = v_ldu((const float *)y);
                    return code->evalBool(*context);
                });
            }, at);
        }
    }

    void builtin_nth_element_dim_any_cblock ( vec4f anything, int32_t elementSize, int32_t length, int32_t n, const Block & cmp, Context * context, LineInfoArg * at ) {
        builtin_nth_element_any_cblock(cast<void *>::to(anything), elementSize, length, n, cmp, context, at);
    }
    void builtin_nth_element_dim_any_ref_cblock ( vec4f anything, int32_t elementSize, int32_t length, int32_t n, const Block & cmp, Context * context, LineInfoArg * at ) {
        builtin_nth_element_any_ref_cblock(cast<void *>::to(anything), elementSize, length, n, cmp, context, at);
    }
    void builtin_nth_element_array_any_cblock ( Array & arr, int32_t elementSize, int32_t, int32_t n, const Block & cmp, Context * context, LineInfoArg * at ) {
        int32_t len = sort_array_size_or_panic(arr, context, at, "nth_element");
        array_lock(*context, arr, at);
        builtin_nth_element_any_cblock(arr.data, elementSize, len, n, cmp, context, at);
        array_unlock(*context, arr, at);
    }
    void builtin_nth_element_array_any_ref_cblock ( Array & arr, int32_t elementSize, int32_t, int32_t n, const Block & cmp, Context * context, LineInfoArg * at ) {
        int32_t len = sort_array_size_or_panic(arr, context, at, "nth_element");
        array_lock(*context, arr, at);
        builtin_nth_element_any_ref_cblock(arr.data, elementSize, len, n, cmp, context, at);
        array_unlock(*context, arr, at);
    }

    // ---- Heap-op any path ----
    //
    // Each of the 3 heap operations gets the same 6-variant surface
    // (any_cblock, any_ref_cblock, dim_any_cblock, dim_any_ref_cblock,
    //  array_any_cblock, array_any_ref_cblock). Body shape is identical
    // modulo which das_<op>_r is invoked.

    #define DEFINE_HEAP_ANY_PATH(OP, DAS_R) \
        void builtin_##OP##_any_cblock ( void * anyData, int32_t elementSize, int32_t length, const Block & cmp, Context * context, LineInfoArg * at ) { \
            if ( length<=1 ) return; \
            vec4f bargs[2]; \
            context->invokeEx(cmp, bargs, nullptr, [&](SimNode * code) { \
                DAS_R(anyData, length, elementSize, [&](const void * x, const void * y){ \
                    bargs[0] = cast<void *>::from(x); \
                    bargs[1] = cast<void *>::from(y); \
                    return code->evalBool(*context); \
                }); \
            }, at); \
        } \
        void builtin_##OP##_any_ref_cblock ( void * anyData, int32_t elementSize, int32_t length, const Block & cmp, Context * context, LineInfoArg * at ) { \
            if ( length<=1 ) return; \
            vec4f bargs[2]; \
            if ( elementSize <= 4 ) { \
                context->invokeEx(cmp, bargs, nullptr, [&](SimNode * code) { \
                    DAS_R(anyData, length, elementSize, [&](const void * x, const void * y){ \
                        bargs[0] = v_ldu_x((const float *)x); \
                        bargs[1] = v_ldu_x((const float *)y); \
                        return code->evalBool(*context); \
                    }); \
                }, at); \
            } else if ( elementSize <= 8 ) { \
                context->invokeEx(cmp, bargs, nullptr, [&](SimNode * code) { \
                    DAS_R(anyData, length, elementSize, [&](const void * x, const void * y){ \
                        bargs[0] = v_ldu_half(x); \
                        bargs[1] = v_ldu_half(y); \
                        return code->evalBool(*context); \
                    }); \
                }, at); \
            } else { \
                context->invokeEx(cmp, bargs, nullptr, [&](SimNode * code) { \
                    DAS_R(anyData, length, elementSize, [&](const void * x, const void * y){ \
                        bargs[0] = v_ldu((const float *)x); \
                        bargs[1] = v_ldu((const float *)y); \
                        return code->evalBool(*context); \
                    }); \
                }, at); \
            } \
        } \
        void builtin_##OP##_dim_any_cblock ( vec4f anything, int32_t elementSize, int32_t length, const Block & cmp, Context * context, LineInfoArg * at ) { \
            builtin_##OP##_any_cblock(cast<void *>::to(anything), elementSize, length, cmp, context, at); \
        } \
        void builtin_##OP##_dim_any_ref_cblock ( vec4f anything, int32_t elementSize, int32_t length, const Block & cmp, Context * context, LineInfoArg * at ) { \
            builtin_##OP##_any_ref_cblock(cast<void *>::to(anything), elementSize, length, cmp, context, at); \
        } \
        void builtin_##OP##_array_any_cblock ( Array & arr, int32_t elementSize, int32_t, const Block & cmp, Context * context, LineInfoArg * at ) { \
            array_lock(*context, arr, at); \
            builtin_##OP##_any_cblock(arr.data, elementSize, arr.size, cmp, context, at); \
            array_unlock(*context, arr, at); \
        } \
        void builtin_##OP##_array_any_ref_cblock ( Array & arr, int32_t elementSize, int32_t, const Block & cmp, Context * context, LineInfoArg * at ) { \
            array_lock(*context, arr, at); \
            builtin_##OP##_any_ref_cblock(arr.data, elementSize, arr.size, cmp, context, at); \
            array_unlock(*context, arr, at); \
        }

    DEFINE_HEAP_ANY_PATH(make_heap, das_make_heap_r)
    DEFINE_HEAP_ANY_PATH(push_heap, das_push_heap_r)
    DEFINE_HEAP_ANY_PATH(pop_heap, das_pop_heap_r)

    #undef DEFINE_HEAP_ANY_PATH

#define xstr(a) str(a)
#define str(a) #a

#define ADD_NUMERIC_SORT(CTYPE) \
    addExtern<DAS_BIND_FUN(builtin_sort<CTYPE>)>(*this, lib, "__builtin_sort", \
        SideEffects::modifyArgumentAndExternal, "builtin_sort<" xstr(CTYPE) ">") \
            ->args({"data","length"}); \
    addExtern<DAS_BIND_FUN(builtin_sort_cblock<CTYPE>)>(*this, lib, "__builtin_sort_cblock", \
        SideEffects::modifyArgumentAndExternal, "builtin_sort_cblock<" xstr(CTYPE) ">") \
            ->args({"array","stride","length","block","context","line"}); \
    addExtern<DAS_BIND_FUN(builtin_sort_cblock_array<CTYPE>)>(*this, lib, "__builtin_sort_cblock_array", \
        SideEffects::modifyArgumentAndExternal, "builtin_sort_array_any_ref_cblock_T") \
            ->args({"array","stride","length","block","context","line"})->setAotTemplate(); \
    addExtern<DAS_BIND_FUN(builtin_sort_cblock<CTYPE>)>(*this, lib, "__builtin_sort_cblock_dim", \
        SideEffects::modifyArgumentAndExternal, "builtin_sort_dim_any_ref_cblock_T") \
            ->args({"array","stride","length","block","context","line"})->setAotTemplate()->setAnyTemplate();

#define ADD_VECTOR_SORT(CTYPE) \
    addExtern<DAS_BIND_FUN(builtin_sort_cblock<CTYPE>)>(*this, lib, "__builtin_sort_cblock", \
        SideEffects::modifyArgumentAndExternal, "builtin_sort_cblock<" xstr(CTYPE) ">") \
            ->args({"array","stride","length","block","context","line"}); \
    addExtern<DAS_BIND_FUN(builtin_sort_cblock_array<CTYPE>)>(*this, lib, "__builtin_sort_cblock_array", \
        SideEffects::modifyArgumentAndExternal, "builtin_sort_array_any_ref_cblock_T") \
            ->args({"array","stride","length","block","context","line"})->setAotTemplate(); \
    addExtern<DAS_BIND_FUN(builtin_sort_cblock<CTYPE>)>(*this, lib, "__builtin_sort_cblock_dim", \
        SideEffects::modifyArgumentAndExternal, "builtin_sort_dim_any_ref_cblock_T") \
            ->args({"array","stride","length","block","context","line"})->setAotTemplate()->setAnyTemplate();

// Phase 0 expansion: partial_sort / nth_element take an extra n; heap ops do not.

#define ADD_NUMERIC_OP_N(OP, CTYPE) \
    addExtern<DAS_BIND_FUN(builtin_##OP<CTYPE>)>(*this, lib, "__builtin_" #OP, \
        SideEffects::modifyArgumentAndExternal, "builtin_" #OP "<" xstr(CTYPE) ">") \
            ->args({"data","length","n"}); \
    addExtern<DAS_BIND_FUN(builtin_##OP##_cblock<CTYPE>)>(*this, lib, "__builtin_" #OP "_cblock", \
        SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_cblock<" xstr(CTYPE) ">") \
            ->args({"array","stride","length","n","block","context","line"}); \
    addExtern<DAS_BIND_FUN(builtin_##OP##_cblock_array<CTYPE>)>(*this, lib, "__builtin_" #OP "_cblock_array", \
        SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_array_any_ref_cblock_T") \
            ->args({"array","stride","length","n","block","context","line"})->setAotTemplate(); \
    addExtern<DAS_BIND_FUN(builtin_##OP##_cblock<CTYPE>)>(*this, lib, "__builtin_" #OP "_cblock_dim", \
        SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_dim_any_ref_cblock_T") \
            ->args({"array","stride","length","n","block","context","line"})->setAotTemplate()->setAnyTemplate();

#define ADD_VECTOR_OP_N(OP, CTYPE) \
    addExtern<DAS_BIND_FUN(builtin_##OP##_cblock<CTYPE>)>(*this, lib, "__builtin_" #OP "_cblock", \
        SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_cblock<" xstr(CTYPE) ">") \
            ->args({"array","stride","length","n","block","context","line"}); \
    addExtern<DAS_BIND_FUN(builtin_##OP##_cblock_array<CTYPE>)>(*this, lib, "__builtin_" #OP "_cblock_array", \
        SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_array_any_ref_cblock_T") \
            ->args({"array","stride","length","n","block","context","line"})->setAotTemplate(); \
    addExtern<DAS_BIND_FUN(builtin_##OP##_cblock<CTYPE>)>(*this, lib, "__builtin_" #OP "_cblock_dim", \
        SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_dim_any_ref_cblock_T") \
            ->args({"array","stride","length","n","block","context","line"})->setAotTemplate()->setAnyTemplate();

#define ADD_NUMERIC_OP(OP, CTYPE) \
    addExtern<DAS_BIND_FUN(builtin_##OP<CTYPE>)>(*this, lib, "__builtin_" #OP, \
        SideEffects::modifyArgumentAndExternal, "builtin_" #OP "<" xstr(CTYPE) ">") \
            ->args({"data","length"}); \
    addExtern<DAS_BIND_FUN(builtin_##OP##_cblock<CTYPE>)>(*this, lib, "__builtin_" #OP "_cblock", \
        SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_cblock<" xstr(CTYPE) ">") \
            ->args({"array","stride","length","block","context","line"}); \
    addExtern<DAS_BIND_FUN(builtin_##OP##_cblock_array<CTYPE>)>(*this, lib, "__builtin_" #OP "_cblock_array", \
        SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_array_any_ref_cblock_T") \
            ->args({"array","stride","length","block","context","line"})->setAotTemplate(); \
    addExtern<DAS_BIND_FUN(builtin_##OP##_cblock<CTYPE>)>(*this, lib, "__builtin_" #OP "_cblock_dim", \
        SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_dim_any_ref_cblock_T") \
            ->args({"array","stride","length","block","context","line"})->setAotTemplate()->setAnyTemplate();

#define ADD_VECTOR_OP(OP, CTYPE) \
    addExtern<DAS_BIND_FUN(builtin_##OP##_cblock<CTYPE>)>(*this, lib, "__builtin_" #OP "_cblock", \
        SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_cblock<" xstr(CTYPE) ">") \
            ->args({"array","stride","length","block","context","line"}); \
    addExtern<DAS_BIND_FUN(builtin_##OP##_cblock_array<CTYPE>)>(*this, lib, "__builtin_" #OP "_cblock_array", \
        SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_array_any_ref_cblock_T") \
            ->args({"array","stride","length","block","context","line"})->setAotTemplate(); \
    addExtern<DAS_BIND_FUN(builtin_##OP##_cblock<CTYPE>)>(*this, lib, "__builtin_" #OP "_cblock_dim", \
        SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_dim_any_ref_cblock_T") \
            ->args({"array","stride","length","block","context","line"})->setAotTemplate()->setAnyTemplate();

// Stamp out all 5 ops × all 19 numeric+vector workhorse types in one shot.

#define STAMP_NUMERIC_OPS(CTYPE) \
    ADD_NUMERIC_OP_N(partial_sort, CTYPE); \
    ADD_NUMERIC_OP_N(nth_element, CTYPE); \
    ADD_NUMERIC_OP(make_heap, CTYPE); \
    ADD_NUMERIC_OP(push_heap, CTYPE); \
    ADD_NUMERIC_OP(pop_heap, CTYPE);

#define STAMP_VECTOR_OPS(CTYPE) \
    ADD_VECTOR_OP_N(partial_sort, CTYPE); \
    ADD_VECTOR_OP_N(nth_element, CTYPE); \
    ADD_VECTOR_OP(make_heap, CTYPE); \
    ADD_VECTOR_OP(push_heap, CTYPE); \
    ADD_VECTOR_OP(pop_heap, CTYPE);

    // this one replaces sort(array<>) via correct builtin
    struct BuiltinSortFunctionAnnotation : FunctionAnnotation {
        BuiltinSortFunctionAnnotation() : FunctionAnnotation("builtin_array_sort") { }
        virtual bool apply ( const FunctionPtr &, ModuleGroup &, const AnnotationArgumentList &, string & ) override { return true; }
        virtual bool finalize ( const FunctionPtr &, ModuleGroup &, const AnnotationArgumentList &, const AnnotationArgumentList &, string & ) override { return true; }
        virtual bool apply ( ExprBlock *, ModuleGroup &, const AnnotationArgumentList &, string & ) override { return false; }
        virtual bool finalize ( ExprBlock *, ModuleGroup &, const AnnotationArgumentList &, const AnnotationArgumentList &, string & ) override { return false; }
        virtual ExpressionPtr transformCall ( ExprCallFunc * call, string & err ) override {
            for ( auto & arg : call->arguments ) {
                if ( !arg->type || !arg->type->isFullySealed() ) {
                    err = "sort requires fully inferred arguments";
                    return nullptr;
                }
            }
            const auto & arg = call->arguments[0];
            if ( arg->type->baseType==Type::tFixedArray ) {
                auto faT = arg->type;   // chain head: sort the outermost level (rows of a multi-dim array)
                auto arrType = new TypeDecl(*faT->firstType);
                auto newCall = static_cast<ExprCall*>(call->clone());
                auto stride = arrType->getSizeOf();
                auto strideArg = new ExprConstInt(call->at, stride);
                strideArg->generated = true;
                newCall->arguments.insert(newCall->arguments.begin()+1,strideArg);
                auto length = faT->fixedDim;
                auto lengthArg = new ExprConstInt(call->at, length);
                lengthArg->generated = true;
                newCall->arguments.insert(newCall->arguments.begin()+2,lengthArg);
                if ( arrType->isNumericComparable() || arrType->isVectorType() || arrType->isString() ) {
                    newCall->name = "__builtin_sort_cblock_dim";
                }  else {
                    if ( arrType->isRefType() ) {
                        newCall->name = "__builtin_sort_dim_any_cblock";
                    } else {
                        newCall->name = "__builtin_sort_dim_any_ref_cblock";
                    }
                }
                return newCall;
            } else if ( arg->type->isGoodArrayType() ) {
                const auto & arrType = arg->type->firstType;
                auto newCall = static_cast<ExprCall*>(call->clone());
                auto stride = arrType->getSizeOf();
                auto strideArg = new ExprConstInt(call->at, stride);
                strideArg->generated = true;
                newCall->arguments.insert(newCall->arguments.begin()+1,strideArg);
                auto lengthArg = new ExprConstInt(call->at, 0);
                lengthArg->generated = true;
                newCall->arguments.insert(newCall->arguments.begin()+2,lengthArg);
                if ( arrType->isNumericComparable() || arrType->isVectorType() || arrType->isString() ) {
                    newCall->name = "__builtin_sort_cblock_array";
                }  else {
                    if ( arrType->isRefType() ) {
                        newCall->name = "__builtin_sort_array_any_cblock";
                    } else {
                        newCall->name = "__builtin_sort_array_any_ref_cblock";
                    }
                }
                return newCall;
            } else {
                err = "expecting array<...> or []";
                return nullptr;
            }
        }
    };

    void Module_BuiltIn::addRuntimeSort(ModuleLibrary & lib) {
        // annotation
        addAnnotation(new BuiltinSortFunctionAnnotation());
        // numeric
        ADD_NUMERIC_SORT(int32_t);
        ADD_NUMERIC_SORT(uint32_t);
        ADD_NUMERIC_SORT(int64_t);
        ADD_NUMERIC_SORT(uint64_t);
        ADD_NUMERIC_SORT(float);
        ADD_NUMERIC_SORT(double);
        // vector
        ADD_VECTOR_SORT(range);
        ADD_VECTOR_SORT(urange);
        ADD_VECTOR_SORT(range64);
        ADD_VECTOR_SORT(urange64);
        ADD_VECTOR_SORT(int2);
        ADD_VECTOR_SORT(int3);
        ADD_VECTOR_SORT(int4);
        ADD_VECTOR_SORT(uint2);
        ADD_VECTOR_SORT(uint3);
        ADD_VECTOR_SORT(uint4);
        ADD_VECTOR_SORT(float2);
        ADD_VECTOR_SORT(float3);
        ADD_VECTOR_SORT(float4);
        // string
        addExtern<DAS_BIND_FUN(builtin_sort_string)>(*this, lib, "__builtin_sort_string",
            SideEffects::modifyArgumentAndExternal, "builtin_sort_string")
                ->args({"data","length"});
        addExtern<DAS_BIND_FUN(builtin_sort_cblock<char *>)>(*this, lib, "__builtin_sort_cblock",
            SideEffects::modifyArgumentAndExternal, "builtin_sort_cblock<char *>")
                ->args({"array","stride","length","block","context","line"});
        addExtern<DAS_BIND_FUN(builtin_sort_cblock_array<char *>)>(*this, lib, "__builtin_sort_cblock_array",
            SideEffects::modifyArgumentAndExternal, "builtin_sort_array_any_ref_cblock_T")
                ->args({"array","stride","length","block","context","line"})->setAotTemplate();
        addExtern<DAS_BIND_FUN(builtin_sort_cblock<char *>)>(*this, lib, "__builtin_sort_cblock_dim",
            SideEffects::modifyArgumentAndExternal, "builtin_sort_dim_any_ref_cblock_T")
                ->args({"array","stride","length","block","context","line"})->setAotTemplate()->setAnyTemplate();
        // generic sort
        addExtern<DAS_BIND_FUN(builtin_sort_any_cblock)>(*this, lib, "__builtin_sort_any_cblock",
            SideEffects::modifyArgumentAndExternal, "builtin_sort_any_cblock")
                ->args({"array","stride","length","block","context","line"});
        addExtern<DAS_BIND_FUN(builtin_sort_any_ref_cblock)>(*this, lib, "__builtin_sort_any_ref_cblock",
            SideEffects::modifyArgumentAndExternal, "builtin_sort_any_ref_cblock")
                ->args({"array","stride","length","block","context","line"});
        // generic array sort
        addExtern<DAS_BIND_FUN(builtin_sort_array_any_cblock)>(*this, lib, "__builtin_sort_array_any_cblock",
            SideEffects::modifyArgumentAndExternal, "builtin_sort_array_any_cblock_T")
                ->args({"array","stride","length","block","context","line"})->setAotTemplate();
        addExtern<DAS_BIND_FUN(builtin_sort_array_any_ref_cblock)>(*this, lib, "__builtin_sort_array_any_ref_cblock",
            SideEffects::modifyArgumentAndExternal, "builtin_sort_array_any_ref_cblock_T")
                ->args({"array","stride","length","block","context","line"})->setAotTemplate();
        // dim sort
        addExtern<DAS_BIND_FUN(builtin_sort_dim_any_cblock)>(*this, lib, "__builtin_sort_dim_any_cblock",
            SideEffects::modifyArgumentAndExternal, "builtin_sort_dim_any_cblock_T")
                ->args({"array","stride","length","block","context","line"})->setAotTemplate()->setAnyTemplate();
        addExtern<DAS_BIND_FUN(builtin_sort_dim_any_ref_cblock)>(*this, lib, "__builtin_sort_dim_any_ref_cblock",
            SideEffects::modifyArgumentAndExternal, "builtin_sort_dim_any_ref_cblock_T")
                ->args({"array","stride","length","block","context","line"})->setAotTemplate()->setAnyTemplate();

        // stable_sort — any-path only (no numeric/string fast-path; always comparator-based).
        // Interp: byte das_stable_sort_r. AOT: typed das_stable_sort<T> via the _T templates.
        addExtern<DAS_BIND_FUN(builtin_stable_sort_any_cblock)>(*this, lib, "__builtin_stable_sort_any_cblock",
            SideEffects::modifyArgumentAndExternal, "builtin_stable_sort_any_cblock")
                ->args({"array","stride","length","block","context","line"});
        addExtern<DAS_BIND_FUN(builtin_stable_sort_any_ref_cblock)>(*this, lib, "__builtin_stable_sort_any_ref_cblock",
            SideEffects::modifyArgumentAndExternal, "builtin_stable_sort_any_ref_cblock")
                ->args({"array","stride","length","block","context","line"});
        addExtern<DAS_BIND_FUN(builtin_stable_sort_array_any_cblock)>(*this, lib, "__builtin_stable_sort_array_any_cblock",
            SideEffects::modifyArgumentAndExternal, "builtin_stable_sort_array_any_cblock_T")
                ->args({"array","stride","length","block","context","line"})->setAotTemplate();
        addExtern<DAS_BIND_FUN(builtin_stable_sort_array_any_ref_cblock)>(*this, lib, "__builtin_stable_sort_array_any_ref_cblock",
            SideEffects::modifyArgumentAndExternal, "builtin_stable_sort_array_any_ref_cblock_T")
                ->args({"array","stride","length","block","context","line"})->setAotTemplate();
        addExtern<DAS_BIND_FUN(builtin_stable_sort_dim_any_cblock)>(*this, lib, "__builtin_stable_sort_dim_any_cblock",
            SideEffects::modifyArgumentAndExternal, "builtin_stable_sort_dim_any_cblock_T")
                ->args({"array","stride","length","block","context","line"})->setAotTemplate()->setAnyTemplate();
        addExtern<DAS_BIND_FUN(builtin_stable_sort_dim_any_ref_cblock)>(*this, lib, "__builtin_stable_sort_dim_any_ref_cblock",
            SideEffects::modifyArgumentAndExternal, "builtin_stable_sort_dim_any_ref_cblock_T")
                ->args({"array","stride","length","block","context","line"})->setAotTemplate()->setAnyTemplate();

        // ==================================================================
        // Phase 0: partial_sort / nth_element / make_heap / push_heap / pop_heap.
        // Same 19-type matrix as sort (6 numeric + 13 vector). String is omitted
        // for now — __builtin_sort_string is a special path with strcmp
        // comparator; partial_sort / heap ops on strings are not a Phase 0 use
        // case.
        // ==================================================================

        // numeric typed bindings (5 ops × 6 types × 4 variants = 120 registrations)
        STAMP_NUMERIC_OPS(int32_t);
        STAMP_NUMERIC_OPS(uint32_t);
        STAMP_NUMERIC_OPS(int64_t);
        STAMP_NUMERIC_OPS(uint64_t);
        STAMP_NUMERIC_OPS(float);
        STAMP_NUMERIC_OPS(double);
        // vector typed bindings (5 ops × 13 types × 3 variants = 195 registrations)
        STAMP_VECTOR_OPS(range);
        STAMP_VECTOR_OPS(urange);
        STAMP_VECTOR_OPS(range64);
        STAMP_VECTOR_OPS(urange64);
        STAMP_VECTOR_OPS(int2);
        STAMP_VECTOR_OPS(int3);
        STAMP_VECTOR_OPS(int4);
        STAMP_VECTOR_OPS(uint2);
        STAMP_VECTOR_OPS(uint3);
        STAMP_VECTOR_OPS(uint4);
        STAMP_VECTOR_OPS(float2);
        STAMP_VECTOR_OPS(float3);
        STAMP_VECTOR_OPS(float4);

        // generic any-path bindings — 5 ops × 6 variants each, one per op.

        #define REGISTER_ANY_PATH_N(OP) \
            addExtern<DAS_BIND_FUN(builtin_##OP##_any_cblock)>(*this, lib, "__builtin_" #OP "_any_cblock", \
                SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_any_cblock") \
                    ->args({"array","stride","length","n","block","context","line"}); \
            addExtern<DAS_BIND_FUN(builtin_##OP##_any_ref_cblock)>(*this, lib, "__builtin_" #OP "_any_ref_cblock", \
                SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_any_ref_cblock") \
                    ->args({"array","stride","length","n","block","context","line"}); \
            addExtern<DAS_BIND_FUN(builtin_##OP##_array_any_cblock)>(*this, lib, "__builtin_" #OP "_array_any_cblock", \
                SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_array_any_cblock_T") \
                    ->args({"array","stride","length","n","block","context","line"})->setAotTemplate(); \
            addExtern<DAS_BIND_FUN(builtin_##OP##_array_any_ref_cblock)>(*this, lib, "__builtin_" #OP "_array_any_ref_cblock", \
                SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_array_any_ref_cblock_T") \
                    ->args({"array","stride","length","n","block","context","line"})->setAotTemplate(); \
            addExtern<DAS_BIND_FUN(builtin_##OP##_dim_any_cblock)>(*this, lib, "__builtin_" #OP "_dim_any_cblock", \
                SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_dim_any_cblock_T") \
                    ->args({"array","stride","length","n","block","context","line"})->setAotTemplate()->setAnyTemplate(); \
            addExtern<DAS_BIND_FUN(builtin_##OP##_dim_any_ref_cblock)>(*this, lib, "__builtin_" #OP "_dim_any_ref_cblock", \
                SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_dim_any_ref_cblock_T") \
                    ->args({"array","stride","length","n","block","context","line"})->setAotTemplate()->setAnyTemplate();

        #define REGISTER_ANY_PATH(OP) \
            addExtern<DAS_BIND_FUN(builtin_##OP##_any_cblock)>(*this, lib, "__builtin_" #OP "_any_cblock", \
                SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_any_cblock") \
                    ->args({"array","stride","length","block","context","line"}); \
            addExtern<DAS_BIND_FUN(builtin_##OP##_any_ref_cblock)>(*this, lib, "__builtin_" #OP "_any_ref_cblock", \
                SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_any_ref_cblock") \
                    ->args({"array","stride","length","block","context","line"}); \
            addExtern<DAS_BIND_FUN(builtin_##OP##_array_any_cblock)>(*this, lib, "__builtin_" #OP "_array_any_cblock", \
                SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_array_any_cblock_T") \
                    ->args({"array","stride","length","block","context","line"})->setAotTemplate(); \
            addExtern<DAS_BIND_FUN(builtin_##OP##_array_any_ref_cblock)>(*this, lib, "__builtin_" #OP "_array_any_ref_cblock", \
                SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_array_any_ref_cblock_T") \
                    ->args({"array","stride","length","block","context","line"})->setAotTemplate(); \
            addExtern<DAS_BIND_FUN(builtin_##OP##_dim_any_cblock)>(*this, lib, "__builtin_" #OP "_dim_any_cblock", \
                SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_dim_any_cblock_T") \
                    ->args({"array","stride","length","block","context","line"})->setAotTemplate()->setAnyTemplate(); \
            addExtern<DAS_BIND_FUN(builtin_##OP##_dim_any_ref_cblock)>(*this, lib, "__builtin_" #OP "_dim_any_ref_cblock", \
                SideEffects::modifyArgumentAndExternal, "builtin_" #OP "_dim_any_ref_cblock_T") \
                    ->args({"array","stride","length","block","context","line"})->setAotTemplate()->setAnyTemplate();

        REGISTER_ANY_PATH_N(partial_sort)
        REGISTER_ANY_PATH_N(nth_element)
        REGISTER_ANY_PATH(make_heap)
        REGISTER_ANY_PATH(push_heap)
        REGISTER_ANY_PATH(pop_heap)

        #undef REGISTER_ANY_PATH_N
        #undef REGISTER_ANY_PATH
    }
}

