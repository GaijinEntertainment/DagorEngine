options gen2
options no_aot

require daslib/aot_cpp
require daslib/ast
require strings
require daslib/strings_boost
require dastest/testing_boost public

// TEMP: das_ordered2 emitter reverted for the vc17 ICE test
// [test]
def test_op2_operand_order_emit(t : T?) {
    var cpp = ""
    using() $(var cop : CodeOfPolicies) {
        cop.aot_macros = true
        cop.aot_module = true
        cpp = aot("{get_das_root()}/tests/aot/_ordered_op2_fixture.das", false, false, cop)
    }
    t |> success(!empty(cpp), "emitter produced output")
    t |> equal(3, length(split(cpp, "das_ordered2\{")) - 1, "exactly the three impure sites are wrapped")
    t |> success(find(cpp, "(__lr.left + __lr.right)") >= 0, "plain arm sequences int +")
    t |> success(find(cpp, "(__lr.left != __lr.right)") >= 0, "impure bool ^^ sequences as !=")
    t |> success(find(cpp, "__lr.left &&") < 0, "short-circuit && is never wrapped")
    t |> success(find(cpp, "__lr.left ||") < 0, "short-circuit || is never wrapped")
    t |> success(find(cpp, "Mul(__lr.left,__lr.right") >= 0, "policy arm sequences the vector mul")
    t |> success(find(cpp, "das_ordered2\{ &(") < 0, "no operand is captured by address")
}
