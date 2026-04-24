options gen2

struct Point {
    x : float
    y : float
}

enum Color {
    Red
    Green
    Blue
}

class Animal {
    name : string
    def abstract speak() : string
}

bitfield Access {
    readable
    writable
}

variant Value {
    i : int
    f : float
}

def add(a, b : int) : int {
    return a + b
}

def greet(name : string) {
    print("Hello {name}\n")
}

[export]
def main() {
    greet("test")
    print("{add(1, 2)}\n")
}
