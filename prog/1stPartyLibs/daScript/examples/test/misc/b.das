options gen2
module b

struct Foo {
    t : table<string; string>
}

struct P {
    iV : int
    t : table<string; Foo>
}

var {
    p : P
}