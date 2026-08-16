options gen2
// STYLE039 default-off check: this fixture is not under daslib/ or modules/ and does
// not set 'options _ascii_strings', so the em-dash below must stay silent (no expect line).

require daslib/style_lint

def default_off() : string {
    return "shader failed — see log"
}

[export]
def main() {
    print("{default_off()}\n")
}
