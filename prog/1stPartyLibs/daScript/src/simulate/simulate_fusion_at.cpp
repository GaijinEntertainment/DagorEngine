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

/* AtR2V SCALAR */

#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_Op2At { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            auto rr = uint32_t(r.subexpr->evalInt(context)); \
            if ( rr >= range ) context.throw_error_at(debugInfo,"index out of range, %u of %u", rr, range); \
            return *((CTYPE *)(pl + rr*stride + offset)); \
        } \
        DAS_NODE(TYPE,CTYPE); \
    };

#define IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2At { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            auto rr = *((uint32_t *)r.compute##COMPUTER(context)); \
            if ( rr >= range ) context.throw_error_at(debugInfo,"index out of range, %u of %u", rr, range); \
            return *((CTYPE *)(pl + rr*stride + offset)); \
        } \
        DAS_NODE(TYPE,CTYPE); \
    };

#define IMPLEMENT_OP2_SET_SETUP_NODE(result,node) \
    auto rn = (SimNode_Op2At *)result; \
    auto sn = (SimNode_At *)node; \
    rn->stride = sn->stride; \
    rn->offset = sn->offset; \
    rn->range = sn->range;

#define FUSION_OP2_SUBEXPR_LEFT(CTYPE,node)     ((static_cast<SimNode_At *>(node))->value)
#define FUSION_OP2_SUBEXPR_RIGHT(CTYPE,node)    ((static_cast<SimNode_At *>(node))->index)

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

    IMPLEMENT_SETOP_SCALAR(AtR2V);

/* AtR2V VECTOR */

#undef IMPLEMENT_OP2_SET_NODE_ANY
#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_Op2At { \
         DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            auto rr = uint32_t(r.subexpr->evalInt(context)); \
            if ( rr >= range ) context.throw_error_at(debugInfo,"index out of range, %u of %u", rr, range); \
            return v_ldu((const float *)(pl + rr*stride + offset)); \
        } \
    };

#undef IMPLEMENT_OP2_SET_NODE
#define IMPLEMENT_OP2_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL,COMPUTER) \
    struct SimNode_##OPNAME##_##COMPUTEL##_##COMPUTER : SimNode_Op2At { \
         DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            auto rr = *((uint32_t *)r.compute##COMPUTER(context)); \
            if ( rr >= range ) context.throw_error_at(debugInfo,"index out of range, %u of %u", rr, range); \
            return v_ldu((const float *)(pl + rr*stride + offset)); \
        } \
    };

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

    IMPLEMENT_SETOP_NUMERIC_VEC(AtR2V);

/* At */

#undef IMPLEMENT_OP2_SET_NODE_ANY
#define IMPLEMENT_OP2_SET_NODE_ANY(INLINE,OPNAME,TYPE,CTYPE,COMPUTEL) \
    struct SimNode_##OPNAME##_##COMPUTEL##_Any : SimNode_Op2At { \
        INLINE auto compute ( Context & context ) { \
            DAS_PROFILE_NODE \
            auto pl = l.compute##COMPUTEL(context); \
            auto rr = uint32_t(r.subexpr->evalInt(context)); \
            if ( rr >= range ) context.throw_error_at(debugInfo,"index out of range, %u of %u", rr, range); \
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
            auto rr = *((uint32_t *)r.compute##COMPUTER(context)); \
            if ( rr >= range ) context.throw_error_at(debugInfo,"index out of range, %u of %u", rr, range); \
            return pl + rr*stride + offset; \
        } \
        DAS_PTR_NODE; \
    };

#undef IMPLEMENT_OP2_SET_SETUP_NODE
#define IMPLEMENT_OP2_SET_SETUP_NODE(result,node) \
    auto rn = (SimNode_Op2At *)result; \
    auto sn = (SimNode_At *)node; \
    rn->stride = sn->stride; \
    rn->offset = sn->offset; \
    rn->range = sn->range; \
    rn->baseType = Type::none;

#include "daScript/simulate/simulate_fusion_op2_set_impl.h"
#include "daScript/simulate/simulate_fusion_op2_set_perm.h"

    IMPLEMENT_ANY_SETOP(__forceinline, At, Ptr, StringPtr, StringPtr);
    IMPLEMENT_ANY_SETOP(__forceinline, At, Ptr, VoidPtr, StringPtr);

    void createFusionEngine_at() {
        REGISTER_SETOP_SCALAR(AtR2V);
        REGISTER_SETOP_NUMERIC_VEC(AtR2V);
        (*g_fusionEngine)["At"].emplace_back(new FusionPoint_Set_At_StringPtr());
        (*g_fusionEngine)["At"].emplace_back(new FusionPoint_Set_At_VoidPtr());
    }
}

#endif

