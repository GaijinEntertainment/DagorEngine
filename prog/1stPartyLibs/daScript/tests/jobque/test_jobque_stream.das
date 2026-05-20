options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/jobque_boost
require daslib/archive

struct CmdStruct {
    id : int
    name : string
    payload : array<int>
}

variant CmdVariant {
    play     : tuple<id : int; name : string>
    stop     : int
    handle   : tuple<id : int; ptr : uint64>
    shutdown : bool
}

[test]
def test_stream_basics(t : T?) {
    t |> run("with_stream creates and destroys") @(t : T?) {
        var entered = false
        with_stream() $(var s : Stream?) {
            entered = true
            t |> equal(true, s.isEmpty)
            t |> equal(true, s.isReady)
            t |> equal(0, s.total)
        }
        t |> equal(true, entered)
    }
    t |> run("with_stream with count") @(t : T?) {
        with_stream(3) $(var s : Stream?) {
            t |> equal(false, s.isReady)
            t |> equal(3, s.size)
            s |> notify
            s |> notify
            s |> notify
            t |> equal(true, s.isReady)
        }
    }
}

[test]
def test_stream_push_pop_bytes(t : T?) {
    t |> run("push + try_pop round-trip bytes") @(t : T?) {
        with_stream() $(var s : Stream?) {
            var bytes = [1u8, 2u8, 3u8, 4u8, 5u8]
            s |> push(bytes)
            t |> equal(1, s.total)
            var popped_len = 0
            var popped_first = 0u8
            var popped_last = 0u8
            let ok = s |> try_pop() $(view : array<uint8># -const) {
                popped_len = length(view)
                popped_first = view[0]
                popped_last = view[4]
            }
            t |> equal(true, ok)
            t |> equal(5, popped_len)
            t |> equal(1u8, popped_first)
            t |> equal(5u8, popped_last)
            t |> equal(0, s.total)
        }
    }
    t |> run("try_pop on empty stream returns false") @(t : T?) {
        with_stream() $(var s : Stream?) {
            var called = false
            let ok = s |> try_pop() $(view : array<uint8># -const) {
                called = true
            }
            t |> equal(false, ok)
            t |> equal(false, called)
        }
    }
    t |> run("FIFO ordering") @(t : T?) {
        with_stream() $(var s : Stream?) {
            for (i in range(5)) {
                var b = [uint8(i * 2)]
                s |> push(b)
            }
            t |> equal(5, s.total)
            var got : array<int>
            s |> gather() $(view : array<uint8># -const) {
                got |> push(int(view[0]))
            }
            t |> equal(5, length(got))
            for (i in range(5)) {
                t |> equal(i * 2, got[i])
            }
        }
    }
}

[test]
def test_stream_gather_peek(t : T?) {
    t |> run("gather drains stream") @(t : T?) {
        with_stream() $(var s : Stream?) {
            var a = [10u8]
            s |> push(a)
            var b = [20u8]
            s |> push(b)
            var c = [30u8]
            s |> push(c)
            t |> equal(3, s.total)
            var sum = 0
            s |> gather() $(view : array<uint8># -const) {
                sum += int(view[0])
            }
            t |> equal(60, sum)
            t |> equal(0, s.total)
        }
    }
    t |> run("peek preserves stream") @(t : T?) {
        with_stream() $(var s : Stream?) {
            var a = [42u8]
            s |> push(a)
            var b = [99u8]
            s |> push(b)
            var peeked : array<int>
            s |> peek() $(view : array<uint8># -const) {
                peeked |> push(int(view[0]))
            }
            t |> equal(2, length(peeked))
            t |> equal(42, peeked[0])
            t |> equal(99, peeked[1])
            t |> equal(2, s.total)
        }
    }
}

[test]
def test_stream_archive_struct(t : T?) {
    t |> run("push_archive + pop_archive struct round-trip") @(t : T?) {
        with_stream() $(var s : Stream?) {
            s |> push_archive(CmdStruct(id = 7, name = "kick", payload <- [1, 2, 3, 4]))
            var got = false
            s |> pop_archive() $(var c : CmdStruct) {
                got = true
                t |> equal(7, c.id)
                t |> equal("kick", c.name)
                t |> equal(4, length(c.payload))
                t |> equal(3, c.payload[2])
            }
            t |> equal(true, got)
            t |> equal(0, s.total)
        }
    }
    t |> run("gather_archive invokes block per message in FIFO order") @(t : T?) {
        with_stream() $(var s : Stream?) {
            for (i in range(4)) {
                s |> push_archive(CmdStruct(id = i, name = "n", payload <- [i * 100]))
            }
            var ids : array<int>
            s |> gather_archive() $(var c : CmdStruct) {
                ids |> push(c.id)
            }
            t |> equal(4, length(ids))
            for (i in range(4)) {
                t |> equal(i, ids[i])
            }
            t |> equal(0, s.total)
        }
    }
}

[test]
def test_stream_archive_variant(t : T?) {
    t |> run("variant with uint64 handles serializes end-to-end") @(t : T?) {
        with_stream() $(var s : Stream?) {
            s |> push_archive(CmdVariant(play = (id = 1, name = "kick")))
            s |> push_archive(CmdVariant(stop = 42))
            s |> push_archive(CmdVariant(handle = (id = 99, ptr = 0xDEADBEEFul)))
            s |> push_archive(CmdVariant(shutdown = true))
            var plays = 0
            var stops = 0
            var handles = 0
            var shutdowns = 0
            var last_ptr = 0ul
            s |> gather_archive() $(var c : CmdVariant) {
                if (c is play) {
                    plays += 1
                } elif (c is stop) {
                    stops += 1
                } elif (c is handle) {
                    handles += 1
                    last_ptr = (c as handle).ptr
                } elif (c is shutdown) {
                    shutdowns += 1
                }
            }
            t |> equal(1, plays)
            t |> equal(1, stops)
            t |> equal(1, handles)
            t |> equal(1, shutdowns)
            t |> equal(0xDEADBEEFul, last_ptr)
        }
    }
}

[test]
def test_stream_try_pop_archive(t : T?) {
    t |> run("try_pop_archive on non-empty stream returns true") @(t : T?) {
        with_stream() $(var s : Stream?) {
            s |> push_archive(CmdStruct(id = 5, name = "x", payload <- [9]))
            var got = false
            let ok = s |> try_pop_archive() $(var c : CmdStruct) {
                got = true
                t |> equal(5, c.id)
            }
            t |> equal(true, ok)
            t |> equal(true, got)
        }
    }
    t |> run("try_pop_archive on empty stream returns false") @(t : T?) {
        with_stream() $(var s : Stream?) {
            var got = false
            let ok = s |> try_pop_archive() $(var c : CmdStruct) {
                got = true
            }
            t |> equal(false, ok)
            t |> equal(false, got)
        }
    }
}

[test]
def test_stream_push_batch(t : T?) {
    t |> run("push_batch adds all items") @(t : T?) {
        with_stream() $(var s : Stream?) {
            var batch : array<array<uint8>>
            var a : array<uint8>
            a |> push(11u8)
            batch |> emplace(a)
            var b : array<uint8>
            b |> push(22u8)
            batch |> emplace(b)
            var c : array<uint8>
            c |> push(33u8)
            batch |> emplace(c)
            s |> push_batch(batch)
            t |> equal(3, s.total)
            var got : array<int>
            s |> gather() $(view : array<uint8># -const) {
                got |> push(int(view[0]))
            }
            t |> equal(3, length(got))
            t |> equal(11, got[0])
            t |> equal(22, got[1])
            t |> equal(33, got[2])
        }
    }
}
