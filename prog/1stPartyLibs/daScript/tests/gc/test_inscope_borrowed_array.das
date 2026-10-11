options gen2
options persistent_heap
options force_inscope_pod

require dastest/testing_boost public

[never_inline]
def read_command_line() : int {
    let args <- get_command_line_arguments()
    assert(!empty(args))
    assert(!empty(args[0]))
    return length(args)
}

[never_inline]
def read_owned_array() : int {
    let values <- [1, 2, 3]
    return length(values)
}

[never_inline]
def make_borrowed_nested(var owner : array<array<int>>) : array<array<int>> {
    var view : array<array<int>>
    unsafe {
        _builtin_make_temp_array(view, addr(owner[0]), length(owner))
    }
    return <- view
}

[never_inline]
def read_borrowed_nested(var owner : array<array<int>>) : int {
    let view <- make_borrowed_nested(owner)
    return length(view)
}

def check_nested_owner(t : T?; owner : array<array<int>>) {
    t |> equal(length(owner), 1)
    t |> equal(length(owner[0]), 3)
    if (length(owner[0]) == 3) {
        t |> equal(owner[0][0], 11)
        t |> equal(owner[0][2], 33)
    }
}

[never_inline]
def reject_borrowed_nested(t : T?; var owner : array<array<int>>; extra_lock : bool) {
    var view : array<array<int>>
    unsafe {
        _builtin_make_temp_array(view, addr(owner[0]), length(owner))
        if (extra_lock) {
            __builtin_array_lock(view)
        } else {
            __builtin_array_unlock(view)
        }
    }
    var refused = false
    let before = heap_bytes_allocated()
    try {
        unsafe(builtin_collect_local(view, uint(typeinfo sizeof(view))))
    } recover {
        refused = true
    }
    t |> success(refused)
    t |> equal(length(view), 1)
    t |> equal(heap_bytes_allocated(), before)
    check_nested_owner(t, owner)
    unsafe {
        _builtin_make_temp_array(view, addr(owner[0]), length(owner))
        _builtin_forget_temp_array(view)
    }
}

[test]
def test_inscope_borrowed_array(t : T?) {
    t |> run("scope exit preserves borrowed nested arrays") @(t : T?) {
        var owner : array<array<int>>
        resize(owner, 1)
        owner[0] <- [11, 22, 33]
        let before = heap_bytes_allocated()
        for (_ in range(8)) {
            t |> equal(read_borrowed_nested(owner), 1)
            check_nested_owner(t, owner)
        }
        t |> equal(heap_bytes_allocated(), before)
    }
    t |> run("multiply locked borrowed arrays fail before visiting elements") @(t : T?) {
        var owner : array<array<int>>
        resize(owner, 1)
        owner[0] <- [11, 22, 33]
        reject_borrowed_nested(t, owner, true)
    }
    t |> run("unlocked borrowed arrays fail before visiting elements") @(t : T?) {
        var owner : array<array<int>>
        resize(owner, 1)
        owner[0] <- [11, 22, 33]
        reject_borrowed_nested(t, owner, false)
    }
    t |> run("scope exit forgets argv views and preserves their storage") @(t : T?) {
        let count = read_command_line()
        let before = heap_bytes_allocated()
        for (_ in range(8)) {
            t |> equal(read_command_line(), count)
        }
        t |> equal(heap_bytes_allocated(), before)
    }
    t |> run("scope exit still frees owned arrays") @(t : T?) {
        let before = heap_bytes_allocated()
        for (_ in range(8)) {
            t |> equal(read_owned_array(), 3)
        }
        t |> equal(heap_bytes_allocated(), before)
    }
}

[test]
def test_inscope_locked_containers(t : T?) {
    t |> run("collection still rejects locked owned arrays") @(t : T?) {
        var values <- [1, 2, 3]
        var refused = false
        lock(values) $(locked) {
            try {
                unsafe(builtin_collect_local(values, uint(typeinfo sizeof(values))))
            } recover {
                refused = true
            }
            t |> equal(length(locked), 3)
            t |> equal(locked[0], 1)
        }
        t |> success(refused)
    }
    t |> run("locked owned nested arrays fail before visiting elements") @(t : T?) {
        var owner : array<array<int>>
        resize(owner, 1)
        owner[0] <- [11, 22, 33]
        var refused = false
        lock(owner) $(locked) {
            let before = heap_bytes_allocated()
            try {
                unsafe(builtin_collect_local(owner, uint(typeinfo sizeof(owner))))
            } recover {
                refused = true
            }
            t |> equal(length(locked), 1)
            t |> equal(length(locked[0]), 3)
            t |> equal(locked[0][0], 11)
            t |> equal(locked[0][2], 33)
            t |> equal(heap_bytes_allocated(), before)
        }
        t |> success(refused)
    }
    t |> run("collection rejects locked owned tables without losing their storage") @(t : T?) {
        var values : table<int; int>
        values[1] = 11
        var refused = false
        lock(values) $(locked) {
            try {
                unsafe(builtin_collect_local(values, uint(typeinfo sizeof(values))))
            } recover {
                refused = true
            }
            t |> equal(length(locked), 1)
            var found = false
            get(locked, 1) $(value) {
                found = true
                t |> equal(value, 11)
            }
            t |> success(found)
        }
        t |> success(refused)
    }
    t |> run("locked tables with nested arrays fail before visiting values") @(t : T?) {
        var owner : table<int; array<int>>
        owner[1] <- [11, 22, 33]
        var refused = false
        lock(owner) $(locked) {
            let before = heap_bytes_allocated()
            try {
                unsafe(builtin_collect_local(owner, uint(typeinfo sizeof(owner))))
            } recover {
                refused = true
            }
            t |> equal(length(locked), 1)
            get(locked, 1) $(value) {
                t |> equal(length(value), 3)
                t |> equal(value[0], 11)
                t |> equal(value[2], 33)
            }
            t |> equal(heap_bytes_allocated(), before)
        }
        t |> success(refused)
    }
}
