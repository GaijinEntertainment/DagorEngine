options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/delegate

// ── delegate typedefs ──────────────────────────────────────────────────

typedef OnEvent   = delegate(type<function<(arg : string) : bool>>)
typedef OnNotify  = delegate(type<lambda<(msg : string) : void>>)
typedef OnCompute = delegate(type<function<(a : int; b : float) : int>>)
typedef OnTick    = delegate(type<function<() : void>>)
typedef OnCollect = delegate(type<function<(n : int) : array<int>>>)

// ── helper functions ───────────────────────────────────────────────────

def event_handler(arg : string) : bool {
    feint("event_handler: {arg}\n")
    return true
}

def event_handler_false(arg : string) : bool {
    feint("event_handler_false: {arg}\n")
    return false
}

def compute_handler(a : int; b : float) : int {
    return a + int(b)
}

def tick_handler() {
    feint("tick\n")
}

// ── tests ──────────────────────────────────────────────────────────────

[test]
def test_empty_delegate(t : T?) {
    t |> run("empty non-void delegate") @(tt : T?) {
        var del <- OnEvent()
        tt |> success(del.empty())
        tt |> equal(del.length(), 0)
        tt |> equal(del.invoke("test"), false)
    }
    t |> run("empty void delegate") @(tt : T?) {
        var del <- OnNotify()
        tt |> success(del.empty())
        tt |> equal(del.length(), 0)
        del.invoke("test")
        tt |> success(true)
    }
    t |> run("empty zero-arg delegate") @(tt : T?) {
        var del <- OnTick()
        tt |> success(del.empty())
        del.invoke()
        tt |> success(true)
    }
}

[test]
def test_lambda_constructor(t : T?) {
    t |> run("non-void lambda ctor") @(tt : T?) {
        var del <- OnEvent(@(arg : string) : bool {
            feint("lambda: {arg}\n")
            return true
        })
        tt |> success(!del.empty())
        tt |> equal(del.length(), 1)
        tt |> equal(del.invoke("hello"), true)
    }
    t |> run("void lambda ctor") @(tt : T?) {
        var del <- OnNotify(@(msg : string) {
            feint("lambda notify: {msg}\n")
        })
        tt |> equal(del.length(), 1)
        del.invoke("ping")
        tt |> success(true)
    }
}

[test]
def test_function_constructor(t : T?) {
    t |> run("non-void function ctor") @(tt : T?) {
        var del <- OnEvent(@@event_handler)
        tt |> equal(del.length(), 1)
        tt |> equal(del.invoke("world"), true)
    }
    t |> run("void function ctor") @(tt : T?) {
        var del <- OnTick(@@tick_handler)
        tt |> equal(del.length(), 1)
        del.invoke()
        tt |> success(true)
    }
}

[test]
def test_assign_lambda(t : T?) {
    t |> run("assign sets single handler") @(tt : T?) {
        var del <- OnEvent()
        del := @(arg : string) : bool {
            return true
        }
        tt |> equal(del.length(), 1)
        tt |> equal(del.invoke("hello"), true)
    }
    t |> run("assign replaces previous") @(tt : T?) {
        var del <- OnEvent(@(arg : string) : bool {
            return true
        })
        del := @(arg : string) : bool {
            return false
        }
        tt |> equal(del.length(), 1)
        tt |> equal(del.invoke("x"), false)
    }
}

[test]
def test_assign_function(t : T?) {
    t |> run("assign from function pointer") @(tt : T?) {
        var del <- OnEvent(@(arg : string) : bool {
            return true
        })
        del := @@event_handler_false
        tt |> equal(del.length(), 1)
        tt |> equal(del.invoke("test"), false)
    }
}

[test]
def test_append_lambda(t : T?) {
    t |> run("append adds handler, last result returned") @(tt : T?) {
        var del <- OnEvent(@(arg : string) : bool {
            return true
        })
        del += @(arg : string) : bool {
            return false
        }
        tt |> equal(del.length(), 2)
        tt |> equal(del.invoke("x"), false)
    }
}

[test]
def test_append_function(t : T?) {
    t |> run("append function pointer") @(tt : T?) {
        var del <- OnEvent(@(arg : string) : bool {
            return true
        })
        del += @@event_handler_false
        tt |> equal(del.length(), 2)
        tt |> equal(del.invoke("test"), false)
    }
}

[test]
def test_multiple_handlers(t : T?) {
    t |> run("all handlers called, last result returned") @(tt : T?) {
        var del <- OnEvent()
        del += @(arg : string) : bool {
            return true
        }
        del += @(arg : string) : bool {
            return false
        }
        del += @(arg : string) : bool {
            return true
        }
        tt |> equal(del.length(), 3)
        tt |> equal(del.invoke("go"), true)
    }
}

[test]
def test_void_delegate(t : T?) {
    t |> run("void delegate invokes without error") @(tt : T?) {
        var del <- OnNotify()
        del += @(msg : string) {
            feint("got: {msg}\n")
        }
        del += @(msg : string) {
            feint("got2: {msg}\n")
        }
        tt |> equal(del.length(), 2)
        del.invoke("go")
        tt |> success(true)
    }
}

[test]
def test_clear(t : T?) {
    t |> run("clear resets delegate") @(tt : T?) {
        var del <- OnEvent(@(arg : string) : bool {
            return true
        })
        del += @(arg : string) : bool {
            return false
        }
        tt |> equal(del.length(), 2)
        del.clear()
        tt |> success(del.empty())
        tt |> equal(del.length(), 0)
        tt |> equal(del.invoke("x"), false)
    }
}

[test]
def test_multi_arg_delegate(t : T?) {
    t |> run("2-arg lambda") @(tt : T?) {
        var del <- OnCompute(@(a : int; b : float) : int {
            return a * int(b)
        })
        tt |> equal(del.invoke(3, 2.0), 6)
    }
    t |> run("2-arg function ctor") @(tt : T?) {
        var del <- OnCompute(@@compute_handler)
        tt |> equal(del.invoke(10, 2.5), 12)
    }
}

[test]
def test_zero_arg_delegate(t : T?) {
    t |> run("zero-arg void delegate invokes") @(tt : T?) {
        var del <- OnTick()
        del += @@tick_handler
        del += @@tick_handler
        tt |> equal(del.length(), 2)
        del.invoke()
        tt |> success(true)
    }
}

// ── each iteration ────────────────────────────────────────────────────

[test]
def test_each_non_void(t : T?) {
    t |> run("each iterates non-void delegate handlers") @(tt : T?) {
        var del <- OnCompute()
        del += @(a : int; b : float) : int { return a + int(b); }
        del += @(a : int; b : float) : int { return a * int(b); }
        var count = 0
        for (_cb in del) {
            count++
        }
        tt |> equal(count, 2)
    }
}

[test]
def test_each_void(t : T?) {
    t |> run("each iterates void delegate handlers") @(tt : T?) {
        var del <- OnNotify()
        del += @(msg : string) {}
        del += @(msg : string) {}
        del += @(msg : string) {}
        var count = 0
        for (_cb in del) {
            count++
        }
        tt |> equal(count, 3)
    }
}

[test]
def test_each_empty(t : T?) {
    t |> run("each on empty delegate iterates zero times") @(tt : T?) {
        var del <- OnCompute()
        var count = 0
        for (_cb in del) {
            count++
        }
        tt |> equal(count, 0)
    }
}

// ── non-copyable return type ──────────────────────────────────────────

[test]
def test_array_return(t : T?) {
    t |> run("non-copyable return type (array)") @(tt : T?) {
        var del <- OnCollect()
        del += @(n : int) : array<int> {
            var res : array<int>
            for (i in range(n)) {
                res |> push(i)
            }
            return <- res
        }
        let result <- del.invoke(3)
        tt |> equal(length(result), 3)
        tt |> equal(result[0], 0)
        tt |> equal(result[1], 1)
        tt |> equal(result[2], 2)
    }
    t |> run("non-copyable return type empty delegate") @(tt : T?) {
        var del <- OnCollect()
        let result <- del.invoke(5)
        tt |> equal(length(result), 0)
    }
}

