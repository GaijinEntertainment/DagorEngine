// Tutorial 3: Operators and Expressions
//
// This tutorial covers:
//   - Arithmetic operators: +  -  *  /  %
//   - Comparison operators: ==  !=  <  >  <=  >=
//   - Logical operators: &&  ||  !
//   - Bitwise operators: &  |  ^  ~  <<  >>
//   - Assignment operators: +=  -=  *=  /=  etc.
//   - Ternary operator: condition ? a : b
//   - Increment/decrement: ++  --
//   - Copy (=) vs move (<-)
//
// Run: daslang.exe tutorials/language/03_operators.das

options gen2

[export]
def main {

    // === Arithmetic ===
    // Both operands must be the same type — no implicit promotion.
    let a = 17
    let b = 5
    print("a={a}  b={b}\n")
    print("a + b = {a + b}\n")
    print("a - b = {a - b}\n")
    print("a * b = {a * b}\n")
    print("a / b = {a / b}\n")      // integer division (truncates)
    print("a % b = {a % b}\n")      // modulo (remainder)

    // Float arithmetic
    let x = 10.0
    let y = 3.0
    print("x / y = {x / y}\n")     // float division

    // === Comparison ===
    print("5 == 5 is {5 == 5}\n")
    print("5 != 3 is {5 != 3}\n")
    print("5 > 3 is {5 > 3}\n")
    print("5 < 3 is {5 < 3}\n")
    print("5 >= 5 is {5 >= 5}\n")
    print("5 <= 4 is {5 <= 4}\n")

    // === Logical ===
    let t = true
    let f = false
    print("true && false = {t && f}\n")
    print("true || false = {t || f}\n")
    print("!true = {!t}\n")

    // Short-circuit evaluation: && stops at false, || stops at true.

    // === Ternary (conditional) operator ===
    let val = 42
    let desc = val > 0 ? "positive" : "non-positive"
    print("{val} is {desc}\n")

    // === Increment and decrement ===
    var counter = 10
    counter++
    print("after ++: {counter}\n")    // 11
    counter--
    print("after --: {counter}\n")    // 10

    // === Compound assignment ===
    var n = 100
    n += 10
    print("n += 10: {n}\n")    // 110
    n -= 20
    print("n -= 20: {n}\n")    // 90
    n *= 2
    print("n *= 2: {n}\n")     // 180
    n /= 3
    print("n /= 3: {n}\n")     // 60
    n %= 7
    print("n %= 7: {n}\n")    // 4

    // === Bitwise operators ===
    let m = 0x0F
    let k = 0xF0
    print("0x0F & 0xF0 = {int(m & k)}\n")   // 0
    print("0x0F | 0xF0 = {int(m | k)}\n")   // 255
    print("0x0F ^ 0xF0 = {int(m ^ k)}\n")   // 255
    print("0x0F << 4 = {int(m << 4u)}\n")    // 240

    // === Copy (=) vs Move (<-) — introduction ===

    // = is copy assignment — works for simple types and copyable values.
    var p = 10
    var q = p       // q is a copy of p
    q = 99
    print("p={p}  q={q}\n")    // p is unchanged

    // <- is move assignment — transfers ownership, zeroes the source.
    // This matters for arrays, tables, lambdas, and other non-copyable types.
    // We'll see move in action in the Arrays tutorial (06).
    // For simple types like int, move works the same as copy.
    var s = 42
    var r <- s      // r gets 42, s is zeroed
    print("r={r}  s={s}\n")
}

// output:
// a=17  b=5
// a + b = 22
// a - b = 12
// a * b = 85
// a / b = 3
// a % b = 2
// x / y = 3.3333333
// 5 == 5 is true
// 5 != 3 is true
// 5 > 3 is true
// 5 < 3 is false
// 5 >= 5 is true
// 5 <= 4 is false
// true && false = false
// true || false = true
// !true = false
// 42 is positive
// after ++: 11
// after --: 10
// n += 10: 110
// n -= 20: 90
// n *= 2: 180
// n /= 3: 60
// n %= 7: 4
// 0x0F & 0xF0 = 0
// 0x0F | 0xF0 = 255
// 0x0F ^ 0xF0 = 255
// 0x0F << 4 = 240
// p=10  q=99
// r=42  s=0
