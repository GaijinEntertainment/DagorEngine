options gen2

require dastest/testing_boost public
require daslib/class_boost

class Animal {
    meals : int = 0

    [class_method]
    def static eat(extra : int) : int {
        meals += extra
        return meals
    }

    [class_method]
    def static const meal_count() : int {
        return meals
    }
}

class Dog : Animal {
    bark_count : int = 0

    [class_method]
    def static bark() : int {
        bark_count += 1
        return bark_count
    }
}

class Food {
    calories : int = 100
}

class Biscuits {
    health : int = 25
}

class GenericAnimal {
    meals : int = 0

    [class_method]
    def static eat(var food : auto ?) : void {
        meals += 1
    }

    def drink() : int {
        eat(self, new Food())   // version with self
        return meals
    }
}

class GenericDog : GenericAnimal {
    def lazy() : int {
        eat(new Biscuits())     // version without self
        drink()
        return meals
    }
}

class Counter {
    value : int = 0

    [explicit_const_class_method]
    def static touch() : int {
        value += 1
        return value
    }

    [explicit_const_class_method]
    def static const touch() : int {
        return value
    }
}

[test]
def test_class_method_on_classes(t : T?) {
    t |> run("class_method mutates self") @(it : T?) {
        var a : Animal? = new Animal()
        it |> equal(a.meal_count(), 0)
        it |> equal(a.eat(2), 2)
        it |> equal(a.eat(3), 5)
        it |> equal(a.meal_count(), 5)
    }

    t |> run("class_method supports inheritance") @(it : T?) {
        var d : Dog? = new Dog()
        it |> equal(d.meal_count(), 0)
        it |> equal(d.eat(4), 4)
        it |> equal(d.meal_count(), 4)
        it |> equal(d.bark(), 1)
        it |> equal(d.bark(), 2)
    }
}

[test]
def test_explicit_const_class_method_overload(t : T?) {
    t |> run("non-const overload mutates") @(it : T?) {
        var c : Counter? = new Counter()
        it |> equal(c.touch(), 1)
        it |> equal(c.touch(), 2)
    }

    t |> run("const overload is selected for const self") @(it : T?) {
        var c : Counter? = new Counter()
        c.touch()
        c.touch()
        let cc : Counter const? = c
        it |> equal(cc.touch(), 2)
        it |> equal(c.touch(), 3)
        it |> equal(cc.touch(), 3)
    }
}

[test]
def test_unqualified_generic_class_method_calls(t : T?) {
    t |> run("base_regular_method_calls_generic_static_without_self") @(it : T?) {
        var a : GenericAnimal? = new GenericAnimal()
        it |> equal(a.drink(), 1)
        it |> equal(a.meals, 1)
    }

    t |> run("derived_regular_method_calls_generic_static_without_self") @(it : T?) {
        var d : GenericDog? = new GenericDog()
        it |> equal(d.lazy(), 2)
        it |> equal(d.meals, 2)
    }
}
