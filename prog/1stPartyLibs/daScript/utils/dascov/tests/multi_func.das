options gen2

def called_func() {
    print("called\n")
}

def uncalled_func() {
    print("uncalled\n")
}

[export]
def main() {
    called_func()
}
