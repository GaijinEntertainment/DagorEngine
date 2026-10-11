options gen2, no_aot, auto_inline_functions = false   // cursor tests assert node positions; splices would move them
require daslib/fio

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
    to_log(LOG_INFO, greet("world"))
    let p = Point(x = 1.0, y = 2.0)
    to_log(LOG_INFO, "{p.x + p.y}\n")
    let result = add(1, 2)
    to_log(LOG_INFO, "{result}\n")
    for (i in range(2)) {
        to_log(LOG_INFO, "{i}\n")
    }
    var pet = new Cat()
    to_log(LOG_INFO, "{pet->speak()}\n")
}

class Animal {
    def speak() : string => "..."
}

class Cat : Animal {
    def override speak() : string => "meow"
}

def no_log() : file {
    return null
}

class Dog : Animal {
    age : int = 3
}

def safe_x(p : Point?) : float {
    return p?.x ?? 0.0
}

def twice(x) => x + x

def use_twice() : int {
    return twice(2)
}

def count_up() : iterator<int> {
    return <- generator<int> {
        yield 1
        return false
    }
}

def kennel(var d : Dog?) : int {
    let heard = d->speak()
    return d.age + length(heard)
}

class Puppy : Dog {
    def bark() : string => "yip"
    def override speak() : string => bark()
}

struct Greeter {
    cb : function<(name : string) : string> = @@greet
}

def captured() : int {
    let base = 10
    let plus = @(x : int) : int => x + base
    return plus(1)
}
