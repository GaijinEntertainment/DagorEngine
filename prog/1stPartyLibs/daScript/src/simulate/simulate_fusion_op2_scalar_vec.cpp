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
#define DAS_NODE(TYPE,CTYPE)                                    \
    DAS_EVAL_ABI virtual vec4f eval ( das::Context & context ) override {    \
        return compute(context);                                \
    }

namespace das {

#undef FUSION_OP_PTR_VALUE_RIGHT
#define FUSION_OP_PTR_VALUE_RIGHT(CTYPE,expr)   (v_ldu_x((const float *)(expr)))

    IMPLEMENT_OP2_NUMERIC_VEC(MulVecScal);
    IMPLEMENT_OP2_NUMERIC_VEC(DivVecScal);

#undef FUSION_OP_PTR_VALUE_RIGHT
#define FUSION_OP_PTR_VALUE_RIGHT(CTYPE,expr)   (v_ldu((const float *)(expr)))

#undef FUSION_OP_PTR_VALUE_LEFT
#define FUSION_OP_PTR_VALUE_LEFT(CTYPE,expr)    (v_ldu_x((const float *)(expr)))

    IMPLEMENT_OP2_NUMERIC_VEC(MulScalVec);
    IMPLEMENT_OP2_NUMERIC_VEC(DivScalVec);

    void createFusionEngine_op2_scalar_vec()
    {
        REGISTER_OP2_NUMERIC_VEC(MulVecScal);
        REGISTER_OP2_NUMERIC_VEC(DivVecScal);
        REGISTER_OP2_NUMERIC_VEC(MulScalVec);
        REGISTER_OP2_NUMERIC_VEC(DivScalVec);
    }
}

#else

namespace das {
    void createFusionEngine_op2_scalar_vec() {
    }
}

#endif

