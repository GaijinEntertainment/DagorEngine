#pragma once

#undef FUSION_OP2_PTR
#define FUSION_OP2_PTR(CTYPE,expr)              ((expr))

#undef FUSION_OP2_RVALUE_LEFT
#define FUSION_OP2_RVALUE_LEFT(CTYPE,expr)      (v_ldu((const float *)(expr)))

#undef FUSION_OP2_RVALUE_RIGHT
#define FUSION_OP2_RVALUE_RIGHT(CTYPE,expr)     (v_ldu((const float *)(expr)))

#undef FUSION_OP2_PTR_TO_LVALUE
#define FUSION_OP2_PTR_TO_LVALUE(expr)          ((expr))
