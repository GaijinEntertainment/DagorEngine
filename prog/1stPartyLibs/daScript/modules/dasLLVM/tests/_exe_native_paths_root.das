options gen2
require daslib/fio

//! the exe prints how many require-resolver rows its startup registered; the test reads the number
[export]
def main {
    var rows = 0
    for_each_registered_native_path() $(_mod_name, _src_path, _dst_path) {
        rows++
    }
    print("NATIVE_PATHS {rows}\n")
}
