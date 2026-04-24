options gen2
// PERF002: character_at in loop with loop variable as index
//
// Problem:
//   `character_at(s, i)` is O(n) per call because it calls strlen internally
//   to bounds-check the index. In a loop iterating over string indices, this
//   becomes O(n^2) total.
//
// Bad pattern:
//   for (i in range(length(s))) {
//       let ch = character_at(s, i)   // O(n) per call, O(n^2) total
//   }
//
// Good pattern:
//   peek_data(s) <| $(arr) {
//       for (i in range(length(arr))) {
//           let ch = int(arr[i])       // O(1) per access
//       }
//   }

require strings

def bad_character_at_for(s : string) : int {
    var total = 0
    for (i in range(length(s))) {
        total += character_at(s, i)     // PERF002
    }
    return total
}

def bad_character_at_nested(s : string) : int {
    var total = 0
    for (i in range(length(s))) {
        for (j in range(length(s))) {
            total += character_at(s, i) // PERF002 (i is a loop var)
        }
    }
    return total
}

def bad_character_at_expr(s : string) : int {
    var total = 0
    for (i in range(length(s))) {
        total += character_at(s, i + 1) // PERF002 (loop var nested in expression)
    }
    return total
}

def bad_character_at_mul(s : string) : int {
    var total = 0
    for (i in range(length(s))) {
        total += character_at(s, i * 2) // PERF002 (loop var in expression)
    }
    return total
}

// --- Good patterns (no warnings from PERF002) ---

def good_peek_data(s : string) : int {
    var total = 0
    peek_data(s) <| $(arr) {
        for (i in range(length(arr))) {
            total += int(arr[i])        // O(1)
        }
    }
    return total
}

def good_character_at_non_loop_var(s : string) : int {
    var idx = 3
    for (i in range(length(s))) {
        idx = character_at(s, idx)      // idx is not a loop variable — no PERF002
    }
    return idx
}
