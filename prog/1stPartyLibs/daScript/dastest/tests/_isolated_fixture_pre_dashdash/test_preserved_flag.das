options gen2

require dastest/testing_boost public
require strings

// This fixture asserts that a daslang flag passed BEFORE the dastest.das
// script (e.g. `daslang -dasroot <path> dastest.das -- ...`) is preserved
// through dastest's isolated-mode singleTest cmd reconstruction. The outer
// test_isolated_mode.das spawns dastest with a pre-`--` `-dasroot` flag;
// this fixture runs in the spawned subprocess and checks that `-dasroot`
// is in its own raw argv (because dastest preserved it via launchPrefix).
[test]
def test_preserved_flag_round_trip(tt : T?) {
    tt |> run("dastest preserves pre-`--` daslang flags in isolated-mode subprocess argv") <| @(t : T?) {
        let argv <- get_command_line_arguments()
        let idx = find_index(argv, "-dasroot")
        t |> success(idx >= 0, "expected -dasroot in argv, got argv={argv}")
        if (idx < 0) {
            return
        }
        t |> success(idx + 1 < length(argv), "-dasroot has no following path arg in argv")
    }
}
