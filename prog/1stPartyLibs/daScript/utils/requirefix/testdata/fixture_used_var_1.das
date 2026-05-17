options gen2

require math

// math is required and the PI variable is used.
// requirefix should report math as used:referenced with a variable ref.

[export]
def main() {
    print("{PI}\n")
}
