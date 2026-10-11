options gen2

expect 30341

require _distinct_eq_a
require _distinct_eq_b

[export]
def main {
    let a = Conn(1)
    let same = a == Conn(1)     // 30341: two modules define operator == for Conn - ambiguous, never the borrowed int compare
}
