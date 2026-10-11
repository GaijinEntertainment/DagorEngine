// The oversized result on its own, with no other error to stop compilation first: this one
// reaches Program::simulate, where the debug-info walk sizes the result and asserts.
options gen2
expect 30512

[export]
def result_too_big_by_ref(p : int[2000000000]?) : int[2000000000]& {
    unsafe { return *p }
}

[export]
def main {
}
