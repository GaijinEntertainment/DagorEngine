options gen2
options _nolint = "LINT031"   // the write through a const-stripped parameter is the shape under test

module _const_arg_readonly_writer

// The same write as the routes of const_arg_readonly.das, in a separate das-module so that
// the call crosses a codegen partition (the default --jit-split-modules). No inlining hint:
// LLVM cannot inline across the partition, so the call site is resolved against the
// declaration and its parameter attributes alone.
def public store_int(dst : void?; v : int) {
    unsafe {
        var p = reinterpret<int?>(dst)
        *p = v
    }
}
