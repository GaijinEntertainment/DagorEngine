// Coverage: the Execute<debughookPresent=true> instantiation of the interpreter,
// plus StartCall<true>, Return<true> and CallDebugHook in sqvm.cpp.
//
// Execute<true> is chosen only when a debug hook is already installed at the
// moment a top-level VM call begins (sqvm.cpp dispatch). A freshly created
// thread copies the parent's _debughook flag at construction, so installing the
// hook first and then running a workload inside a thread executes that whole
// workload under Execute<true>. The workload below touches as many opcodes as
// possible so the <true> instantiation is broadly covered.
#allow-delete-operator

let dbg = require("debug")

local hookEvents = 0
dbg.setdebughook(function(etype, src, line, funcname) { hookEvents++ })

class Animal {
  name = ""
  constructor(n) { this.name = n }
  function speak() { return this.name + "!" }
}

function makeGen() {
  yield 1
  yield 2
  yield 3
}

// Broad opcode workload, executed under the debug hook (inside the thread).
function workload() {
  // integer + float arithmetic, mixed comparisons (JCMP/JCMPI/JCMPF/JCMPK)
  local s = 0
  for (local i = 0; i < 8; i++) {
    if (i % 2 == 0) s += i * 2
    else s -= 1
    local f = i + 0.5
    if (f > 3.5) s += 1
    if (f < 1.5) s -= 1
  }

  // strings, ternary, logical and/or, null-coalesce
  local label = (s > 0) ? "pos" : "nonpos"
  local ok = (s != 0) && (label.len() > 0)
  local maybe = null ?? "fallback"
  local text = label + "/" + maybe + "/" + ok.tostring()

  // tables + arrays: create, newslot, get/set, foreach
  local t = { a = 1, b = 2, c = 3 }
  t.d <- 4
  local arr = [10, 20, 30]
  arr.append(40)
  local acc = 0
  foreach (v in arr) acc += v
  foreach (k, val in t) acc += val
  arr[0] = arr[0] + acc
  delete t.d

  // closures + recursion (CALL / TAILCALL / RETURN)
  local mul = @(x, y) x * y
  function fact(n) { return n <= 1 ? 1 : n * fact(n - 1) }
  acc += mul(6, 7) + fact(5)

  // class instance, method call, instanceof, typeof, clone
  local a = Animal("cat")
  local cloned = clone arr
  local isAnimal = a instanceof Animal
  local kind = typeof a

  // exception handling under the hook (PUSHTRAP/POPTRAP/THROW)
  local caught = 0
  try { throw "boom" } catch (e) { caught = 1 }

  // generator resume under the hook (PREFOREACH/FOREACH on a generator)
  local gsum = 0
  foreach (g in makeGen()) gsum += g

  return {
    s = s, text = text, acc = acc, caught = caught,
    isAnimal = isAnimal, kind = kind, speak = a.speak(),
    clonedLen = cloned.len(), gsum = gsum
  }
}

// Run the workload inside a thread (created AFTER the hook -> Execute<true>).
local th = newthread(workload)
local res = th.call()

// Also resume a generator at top level while the hook is active.
local g = makeGen()
local total = 0
foreach (v in g) total += v

dbg.setdebughook(null)

// Deterministic summary (hook-call count varies, so only report a boolean).
println($"hook fired: {hookEvents > 0}")
println($"text: {res.text}")
println($"acc: {res.acc}")
println($"caught: {res.caught}")
println($"isAnimal: {res.isAnimal}")
println($"kind: {res.kind}")
println($"speak: {res.speak}")
println($"clonedLen: {res.clonedLen}")
println($"gsum: {res.gsum}")
println($"top-gen total: {total}")
