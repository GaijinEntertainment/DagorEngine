require ecs
require failed_load_common

// fails to compile on purpose, after failed_load_common is promoted and its blocks are finalized
[export]
def use_broken() : int
  return count_ints() + no_such_function_here()
