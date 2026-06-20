options gen2

require dastest/testing_boost public
require daslib/clargs
require strings

[test]
def test_worker_index_round_trip(tt : T?) {
    tt |> run("--worker-index N is visible via get_user_args() and in range") <| @(t : T?) {
        let args <- get_user_args()
        let idx = find_index(args, "--worker-index")
        t |> success(idx >= 0, "--worker-index not found in get_user_args() args={args}")
        if (idx < 0) {
            return
        }
        t |> success(idx + 1 < length(args), "no value after --worker-index")
        if (idx + 1 >= length(args)) {
            return
        }
        // Worker indices are 1-based (dastest passes --worker-index t+1, so a client
        // can map index N -> live-API port 9090+N and leave 9090 free); with the 4
        // threads test_isolated_mode launches, they span [1, 4].
        let val = to_int(args[idx + 1])
        t |> success(val >= 1 && val <= 4, "worker_index {val} out of range [1, 4]")
    }
}
