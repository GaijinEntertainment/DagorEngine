// A generic whose first argument is an unresolved type expression with a default value. The
// call matches at lookup; the alias map then stops growing with that argument still
// unmatched, and the instancing after the loop used to read types the loop never produced.
options gen2
expect 30341

def generic_with_unknown_arg_type(r : l() <- 1, v) => 0

[export]
def call_generic_with_unknown_arg_type {
    generic_with_unknown_arg_type(){}
}
