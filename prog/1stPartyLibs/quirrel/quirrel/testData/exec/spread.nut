function dumpArray(a) {
  local s = ""
  foreach (v in a)
    s = $"{s}{v} "
  println($"array: {s}")
}

function dumpTable(t) {
  let pairs = []
  foreach (k, v in t)
    pairs.append($"{k}={v}")
  pairs.sort()
  local s = ""
  foreach (p in pairs)
    s = $"{s}{p} "
  println($"table: {s}")
}

function expectError(fn) {
  try {
    fn()
    println("no error")
  }
  catch (e) {
    println(e)
  }
}

let a = [1, 2]

dumpArray([...a])
dumpArray([0, ...a])
dumpArray([...a, 3])
dumpArray([0, ...a, 3, ...a])
dumpArray([...[]])
dumpArray([...a, ...[], ...a])
dumpArray(a)

let t = { x = 1, y = 2 }

dumpTable({ ...t })
dumpTable({ z = 0, ...t })
dumpTable({ ...t, y = 20 })
dumpTable({ y = 20, ...t })
dumpTable({ ...t, ...{ y = 20, z = 30 } })
dumpTable(t)

let nothing = null
dumpArray([...nothing, 7])
dumpTable({ ...nothing, k = 1 })

let frozenTable = freeze({ p = 1 })
let thawed = { ...frozenTable }
thawed.p = 5
dumpTable(thawed)
dumpTable(frozenTable)

let frozenArray = freeze([1, 2])
let thawedArray = [...frozenArray]
thawedArray.append(3)
dumpArray(thawedArray)

class Point {
  static dims = 2
  x = 10
  function len() { return 0 }
}

dumpTable({ ...Point, len = "overridden" })
let fromInstance = { ...Point() }
println(fromInstance.x)
println(fromInstance.dims)
println(typeof fromInstance.len)

const CONST_ARRAY = [1, 2]
const SPREAD_CONST_ARRAY = [0, ...CONST_ARRAY, 3]
dumpArray(SPREAD_CONST_ARRAY)

const CONST_TABLE = { a = 1, b = 2 }
const SPREAD_CONST_TABLE = { ...CONST_TABLE, b = 20, c = 3 }
dumpTable(SPREAD_CONST_TABLE)
dumpTable(const { ...CONST_TABLE, b = 20, c = 3 })

const CONST_NOTHING = null
const SPREAD_CONST_NULL_ARRAY = [1, ...CONST_NOTHING, 2]
const SPREAD_CONST_NULL_TABLE = { a = 1, ...CONST_NOTHING }
dumpArray(SPREAD_CONST_NULL_ARRAY)
dumpTable(SPREAD_CONST_NULL_TABLE)

dumpArray([...CONST_ARRAY])
dumpTable({ ...CONST_TABLE })

expectError(@() [...t])
expectError(@() [...5])
expectError(@() ({ ...a }))
expectError(@() ({ ...5 }))

// a spread that starts a table literal takes the fast path in the VM; a spread
// that follows a key does not. The two must agree on everything below.
function copyLeadingSpread(source) { return { ...source } }
function copyTrailingSpread(source) { return { lead = 0, ...source } }

function makeWeakValueHolder() {
  local target = { id = "kept alive by the copy" }
  return { w = target.weakref() }
}

function makeWeakElementHolder() {
  local target = { id = "kept alive by the copy" }
  return [target.weakref()]
}
let weakElements = [...makeWeakElementHolder()]

let leadingWeak = copyLeadingSpread(makeWeakValueHolder())
let trailingWeak = copyTrailingSpread(makeWeakValueHolder())
function churn() {
  local junk = []
  for (local i = 0; i < 1000; i++)
    junk.append({ i = i })
  return junk.len()
}
churn()
println(weakElements[0].id)
println(leadingWeak.w.id)
println(trailingWeak.w.id)

// a null key is a literal node, not an absent one, so it is not a spread
let nullKeyed = { [null] = "n", a = 1 }
dumpTable({ ...nullKeyed })
dumpTable({ [null] = "x", ...nullKeyed })
const CONST_NULL_KEYED = { [null] = "c" }
dumpTable({ ...CONST_NULL_KEYED })

let frozenArrayOfTables = freeze([{ q = 1 }])
expectError(@() [...frozenArrayOfTables][0].q = 5)
expectError(@() [0, ...frozenArrayOfTables][1].q = 5)

let frozenValue = { p = freeze({ q = 1 }) }
let leadingFrozen = copyLeadingSpread(frozenValue)
let trailingFrozen = copyTrailingSpread(frozenValue)
expectError(@() leadingFrozen.p.q = 5)
expectError(@() trailingFrozen.p.q = 5)

let frozenSource = freeze({ p = { q = 1 } })
expectError(@() copyLeadingSpread(frozenSource).p.q = 5)
expectError(@() copyTrailingSpread(frozenSource).p.q = 5)

dumpTable(copyLeadingSpread(t))
dumpTable(copyTrailingSpread(t))

let shorthand = 0
dumpTable({ shorthand
  ...t })
