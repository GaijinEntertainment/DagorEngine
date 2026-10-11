options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/jobque_boost

struct RoundResult {
    sum : int
}

// A new_thread context that pools its job forks dies as soon as its lambda returns, while the
// worker that ran the last chunk is still handing its borrowed fork back: the job notified the
// wait group from inside the lambda, before the release. The owner's destructor waits for every
// borrowed fork, so nothing outlives the process (the alive-pointer check at exit) and nothing
// releases into a dead pool. One round loses the race rarely; many rounds make it certain.
[test]
def test_pooled_fork_returns_before_owner_dies(t : T?) {
    with_job_que() {
        for (_round in range(300)) {
            with_channel(1) $(var channel) {
                new_thread() <| @capture(= channel) {
                    set_jobque_fork_pool(true, true)
                    var counts : array<int>
                    counts |> resize(4)
                    var pcounts = unsafe(addr(counts[0]))
                    parallel_for(0, 4096, 4) $(job_begin, job_end, wg) {
                        var local = 0
                        for (i in range(job_begin, job_end)) {
                            local++
                        }
                        unsafe(pcounts[job_begin / 1024]) = local
                        wg |> notify_and_release()
                    }
                    var acc = 0
                    for (c in counts) {
                        acc += c
                    }
                    channel |> push_clone(RoundResult(sum = acc))
                    channel |> notify_and_release()
                }
                channel |> for_each_clone() $(result : RoundResult#) {
                    t |> equal(result.sum, 4096)
                }
            }
        }
    }
}
