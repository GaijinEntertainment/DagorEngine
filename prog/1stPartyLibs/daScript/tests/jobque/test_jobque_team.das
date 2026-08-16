options gen2
require dastest/testing_boost public
require daslib/jobque_boost
require math

// Correctness of team mode (set_jobque_team_mode + team_parallel_for): workers self-serve chunks
// off one published atomic instead of per-chunk fifo jobs. Checks the fallback (mode off → one
// inline invoke), coverage/exactly-once across chunk shapes, back-to-back dispatch storms
// (seq-bump reuse, stragglers), fifo parallel_for still working while team mode is on, mode
// toggling (workers park again after off), the dormant worker limit, and the per-op rank gate.

struct private LimitProbe {
    v : int
}

def team_sum_round(t : T?; nj, total : int) {
    var results : array<int>
    results |> resize(total)
    var pr = unsafe(addr(results[0]))
    team_parallel_for(0, total, nj) <| @(job_begin : int; job_end : int) {
        for (i in range(job_begin, job_end)) {
            unsafe(pr[i]) = i * 3 + 1
        }
    }
    var sum = 0
    for (i in range(total)) {
        sum += results[i]
    }
    t |> equal(3 * total * (total - 1) / 2 + total, sum)
    delete results
}

def team_once_round(t : T?; nj, total : int) {
    // every index written exactly once — catches both dropped and double-served chunks
    var hits : array<int>
    hits |> resize(total)
    var pr = unsafe(addr(hits[0]))
    team_parallel_for(0, total, nj) <| @(job_begin : int; job_end : int) {
        for (i in range(job_begin, job_end)) {
            unsafe(pr[i]) += 1
        }
    }
    var bad = 0
    for (i in range(total)) {
        if (hits[i] != 1) {
            bad++
        }
    }
    t |> equal(0, bad)
    delete hits
}

def fifo_sum_round(t : T?; nj, total : int) {
    // plain fifo parallel_for — must keep working while workers also poll the team slot
    var results : array<int>
    results |> resize(total)
    var pr = unsafe(addr(results[0]))
    parallel_for(0, total, nj) $(job_begin, job_end, wg) {
        for (i in range(job_begin, job_end)) {
            unsafe(pr[i]) = i * 3 + 1
        }
        wg |> notify_and_release
    }
    var sum = 0
    for (i in range(total)) {
        sum += results[i]
    }
    t |> equal(3 * total * (total - 1) / 2 + total, sum)
    delete results
}

def team_epoch_storm(t : T?; rounds : int) {
    // the straggler epoch shape: K == lanes means every lane's claim loop ends in an overflow
    // fetch_add right as the join releases, and the next (bigger-K) op publishes with no gap.
    // Pre-fix a straggler could pair its old-epoch chunk index with the new op's chunk count —
    // dangling-work crash or lost decrement (join hang). Exactly-once verified per round.
    let lanes = get_total_hw_jobs() + 1
    let halfK = max(lanes / 2, 2)
    let bigK = 4 * lanes
    var hits : array<int>
    hits |> resize(bigK)
    var pr = unsafe(addr(hits[0]))
    for (_r in range(rounds)) {
        for (i in range(bigK)) {
            hits[i] = 0
        }
        team_parallel_for(0, halfK, halfK) <| @(b : int; e : int) {
            for (i in range(b, e)) {
                unsafe(pr[i]) += 1
            }
        }
        team_parallel_for(0, lanes, lanes) <| @(b : int; e : int) {
            for (i in range(b, e)) {
                unsafe(pr[i]) += 1
            }
        }
        team_parallel_for(0, bigK, bigK) <| @(b : int; e : int) {
            for (i in range(b, e)) {
                unsafe(pr[i]) += 1
            }
        }
        var bad = 0
        for (i in range(bigK)) {
            let want = i < halfK ? 3 : (i < lanes ? 2 : 1)
            if (hits[i] != want) {
                bad++
            }
        }
        t |> equal(0, bad)
    }
    delete hits
}

def team_early_return_round(t : T?; nj, total : int) {
    // chunk lambdas that RETURN early — pins the invoke fastpath's stopFlags clear (a leaked
    // stopForReturn would make the serial code after the dispatch misbehave)
    var hits : array<int>
    hits |> resize(total)
    var pr = unsafe(addr(hits[0]))
    team_parallel_for(0, total, nj) <| @(job_begin : int; job_end : int) {
        for (i in range(job_begin, job_end)) {
            unsafe(pr[i]) += 1
        }
        return                              // explicit early return out of the chunk lambda
    }
    var bad = 0
    for (i in range(total)) {
        if (hits[i] != 1) {
            bad++
        }
    }
    t |> equal(0, bad)                      // this line must RUN — a leaked stop flag skips it
    delete hits
}

def team_stages_round(t : T?; wide : int) {
    // multi-stage chain with a wide middle stage, then a tiny single-stage op right after —
    // under the rank gate the tiny op's admit set must come from ITS chunk count, not the wide
    // chain's stale stage slots (publish zeroes the unused ones), and gated stragglers must
    // still clear every stage barrier. Exactly-once across all three stage ranges.
    let n0 = 16
    let n1 = 4 * wide
    let total = n0 + n1 + 8
    var hits : array<int>
    hits |> resize(total)
    var pr = unsafe(addr(hits[0]))
    team_parallel_stages([int3(0, n0, 4), int3(n0, n0 + n1, wide), int3(n0 + n1, total, 2)]) <| @(stage : int; b : int; e : int) {
        for (i in range(b, e)) {
            unsafe(pr[i]) += 1
        }
    }
    team_sum_round(t, 2, 8)
    var bad = 0
    for (i in range(total)) {
        if (hits[i] != 1) {
            bad++
        }
    }
    t |> equal(0, bad)
    delete hits
}

def team_storm(t : T?; rounds : int) {
    // back-to-back dispatches at several shapes: 1 chunk (inline path), fewer chunks than items,
    // more chunks than workers, more chunks than items (clamp) — exercises seq reuse + stragglers
    for (_r in range(rounds)) {
        team_sum_round(t, 1, 4)
        team_sum_round(t, 4, 16)
        team_sum_round(t, 2 * get_total_hw_jobs(), 4 * get_total_hw_jobs())
        team_once_round(t, 4 * get_total_hw_jobs(), 8 * get_total_hw_jobs())
        team_once_round(t, 64, 7)
    }
}

def team_indexed_round(t : T?; nj, total : int) {
    // team_parallel_for_indexed contract: exactly-once coverage, slot within
    // [0, get_total_hw_jobs()] (the caller is the last slot), and per-slot scratch — each
    // invocation accumulates into its slot's cell with a plain unsynchronized +=, so a slot
    // shared by two in-flight invocations would lose updates and break the checksum
    let nslots = get_total_hw_jobs() + 1
    var hits : array<int>
    hits |> resize(total)
    var pr = unsafe(addr(hits[0]))
    var acc : array<int>
    acc |> resize(nslots + 1)   // +1 guard cell — out-of-range slots land there
    var pa = unsafe(addr(acc[0]))
    team_parallel_for_indexed(0, total, nj) <| @(slot : int; job_begin : int; job_end : int) {
        let cell = (slot >= 0 && slot < nslots) ? slot : nslots
        for (i in range(job_begin, job_end)) {
            unsafe(pr[i]) += 1
            unsafe(pa[cell]) += i * 3 + 1
        }
    }
    var bad = 0
    for (i in range(total)) {
        if (hits[i] != 1) {
            bad++
        }
    }
    t |> equal(0, bad)
    t |> equal(0, acc[nslots])   // no out-of-range slot fired
    var sum = 0
    for (s in range(nslots)) {
        sum += acc[s]
    }
    t |> equal(3 * total * (total - 1) / 2 + total, sum)
    delete hits
    delete acc
}

def team_indexed_single_invoke_round(t : T?; nj, total : int) {
    // degenerate paths (team mode off, or a single chunk): ONE invoke, slot 0, whole range
    var stats : array<int>   // [invocations, non-zero-slot invocations, index checksum]
    stats |> resize(3)
    var ps = unsafe(addr(stats[0]))
    team_parallel_for_indexed(0, total, nj) <| @(slot : int; job_begin : int; job_end : int) {
        unsafe(ps[0]) += 1
        if (slot != 0) {
            unsafe(ps[1]) += 1
        }
        for (i in range(job_begin, job_end)) {
            unsafe(ps[2]) += i
        }
    }
    t |> equal(1, stats[0])
    t |> equal(0, stats[1])
    t |> equal(total * (total - 1) / 2, stats[2])
    delete stats
}

def team_indexed_storm(t : T?; rounds : int) {
    for (_r in range(rounds)) {
        team_indexed_round(t, 4, 16)
        team_indexed_round(t, 2 * get_total_hw_jobs(), 4 * get_total_hw_jobs())
        team_indexed_round(t, 8 * get_total_hw_jobs(), 8 * get_total_hw_jobs())   // chunk-per-item
        team_indexed_round(t, 64, 7)
        team_indexed_single_invoke_round(t, 1, 32)
    }
}

[test]
def test_jobque_team(t : T?) {
    with_job_que() {
        t |> run("mode off - inline fallback") @(t : T?) {
            team_sum_round(t, 4, 16)
            team_once_round(t, 8, 32)
        }
        t |> run("team mode - no fork pool") @(t : T?) {
            set_jobque_team_mode(true)
            team_storm(t, 5)
            set_jobque_team_mode(false)
        }
        t |> run("team mode - fork pool") @(t : T?) {
            set_jobque_fork_pool(true, true)
            set_jobque_team_mode(true)
            team_storm(t, 10)
            set_jobque_team_mode(false)
        }
        t |> run("fifo while team mode on") @(t : T?) {
            set_jobque_team_mode(true)
            for (_r in range(5)) {
                team_sum_round(t, 4, 16)
                fifo_sum_round(t, 4, 16)
                fifo_sum_round(t, 2 * get_total_hw_jobs(), 4 * get_total_hw_jobs())
            }
            set_jobque_team_mode(false)
        }
        t |> run("early-return chunk lambda - stop flags do not leak") @(t : T?) {
            set_jobque_team_mode(true)
            team_early_return_round(t, 8, 32)
            team_early_return_round(t, 2 * get_total_hw_jobs(), 4 * get_total_hw_jobs())
            set_jobque_team_mode(false)
        }
        t |> run("workers park again after off") @(t : T?) {
            // team mode off → team_parallel_for falls back inline, fifo path unaffected
            team_sum_round(t, 4, 16)
            fifo_sum_round(t, 4, 16)
        }
        t |> run("worker limit - dormant workers never break a dispatch") @(t : T?) {
            set_jobque_team_mode(true)
            // any limit completes every dispatch: chunks self-serve, the caller drains solo.
            // limit 2 (a live sliver), limit 0 (caller-only), then unlimited again — team AND
            // fifo rounds under each, exactly-once coverage throughout.
            set_jobque_worker_limit(2)
            t |> equal(get_jobque_worker_limit(), 2)
            team_sum_round(t, 8, 32)
            team_once_round(t, 2 * get_total_hw_jobs(), 4 * get_total_hw_jobs())
            fifo_sum_round(t, 4, 16)
            set_jobque_worker_limit(0)
            team_once_round(t, 8, 32)
            set_jobque_worker_limit(-1)   // raise re-admits the dormant workers (the notify path)
            team_storm(t, 5)
            fifo_sum_round(t, 4, 16)
            set_jobque_team_mode(false)
        }
        t |> run("worker limit - a fifo notify must land on an ELIGIBLE worker") @(t : T?) {
            // regression: dormant workers once shared mCond — a push()'s notify_one could land on
            // a dormant waiter (predicate false, token consumed, re-sleeps) and the pushed job
            // stalled with every eligible worker parked. Dormant workers now wait on mLimitCond,
            // so this must complete promptly: spin 0 parks eligible workers between jobs (the
            // theft window), and new_job + a blocking channel read has no joiner work-steal to
            // paper over a lost wake (a regression HANGS here, it does not just slow down).
            set_jobque_worker_spin(0)
            set_jobque_worker_limit(2)
            for (_r in range(20)) {
                with_channel(1) $(ch) {
                    new_job() @() {
                        ch |> push_clone(LimitProbe(v = 42))
                        ch |> notify_and_release
                    }
                    var got = 0
                    ch |> for_each_clone() $(val : LimitProbe#) {
                        got = val.v
                    }
                    t |> equal(42, got)
                }
            }
            set_jobque_worker_limit(-1)
        }
        t |> run("team rank gate - per-op admit set never breaks a dispatch") @(t : T?) {
            // opt-in per-op participation gate: an op admits only chunk-count-many workers
            // (limit-rank + 1 < chunks; the caller always serves), higher ranks spin past it.
            // Pure perf dial — every admit set must stay exactly-once correct across shapes,
            // storms, epoch crossings, stage chains, and both worker-limit states.
            set_jobque_team_mode(true)
            set_jobque_team_rank_gate(true)
            t |> equal(get_jobque_team_rank_gate() ? 1 : 0, 1)
            team_sum_round(t, 2, 8)             // tiny op: one admitted worker + the caller
            team_once_round(t, 2 * get_total_hw_jobs(), 4 * get_total_hw_jobs())   // wide: everyone
            team_sum_round(t, 2, 8)             // tiny again, right after the wide one
            team_storm(t, 5)
            set_jobque_worker_spin(0)           // explicit: coverage must not depend on subtest order
            team_epoch_storm(t, 300)            // spin 0: gated workers park and re-wake per publish
            set_jobque_worker_spin(200)
            team_epoch_storm(t, 300)            // spin window: gated workers stay hot between ops
            set_jobque_worker_spin(0)
            team_stages_round(t, 2 * get_total_hw_jobs())
            set_jobque_worker_limit(2)          // compose with the dormant worker limit
            team_once_round(t, 8, 32)
            team_sum_round(t, 2, 8)
            set_jobque_worker_limit(-1)
            fifo_sum_round(t, 4, 16)            // fifo path is not gated
            set_jobque_team_rank_gate(false)
            t |> equal(get_jobque_team_rank_gate() ? 1 : 0, 0)
            team_sum_round(t, 4, 16)
            set_jobque_team_mode(false)
        }
        t |> run("epoch crossing - back-to-back shape storm") @(t : T?) {
            // production shape (spin window + fork pool, dasLLAMA decode's config)
            set_jobque_worker_spin(200)
            set_jobque_fork_pool(true, true)
            set_jobque_team_mode(true)
            team_epoch_storm(t, 5000)
            set_jobque_team_mode(false)
            set_jobque_worker_spin(0)
        }
        t |> run("production shape") @(t : T?) {
            // team mode + the fork/join knobs it will ship next to
            set_jobque_worker_spin(200)
            set_jobque_batch_dispatch(true)
            set_jobque_join_spin(2)
            set_jobque_team_mode(true)
            team_storm(t, 10)
            set_jobque_team_mode(false)
            set_jobque_join_spin(0)
            set_jobque_batch_dispatch(false)
            set_jobque_worker_spin(0)
        }
        t |> run("indexed - mode off falls back to one slot-0 invoke") @(t : T?) {
            team_indexed_single_invoke_round(t, 4, 16)
            team_indexed_single_invoke_round(t, 1, 16)
        }
        t |> run("indexed - team mode, no fork pool") @(t : T?) {
            set_jobque_team_mode(true)
            team_indexed_storm(t, 5)
            set_jobque_team_mode(false)
        }
        t |> run("indexed - team mode, fork pool") @(t : T?) {
            set_jobque_fork_pool(true, true)
            set_jobque_team_mode(true)
            team_indexed_storm(t, 10)
            set_jobque_team_mode(false)
        }
        t |> run("indexed - worker limit keeps slots in bounds") @(t : T?) {
            set_jobque_team_mode(true)
            set_jobque_worker_limit(2)
            team_indexed_round(t, 8, 32)
            team_indexed_round(t, 2 * get_total_hw_jobs(), 4 * get_total_hw_jobs())
            set_jobque_worker_limit(0)    // caller-only
            team_indexed_round(t, 8, 32)
            set_jobque_worker_limit(-1)
            team_indexed_storm(t, 5)
            set_jobque_team_mode(false)
        }
    }
}
