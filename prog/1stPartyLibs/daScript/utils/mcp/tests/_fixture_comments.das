// header block stays
options gen2

//! doc comment survives
def documented(a : int) : int {
    // narration dies
    let x = a + 1  // trailing dies
    /* block dies */
    let y = /* inline dies */ x * 2
    let z = x /* multi
    line dies */ + y
    return z  // nolint:STYLE005 — kept with its why
}

// standalone narration dies

def two() {
    // @nolint kept form
    return
}

[export]
def main() {
    two()
}
