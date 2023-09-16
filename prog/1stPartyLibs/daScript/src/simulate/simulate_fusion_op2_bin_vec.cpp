#include "daScript/misc/platform.h"

#ifdef _MSC_VER
#pragma warning(disable:4505)
#endif

#include "daScript/simulate/simulate_fusion.h"

#if DAS_FUSION>=2

#include "daScript/simulate/sim_policy.h"
#include "daScript/ast/ast.h"
#include "daScript/simulate/simulate_fusion_op2.h"
#include "daScript/simulate/simulate_fusion_op2_vec_settings.h"

// fake DAS_NODE to support regular eval
#undef DAS_NODE
#undef DAS_NODE
#define DAS_NODE(TYPE,CTYPE)                                    \
    virtual vec4f DAS_EVAL_ABI eval ( das::Context & context ) override {    \
        return compute(context);                                \
    }

namespace das {

    IMPLEMENT_OP2_INTEGER_VEC(BinAnd);
    IMPLEMENT_OP2_INTEGER_VEC(BinOr);
    IMPLEMENT_OP2_INTEGER_VEC(BinXor);
    IMPLEMENT_OP2_INTEGER_VEC(BinShl);
    IMPLEMENT_OP2_INTEGER_VEC(BinShr);

    void createFusionEngine_op2_bin_vec()
    {
        REGISTER_OP2_INTEGER_VEC(BinAnd);
        REGISTER_OP2_INTEGER_VEC(BinOr);
        REGISTER_OP2_INTEGER_VEC(BinXor);
        REGISTER_OP2_INTEGER_VEC(BinShl);
        REGISTER_OP2_INTEGER_VEC(BinShr);
    }
}

#else

namespace das {
    void createFusionEngine_op2_bin_vec() {
    }
}

#endif

