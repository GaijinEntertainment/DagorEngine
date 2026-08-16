options gen2

require daslib/clargs
require daslib/fio

[export]
def main() {
    let args <- get_cli_arguments()
    if (length(args) != 3) return
    let ready = args[0]
    let release = args[1]
    let done = args[2]
    if (!fwrite(ready, "ready")) return

    // Wait for an explicit parent event. The elapsed-time check is only a
    // deadlock guard so a failed test cannot leave a spinning child behind.
    let started = ref_time_ticks()
    while (!fexist(release)) {
        if (get_time_usec(started) > 10000000) return
    }
    // Publish completion with an atomic rename so the parent cannot observe
    // the event while this process still has the destination file open.
    let staging = "{done}.tmp"
    if (!fwrite(staging, "done")) return
    var error : string
    let _ = rename(staging, done, error)
}
