module failed_load_common shared
require ecs

// promoted while the script that requires it compiles; the failed compile must not take this
// block's descriptor with it
def count_ints() : int
  var n = 0
  ecs::query() <| $ [es] (just_int_variable : int)
    n++
  return n
