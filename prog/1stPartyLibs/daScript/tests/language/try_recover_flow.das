options gen2
require dastest/testing_boost public

var g_rounds = 5
var g_void_seen = 0
var g_refs <- [11, 22]

[export, no_jit]
def break_in_for : int {
    var seen = 0
    for (i in range(0, g_rounds)) {
        try {
            break if (i == 2)
            seen ++
        } recover {
        }
        seen += 10
    }
    return seen
}

[export, no_jit]
def continue_in_while : int {
    var seen = 0
    var i = 0
    while (i < g_rounds) {
        i ++
        try {
            continue if (i % 2 == 0)
            seen ++
        } recover {
        }
        seen += 10
    }
    return seen
}

[export, no_jit]
def break_in_recover : int {
    var seen = 0
    for (i in range(0, g_rounds)) {
        try {
            panic("stop") if (i == 2)
            seen ++
        } recover {
            break
        }
        seen += 10
    }
    return seen
}

[export, no_jit]
def nested_try : int {
    var seen = 0
    for (i in range(0, g_rounds)) {
        try {
            try {
                continue if (i == 1)
                break if (i == 3)
            } recover {
            }
            seen ++
        } recover {
        }
        seen += 10
    }
    return seen
}

[export, no_jit]
def inner_loop_break : int {
    var seen = 0
    for (i in range(0, g_rounds)) {
        try {
            for (j in range(0, 4)) {
                break if (j == 1)
                seen ++
            }
            break if (i == 2)
        } recover {
        }
        seen += 10
    }
    return seen
}

[export]
def inner_only_break : int {
    var seen = 0
    try {
        for (j in range(0, 4)) {
            break if (j == 2)
            seen ++
        }
        seen += 100
    } recover {
    }
    return seen
}

[export]
def try_without_exit : int {
    var seen = 0
    for (_i in range(0, g_rounds)) {
        try {
            seen ++
        } recover {
        }
    }
    return seen
}

[export, no_jit]
def return_in_try : int {
    try {
        return g_rounds * 2
    } recover {
    }
    return -1
}

[export, no_jit]
def void_return_in_try {
    try {
        g_void_seen = 1
        return
    } recover {
    }
    g_void_seen = 2
}

[export, no_jit]
def return_from_loop_in_try : int {
    try {
        var s = 0
        for (i in range(0, g_rounds)) {
            s += i
            return s if (s > 2)
        }
    } recover {
    }
    return -1
}

[export, no_jit]
def return_from_nested_try : int {
    try {
        try {
            return g_rounds + 4
        } recover {
        }
    } recover {
    }
    return -1
}

[export, no_jit]
def continue_and_return : int {
    var acc = 0
    for (i in range(0, g_rounds)) {
        try {
            continue if (i == 1)
            return acc if (i == 3)
            acc += i
        } recover {
        }
        acc += 100
    }
    return -1
}

[no_jit]
def moved_array : array<int> {
    try {
        return <- [1, 2, g_rounds]
    } recover {
    }
    var e : array<int>
    return <- e
}

[export]
def return_moved_array : int {
    let got <- moved_array()
    return length(got)
}

def call_it(blk : block<(x : int) : int>) : int {
    return invoke(blk, 5)
}

[export, no_jit]
def return_ref_in_try : int & {
    try {
        unsafe {
            return g_refs[0]
        }
    } recover {
    }
    unsafe {
        return g_refs[1]
    }
}

[export, no_jit]
def block_return_in_try : int {
    try {
        return call_it() $(x : int) : int {
            return x * 2
        }
    } recover {
    }
    return -1
}

[test]
def test_try_recover_control_flow(t : T?) {
    t |> equal(break_in_for(), 22, "break leaves the for loop")
    t |> equal(continue_in_while(), 33, "continue skips the rest of the while iteration")
    t |> equal(break_in_recover(), 22, "break in the recover block leaves the loop")
    t |> equal(nested_try(), 22, "nested try chains the exit outwards")
    t |> equal(inner_loop_break(), 23, "break in a loop inside the try stays inner")
    t |> equal(inner_only_break(), 102, "a loop break with no loop around the try needs no exit")
    t |> equal(try_without_exit(), 5, "try with no exit is unaffected")
    t |> equal(return_in_try(), 10, "return carries its value out")
    void_return_in_try()
    t |> equal(g_void_seen, 1, "a void return needs no slot")
    t |> equal(return_from_loop_in_try(), 3, "return crosses a loop inside the try")
    t |> equal(return_from_nested_try(), 9, "nested try shares the return slot")
    t |> equal(continue_and_return(), 202, "one try with continue and return")
    t |> equal(return_moved_array(), 3, "moved array survives the return slot")
    t |> equal(block_return_in_try(), 10, "a block return is not the function's")
    return_ref_in_try() = 99
    t |> equal(g_refs[0], 99, "a returned reference rides the slot as a pointer to the same storage")
}
