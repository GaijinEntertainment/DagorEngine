options gen2
options no_aot

struct Point {
    x : float
    y : float
}

def add(a, b : int) : int {
    return a + b
}

def greet(name : string) : string {
    return "hello {name}"
}

[export]
def main() {
    print(greet("world"))
    let p = Point(x = 1.0, y = 2.0)
    print("{p.x + p.y}\n")
    let result = add(1, 2)
    print("{result}\n")
}
