// Valid syntax: Metamethods

// _add and _sub - use getclass() for self-reference
class Money {
    amount = 0
    constructor(a) { this.amount = a }
    function _add(other) { return this.getclass()(this.amount + other.amount) }
    function _sub(other) { return this.getclass()(this.amount - other.amount) }
    function _mul(factor) { return this.getclass()(this.amount * factor) }
    function _div(divisor) { return this.getclass()(this.amount / divisor) }
    function _modulo(m) { return this.getclass()(this.amount % m) }
    function _unm() { return this.getclass()(-this.amount) }
    function _cmp(other) {
        if (this.amount < other.amount) return -1
        if (this.amount > other.amount) return 1
        return 0
    }
    function _tostring() { return $"${this.amount}" }
    function _typeof() { return "Money" }
}

let a = Money(100)
let b = Money(50)

let sum = a + b
assert(sum.amount == 150)

let diff = a - b
assert(diff.amount == 50)

let product = a * 3
assert(product.amount == 300)

let neg = -a
assert(neg.amount == -100)

assert(a > b)
assert(b < a)

assert(typeof a == "Money")

// _get and _set
class FlexTable {
    _data = null
    constructor() { this._data = {} }
    function _get(key) {
        if (key in this._data)
            return this._data[key]
        return null
    }
    function _set(key, val) {
        this._data[key] <- val
    }
}

let ft = FlexTable()
ft.name = "test"
ft.value = 42
assert(ft.name == "test")
assert(ft.value == 42)
assert(ft.missing == null)

// _nexti for iteration
class Range {
    _start = 0
    _end = 0
    constructor(s, e) { this._start = s; this._end = e }
    function _nexti(prev) {
        if (prev == null) return this._start
        if (prev + 1 >= this._end) return null
        return prev + 1
    }
    function _get(idx) { return idx }
}

local range_sum = 0
foreach (v in Range(0, 5)) {
    range_sum += v
}
assert(range_sum == 10) // 0+1+2+3+4

// _call metamethod
class Callable {
    _value = 0
    constructor(v) { this._value = v }
    function _call(env, arg) {
        return this._value + arg
    }
}

let callable = Callable(10)
assert(callable(5) == 15)
assert(callable(20) == 30)

// _cloned metamethod
class Tracked {
    id = 0
    cloned_from = null
    constructor(id) { this.id = id }
    function _cloned(orig) {
        this.cloned_from = orig.id
    }
}

let original = Tracked(1)
let cloned = clone original
assert(cloned.cloned_from == 1)

print("metamethods: OK\n")
