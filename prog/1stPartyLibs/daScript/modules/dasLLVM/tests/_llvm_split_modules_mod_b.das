options gen2
module _llvm_split_modules_mod_b

// interp-only: called from BOTH sibling partitions, so each bakes the same
// interp-call glob name (the linkonce_odr dedup path)
[no_jit]
def public slow_b(x : int) : int => x - 2

def public helper_b(x : int) : int => x * 3 + 1

def public table_b(t : table<int; int>; k : int) : int => t?[k] ?? -1
