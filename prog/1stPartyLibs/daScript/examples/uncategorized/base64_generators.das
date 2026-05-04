// Base64 Encoder -- Generator Chain Example
//
// Encodes a string to Base64 by piping data through a chain of
// composable generators, each performing one step of the transform:
//
//   from_sv   -- iterates characters of the input string
//   chunk6    -- groups 8-bit bytes into 6-bit values
//   xlat      -- maps 6-bit values to the Base64 alphabet
//   pad4      -- pads output to a multiple of 4 characters
//   linebreak -- inserts newlines every N characters
//
// The full pipeline:
//   input |> from_sv() |> chunk6() |> xlat() |> pad4() |> linebreak()
//
// NOTE: This is deliberately slow -- each generator yield crosses
// a coroutine boundary, so there's significant overhead per byte.
// This is a demonstration of generator composition, not a fast
// Base64 implementation. For production use, process data in
// blocks with bit manipulation directly.
//
// Run: daslang.exe examples/uncategorized/base64_generators.das

options gen2
require strings

// Iterate characters of a string as ints.
// Uses a generator to keep the string alive across yields.
def from_sv(inp : string) {
    return <- generator<int>() <| $() {
        for (x in inp) {
            yield x
        }
        return false
    }
}

// Group 8-bit input bytes into 6-bit output values.
// Every 3 input bytes produce 4 output values (with padding handled later).
def chunk6(var inp : iterator<int>) {
    return <- generator<int> capture(<- inp) () <| $() {
        var b0 = 0
        while (next(inp, b0)) {
            var b1, b2 = 0
            let has1 = next(inp, b1)
            let has2 = next(inp, b2)
            yield b0 >> 2
            yield ((b0 & 3) << 4) | (b1 >> 4)
            if (has1) {
                yield ((b1 & 15) << 2) | (b2 >> 6)
            }
            if (has2) {
                yield b2 & 63
            }
        }
        return false
    }
}

// Base64 alphabet lookup table (A-Z, a-z, 0-9, +, /)
let {
    xlattable64 <- [for (t in "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"); t]
}

// Map each 6-bit index to the corresponding Base64 character.
def xlat(var inp : iterator<int>) {
    return <- generator<int> capture(<- inp) () <| $() {
        for (i in inp) {
            yield xlattable64[i]
        }
        return false
    }
}

// Pad output with '=' to make the total length a multiple of 4.
def pad4(var inp : iterator<int>) {
    return <- generator<int> capture(<- inp) () <| $() {
        var len = 0
        for (c in inp) {
            yield c
            ++len
        }
        while ((len++ % 4) != 0) {
            yield '='
        }
        return false
    }
}

// Insert a newline every `linewidth` characters.
def linebreak(var inp : iterator<int>; linewidth = 76) {
    return <- generator<int> capture(<- inp) () <| $() {
        for (c, len in inp, range(1, INT_MAX)) {
            yield c
            if ((len % linewidth) == 0) {
                yield '\n'
            }
        }
        return false
    }
}

// Full Base64 pipeline: compose all generator stages.
def base64(inp : string; linewidth = 76) {
    return <- generator<int>() <| $() {
        var lb <- inp |> from_sv() |> chunk6() |> xlat() |> pad4() |> linebreak(linewidth)
        for (c in lb) {
            yield c
        }
        return false
    }
}

// --- Demo ---

let {
    sample = "Man is distinguished, not only by his reason, but by this singular passion from other animals,
which is a lust of the mind, that by a perseverance of delight in the continued and indefatigable
generation of knowledge, exceeds the short vehemence of any carnal pleasure."
}

[export]
def main() {
    print("input:\n{sample}\n\nbase64:\n")
    for (ch in base64(sample)) {
        print(to_char(ch))
    }
    print("\n")
}
