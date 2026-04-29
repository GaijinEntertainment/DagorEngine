options gen2
require dastest/testing_boost public
require math

struct Point {
    x : float
    y : float
}

def distance(a, b : Point) : float {
    let dx = a.x - b.x
    let dy = a.y - b.y
    return sqrt(dx * dx + dy * dy)
}

[test]
def test_struct_basics(t : T?) {
    t |> run("struct creation and access") @(t : T?) {
        var p = Point(x = 3.0, y = 4.0)
        t |> equal(p.x, 3.0)
        t |> equal(p.y, 4.0)
    }
    t |> run("struct function") @(t : T?) {
        let a = Point(x = 0.0, y = 0.0)
        let b = Point(x = 3.0, y = 4.0)
        t |> equal(distance(a, b), 5.0)
    }
}

struct Counter {
    value : int
}

def inc(var c : Counter) {
    c.value += 1
}

def get_value(c : Counter) : int {
    return c.value
}

[test]
def test_struct_mutation(t : T?) {
    t |> run("struct pass by ref") @(t : T?) {
        var c = Counter(value = 0)
        inc(c)
        inc(c)
        inc(c)
        t |> equal(get_value(c), 3)
    }
}

class Animal {
    name : string
    def abstract speak() : string
}

class Dog : Animal {
    def override speak() : string {
        return "Woof"
    }
}

class Cat : Animal {
    def override speak() : string {
        return "Meow"
    }
}

[test]
def test_classes(t : T?) {
    t |> run("class hierarchy") @(t : T?) {
        unsafe {
            var inscope dog <- new Dog()
            dog.name = "Rex"
            t |> equal(dog.name, "Rex")
            t |> equal(dog->speak(), "Woof")

            var inscope cat <- new Cat()
            cat.name = "Whiskers"
            t |> equal(cat.name, "Whiskers")
            t |> equal(cat->speak(), "Meow")
        }
    }
}

[test]
def test_arrays(t : T?) {
    t |> run("array operations") @(t : T?) {
        var arr : array<int>
        for (i in range(5)) {
            arr |> push(i * i)
        }
        t |> equal(length(arr), 5)
        t |> equal(arr[0], 0)
        t |> equal(arr[1], 1)
        t |> equal(arr[2], 4)
        t |> equal(arr[3], 9)
        t |> equal(arr[4], 16)
    }
    t |> run("table operations") @(t : T?) {
        var tab : table<string; int>
        unsafe {
            tab["one"] = 1
            tab["two"] = 2
            tab["three"] = 3
        }
        t |> equal(tab?["one"] ?? -1, 1)
        t |> equal(tab?["two"] ?? -1, 2)
        t |> equal(tab?["missing"] ?? -1, -1)
    }
}

[test]
def test_variant(t : T?) {
    t |> run("tuple") @(t : T?) {
        let tup = 42 => "hello"
        t |> equal(tup._0, 42)
        t |> equal(tup._1, "hello")
    }
}
