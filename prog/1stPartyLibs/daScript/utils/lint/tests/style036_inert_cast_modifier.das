options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// STYLE036: type contract that cannot act on an already-resolved cast type
//
// Problem: -const / -& / -[] / -# / ==const / ==& are substitution contracts.
// They do work only while a generic binds, and infer clears them once it
// consumes them. A cast target is already concrete, so nothing ever consumes
// the contract — void? is void? regardless of -const.
//
// Bad pattern:
//   let p = unsafe(addr<void? -const>(x))
//
// Good pattern:
//   let p = unsafe(addr<void?>(x))

typedef CI = int const

// bad: -const on an already-resolved pointer type (1 warning)
def bad_remove_const(p : void?) : int? {
    return unsafe(reinterpret<int? -const>(p))
}

// bad: -const through the addr<T?> sugar (1 warning)
def bad_addr_sugar(var x : int) : void? {
    return unsafe(addr<void? -const>(x))
}

// bad: -# on a type that is not temporary (1 warning)
def bad_remove_temporary(p : void?) : int? {
    return unsafe(reinterpret<int? -#>(p))
}

// bad: an alias is no different — the contract is inert there too (1 warning)
def bad_alias_remove_const(p : void?) : CI? {
    return unsafe(reinterpret<CI? -const>(p))
}

// good: no contract at all
def good_plain(p : void?) : int? {
    return unsafe(reinterpret<int?>(p))
}

// good: the sugar without a contract
def good_addr_sugar(var x : int) : void? {
    return unsafe(addr<void?>(x))
}

// good: a contract on a generic parameter is consumed by substitution, so it is
// already cleared by the time any cast target is inspected
def good_generic_contract(a : array<auto(TT)> -const) : int {
    return length(a)
}

[export]
def use_generic_contract() : int {
    var xs = [1, 2, 3]
    return good_generic_contract(xs)
}
