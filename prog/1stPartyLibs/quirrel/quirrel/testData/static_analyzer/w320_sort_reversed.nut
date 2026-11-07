//-file:declared-never-used
//-file:named-like-should-return
//-file:all-paths-return-value
//-file:result-not-utilized

function test_reversed(arr) {
  // should warn: the callback returns nothing
  arr.sort_reversed(function(a, b) { print(a + b) })
  arr.sortby_reversed(function(v) { print(v) })

  // should not warn
  arr.sort_reversed(@(a, b) a <=> b)
  arr.sortby_reversed(@(v) v)
  arr.sort_reversed()
}

function sorts_shared_default(a = []) {
  a.sort_reversed()
}

function sorts_by_key_shared_default(a = []) {
  a.sortby_reversed(@(v) v)
}

function sorts_temporary_or_param(arr) {
  return (arr ?? []).sort_reversed()
}
