options gen2
options persistent_heap = true
options gc

require dastest/testing_boost public

options very_safe_context
options unsafe_table_lookup = false

struct Foo {
    a : array<int>
}

struct Bar {
    foo : Foo
}

[test]
def test_resize_array(t : T?) {
    t |> run("basic locked array") @(t : T?) {
        var a : array<int>
        resize(a, 10)
        var failed = false
        try {
            for (x in a) {
                resize(a, 10)           // exception: can't resize locked array
            }
        } recover {
            failed = true
        }
        t |> success(failed)
    }
    t |> run("nested locked array") @(t : T?) {
        var a : array< array<int> >
        resize(a, 10)
        resize(a[0], 10)
        var failed = false
        try {
            for (x in a[0]) {
                erase(a, 3)             // exception: object contains locked elements and can't be resized
            }
        } recover {
            failed = true
        }
        t |> success(failed)
    }
    t |> run("nested complicated array") @(t : T?) {
        var a : array< Bar >
        resize(a, 10)
        resize(a[0].foo.a, 10)
        var failed = false
        try {
            for (x in a[0].foo.a) {
                a |> emplace(Bar()) // this is fine, memory after 'resize' stays allocated in the very safe context
            }
        } recover {
            failed = true
        }
        t |> success(!failed)
        failed = false
        try {
            delete a        // this would be deleting locked array, since resize moved old one out of the way
                            // so a[0].foo.a stayed locked
        } recover {
            failed = true
        }
        t |> success(failed)
    }
}
struct Goo {
    a : table<int; int>
}

struct Bag {
    foo : Goo
}

[test]
def test_resize_table(t : T?) {
    t |> run("basic locked table") @(t : T?) {
        var a : table<int; int>
        for (tt in range(10)) {
            a |> insert(tt, tt)
        }
        var failed = false
        try {
            for (x in values(a)) {
                a |> insert(13, 133)            // can't insert into locked table
            }
        } recover {
            failed = true
        }
        t |> success(failed)
    }
    t |> run("nested locked table") @(t : T?) {
        var a : table<int; array<int>>
        for (tt in range(10)) {
            a[tt] <- [for (x in range(tt + 1)); x]
        }
        var failed = false
        try {
            for (x in a[3]) {
                a[13] <- [for (q in range(3)); x]   // this is fine in the very safe context, afaik
            }
        } recover {
            failed = true
        }
        t |> success(!failed)
        failed = false
        try {
            delete a
        } recover {
            failed = true
        }
        t |> success(!failed)
    }
    t |> run("nested complicated table") @(t : T?) {
        var a : table< int; Bag >
        a[10] <- Bag(
            foo <- Goo(
                a <- { 1 => 1, 2 => 2 }
            )
        )
        var failed = false
        try {
            for (x in values(a[10].foo.a)) {    // this fails on exit of the loop, since element gets erased
                                                // 'table was modified during iteration'
                a |> erase(10)
            }
        } recover {
            failed = true
        }
        t |> success(failed)
    }
}

