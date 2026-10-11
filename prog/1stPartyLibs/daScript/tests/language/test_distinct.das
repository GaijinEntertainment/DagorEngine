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
        let arr <- [Id(1), Id(2)]
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

typedef distinct Conn = int

var g_conn_eq_calls = 0

def operator ==(a, b : Conn) : bool {
    g_conn_eq_calls++
    return *a == *b
}

[test]
def test_distinct_user_operator_beats_borrow(t : T?) {
    t |> run("a user-defined == on the distinct type is the one called") @(t : T?) {
        g_conn_eq_calls = 0
        let a = Conn(1)
        let b = Conn(1)
        let c = Conn(2)
        t |> success(a == b)
        t |> success(!(a == c))
        t |> equal(g_conn_eq_calls, 2)
    }
    t |> run("!= still borrows when only == is defined") @(t : T?) {
        g_conn_eq_calls = 0
        let a = Conn(1)
        let c = Conn(2)
        t |> success(a != c)
        t |> success(!(a != Conn(1)))
        t |> equal(g_conn_eq_calls, 0)
    }
}

typedef distinct Cid = int

var g_cid_ops : string

def operator +(a, b : Cid) : Cid {
    g_cid_ops += "+,"
    return Cid(*a + *b)
}
def operator -(a, b : Cid) : Cid {
    g_cid_ops += "-,"
    return Cid(*a - *b)
}
def operator *(a, b : Cid) : Cid {
    g_cid_ops += "*,"
    return Cid(*a * *b)
}
def operator /(a, b : Cid) : Cid {
    g_cid_ops += "/,"
    return Cid(*a / *b)
}
def operator %(a, b : Cid) : Cid {
    g_cid_ops += "%,"
    return Cid(*a % *b)
}
def operator -(a : Cid) : Cid {
    g_cid_ops += "neg,"
    return Cid(-*a)
}
def operator !(a : Cid) : bool {
    g_cid_ops += "!,"
    return *a == 0
}
def operator <(a, b : Cid) : bool {
    g_cid_ops += "<,"
    return *a < *b
}
def operator <=(a, b : Cid) : bool {
    g_cid_ops += "<=,"
    return *a <= *b
}
def operator >(a, b : Cid) : bool {
    g_cid_ops += ">,"
    return *a > *b
}
def operator >=(a, b : Cid) : bool {
    g_cid_ops += ">=,"
    return *a >= *b
}
def operator +=(var a : Cid&; b : Cid) {
    g_cid_ops += "+=,"
    a = Cid(*a + *b)
}
def operator -=(var a : Cid&; b : Cid) {
    g_cid_ops += "-=,"
    a = Cid(*a - *b)
}
def operator [] (a : Cid; i : int) : int {
    g_cid_ops += "[],"
    return *a * i
}
def operator ++(var a : Cid&) {
    g_cid_ops += "p++,"
    a = Cid(*a + 1)
}
def ++operator(var a : Cid &) {
    g_cid_ops += "++p,"
    a = Cid(*a + 1)
}
def operator --(var a : Cid&) {
    g_cid_ops += "p--,"
    a = Cid(*a - 1)
}
def --operator(var a : Cid &) {
    g_cid_ops += "--p,"
    a = Cid(*a - 1)
}

struct Counter {
    ops : string
}

def operator ++(var c : Counter) {
    c.ops += "p++,"
}
def ++operator(var c : Counter) {
    c.ops += "++p,"
}

[test]
def test_distinct_user_operators(t : T?) {
    t |> run("arithmetic, ordering, unary, index") @(t : T?) {
        g_cid_ops = ""
        let a = Cid(6)
        let b = Cid(3)
        t |> equal(*(a + b), 9)
        t |> equal(*(a - b), 3)
        t |> equal(*(a * b), 18)
        t |> equal(*(a / b), 2)
        t |> equal(*(a % b), 0)
        t |> equal(*(-a), -6)
        t |> success(!Cid(0))
        t |> success(b < a)
        t |> success(b <= a)
        t |> success(a > b)
        t |> success(a >= b)
        t |> equal(a[2], 12)
        t |> equal(g_cid_ops, "+,-,*,/,%,neg,!,<,<=,>,>=,[],")
    }
    t |> run("compound assignment and both increment forms take the reference") @(t : T?) {
        g_cid_ops = ""
        var a = Cid(6)
        a += Cid(4)
        a -= Cid(1)
        a++
        ++a
        a--
        --a
        t |> equal(*a, 9)
        t |> equal(g_cid_ops, "+=,-=,p++,++p,p--,--p,")
    }
    t |> run("postfix and prefix increments are separate operators on a struct too") @(t : T?) {
        var c : Counter
        c++
        ++c
        t |> equal(c.ops, "p++,++p,")
    }
}
