options gen2
module _spawn_common shared public

require daslib/fio
require strings

//! runs `cmd` with stderr folded into stdout, collects every non-empty stripped line, returns the exit code
def spawn_child(cmd : string; var lines : array<string>) : int {
    let rc = unsafe(popen_timeout("{cmd} 2>&1", 300.0) $(f) {
        if (f == null) {
            return
        }
        while (!feof(f)) {
            let ln = strip(fgets(f))
            if (!empty(ln)) {
                lines |> push("{ln}")
            }
        }
    })
    return rc
}
