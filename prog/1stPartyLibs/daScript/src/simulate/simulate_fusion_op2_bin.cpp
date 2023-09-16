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

    IMPLEMENT_OP2_INTEGER(BinAnd);
    IMPLEMENT_OP2_INTEGER(BinOr);
    IMPLEMENT_OP2_INTEGER(BinXor);
    IMPLEMENT_OP2_INTEGER(BinShl);
    IMPLEMENT_OP2_INTEGER(BinShr);
    IMPLEMENT_OP2_INTEGER(BinRotl);
    IMPLEMENT_OP2_INTEGER(BinRotr);

    void createFusionEngine_op2_bin()
    {
        REGISTER_OP2_INTEGER(BinAnd);
        REGISTER_OP2_INTEGER(BinOr);
        REGISTER_OP2_INTEGER(BinXor);
        REGISTER_OP2_INTEGER(BinShl);
        REGISTER_OP2_INTEGER(BinShr);
        REGISTER_OP2_INTEGER(BinRotl);
        REGISTER_OP2_INTEGER(BinRotr);
    }
}

#endif

