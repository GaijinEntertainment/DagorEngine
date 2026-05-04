options gen2

// A simple script for standalone context demonstration.
// This will be AOT-compiled into a self-contained C++ context
// that requires no runtime compilation.

[export]
def test {
    var total = 0
    for (i in range(10)) {
        total += i
    }
    print("Sum of 0..9 = {total}\n")
}
