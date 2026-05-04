options gen2
// PERF003: character_at anywhere (informational)
//
// Problem:
//   `character_at(s, i)` is O(n) per call because it internally calls strlen
//   to validate the index is in bounds. For isolated single-character checks
//   this is acceptable, but in hot paths consider peek_data.
//
// Alternatives:
//   peek_data(s) <| $(arr) {
//       let ch = int(arr[i])           // O(1) per access, one strlen total
//   }
//
//   modify_data(s) <| $(var arr) {
//       arr[i] = uint8('X')            // O(1) mutation, returns new string
//   }

require strings

def info_character_at_standalone(s : string) : int {
    return character_at(s, 0)              // PERF003 (informational)
}

def info_character_at_if(s : string) : bool {
    if (character_at(s, 0) == 'h') {       // PERF003 (informational)
        return true
    }
    return false
}

// --- No warnings for peek_data ---

def good_peek_data_single(s : string) : int {
    var ch = 0
    peek_data(s) <| $(arr) {
        ch = int(arr[0])
    }
    return ch
}
