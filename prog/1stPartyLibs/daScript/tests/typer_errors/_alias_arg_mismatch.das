// Fixture for test_alias_message.das — intentionally fails to compile with an
// argument-type mismatch whose expected type carries a deep type alias.
// `_`-prefix keeps dastest from auto-running it; the `expect` directive makes
// lint skip it (a non-compiling program never reaches the lint passes).
expect 30341

options gen2

typedef RttiValue = tuple<argname : string; argvalue : variant<tBool : bool; tInt : int; tString : string>>

def take_args(args : array<RttiValue>) {}

[export]
def main() {
    var wrong : array<int>
    take_args(wrong)
}
