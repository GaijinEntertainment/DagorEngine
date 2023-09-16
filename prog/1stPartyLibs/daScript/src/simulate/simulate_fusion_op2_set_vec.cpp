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

namespace das {

    IMPLEMENT_SETOP_NUMERIC_VEC(Set);

    IMPLEMENT_SETOP_NUMERIC_VEC(SetAdd);
    IMPLEMENT_SETOP_NUMERIC_VEC(SetSub);
    IMPLEMENT_SETOP_NUMERIC_VEC(SetDiv);
    IMPLEMENT_SETOP_NUMERIC_VEC(SetMod);
    IMPLEMENT_SETOP_NUMERIC_VEC(SetMul);

    IMPLEMENT_SETOP_INTEGER_VEC(SetBinAnd);
    IMPLEMENT_SETOP_INTEGER_VEC(SetBinOr);
    IMPLEMENT_SETOP_INTEGER_VEC(SetBinXor);
    IMPLEMENT_SETOP_INTEGER_VEC(SetBinShl);
    IMPLEMENT_SETOP_INTEGER_VEC(SetBinShr);

#undef FUSION_OP2_RVALUE_RIGHT
#define FUSION_OP2_RVALUE_RIGHT(CTYPE,expr)     (v_ldu_x((const float *)(expr)))

    IMPLEMENT_SETOP_NUMERIC_VEC(SetDivScal);
    IMPLEMENT_SETOP_NUMERIC_VEC(SetMulScal);

    void createFusionEngine_op2_set_vec()
    {
        REGISTER_SETOP_NUMERIC_VEC(Set);

        REGISTER_SETOP_NUMERIC_VEC(SetAdd);
        REGISTER_SETOP_NUMERIC_VEC(SetSub);
        REGISTER_SETOP_NUMERIC_VEC(SetDiv);
        REGISTER_SETOP_NUMERIC_VEC(SetMod);
        REGISTER_SETOP_NUMERIC_VEC(SetMul);
        REGISTER_SETOP_NUMERIC_VEC(SetDivScal);
        REGISTER_SETOP_NUMERIC_VEC(SetMulScal);

        REGISTER_SETOP_INTEGER_VEC(SetBinAnd);
        REGISTER_SETOP_INTEGER_VEC(SetBinOr);
        REGISTER_SETOP_INTEGER_VEC(SetBinXor);
        REGISTER_SETOP_INTEGER_VEC(SetBinShl);
        REGISTER_SETOP_INTEGER_VEC(SetBinShr);
    }
}

#else

namespace das {
    void createFusionEngine_op2_set_vec() {
    }
}

#endif

