options gen2
// STYLE010: if (true) should be a bare block
//
// Problem: if (true) is always taken and adds unnecessary noise.
//
// Bad pattern:
//   if (true) { do_something() }
//
// Good pattern:
//   { do_something() }

// bad: if (true) (1 warning)
def bad_if_true() {
    if (true) {
        print("always\n")
    }
}

// good: bare block
def good_bare_block() {
    {
        print("always\n")
    }
}
