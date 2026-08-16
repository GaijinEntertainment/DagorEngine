options gen2
require dastest/testing_boost public
require daslib/jobque_boost

// Correctness of multi-stage team dispatch (team_parallel_stages): one rendezvous, dependent
// phases separated by worker-side barriers. Checks the inline fallback (mode off → stages in
// order), the stage-ordering invariant under storm (stage s+1 reads stage s's cross-chunk
// writes — a mirror read makes any barrier violation visible), ragged stage shapes (empty /
// single-chunk / clamped stages inside a chain), and interleaving with single-op
// team_parallel_for (both protocols share the publish slots).

def stages_pipeline_round(t : T?; nj0, nj1, total, rounds : int) {
    // stage 1 chunk j reads stage 0's MIRROR element — guaranteed cross-chunk for any split, so
    // a stage barrier violation (stale read) shows up as a wrong value after the join
    var a : array<int>
    var b : array<int>
    a |> resize(total)
    b |> resize(total)
    var pa = unsafe(addr(a[0]))
    var pb = unsafe(addr(b[0]))
    let stages <- [int3(0, total, nj0), int3(0, total, nj1)]
    for (r in range(1, rounds + 1)) {
        team_parallel_stages(stages) <| @(stage : int; jb : int; je : int) {
            if (stage == 0) {
                for (i in range(jb, je)) {
                    unsafe(pa[i]) = r * 1000000 + i
                }
            } else {
                for (i in range(jb, je)) {
                    unsafe(pb[i]) = unsafe(pa[total - 1 - i])
                }
            }
        }
        var bad = 0
        for (i in range(total)) {
            if (b[i] != r * 1000000 + (total - 1 - i)) {
                bad++
            }
        }
        t |> equal(0, bad)
    }
    delete a
    delete b
}

def stages_three_round(t : T?; total, rounds : int) {
    // 3-stage chain, each stage mirror-reads the previous — after two mirrors c[i] traces back to
    // a[i], so every element checks both barriers
    let lanes = get_total_hw_jobs() + 1
    var a : array<int>
    var b : array<int>
    var c : array<int>
    a |> resize(total)
    b |> resize(total)
    c |> resize(total)
    var pa = unsafe(addr(a[0]))
    var pb = unsafe(addr(b[0]))
    var pc = unsafe(addr(c[0]))
    let stages <- [int3(0, total, lanes), int3(0, total, 4 * lanes), int3(0, total, lanes)]
    for (r in range(1, rounds + 1)) {
        team_parallel_stages(stages) <| @(stage : int; jb : int; je : int) {
            if (stage == 0) {
                for (i in range(jb, je)) {
                    unsafe(pa[i]) = r * 1000000 + i
                }
            } elif (stage == 1) {
                for (i in range(jb, je)) {
                    unsafe(pb[i]) = unsafe(pa[total - 1 - i]) + 1
                }
            } else {
                for (i in range(jb, je)) {
                    unsafe(pc[i]) = unsafe(pb[total - 1 - i]) + 1
                }
            }
        }
        var bad = 0
        for (i in range(total)) {
            if (c[i] != r * 1000000 + i + 2) {
                bad++
            }
        }
        t |> equal(0, bad)
    }
    delete a
    delete b
    delete c
}

def stages_ragged_round(t : T?) {
    // ragged chain: wide stage, EMPTY stage (zero range — pure barrier), single-chunk stage,
    // clamped stage (more chunks than items) — all inside one chain
    let total = 96
    var a : array<int>
    var b : array<int>
    a |> resize(total)
    b |> resize(total)
    var pa = unsafe(addr(a[0]))
    var pb = unsafe(addr(b[0]))
    let stages <- [int3(0, total, 16), int3(5, 5, 4), int3(0, 1, 1), int3(0, total, 4 * total)]
    team_parallel_stages(stages) <| @(stage : int; jb : int; je : int) {
        if (stage == 0) {
            for (i in range(jb, je)) {
                unsafe(pa[i]) = i * 7
            }
        } elif (stage == 2) {
            unsafe(pa[0]) += 1   // single chunk: [0,1)
        } elif (stage == 3) {
            for (i in range(jb, je)) {
                unsafe(pb[i]) = unsafe(pa[total - 1 - i])
            }
        }
    }
    var bad = 0
    for (i in range(total)) {
        let src = total - 1 - i
        let want = src * 7 + (src == 0 ? 1 : 0)
        if (b[i] != want) {
            bad++
        }
    }
    t |> equal(0, bad)
    delete a
    delete b
}

def stages_mixed_storm(t : T?; rounds : int) {
    // chains and single-op team dispatches back-to-back with zero gap — both share the publish
    // slots, so this is the cross-protocol straggler shape
    let lanes = get_total_hw_jobs() + 1
    let total = 4 * lanes
    var hits : array<int>
    var a : array<int>
    var b : array<int>
    hits |> resize(total)
    a |> resize(total)
    b |> resize(total)
    var ph = unsafe(addr(hits[0]))
    var pa = unsafe(addr(a[0]))
    var pb = unsafe(addr(b[0]))
    let stages <- [int3(0, total, lanes), int3(0, total, 4 * lanes)]
    for (r in range(1, rounds + 1)) {
        for (i in range(total)) {
            hits[i] = 0
        }
        team_parallel_for(0, lanes, lanes) <| @(jb : int; je : int) {
            for (i in range(jb, je)) {
                unsafe(ph[i]) += 1
            }
        }
        team_parallel_stages(stages) <| @(stage : int; jb : int; je : int) {
            if (stage == 0) {
                for (i in range(jb, je)) {
                    unsafe(pa[i]) = r + i
                }
            } else {
                for (i in range(jb, je)) {
                    unsafe(pb[i]) = unsafe(pa[total - 1 - i])
                }
            }
        }
        team_parallel_for(0, total, 64) <| @(jb : int; je : int) {
            for (i in range(jb, je)) {
                unsafe(ph[i]) += 1
            }
        }
        var bad = 0
        for (i in range(total)) {
            if (hits[i] != (i < lanes ? 2 : 1)) {
                bad++
            }
            if (b[i] != r + total - 1 - i) {
                bad++
            }
        }
        t |> equal(0, bad)
    }
    delete hits
    delete a
    delete b
}

[test]
def test_jobque_stages(t : T?) {
    with_job_que() {
        t |> run("mode off - stages run in order inline") @(t : T?) {
            stages_pipeline_round(t, 4, 8, 32, 3)
            stages_ragged_round(t)
        }
        t |> run("stage barrier - mirror dependency") @(t : T?) {
            set_jobque_team_mode(true)
            let lanes = get_total_hw_jobs() + 1
            stages_pipeline_round(t, lanes, 4 * lanes, 8 * lanes, 200)
            stages_pipeline_round(t, 64, 7, 96, 50)
            set_jobque_team_mode(false)
        }
        t |> run("three stages + ragged shapes") @(t : T?) {
            set_jobque_team_mode(true)
            let lanes = get_total_hw_jobs() + 1
            stages_three_round(t, 4 * lanes, 100)
            for (_r in range(20)) {
                stages_ragged_round(t)
            }
            set_jobque_team_mode(false)
        }
        t |> run("chain storm - production shape") @(t : T?) {
            // production config (spin window + fork pool, dasLLAMA decode's setup): the
            // K==lanes chain chased by single ops with zero gap is the straggler shape the
            // publish-side entrant gate must survive
            set_jobque_worker_spin(200)
            set_jobque_fork_pool(true, true)
            set_jobque_team_mode(true)
            stages_mixed_storm(t, 3000)
            set_jobque_team_mode(false)
            set_jobque_worker_spin(0)
        }
    }
}
