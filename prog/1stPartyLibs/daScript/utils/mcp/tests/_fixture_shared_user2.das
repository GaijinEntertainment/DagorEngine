options gen2
require fixture_shared_mod

[export]
def main() {
    var b = Bar(y = 99)
    print("{b.y}\n")
}
