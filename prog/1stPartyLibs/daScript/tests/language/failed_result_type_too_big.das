// An oversized function result. The size gate covered locals, arguments, structures and
// aliases but not the result, so the type reached the debug-info walk and asserted there.
options gen2
expect 30512

// the reported shape - the make-value on the path is diagnosed as well. the crash itself is
// in failed_result_type_by_ref_too_big.das, where no earlier error stops the walk
[export]
def result_too_big => <- tuple<int[2000000000]>(uninitialized)

[export]
def main {
}
