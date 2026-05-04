options gen2

// Companion script for the "Binding C++ Enumerations" tutorial.
// The module "tutorial_05_cpp" exposes Direction and Severity enums
// along with helper functions from C++.

require tutorial_05_cpp

[export]
def test() {
    // --- Using the macro-bound enum (Direction) ---
    print("=== Direction enum ===\n")

    // Access enum values with dot syntax
    let dir = Direction.North
    print("dir = {direction_name(dir)}\n")

    // Passing enum to functions
    print("opposite of North = {direction_name(opposite_direction(dir))}\n")

    // Rotating through all directions
    var current = Direction.North
    print("Rotating CW: ")
    for (i in range(4)) {
        print("{direction_name(current)} ")
        current = rotate_cw(current)
    }
    print("\n")

    // Enum comparison
    let east = Direction.East
    let west = Direction.West
    print("East == West? {east == west}\n")
    print("opposite(East) == West? {opposite_direction(east) == west}\n")

    // --- Using the manually-bound enum (Severity) ---
    print("\n=== Severity enum ===\n")

    // Each level prints with its prefix via C++
    log_message(Severity.Debug,   "system starting up")
    log_message(Severity.Info,    "ready to serve")
    log_message(Severity.Warning, "disk space low")
    log_message(Severity.Error,   "connection lost")

    // Severity check
    print("Debug is severe?   {is_severe(Severity.Debug)}\n")
    print("Warning is severe? {is_severe(Severity.Warning)}\n")
    print("Error is severe?   {is_severe(Severity.Error)}\n")
}