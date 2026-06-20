// Regression test for invalid_empty_name (30296) on ExprCall.
// Cascading 30341 "no matching functions or generics" fires when the
// resolver attempts to look up the empty function name.
options gen2
expect 30296, 30341

require _helper_empty_names

[export]
def main {
    let _ = make_empty_call()
}
