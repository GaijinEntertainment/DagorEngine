options gen2

require UnitTest

[sideeffects]
def fib(n : int) {
                                // MOV     A, ARG[0]
                                // DEC     A
                                // CMPLE   A, 0
                                // CJMP    @FOR_END
                                // MOV     C, A
    var A = 0                   // MOV     A, 0
    var B = 1                   // MOV     B, 1
// @FOR:
    for (C in range(n - 1)) {
        let tmp = B             // XCHANGE A, B         // MOV    ARG[0], B
        B += A                  // ADD     B, A         // ADD    B, A
        A = tmp                                         // MOV    A, ARG[0]
    }                           // LOOP    @FOR
// @FOREND:
    return B                    // RETURN  B
}

def mov_a_arg(arg : int) {
    return ByteCode(op_code = OpCode.op_mov_a_arg, high = uint8(arg), low = int16(0))
}

def mov_arg_b(arg : int) {
    return ByteCode(op_code = OpCode.op_mov_arg_b, high = uint8(arg), low = int16(0))
}

def dec_a {
    return ByteCode(op_code = OpCode.op_dec_a, high = 0u8, low = int16(0))
}

def cmple_a_low_zx(value : int) {
    return ByteCode(op_code = OpCode.op_cmple_a_low_zx, high = 0u8, low = int16(value))
}

def cjmp(lab : int) {
    return ByteCode(op_code = OpCode.op_cjmp, high = 0u8, low = int16(lab))
}

def mov_c_a() {
    return ByteCode(op_code = OpCode.op_mov_c_a, high = 0u8, low = int16(0))
}

def mov_a_low_zx(value : int) {
    return ByteCode(op_code = OpCode.op_mov_a_low_zx, high = 0u8, low = int16(value))
}

def mov_b_low_zx(value : int) {
    return ByteCode(op_code = OpCode.op_mov_b_low_zx, high = 0u8, low = int16(value))
}

def xchange_a_b() {
    return ByteCode(op_code = OpCode.op_xchange_a_b, high = 0u8, low = int16(0))
}
def add_b_a() {
    return ByteCode(op_code = OpCode.op_add_b_a, high = 0u8, low = int16(0))
}

def loop(lab : int) {
    return ByteCode(op_code = OpCode.op_loop, high = 0u8, low = int16(lab))
}

def return_b() {
    return ByteCode(op_code = OpCode.op_return_b, high = 0u8, low = int16(0))
}

let fib_bytecode_opt <- [
    mov_a_arg(0),           // MOV     A, ARG[0]
    dec_a(),                // DEC     A
    cmple_a_low_zx(0),      // CMPLE   A, 0
    cjmp(7),                // CJMP    @FOR_END
    mov_c_a(),              // MOV     C, A
    mov_a_low_zx(0),        // MOV     A, 0
    mov_b_low_zx(1),        // MOV     B, 1
// @FOR:
    xchange_a_b(),          // XCHANGE A, B
    add_b_a(),              // ADD     B, A
    loop(-2),               // LOOP    @FOR
// @FOREND:
    return_b()              // RETURN  B
]

let fib_bytecode <- [
    mov_a_arg(0),           // MOV     A, ARG[0]
    dec_a(),                // DEC     A
    cmple_a_low_zx(0),      // CMPLE   A, 0
    cjmp(7),                // CJMP    @FOR_END
    mov_c_a(),              // MOV     C, A
    mov_a_low_zx(0),        // MOV     A, 0
    mov_b_low_zx(1),        // MOV     B, 1
// @FOR:
    mov_arg_b(0),           // MOV     ARG[0], B
    add_b_a(),              // ADD     B, A
    mov_a_arg(0),           // MOV     A, ARG[0]
    loop(-3),               // LOOP    @FOR
// @FOREND:
    return_b()              // RETURN  B
]

def fib_bytecode_i(n : int) {
    return invoke($(N : int) : int {
        return evalByteCode(unsafe(addr(fib_bytecode[0])))
    }, n);
}


def fib_bytecode_i_opt(n : int) {
    return invoke($(N : int) : int {
        return evalByteCode(unsafe(addr(fib_bytecode_opt[0])))
    }, n);
}

[export]
def main() {
    var f1_j = 0
    profile(20, "fibonacci loop - interpreted") <| $() {
        f1_j = fib(6511134)
    }
    assert(f1_j == 1781508648)
    var f2_j = 0
    profile(20, "fibonacci loop - bytecode") <| $() {
        f2_j = fib_bytecode_i(6511134)
    }
    assert(f2_j == 1781508648)
    var f3_j = 0
    profile(20, "fibonacci loop - bytecode optimized (with xchg)") <| $() {
        f3_j = fib_bytecode_i_opt(6511134)
    }
    assert(f3_j == 1781508648)

}
