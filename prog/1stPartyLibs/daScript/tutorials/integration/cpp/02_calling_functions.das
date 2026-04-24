options gen2

// Companion script for the "Calling Functions" C++ integration tutorial.
// The C++ host calls these functions, passing arguments and reading return values.

// A simple function that adds two integers.
[export]
def add(a : int; b : int) : int {
    return a + b
}

// A function that takes a float and returns its square.
[export]
def square(x : float) : float {
    return x * x
}

// A function that takes a string and returns a greeting.
[export]
def greet(name : string) : string {
    return "Hello, {name}!"
}

// A function that takes multiple argument types and prints them.
[export]
def show_types(i : int; f : float; s : string; b : bool) {
    print("int={i} float={f} string={s} bool={b}\n")
}

// A structure returned by value (complex result / cmres).
struct Vec2 {
    x : float
    y : float
}

// A function that returns a structure.
[export]
def make_vec2(x : float; y : float) : Vec2 {
    return Vec2(x = x, y = y)
}

// A function that deliberately panics — the C++ host catches the exception.
[export]
def will_fail() {
    panic("something went wrong!")
}
