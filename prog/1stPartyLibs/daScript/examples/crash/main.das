options gen2
options no_fast_call

require crash

def deep_function() {
    native_crash()
}

def middle_function() {
    deep_function()
}

def outer_function() {
    middle_function()
}

[export]
def main() {
    outer_function()
}
