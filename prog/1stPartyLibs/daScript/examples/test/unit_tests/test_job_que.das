// options log

require daslib/jobque_boost

struct Work
    x, t : int

[export]
def test
    with_job_que <|
        // jobs and status
        with_job_status(5) <| $ ( status )
            for x in range(5)
                var y <- [{for t in range(3); t+x}]
                new_job <| @ [[<-y]]
                    assert(length(y)==3)
                    for t,c in range(3),y
                        assert(t+x==c)
                    status |> notify_and_release
            status |> join
        // channels (foreach)
        with_channel(5) <| $ ( channel )
            for x in range(5)
                new_job <| @
                    for t in range(3)
                        channel |> push_clone ( [[Work x=x, t=t]] )
                    channel |> notify_and_release                               // at this point we are done with the channel on this job
            var summ = 0
            channel |> for_each_clone <| $ ( w : Work# )
                summ += w.x * w.t
            assert(summ==30)
            assert(channel.isEmpty)
            assert(channel.isReady)
        // more channels (for)
        with_channel(5) <| $ ( channel )
            for x in range(5)
                new_job <| @
                    for t in range(3)
                        channel |> push_clone ( [[Work x=x, t=t]] )
                    channel |> notify_and_release                               // at this point we are done with the channel on this job
            var summ = 0
            for w in each_clone(channel,type<Work>)
                summ += w.x * w.t
            assert(summ==30)
            assert(channel.isEmpty)
            assert(channel.isReady)
    return true

