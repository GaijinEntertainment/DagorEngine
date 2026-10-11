options gen2
options no_aot

require daslib/clargs
require daslib/fio

//! child for fio_poll.das: writes nothing until the parent's release file appears, then one
//! line, and exits - so a poll on its stdout is silent by construction until the release
[export]
def main() : int {
    let args <- get_cli_arguments()
    if (length(args) != 1) return 2
    let release = args[0]
    let started = ref_time_ticks()
    while (!fexist(release)) {
        if (get_time_usec(started) > 30000000) return 4
        sleep(5u)
    }
    print("polled\n")
    return 0
}
