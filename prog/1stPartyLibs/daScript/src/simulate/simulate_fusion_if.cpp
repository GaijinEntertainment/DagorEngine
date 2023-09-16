#include "daScript/misc/platform.h"

#ifdef _MSC_VER
#pragma warning(disable:4505)
#endif

#include "daScript/simulate/simulate_fusion.h"

#if DAS_FUSION

#include "daScript/simulate/sim_policy.h"
#include "daScript/ast/ast.h"
#include "daScript/simulate/simulate_fusion_op1.h"
#include "daScript/simulate/simulate_visit_op.h"

namespace das {

    struct SimNode_Op1If : SimNode_Op1Fusion {
        virtual SimNode * visit(SimVisitor & vis) override {
            V_BEGIN_CR();
            string name = op;
            name += getSimSourceName(subexpr.type);
            if ( baseType != Type::none && baseType != Type::anyArgument ) {
                vis.op(name.c_str(), getTypeBaseSize(baseType), das_to_string(baseType));
            } else {
                vis.op(name.c_str());
            }
            subexpr.visit(vis);
            V_SUB(if_true);
            if ( if_false ) {
                V_SUB(if_false);
            }
            V_END();
        }
        SimNode * if_true, * if_false;
    };

/* IfZeroThenElse */

#undef IMPLEMENT_ANY_OP1_NODE
#define IMPLEMENT_ANY_OP1_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,COMPUTE) \
    struct SimNode_Op1##COMPUTE : SimNode_Op1If { \
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto res =  *(CTYPE *)(subexpr.compute##COMPUTE(context)); \
            if ( res == 0 ) { \
                return if_true->eval(context); \
            } else { \
                return if_false->eval(context); \
            } \
            return v_zero(); \
        } \
        virtual RCTYPE eval##TYPE ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto res =  *(CTYPE *)(subexpr.compute##COMPUTE(context)); \
            if ( res == 0 ) { \
                return if_true->eval##TYPE(context); \
            } else { \
                return if_false->eval##TYPE(context); \
            } \
        } \
    };

#undef FUSION_OP1_SUBEXPR
#define FUSION_OP1_SUBEXPR(CTYPE,node)      ((static_cast<SimNode_IfTheElseAny *>(node))->cond)

#undef IMPLEMENT_OP1_SETUP_NODE
#define IMPLEMENT_OP1_SETUP_NODE(result,node) \
    auto rn = (SimNode_Op1If *)result; \
    auto sn = (SimNode_IfTheElseAny *)node; \
    rn->if_true = sn->if_true; \
    rn->if_false = sn->if_false;

#include "daScript/simulate/simulate_fusion_op1_impl.h"
#include "daScript/simulate/simulate_fusion_op1_perm.h"

    IMPLEMENT_OP1_WORKHORSE_FUSION_POINT(IfZeroThenElse);

/* IfNotZeroThenElse */

#undef IMPLEMENT_ANY_OP1_NODE
#define IMPLEMENT_ANY_OP1_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,COMPUTE) \
    struct SimNode_Op1##COMPUTE : SimNode_Op1If { \
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto res =  *(CTYPE *)(subexpr.compute##COMPUTE(context)); \
            if ( res ) { \
                return if_true->eval(context); \
            } else { \
                return if_false->eval(context); \
            } \
            return v_zero(); \
        } \
        virtual RCTYPE eval##TYPE ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto res =  *(CTYPE *)(subexpr.compute##COMPUTE(context)); \
            if ( res ) { \
                return if_true->eval##TYPE(context); \
            } else { \
                return if_false->eval##TYPE(context); \
            } \
        } \
    };

#include "daScript/simulate/simulate_fusion_op1_impl.h"
#include "daScript/simulate/simulate_fusion_op1_perm.h"

    IMPLEMENT_OP1_WORKHORSE_FUSION_POINT(IfNotZeroThenElse);

/* IfZeroThen */

#undef IMPLEMENT_ANY_OP1_NODE
#define IMPLEMENT_ANY_OP1_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,COMPUTE) \
    struct SimNode_Op1##COMPUTE : SimNode_Op1If { \
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto res =  *(CTYPE *)(subexpr.compute##COMPUTE(context)); \
            if ( res == 0 ) { \
                return if_true->eval(context); \
            } else { \
                return v_zero(); \
            } \
        } \
    };

#include "daScript/simulate/simulate_fusion_op1_impl.h"
#include "daScript/simulate/simulate_fusion_op1_perm.h"

    IMPLEMENT_OP1_WORKHORSE_FUSION_POINT(IfZeroThen);

/* IfNotZeroThen */

#undef IMPLEMENT_ANY_OP1_NODE
#define IMPLEMENT_ANY_OP1_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,COMPUTE) \
    struct SimNode_Op1##COMPUTE : SimNode_Op1If { \
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto res =  *(CTYPE *)(subexpr.compute##COMPUTE(context)); \
            if ( res ) { \
                return if_true->eval(context); \
            } else { \
                return v_zero(); \
            } \
        } \
    };

#include "daScript/simulate/simulate_fusion_op1_impl.h"
#include "daScript/simulate/simulate_fusion_op1_perm.h"

    IMPLEMENT_OP1_WORKHORSE_FUSION_POINT(IfNotZeroThen);

#include "daScript/simulate/simulate_fusion_op1_reg.h"

    void createFusionEngine_if()
    {
        REGISTER_OP1_WORKHORSE_FUSION_POINT(IfZeroThenElse);
        REGISTER_OP1_WORKHORSE_FUSION_POINT(IfNotZeroThenElse);

        REGISTER_OP1_WORKHORSE_FUSION_POINT(IfZeroThen);
        REGISTER_OP1_WORKHORSE_FUSION_POINT(IfNotZeroThen);
    }
}

#endif

