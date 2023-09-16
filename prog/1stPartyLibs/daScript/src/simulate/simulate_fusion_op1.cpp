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

    // unary
    IMPLEMENT_OP1_NUMERIC_FUSION_POINT(Unm);
    IMPLEMENT_OP1_NUMERIC_FUSION_POINT(Unp);
    // binary
    IMPLEMENT_OP1_INTEGER_FUSION_POINT(BinNot);
    // boolean
    IMPLEMENT_ANY_OP1_FUSION_POINT(__forceinline,BoolNot,Bool,bool,bool);
    // inc and dec
    IMPLEMENT_OP1_SET_NUMERIC_FUSION_POINT(Inc);
    IMPLEMENT_OP1_SET_NUMERIC_FUSION_POINT(Dec);
    IMPLEMENT_OP1_SET_NUMERIC_FUSION_POINT(IncPost);
    IMPLEMENT_OP1_SET_NUMERIC_FUSION_POINT(DecPost);

    void createFusionEngine_op1()
    {
        // unary
        REGISTER_OP1_NUMERIC_FUSION_POINT(Unm);
        REGISTER_OP1_NUMERIC_FUSION_POINT(Unp);
        // binary
        REGISTER_OP1_INTEGER_FUSION_POINT(BinNot);
        // boolean
        REGISTER_OP1_FUSION_POINT(BoolNot,Bool,bool);
        // inc and dec
        REGISTER_OP1_NUMERIC_FUSION_POINT(Inc);
        REGISTER_OP1_NUMERIC_FUSION_POINT(Dec);
        REGISTER_OP1_NUMERIC_FUSION_POINT(IncPost);
        REGISTER_OP1_NUMERIC_FUSION_POINT(DecPost);
    }
}

#endif

