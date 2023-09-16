require daslib/jobque_boost

struct OneJob
    name : string

[export]
def main
    let some_job_list <- [{for x in range(10); "job_{x}"}]
    let total_jobs = length(some_job_list)
    let total_threads = 3 // get_total_hw_threads() - 2
    with_channel(total_jobs) <| $ ( channel )
        with_job_status(total_threads) <| $ ( completion )
            // create threads
            for t in range(total_threads)
                new_thread <| @
                    print("in thread {t}\n")
                    for_each_clone(channel) <| $ ( j : OneJob# )
                        // each thread reads from channel, until its empty
                        print("in thread {t} job {j.name}\n")
                    channel |> release
                    // once done, notifies for the job completion
                    completion |> notify_and_release
                    print("done thread {t}\n")
            // this could be anywhere
            print("pushing jobs\n")
            for jb in some_job_list
                channel |> push_clone([[OneJob name=jb]])
                channel |> notify
            print("done pushing jobs\n")
            // wait for threads to finish
            completion |> join  // if we comment out this line, we get `job status beeing deleted while being used` exception
        print("done\n")


