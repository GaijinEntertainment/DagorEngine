options gen2

def describe(x : int) {
    print("int: {x}\n")
}

def describe(x : float) {
    print("float: {x}\n")
}

def helper() {
    print("helper\n")
}

[export]
def main() {
    describe(42)
    describe(3.14)
    helper()
}
