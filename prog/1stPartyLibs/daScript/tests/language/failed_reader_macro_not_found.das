// a `%name~ ... %%` whose reader no required module provides is one lookup error, not a
// syntax cascade: the parser reads the sequence to its `%%` and goes on
options gen2
expect 30821

def after_the_reader() : int {
    return 1
}

[export]
def main() {
    let pattern <- %nope~[a-z]+%%
    assert(after_the_reader() == 1)
}
