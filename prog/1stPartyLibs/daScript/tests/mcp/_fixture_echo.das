options gen2

//! Fixture: read lines from stdin, echo each back to stdout (with the
//! trailing newline) and flush. Exits when stdin closes. Used by
//! ``test_popen_argv_pipe`` to verify bidirectional pipe round-trip.

require daslib/fio
require strings

[export]
def main() {
    let sin = fstdin()
    let sout = fstdout()
    while (!feof(sin)) {
        let line = fgets(sin)
        break if (empty(line))
        fprint(sout, line)
        fflush(sout)
    }
}
