options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/jobque_boost

// SeqBox is the lock-free sibling of LockBox: a POD snapshot that one side publishes and any
// number of others copy, with neither side ever waiting. These pin the parts a caller can observe
// — that a read consumes nothing, that an empty or wrongly-sized read fails instead of handing
// back garbage, and that the box is deleted by whoever drops the last reference rather than by a
// designated owner who must go last.

struct Snap {
    tag : int
    a : uint64
    b : uint64
    n : int
}

[test]
def test_empty_box_reads_nothing(t : T?) {
    t |> run("a box with nothing published reads back false") @(t : T?) {
        with_seq_box() $(box) {
            t |> equal(box.isValid, true)
            t |> equal(box.hasValue, false)
            var entered = false
            let got = box |> read() $(s : Snap) {
                entered = true
            }
            t |> equal(got, false)
            t |> equal(entered, false, "the block must not run when there is nothing to read")
        }
    }
}

[test]
def test_publish_then_read(t : T?) {
    t |> run("read hands back exactly what was published") @(t : T?) {
        with_seq_box() $(box) {
            box |> publish(Snap(tag = 7, a = 4242ul, b = 99ul, n = -3))
            t |> equal(box.hasValue, true)
            var seen = Snap()
            let got = box |> read() $(s : Snap) {
                seen = s
            }
            t |> equal(got, true)
            t |> equal(seen.tag, 7)
            t |> equal(seen.a, 4242ul)
            t |> equal(seen.b, 99ul)
            t |> equal(seen.n, -3)
        }
    }
}

[test]
def test_read_consumes_nothing(t : T?) {
    t |> run("the same snapshot reads back until the next publish") @(t : T?) {
        with_seq_box() $(box) {
            box |> publish(Snap(tag = 1, a = 10ul, b = 0ul, n = 0))
            var first = 0
            var second = 0
            box |> read() $(s : Snap) {
                first = s.tag
            }
            box |> read() $(s : Snap) {
                second = s.tag
            }
            t |> equal(first, 1)
            t |> equal(second, 1)
            box |> publish(Snap(tag = 2, a = 20ul, b = 0ul, n = 0))
            var third = 0
            box |> read() $(s : Snap) {
                third = s.tag
            }
            t |> equal(third, 2, "publish replaces the snapshot")
        }
    }
}

[test]
def test_clear_empties_the_box(t : T?) {
    t |> run("clear drops the snapshot") @(t : T?) {
        with_seq_box() $(box) {
            box |> publish(Snap(tag = 5, a = 0ul, b = 0ul, n = 0))
            box |> clear
            t |> equal(box.hasValue, false)
            let got = box |> read() $(s : Snap) {
                pass
            }
            t |> equal(got, false)
        }
    }
}

[test]
def test_size_mismatch_is_refused(t : T?) {
    t |> run("reading as a different-sized type fails instead of reinterpreting") @(t : T?) {
        with_seq_box() $(box) {
            box |> publish(Snap(tag = 3, a = 1ul, b = 2ul, n = 4))
            var entered = false
            let got = box |> read() $(s : IntVal) {
                entered = true
            }
            t |> equal(got, false)
            t |> equal(entered, false)
        }
    }
}

[test]
def test_equal_size_read_reinterprets(t : T?) {
    t |> run("an equal-size different type reads back byte-reinterpreted, as documented") @(t : T?) {
        with_seq_box() $(box) {
            box |> publish(PairA(x = 7, y = -9))
            var entered = false
            let got = box |> read() $(s : PairB) {
                entered = true
                t |> equal(s.u, 7)
                t |> equal(s.v, -9)
            }
            t |> equal(got, true, "size is the only discriminator - the read must succeed")
            t |> equal(entered, true)
        }
    }
}

[test]
def test_payload_boundary_roundtrips(t : T?) {
    t |> run("a payload of exactly SEQ_BOX_PAYLOAD bytes round-trips") @(t : T?) {
        static_assert(typeinfo sizeof(type<Full64>) == SEQ_BOX_PAYLOAD, "Full64 must fill the box exactly")
        with_seq_box() $(box) {
            var v = Full64()
            for (i in range(8)) {
                v.w[i] = uint64(i * 1000 + 7)
            }
            let ok = box |> publish(v)
            t |> equal(ok, true)
            var entered = false
            box |> read() $(s : Full64) {
                entered = true
                for (i in range(8)) {
                    t |> equal(s.w[i], uint64(i * 1000 + 7))
                }
            }
            t |> equal(entered, true)
        }
    }
}

struct IntVal {
    v : int
}

// PairA and PairB have identical size and layout on purpose: the box stores no type identity,
// so an equal-size read succeeds and reinterprets - the contract the read() doc states.
struct PairA {
    x : int
    y : int
}

struct PairB {
    u : int
    v : int
}

struct Full64 {
    w : uint64[8]
}

// A value larger than SEQ_BOX_PAYLOAD is rejected by concept_assert, so oversizing the payload is a
// compile error rather than a runtime condition — there is nothing to assert at run time.

// ---- lifetime: the last reference deletes, in either order ----

[test]
def test_owner_releases_last(t : T?) {
    t |> run("box survives until the second holder releases it") @(t : T?) {
        let before = count_jobque_leaks()
        var box = seq_box_create()
        box |> add_ref                       // stand in for the audio thread's reference
        var mine = box
        mine |> seq_box_release              // rc 2 -> 1, still alive
        box |> publish(Snap(tag = 9, a = 0ul, b = 0ul, n = 0))
        var seen = 0
        box |> read() $(s : Snap) {
            seen = s.tag
        }
        t |> equal(seen, 9, "box must still be usable while a reference remains")
        box |> seq_box_release               // rc 1 -> 0, deleted here
        t |> equal(count_jobque_leaks(), before)
    }
}

[test]
def test_other_holder_releases_last(t : T?) {
    t |> run("releasing out of order leaks nothing") @(t : T?) {
        let before = count_jobque_leaks()
        var box = seq_box_create()
        box |> add_ref
        var other = box
        box |> seq_box_release               // the creator goes FIRST — the old box would trap here
        other |> seq_box_release             // the late holder deletes
        t |> equal(count_jobque_leaks(), before)
    }
}

[test]
def test_with_seq_box_leaves_nothing(t : T?) {
    t |> run("with_seq_box balances its own reference") @(t : T?) {
        let before = count_jobque_leaks()
        with_seq_box() $(box) {
            box |> publish(Snap(tag = 1, a = 0ul, b = 0ul, n = 0))
        }
        t |> equal(count_jobque_leaks(), before)
    }
}

// ---- capture: a lambda captures the box by reference count ----

[test]
def test_capture_in_job(t : T?) {
    t |> run("a job captures the box, publishes, and releases its share explicitly") @(t : T?) {
        let before = count_jobque_leaks()
        with_job_que() {
            var box = seq_box_create()
            with_channel(1) $(ch) {
                new_job() @() {
                    box |> publish(Snap(tag = 21, a = 1ul, b = 2ul, n = 3))
                    // the captured share; the lambda finalizer asserts this release happened
                    box |> seq_box_release
                    ch |> push_clone(IntVal(v = 1))
                    ch |> notify_and_release
                }
                ch |> for_each_clone() $(val : IntVal#) {
                    pass
                }
            }
            var seen = 0
            box |> read() $(s : Snap) {
                seen = s.tag
            }
            t |> equal(seen, 21, "the snapshot published from the job must read back")
            box |> seq_box_release
        }
        t |> equal(count_jobque_leaks(), before)
    }
}
