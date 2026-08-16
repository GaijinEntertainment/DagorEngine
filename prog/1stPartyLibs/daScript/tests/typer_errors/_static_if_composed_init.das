options gen2

// compiled by test_static_if_profiles.das under the lint/IDE profile
// (no_optimizations + no_infer_time_folding): every static_if below must resolve
// even though the composed global initializers are never folded in place there

let A = true
let B = false
let AB = A || B
let NB = !B
let CHAIN = AB
let LEVEL = 3
let LEVEL2 = LEVEL * 2 + 1

def f() {
    static_if (AB) {
        print("ab\n")
    } else {
        print("no ab\n")
    }
    static_if (NB) {
        print("nb\n")
    }
    static_if (CHAIN && !B) {
        print("chain\n")
    }
    static_if (LEVEL2 > 6) {
        print("lvl\n")
    }
}

[export]
def main() {
    f()
}
