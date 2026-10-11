// A piped call pads a default in for the parameter it shifts across, and lookup admits the
// candidate on that default's existence alone. When the parameter type is concrete the
// candidate is still instanced, so the argument-matching loop gives up without progress and
// the instanced function names the real mistake - which is why that exit falls through.
options gen2
expect 30161, 30341

def generic_with_mismatched_default(r : string <- 1, v) => 0

[export]
def main {
    generic_with_mismatched_default(){}
}
