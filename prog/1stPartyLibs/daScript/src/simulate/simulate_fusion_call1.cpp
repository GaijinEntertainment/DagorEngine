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

    struct SimNode_Op1Call1 : SimNode_Op1Fusion {
        virtual SimNode * visit(SimVisitor & vis) override {
            V_BEGIN();
            string name = op;
            name += "1";
            name += getSimSourceName(subexpr.type);
            if ( baseType != Type::none && baseType != Type::anyArgument ) {
                vis.op(name.c_str(), getTypeBaseSize(baseType), das_to_string(baseType));
            } else {
                vis.op(name.c_str());
            }
            if ( fnPtr ) {
                vis.arg(fnPtr->name,"fnPtr");
                vis.arg(Func(), fnPtr->mangledName, "fnIndex");
            }
            if ( cmresEval ) {
                V_SUB(cmresEval);
            }
            subexpr.visit(vis);
            V_END();
        }
        virtual SimNode * copyNode ( Context & context, NodeAllocator * code ) override {
            SimNode_Op1Call1 * that = (SimNode_Op1Call1 *) SimNode_Op1Fusion::copyNode(context, code);
            if ( fnPtr ) {
                that->fnPtr = context.fnByMangledName(fnPtr->mangledNameHash);
                // printf("OP1 %p to %p\n", fnPtr, that->fnPtr );
            }
            return that;
        }
        SimFunction * fnPtr = nullptr;
        SimNode * cmresEval = nullptr;
    };

#define EVAL_NODE(TYPE,CTYPE)\
        virtual CTYPE eval##TYPE ( Context & context ) override { \
            return cast<CTYPE>::to(compute(context)); \
        }

#define DAS_EVAL_NODE               \
    EVAL_NODE(Ptr,char *);          \
    EVAL_NODE(Int,int32_t);         \
    EVAL_NODE(UInt,uint32_t);       \
    EVAL_NODE(Int64,int64_t);       \
    EVAL_NODE(UInt64,uint64_t);     \
    EVAL_NODE(Float,float);         \
    EVAL_NODE(Double,double);       \
    EVAL_NODE(Bool,bool);

/* FastCall1 */

#undef IMPLEMENT_OP1_SETUP_NODE
#define IMPLEMENT_OP1_SETUP_NODE(result,node) \
    auto rn = (SimNode_Op1Call1 *)result; \
    auto sn = (SimNode_CallBase *)node; \
    rn->fnPtr = sn->fnPtr; \
    rn->cmresEval = sn->cmresEval; \
    rn->baseType = Type::none;

__forceinline SimNode * safeArg1 ( SimNode * node, int index ) {
    auto cb = static_cast<SimNode_CallBase *>(node);
    return (cb->nArguments==1) ? cb->arguments[index] : nullptr;
}

#undef FUSION_OP1_SUBEXPR
#define FUSION_OP1_SUBEXPR(CTYPE,node)      (safeArg1(node,0))

/* FastCall op1 */

#undef IMPLEMENT_ANY_OP1_NODE
#define IMPLEMENT_ANY_OP1_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,COMPUTE) \
    struct SimNode_Op1##COMPUTE : SimNode_Op1Call1 { \
        INLINE vec4f compute(Context & context) { \
            DAS_PROFILE_NODE \
            vec4f argValues[1]; \
            argValues[0] = v_ldu((const float *)subexpr.compute##COMPUTE(context)); \
            auto aa = context.abiArg; \
            context.abiArg = argValues; \
            auto res = fnPtr->code->eval(context); \
            context.stopFlags &= ~(EvalFlags::stopForReturn | EvalFlags::stopForBreak | EvalFlags::stopForContinue); \
            context.abiArg = aa; \
            return res; \
        } \
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override { \
            return compute(context); \
        } \
        DAS_EVAL_NODE \
    };

#include "daScript/simulate/simulate_fusion_op1_impl.h"
#include "daScript/simulate/simulate_fusion_op1_perm.h"

    IMPLEMENT_ANY_OP1_FUSION_POINT(__forceinline,FastCall,,vec4f,vec4f)

/* Call op1 */

#undef IMPLEMENT_ANY_OP1_NODE
#define IMPLEMENT_ANY_OP1_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,COMPUTE) \
    struct SimNode_Op1##COMPUTE : SimNode_Op1Call1 { \
        INLINE vec4f compute(Context & context) { \
            DAS_PROFILE_NODE \
            vec4f argValues[1]; \
            argValues[0] = v_ldu((const float *)subexpr.compute##COMPUTE(context)); \
            return context.call(fnPtr, argValues, &debugInfo); \
        } \
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override { \
            return compute(context); \
        } \
        DAS_EVAL_NODE \
    };

#include "daScript/simulate/simulate_fusion_op1_impl.h"
#include "daScript/simulate/simulate_fusion_op1_perm.h"

    IMPLEMENT_ANY_OP1_FUSION_POINT(__forceinline,Call,,vec4f,vec4f)

    void createFusionEngine_call1()
    {
        (*g_fusionEngine)["FastCall"].emplace_back(new Op1FusionPoint_FastCall_vec4f());
        (*g_fusionEngine)["Call"].emplace_back(new Op1FusionPoint_Call_vec4f());
    }
}

#endif

