options gen2
options gc
options persistent_heap

require daslib/jobque_boost

// A thread lambda's capture outlives a collect the thread runs: the capture block's header names
// the capture's type, so the root walk reaches the captured array where a headerless block
// would have the walk read a null type. Each collect runs in the thread's or the job's own context,
// on a stack the host starts empty - the case `[unsafe_heap_collect]` exists for.

[export, unsafe_heap_collect]
def main {
    var data <- [for (i in range(64)); i]
    with_channel(1) $(ch) {
        new_thread() <| @ [unsafe_heap_collect] capture(:= data) {
            unsafe(heap_collect(false, true))
            let churn <- [for (i in range(4096)); i]
            var sum = 0
            for (v in data) {
                sum += v
            }
            print("THREAD_COLLECT_SUM {sum}\n")
            ch |> notify_and_release()
        }
        ch |> join()
    }
    // the job path writes the same header from its own site
    var jdata <- [for (i in range(32)); i * 2]
    with_job_que() {
        with_channel(1) $(ch) {
            new_job() <| @ [unsafe_heap_collect] capture(:= jdata) {
                unsafe(heap_collect(false, true))
                let churn <- [for (i in range(4096)); i]
                var sum = 0
                for (v in jdata) {
                    sum += v
                }
                print("JOB_COLLECT_SUM {sum}\n")
                ch |> notify_and_release()
            }
            ch |> join()
        }
    }
    print("THREAD_COLLECT_DONE\n")
}
