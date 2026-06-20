// options/main.das — Script that should FAIL to compile
//
// `options rtti` is neither allowed nor blocked by the project policy, so the
// parser must reject it with "option rtti is not allowed here".

options gen2
options rtti

[export]
def test {
    return true
}
