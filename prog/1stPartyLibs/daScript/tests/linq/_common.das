options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module _common shared public

struct ComplexType { // this one can't be copied
    a : array<int>
    def operator +=(b : ComplexType) {  // we need this one for Sum and Average
        a.resize(b.a.length())
        for (i, j in a, b.a) {
            i += j
        }
    }
    def operator /=(b : uint64) {    // we need this one for Average
        let divisor = int64(b)
        for (i in a) {
            i = int(int64(i) / divisor)
        }
    }
    def operator ==(b : ComplexType) : bool {
        for (i, j in a, b.a) {
            if (i != j) {
                return false
            }
        }
        return true
    }

    def operator !=(b : ComplexType) : bool {
        for (i, j in a, b.a) {
            if (i != j) {
                return true
            }
        }
        return false
    }
}

struct Person {
    name : string
    age : int
}

let people = [
    Person(name = "Alice", age = 30),
    Person(name = "Bob", age = 25),
    Person(name = "Charlie", age = 35),
    Person(name = "Bob", age = 20),
    Person(name = "Alice", age = 25)
]

let sorted = [
    Person(name = "Alice", age = 25),
    Person(name = "Alice", age = 30),
    Person(name = "Bob", age = 20),
    Person(name = "Bob", age = 25),
    Person(name = "Charlie", age = 35)
]

let sorted_r = [
    Person(name = "Charlie", age = 35),
    Person(name = "Bob", age = 25),
    Person(name = "Bob", age = 20),
    Person(name = "Alice", age = 30),
    Person(name = "Alice", age = 25)
]

struct Pet {
    name : string
    owner : Person
    age : float
}

let magnus = Person(name = "Hedlund, Magnus")
let terry = Person(name = "Adams, Terry")
let charlotte = Person(name = "Weiss, Charlotte")
let barley = Pet(name = "Barley", owner = terry, age = 8.3)
let boots = Pet(name = "Boots", owner = terry, age = 4.9)
let whiskers = Pet(name = "Whiskers", owner = charlotte, age = 1.5)
let daisy = Pet(name = "Daisy", owner = magnus, age = 4.3)
let persons = [
    magnus, terry, charlotte
]
let pets = [
    barley, boots, whiskers, daisy
]

def Equal(a, b : array<int>) : bool {
    if (a.length() != b.length()) {
        return false
    }
    for (i, j in a, b) {
        if (i != j) {
            return false
        }
    }
    return true
}
