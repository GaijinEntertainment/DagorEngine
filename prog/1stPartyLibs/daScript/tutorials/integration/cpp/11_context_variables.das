options gen2

// Companion script for the "Context Variables" tutorial.
// Defines global variables that the C++ host reads and writes.

require tutorial_11_cpp

// --- Scalar globals ---
var score : int = 42
var player_name : string = "Hero"
var alive : bool = true

// --- Struct global ---
var config = GameConfig(gravity = 9.8, speed = 5.0, max_enemies = 10)

// This function is called from C++ AFTER the host modifies globals.
// It prints the current values, proving C++ writes are visible.
[export]
def print_globals() {
    print("  score        = {score}\n")
    print("  player_name  = {player_name}\n")
    print("  alive        = {alive}\n")
    print("  config.gravity     = {config.gravity}\n")
    print("  config.speed       = {config.speed}\n")
    print("  config.max_enemies = {config.max_enemies}\n")
}
