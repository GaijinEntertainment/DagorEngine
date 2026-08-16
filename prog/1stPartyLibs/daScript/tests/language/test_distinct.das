options gen2
require dastest/testing_boost public
require _distinct_fixture

typedef distinct Id = int
typedef distinct OtherId = int
typedef distinct Name = string
typedef distinct Handle = void?
typedef distinct Pos = float3
typedef distinct Meters = float

def operator +(a, b : Meters) : Meters => Meters(*a + *b)

def describe(x : int) : string => "int {x}"

def describe(x : Id) : string => "id {*x}"

def generic_keep(x : auto(TT)) : TT => x

struct Player {
    id : Id
    hp : int
}

variant IdOrName {
    ident : Id
    title : Name
}

[test]
def test_distinct_construct_and_deref(t : T?) {
    t |> run("construct, read, write") @(t : T?) {
        var a = Id(5)
        t |> equal(*a, 5)
        *a = 7
        t |> equal(*a, 7)
    }
    t |> run("let handle derefs const") @(t : T?) {
        let b = Id(3)
        t |> equal(*b, 3)
    }
    t |> run("default is zero") @(t : T?) {
        let z : Id
        t |> success(z == Id(0))
        t |> success(default<Id> == Id(0))
    }
    t |> run("default over string, pointer, vector underlyings") @(t : T?) {
        t |> success(default<Name> == Name(""))
        t |> success(default<Handle> == Handle(null))
        t |> equal((*default<Pos>).y, 0.0)
    }
}

[test]
def test_distinct_equality(t : T?) {
    t |> run("== and != borrow") @(t : T?) {
        let a = Id(7)
        let b = Id(7)
        let c = Id(8)
        t |> success(a == b)
        t |> success(a != c)
    }
}

[test]
def test_distinct_underlyings(t : T?) {
    t |> run("string underlying") @(t : T?) {
        let n = Name("boris")
        t |> equal(*n, "boris")
        t |> success(n == Name("boris"))
    }
    t |> run("pointer underlying") @(t : T?) {
        let h = Handle(null)
        t |> success(h == Handle(null))
    }
    t |> run("vector underlying") @(t : T?) {
        let p = Pos(float3(1.0, 2.0, 3.0))
        t |> equal((*p).y, 2.0)
    }
}

[test]
def test_distinct_operators_and_overloads(t : T?) {
    t |> run("user-defined operator +") @(t : T?) {
        let m = Meters(2.0) + Meters(3.0)
        t |> equal(*m, 5.0)
    }
    t |> run("overloads on distinct vs underlying coexist") @(t : T?) {
        t |> equal(describe(5), "int 5")
        t |> equal(describe(Id(5)), "id 5")
    }
    t |> run("two distincts over the same underlying are unrelated") @(t : T?) {
        // Id and OtherId are both int underneath; describe(Id) must not accept OtherId
        t |> equal(typeinfo typename(default<Id>), "Id")
        t |> equal(typeinfo typename(default<OtherId>), "OtherId")
    }
}

[test]
def test_distinct_containers(t : T?) {
    t |> run("array of distinct") @(t : T?) {
        var arr : array<Id>
        arr |> push(Id(1))
        arr |> push(Id(2))
        t |> equal(*arr[1], 2)
        t |> success(arr[0] == Id(1))
    }
    t |> run("struct field") @(t : T?) {
        var p = Player(id = Id(42), hp = 100)
        t |> equal(*p.id, 42)
        p.id = Id(43)
        t |> success(p.id == Id(43))
    }
    t |> run("variant field") @(t : T?) {
        var v : IdOrName
        unsafe {
            v.ident = Id(9)
        }
        t |> success(v is ident)
        t |> success((v as ident) == Id(9))
    }
    t |> run("tuple field") @(t : T?) {
        let tp = (Id(1), Name("x"))
        t |> success(tp._0 == Id(1))
    }
}

[test]
def test_distinct_generics(t : T?) {
    t |> run("auto(T) binds the distinct type") @(t : T?) {
        let g = generic_keep(Meters(4.0))
        t |> equal(typeinfo typename(g), "Meters const")
        t |> equal(*g, 4.0)
    }
    t |> run("typeinfo is_distinct") @(t : T?) {
        t |> success(typeinfo is_distinct(type<Id>))
        t |> success(!typeinfo is_distinct(type<int>))
    }
    t |> run("typedecl of deref reaches the underlying") @(t : T?) {
        let m = Meters(2.5)
        let under : typedecl(*m) = 1.5
        t |> equal(under, 1.5)
    }
}

[test]
def test_distinct_cross_module(t : T?) {
    t |> run("public distinct from another module") @(t : T?) {
        let id = make_pub_id(11)
        t |> equal(pub_id_value(id), 11)
        t |> success(id == PubId(11))
        t |> success(id != PubId(12))
    }
}
