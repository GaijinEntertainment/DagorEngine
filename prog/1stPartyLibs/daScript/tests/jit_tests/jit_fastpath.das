options gen2
require dastest/testing_boost
require daslib/jobque_boost
require jit

// The SimFunction::jitFunction invoke fastpath (dispatch arc step 1): C++ invoke paths
// (das_invoke_function / das_invoke_lambda / jit_call_or_fastcall) call the JIT wrapper
// directly when the mirror is armed, skipping callOrFastcall + the virtual SimNode_Jit::eval.
// This pins: (a) the mirror arms under -jit and always AGREES with is_jit_function,
// (b) results through the fastpath match direct calls, (c) remove_jit disarms it and the
// function still runs (interp), (d) a team-dispatched lambda (the hot production shape)
// covers its range exactly once through the fastpath.

def add_pair(a : int; b : int) : int => a + b * 3

def sum_via_invoke(fun : function<(a : int; b : int) : int>) {
    return invoke(fun, 5, 7)
}

[test]
def test_fastpath_arms(t : T?) {
    t |> run("mirror agrees with is_jit_function") @(t : T?) {
        // tolerant of the multi-program reference-tier wrinkle: armed OR not-jit, but the
        // mirror must SHADOW the node exactly
        t |> success(is_jit_function(@@add_pair) == has_jit_fastpath(@@add_pair))
    }
    t |> run("fastpath result == direct call") @(t : T?) {
        t |> equal(26, sum_via_invoke(@@add_pair))
        t |> equal(26, add_pair(5, 7))
    }
    t |> run("remove_jit disarms, function still correct") @(t : T?) {
        if (is_jit_function(@@add_pair)) {
            unsafe {
                remove_jit(@@add_pair)
            }
            t |> success(!has_jit_fastpath(@@add_pair))
            t |> equal(26, sum_via_invoke(@@add_pair))      // interp path after disarm
        } else {
            t |> success(!has_jit_fastpath(@@add_pair))     // never armed without jit
        }
    }
}

[test]
def test_fastpath_team_lambda(t : T?) {
    t |> run("team-dispatched lambda covers exactly once") @(t : T?) {
        with_job_que() {
            set_jobque_team_mode(true)
            let N = 64
            var hits : array<int>
            hits |> resize(N)
            var pr = unsafe(addr(hits[0]))
            team_parallel_for(0, N, 16) <| @(job_begin : int; job_end : int) {
                for (i in range(job_begin, job_end)) {
                    unsafe(pr[i]) += 1
                }
            }
            var bad = 0
            for (i in range(N)) {
                if (hits[i] != 1) {
                    bad++
                }
            }
            t |> equal(0, bad)
            delete hits
        }
    }
}
