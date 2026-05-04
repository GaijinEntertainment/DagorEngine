options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/functional

// ===========================
// Helper functions / lambdas for tests
// ===========================

def is_even(x : int) : bool {
    return x % 2 == 0
}

def square(x : int) : int {
    return x * x
}

def add(a, b : int) : int {
    return a + b
}

def max_of(a, b : int) : int {
    return a if (a > b)
    return b
}

def double_it(x : int) : int {
    return x * 2
}

// ===========================
// filter tests
// ===========================

[test]
def test_filter(t : T?) {

    t |> run("filter with lambda") @(t : T?) {
        var src <- [iterator for(x in range(6)); x]
        var evens <- filter(src, @(x : int) : bool { return x % 2 == 0; })
        var result : array<int>
        for (v in evens) {
            result |> push(v)
        }
        t |> equal(length(result), 3)
        t |> equal(result[0], 0)
        t |> equal(result[1], 2)
        t |> equal(result[2], 4)
    }

    t |> run("filter with function") @(t : T?) {
        var src <- [iterator for(x in range(6)); x]
        var evens <- filter(src, @@is_even)
        var result : array<int>
        for (v in evens) {
            result |> push(v)
        }
        t |> equal(length(result), 3)
        t |> equal(result[0], 0)
        t |> equal(result[1], 2)
        t |> equal(result[2], 4)
    }

    t |> run("filter removes all above 3") @(t : T?) {
        var src <- [iterator for(x in range(6)); x]
        var evens <- filter(src, @(x : int) : bool { return x > 3; })
        var result : array<int>
        for (v in evens) {
            result |> push(v)
        }
        t |> equal(length(result), 2)
        t |> equal(result[0], 4)
        t |> equal(result[1], 5)
    }

    t |> run("filter empty input") @(t : T?) {
        var src <- [iterator for(x in range(0)); x]
        var res <- filter(src, @(x : int) : bool { return true; })
        var cnt = 0
        for (v in res) {
            cnt ++
        }
        t |> equal(cnt, 0)
    }

    t |> run("filter all rejected") @(t : T?) {
        var src <- [iterator for(x in range(5)); x]
        var res <- filter(src, @(x : int) : bool { return false; })
        var cnt = 0
        for (v in res) {
            cnt ++
        }
        t |> equal(cnt, 0)
    }
}

// ===========================
// map tests
// ===========================

[test]
def test_map(t : T?) {

    t |> run("map with lambda") @(t : T?) {
        var src <- [iterator for(x in range(4)); x]
        var squared <- map(src, @(x : int) : int { return x * x; })
        var result : array<int>
        for (v in squared) {
            result |> push(v)
        }
        t |> equal(length(result), 4)
        t |> equal(result[0], 0)
        t |> equal(result[1], 1)
        t |> equal(result[2], 4)
        t |> equal(result[3], 9)
    }

    t |> run("map with function") @(t : T?) {
        var src <- [iterator for(x in range(4)); x]
        var squared <- map(src, @@square)
        var result : array<int>
        for (v in squared) {
            result |> push(v)
        }
        t |> equal(length(result), 4)
        t |> equal(result[0], 0)
        t |> equal(result[3], 9)
    }

    t |> run("map doubling") @(t : T?) {
        var src <- [iterator for(x in range(3)); x]
        var doubled <- map(src, @(x : int) : int { return x * 2; })
        var result : array<int>
        for (v in doubled) {
            result |> push(v)
        }
        t |> equal(length(result), 3)
        t |> equal(result[0], 0)
        t |> equal(result[1], 2)
        t |> equal(result[2], 4)
    }

    t |> run("map type conversion") @(t : T?) {
        var src <- [iterator for(x in range(3)); x]
        var as_float <- map(src, @(x : int) : float { return float(x); })
        var result : array<float>
        for (v in as_float) {
            result |> push(v)
        }
        t |> equal(length(result), 3)
        t |> equal(result[0], 0.0)
        t |> equal(result[2], 2.0)
    }

    t |> run("map empty input") @(t : T?) {
        var src <- [iterator for(x in range(0)); x]
        var res <- map(src, @(x : int) : int { return x + 1; })
        var cnt = 0
        for (v in res) {
            cnt ++
        }
        t |> equal(cnt, 0)
    }
}

// ===========================
// reduce tests
// ===========================

[test]
def test_reduce(t : T?) {

    t |> run("reduce sum") @(t : T?) {
        var src <- [iterator for(x in range(1, 5)); x]
        var result = reduce(src) $(a, b : int) : int {
            return a + b
        }
        t |> equal(result, 10) // 1+2+3+4
    }

    t |> run("reduce max") @(t : T?) {
        var src <- [iterator for(x in [3, 1, 4, 1, 5]); x]
        var result = reduce(src) $(a, b : int) : int {
            return a if (a > b)
            return b
        }
        t |> equal(result, 5)
    }

    t |> run("reduce with function") @(t : T?) {
        var src <- [iterator for(x in range(1, 5)); x]
        var result = reduce(src, @@add)
        t |> equal(result, 10)
    }

    t |> run("reduce single element") @(t : T?) {
        var src <- [iterator for(x in [42]); x]
        var result = reduce(src) $(a, b : int) : int {
            return a + b
        }
        t |> equal(result, 42)
    }

    t |> run("reduce with lambda") @(t : T?) {
        var src <- [iterator for(x in range(1, 5)); x]
        var result = reduce(src, @(a, b : int) : int { return a + b; })
        t |> equal(result, 10)
    }
}

// ===========================
// reduce_or_default tests
// ===========================

[test]
def test_reduce_or_default(t : T?) {

    t |> run("reduce_or_default non-empty") @(t : T?) {
        var src <- [iterator for(x in range(1, 5)); x]
        var result = reduce_or_default(src, @(a, b : int) : int { return a + b; }, -1)
        t |> equal(result, 10)
    }

    t |> run("reduce_or_default empty") @(t : T?) {
        var src <- [iterator for(x in range(0)); x]
        var result = reduce_or_default(src, @(a, b : int) : int { return a + b; }, -1)
        t |> equal(result, -1)
    }

    t |> run("reduce_or_default with function") @(t : T?) {
        var src <- [iterator for(x in range(0)); x]
        var result = reduce_or_default(src, @@add, 99)
        t |> equal(result, 99)
    }

    t |> run("reduce_or_default with block") @(t : T?) {
        var src <- [iterator for(x in range(0)); x]
        var result = reduce_or_default(src, -1) $(a, b : int) : int {
            return a + b
        }
        t |> equal(result, -1)
    }
}

// ===========================
// fold tests
// ===========================

[test]
def test_fold(t : T?) {

    t |> run("fold sum with seed") @(t : T?) {
        var src <- [iterator for(x in range(1, 5)); x]
        var result = fold(src, 100) $(acc, x : int) : int {
            return acc + x
        }
        t |> equal(result, 110) // 100+1+2+3+4
    }

    t |> run("fold empty with seed") @(t : T?) {
        var src <- [iterator for(x in range(0)); x]
        var result = fold(src, 42) $(acc, x : int) : int {
            return acc + x
        }
        t |> equal(result, 42) // just the seed
    }

    t |> run("fold string concat") @(t : T?) {
        var src <- [iterator for(x in [1, 2, 3]); x]
        var result = fold(src, "numbers:") $(acc : string; x : int) : string {
            return "{acc} {x}"
        }
        t |> equal(result, "numbers: 1 2 3")
    }

    t |> run("fold with lambda") @(t : T?) {
        var src <- [iterator for(x in range(1, 4)); x]
        var result = fold(src, 0, @(acc, x : int) : int { return acc + x; })
        t |> equal(result, 6) // 0+1+2+3
    }

    t |> run("fold product") @(t : T?) {
        var src <- [iterator for(x in range(1, 5)); x]
        var result = fold(src, 1) $(acc, x : int) : int {
            return acc * x
        }
        t |> equal(result, 24) // 1*2*3*4
    }
}

// ===========================
// scan tests
// ===========================

[test]
def test_scan(t : T?) {

    t |> run("scan running sum") @(t : T?) {
        var src <- [iterator for(x in [1, 2, 3, 4]); x]
        var sums <- scan(src, 0, @(acc, x : int) : int { return acc + x; })
        var result : array<int>
        for (v in sums) {
            result |> push(v)
        }
        // seed=0, then 0+1=1, 1+2=3, 3+3=6, 6+4=10
        t |> equal(length(result), 5)
        t |> equal(result[0], 0)
        t |> equal(result[1], 1)
        t |> equal(result[2], 3)
        t |> equal(result[3], 6)
        t |> equal(result[4], 10)
    }

    t |> run("scan empty") @(t : T?) {
        var src <- [iterator for(x in range(0)); x]
        var sums <- scan(src, 42, @(acc, x : int) : int { return acc + x; })
        var result : array<int>
        for (v in sums) {
            result |> push(v)
        }
        t |> equal(length(result), 1) // just the seed
        t |> equal(result[0], 42)
    }

    t |> run("scan running max") @(t : T?) {
        var src <- [iterator for(x in [3, 1, 4, 1, 5, 9]); x]
        var maxs <- scan(src, 0, @(acc, x : int) : int { return acc if (acc > x); return x; })
        var result : array<int>
        for (v in maxs) {
            result |> push(v)
        }
        // 0, max(0,3)=3, max(3,1)=3, max(3,4)=4, max(4,1)=4, max(4,5)=5, max(5,9)=9
        t |> equal(length(result), 7)
        t |> equal(result[0], 0)
        t |> equal(result[1], 3)
        t |> equal(result[2], 3)
        t |> equal(result[3], 4)
        t |> equal(result[4], 4)
        t |> equal(result[5], 5)
        t |> equal(result[6], 9)
    }
}

// ===========================
// sum tests
// ===========================

[test]
def test_sum(t : T?) {

    t |> run("sum ints") @(t : T?) {
        var src <- [iterator for(x in range(1, 5)); x]
        var result = sum(src)
        t |> equal(result, 10) // 1+2+3+4
    }

    t |> run("sum empty") @(t : T?) {
        var src <- [iterator for(x in range(0)); x]
        var result = sum(src)
        t |> equal(result, 0) // default int is 0
    }

    t |> run("sum floats") @(t : T?) {
        var src <- [iterator for(x in [1.0, 2.0, 3.0]); x]
        var result = sum(src)
        t |> equal(result, 6.0)
    }
}

// ===========================
// any / all tests
// ===========================

[test]
def test_any_all(t : T?) {

    t |> run("any true") @(t : T?) {
        var src <- [iterator for(x in [false, false, true, false]); x]
        t |> equal(any(src), true)
    }

    t |> run("any false") @(t : T?) {
        var src <- [iterator for(x in [false, false, false]); x]
        t |> equal(any(src), false)
    }

    t |> run("any empty") @(t : T?) {
        var src <- [iterator for(x in range(0)); x != 0]
        t |> equal(any(src), false)
    }

    t |> run("all true") @(t : T?) {
        var src <- [iterator for(x in [true, true, true]); x]
        t |> equal(all(src), true)
    }

    t |> run("all false") @(t : T?) {
        var src <- [iterator for(x in [true, false, true]); x]
        t |> equal(all(src), false)
    }

    t |> run("all empty") @(t : T?) {
        var src <- [iterator for(x in range(0)); x != 0]
        t |> equal(all(src), true) // vacuous truth
    }

    t |> run("any on array") @(t : T?) {
        var arr = [true, false, true]
        t |> equal(any(arr), true)
    }

    t |> run("all on array") @(t : T?) {
        var arr = [true, true, true]
        t |> equal(all(arr), true)
    }
}

// ===========================
// repeat tests
// ===========================

[test]
def test_repeat(t : T?) {

    t |> run("repeat N times") @(t : T?) {
        var src <- repeat(42, 3)
        var result : array<int>
        for (v in src) {
            result |> push(v)
        }
        t |> equal(length(result), 3)
        t |> equal(result[0], 42)
        t |> equal(result[1], 42)
        t |> equal(result[2], 42)
    }

    t |> run("repeat 0 times is empty") @(t : T?) {
        var src <- repeat(42, 0)
        var cnt = 0
        for (v in src) {
            cnt ++
        }
        t |> equal(cnt, 0) // BUG FIX: was infinite before
    }

    t |> run("repeat 1 time") @(t : T?) {
        var src <- repeat("hello", 1)
        var result : array<string>
        for (v in src) {
            result |> push(v)
        }
        t |> equal(length(result), 1)
        t |> equal(result[0], "hello")
    }

    t |> run("repeat infinite (take 5)") @(t : T?) {
        var src <- repeat(7)
        var cnt = 0
        for (v in src) {
            if (cnt >= 5) {
                break
            }
            t |> equal(v, 7)
            cnt ++
        }
        t |> equal(cnt, 5)
    }
}

// ===========================
// repeat_ref tests
// ===========================

[test]
def test_repeat_ref(t : T?) {

    t |> run("repeat_ref basic") @(t : T?) {
        var src <- repeat_ref(99, 3)
        var cnt = 0
        for (v in src) {
            t |> equal(v, 99)
            cnt ++
        }
        t |> equal(cnt, 3)
    }

    t |> run("repeat_ref 0 is empty") @(t : T?) {
        var src <- repeat_ref(1, 0)
        var cnt = 0
        for (v in src) {
            cnt ++
        }
        t |> equal(cnt, 0)
    }
}

// ===========================
// cycle tests
// ===========================

[test]
def test_cycle(t : T?) {

    t |> run("cycle basic") @(t : T?) {
        var src <- [iterator for(x in [1, 2, 3]); x]
        var cyc <- cycle(src)
        var result : array<int>
        var cnt = 0
        for (v in cyc) {
            result |> push(v)
            cnt ++
            if (cnt >= 9) {
                break
            }
        }
        t |> equal(length(result), 9)
        // 1,2,3,1,2,3,1,2,3
        t |> equal(result[0], 1)
        t |> equal(result[3], 1)
        t |> equal(result[6], 1)
        t |> equal(result[8], 3)
    }
}

// ===========================
// islice tests
// ===========================

[test]
def test_islice(t : T?) {

    t |> run("islice basic") @(t : T?) {
        var src <- [iterator for(x in range(10)); x]
        var sliced <- islice(src, 2, 5)
        var result : array<int>
        for (v in sliced) {
            result |> push(v)
        }
        t |> equal(length(result), 3) // elements at index 2,3,4
        t |> equal(result[0], 2)
        t |> equal(result[1], 3)
        t |> equal(result[2], 4)
    }

    t |> run("islice start=0") @(t : T?) {
        var src <- [iterator for(x in range(5)); x]
        var sliced <- islice(src, 0, 3)
        var result : array<int>
        for (v in sliced) {
            result |> push(v)
        }
        t |> equal(length(result), 3)
        t |> equal(result[0], 0)
    }

    t |> run("islice start >= stop is empty") @(t : T?) {
        var src <- [iterator for(x in range(5)); x]
        var sliced <- islice(src, 3, 3)
        var cnt = 0
        for (v in sliced) {
            cnt ++
        }
        t |> equal(cnt, 0)
    }

    t |> run("islice beyond source") @(t : T?) {
        var src <- [iterator for(x in range(3)); x]
        var sliced <- islice(src, 0, 100)
        var result : array<int>
        for (v in sliced) {
            result |> push(v)
        }
        t |> equal(length(result), 3) // only 3 available
    }
}

// ===========================
// enumerate tests
// ===========================

[test]
def test_enumerate(t : T?) {

    t |> run("enumerate basic") @(t : T?) {
        var src <- [iterator for(x in [10, 20, 30]); x]
        var en <- enumerate(src)
        var result : array<int>
        var indices : array<int>
        for (v in en) {
            indices |> push(v._0)
            result |> push(v._1)
        }
        t |> equal(length(result), 3)
        t |> equal(indices[0], 0)
        t |> equal(indices[1], 1)
        t |> equal(indices[2], 2)
        t |> equal(result[0], 10)
        t |> equal(result[1], 20)
        t |> equal(result[2], 30)
    }

    t |> run("enumerate empty") @(t : T?) {
        var src <- [iterator for(x in range(0)); x]
        var en <- enumerate(src)
        var cnt = 0
        for (v in en) {
            cnt ++
        }
        t |> equal(cnt, 0)
    }
}

// ===========================
// for_each tests
// ===========================

[test]
def test_for_each(t : T?) {

    t |> run("for_each with block") @(t : T?) {
        var total = 0
        var src <- [iterator for(x in range(1, 5)); x]
        for_each(src) $(x : int) {
            total += x
        }
        t |> equal(total, 10)
    }

    t |> run("for_each empty") @(t : T?) {
        var total = 0
        var src <- [iterator for(x in range(0)); x]
        for_each(src) $(x : int) {
            total += x
        }
        t |> equal(total, 0)
    }
}

// ===========================
// find tests
// ===========================

[test]
def test_find(t : T?) {

    t |> run("find existing") @(t : T?) {
        var src <- [iterator for(x in [3, 7, 1, 8, 2]); x]
        var result = find(src, @(x : int) : bool { return x > 5; }, -1)
        t |> equal(result, 7)
    }

    t |> run("find missing") @(t : T?) {
        var src <- [iterator for(x in [1, 2, 3]); x]
        var result = find(src, @(x : int) : bool { return x > 100; }, -1)
        t |> equal(result, -1)
    }

    t |> run("find with block") @(t : T?) {
        var src <- [iterator for(x in [10, 20, 30]); x]
        var result = find(src, -1) $(x : int) : bool {
            return x > 15
        }
        t |> equal(result, 20)
    }

    t |> run("find empty") @(t : T?) {
        var src <- [iterator for(x in range(0)); x]
        var result = find(src, 42) $(x : int) : bool {
            return true
        }
        t |> equal(result, 42)
    }

    t |> run("find first match only") @(t : T?) {
        var src <- [iterator for(x in [1, 2, 3, 4, 5]); x]
        var result = find(src, @@is_even, -1)
        t |> equal(result, 2)
    }
}

// ===========================
// find_index tests
// ===========================

[test]
def test_find_index(t : T?) {

    t |> run("find_index found") @(t : T?) {
        var src <- [iterator for(x in [10, 20, 30, 40]); x]
        var idx = find_index(src, @(x : int) : bool { return x == 30; })
        t |> equal(idx, 2)
    }

    t |> run("find_index not found") @(t : T?) {
        var src <- [iterator for(x in [1, 2, 3]); x]
        var idx = find_index(src, @(x : int) : bool { return x == 99; })
        t |> equal(idx, -1)
    }

    t |> run("find_index empty") @(t : T?) {
        var src <- [iterator for(x in range(0)); x]
        var idx = find_index(src) $(x : int) : bool {
            return true
        }
        t |> equal(idx, -1)
    }

    t |> run("find_index first match") @(t : T?) {
        var src <- [iterator for(x in [5, 3, 5, 3]); x]
        var idx = find_index(src, @(x : int) : bool { return x == 3; })
        t |> equal(idx, 1) // first occurrence
    }
}

// ===========================
// partition tests
// ===========================

[test]
def test_partition(t : T?) {

    t |> run("partition evens/odds") @(t : T?) {
        var src <- [iterator for(x in range(6)); x]
        var result = partition(src, @@is_even)
        t |> equal(length(result._0), 3)  // evens: 0,2,4
        t |> equal(length(result._1), 3)  // odds: 1,3,5
        t |> equal(result._0[0], 0)
        t |> equal(result._0[1], 2)
        t |> equal(result._1[0], 1)
        t |> equal(result._1[1], 3)
    }

    t |> run("partition all match") @(t : T?) {
        var src <- [iterator for(x in [2, 4, 6]); x]
        var result = partition(src, @@is_even)
        t |> equal(length(result._0), 3)
        t |> equal(length(result._1), 0)
    }

    t |> run("partition none match") @(t : T?) {
        var src <- [iterator for(x in [1, 3, 5]); x]
        var result = partition(src, @@is_even)
        t |> equal(length(result._0), 0)
        t |> equal(length(result._1), 3)
    }

    t |> run("partition empty") @(t : T?) {
        var src <- [iterator for(x in range(0)); x]
        var result = partition(src, @@is_even)
        t |> equal(length(result._0), 0)
        t |> equal(length(result._1), 0)
    }

    t |> run("partition with block") @(t : T?) {
        var src <- [iterator for(x in range(5)); x]
        var result = partition(src) $(x : int) : bool {
            return x < 3
        }
        t |> equal(length(result._0), 3) // 0,1,2
        t |> equal(length(result._1), 2) // 3,4
    }
}

// ===========================
// tap tests
// ===========================

[test]
def test_tap(t : T?) {

    t |> run("tap passthrough") @(t : T?) {
        var src <- [iterator for(x in [10, 20, 30]); x]
        var tapped <- tap(src, @(x : int) { feint("{x}\n"); })
        var result : array<int>
        for (v in tapped) {
            result |> push(v)
        }
        // tap should not change values
        t |> equal(length(result), 3)
        t |> equal(result[0], 10)
        t |> equal(result[1], 20)
        t |> equal(result[2], 30)
    }

    t |> run("tap empty") @(t : T?) {
        var src <- [iterator for(x in range(0)); x]
        var tapped <- tap(src, @(x : int) { feint("{x}\n"); })
        var cnt = 0
        for (v in tapped) {
            cnt ++
        }
        t |> equal(cnt, 0)
    }
}

// ===========================
// iterate tests
// ===========================

[test]
def test_iterate(t : T?) {

    t |> run("iterate powers of 2") @(t : T?) {
        var powers <- iterate(1, @(x : int) : int { return x * 2; })
        var result : array<int>
        var cnt = 0
        for (v in powers) {
            result |> push(v)
            cnt ++
            if (cnt >= 5) {
                break
            }
        }
        // 1, 2, 4, 8, 16
        t |> equal(length(result), 5)
        t |> equal(result[0], 1)
        t |> equal(result[1], 2)
        t |> equal(result[2], 4)
        t |> equal(result[3], 8)
        t |> equal(result[4], 16)
    }

    t |> run("iterate with lambda") @(t : T?) {
        var seq <- iterate(0, @(x : int) : int { return x + 3; })
        var result : array<int>
        var cnt = 0
        for (v in seq) {
            result |> push(v)
            cnt ++
            if (cnt >= 4) {
                break
            }
        }
        // 0, 3, 6, 9
        t |> equal(result[0], 0)
        t |> equal(result[1], 3)
        t |> equal(result[2], 6)
        t |> equal(result[3], 9)
    }
}

// ===========================
// chain tests
// ===========================

[test]
def test_chain(t : T?) {

    t |> run("chain two") @(t : T?) {
        var a <- [iterator for(x in range(1, 4)); x]
        var b <- [iterator for(x in range(4, 7)); x]
        var combined <- chain(a, b)
        var result : array<int>
        for (v in combined) {
            result |> push(v)
        }
        t |> equal(length(result), 6)
        t |> equal(result[0], 1)
        t |> equal(result[3], 4)
        t |> equal(result[5], 6)
    }

    t |> run("chain first empty") @(t : T?) {
        var a <- [iterator for(x in range(0)); x]
        var b <- [iterator for(x in range(7, 9)); x]
        var combined <- chain(a, b)
        var result : array<int>
        for (v in combined) {
            result |> push(v)
        }
        t |> equal(length(result), 2)
        t |> equal(result[0], 7)
    }

    t |> run("chain second empty") @(t : T?) {
        var a <- [iterator for(x in range(1, 3)); x]
        var b <- [iterator for(x in range(0)); x]
        var combined <- chain(a, b)
        var result : array<int>
        for (v in combined) {
            result |> push(v)
        }
        t |> equal(length(result), 2)
    }

    t |> run("chain both empty") @(t : T?) {
        var a <- [iterator for(x in range(0)); x]
        var b <- [iterator for(x in range(0)); x]
        var combined <- chain(a, b)
        var cnt = 0
        for (v in combined) {
            cnt ++
        }
        t |> equal(cnt, 0)
    }
}

// ===========================
// pairwise tests
// ===========================

[test]
def test_pairwise(t : T?) {

    t |> run("pairwise basic") @(t : T?) {
        var src <- [iterator for(x in [1, 2, 3, 4]); x]
        var pairs <- pairwise(src)
        var result : array<tuple<int; int>>
        for (v in pairs) {
            result |> push(v)
        }
        // (1,2), (2,3), (3,4)
        t |> equal(length(result), 3)
        t |> equal(result[0]._0, 1)
        t |> equal(result[0]._1, 2)
        t |> equal(result[1]._0, 2)
        t |> equal(result[1]._1, 3)
        t |> equal(result[2]._0, 3)
        t |> equal(result[2]._1, 4)
    }

    t |> run("pairwise single element") @(t : T?) {
        var src <- [iterator for(x in [42]); x]
        var pairs <- pairwise(src)
        var cnt = 0
        for (v in pairs) {
            cnt ++
        }
        t |> equal(cnt, 0) // need at least 2 elements
    }

    t |> run("pairwise empty") @(t : T?) {
        var src <- [iterator for(x in range(0)); x]
        var pairs <- pairwise(src)
        var cnt = 0
        for (v in pairs) {
            cnt ++
        }
        t |> equal(cnt, 0)
    }

    t |> run("pairwise two elements") @(t : T?) {
        var src <- [iterator for(x in [10, 20]); x]
        var pairs <- pairwise(src)
        var result : array<tuple<int; int>>
        for (v in pairs) {
            result |> push(v)
        }
        t |> equal(length(result), 1)
        t |> equal(result[0]._0, 10)
        t |> equal(result[0]._1, 20)
    }
}

// ===========================
// echo tests
// ===========================

[test]
def test_echo(t : T?) {

    t |> run("echo returns value") @(t : T?) {
        var x = echo(42, "")
        t |> equal(x, 42)
    }

    t |> run("echo non-destructive") @(t : T?) {
        var original = 100
        var copy = echo(original, "")
        t |> equal(original, 100) // should NOT be zeroed
        t |> equal(copy, 100)
    }
}

// ===========================
// is_equal / is_not_equal / not tests
// ===========================

[test]
def test_predicates(t : T?) {

    t |> run("is_equal") @(t : T?) {
        t |> equal(is_equal(1, 1), true)
        t |> equal(is_equal(1, 2), false)
    }

    t |> run("is_not_equal") @(t : T?) {
        t |> equal(is_not_equal(1, 2), true)
        t |> equal(is_not_equal(1, 1), false)
    }

    t |> run("not") @(t : T?) {
        t |> equal(not(false), true)
        t |> equal(not(true), false)
    }
}

// ===========================
// sorted tests
// ===========================

[test]
def test_sorted(t : T?) {

    t |> run("sorted array") @(t : T?) {
        var arr = [3, 1, 4, 1, 5]
        var s <- sorted(arr)
        t |> equal(s[0], 1)
        t |> equal(s[1], 1)
        t |> equal(s[2], 3)
        t |> equal(s[3], 4)
        t |> equal(s[4], 5)
    }

    t |> run("sorted iterator") @(t : T?) {
        var src <- [iterator for(x in [5, 3, 1, 4, 2]); x]
        var s <- sorted(src)
        var result : array<int>
        for (v in s) {
            result |> push(v)
        }
        t |> equal(length(result), 5)
        t |> equal(result[0], 1)
        t |> equal(result[4], 5)
    }

    t |> run("sorted empty") @(t : T?) {
        var arr : array<int>
        var s <- sorted(arr)
        t |> equal(length(s), 0)
    }

    t |> run("sorted already sorted") @(t : T?) {
        var arr = [1, 2, 3]
        var s <- sorted(arr)
        t |> equal(s[0], 1)
        t |> equal(s[1], 2)
        t |> equal(s[2], 3)
    }
}

// ===========================
// composition tests (filter + map + reduce/fold chains)
// ===========================

[test]
def test_composition(t : T?) {

    t |> run("filter then map") @(t : T?) {
        var src <- [iterator for(x in range(6)); x]
        var evens <- filter(src, @(x : int) : bool { return x % 2 == 0; })
        var doubled <- map(evens, @(x : int) : int { return x * 2; })
        var result : array<int>
        for (v in doubled) {
            result |> push(v)
        }
        t |> equal(length(result), 3)
        t |> equal(result[0], 0)
        t |> equal(result[1], 4)
        t |> equal(result[2], 8)
    }

    t |> run("map then reduce") @(t : T?) {
        var src <- [iterator for(x in range(1, 4)); x]
        var squared <- map(src, @(x : int) : int { return x * x; })
        var result = reduce(squared) $(a, b : int) : int {
            return a + b
        }
        t |> equal(result, 14) // 1+4+9
    }

    t |> run("filter + map + fold") @(t : T?) {
        var src <- [iterator for(x in range(10)); x]
        var big <- filter(src, @(x : int) : bool { return x >= 5; })
        var doubled <- map(big, @(x : int) : int { return x * 2; })
        var total = fold(doubled, 0) $(acc, x : int) : int {
            return acc + x
        }
        // 5*2+6*2+7*2+8*2+9*2 = 70
        t |> equal(total, 70)
    }

    t |> run("enumerate + filter") @(t : T?) {
        var src <- [iterator for(x in [10, 20, 30, 40, 50]); x]
        var en <- enumerate(src)
        var evens <- filter(en, @(t : tuple<int; int>) : bool { return t._0 % 2 == 0; })
        var result : array<int>
        for (v in evens) {
            result |> push(v._1)
        }
        // indices 0, 2, 4 → values 10, 30, 50
        t |> equal(length(result), 3)
        t |> equal(result[0], 10)
        t |> equal(result[1], 30)
        t |> equal(result[2], 50)
    }

    t |> run("chain + filter") @(t : T?) {
        var a <- [iterator for(x in range(1, 4)); x]
        var b <- [iterator for(x in range(4, 7)); x]
        var combined <- chain(a, b)
        var evens <- filter(combined, @@is_even)
        var result : array<int>
        for (v in evens) {
            result |> push(v)
        }
        t |> equal(length(result), 3)
        t |> equal(result[0], 2)
        t |> equal(result[1], 4)
        t |> equal(result[2], 6)
    }
}
