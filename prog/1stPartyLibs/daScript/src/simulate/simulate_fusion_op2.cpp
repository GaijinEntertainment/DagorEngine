#include "daScript/misc/platform.h"

#ifdef _MSC_VER
#pragma warning(disable:4505)
#endif

#include "daScript/simulate/simulate_fusion.h"

#if DAS_FUSION

#include "daScript/simulate/sim_policy.h"
#include "daScript/ast/ast.h"
#include "daScript/simulate/simulate_fusion_op2.h"

namespace das {

    IMPLEMENT_OP2_NUMERIC(Add);
    IMPLEMENT_OP2_NUMERIC(Sub);
    IMPLEMENT_OP2_NUMERIC(Div);
    IMPLEMENT_OP2_NUMERIC(Mod);
    IMPLEMENT_OP2_NUMERIC(Mul);

    void createFusionEngine_op2()
    {
        REGISTER_OP2_NUMERIC(Add);
        REGISTER_OP2_NUMERIC(Sub);
        REGISTER_OP2_NUMERIC(Div);
        REGISTER_OP2_NUMERIC(Mod);
        REGISTER_OP2_NUMERIC(Mul);
    }
}

#endif



