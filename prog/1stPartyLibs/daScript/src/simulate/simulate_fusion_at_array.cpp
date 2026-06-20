#include "daScript/misc/platform.h"

#ifdef _MSC_VER
#pragma warning(disable:4505)
#endif

#include "daScript/simulate/simulate_fusion.h"

#if DAS_FUSION

#include "daScript/simulate/runtime_array.h"
#include "daScript/simulate/sim_policy.h"
#include "daScript/ast/ast.h"
#include "daScript/simulate/simulate_visit_op.h"

namespace das {

    struct SimNode_Op2ArrayAt : SimNode_Op2Fusion {
        virtual SimNode * visit(SimVisitor & vis) override {
            V_BEGIN();
            string name = op;
            name += getSimSourceName(l.type);
            name += getSimSourceName(r.type);
            if (baseType != Type::none) {
                vis.op(name.c_str(), getTypeBaseSize(baseType), das_to_string(baseType));
            } else {
                vis.op(name.c_str());
            }
            l.visit(vis);
            r.visit(vis);
            V_ARG(stride);
            V_ARG(offset);
            V_END();
        }
        uint32_t  stride, offset;
    };

    // int64-indexed parallel base for fused arr[i64] access
    struct SimNode_Op2ArrayAt_I64 : SimNode_Op2ArrayAt {};

    // uint64-indexed parallel base for fused arr[u64] access
    struct SimNode_Op2ArrayAt_U64 : SimNode_Op2ArrayAt {};

/* ArrayAtR2V SCALAR */

#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_Op2ArrayAt { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = (Array *) l.compute##COMPUTEL(context); \
            int32_t rr = r.subexpr->evalInt(context); \
            if ( rr<0 || uint64_t(rr) >= pl->size ) context.throw_error_at(debugInfo,"array index out of range, %d of %llu", rr, (unsigned long long)pl->size); \
            return *((CTYPE *)(pl->data + uint64_t(rr)*uint64_t(stride) + offset)); \
        } \
        DAS_NODE(TYPE,CTYPE); \
    };

#define IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2ArrayAt { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = (Array *) l.compute##COMPUTEL(context); \
            int32_t rr = *((int32_t *)r.compute##COMPUTER(context)); \
            if ( rr<0 || uint64_t(rr) >= pl->size ) context.throw_error_at(debugInfo,"array index out of range, %d of %llu", rr, (unsigned long long)pl->size); \
            return *((CTYPE *)(pl->data + uint64_t(rr)*uint64_t(stride) + offset)); \
        } \
        DAS_NODE(TYPE,CTYPE); \
    };

#define IMPLEMENT_OP2_SET_SETUP_NODE(result,node) \
    auto rn = (SimNode_Op2ArrayAt *)result; \
    auto sn = (SimNode_ArrayAt *)node; \
    rn->stride = sn->stride; \
    rn->offset = sn->offset;

#define FUSION_OP2_SUBEXPR_LEFT(CTYPE,node)     ((static_cast<SimNode_ArrayAt *>(node))->l)
#define FUSION_OP2_SUBEXPR_RIGHT(CTYPE,node)    ((static_cast<SimNode_ArrayAt *>(node))->r)

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

    IMPLEMENT_SETOP_SCALAR(ArrayAtR2V);

/* ArrayAtR2V VECTOR */

#undef IMPLEMENT_OP2_SET_NODE_ANY
#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_Op2ArrayAt { \
         DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto pl = (Array *) l.compute##COMPUTEL(context); \
            int32_t rr = r.subexpr->evalInt(context); \
            if ( rr<0 || uint64_t(rr) >= pl->size ) context.throw_error_at(debugInfo,"array index out of range, %d of %llu", rr, (unsigned long long)pl->size); \
            vec4f __r; \
            DAS_LDU_WORKHORSE(__r, pl->data + uint64_t(rr)*uint64_t(stride) + offset, CTYPE); \
            return __r; \
        } \
    };

#undef IMPLEMENT_OP2_SET_NODE
#define IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2ArrayAt { \
         DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto pl = (Array *) l.compute##COMPUTEL(context); \
            int32_t rr = *((int32_t *)r.compute##COMPUTER(context)); \
            if ( rr<0 || uint64_t(rr) >= pl->size ) context.throw_error_at(debugInfo,"array index out of range, %d of %llu", rr, (unsigned long long)pl->size); \
            vec4f __r; \
            DAS_LDU_WORKHORSE(__r, pl->data + uint64_t(rr)*uint64_t(stride) + offset, CTYPE); \
            return __r; \
        } \
    };

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

    IMPLEMENT_SETOP_NUMERIC_VEC(ArrayAtR2V);

/* ArrayAt */

#undef IMPLEMENT_OP2_SET_NODE_ANY
#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_Op2ArrayAt { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = (Array *) l.compute##COMPUTEL(context); \
            int32_t rr = r.subexpr->evalInt(context); \
            if ( rr<0 || uint64_t(rr) >= pl->size ) context.throw_error_at(debugInfo,"array index out of range, %d of %llu", rr, (unsigned long long)pl->size); \
            return pl->data + uint64_t(rr)*uint64_t(stride) + offset; \
        } \
        DAS_PTR_NODE; \
    };

#undef IMPLEMENT_OP2_SET_NODE
#define IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2ArrayAt { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = (Array *) l.compute##COMPUTEL(context); \
            int32_t rr = *((int32_t *)r.compute##COMPUTER(context)); \
            if ( rr<0 || uint64_t(rr) >= pl->size ) context.throw_error_at(debugInfo,"array index out of range, %d of %llu", rr, (unsigned long long)pl->size); \
            return pl->data + uint64_t(rr)*uint64_t(stride) + offset; \
        } \
        DAS_PTR_NODE; \
    };

#undef IMPLEMENT_OP2_SET_SETUP_NODE
#define IMPLEMENT_OP2_SET_SETUP_NODE(result,node) \
    auto rn = (SimNode_Op2ArrayAt *)result; \
    auto sn = (SimNode_ArrayAt *)node; \
    rn->stride = sn->stride; \
    rn->offset = sn->offset; \
    rn->baseType = Type::none;

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

    IMPLEMENT_ANY_SETOP(__forceinline, ArrayAt, Ptr, StringPtr, StringPtr);

/* ArrayAtR2V_I64 SCALAR */

#undef IMPLEMENT_OP2_SET_NODE_ANY
#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_Op2ArrayAt_I64 { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = (Array *) l.compute##COMPUTEL(context); \
            int64_t rr = r.subexpr->evalInt64(context); \
            if ( rr<0 || uint64_t(rr) >= pl->size ) context.throw_error_at(debugInfo,"array index out of range, %lld of %llu", (long long)rr, (unsigned long long)pl->size); \
            return *((CTYPE *)(pl->data + uint64_t(rr)*uint64_t(stride) + offset)); \
        } \
        DAS_NODE(TYPE,CTYPE); \
    };

#undef IMPLEMENT_OP2_SET_NODE
#define IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2ArrayAt_I64 { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = (Array *) l.compute##COMPUTEL(context); \
            int64_t rr = *((int64_t *)r.compute##COMPUTER(context)); \
            if ( rr<0 || uint64_t(rr) >= pl->size ) context.throw_error_at(debugInfo,"array index out of range, %lld of %llu", (long long)rr, (unsigned long long)pl->size); \
            return *((CTYPE *)(pl->data + uint64_t(rr)*uint64_t(stride) + offset)); \
        } \
        DAS_NODE(TYPE,CTYPE); \
    };

#undef IMPLEMENT_OP2_SET_SETUP_NODE
#define IMPLEMENT_OP2_SET_SETUP_NODE(result,node) \
    auto rn = (SimNode_Op2ArrayAt_I64 *)result; \
    auto sn = (SimNode_ArrayAt_I64 *)node; \
    rn->stride = sn->stride; \
    rn->offset = sn->offset;

#undef FUSION_OP2_SUBEXPR_LEFT
#undef FUSION_OP2_SUBEXPR_RIGHT
#define FUSION_OP2_SUBEXPR_LEFT(CTYPE,node)     ((static_cast<SimNode_ArrayAt_I64 *>(node))->l)
#define FUSION_OP2_SUBEXPR_RIGHT(CTYPE,node)    ((static_cast<SimNode_ArrayAt_I64 *>(node))->r)

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

    IMPLEMENT_SETOP_SCALAR(ArrayAtR2V_I64);

/* ArrayAtR2V_I64 VECTOR */

#undef IMPLEMENT_OP2_SET_NODE_ANY
#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_Op2ArrayAt_I64 { \
         DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto pl = (Array *) l.compute##COMPUTEL(context); \
            int64_t rr = r.subexpr->evalInt64(context); \
            if ( rr<0 || uint64_t(rr) >= pl->size ) context.throw_error_at(debugInfo,"array index out of range, %lld of %llu", (long long)rr, (unsigned long long)pl->size); \
            vec4f __r; \
            DAS_LDU_WORKHORSE(__r, pl->data + uint64_t(rr)*uint64_t(stride) + offset, CTYPE); \
            return __r; \
        } \
    };

#undef IMPLEMENT_OP2_SET_NODE
#define IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2ArrayAt_I64 { \
         DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto pl = (Array *) l.compute##COMPUTEL(context); \
            int64_t rr = *((int64_t *)r.compute##COMPUTER(context)); \
            if ( rr<0 || uint64_t(rr) >= pl->size ) context.throw_error_at(debugInfo,"array index out of range, %lld of %llu", (long long)rr, (unsigned long long)pl->size); \
            vec4f __r; \
            DAS_LDU_WORKHORSE(__r, pl->data + uint64_t(rr)*uint64_t(stride) + offset, CTYPE); \
            return __r; \
        } \
    };

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

    IMPLEMENT_SETOP_NUMERIC_VEC(ArrayAtR2V_I64);

/* ArrayAt_I64 */

#undef IMPLEMENT_OP2_SET_NODE_ANY
#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_Op2ArrayAt_I64 { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = (Array *) l.compute##COMPUTEL(context); \
            int64_t rr = r.subexpr->evalInt64(context); \
            if ( rr<0 || uint64_t(rr) >= pl->size ) context.throw_error_at(debugInfo,"array index out of range, %lld of %llu", (long long)rr, (unsigned long long)pl->size); \
            return pl->data + uint64_t(rr)*uint64_t(stride) + offset; \
        } \
        DAS_PTR_NODE; \
    };

#undef IMPLEMENT_OP2_SET_NODE
#define IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2ArrayAt_I64 { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = (Array *) l.compute##COMPUTEL(context); \
            int64_t rr = *((int64_t *)r.compute##COMPUTER(context)); \
            if ( rr<0 || uint64_t(rr) >= pl->size ) context.throw_error_at(debugInfo,"array index out of range, %lld of %llu", (long long)rr, (unsigned long long)pl->size); \
            return pl->data + uint64_t(rr)*uint64_t(stride) + offset; \
        } \
        DAS_PTR_NODE; \
    };

#undef IMPLEMENT_OP2_SET_SETUP_NODE
#define IMPLEMENT_OP2_SET_SETUP_NODE(result,node) \
    auto rn = (SimNode_Op2ArrayAt_I64 *)result; \
    auto sn = (SimNode_ArrayAt_I64 *)node; \
    rn->stride = sn->stride; \
    rn->offset = sn->offset; \
    rn->baseType = Type::none;

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

    IMPLEMENT_ANY_SETOP(__forceinline, ArrayAt_I64, Ptr, StringPtr, StringPtr);

/* ArrayAtR2V_U64 SCALAR */

#undef IMPLEMENT_OP2_SET_NODE_ANY
#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_Op2ArrayAt_U64 { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = (Array *) l.compute##COMPUTEL(context); \
            uint64_t rr = r.subexpr->evalUInt64(context); \
            if ( rr >= pl->size ) context.throw_error_at(debugInfo,"array index out of range, %llu of %llu", (unsigned long long)rr, (unsigned long long)pl->size); \
            return *((CTYPE *)(pl->data + rr*uint64_t(stride) + offset)); \
        } \
        DAS_NODE(TYPE,CTYPE); \
    };

#undef IMPLEMENT_OP2_SET_NODE
#define IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2ArrayAt_U64 { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = (Array *) l.compute##COMPUTEL(context); \
            uint64_t rr = *((uint64_t *)r.compute##COMPUTER(context)); \
            if ( rr >= pl->size ) context.throw_error_at(debugInfo,"array index out of range, %llu of %llu", (unsigned long long)rr, (unsigned long long)pl->size); \
            return *((CTYPE *)(pl->data + rr*uint64_t(stride) + offset)); \
        } \
        DAS_NODE(TYPE,CTYPE); \
    };

#undef IMPLEMENT_OP2_SET_SETUP_NODE
#define IMPLEMENT_OP2_SET_SETUP_NODE(result,node) \
    auto rn = (SimNode_Op2ArrayAt_U64 *)result; \
    auto sn = (SimNode_ArrayAt_U64 *)node; \
    rn->stride = sn->stride; \
    rn->offset = sn->offset;

#undef FUSION_OP2_SUBEXPR_LEFT
#undef FUSION_OP2_SUBEXPR_RIGHT
#define FUSION_OP2_SUBEXPR_LEFT(CTYPE,node)     ((static_cast<SimNode_ArrayAt_U64 *>(node))->l)
#define FUSION_OP2_SUBEXPR_RIGHT(CTYPE,node)    ((static_cast<SimNode_ArrayAt_U64 *>(node))->r)

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

    IMPLEMENT_SETOP_SCALAR(ArrayAtR2V_U64);

/* ArrayAtR2V_U64 VECTOR */

#undef IMPLEMENT_OP2_SET_NODE_ANY
#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_Op2ArrayAt_U64 { \
         DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto pl = (Array *) l.compute##COMPUTEL(context); \
            uint64_t rr = r.subexpr->evalUInt64(context); \
            if ( rr >= pl->size ) context.throw_error_at(debugInfo,"array index out of range, %llu of %llu", (unsigned long long)rr, (unsigned long long)pl->size); \
            vec4f __r; \
            DAS_LDU_WORKHORSE(__r, pl->data + rr*uint64_t(stride) + offset, CTYPE); \
            return __r; \
        } \
    };

#undef IMPLEMENT_OP2_SET_NODE
#define IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2ArrayAt_U64 { \
         DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto pl = (Array *) l.compute##COMPUTEL(context); \
            uint64_t rr = *((uint64_t *)r.compute##COMPUTER(context)); \
            if ( rr >= pl->size ) context.throw_error_at(debugInfo,"array index out of range, %llu of %llu", (unsigned long long)rr, (unsigned long long)pl->size); \
            vec4f __r; \
            DAS_LDU_WORKHORSE(__r, pl->data + rr*uint64_t(stride) + offset, CTYPE); \
            return __r; \
        } \
    };

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

    IMPLEMENT_SETOP_NUMERIC_VEC(ArrayAtR2V_U64);

/* ArrayAt_U64 */

#undef IMPLEMENT_OP2_SET_NODE_ANY
#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_Op2ArrayAt_U64 { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = (Array *) l.compute##COMPUTEL(context); \
            uint64_t rr = r.subexpr->evalUInt64(context); \
            if ( rr >= pl->size ) context.throw_error_at(debugInfo,"array index out of range, %llu of %llu", (unsigned long long)rr, (unsigned long long)pl->size); \
            return pl->data + rr*uint64_t(stride) + offset; \
        } \
        DAS_PTR_NODE; \
    };

#undef IMPLEMENT_OP2_SET_NODE
#define IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2ArrayAt_U64 { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = (Array *) l.compute##COMPUTEL(context); \
            uint64_t rr = *((uint64_t *)r.compute##COMPUTER(context)); \
            if ( rr >= pl->size ) context.throw_error_at(debugInfo,"array index out of range, %llu of %llu", (unsigned long long)rr, (unsigned long long)pl->size); \
            return pl->data + rr*uint64_t(stride) + offset; \
        } \
        DAS_PTR_NODE; \
    };

#undef IMPLEMENT_OP2_SET_SETUP_NODE
#define IMPLEMENT_OP2_SET_SETUP_NODE(result,node) \
    auto rn = (SimNode_Op2ArrayAt_U64 *)result; \
    auto sn = (SimNode_ArrayAt_U64 *)node; \
    rn->stride = sn->stride; \
    rn->offset = sn->offset; \
    rn->baseType = Type::none;

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

    IMPLEMENT_ANY_SETOP(__forceinline, ArrayAt_U64, Ptr, StringPtr, StringPtr);

    void createFusionEngine_at_array() {
        REGISTER_SETOP_SCALAR(ArrayAtR2V);
        REGISTER_SETOP_NUMERIC_VEC(ArrayAtR2V);
        (*getFusionEngine())["ArrayAt"].emplace_back(new FusionPoint_Set_ArrayAt_StringPtr());
        REGISTER_SETOP_SCALAR(ArrayAtR2V_I64);
        REGISTER_SETOP_NUMERIC_VEC(ArrayAtR2V_I64);
        (*getFusionEngine())["ArrayAt_I64"].emplace_back(new FusionPoint_Set_ArrayAt_I64_StringPtr());
        REGISTER_SETOP_SCALAR(ArrayAtR2V_U64);
        REGISTER_SETOP_NUMERIC_VEC(ArrayAtR2V_U64);
        (*getFusionEngine())["ArrayAt_U64"].emplace_back(new FusionPoint_Set_ArrayAt_U64_StringPtr());
    }
}

#endif

