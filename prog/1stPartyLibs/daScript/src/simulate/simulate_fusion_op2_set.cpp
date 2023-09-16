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

    IMPLEMENT_SETOP_WORKHORSE(Set);

    IMPLEMENT_SETOP_NUMERIC(SetAdd);
    IMPLEMENT_SETOP_NUMERIC(SetSub);
    IMPLEMENT_SETOP_NUMERIC(SetDiv);
    IMPLEMENT_SETOP_NUMERIC(SetMod);
    IMPLEMENT_SETOP_NUMERIC(SetMul);

    IMPLEMENT_SETOP_INTEGER(SetBinAnd);
    IMPLEMENT_SETOP_INTEGER(SetBinOr);
    IMPLEMENT_SETOP_INTEGER(SetBinXor);
    IMPLEMENT_SETOP_INTEGER(SetBinShl);
    IMPLEMENT_SETOP_INTEGER(SetBinShr);
    IMPLEMENT_SETOP_INTEGER(SetBinRotl);
    IMPLEMENT_SETOP_INTEGER(SetBinRotr);

    void createFusionEngine_op2_set()
    {
        REGISTER_SETOP_WORKHORSE(Set);

        REGISTER_SETOP_NUMERIC(SetAdd);
        REGISTER_SETOP_NUMERIC(SetSub);
        REGISTER_SETOP_NUMERIC(SetDiv);
        REGISTER_SETOP_NUMERIC(SetMod);
        REGISTER_SETOP_NUMERIC(SetMul);

        REGISTER_SETOP_INTEGER(SetBinAnd);
        REGISTER_SETOP_INTEGER(SetBinOr);
        REGISTER_SETOP_INTEGER(SetBinXor);
        REGISTER_SETOP_INTEGER(SetBinShl);
        REGISTER_SETOP_INTEGER(SetBinShr);
        REGISTER_SETOP_INTEGER(SetBinRotl);
        REGISTER_SETOP_INTEGER(SetBinRotr);
    }
}

#endif

