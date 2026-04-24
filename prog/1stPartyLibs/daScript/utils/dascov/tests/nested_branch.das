options gen2

[export]
def main() {
    var x = 10
    if (x > 5) {
        if (x > 20) {
            print("deep\n")
        } else {
            print("shallow\n")
        }
    } else {
        print("low\n")
    }
}
