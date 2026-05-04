#pragma once

#include "daScript/simulate/simulate.h"

namespace das {

enum class OpCode : uint8_t {   // only used op-codes for now
    op_nop,
    op_mov_a_arg,
    op_mov_arg_b,
    op_dec_a,
    op_cmple_a_low_zx,
    op_cjmp,
    op_mov_c_a,
    op_mov_a_low_zx,
    op_mov_b_low_zx,
    op_xchange_a_b,
    op_add_b_a,
    op_loop,
    op_return_b,
};

#pragma pack(1)
struct ByteCode {
    OpCode   op_code;
    uint8_t  high;
    int16_t  low;
};
#pragma pack()
static_assert(sizeof(ByteCode) == 4, "ByteCode size mismatch");

int32_t evalByteCode ( const ByteCode * _ptr, Context * ctx, LineInfoArg * at );

}
