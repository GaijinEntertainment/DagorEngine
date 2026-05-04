options gen2

// Companion script for the "Sandbox" tutorial.
// This script is designed to work under restricted policies.
// It uses only safe operations — no unsafe blocks, no global heap.

var score : int = 0

[export]
def test() {
    print("=== Sandbox Tutorial ===\n")
    score = 42
    print("score = {score}\n")

    // Simple computation — no unsafe needed
    var total = 0
    for (i in range(1, 11)) {
        total += i
    }
    print("sum(1..10) = {total}\n")

    // String operations
    let greeting = "Hello from the sandbox!"
    print("{greeting}\n")
}
