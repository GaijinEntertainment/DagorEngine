#include "daScript/misc/platform.h"

#include "module_builtin.h"

#include "daScript/ast/ast_interop.h"
#include "daScript/simulate/aot_builtin.h"
#include "daScript/simulate/sim_policy.h"
#include "das_qsort_r.h"

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

    void builtin_sort_array_any_cblock ( Array & arr, int32_t elementSize, int32_t, const Block & cmp, Context * context, LineInfoArg * at ) {
        array_lock(*context,arr,at);
        builtin_sort_any_cblock(arr.data, elementSize, arr.size, cmp, context, at);
        array_unlock(*context,arr,at);
    }

    void builtin_sort_array_any_ref_cblock ( Array & arr, int32_t elementSize, int32_t, const Block & cmp, Context * context, LineInfoArg * at ) {
        array_lock(*context,arr,at);
        builtin_sort_any_ref_cblock(arr.data, elementSize, arr.size, cmp, context, at);
        array_unlock(*context,arr,at);
    }

    void builtin_sort_string ( void * data, int32_t length ) {
        if ( length<=1 ) return;
        const char ** pdata = (const char **) data;
        sort ( pdata, pdata + length, [&](const char * a, const char * b){
            return strcmp(to_rts(a), to_rts(b))<0;
        });
    }

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

    // this one replaces sort(array<>) via correct builtin
    struct BuiltinSortFunctionAnnotation : FunctionAnnotation {
        BuiltinSortFunctionAnnotation() : FunctionAnnotation("builtin_array_sort") { }
        virtual bool apply ( const FunctionPtr &, ModuleGroup &, const AnnotationArgumentList &, string & ) override { return true; }
        virtual bool finalize ( const FunctionPtr &, ModuleGroup &, const AnnotationArgumentList &, const AnnotationArgumentList &, string & ) override { return true; }
        virtual bool apply ( ExprBlock *, ModuleGroup &, const AnnotationArgumentList &, string & ) override { return false; }
        virtual bool finalize ( ExprBlock *, ModuleGroup &, const AnnotationArgumentList &, const AnnotationArgumentList &, string & ) override { return false; }
        virtual ExpressionPtr transformCall ( ExprCallFunc * call, string & err ) override {
            for ( auto & arg : call->arguments ) {
                if ( !arg->type || !arg->type->isFullyInferred() ) {
                    err = "sort requires fully inferred arguments";
                    return nullptr;
                }
            }
            const auto & arg = call->arguments[0];
            if ( arg->type->dim.size() ) {
                auto arrType = make_smart<TypeDecl>(*arg->type);
                arrType->dim.clear();
                auto newCall = static_pointer_cast<ExprCall>(call->clone());
                auto stride = arrType->getSizeOf();
                auto strideArg = make_smart<ExprConstInt>(call->at, stride);
                strideArg->generated = true;
                newCall->arguments.insert(newCall->arguments.begin()+1,strideArg);
                auto length = arg->type->dim.back();
                auto lengthArg = make_smart<ExprConstInt>(call->at, length);
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
                auto newCall = static_pointer_cast<ExprCall>(call->clone());
                auto stride = arrType->getSizeOf();
                auto strideArg = make_smart<ExprConstInt>(call->at, stride);
                strideArg->generated = true;
                newCall->arguments.insert(newCall->arguments.begin()+1,strideArg);
                auto lengthArg = make_smart<ExprConstInt>(call->at, 0);
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
        addAnnotation(make_smart<BuiltinSortFunctionAnnotation>());
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
    }
}

