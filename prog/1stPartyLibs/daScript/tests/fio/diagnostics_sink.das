options gen2

require dastest/testing_boost
require daslib/fio

// `diagnostics_to_file` points the process-wide diagnostics sink at a file, so a
// program whose stdout carries a protocol and whose launcher keeps no stderr still
// has somewhere to leave a leak dump or a fatal. It is fail-closed: a path it
// cannot open returns false and leaves the sink where it was.

[test]
def test_diagnostics_to_file(t : T?) {
    t |> run("a path that cannot be opened is refused") @(t : T?) {
        t |> success(!diagnostics_to_file("/no/such/directory/diagnostics.log"))
    }
    t |> run("a writable path is taken") @(t : T?) {
        var terr = ""
        let root = create_temp_directory("das_diag_", terr)
        t |> success(!empty(root), "temp dir created: {terr}")
        if (!empty(root)) {
            t |> success(diagnostics_to_file(path_join(root, "diag.log")))
            // moving the sink off the file closes it, and a removal that succeeds
            // is the proof: on Windows an open handle holds a file against deletion
            diagnostics_to_stderr()
            t |> success(rmdir_rec(root), "the sink held its file open")
        }
    }
}
