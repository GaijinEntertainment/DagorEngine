options gen2

[jit]
def jitted() : int {
    return 1
}

def plain() : int {
    return 2
}

[export]
def main {
    print("{jitted() + plain()}\n")
}
