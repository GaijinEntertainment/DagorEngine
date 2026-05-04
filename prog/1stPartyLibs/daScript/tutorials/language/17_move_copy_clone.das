// Tutorial 17: Move, Copy, and Clone
//
// This tutorial covers:
//   - Copy (=) — bitwise copy for POD types
//   - Move (<-) — transfer ownership, source zeroed
//   - Clone (:=) — deep copy, source unchanged
//   - Which types support which operators
//   - return <- for non-copyable types
//   - Struct field initialization modes
//   - Custom clone functions
//   - Relaxed assign (automatic = to <- promotion)
//
// Run: daslang.exe tutorials/language/17_move_copy_clone.das

options gen2

struct Inventory {
    items : array<string>
    weight : int
}

struct GameState {
    name : string
    scores : array<int>
    level : int
}

// Custom clone: controls what := does for GameState
def clone(var dst : GameState; src : GameState) {
    dst.name = src.name
    dst.scores := src.scores        // clone the array normally
    dst.level = src.level + 1000    // mark clones with offset
}

def make_inventory() : Inventory {
    var inv : Inventory
    inv.items |> push("sword")
    inv.items |> push("shield")
    inv.weight = 15
    return <- inv     // must use <- because Inventory contains array
}

def make_numbers() : array<int> {
    var result : array<int>
    result |> push(10)
    result |> push(20)
    result |> push(30)
    return <- result  // move out of function
}

[export]
def main {

    // === Copy (=) ===

    // For simple types, = does a bitwise copy. Both sides remain valid.
    var a = 42
    var b = a
    print("copy int: a={a}, b={b}\n")

    var name = "Alice"
    var greeting = "Hello, {name}"
    print("copy string: {greeting}\n")

    // Structs with only POD fields can be copied too:
    // (struct with arrays CANNOT be copied — see below)

    // === Move (<-) ===

    // Move transfers ownership. The source is zeroed after the move.
    // This is the primary way to work with arrays, tables, and other containers.

    var nums <- make_numbers()
    print("moved array: {nums}\n")

    // Move between variables
    var other <- nums
    print("after move: other={other}\n")
    print("nums is now empty: length={length(nums)}\n")

    // === Clone (:=) ===

    // Clone creates a deep, independent copy. Both sides remain valid.
    var original : array<int>
    original |> push(1)
    original |> push(2)
    original |> push(3)

    var copy : array<int>
    copy := original
    copy |> push(999)

    print("original: {original}\n")
    print("clone:    {copy}\n")

    // Clone initialization at declaration
    var another := original
    print("clone-init: {another}\n")

    // === Structs with containers ===

    // Structs containing arrays/tables must be moved or cloned, not copied.
    var inv <- make_inventory()
    print("inventory: {inv.items}, weight={inv.weight}\n")

    // Clone the entire struct (deep copies the array field)
    var backup := inv
    backup.items |> push("potion")
    print("original items: {inv.items}\n")
    print("backup items:   {backup.items}\n")

    // === Struct field initialization ===

    // Each field can use a different mode in construction:
    var weapons : array<string>
    weapons |> push("bow")
    weapons |> push("arrow")

    // Move the array into the struct field with <-
    var loadout = Inventory(items <- weapons, weight = 5)
    print("loadout: {loadout.items}\n")
    print("weapons after move: length={length(weapons)}\n")

    // === Type compatibility summary ===

    // Type            =    <-   :=
    // -----------------------------
    // int, float      ✓    ✓    ✓
    // string          ✓    ✓    ✓
    // array<T>        ✗    ✓    ✓
    // table<K;V>      ✗    ✓    ✓
    // lambda          ✗    ✓    ✗
    // iterator        ✗    ✓    ✗
    // block           ✗    ✗    ✗
    // POD struct      ✓    ✓    ✓
    // struct w/array  ✗    ✓    ✓

    // === Relaxed assign ===

    // By default, the compiler auto-promotes = to <- when:
    //   - the right side is temporary (literal, function return)
    //   - the type can't be copied but can be moved
    // So this works:
    var data = make_numbers()   // auto-promoted to: var data <- make_numbers()
    print("relaxed: {data}\n")

    // To disable this: options relaxed_assign = false
    // Then you must always write <- explicitly for non-copyable types.

    // === Custom clone function ===

    // Define a custom 'clone' function to control how := works.
    // The signature is: def clone(var dst : T; src : T)

    var game = GameState(name = "save1", level = 5)
    game.scores |> push(100)
    game.scores |> push(200)

    var save := game    // uses our custom clone below
    print("game:  level={game.level}, scores={game.scores}\n")
    print("save:  level={save.level}, scores={save.scores}\n")
    // save.level is 5 + 1000 = 1005 (our custom clone marks copies)
}

// output:
// copy int: a=42, b=42
// copy string: Hello, Alice
// moved array: [[ 10; 20; 30]]
// after move: other=[[ 10; 20; 30]]
// nums is now empty: length=0
// original: [[ 1; 2; 3]]
// clone:    [[ 1; 2; 3; 999]]
// clone-init: [[ 1; 2; 3]]
// inventory: [[ sword; shield]], weight=15
// original items: [[ sword; shield]]
// backup items:   [[ sword; shield; potion]]
// loadout: [[ bow; arrow]]
// weapons after move: length=0
// relaxed: [[ 10; 20; 30]]
// game:  level=5, scores=[[ 100; 200]]
// save:  level=1005, scores=[[ 100; 200]]
