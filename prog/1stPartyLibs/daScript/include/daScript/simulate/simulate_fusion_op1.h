#define FUSION_OP_PTR_VALUE(CTYPE,expr)    *((CTYPE *)((expr)))
#define FUSION_OP_PTR_RVALUE(CTYPE,expr)   ((CTYPE *)((expr)))
#define FUSION_OP_EVAL_CAST(expr)          (*(expr))

#define IMPLEMENT_ANY_OP1_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,COMPUTE) \
    struct SimNode_Op1##COMPUTE : SimNode_Op1Fusion { \
            INLINE auto compute ( Context & context ) { \
                DAS_PROFILE_NODE \
                auto lv =  FUSION_OP_PTR_VALUE(CTYPE,subexpr.compute##COMPUTE(context)); \
                return SimPolicy<CTYPE>::OPNAME(lv,context,&debugInfo); \
            } \
            DAS_NODE(TYPE,CTYPE); \
        };

#define IMPLEMENT_ANY_OP1_SET_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,COMPUTE) \
    struct SimNode_Op1##COMPUTE : SimNode_Op1Fusion { \
            INLINE auto compute ( Context & context ) { \
                DAS_PROFILE_NODE \
                auto lv =  FUSION_OP_PTR_RVALUE(CTYPE,subexpr.compute##COMPUTE(context)); \
                return SimPolicy<CTYPE>::OPNAME(FUSION_OP_EVAL_CAST(lv),context,&debugInfo); \
            } \
            DAS_NODE(TYPE,CTYPE); \
        };

#define MATCH_ANY_OP1_NODE(CTYPE,NODENAME,COMPUTE) \
    else if ( is(info,node_x,NODENAME) ) { return ccode.makeNode<SimNode_Op1##COMPUTE>(); }

#define IMPLEMENT_OP1_SETUP_NODE(result,node)

#define FUSION_OP1_SUBEXPR(CTYPE,node)      ((static_cast<SimNode_Op1*>(node))->x)

#include "daScript/simulate/simulate_fusion_op1_impl.h"
#include "daScript/simulate/simulate_fusion_op1_set_impl.h"

#include "daScript/simulate/simulate_fusion_op1_perm.h"
#include "daScript/simulate/simulate_fusion_op1_set_perm.h"

#define REGISTER_OP1_FUSION_POINT(OPNAME,TYPE,CTYPE) \
    registerFusion(#OPNAME,typeName<CTYPE>::name(),new Op1FusionPoint_##OPNAME##_##CTYPE());

#include "daScript/simulate/simulate_fusion_op1_reg.h"
