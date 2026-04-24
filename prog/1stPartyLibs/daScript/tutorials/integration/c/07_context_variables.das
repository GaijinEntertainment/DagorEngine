options gen2

// Companion script for the C "Context Variables" tutorial.
// Defines simple global variables that the C host reads and writes.
// No custom struct binding needed — only scalar types.

// --- Scalar globals ---
var score : int = 42
var speed : float = 3.14
var player_name : string = "Hero"
var alive : bool = true

// This function is called from C AFTER the host modifies globals.
// It prints the current values, proving the C writes are visible.
[export]
def print_globals() {
    print("  score       = {score}\n")
    print("  speed       = {speed}\n")
    print("  player_name = {player_name}\n")
    print("  alive       = {alive}\n")
}
