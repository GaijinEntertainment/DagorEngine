options gen2

var x = 123

def foo() {
    print("NO")
}

[export]
def main() {
    x += 1
    if (x > 1) {
        print("This")
    } else {
        print("That")
    }
}