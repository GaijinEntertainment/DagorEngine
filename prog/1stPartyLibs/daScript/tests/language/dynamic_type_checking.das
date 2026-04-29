options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
options rtti

require dastest/testing_boost public
require daslib/dynamic_cast_rtti

class Shape {
    def abstract const area() : float
}

class Circle : Shape {
    radius : float = 1.0
    def override const area() : float {
        return 3.14159 * radius * radius
    }
}

class Rectangle : Shape {
    width : float = 1.0
    height : float = 1.0
    def override const area() : float {
        return width * height
    }
}

class Square : Rectangle {
    def Square(s : float) {
        self.width = s
        self.height = s
    }
}

struct ShapeHolder {
    shape : Shape?
}

// -- original tests --

[test]
def test_is_instance_of(t : T?) {
    t |> run("circle is Circle") @(t : T?) {
        var c = new Circle()
        t |> equal(is_instance_of(c, type<Circle>), true)
        t |> equal(is_instance_of(c, type<Shape>), true)
        t |> equal(is_instance_of(c, type<Rectangle>), false)
    }
    t |> run("square is Rectangle and Shape") @(t : T?) {
        var sq = new Square(5.0)
        t |> equal(is_instance_of(sq, type<Square>), true)
        t |> equal(is_instance_of(sq, type<Rectangle>), true)
        t |> equal(is_instance_of(sq, type<Shape>), true)
        t |> equal(is_instance_of(sq, type<Circle>), false)
    }
}

[test]
def test_dynamic_type_cast(t : T?) {
    t |> run("cast Shape? to Circle?") @(t : T?) {
        var s : Shape? = new Circle()
        var c = dynamic_type_cast(s, type<Circle>)
        t |> equal(c != null, true)
        var r = dynamic_type_cast(s, type<Rectangle>)
        t |> equal(r == null, true)
    }
    t |> run("cast to base type succeeds") @(t : T?) {
        var sq : Shape? = new Square(3.0)
        var asRect = dynamic_type_cast(sq, type<Rectangle>)
        t |> equal(asRect != null, true)
    }
}

[test]
def test_force_dynamic_type_cast(t : T?) {
    t |> run("force cast succeeds on correct type") @(t : T?) {
        var s : Shape? = new Circle()
        var c = force_dynamic_type_cast(s, type<Circle>)
        t |> equal(c != null, true)
    }
}

[test]
def test_is_as_syntax(t : T?) {
    t |> run("is syntax") @(t : T?) {
        var s : Shape? = new Circle()
        t |> equal(s is Circle, true)
        t |> equal(s is Rectangle, false)
    }
    t |> run("as syntax") @(t : T?) {
        var s : Shape? = new Circle()
        var c = s as Circle
        t |> equal(c != null, true)
    }
}

[test]
def test_polymorphic_area(t : T?) {
    t |> run("area via const method") @(t : T?) {
        var c : Shape? = new Circle()
        t |> equal(c->area() > 3.14, true)
        t |> equal(c->area() < 3.15, true)
    }
    t |> run("square area via inheritance") @(t : T?) {
        var sq : Shape? = new Square(4.0)
        t |> equal(sq->area(), 16.0)
    }
}

// -- pointer type coercion in data structures --

[test]
def test_array_of_base_pointers(t : T?) {
    t |> run("array push derived into base array") @(t : T?) {
        var arr : array<Shape?>
        arr |> push(new Circle())
        arr |> push(new Rectangle())
        arr |> push(new Square(2.0))
        t |> equal(length(arr), 3)
        t |> equal(arr[0]->area() > 3.14, true)
        t |> equal(arr[1]->area(), 1.0)
        t |> equal(arr[2]->area(), 4.0)
    }
}

[test]
def test_copy_derived_to_base(t : T?) {
    t |> run("copy assign derived to base pointer") @(t : T?) {
        var s : Shape?
        s = new Circle()
        t |> equal(s != null, true)
        t |> equal(s->area() > 3.14, true)
    }
    t |> run("move assign derived to base pointer") @(t : T?) {
        var c : Circle? = new Circle()
        var s : Shape?
        s <- c
        t |> equal(s != null, true)
        t |> equal(c == null, true)
    }
}

[test]
def test_struct_field_init(t : T?) {
    t |> run("struct field with derived pointer") @(t : T?) {
        var h = ShapeHolder(shape = new Circle())
        t |> equal(h.shape != null, true)
        t |> equal(h.shape->area() > 3.14, true)
    }
}

[test]
def test_tuple_with_base_pointers(t : T?) {
    t |> run("tuple of base pointers via assignment") @(t : T?) {
        var tup : tuple<Shape?; Shape?>
        tup._0 = new Circle()
        tup._1 = new Rectangle()
        t |> equal(tup._0 != null, true)
        t |> equal(tup._1 != null, true)
        t |> equal(tup._0->area() > 3.14, true)
        t |> equal(tup._1->area(), 1.0)
    }
}

variant ShapeOrName {
    shape : Shape?
    name : string
}

[test]
def test_variant_with_base_pointers(t : T?) {
    t |> run("variant holding base pointer") @(t : T?) {
        var v = ShapeOrName(shape = new Circle())
        t |> equal(v is shape, true)
        t |> equal((v as shape)->area() > 3.14, true)
    }
}

def take_shape(s : Shape?) : float {
    return s->area()
}

[test]
def test_pass_derived_to_function(t : T?) {
    t |> run("pass derived pointer as base arg") @(t : T?) {
        var c = new Circle()
        t |> equal(take_shape(c) > 3.14, true)
    }
}

[test]
def test_return_derived_as_base(t : T?) {
    t |> run("return derived pointer as base") @(t : T?) {
        var s : Shape? = new Circle()
        var a = s->area()
        t |> equal(a > 3.14, true)
    }
}
