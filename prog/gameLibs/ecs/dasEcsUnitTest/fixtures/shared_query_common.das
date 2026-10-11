module shared_query_common shared
require ecs

// a query block in a shared module: its descriptor must outlive the group that compiled the module
def sum_ints() : int
  var sum = 0
  ecs::query() <| $ [es] (just_int_variable : int)
    sum += just_int_variable
  return sum
