require ecs
require failed_load_common

// loaded after the broken script; the query block it calls lives in the promoted module
[export]
def use_count_ints() : int
  return count_ints()
