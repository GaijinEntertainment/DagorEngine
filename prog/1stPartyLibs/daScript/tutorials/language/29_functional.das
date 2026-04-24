// Tutorial 29: Functional Programming
//
// This tutorial covers:
//   - filter — keep matching elements
//   - map — transform each element
//   - reduce / fold — combine elements into one value
//   - scan — running reduce
//   - chain, enumerate, pairwise — iterator combinators
//   - iterate, repeat, cycle — generators
//   - any, all, sum — aggregations
//   - find, find_index, partition — search & split
//   - for_each, tap, echo — side-effects & debugging
//   - Composing pipelines
//
// Run: daslang.exe tutorials/language/29_functional.das

options gen2

require daslib/functional

// === Helper functions ===

def is_even(x : int) : bool {
    return x % 2 == 0
}

def add(a, b : int) : int {
    return a + b
}

// === filter — keep matching elements ===
// filter takes an iterator and a lambda or function predicate.
// It yields only elements for which the predicate returns true.

[export]
def main {

    // --- filter ---
    print("filter:\n")
    var src <- [iterator for(x in range(8)); x]
    var evens <- filter(src, @(x : int) : bool { return x % 2 == 0; })
    for (v in evens) {
        print("  {v}")
    }
    print("\n")

    // filter with named function
    var src2 <- [iterator for(x in range(6)); x]
    var evens2 <- filter(src2, @@is_even)
    print("  with @@is_even:")
    for (v in evens2) {
        print(" {v}")
    }
    print("\n")

    // --- map ---
    print("map:\n")
    var nums <- [iterator for(x in range(5)); x]
    var squared <- map(nums, @(x : int) : int { return x * x; })
    for (v in squared) {
        print("  {v}")
    }
    print("\n")

    // map with type conversion
    var ints <- [iterator for(x in range(3)); x]
    var floats <- map(ints, @(x : int) : float { return float(x) * 1.5; })
    print("  as float:")
    for (v in floats) {
        print(" {v}")
    }
    print("\n")

    // --- reduce ---
    print("reduce:\n")
    var r1 <- [iterator for(x in range(1, 6)); x]
    var total = reduce(r1) $(a, b : int) : int {
        return a + b
    }
    print("  sum 1..5 = {total}\n")

    // reduce with function
    var r2 <- [iterator for(x in range(1, 6)); x]
    print("  sum (@@add) = {reduce(r2, @@add)}\n")

    // --- reduce_or_default ---
    print("reduce_or_default:\n")
    var empty_it <- [iterator for(x in range(0)); x]
    var safe_sum = reduce_or_default(empty_it, @(a, b : int) : int { return a + b; }, -1)
    print("  empty → {safe_sum}\n") // -1, no panic

    // --- fold ---
    print("fold:\n")
    var f1 <- [iterator for(x in range(1, 5)); x]
    var product = fold(f1, 1) $(acc, x : int) : int {
        return acc * x
    }
    print("  product 1..4 = {product}\n")

    // fold with different accumulator type
    var f2 <- [iterator for(x in range(1, 4)); x]
    var csv = fold(f2, "items:") $(acc : string; x : int) : string {
        return "{acc} {x}"
    }
    print("  string fold: {csv}\n")

    // --- scan ---
    print("scan (running sum):\n")
    var s1 <- [iterator for(x in range(1, 6)); x]
    var running <- scan(s1, 0, @(acc, x : int) : int { return acc + x; })
    for (v in running) {
        print("  {v}")
    }
    print("\n") // 0 1 3 6 10 15

    // --- sum ---
    print("sum:\n")
    var sm <- [iterator for(x in range(1, 11)); x]
    print("  1..10 = {sum(sm)}\n")

    // --- any / all ---
    print("any/all:\n")
    var a1 <- [iterator for(x in [false, false, true]); x]
    print("  any [F,F,T] = {any(a1)}\n")
    var a2 <- [iterator for(x in [true, true, true]); x]
    print("  all [T,T,T] = {all(a2)}\n")

    // --- enumerate ---
    print("enumerate:\n")
    var names <- [iterator for(x in ["alpha", "beta", "gamma"]); x]
    var en <- enumerate(names)
    for (v in en) {
        print("  {v._0}: {v._1}\n")
    }

    // --- chain ---
    print("chain:\n")
    var ca <- [iterator for(x in range(1, 4)); x]
    var cb <- [iterator for(x in range(4, 7)); x]
    var combined <- chain(ca, cb)
    for (v in combined) {
        print("  {v}")
    }
    print("\n") // 1 2 3 4 5 6

    // --- pairwise ---
    print("pairwise:\n")
    var pw <- [iterator for(x in range(1, 6)); x]
    var pairs <- pairwise(pw)
    for (v in pairs) {
        print("  ({v._0},{v._1})")
    }
    print("\n") // (1,2) (2,3) (3,4) (4,5)

    // --- iterate ---
    print("iterate (powers of 2):\n")
    var powers <- iterate(1, @(x : int) : int { return x * 2; })
    var cnt = 0
    for (v in powers) {
        print("  {v}")
        cnt ++
        if (cnt >= 6) {
            break
        }
    }
    print("\n") // 1 2 4 8 16 32

    // --- repeat ---
    print("repeat:\n")
    var rep <- repeat(42, 4)
    for (v in rep) {
        print("  {v}")
    }
    print("\n") // 42 42 42 42

    // --- cycle ---
    print("cycle:\n")
    var cy <- [iterator for(x in range(1, 4)); x]
    var cyc <- cycle(cy)
    cnt = 0
    for (v in cyc) {
        print("  {v}")
        cnt ++
        if (cnt >= 9) {
            break
        }
    }
    print("\n") // 1 2 3 1 2 3 1 2 3

    // --- islice ---
    print("islice:\n")
    var isl <- [iterator for(x in range(10)); x]
    var sliced <- islice(isl, 3, 7)
    for (v in sliced) {
        print("  {v}")
    }
    print("\n") // 3 4 5 6

    // --- sorted ---
    print("sorted:\n")
    var sa <- [iterator for(x in [5, 3, 1, 4, 2]); x]
    var ss <- sorted(sa)
    for (v in ss) {
        print("  {v}")
    }
    print("\n") // 1 2 3 4 5

    // --- find ---
    print("find:\n")
    var fd <- [iterator for(x in range(10)); x]
    var found = find(fd, @(x : int) : bool { return x > 5; }, -1)
    print("  first > 5: {found}\n")

    var fd2 <- [iterator for(x in range(3)); x]
    var notfound = find(fd2, @(x : int) : bool { return x > 100; }, -1)
    print("  first > 100: {notfound}\n")

    // --- find_index ---
    print("find_index:\n")
    var fi <- [iterator for(x in range(10, 20)); x]
    var idx = find_index(fi, @(x : int) : bool { return x == 15; })
    print("  15 in 10..19: index {idx}\n")

    // --- partition ---
    print("partition:\n")
    var pt <- [iterator for(x in range(8)); x]
    var parts = partition(pt, @@is_even)
    print("  evens: ")
    for (v in parts._0) {
        print("{v} ")
    }
    print("\n  odds:  ")
    for (v in parts._1) {
        print("{v} ")
    }
    print("\n")

    // --- for_each ---
    print("for_each:\n")
    var total2 = 0
    var fe <- [iterator for(x in range(1, 5)); x]
    for_each(fe) $(x : int) {
        total2 += x
    }
    print("  sum via for_each: {total2}\n")

    // --- tap ---
    print("tap:\n")
    var tp <- [iterator for(x in range(1, 4)); x]
    var tapped <- tap(tp, @(x : int) { print("  tap:{x}"); })
    var tsum = 0
    for (v in tapped) {
        tsum += v
    }
    print(" sum={tsum}\n")

    // --- echo ---
    print("echo:\n")
    var val = echo(42, "")
    print("  → returned {val}\n")

    // --- flatten ---
    print("flatten:\n")
    var nested <- [iterator for(x in range(3)); [iterator for(y in range(x, x + 2)); y]]
    var flat <- flatten(nested)
    for (v in flat) {
        print("  {v}")
    }
    print("\n") // 0 1 1 2 2 3

    // --- composition ---
    print("composition (filter→map→fold):\n")
    var pipe <- [iterator for(x in range(10)); x]
    var big <- filter(pipe, @(x : int) : bool { return x >= 5; })
    var doubled <- map(big, @(x : int) : int { return x * 2; })
    var result = fold(doubled, 0) $(acc, x : int) : int {
        return acc + x
    }
    print("  sum of doubled >=5: {result}\n") // (5+6+7+8+9)*2 = 70

    // --- is_equal / is_not_equal / not ---
    print("predicates:\n")
    print("  is_equal(3,3) = {is_equal(3, 3)}\n")
    print("  is_not_equal(1,2) = {is_not_equal(1, 2)}\n")
    print("  not(false) = {not(false)}\n")

    // === Summary ===
    // - require daslib/functional
    // - filter(src, lambda/function) — keep matching elements
    // - map(src, lambda/function) — transform each element
    // - reduce(src, lambda/function/block) — combine into one value
    // - reduce_or_default(src, lambda/function, default) — safe reduce
    // - fold(src, seed, lambda/function/block) — reduce with initial value
    // - scan(src, seed, lambda/function) — running fold (yields intermediates)
    // - sum(src) — sum all elements
    // - any(src) / all(src) — boolean aggregation
    // - enumerate(src) — (index, element) pairs
    // - chain(a, b) — concatenate two iterators
    // - pairwise(src) — consecutive pairs
    // - iterate(seed, fn) — infinite sequence: seed, f(seed), ...
    // - repeat(val, n) — repeat value n times
    // - cycle(src) — endlessly repeat an iterator
    // - islice(src, start, stop) — slice [start, stop)
    // - sorted(src) — sort iterator/array
    // - flatten(src) — flatten nested iterators
    // - find(src, fn, default) — first match or default
    // - find_index(src, fn) — index of first match or -1
    // - partition(src, fn) — split (matching, non-matching)
    // - for_each(src, fn/block) — side-effect on every element
    // - tap(src, fn) — passthrough with side-effect
    // - echo(x) — print and return value
    // - is_equal, is_not_equal, not — simple predicates
    //
    // Note: filter, map, tap, scan return lazy iterators and only
    // accept lambdas or functions (not blocks), because blocks
    // cannot be captured into generators.

    print("done\n")
}

// output:
// filter:
//   0  2  4  6
//   with @@is_even: 0 2 4
// map:
//   0  1  4  9  16
//   as float: 0 1.5 3
// reduce:
//   sum 1..5 = 15
//   sum (@@add) = 15
// reduce_or_default:
//   empty → -1
// fold:
//   product 1..4 = 24
//   string fold: items: 1 2 3
// scan (running sum):
//   0  1  3  6  10  15
// sum:
//   1..10 = 55
// any/all:
//   any [F,F,T] = true
//   all [T,T,T] = true
// enumerate:
//   0: alpha
//   1: beta
//   2: gamma
// chain:
//   1  2  3  4  5  6
// pairwise:
//   (1,2)  (2,3)  (3,4)  (4,5)
// iterate (powers of 2):
//   1  2  4  8  16  32
// repeat:
//   42  42  42  42
// cycle:
//   1  2  3  1  2  3  1  2  3
// islice:
//   3  4  5  6
// sorted:
//   1  2  3  4  5
// find:
//   first > 5: 6
//   first > 100: -1
// find_index:
//   15 in 10..19: index 5
// partition:
//   evens: 0 2 4 6
//   odds:  1 3 5 7
// for_each:
//   sum via for_each: 10
// tap:
//   tap:1  tap:2  tap:3 sum=6
// echo:
// 42  → returned 42
// flatten:
//   0  1  1  2  2  3
// composition (filter→map→fold):
//   sum of doubled >=5: 70
// predicates:
//   is_equal(3,3) = true
//   is_not_equal(1,2) = true
//   not(false) = true
// done
