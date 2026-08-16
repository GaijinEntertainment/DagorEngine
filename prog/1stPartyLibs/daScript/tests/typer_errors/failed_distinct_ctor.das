options gen2

expect 30413 : 2
expect 30243

typedef distinct Id = int

[export]
def main {
    var b = Id(5.0)     // 30413: not the underlying type
    var c = Id()        // 30413: exactly one argument
    var d = Id(a = 1)   // 30243: distinct has no fields
}
