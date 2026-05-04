println("=== Part 1: Truthy/falsy differences ===\n")

// In JS: "" is falsy, 0 is falsy, [] is truthy, {} is truthy
// In Quirrel...
let truthy_tests = {
  ["empty string \"\""]  = "",      // JS: falsy, Quirrel: truthy
  ["zero 0"]             = 0,       // falsy (same as JS)
  ["zero 0.0"]           = 0.0,     // falsy (same as JS)
  ["empty array []"]     = [],      // JS: truthy, Quirrel: truthy too
  ["empty table {}"]     = {},      // JS: truthy, Quirrel: truthy too
}
foreach (name, val in truthy_tests) {
  let verdict = val ? "TRUTHY" : "FALSY"
  println($"  {name} is {verdict}")
}

println("\n  Note: unlike JS, empty string is truthy in Quirrel.\n")


println("=== Part 2: Integer division (no implicit float) ===\n")

println($"  JS:  10 / 3 = 3.333...  |  Quirrel: 10 / 3 = {10 / 3}")
println($"  Want float? Use 10.0 / 3 = {10.0 / 3}")
println($"  Negative modulo: -7 % 3 = {-7 % 3}  (C-style, not math-style)\n")


println("=== Part 3: Cooperative multitasking with coroutines ===\n")

// Quirrel has real coroutines with bidirectional data passing.
// JS generators can yield, but Quirrel threads can suspend/wakeup
// with values flowing in both directions.

function fibonacci_server() {
  local a = 0, b = 1
  while (true) {
    local request = suspend({value = a, msg = "fib value"})
    if (request == "next") {
      let temp = a + b; a = b; b = temp
    } else if (request == "reset") {
      a = 0; b = 1
    }
  }
}

let fib = newthread(fibonacci_server)
local result = fib.call()       // start the coroutine
print("  Fibonacci server: ")
for (local i = 0; i < 10; i++) {
  print($"{result.value} ")
  result = fib.wakeup("next")   // send command, receive response
}
fib.wakeup("reset")
let after_reset = fib.wakeup("next")
println($"\n  After reset: {after_reset.value}")


println("\n=== Part 4: Build a DSL with metamethods ===\n")

// JS needs Proxy for this. Quirrel has _get, _set, _call, _add, _mul, _tostring...
// Let's build a fluent SQL-like query builder using metamethods.

class Query {
  _table = null
  _conditions = null
  _fields = null
  _order = null
  _limit_val = null

  constructor(table_name = null) {
    this._table = table_name
    this._conditions = []
    this._fields = ["*"]
    this._order = null
    this._limit_val = null
  }

  function select(...) {
    let q = clone this
    q._fields = vargv
    return q
  }

  function from(table_name) {
    let q = clone this
    q._table = table_name
    return q
  }

  function where(condition) {
    let q = clone this
    q._conditions = clone this._conditions
    q._conditions.append(condition)
    return q
  }

  function order_by(field) {
    let q = clone this
    q._order = field
    return q
  }

  function limit(n) {
    let q = clone this
    q._limit_val = n
    return q
  }

  // Metamethod: print the query as SQL
  function _tostring() {
    let fields_str = ", ".join(this._fields)
    local sql = $"SELECT {fields_str} FROM {this._table}"
    if (this._conditions.len() > 0) {
      let cond_str = " AND ".join(this._conditions)
      sql = $"{sql} WHERE {cond_str}"
    }
    if (this._order)
      sql = $"{sql} ORDER BY {this._order}"
    if (this._limit_val != null)
      sql = $"{sql} LIMIT {this._limit_val}"
    return sql
  }

  // Metamethod: combine two queries with UNION
  function _add(other) {
    let result = this.getclass()()
    result._table = $"({this}) UNION ({other})"
    return result
  }
}

let q = Query()
  .select("name", "age", "email")
  .from("users")
  .where("age > 18")
  .where("active = true")
  .order_by("name")
  .limit(10)

println($"  {q}")

let admins = Query().select("name").from("admins").where("role = 'super'")
let mods = Query().select("name").from("moderators").where("level > 3")
println($"  {admins + mods}")


println("\n=== Part 5: freeze - deep immutability ===\n")

// JS Object.freeze is shallow and you can still reassign the variable.
// Quirrel freeze is enforced at runtime - any mutation attempt throws an error.
// The `static` keyword creates compile-time frozen literals.

let config = freeze({
  db = freeze({
    host = "localhost"
    port = 5432
    pool = freeze([5, 10, 20])
  })
  app = freeze({
    name = "quirrel-app"
    version = "1.0"
  })
})

println($"  config.app.name = {config.app.name}")
println($"  config is frozen: {config.is_frozen()}")
println($"  config.db is frozen: {config.db.is_frozen()}")
println($"  config.db.pool is frozen: {config.db.pool.is_frozen()}")

try {
  config.app.name = "hacked"
} catch(e) {
  println($"  Mutation blocked: {e}")
}


println("\n=== Part 6: delete returns the value ===\n")

#allow-delete-operator
local inventory = {sword = 100, shield = 50, potion = 25}
let sold_item = delete inventory.sword
println($"  Sold sword for {sold_item} gold! Remaining: {inventory.len()} items")
// In JS, delete returns a boolean. In Quirrel, it returns the removed value.


println("\n=== Part 7: The _call metamethod - objects as functions ===\n")

// In JS you can't make an object callable without Proxy.
// In Quirrel, _call makes any instance behave as a function.

class Validator {
  _rules = null

  constructor(...) {
    this._rules = vargv
  }

  // env is the hidden first param - the call environment
  function _call(env, value) {
    foreach (rule in this._rules) {
      let result = rule(value)
      if (result != null)
        return result  // return error message
    }
    return null  // all passed
  }

  function _add(other) {
    // Combine validators with +
    let combined = this.getclass()()  // can't use `Validator()` inside class!
    combined._rules = clone this._rules
    combined._rules.extend(other._rules)
    return combined
  }
}

let not_null = Validator(@(v) v == null ? "must not be null" : null)
let is_string = Validator(@(v) typeof v != "string" ? "must be a string" : null)
let min_len = @(n) Validator(@(v) typeof v == "string" && v.len() < n
  ? $"must be at least {n} chars" : null)

let username_validator = not_null + is_string + min_len(3)

println($"  validate(null):   {username_validator(null)}")
println($"  validate(42):     {username_validator(42)}")
println($"  validate(\"ab\"):   {username_validator("ab")}")
println($"  validate(\"john\"): {username_validator("john") ?? "OK"}")


println("\n=== Part 8: Coroutine pipeline ===\n")

// Build a lazy data processing pipeline using coroutines.
// Each stage is a coroutine that pulls from the previous one.

function range(start, end_val) {
  for (local i = start; i < end_val; i++)
    yield i
}

function map_gen(source, fn) {
  foreach (val in source)
    yield fn(val)
}

function filter_gen(source, pred) {
  foreach (val in source)
    if (pred(val))
      yield val
}

function take_gen(source, n) {
  local count = 0
  foreach (val in source) {
    if (count >= n) return
    yield val
    count++
  }
}

// Pipeline: range(1,100) |> filter(odd) |> map(square) |> take(8)
// All lazy - only computes what's needed.
let pipeline = take_gen(
  map_gen(
    filter_gen(
      range(1, 100),
      @(x) x % 2 == 1
    ),
    @(x) x * x
  ),
  8
)

print("  Lazy pipeline (odd squares): ")
foreach (val in pipeline)
  print($"{val} ")
println("")


println("\n=== Part 9: Three-way comparison & custom sorting ===\n")

// The <=> operator returns -1, 0, or 1. JS doesn't have this.

class Card {
  suit = null
  rank = null
  static suit_order = {clubs = 0, diamonds = 1, hearts = 2, spades = 3}
  constructor(rank, suit) { this.rank = rank; this.suit = suit }
  function _cmp(other) {
    let r = this.rank <=> other.rank
    return r != 0 ? r : this.getclass().suit_order[this.suit] <=> this.getclass().suit_order[other.suit]
  }
  function _tostring() {
    let symbols = {clubs = "C", diamonds = "D", hearts = "H", spades = "S"}
    let names = {[11] = "J", [12] = "Q", [13] = "K", [14] = "A"}
    let r = this.rank in names ? names[this.rank] : $"{this.rank}"
    return $"{r}{symbols[this.suit]}"
  }
}

local hand = [Card(14, "spades"), Card(7, "hearts"), Card(14, "clubs"),
              Card(7, "diamonds"), Card(10, "spades")]
hand.sort(@(a, b) a <=> b)
print("  Sorted hand: ")
foreach (card in hand)
  print($"{card} ")


println("\n\n=== Part 10: Destructuring + null safety chain ===\n")

// Nested null-safe access - no more `obj && obj.a && obj.a.b && obj.a.b.c`

let api_response = {
  data = {
    user = {
      profile = {
        avatar = "cat.png"
      }
    }
  }
}

let avatar = api_response?.data?.user?.profile?.avatar ?? "default.png"
let missing = api_response?.data?.user?.settings?.theme ?? "dark"
println($"  Avatar: {avatar}")
println($"  Theme (missing path): {missing}")

// Destructuring with defaults - cleaner than JS
let {
  data = null
} = api_response
let {
  user = null
} = data
let {
  profile = {avatar = "fallback.png"}
} = user
println($"  Destructured avatar: {profile.avatar}")
