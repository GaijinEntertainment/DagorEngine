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

    struct SimNode_Op1PtrFdr : SimNode_Op1Fusion {
        virtual SimNode * visit(SimVisitor & vis) override {
            V_BEGIN();
            string name = op;
            name += getSimSourceName(subexpr.type);
            if ( baseType != Type::none && baseType != Type::anyArgument ) {
                vis.op(name.c_str(), getTypeBaseSize(baseType), das_to_string(baseType));
            } else {
                vis.op(name.c_str());
            }
            subexpr.visit(vis);
            V_ARG(offset);
            V_END();
        }
        uint32_t  offset;
    };

    /* PtrFdr Any */

#undef IMPLEMENT_OP1_SETUP_NODE
#define IMPLEMENT_OP1_SETUP_NODE(result,node) \
    auto rn = (SimNode_Op1PtrFdr *)result; \
    auto sn = (SimNode_PtrFieldDeref *)node; \
    rn->offset = sn->offset; \
    rn->baseType = Type::none;

#undef FUSION_OP1_SUBEXPR
#define FUSION_OP1_SUBEXPR(CTYPE,node)      ((static_cast<SimNode_PtrFieldDeref*>(node))->subexpr)

/* PtrFieldDeref any */

#undef IMPLEMENT_ANY_OP1_NODE
#define IMPLEMENT_ANY_OP1_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,COMPUTE) \
    struct SimNode_Op1##COMPUTE : SimNode_Op1PtrFdr { \
        INLINE char * compute(Context & context) { \
            DAS_PROFILE_NODE \
            auto prv = (char **) subexpr.compute##COMPUTE(context); \
            if ( !prv || !*prv ) context.throw_error_at(debugInfo,"dereferencing null pointer"); \
            return (*prv) + offset; \
        } \
        DAS_PTR_NODE; \
    };

#include "daScript/simulate/simulate_fusion_op1_impl.h"
#include "daScript/simulate/simulate_fusion_op1_perm.h"

    IMPLEMENT_ANY_OP1_FUSION_POINT(__forceinline,PtrFieldDeref,,vec4f,vec4f)

/* FieldDeref any */

#undef IMPLEMENT_ANY_OP1_NODE
#define IMPLEMENT_ANY_OP1_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,COMPUTE) \
    struct SimNode_Op1##COMPUTE : SimNode_Op1PtrFdr { \
        INLINE char * compute(Context & context) { \
            DAS_PROFILE_NODE \
            auto prv = (char **) subexpr.compute##COMPUTE(context); \
            return (*prv) + offset; \
        } \
        DAS_PTR_NODE; \
    };

#include "daScript/simulate/simulate_fusion_op1_impl.h"
#include "daScript/simulate/simulate_fusion_op1_perm.h"

    IMPLEMENT_ANY_OP1_FUSION_POINT(__forceinline,FieldDeref,,vec4f,vec4f)

/* PtrFieldDeref<Scalar> */

#undef IMPLEMENT_OP1_SETUP_NODE
#define IMPLEMENT_OP1_SETUP_NODE(result,node) \
    auto rn = (SimNode_Op1PtrFdr *)result; \
    auto sn = (SimNode_PtrFieldDeref *)node; \
    rn->offset = sn->offset;

#undef IMPLEMENT_ANY_OP1_NODE
#define IMPLEMENT_ANY_OP1_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,COMPUTE) \
    struct SimNode_Op1##COMPUTE : SimNode_Op1PtrFdr { \
        INLINE auto compute(Context & context) { \
            DAS_PROFILE_NODE \
            auto prv = (char **) subexpr.compute##COMPUTE(context); \
            if ( !prv || !*prv ) context.throw_error_at(debugInfo,"dereferencing null pointer"); \
            return *((RCTYPE *)((*prv) + offset)); \
        } \
        DAS_NODE(TYPE,RCTYPE); \
    };

#include "daScript/simulate/simulate_fusion_op1_impl.h"
#include "daScript/simulate/simulate_fusion_op1_perm.h"

    IMPLEMENT_OP1_WORKHORSE_FUSION_POINT(PtrFieldDerefR2V);

/* FieldDeref<Scalar> */

#undef IMPLEMENT_ANY_OP1_NODE
#define IMPLEMENT_ANY_OP1_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,COMPUTE) \
    struct SimNode_Op1##COMPUTE : SimNode_Op1PtrFdr { \
        INLINE auto compute(Context & context) { \
            DAS_PROFILE_NODE \
            auto prv = (char **) subexpr.compute##COMPUTE(context); \
            return *((RCTYPE *)((*prv) + offset)); \
        } \
        DAS_NODE(TYPE,RCTYPE); \
    };

#include "daScript/simulate/simulate_fusion_op1_impl.h"
#include "daScript/simulate/simulate_fusion_op1_perm.h"

    IMPLEMENT_OP1_WORKHORSE_FUSION_POINT(FieldDerefR2V);

/* PtrFieldDeref<Vec> */

#undef IMPLEMENT_ANY_OP1_NODE
#define IMPLEMENT_ANY_OP1_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,COMPUTE) \
    struct SimNode_Op1##COMPUTE : SimNode_Op1PtrFdr { \
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto prv = (char **) subexpr.compute##COMPUTE(context); \
            if ( !prv || !*prv ) context.throw_error_at(debugInfo,"dereferencing null pointer"); \
            return v_ldu((const float *) ((*prv)+offset)); \
        } \
    };

#include "daScript/simulate/simulate_fusion_op1_impl.h"
#include "daScript/simulate/simulate_fusion_op1_perm.h"

    IMPLEMENT_OP1_NUMERIC_VEC(PtrFieldDerefR2V);

/* FieldDeref<Vec> */

#undef IMPLEMENT_ANY_OP1_NODE
#define IMPLEMENT_ANY_OP1_NODE(INLINE,OPNAME,TYPE,CTYPE,RCTYPE,COMPUTE) \
    struct SimNode_Op1##COMPUTE : SimNode_Op1PtrFdr { \
        virtual vec4f DAS_EVAL_ABI eval ( Context & context ) override { \
            DAS_PROFILE_NODE \
            auto prv = (char **) subexpr.compute##COMPUTE(context); \
            return v_ldu((const float *) ((*prv)+offset)); \
        } \
    };

#include "daScript/simulate/simulate_fusion_op1_impl.h"
#include "daScript/simulate/simulate_fusion_op1_perm.h"

    IMPLEMENT_OP1_NUMERIC_VEC(FieldDerefR2V);

#include "daScript/simulate/simulate_fusion_op1_reg.h"

    void createFusionEngine_ptrfdr()
    {
        REGISTER_OP1_WORKHORSE_FUSION_POINT(FieldDerefR2V);
        REGISTER_OP1_NUMERIC_VEC(FieldDerefR2V);
        (*g_fusionEngine)["FieldDeref"].emplace_back(new Op1FusionPoint_FieldDeref_vec4f());

        REGISTER_OP1_WORKHORSE_FUSION_POINT(PtrFieldDerefR2V);
        REGISTER_OP1_NUMERIC_VEC(PtrFieldDerefR2V);
        (*g_fusionEngine)["PtrFieldDeref"].emplace_back(new Op1FusionPoint_PtrFieldDeref_vec4f());
    }
}

#endif

