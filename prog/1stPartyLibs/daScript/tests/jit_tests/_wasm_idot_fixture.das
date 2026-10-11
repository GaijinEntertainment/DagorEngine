// The fixture wasm_idot_lowering.das compiles for the wasm64 cross target and for the host, dumping
// the IR: one signed idot4 over byte16 operands and one UNSIGNED-by-signed idot4, neither foldable
// by the emitter, since the bytes arrive through the command line's length. The unsigned one is the
// arm that widens with ZExt - without it that form falls to the 16-lane generic chain.
options gen2

def taint(n : int) : byte16 {
    var b : byte16
    unsafe {
        var p = addr<int8?>(b)
        for (i in range(16)) {
            p[i] = int8(i * 3 - n)
        }
    }
    return b
}

def taint_u(n : int) : ubyte16 {
    var b : ubyte16
    unsafe {
        var p = addr<uint8?>(b)
        for (i in range(16)) {
            p[i] = uint8((i * 37 + n) % 256)
        }
    }
    return b
}

[export]
def main() {
    let args <- get_command_line_arguments()
    let w = taint(length(args))
    let x = taint(length(args) + 5)
    let d = idot4(w, x)
    print("IDOT4=[{d.x} {d.y} {d.z} {d.w}]\n")
    let wu = taint_u(length(args))
    let du = idot4(wu, x)
    print("IDOT4U=[{du.x} {du.y} {du.z} {du.w}]\n")
}
