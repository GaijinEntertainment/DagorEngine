// Tutorial 51: Delegates
//
// This tutorial covers:
//   - What delegates are (multicast callbacks)
//   - Declaring delegate types with typedef + type macro
//   - Construction from lambdas and function pointers
//   - Assignment (:=) replaces all handlers with one
//   - Append (+=) adds additional handlers
//   - Invoking delegates (all handlers called, last result returned)
//   - Querying: empty, length, clear
//   - Iterating handlers with for-loop (each)
//   - Void vs non-void delegates
//   - Practical example: event system
//
// Run: daslang.exe tutorials/language/51_delegate.das

options gen2
options no_unused_function_arguments = false

require daslib/delegate

// ============================================================
// Section 1: What are delegates?
// ============================================================
// A delegate is a multicast callback — a list of handlers
// (lambdas or function pointers) that are all invoked when
// the delegate is called.  Think C# events/delegates.
//
// The daslib/delegate module provides a type macro that
// generates a full delegate struct from a function signature.

// ============================================================
// Section 2: Declaring delegate types
// ============================================================
// Use typedef with the delegate() type macro.
// Pass either function<...> or lambda<...> — both produce
// the same delegate type (internally stores lambdas).

typedef OnDamage  = delegate(type<function<(target : string; amount : int) : int>>)
typedef OnNotify  = delegate(type<function<(msg : string) : void>>)
typedef OnTick    = delegate(type<function<() : void>>)

// ============================================================
// Section 3: Construction from lambdas
// ============================================================

def test_lambda_construction() {
    print("=== Lambda construction ===\n")
    var del <- OnDamage(@(target : string; amount : int) : int {
        print("  hit {target} for {amount}\n")
        return amount
    })
    let result = del.invoke("goblin", 10)
    print("  result = {result}\n")
    // output:
    //   hit goblin for 10
    //   result = 10
}

// ============================================================
// Section 4: Construction from function pointers
// ============================================================

def apply_damage(target : string; amount : int) : int {
    print("  apply_damage({target}, {amount})\n")
    return amount * 2
}

def test_function_construction() {
    print("\n=== Function pointer construction ===\n")
    var del <- OnDamage(@@apply_damage)
    let result = del.invoke("dragon", 25)
    print("  result = {result}\n")
    // output:
    //   apply_damage(dragon, 25)
    //   result = 50
}

// ============================================================
// Section 5: Assignment (:=) — replace all handlers
// ============================================================

def test_assignment() {
    print("\n=== Assignment (:=) ===\n")
    var del <- OnDamage()
    del := @(target : string; amount : int) : int {
        print("  handler A: {target} {amount}\n")
        return amount
    }
    print("  length after first := is {del.length()}\n")

    // := replaces the previous handler
    del := @(target : string; amount : int) : int {
        print("  handler B: {target} {amount}\n")
        return amount * 3
    }
    print("  length after second := is {del.length()}\n")
    let result = del.invoke("troll", 5)
    print("  result = {result}\n")
    // output:
    //   handler B: troll 5
    //   result = 15
}

// ============================================================
// Section 6: Append (+=) — add handlers
// ============================================================

def test_append() {
    print("\n=== Append (+=) ===\n")
    var del <- OnDamage()
    del += @(target : string; amount : int) : int {
        print("  handler 1: {amount}\n")
        return amount
    }
    del += @(target : string; amount : int) : int {
        print("  handler 2: {amount * 2}\n")
        return amount * 2
    }
    del += @@apply_damage
    print("  {del.length()} handlers registered\n")

    // All handlers are called; the last handler's return value is returned
    let result = del.invoke("orc", 10)
    print("  result (from last handler) = {result}\n")
    // output:
    //   handler 1: 10
    //   handler 2: 20
    //   apply_damage(orc, 10)
    //   result (from last handler) = 20
}

// ============================================================
// Section 7: Void delegates
// ============================================================

def test_void_delegate() {
    print("\n=== Void delegate ===\n")
    var del <- OnNotify()
    del += @(msg : string) {
        print("  log: {msg}\n")
    }
    del += @(msg : string) {
        print("  alert: {msg}\n")
    }
    del.invoke("server started")
    // output:
    //   log: server started
    //   alert: server started
}

// ============================================================
// Section 8: Empty, length, clear
// ============================================================

def test_utilities() {
    print("\n=== Utilities ===\n")
    var del <- OnTick()
    print("  empty = {del.empty()}, length = {del.length()}\n")

    del += @() {
        print("  tick!\n")
    }
    del += @() {
        print("  tock!\n")
    }
    print("  empty = {del.empty()}, length = {del.length()}\n")

    del.clear()
    print("  after clear: empty = {del.empty()}, length = {del.length()}\n")
    // output:
    //   empty = true, length = 0
    //   empty = false, length = 2
    //   after clear: empty = true, length = 0
}

// ============================================================
// Section 9: Iterating handlers
// ============================================================

def test_iteration() {
    print("\n=== Iteration ===\n")
    var del <- OnNotify()
    del += @(msg : string) {
        print("  handler A: {msg}\n")
    }
    del += @(msg : string) {
        print("  handler B: {msg}\n")
    }
    del += @(msg : string) {
        print("  handler C: {msg}\n")
    }

    // for-loop uses the each() iterator
    var count = 0
    for (_handler in del) {
        count++
    }
    print("  iterated over {count} handlers\n")
    // output:
    //   iterated over 3 handlers
}

// ============================================================
// Section 10: Practical example — simple event system
// ============================================================

typedef OnPlayerHit = delegate(type<function<(player : string; damage : int) : int>>)

def armor_reduction(player : string; damage : int) : int {
    let reduced = damage - 5
    print("  armor: {damage} -> {reduced}\n")
    return reduced > 0 ? reduced : 0
}

def log_hit(player : string; damage : int) : int {
    print("  log: {player} took {damage} damage\n")
    return damage
}

def test_event_system() {
    print("\n=== Event system example ===\n")
    var on_hit <- OnPlayerHit()

    // register handlers
    on_hit += @@armor_reduction
    on_hit += @@log_hit

    // fire the event
    let final_damage = on_hit.invoke("Hero", 12)
    print("  final damage = {final_damage}\n")

    // replace all handlers with a single one
    on_hit := @(player : string; damage : int) : int {
        print("  god mode: {player} takes 0 damage\n")
        return 0
    }
    let god_damage = on_hit.invoke("Hero", 100)
    print("  final damage = {god_damage}\n")
    // output:
    //   armor: 12 -> 7
    //   log: Hero took 12 damage
    //   final damage = 12
    //   god mode: Hero takes 0 damage
    //   final damage = 0
}

// ============================================================
// Main
// ============================================================

[export]
def main() {
    test_lambda_construction()
    test_function_construction()
    test_assignment()
    test_append()
    test_void_delegate()
    test_utilities()
    test_iteration()
    test_event_system()
}
