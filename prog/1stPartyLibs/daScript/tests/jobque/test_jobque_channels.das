options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/jobque_boost

// Channel boost helpers (push_clone, for_each_clone, etc.) require struct types.
// POD types (int, float, int2) cannot be used with `new TT` or `delete temp`.
struct IntVal {
    v : int
}

struct PairVal {
    x : int
    y : int
}

// Note: with_channel() creates a channel with mRemaining=0 (immediately "ready").
// Pop from such a channel returns items while the pipe is non-empty, and returns
// null when the pipe is empty. This is the correct pattern for direct push/pop
// without jobs.
//
// with_channel(N) creates a channel expecting N notify() calls before pop stops
// blocking. This is used with new_job/new_thread patterns where each job notifies.

[test]
def test_channel_basics(t : T?) {
    t |> run("with_channel creates and destroys") @(t : T?) {
        var entered = false
        with_channel() $(ch) {
            entered = true
            t |> equal(true, ch.isEmpty)
            t |> equal(true, ch.isReady)
            t |> equal(0, ch.size)
        }
        t |> equal(true, entered)
    }
    t |> run("with_channel with count") @(t : T?) {
        with_channel(3) $(ch) {
            t |> equal(true, ch.isEmpty)
            t |> equal(false, ch.isReady)
            t |> equal(3, ch.size)
            // notify 3 times to make it ready
            ch |> notify
            ch |> notify
            ch |> notify
            t |> equal(true, ch.isReady)
        }
    }
}

[test]
def test_channel_push_pop(t : T?) {
    t |> run("push_clone and pop_and_clone_one") @(t : T?) {
        with_channel() $(ch) {
            ch |> push_clone(IntVal(v = 42))
            ch |> push_clone(IntVal(v = 99))
            t |> equal(false, ch.isEmpty)
            t |> equal(2, ch.total)
            var got_first = false
            ch |> pop_and_clone_one() $(val : IntVal#) {
                t |> equal(42, val.v)
                got_first = true
            }
            t |> equal(true, got_first)
            var got_second = false
            ch |> pop_and_clone_one() $(val : IntVal#) {
                t |> equal(99, val.v)
                got_second = true
            }
            t |> equal(true, got_second)
            // channel pipe should now be empty
            t |> equal(true, ch.isEmpty)
            // pop from empty should return false
            var got_empty = false
            ch |> pop_and_clone_one() $(val : IntVal#) {
                got_empty = true
            }
            t |> equal(false, got_empty)
        }
    }
    t |> run("push_clone with pair data") @(t : T?) {
        with_channel() $(ch) {
            ch |> push_clone(PairVal(x = 10, y = 20))
            ch |> push_clone(PairVal(x = 30, y = 40))
            var results : array<PairVal>
            ch |> for_each_clone() $(val : PairVal#) {
                results |> push(PairVal(x = val.x, y = val.y))
            }
            t |> equal(2, length(results))
            t |> equal(10, results[0].x)
            t |> equal(20, results[0].y)
            t |> equal(30, results[1].x)
            t |> equal(40, results[1].y)
        }
    }
}

[test]
def test_channel_for_each_clone(t : T?) {
    t |> run("for_each_clone reads all pushed values") @(t : T?) {
        with_channel() $(ch) {
            for (i in range(5)) {
                ch |> push_clone(IntVal(v = i))
            }
            t |> equal(5, ch.total)
            var results : array<int>
            ch |> for_each_clone() $(val : IntVal#) {
                results |> push(val.v)
            }
            t |> equal(5, length(results))
            for (i in range(5)) {
                t |> equal(i, results[i])
            }
        }
    }
    t |> run("for_each_clone on empty channel") @(t : T?) {
        with_channel() $(ch) {
            var count = 0
            ch |> for_each_clone() $(val : IntVal#) {
                count += 1
            }
            t |> equal(0, count)
        }
    }
}

[test]
def test_channel_gather(t : T?) {
    t |> run("gather reads all pushed values") @(t : T?) {
        with_channel() $(ch) {
            ch |> push_clone(IntVal(v = 10))
            ch |> push_clone(IntVal(v = 20))
            ch |> push_clone(IntVal(v = 30))
            var sum = 0
            ch |> gather() $(val : IntVal#) {
                sum += val.v
            }
            t |> equal(60, sum)
        }
    }
}

[test]
def test_channel_peek(t : T?) {
    t |> run("peek reads without consuming") @(t : T?) {
        with_channel() $(ch) {
            ch |> push_clone(IntVal(v = 42))
            ch |> push_clone(IntVal(v = 99))
            var peeked : array<int>
            ch |> peek() $(val : IntVal#) {
                peeked |> push(val.v)
            }
            t |> equal(2, length(peeked))
            t |> equal(42, peeked[0])
            t |> equal(99, peeked[1])
            // values should still be in channel
            t |> equal(2, ch.total)
        }
    }
}

[test]
def test_channel_each_clone_iterator(t : T?) {
    t |> run("each_clone iterator") @(t : T?) {
        with_channel() $(ch) {
            ch |> push_clone(IntVal(v = 100))
            ch |> push_clone(IntVal(v = 200))
            ch |> push_clone(IntVal(v = 300))
            var results : array<int>
            for (val in each_clone(ch, type<IntVal>)) {
                results |> push(val.v)
            }
            t |> equal(3, length(results))
            t |> equal(100, results[0])
            t |> equal(200, results[1])
            t |> equal(300, results[2])
        }
    }
}

[test]
def test_channel_push_batch_clone(t : T?) {
    t |> run("push_batch_clone pushes all items") @(t : T?) {
        with_channel() $(ch) {
            var items <- [IntVal(v=10), IntVal(v=20), IntVal(v=30)]
            ch |> push_batch_clone(items)
            t |> equal(3, ch.total)
            var results : array<int>
            ch |> for_each_clone() $(val : IntVal#) {
                results |> push(val.v)
            }
            t |> equal(3, length(results))
            t |> equal(10, results[0])
            t |> equal(20, results[1])
            t |> equal(30, results[2])
        }
    }
}

[test]
def test_channel_ordering(t : T?) {
    t |> run("FIFO ordering preserved") @(t : T?) {
        with_channel() $(ch) {
            for (i in range(10)) {
                ch |> push_clone(IntVal(v = i * 10))
            }
            var results : array<int>
            ch |> for_each_clone() $(val : IntVal#) {
                results |> push(val.v)
            }
            for (i in range(10)) {
                t |> equal(i * 10, results[i])
            }
        }
    }
}
