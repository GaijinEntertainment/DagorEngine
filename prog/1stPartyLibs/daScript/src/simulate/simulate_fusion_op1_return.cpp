#include "daScript/misc/platform.h"

#ifdef _MSC_VER
#pragma warning(disable:4505)
#endif

#include "daScript/simulate/simulate_fusion.h"

#if DAS_FUSION

#include "daScript/simulate/sim_policy.h"
#include "daScript/ast/ast.h"
#include "daScript/simulate/simulate_fusion_op1.h"

namespace das {

/* Return Any */

#undef IMPLEMENT_ANY_OP1_NODE
#define IMPLEMENT_ANY_OP1_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,COMPUTE) \
    struct SimNode_Op1##COMPUTE : SimNode_Op1Fusion { \
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto lv =  subexpr.compute##COMPUTE(context); \
            context.stopFlags |= EvalFlags::stopForReturn; \
            context.abiResult() = v_ldu((const float *) lv); \
            return v_zero(); \
        } \
    };

#undef FUSION_OP1_SUBEXPR
#define FUSION_OP1_SUBEXPR(CTYPE,node)      ((static_cast<SimNode_Return*>(node))->subexpr)

#include "daScript/simulate/simulate_fusion_op1_impl.h"
#include "daScript/simulate/simulate_fusion_op1_perm.h"

IMPLEMENT_ANY_OP1_FUSION_POINT(__forceinline,Return,,vec4f,vec4f)

/* Return Scalar */

#undef MATCH_ANY_OP1_NODE
#define MATCH_ANY_OP1_NODE(CTYPE,NODENAME,COMPUTE) \
    else if ( is(info,node_x,NODENAME,(typeName<CTYPE>::name())) ) { return ccode.makeNode<SimNode_Op1##COMPUTE>(); }

#undef IMPLEMENT_ANY_OP1_NODE
#define IMPLEMENT_ANY_OP1_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,COMPUTE) \
    struct SimNode_Op1##COMPUTE : SimNode_Op1Fusion { \
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto lv =  FUSION_OP_PTR_VALUE(CTYPE,subexpr.compute##COMPUTE(context)); \
            context.stopFlags |= EvalFlags::stopForReturn; \
            context.abiResult() = cast<CTYPE>::from(lv); \
            return v_zero(); \
        } \
    };

#include "daScript/simulate/simulate_fusion_op1_impl.h"
#include "daScript/simulate/simulate_fusion_op1_perm.h"

    IMPLEMENT_OP1_WORKHORSE_FUSION_POINT(Return);

/* Return Vec */

#undef IMPLEMENT_ANY_OP1_NODE
#define IMPLEMENT_ANY_OP1_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,COMPUTE) \
    struct SimNode_Op1##COMPUTE : SimNode_Op1Fusion { \
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto lv =  subexpr.compute##COMPUTE(context); \
            context.stopFlags |= EvalFlags::stopForReturn; \
            context.abiResult() = v_ldu((const float *) lv); \
            return v_zero(); \
        } \
    };

#include "daScript/simulate/simulate_fusion_op1_impl.h"
#include "daScript/simulate/simulate_fusion_op1_perm.h"

    IMPLEMENT_OP1_NUMERIC_VEC(Return);

#undef REGISTER_OP1_FUSION_POINT
#define REGISTER_OP1_FUSION_POINT(OPNAME,TYPE,CTYPE) \
    (*g_fusionEngine)[#OPNAME].emplace_back(new Op1FusionPoint_##OPNAME##_##CTYPE());

#include "daScript/simulate/simulate_fusion_op1_reg.h"

    void createFusionEngine_return()
    {
        REGISTER_OP1_WORKHORSE_FUSION_POINT(Return);
        REGISTER_OP1_NUMERIC_VEC(Return);
        (*g_fusionEngine)["Return"].emplace_back(new Op1FusionPoint_Return_vec4f());
    }
}

#endif

