options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4
options strict_smart_pointers = true

module functional shared public

//! Functional programming primitives.
//!
//! Lazy iterator adapters: ``filter``, ``map``, ``reduce``, ``scan``,
//! ``flatten``, ``flat_map``, ``repeat``, ``cycle``, ``iterate``,
//! ``chain``, ``enumerate``, ``pairwise``, ``islice``, ``sorted``,
//! ``sum``, ``any``, ``all``, ``tap``, ``for_each``, ``find``,
//! ``find_index``, ``partition``, and more.
//!
//! Unlike ``daslib/linq``, which uses blocks and focuses on query
//! expressions, this module accepts lambdas, functions, and blocks
//! and emphasises composability over raw performance.

def private filter_any(var src : iterator<auto(TT)>; var blk) {
    //! iterates over `src` and yields only those elements for which `blk` returns true
    static_if (!typeinfo is_ref_type(type<TT>)) {
        return <- generator<TT -&> capture(<- src, <- blk) () <| $() {
            for (w in src) {
                if (invoke(blk, w)) {
                    yield w
                }
            }
            return false
        }
    } else {
        return <- generator<TT&> capture(<- src, <- blk) () <| $() {
            for (w in src) {
                if (invoke(blk, w)) {
                    yield w
                }
            }
            return false
        }
    }
}

def filter(var src : iterator<auto(TT)>; blk : lambda<(what : TT -&) : bool>) {
    //! iterates over `src` and yields only those elements for which `blk` returns true
    return <- filter_any(src, blk)
}

// filter with function
def filter(var src : iterator<auto(TT)>; blk : function<(what : TT -&) : bool>) {
    //! iterates over `src` and yields only those elements for which `blk` returns true
    return <- filter_any(src, blk)
}

def private map_any(var src : iterator<auto(TT)>; var blk) {
    //! iterates over `src` and yields the result of `blk` for each element
    static_if (typeinfo can_copy(type<QQ>)) {
        return <- generator<QQ -&> capture(<- src, <- blk) () <| $() {
            for (w in src) {
                static_if (typeinfo is_ref_type(invoke(blk, w))) {
                    yield copy_to_local(invoke(blk, w))
                } else {
                    yield invoke(blk, w)
                }
            }
            return false
        }
    } static_elif (typeinfo can_move(type<QQ>)) {
        return <- generator<QQ -&> capture(<- src, <- blk) () <| $() {
            for (w in src) {
                static_if (typeinfo is_ref_type(invoke(blk, w))) {
                    yield <- move_to_local(invoke(blk, w))
                } else {
                    yield <- invoke(blk, w)
                }
            }
            return false
        }
    } else {
        concept_assert(false, "can't copy or move return type")
        var dummy : iterator<QQ>
        return <- dummy
    }
}


// map with lambda
def map(var src : iterator<auto(TT)>; blk : lambda<(what : TT -&) : auto(QQ)>) {
    //! iterates over `src` and yields the result of `blk` for each element
    return <- map_any(src, blk)
}

// map with function
def map(var src : iterator<auto(TT)>; blk : function<(what : TT -&) : auto(QQ)>) {
    //! iterates over `src` and yields the result of `blk` for each element
    return <- map_any(src, blk)
}

// reduce value, any invokable
def private reduce_any(var it : iterator<auto(TT)>; functor) {
    //! iterates over `it` and yields the reduced (combined) result of `functor` for each element
    var left : TT -&
    var first = true
    for (right in it) {
        if (first) {
            first = false
            left = right
        } else {
            static_if (typeinfo is_ref_type(type<TT -&>)) {
                left <- copy_to_local(invoke(functor, left, right))
            } else {
                left = invoke(functor, left, right)
            }
        }
    }
    if (first) {
        panic("can't reduce empty sequence")
    }
    return left
}

// reduce with lambda
def reduce(var it : iterator<auto(TT)>; blk : lambda<(left, right : TT -&) : TT const -&>) {
    //! iterates over `it` and yields the reduced (combined) result of `blk` for each element
    //! and previous reduction result
    return reduce_any(it, blk)
}

// reduce with function
def reduce(var it : iterator<auto(TT)>; blk : function<(left, right : TT -&) : TT const -&>) {
    //! iterates over `it` and yields the reduced (combined) result of `blk` for each element
    //! and previous reduction result
    return reduce_any(it, blk)
}

// reduce with block
def reduce(var it : iterator<auto(TT)>; blk : block<(left, right : TT -&) : TT const -&>) {
    //! iterates over `it` and yields the reduced (combined) result of `blk` for each element
    //! and previous reduction result
    return reduce_any(it, blk)
}

// summ of all elements
def sum(var it : iterator<auto(TT)>) {
    //! iterates over `it` and yields the sum of all elements
    //! same as reduce(it, @(a,b) => a + b)
    var temp : TT -&
    for (x in it) {
        temp += x
    }
    return temp
}

// any
def any(it) {
    //! iterates over `it` and yields true if any element is true
    static_if (!typeinfo is_iterable(it)) {
        concept_assert(false, "can't ANY non-iterable type")
        return false
    } else {
        for (x in it) {
            if (x) {
                return true
            }
        }
        return false
    }
}

def any(var it : iterator<auto(TT)>) {
    //! iterates over `it` and yields true if any element is true
    for (x in it) {
        if (x) {
            return true
        }
    }
    return false
}

// all
def all(it) {
    //! iterates over `it` and yields true if all elements are true
    static_if (!typeinfo is_iterable(it)) {
        concept_assert(false, "can't ALL non-iterable type")
        return false
    } else {
        for (x in it) {
            if (!x) {
                return false
            }
        }
        return true
    }
}

def all(var it : iterator<auto(TT)>) {
    //! iterates over `it` and yields true if all elements are true
    for (x in it) {
        if (!x) {
            return false
        }
    }
    return true
}

def cycle(var src : iterator<auto(TT)>) {
    //! endlessly iterates over `src`
    static_if (!typeinfo is_ref_type(type<TT>)) {
        return <- generator<TT -&> capture(<- src) () <| $() {
            var arr : array<TT -const -&>
            for (x in src) {
                yield x
                push(arr, x)
            }
            while (true) {
                for (x in arr) {
                    yield x
                }
            }
            return false
        }
    } else {
        return <- generator<TT&> capture(<- src) () <| $() {
            var arr : array<TT -const -&>
            for (x in src) {
                yield x
                push_clone(arr, x)
            }
            while (true) {
                for (x in arr) {
                    yield x
                }
            }
            return false
        }
    }
}

def islice(var src : iterator<auto(TT)>; start, stop : int) {
    //! iterates over `src` and yields only the elements in the range [start,stop)
    static_if (!typeinfo is_ref_type(type<TT>)) {
        return <- generator<TT -&> capture(<- src) () <| $() {
            for (x, i in src, range(stop)) {
                if (i >= start) {
                    yield x
                }
            }
            return false
        }
    } else {
        return <- generator<TT&> capture(<- src) () <| $() {
            for (x, i in src, range(stop)) {
                if (i >= start) {
                    yield x
                }
            }
            return false
        }
    }
}

// [[ value; value; value; .... count times ]]
def repeat_ref(value : auto(TT); var total : int) {
    //! yields `value` by reference `count` times
    return <- generator<TT const&>() <| $() {
        while (total > 0) {
            yield value
            total --
        }
        return false
    }
}

// [[ value; value; value; .... count times ]]
def repeat(value : auto(TT); var count : int = -1) {
    //! yields `value` `count` times. If `count` is negative, repeats forever.
    return <- generator<TT const -&>() <| $() {
        if (count < 0) {
            while (true) {
                yield value
            }
        }
        for (x in range(count)) {
            yield value
        }
        return false
    }
}

def is_equal(a, b) {
    //! yields true if `a` and `b` are equal
    return a == b
}

def is_not_equal(a, b) {
    //! yields true if `a` and `b` are not equal
    return a != b
}

def not(x) {
    //! yields !x
    return !x
}

def echo(x; extra : string = "\n") {
    //! prints `x` to the output with `extra` appended, then returns `x` unchanged.
    //! Non-destructive — safe to use in expression chains.
    print("{x}{extra}")
    return x
}

def private flatten_one(var src : iterator<iterator<auto(TT)>>) {
    //! iterates over `src`, then iterates over each element of each element of `src` and yields it
    static_if (!typeinfo is_ref_type(type<TT>)) {
        return <- generator<TT -&> capture(<- src) () <| $() {
            for (i in src) {
                for (j in i) {
                    yield j
                }
            }
            return false
        }
    } else {
        return <- generator<TT&> capture(<- src) () <| $() {
            for (i in src) {
                for (j in i) {
                    yield j
                }
            }
            return false
        }
    }
}

def flatten(var it : iterator<auto(TT)>) {
    //! iterates over `it`, then iterates over each element of each element of `it` and yields it
    unsafe {
        static_if (typeinfo is_iterator(type<TT>)) {
            return <- flatten_one([iterator for(i in it); flatten(i)])
        } else {
            return <- [iterator for(i in it); i]
        }
    }
}

def sorted(var arr : array<auto>) {
    //! iterates over input and returns it sorted version
    arr |> sort
    return <- arr
}

def sorted(var it : iterator<auto(TT)>) {
    //! iterates over input and returns it sorted version
    var arr <- to_array(it)
    sort(arr)
    return <- generator<TT> capture(<- arr) () <| $() {
        for (x in arr) {
            yield x
        }
        return false
    }
}

// ===========================
//  reduce_or_default — safe reduce that returns a default on empty input
// ===========================

def private reduce_or_default_any(var it : iterator<auto(TT)>; functor; default_value : TT -&) {
    //! like reduce, but returns `default_value` instead of panicking on empty input
    var left : TT -&
    var first = true
    for (right in it) {
        if (first) {
            first = false
            left = right
        } else {
            static_if (typeinfo is_ref_type(type<TT -&>)) {
                left <- copy_to_local(invoke(functor, left, right))
            } else {
                left = invoke(functor, left, right)
            }
        }
    }
    return left if (!first)
    return default_value
}

def reduce_or_default(var it : iterator<auto(TT)>; blk : lambda<(left, right : TT -&) : TT const -&>; default_value : TT -&) {
    //! like reduce, but returns `default_value` on empty input
    return reduce_or_default_any(it, blk, default_value)
}

def reduce_or_default(var it : iterator<auto(TT)>; blk : function<(left, right : TT -&) : TT const -&>; default_value : TT -&) {
    //! like reduce, but returns `default_value` on empty input
    return reduce_or_default_any(it, blk, default_value)
}

def reduce_or_default(var it : iterator<auto(TT)>; default_value : TT -&; blk : block<(left, right : TT -&) : TT const -&>) {
    //! like reduce, but returns `default_value` on empty input (block version, `default_value` before block for `<|` syntax)
    return reduce_or_default_any(it, blk, default_value)
}

// ===========================
//  fold — reduce with explicit initial value (seed)
// ===========================

def private fold_any(var it : iterator<auto(TT)>; seed : auto(AGG); functor) {
    //! combines elements left-to-right starting from `seed`
    var acc : AGG -& = seed
    for (x in it) {
        static_if (typeinfo is_ref_type(type<AGG -&>)) {
            acc <- copy_to_local(invoke(functor, acc, x))
        } else {
            acc = invoke(functor, acc, x)
        }
    }
    return acc
}

def fold(var it : iterator<auto(TT)>; seed : auto(AGG); blk : lambda<(acc : AGG -&, x : TT -&) : AGG const -&>) {
    //! combines elements left-to-right starting from `seed`
    return fold_any(it, seed, blk)
}

def fold(var it : iterator<auto(TT)>; seed : auto(AGG); blk : function<(acc : AGG -&, x : TT -&) : AGG const -&>) {
    //! combines elements left-to-right starting from `seed`
    return fold_any(it, seed, blk)
}

def fold(var it : iterator<auto(TT)>; seed : auto(AGG); blk : block<(acc : AGG -&, x : TT -&) : AGG const -&>) {
    //! combines elements left-to-right starting from `seed`
    return fold_any(it, seed, blk)
}

// ===========================
//  scan — running reduce, yields every intermediate value
// ===========================

def private scan_any(var src : iterator<auto(TT)>; seed : auto(AGG); var functor) {
    //! like fold, but yields every intermediate accumulator value
    return <- generator<AGG -&> capture(<- src, <- functor) () <| $() {
        var acc : AGG -& = seed
        yield acc
        for (x in src) {
            static_if (typeinfo is_ref_type(type<AGG -&>)) {
                acc <- copy_to_local(invoke(functor, acc, x))
            } else {
                acc = invoke(functor, acc, x)
            }
            yield acc
        }
        return false
    }
}

def scan(var src : iterator<auto(TT)>; seed : auto(AGG); blk : lambda<(acc : AGG -&, x : TT -&) : AGG const -&>) {
    //! yields every intermediate accumulator value, starting from `seed`
    return <- scan_any(src, seed, blk)
}

def scan(var src : iterator<auto(TT)>; seed : auto(AGG); blk : function<(acc : AGG -&, x : TT -&) : AGG const -&>) {
    //! yields every intermediate accumulator value, starting from `seed`
    return <- scan_any(src, seed, blk)
}

// ===========================
//  enumerate — pair each element with its index
// ===========================

def enumerate(var src : iterator<auto(TT)>) {
    //! yields tuples of `(index, element)` for each element in `src`
    return <- generator<tuple<int, TT -&>> capture(<- src) () <| $() {
        var idx = 0
        for (x in src) {
            yield (idx, x)
            idx ++
        }
        return false
    }
}

// ===========================
//  for_each — apply side-effect to every element
// ===========================

def private for_each_any(var src : iterator<auto(TT)>; blk) {
    //! invokes `blk` on every element of `src`
    for (x in src) {
        invoke(blk, x)
    }
}

def for_each(var src : iterator<auto(TT)>; blk : lambda<(what : TT -&) : void>) {
    //! invokes `blk` on every element of `src`
    for_each_any(src, blk)
}

def for_each(var src : iterator<auto(TT)>; blk : function<(what : TT -&) : void>) {
    //! invokes `blk` on every element of `src`
    for_each_any(src, blk)
}

def for_each(var src : iterator<auto(TT)>; blk : block<(what : TT -&) : void>) {
    //! invokes `blk` on every element of `src`
    for_each_any(src, blk)
}

// ===========================
//  find — first matching element
// ===========================

def private find_any(var src : iterator<auto(TT)>; predicate; default_value : TT -&) {
    //! returns the first element for which `predicate` returns true, or `default_value`
    for (x in src) {
        if (invoke(predicate, x)) {
            return x
        }
    }
    return default_value
}

def find(var src : iterator<auto(TT)>; blk : lambda<(what : TT -&) : bool>; default_value : TT -&) {
    //! returns the first element for which `blk` returns true, or `default_value`
    return find_any(src, blk, default_value)
}

def find(var src : iterator<auto(TT)>; blk : function<(what : TT -&) : bool>; default_value : TT -&) {
    //! returns the first element for which `blk` returns true, or `default_value`
    return find_any(src, blk, default_value)
}

def find(var src : iterator<auto(TT)>; default_value : TT -&; blk : block<(what : TT -&) : bool>) {
    //! returns the first element for which `blk` returns true, or `default_value` (block version, `default_value` before block for `<|` syntax)
    return find_any(src, blk, default_value)
}

// ===========================
//  find_index — index of first matching element
// ===========================

def private find_index_any(var src : iterator<auto(TT)>; predicate) : int {
    //! returns the index of the first element for which `predicate` returns true, or -1
    var idx = 0
    for (x in src) {
        if (invoke(predicate, x)) {
            return idx
        }
        idx ++
    }
    return -1
}

def find_index(var src : iterator<auto(TT)>; blk : lambda<(what : TT -&) : bool>) : int {
    //! returns the index of the first element for which `blk` returns true, or -1
    return find_index_any(src, blk)
}

def find_index(var src : iterator<auto(TT)>; blk : function<(what : TT -&) : bool>) : int {
    //! returns the index of the first element for which `blk` returns true, or -1
    return find_index_any(src, blk)
}

def find_index(var src : iterator<auto(TT)>; blk : block<(what : TT -&) : bool>) : int {
    //! returns the index of the first element for which `blk` returns true, or -1
    return find_index_any(src, blk)
}

// ===========================
//  partition — split by predicate
// ===========================

def private partition_any(var src : iterator<auto(TT)>; predicate) : tuple<array<TT -const -&>, array<TT -const -&>> {
    //! splits elements into `(matching, non_matching)` arrays
    var yes : array<TT -const -&>
    var no  : array<TT -const -&>
    for (x in src) {
        if (invoke(predicate, x)) {
            yes |> push(x)
        } else {
            no |> push(x)
        }
    }
    return (yes, no)
}

def partition(var src : iterator<auto(TT)>; blk : lambda<(what : TT -&) : bool>) : tuple<array<TT -const -&>, array<TT -const -&>> {
    //! splits elements into `(matching, non_matching)` arrays
    return partition_any(src, blk)
}

def partition(var src : iterator<auto(TT)>; blk : function<(what : TT -&) : bool>) : tuple<array<TT -const -&>, array<TT -const -&>> {
    //! splits elements into `(matching, non_matching)` arrays
    return partition_any(src, blk)
}

def partition(var src : iterator<auto(TT)>; blk : block<(what : TT -&) : bool>) : tuple<array<TT -const -&>, array<TT -const -&>> {
    //! splits elements into `(matching, non_matching)` arrays
    return partition_any(src, blk)
}

// ===========================
//  tap — side-effect passthrough for debugging
// ===========================

def private tap_any(var src : iterator<auto(TT)>; var blk) {
    //! yields every element unchanged, calling `blk` on each as a side-effect
    static_if (!typeinfo is_ref_type(type<TT>)) {
        return <- generator<TT -&> capture(<- src, <- blk) () <| $() {
            for (x in src) {
                invoke(blk, x)
                yield x
            }
            return false
        }
    } else {
        return <- generator<TT&> capture(<- src, <- blk) () <| $() {
            for (x in src) {
                invoke(blk, x)
                yield x
            }
            return false
        }
    }
}

def tap(var src : iterator<auto(TT)>; blk : lambda<(what : TT -&) : void>) {
    //! yields every element unchanged, calling `blk` on each as a side-effect
    return <- tap_any(src, blk)
}

def tap(var src : iterator<auto(TT)>; blk : function<(what : TT -&) : void>) {
    //! yields every element unchanged, calling `blk` on each as a side-effect
    return <- tap_any(src, blk)
}

// ===========================
//  iterate — infinite sequence from seed + function
// ===========================

def iterate(seed : auto(TT); var blk : lambda<(what : TT -&) : TT const -&>) {
    //! yields `seed`, `f(seed)`, `f(f(seed))`, ... infinitely.
    //! Use with `islice` or `take` to limit output.
    return <- generator<TT -&> capture(<- blk) () <| $() {
        var current : TT -& = seed
        while (true) {
            yield current
            static_if (typeinfo is_ref_type(type<TT -&>)) {
                current <- copy_to_local(invoke(blk, current))
            } else {
                current = invoke(blk, current)
            }
        }
        return false
    }
}

def iterate(seed : auto(TT); var blk : function<(what : TT -&) : TT const -&>) {
    //! yields `seed`, `f(seed)`, `f(f(seed))`, ... infinitely.
    return <- generator<TT -&> capture(<- blk) () <| $() {
        var current : TT -& = seed
        while (true) {
            yield current
            static_if (typeinfo is_ref_type(type<TT -&>)) {
                current <- copy_to_local(invoke(blk, current))
            } else {
                current = invoke(blk, current)
            }
        }
        return false
    }
}

// ===========================
//  chain — concatenate two iterators
// ===========================

def chain(var a : iterator<auto(TT)>; var b : iterator<auto(TT)>) {
    //! yields all elements of `a`, then all elements of `b`
    static_if (!typeinfo is_ref_type(type<TT>)) {
        return <- generator<TT -&> capture(<- a, <- b) () <| $() {
            for (x in a) {
                yield x
            }
            for (x in b) {
                yield x
            }
            return false
        }
    } else {
        return <- generator<TT&> capture(<- a, <- b) () <| $() {
            for (x in a) {
                yield x
            }
            for (x in b) {
                yield x
            }
            return false
        }
    }
}

// ===========================
//  pairwise — consecutive pairs
// ===========================

def pairwise(var src : iterator<auto(TT)>) {
    //! yields consecutive pairs: `(a,b)`, `(b,c)`, `(c,d)`, ...
    return <- generator<tuple<TT -&, TT -&>> capture(<- src) () <| $() {
        var prev : TT -&
        var first = true
        for (x in src) {
            if (first) {
                first = false
                prev = x
            } else {
                yield (prev, x)
                prev = x
            }
        }
        return false
    }
}

// ===========================
//  flat_map — map then flatten one level
// ===========================

def private flat_map_any(var src : iterator<auto(TT)>; var blk) {
    //! maps each element to an iterator, then flattens the results one level
    return <- generator<auto(QQ) -&> capture(<- src, <- blk) () <| $() {
        for (x in src) {
            for (y in invoke(blk, x)) {
                yield y
            }
        }
        return false
    }
}

def flat_map(var src : iterator<auto(TT)>; blk : lambda<(what : TT -&) : auto(QQ)>) {
    //! maps each element to an iterator, then flattens the results one level
    return <- flat_map_any(src, blk)
}

def flat_map(var src : iterator<auto(TT)>; blk : function<(what : TT -&) : auto(QQ)>) {
    //! maps each element to an iterator, then flattens the results one level
    return <- flat_map_any(src, blk)
}
