options gen2
// STYLE026: nested `unsafe { ... }` block
//
// Problem: `unsafe { ... unsafe { ... } ... }` — the outer block already
// elevates the entire scope to `unsafeDepth > 0`, so the inner block
// adds nothing and is pure noise. Drop the inner wrap.
//
// Bad pattern:
//   unsafe {
//       my_unsafe_void(5)
//       unsafe {
//           my_unsafe_void(7)        // <-- redundant inner block
//       }
//   }
//
// Good pattern:
//   unsafe {
//       my_unsafe_void(5)
//       my_unsafe_void(7)
//   }
//
// Detection: a counter tracks the depth of currently-open ExprUnsafe nodes
// via `preVisitExprUnsafe` / `visitExprUnsafe`. Any second push (depth > 0
// at entry) is a nested block.

expect 31209:2

require daslib/style_lint

[unsafe_operation]
def my_unsafe_void(a : int) : void {
    pass
}

// --- Bad patterns ---

def bad_nested_block() : void {
    unsafe {                                          // outer
        my_unsafe_void(5)
        unsafe {                                      // STYLE026 — nested
            my_unsafe_void(7)
        }
    }
}

def bad_deeply_nested() : void {
    unsafe {
        my_unsafe_void(1)
        unsafe {                                      // STYLE026 — depth 2
            my_unsafe_void(2)
            my_unsafe_void(3)
        }
    }
}

// --- Good patterns (silent) ---

def good_flat_block() : void {
    unsafe {                                          // no nesting
        my_unsafe_void(5)
        my_unsafe_void(7)
    }
}

def good_sibling_blocks() : void {
    // Two unrelated `unsafe { }` blocks in sequence — not nested, just
    // consecutive. STYLE026 must NOT fire on either.
    unsafe {
        my_unsafe_void(1)
        my_unsafe_void(2)
    }
    unsafe {
        my_unsafe_void(3)
        my_unsafe_void(4)
    }
}
