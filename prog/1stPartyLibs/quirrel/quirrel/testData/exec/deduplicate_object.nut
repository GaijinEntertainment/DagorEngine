// Coverage: sqdedupshrinker.cpp (exposed as base func deduplicate_object()).
// deduplicate_object(obj) freezes obj (and children) in place and, internally,
// dedups binary-equal objects against a cache. Exercises table & array shrink,
// the simple-types cache store + hit, nested recursion, non-simple-type
// rejection, the >CACHE_ITEMS_LIMIT path, the cyclic-reference guard, and null.

function caught(fn) {
  try { fn(); return "ok" }
  catch (e) { return "frozen" }
}

// simple table -> cached + frozen immutable
local t = { a = 1, b = "two", c = 3.0, d = true }
deduplicate_object(t)
println(type(t))                           // table
println(caught(function() { t.a = 99 }))   // frozen (immutable after dedup)

// simple array -> shrunk + cached + frozen
local arr = [1, 2, 3, "x", false]
deduplicate_object(arr)
println(type(arr))                         // array
println(caught(function() { arr.append(6) }))  // frozen

// two binary-equal tables in a row: store then cache-hit branch
deduplicate_object({ k = 1, s = "z" })
deduplicate_object({ k = 1, s = "z" })
// two binary-equal arrays: store then cache-hit branch
deduplicate_object([10, 20, 30])
deduplicate_object([10, 20, 30])
println("dedup-pairs-ok")

// nested tables/arrays -> recursion into children
local nested = { inner = { x = 1, y = 2 }, list = [1, [2, 3], { z = 9 }] }
deduplicate_object(nested)
println(type(nested.inner))                // table
println(type(nested.list))                 // array

// non-simple value types (function value) -> simpleTypes=false (no caching)
local ns = { fn = function() { return 1 }, n = 5 }
deduplicate_object(ns)
println(ns.n)                              // 5

// table larger than the cache item limit (>20 entries)
local big = {}
for (local i = 0; i < 30; i++)
  big[i.tostring()] <- i
deduplicate_object(big)
println(big.len())                         // 30

// array larger than the limit
local biga = []
for (local i = 0; i < 30; i++)
  biga.append(i)
deduplicate_object(biga)
println(biga.len())                        // 30

// cyclic array reference -> visited-guard prevents infinite recursion
local cyc = [1]
cyc.append(cyc)
deduplicate_object(cyc)
println("cycle-ok")

// null is accepted
deduplicate_object(null)
println("null-ok")
