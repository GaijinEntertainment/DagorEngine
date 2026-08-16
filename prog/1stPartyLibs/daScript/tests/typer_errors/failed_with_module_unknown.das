options gen2

expect 30298

[export]
def main {
    with (module totally_unknown_module) {  // 30298: module is not part of the program
        let x = 1
    }
}
