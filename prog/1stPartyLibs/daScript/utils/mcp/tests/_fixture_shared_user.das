options gen2
require fixture_shared_mod

[export]
def main() {
    var f = Foo(x = 42)
    print("{f.x}\n")
}
