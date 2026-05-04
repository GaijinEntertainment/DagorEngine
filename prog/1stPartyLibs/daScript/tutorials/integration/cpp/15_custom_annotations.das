options gen2

// Companion script for the "Custom Annotations" tutorial.
// Annotations defined in C++ are used to modify struct and function
// behavior at compile time.

require tutorial_15_cpp

// --- [log_calls] function annotation ---
// The C++ annotation prints a message each time an annotated function
// is compiled, and can inject custom simulation behavior.

[log_calls]
def attack(target : string; damage : int) {
    print("  {target} takes {damage} damage!\n")
}

[log_calls]
def heal(target : string; amount : int) {
    print("  {target} heals for {amount} hp\n")
}

// A function WITHOUT the annotation — no compile-time message
def normal_function() {
    print("  (this function has no annotation)\n")
}

// --- [add_field] struct annotation ---
// The C++ annotation adds an "id" field to the struct at compile time.

[add_field]
struct Monster {
    name  : string
    hp    : int
}

[export]
def test() {
    print("=== Custom Annotations Tutorial ===\n\n")

    // Functions annotated with [log_calls]
    print("--- Annotated functions ---\n")
    attack("Goblin", 25)
    heal("Hero", 10)

    print("\n--- Normal function ---\n")
    normal_function()

    // Struct annotated with [add_field] — has an extra "id" field
    print("\n--- Annotated struct ---\n")
    var m = Monster(name = "Dragon", hp = 500, id = 42)
    print("  {m.name}: hp={m.hp}, id={m.id}\n")
}
