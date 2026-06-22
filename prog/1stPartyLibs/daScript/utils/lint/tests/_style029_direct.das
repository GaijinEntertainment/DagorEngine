options gen2

module _style029_direct

// Direct module: its own symbol is used directly, so requiring it is correct
// and STYLE029 must NOT fire.
def style029_direct_value() : int {
    return 2
}
