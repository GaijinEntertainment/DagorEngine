options gen2

expect 30341 : 6
expect 30344

typedef distinct Id = int
typedef distinct OtherId = int

def take_int(i : int) : int => i + 1

def take_id(f : Id) : int => *f

[export]
def main {
    var a = Id(5)
    take_int(a)         // 30341: Id is not int
    take_id(3)          // 30341: int is not Id
    var b : Id = 5      // 30344: no implicit init from the underlying
    let eq = a == 5     // 30341: Id == int does not borrow
    let sum = a + a     // 30341: no borrowed arithmetic
    var o = OtherId(5)
    take_id(o)          // 30341: two distincts over the same underlying are unrelated
    let eq2 = a == o    // 30341: == does not borrow across distinct types
}
