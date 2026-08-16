options gen2

expect 30220 : 2

require daslib/match

enum EOption {
    True
    False
    Maybe
}

// `match` is a statement macro - it folds into a statement block. In return
// position that block never resolves to a value; before the unresolved-return
// check this compiled silently and returned garbage (interp and jit alike).

def get_string(option : EOption) : string {
    return match(option) {         // 30220: can't return a block
        if (EOption.True) {
            return "true";
        }
        if (EOption.False) {
            return "false";
        }
        if (EOption.Maybe) {
            return "maybe";
        }
    }
}

def get_string_lambda(option : EOption) : string {
    var get = @(o : EOption) : string {
        return match(o) {          // 30220: same hole in the block-return path
            if (EOption.True) {
                return "true";
            }
            if (EOption.False) {
                return "false";
            }
            if (EOption.Maybe) {
                return "maybe";
            }
        }
    }
    return invoke(get, option)
}

[export]
def main() {
    print("{get_string(EOption.Maybe)}\n")
    print("{get_string_lambda(EOption.Maybe)}\n")
}
