#include "daScript/misc/platform.h"

#ifdef _MSC_VER
#pragma warning(disable:4505)
#endif

#include "daScript/simulate/simulate_fusion.h"

#if DAS_FUSION

#include "daScript/simulate/sim_policy.h"
#include "daScript/ast/ast.h"
#include "daScript/simulate/simulate_visit_op.h"

namespace das {

    struct SimNode_Op2At : SimNode_Op2Fusion {
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
            V_ARG(range);
            V_END();
        }
        uint32_t  stride, offset, range;
    };

    // the bounds check, toggled per node family: the checked op-name (At/AtR2V) instantiates with it
    // ON, the unchecked op-name (AtU/AtR2VU, emitted only where the index is proven in range by the
    // bound-check elision pass) with it OFF - so both families share one node-body macro.
#define DAS_AT_CHECK_ON(rr,rng)  if ( rr<0 || uint32_t(rr) >= rng ) context.throw_error_at(debugInfo,"index out of range, %d of %u%s", rr, rng, errorMessage)
#define DAS_AT_CHECK_OFF(rr,rng)

/* AtR2V SCALAR */

#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_Op2At { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            int32_t rr = r.subexpr->evalInt(context); \
            DAS_AT_CHECK(rr, range); \
            return *((CTYPE *)(pl + rr*stride + offset)); \
        } \
        DAS_NODE(TYPE,CTYPE); \
    };

#define IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2At { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            int32_t rr = *((int32_t *)r.compute##COMPUTER(context)); \
            DAS_AT_CHECK(rr, range); \
            return *((CTYPE *)(pl + rr*stride + offset)); \
        } \
        DAS_NODE(TYPE,CTYPE); \
    };

#define IMPLEMENT_OP2_SET_SETUP_NODE(result,node) \
    auto rn = (SimNode_Op2At *)result; \
    auto sn = (SimNode_At *)node; \
    rn->errorMessage = sn->errorMessage; \
    rn->stride = sn->stride; \
    rn->offset = sn->offset; \
    rn->range = sn->range;

#define FUSION_OP2_SUBEXPR_LEFT(CTYPE,node)     ((static_cast<SimNode_At *>(node))->value)
#define FUSION_OP2_SUBEXPR_RIGHT(CTYPE,node)    ((static_cast<SimNode_At *>(node))->index)

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

#define DAS_AT_CHECK DAS_AT_CHECK_ON
    IMPLEMENT_SETOP_SCALAR(AtR2V);
#undef DAS_AT_CHECK
#define DAS_AT_CHECK DAS_AT_CHECK_OFF
    IMPLEMENT_SETOP_SCALAR(AtR2VU);
#undef DAS_AT_CHECK

/* AtR2V VECTOR */

#undef IMPLEMENT_OP2_SET_NODE_ANY
#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_Op2At { \
         DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            int32_t rr = r.subexpr->evalInt(context); \
            DAS_AT_CHECK(rr, range); \
            vec4f __r; \
            DAS_LDU_WORKHORSE(__r, pl + rr*stride + offset, CTYPE); \
            return __r; \
        } \
    };

#undef IMPLEMENT_OP2_SET_NODE
#define IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2At { \
         DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            int32_t rr = *((int32_t *)r.compute##COMPUTER(context)); \
            DAS_AT_CHECK(rr, range); \
            vec4f __r; \
            DAS_LDU_WORKHORSE(__r, pl + rr*stride + offset, CTYPE); \
            return __r; \
        } \
    };

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

#define DAS_AT_CHECK DAS_AT_CHECK_ON
    IMPLEMENT_SETOP_NUMERIC_VEC(AtR2V);
#undef DAS_AT_CHECK
#define DAS_AT_CHECK DAS_AT_CHECK_OFF
    IMPLEMENT_SETOP_NUMERIC_VEC(AtR2VU);
#undef DAS_AT_CHECK

/* At */

#undef IMPLEMENT_OP2_SET_NODE_ANY
#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_Op2At { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            int32_t rr = r.subexpr->evalInt(context); \
            DAS_AT_CHECK(rr, range); \
            return pl + rr*stride + offset; \
        } \
        DAS_PTR_NODE; \
    };

#undef IMPLEMENT_OP2_SET_NODE
#define IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2At { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            int32_t rr = *((int32_t *)r.compute##COMPUTER(context)); \
            DAS_AT_CHECK(rr, range); \
            return pl + rr*stride + offset; \
        } \
        DAS_PTR_NODE; \
    };

#undef IMPLEMENT_OP2_SET_SETUP_NODE
#define IMPLEMENT_OP2_SET_SETUP_NODE(result,node) \
    auto rn = (SimNode_Op2At *)result; \
    auto sn = (SimNode_At *)node; \
    rn->errorMessage = sn->errorMessage; \
    rn->stride = sn->stride; \
    rn->offset = sn->offset; \
    rn->range = sn->range; \
    rn->baseType = Type::none;

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

#define DAS_AT_CHECK DAS_AT_CHECK_ON
    IMPLEMENT_ANY_SETOP(__forceinline, At, Ptr, StringPtr, StringPtr);
    IMPLEMENT_ANY_SETOP(__forceinline, At, Ptr, VoidPtr, StringPtr);
#undef DAS_AT_CHECK
#define DAS_AT_CHECK DAS_AT_CHECK_OFF
    IMPLEMENT_ANY_SETOP(__forceinline, AtU, Ptr, StringPtr, StringPtr);
    IMPLEMENT_ANY_SETOP(__forceinline, AtU, Ptr, VoidPtr, StringPtr);
#undef DAS_AT_CHECK

    struct SimNode_Op2At_I64 : SimNode_Op2At {};
    struct SimNode_Op2At_U64 : SimNode_Op2At {};

/* AtR2V_I64 SCALAR */

#undef IMPLEMENT_OP2_SET_NODE_ANY
#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_Op2At_I64 { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            int64_t rr = r.subexpr->evalInt64(context); \
            if ( rr<0 || uint64_t(rr) >= uint64_t(range) ) context.throw_error_at(debugInfo,"index out of range, %lld of %u%s", (long long)rr, range, errorMessage); \
            return *((CTYPE *)(pl + uint32_t(rr)*stride + offset)); \
        } \
        DAS_NODE(TYPE,CTYPE); \
    };

#undef IMPLEMENT_OP2_SET_NODE
#define IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2At_I64 { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            int64_t rr = *((int64_t *)r.compute##COMPUTER(context)); \
            if ( rr<0 || uint64_t(rr) >= uint64_t(range) ) context.throw_error_at(debugInfo,"index out of range, %lld of %u%s", (long long)rr, range, errorMessage); \
            return *((CTYPE *)(pl + uint32_t(rr)*stride + offset)); \
        } \
        DAS_NODE(TYPE,CTYPE); \
    };

#undef IMPLEMENT_OP2_SET_SETUP_NODE
#define IMPLEMENT_OP2_SET_SETUP_NODE(result,node) \
    auto rn = (SimNode_Op2At_I64 *)result; \
    auto sn = (SimNode_At_I64 *)node; \
    rn->errorMessage = sn->errorMessage; \
    rn->stride = sn->stride; \
    rn->offset = sn->offset; \
    rn->range = sn->range;

#undef FUSION_OP2_SUBEXPR_LEFT
#undef FUSION_OP2_SUBEXPR_RIGHT
#define FUSION_OP2_SUBEXPR_LEFT(CTYPE,node)     ((static_cast<SimNode_At_I64 *>(node))->value)
#define FUSION_OP2_SUBEXPR_RIGHT(CTYPE,node)    ((static_cast<SimNode_At_I64 *>(node))->index)

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

    IMPLEMENT_SETOP_SCALAR(AtR2V_I64);

/* AtR2V_I64 VECTOR */

#undef IMPLEMENT_OP2_SET_NODE_ANY
#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_Op2At_I64 { \
         DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            int64_t rr = r.subexpr->evalInt64(context); \
            if ( rr<0 || uint64_t(rr) >= uint64_t(range) ) context.throw_error_at(debugInfo,"index out of range, %lld of %u%s", (long long)rr, range, errorMessage); \
            vec4f __r; \
            DAS_LDU_WORKHORSE(__r, pl + uint32_t(rr)*stride + offset, CTYPE); \
            return __r; \
        } \
    };

#undef IMPLEMENT_OP2_SET_NODE
#define IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2At_I64 { \
         DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            int64_t rr = *((int64_t *)r.compute##COMPUTER(context)); \
            if ( rr<0 || uint64_t(rr) >= uint64_t(range) ) context.throw_error_at(debugInfo,"index out of range, %lld of %u%s", (long long)rr, range, errorMessage); \
            vec4f __r; \
            DAS_LDU_WORKHORSE(__r, pl + uint32_t(rr)*stride + offset, CTYPE); \
            return __r; \
        } \
    };

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

    IMPLEMENT_SETOP_NUMERIC_VEC(AtR2V_I64);

/* At_I64 */

#undef IMPLEMENT_OP2_SET_NODE_ANY
#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_Op2At_I64 { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            int64_t rr = r.subexpr->evalInt64(context); \
            if ( rr<0 || uint64_t(rr) >= uint64_t(range) ) context.throw_error_at(debugInfo,"index out of range, %lld of %u%s", (long long)rr, range, errorMessage); \
            return pl + uint32_t(rr)*stride + offset; \
        } \
        DAS_PTR_NODE; \
    };

#undef IMPLEMENT_OP2_SET_NODE
#define IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2At_I64 { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            int64_t rr = *((int64_t *)r.compute##COMPUTER(context)); \
            if ( rr<0 || uint64_t(rr) >= uint64_t(range) ) context.throw_error_at(debugInfo,"index out of range, %lld of %u%s", (long long)rr, range, errorMessage); \
            return pl + uint32_t(rr)*stride + offset; \
        } \
        DAS_PTR_NODE; \
    };

#undef IMPLEMENT_OP2_SET_SETUP_NODE
#define IMPLEMENT_OP2_SET_SETUP_NODE(result,node) \
    auto rn = (SimNode_Op2At_I64 *)result; \
    auto sn = (SimNode_At_I64 *)node; \
    rn->errorMessage = sn->errorMessage; \
    rn->stride = sn->stride; \
    rn->offset = sn->offset; \
    rn->range = sn->range; \
    rn->baseType = Type::none;

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

    IMPLEMENT_ANY_SETOP(__forceinline, At_I64, Ptr, StringPtr, StringPtr);
    IMPLEMENT_ANY_SETOP(__forceinline, At_I64, Ptr, VoidPtr, StringPtr);

/* AtR2V_U64 SCALAR */

#undef IMPLEMENT_OP2_SET_NODE_ANY
#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_Op2At_U64 { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            uint64_t rr = r.subexpr->evalUInt64(context); \
            if ( rr >= uint64_t(range) ) context.throw_error_at(debugInfo,"index out of range, %llu of %u%s", (unsigned long long)rr, range, errorMessage); \
            return *((CTYPE *)(pl + uint32_t(rr)*stride + offset)); \
        } \
        DAS_NODE(TYPE,CTYPE); \
    };

#undef IMPLEMENT_OP2_SET_NODE
#define IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2At_U64 { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            uint64_t rr = *((uint64_t *)r.compute##COMPUTER(context)); \
            if ( rr >= uint64_t(range) ) context.throw_error_at(debugInfo,"index out of range, %llu of %u%s", (unsigned long long)rr, range, errorMessage); \
            return *((CTYPE *)(pl + uint32_t(rr)*stride + offset)); \
        } \
        DAS_NODE(TYPE,CTYPE); \
    };

#undef IMPLEMENT_OP2_SET_SETUP_NODE
#define IMPLEMENT_OP2_SET_SETUP_NODE(result,node) \
    auto rn = (SimNode_Op2At_U64 *)result; \
    auto sn = (SimNode_At_U64 *)node; \
    rn->errorMessage = sn->errorMessage; \
    rn->stride = sn->stride; \
    rn->offset = sn->offset; \
    rn->range = sn->range;

#undef FUSION_OP2_SUBEXPR_LEFT
#undef FUSION_OP2_SUBEXPR_RIGHT
#define FUSION_OP2_SUBEXPR_LEFT(CTYPE,node)     ((static_cast<SimNode_At_U64 *>(node))->value)
#define FUSION_OP2_SUBEXPR_RIGHT(CTYPE,node)    ((static_cast<SimNode_At_U64 *>(node))->index)

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

    IMPLEMENT_SETOP_SCALAR(AtR2V_U64);

/* AtR2V_U64 VECTOR */

#undef IMPLEMENT_OP2_SET_NODE_ANY
#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_Op2At_U64 { \
         DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            uint64_t rr = r.subexpr->evalUInt64(context); \
            if ( rr >= uint64_t(range) ) context.throw_error_at(debugInfo,"index out of range, %llu of %u%s", (unsigned long long)rr, range, errorMessage); \
            vec4f __r; \
            DAS_LDU_WORKHORSE(__r, pl + uint32_t(rr)*stride + offset, CTYPE); \
            return __r; \
        } \
    };

#undef IMPLEMENT_OP2_SET_NODE
#define IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2At_U64 { \
         DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            uint64_t rr = *((uint64_t *)r.compute##COMPUTER(context)); \
            if ( rr >= uint64_t(range) ) context.throw_error_at(debugInfo,"index out of range, %llu of %u%s", (unsigned long long)rr, range, errorMessage); \
            vec4f __r; \
            DAS_LDU_WORKHORSE(__r, pl + uint32_t(rr)*stride + offset, CTYPE); \
            return __r; \
        } \
    };

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

    IMPLEMENT_SETOP_NUMERIC_VEC(AtR2V_U64);

/* At_U64 */

#undef IMPLEMENT_OP2_SET_NODE_ANY
#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_Op2At_U64 { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            uint64_t rr = r.subexpr->evalUInt64(context); \
            if ( rr >= uint64_t(range) ) context.throw_error_at(debugInfo,"index out of range, %llu of %u%s", (unsigned long long)rr, range, errorMessage); \
            return pl + uint32_t(rr)*stride + offset; \
        } \
        DAS_PTR_NODE; \
    };

#undef IMPLEMENT_OP2_SET_NODE
#define IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2At_U64 { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            uint64_t rr = *((uint64_t *)r.compute##COMPUTER(context)); \
            if ( rr >= uint64_t(range) ) context.throw_error_at(debugInfo,"index out of range, %llu of %u%s", (unsigned long long)rr, range, errorMessage); \
            return pl + uint32_t(rr)*stride + offset; \
        } \
        DAS_PTR_NODE; \
    };

#undef IMPLEMENT_OP2_SET_SETUP_NODE
#define IMPLEMENT_OP2_SET_SETUP_NODE(result,node) \
    auto rn = (SimNode_Op2At_U64 *)result; \
    auto sn = (SimNode_At_U64 *)node; \
    rn->errorMessage = sn->errorMessage; \
    rn->stride = sn->stride; \
    rn->offset = sn->offset; \
    rn->range = sn->range; \
    rn->baseType = Type::none;

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

    IMPLEMENT_ANY_SETOP(__forceinline, At_U64, Ptr, StringPtr, StringPtr);
    IMPLEMENT_ANY_SETOP(__forceinline, At_U64, Ptr, VoidPtr, StringPtr);

    void createFusionEngine_at() {
        REGISTER_SETOP_SCALAR(AtR2V);
        REGISTER_SETOP_NUMERIC_VEC(AtR2V);
        (*getFusionEngine())["At"].emplace_back(new FusionPoint_Set_At_StringPtr());
        (*getFusionEngine())["At"].emplace_back(new FusionPoint_Set_At_VoidPtr());
        REGISTER_SETOP_SCALAR(AtR2V_I64);
        REGISTER_SETOP_NUMERIC_VEC(AtR2V_I64);
        (*getFusionEngine())["At_I64"].emplace_back(new FusionPoint_Set_At_I64_StringPtr());
        (*getFusionEngine())["At_I64"].emplace_back(new FusionPoint_Set_At_I64_VoidPtr());
        REGISTER_SETOP_SCALAR(AtR2V_U64);
        REGISTER_SETOP_NUMERIC_VEC(AtR2V_U64);
        (*getFusionEngine())["At_U64"].emplace_back(new FusionPoint_Set_At_U64_StringPtr());
        (*getFusionEngine())["At_U64"].emplace_back(new FusionPoint_Set_At_U64_VoidPtr());
        REGISTER_SETOP_SCALAR(AtR2VU);
        REGISTER_SETOP_NUMERIC_VEC(AtR2VU);
        (*getFusionEngine())["AtU"].emplace_back(new FusionPoint_Set_AtU_StringPtr());
        (*getFusionEngine())["AtU"].emplace_back(new FusionPoint_Set_AtU_VoidPtr());
    }
}

#endif

