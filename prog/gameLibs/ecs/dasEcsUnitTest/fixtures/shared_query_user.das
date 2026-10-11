require ecs
require shared_query_common

// compiled by unit_test.cpp in a throwaway module group, the way the AOT and dry-run tools do
[export]
def use_shared_query() : int
  return sum_ints()
