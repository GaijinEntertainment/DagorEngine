options gen2
require daslib/linq
require dastest/testing_boost public
require math

require daslib/linq
require daslib/linq_boost
require daslib/linq_fold

require _common

[test]
def test_join(t : T?) {
    t |> run("basic join") @(t : T?) {
        var query = join(
            persons.to_sequence(),
            pets.to_sequence(),
            $(p : Person) => p,
            $(pet : Pet) => pet.owner,
            $(p : Person, pet : Pet) => (p.name, pet.name)
        )
        var expected = [
            ("Hedlund, Magnus", "Daisy"),
            ("Adams, Terry", "Barley"),
            ("Adams, Terry", "Boots"),
            ("Weiss, Charlotte", "Whiskers")
        ]
        for (c, e in query, expected) {
            t |> equal(c._0, e._0)
            t |> equal(c._1, e._1)
        }
    }
    t |> run("complex join") @(t : T?) {
        var qcomplex = join(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            $(c : ComplexType) => c.a[0],
            $(c : ComplexType) => c.a[0],
            $(c1 : ComplexType, c2 : ComplexType) => ComplexType(a = [c1.a[0], c2.a[1]])
        )
        var expected_complex = [
            (0, 0),
            (1, 10),
            (2, 20),
            (3, 30),
            (4, 40)
        ]
        for (c, e in qcomplex, expected_complex) {
            t |> equal(c.a[0], e._0)
            t |> equal(c.a[1], e._1)
        }
    }
    t |> run("array join") @(t : T?) {
        let arr1 = [1, 2, 3, 4, 5]
        let arr2 = [4, 5, 6, 7, 8]
        var query = join(
            arr1,
            arr2,
            $(a : int) => a,
            $(b : int) => b,
            $(a : int, b : int) => (a, b)
        )
        var expected = [
            (4, 4),
            (5, 5)
        ]
        for (c, e in query, expected) {
            t |> equal(c._0, e._0)
            t |> equal(c._1, e._1)
        }
    }
    t |> run("join to array") @(t : T?) {
        var arr1 = [1, 2, 3, 4, 5].to_sequence_move()
        var arr2 = [4, 5, 6, 7, 8].to_sequence_move()
        var query = join(
            arr1,
            arr2,
            $(a : int) => a,
            $(b : int) => b,
            $(a : int, b : int) => (a, b)
        ).to_array()
        var expected = [
            (4, 4),
            (5, 5)
        ]
        for (c, e in query, expected) {
            t |> equal(c._0, e._0)
            t |> equal(c._1, e._1)
        }
    }
    t |> run("join to sequence") @(t : T?) {
        let arr1 = [1, 2, 3, 4, 5]
        let arr2 = [4, 5, 6, 7, 8]
        var query = join(
            arr1,
            arr2,
            $(a : int) => a,
            $(b : int) => b,
            $(a : int, b : int) => (a, b)
        ).to_sequence()
        var expected = [
            (4, 4),
            (5, 5)
        ]
        for (c, e in query, expected) {
            t |> equal(c._0, e._0)
            t |> equal(c._1, e._1)
        }
    }
}

[test]
def test_left_join(t : T?) {
    t |> run("left join — numeric keys") @(t : T?) {
        let arr1 = [1, 2, 3, 4, 5]
        let arr2 = [2, 3, 3, 4]   // 1 & 5 have no match; 3 has two
        var query = left_join(
            arr1,
            arr2,
            $(a : int) => a,
            $(b : int) => b,
            $(a : int, b : Option<int>) => (a, b |> unwrap_or(-1))
        )
        var expected = [
            (1, -1),    // no match
            (2, 2),
            (3, 3),
            (3, 3),     // two matches
            (4, 4),
            (5, -1)     // no match
        ]
        for (c, e in query, expected) {
            t |> equal(c._0, e._0)
            t |> equal(c._1, e._1)
        }
    }
    t |> run("left join — empty right side") @(t : T?) {
        let left = [1, 2, 3]
        let right : array<int>
        var query = left_join(
            left,
            right,
            $(a : int) => a,
            $(b : int) => b,
            $(a : int, b : Option<int>) => (a, b |> unwrap_or(-1))
        )
        var expected = [(1, -1), (2, -1), (3, -1)]
        for (c, e in query, expected) {
            t |> equal(c._0, e._0)
            t |> equal(c._1, e._1)
        }
    }
    t |> run("left join — empty left side") @(t : T?) {
        let left : array<int>
        let right = [1, 2, 3]
        let query = left_join(
            left,
            right,
            $(a : int) => a,
            $(b : int) => b,
            $(a : int, b : Option<int>) => (a, b |> unwrap_or(-1))
        )
        t |> equal(query |> length, 0)
    }
    t |> run("left_join_to_array") @(t : T?) {
        let arr = left_join_to_array(
            [1, 2, 3].to_sequence_move(),
            [2, 3].to_sequence_move(),
            $(a : int) => a,
            $(b : int) => b,
            $(a : int, b : Option<int>) => (a, b |> is_some)
        )
        t |> equal(arr |> length, 3)
        t |> equal(arr[0]._0, 1)
        t |> success(!arr[0]._1)
        t |> equal(arr[1]._0, 2)
        t |> success(arr[1]._1)
        t |> equal(arr[2]._0, 3)
        t |> success(arr[2]._1)
    }
    // Cross-module user-struct cases — previously blocked by the splitTypeName
    // template-instance bug. Option<_common::Pet> reification now works because
    // find_unique_structure correctly locates the cached reified struct rather
    // than minting a duplicate per call site.
    t |> run("left join — user-struct keys (Person+Pet): unmatched left, multi-match") @(t : T?) {
        var query = left_join(
            persons,
            pets,
            $(p : Person) => p.name,
            $(pet : Pet) => pet.owner.name,
            $(p : Person, pets : Option<Pet>) => (p.name, pets |> is_some)
        )
        // magnus -> daisy (matched), terry -> barley+boots (matched), charlotte -> whiskers (matched)
        // All three have at least one match — every Option should be some.
        for (c in query) {
            t |> success(c._1)
        }
        t |> equal(query |> length, 4)  // magnus+daisy, terry+barley, terry+boots, charlotte+whiskers
    }
    t |> run("left join — user-struct keys with unmatched left row") @(t : T?) {
        // Add an orphan person with no pets. left_join must emit them once with none.
        let all_persons <- [magnus, terry, charlotte, Person(name = "Orphan", age = 99)]
        var query = left_join(
            all_persons,
            pets,
            $(p : Person) => p.name,
            $(pet : Pet) => pet.owner.name,
            $(p : Person, pet : Option<Pet>) => (p.name, pet |> unwrap_or(Pet(name = "<none>", owner = p, age = 0.0f)).name)
        )
        var found_orphan = false
        for (row in query) {
            if (row._0 == "Orphan") {
                t |> equal(row._1, "<none>")
                found_orphan = true
            }
        }
        t |> success(found_orphan)
    }
    t |> run("left_join_to_array — user-struct keys") @(t : T?) {
        var arr = left_join_to_array(
            persons.to_sequence(),
            pets.to_sequence(),
            $(p : Person) => p.name,
            $(pet : Pet) => pet.owner.name,
            $(p : Person, pet : Option<Pet>) => (p.name, pet |> is_some)
        )
        t |> success(arr |> length > 0)
        for (row in arr) {
            t |> success(row._1)  // every _common person has at least one pet
        }
    }
}

[test]
def test_groupjoin(t : T?) {
    t |> run("basic group join") @(t : T?) {
        var query = group_join(
            persons.to_sequence(),
            pets.to_sequence(),
            $(p : Person) => p.name,
            $(pet : Pet) => pet.owner.name,
            $(p : Person, var pets : array<Pet>) => (p.name, pets)
        )
        var expected = [
            ("Hedlund, Magnus", [daisy]),
            ("Adams, Terry", [barley, boots]),
            ("Weiss, Charlotte", [whiskers])
        ]
        for (c, e in query, expected) {
            t |> equal(c._0, e._0)
            for (p1, p2 in c._1, e._1) {
                t |> equal(p1.name, p2.name)
                t |> equal(p1.owner.name, p2.owner.name)
            }
        }
    }
    t |> run("complex group join") @(t : T?) {
        var qcomplex = group_join(
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            [iterator for(x in 0..5); ComplexType(a = [x, x * 10])],
            $(c : ComplexType) => c.a[0],
            $(c : ComplexType) => c.a[1] / 10,
            $(c : ComplexType, var pets : array<ComplexType>) => (c.a[0], pets)
        )
        var expected_complex = [

            (0, [ComplexType(a = [0, 0])]),
            (1, [ComplexType(a = [1, 10])]),
            (2, [ComplexType(a = [2, 20])]),
            (3, [ComplexType(a = [3, 30])]),
            (4, [ComplexType(a = [4, 40])])
        ]
        for (c, e in qcomplex, expected_complex) {
            t |> equal(c._0, e._0)
            for (p1, p2 in c._1, e._1) {
                t |> equal(p1.a[0], p2.a[0])
                t |> equal(p1.a[1], p2.a[1])
            }
        }
    }
    t |> run("group join from array") @(t : T?) {
        var query = group_join(
            persons,
            pets,
            $(p : Person) => p.name,
            $(pet : Pet) => pet.owner.name,
            $(p : Person, var pets : array<Pet>) => (p.name, pets)
        )
        var expected = [
            ("Hedlund, Magnus", [daisy]),
            ("Adams, Terry", [barley, boots]),
            ("Weiss, Charlotte", [whiskers])
        ]
        for (c, e in query, expected) {
            t |> equal(c._0, e._0)
            for (p1, p2 in c._1, e._1) {
                t |> equal(p1.name, p2.name)
                t |> equal(p1.owner.name, p2.owner.name)
            }
        }
    }
    t |> run("group join to array") @(t : T?) {
        var query = group_join_to_array(
            persons.to_sequence(),
            pets.to_sequence(),
            $(p : Person) => p.name,
            $(pet : Pet) => pet.owner.name,
            $(p : Person, var pets : array<Pet>) => (p.name, pets)
        )
        var expected = [
            ("Hedlund, Magnus", [daisy]),
            ("Adams, Terry", [barley, boots]),
            ("Weiss, Charlotte", [whiskers])
        ]
        for (c, e in query, expected) {
            t |> equal(c._0, e._0)
            for (p1, p2 in c._1, e._1) {
                t |> equal(p1.name, p2.name)
                t |> equal(p1.owner.name, p2.owner.name)
            }
        }
    }
    t |> run("group join to sequence") @(t : T?) {
        var query = group_join_to_array(
            persons.to_sequence(),
            pets.to_sequence(),
            $(p : Person) => p.name,
            $(pet : Pet) => pet.owner.name,
            $(p : Person, var pets : array<Pet>) => (p.name, pets)
        ).to_sequence()
        var expected = [
            ("Hedlund, Magnus", [daisy]),
            ("Adams, Terry", [barley, boots]),
            ("Weiss, Charlotte", [whiskers])
        ]
        for (c, e in query, expected) {
            t |> equal(c._0, e._0)
            for (p1, p2 in c._1, e._1) {
                t |> equal(p1.name, p2.name)
                t |> equal(p1.owner.name, p2.owner.name)
            }
        }
    }
    t |> run("group join — unmatched left row surfaces an empty group (C# GroupJoin / outer)") @(t : T?) {
        // Orphan has no pets. Outer GroupJoin keeps every left row — Orphan must surface with an empty group.
        let all_persons <- [magnus, terry, charlotte, Person(name = "Orphan", age = 99)]
        var query = group_join(
            all_persons,
            pets,
            $(p : Person) => p.name,
            $(pet : Pet) => pet.owner.name,
            $(p : Person, var pets : array<Pet>) => (p.name, length(pets))
        )
        t |> equal(query |> length, 4, "every left row surfaces, including the orphan")
        var found_orphan = false
        for (row in query) {
            if (row._0 == "Hedlund, Magnus") { t |> equal(row._1, 1) }
            if (row._0 == "Adams, Terry")    { t |> equal(row._1, 2) }
            if (row._0 == "Weiss, Charlotte"){ t |> equal(row._1, 1) }
            if (row._0 == "Orphan") {
                t |> equal(row._1, 0, "unmatched left → empty group, count 0")
                found_orphan = true
            }
        }
        t |> success(found_orphan)
    }
}

// group_by output order follows first-seen-key (insertion) order — an implementation
// detail that shifted when small tables became packed. The pet-age groups are unordered,
// so compare them sorted by key.
def check_pet_age_groups(t : T?; var got : array<tuple<float; int; float; float>>) {
    sort(got) $(a, b) => a._0 < b._0
    t |> equal(length(got), 3)
    if (length(got) == 3) {
        t |> equal(got[0]._0, 1.); t |> equal(got[0]._1, 1); t |> equal(got[0]._2, 1.5); t |> equal(got[0]._3, 1.5)
        t |> equal(got[1]._0, 4.); t |> equal(got[1]._1, 2); t |> equal(got[1]._2, 4.3); t |> equal(got[1]._3, 4.9)
        t |> equal(got[2]._0, 8.); t |> equal(got[2]._1, 1); t |> equal(got[2]._2, 8.3); t |> equal(got[2]._3, 8.3)
    }
}

[test]
def test_groupby(t : T?) {
    t |> run("basic group by") @(t : T?) {
        var query = group_by(
            pets.to_sequence(),
            $(pet : Pet) => floor(pet.age),
            $(pet : Pet) => pet.age,
            $(baseAge : float, var ages : iterator<float>) {
                var inscope aa = ages.to_array()
                return (baseAge, aa.length(), aa.to_sequence().min(), aa.to_sequence().max())
            }
        )
        var got : array<tuple<float; int; float; float>>
        for (c in query) {
            got |> push(c)
        }
        check_pet_age_groups(t, got)
        delete got
    }
    t |> run("complex group by") @(t : T?) {
        var qcomplex = group_by(
            [iterator for(x in 0..5); ComplexType(a = [x % 2, x * 10])],
            $(c : ComplexType) => c.a[0],
            $(c : ComplexType) => c.a[1],
            $(base : int, var vals : iterator<int>) {
                var inscope aa = vals.to_array()
                return (base, aa.length(), aa.to_sequence().min(), aa.to_sequence().max())
            }
        )
        var expected_complex = [
            (0, 3, 0, 40),      // even
            (1, 2, 10, 30)      // odd
        ]
        for (c, e in qcomplex, expected_complex) {
            t |> equal(c._0, e._0)
            t |> equal(c._1, e._1)
            t |> equal(c._2, e._2)
            t |> equal(c._3, e._3)
        }
    }
    t |> run("group by from array") @(t : T?) {
        var query = group_by(
            pets,
            $(pet : Pet) => floor(pet.age),
            $(pet : Pet) => pet.age,
            $(baseAge : float, var ages : iterator<float>) {
                var inscope aa = ages.to_array()
                return (baseAge, aa.length(), aa.to_sequence().min(), aa.to_sequence().max())
            }
        )
        var got : array<tuple<float; int; float; float>>
        for (c in query) {
            got |> push(c)
        }
        check_pet_age_groups(t, got)
        delete got
    }
    t |> run("group by to array") @(t : T?) {
        var query = group_by_to_array(
            pets.to_sequence(),
            $(pet : Pet) => floor(pet.age),
            $(pet : Pet) => pet.age,
            $(baseAge : float, var ages : iterator<float>) {
                var inscope aa = ages.to_array()
                return (baseAge, aa.length(), aa.to_sequence().min(), aa.to_sequence().max())
            }
        )
        var got : array<tuple<float; int; float; float>>
        for (c in query) {
            got |> push(c)
        }
        check_pet_age_groups(t, got)
        delete got
    }
    t |> run("group by to sequence") @(t : T?) {
        var query = group_by_to_array(
            pets.to_sequence(),
            $(pet : Pet) => floor(pet.age),
            $(pet : Pet) => pet.age,
            $(baseAge : float, var ages : iterator<float>) {
                var inscope aa = ages.to_array()
                return (baseAge, aa.length(), aa.to_sequence().min(), aa.to_sequence().max())
            }
        ).to_sequence()
        var got : array<tuple<float; int; float; float>>
        for (c in query) {
            got |> push(c)
        }
        check_pet_age_groups(t, got)
        delete got
    }
}

[test]
def test_join_macro(t : T?) {
    t |> run("_join — primitive keys on arrays") @(t : T?) {
        let arr1 = [1, 2, 3, 4, 5]
        let arr2 = [4, 5, 6, 7, 8]
        var query = _join(
            arr1,
            arr2,
            $(a : int, b : int) => a == b,
            $(a : int, b : int) => (a, b)
        )
        var expected = [(4, 4), (5, 5)]
        for (c, e in query, expected) {
            t |> equal(c._0, e._0)
            t |> equal(c._1, e._1)
        }
    }
    t |> run("_join — field keys on user structs") @(t : T?) {
        var query = _join(
            persons,
            pets,
            $(p : Person, pet : Pet) => p.name == pet.owner.name,
            $(p : Person, pet : Pet) => (p.name, pet.name)
        )
        var expected = [
            ("Hedlund, Magnus", "Daisy"),
            ("Adams, Terry", "Barley"),
            ("Adams, Terry", "Boots"),
            ("Weiss, Charlotte", "Whiskers")
        ]
        for (c, e in query, expected) {
            t |> equal(c._0, e._0)
            t |> equal(c._1, e._1)
        }
    }
    t |> run("_join — iterator inputs") @(t : T?) {
        var query = _join(
            persons.to_sequence(),
            pets.to_sequence(),
            $(p : Person, pet : Pet) => p.name == pet.owner.name,
            $(p : Person, pet : Pet) => (p.name, pet.name)
        )
        var count = 0
        for (row in query) {
            t |> success(!empty(row._0))
            t |> success(!empty(row._1))
            count++
        }
        t |> equal(count, 4)
    }
    t |> run("_join — no matches yields empty result") @(t : T?) {
        let arr1 = [1, 2, 3]
        let arr2 = [10, 20, 30]
        let query = _join(
            arr1,
            arr2,
            $(a : int, b : int) => a == b,
            $(a : int, b : int) => (a, b)
        )
        t |> equal(query |> length, 0)
    }
    t |> run("_join — computed key expression on one side") @(t : T?) {
        let arr1 = [1, 2, 3, 4]
        let arr2 = [2, 4, 6, 8]
        // Match arr1 * 2 against arr2 — exercises non-trivial key expression.
        var query = _join(
            arr1,
            arr2,
            $(a : int, b : int) => a * 2 == b,
            $(a : int, b : int) => (a, b)
        )
        var expected = [(1, 2), (2, 4), (3, 6), (4, 8)]
        for (c, e in query, expected) {
            t |> equal(c._0, e._0)
            t |> equal(c._1, e._1)
        }
    }
}

[test]
def test_left_join_macro(t : T?) {
    t |> run("_left_join — primitive keys with unmatched rows") @(t : T?) {
        let arr1 = [1, 2, 3, 4, 5]
        let arr2 = [2, 3, 3, 4]   // 1 & 5 unmatched; 3 has two matches
        var query = _left_join(
            arr1,
            arr2,
            $(a : int, b : int) => a == b,
            $(a : int, b : Option<int>) => (a, b |> unwrap_or(-1))
        )
        var expected = [
            (1, -1),
            (2, 2),
            (3, 3),
            (3, 3),
            (4, 4),
            (5, -1)
        ]
        for (c, e in query, expected) {
            t |> equal(c._0, e._0)
            t |> equal(c._1, e._1)
        }
    }
    t |> run("_left_join — empty right side") @(t : T?) {
        let left = [1, 2, 3]
        let right : array<int>
        var query = _left_join(
            left,
            right,
            $(a : int, b : int) => a == b,
            $(a : int, b : Option<int>) => (a, b |> is_some)
        )
        var rows = 0
        for (row in query) {
            t |> success(!row._1)   // every right is none
            rows++
        }
        t |> equal(rows, 3)
    }
    t |> run("_left_join — user-struct keys, all matched") @(t : T?) {
        var query = _left_join(
            persons,
            pets,
            $(p : Person, pet : Pet) => p.name == pet.owner.name,
            $(p : Person, pet : Option<Pet>) => (p.name, pet |> is_some)
        )
        // _common persons all have at least one pet → every row is some.
        var count = 0
        for (row in query) {
            t |> success(row._1)
            count++
        }
        t |> equal(count, 4)   // magnus+daisy, terry+barley, terry+boots, charlotte+whiskers
    }
    t |> run("_left_join — user-struct keys with an orphan left row") @(t : T?) {
        let all_persons <- [magnus, terry, charlotte, Person(name = "Orphan", age = 99)]
        var query = _left_join(
            all_persons,
            pets,
            $(p : Person, pet : Pet) => p.name == pet.owner.name,
            $(p : Person, pet : Option<Pet>) => (p.name, pet |> unwrap_or(Pet(name = "<none>", owner = p, age = 0.0f)).name)
        )
        var found_orphan = false
        for (row in query) {
            if (row._0 == "Orphan") {
                t |> equal(row._1, "<none>")
                found_orphan = true
            }
        }
        t |> success(found_orphan)
    }
}

// ── array-side _join splice (chunk N+2) — exercises emit_array_join via _fold(...) wraps ───────

struct AJCar {
    id : int
    dealer_id : int
    name : string
    price : int
}

struct AJDealer {
    id : int
    name : string
    brand : string
}

def aj_cars() : array<AJCar> {
    return <- [
        AJCar(id = 1, dealer_id = 10, name = "C1", price = 1000),
        AJCar(id = 2, dealer_id = 10, name = "C2", price = 2000),
        AJCar(id = 3, dealer_id = 20, name = "C3", price = 3000),
        AJCar(id = 4, dealer_id = 30, name = "C4", price = 4000),
        AJCar(id = 5, dealer_id = 99, name = "C5_orphan", price = 5000)
    ]
}

def aj_dealers() : array<AJDealer> {
    return <- [
        AJDealer(id = 10, name = "D10", brand = "Acme"),
        AJDealer(id = 20, name = "D20", brand = "Beta"),
        AJDealer(id = 30, name = "D30", brand = "Acme")
    ]
}

struct AJKey {
    a : int
    b : int
}

[test]
def test_array_join_splice(t : T?) {
    t |> run("_join + count — int key, 4 matches (orphan dropped)") @(t : T?) {
        let cars <- aj_cars()
        let dealers <- aj_dealers()
        let n = _fold(cars |> _join(dealers,
                                    $(c : AJCar, d : AJDealer) => c.dealer_id == d.id,
                                    $(c : AJCar, d : AJDealer) => (c.name, d.brand))
                           |> count())
        t |> equal(n, 4)
    }
    t |> run("_join + count(where) — runs predicate on each pair") @(t : T?) {
        let cars <- aj_cars()
        let dealers <- aj_dealers()
        let n = _fold(cars |> _join(dealers,
                                    $(c : AJCar, d : AJDealer) => c.dealer_id == d.id,
                                    $(c : AJCar, d : AJDealer) => (CarName = c.name, Brand = d.brand))
                           |> _where(_.Brand == "Acme")
                           |> count())
        t |> equal(n, 3)
    }
    t |> run("_join + _select(_.X) — bare-select on array chain (no to_array)") @(t : T?) {
        let cars <- aj_cars()
        let dealers <- aj_dealers()
        let rows <- _fold(cars |> _join(dealers,
                                        $(c : AJCar, d : AJDealer) => c.dealer_id == d.id,
                                        $(c : AJCar, d : AJDealer) => (CarName = c.name, Brand = d.brand))
                               |> _select(_.CarName))
        t |> equal(length(rows), 4)
        var has_c1 = false
        for (r in rows) {
            if (r == "C1") {
                has_c1 = true
            }
        }
        t |> success(has_c1)
    }
    t |> run("_join + _select(_.X) via iterator chain + to_array — projects single field") @(t : T?) {
        let cars <- aj_cars()
        let dealers <- aj_dealers()
        let rows <- _fold(each(cars) |> _join(each(dealers),
                                              $(c : AJCar, d : AJDealer) => c.dealer_id == d.id,
                                              $(c : AJCar, d : AJDealer) => (CarName = c.name, Brand = d.brand))
                                     |> _select(_.CarName)
                                     |> to_array())
        t |> equal(length(rows), 4)
    }

    t |> run("_join + _where + to_array — filters pairs") @(t : T?) {
        let cars <- aj_cars()
        let dealers <- aj_dealers()
        let rows <- _fold(cars |> _join(dealers,
                                        $(c : AJCar, d : AJDealer) => c.dealer_id == d.id,
                                        $(c : AJCar, d : AJDealer) => (CarName = c.name, Brand = d.brand))
                               |> _where(_.Brand == "Beta"))
        t |> equal(length(rows), 1)
        t |> equal(rows[0].CarName, "C3")
        t |> equal(rows[0].Brand, "Beta")
    }
    t |> run("_join + named-tuple projection — round-trips both sides") @(t : T?) {
        let cars <- aj_cars()
        let dealers <- aj_dealers()
        let rows <- _fold(cars |> _join(dealers,
                                        $(c : AJCar, d : AJDealer) => c.dealer_id == d.id,
                                        $(c : AJCar, d : AJDealer) => (Price = c.price, Brand = d.brand)))
        t |> equal(length(rows), 4)
        var total = 0
        for (r in rows) {
            total += r.Price
        }
        t |> equal(total, 1000 + 2000 + 3000 + 4000)
    }
    t |> run("_join + count — string key") @(t : T?) {
        let a <- [
            (k = "x", v = 1),
            (k = "y", v = 2),
            (k = "z", v = 3)
        ]
        let b <- [
            (k = "x", w = 10),
            (k = "y", w = 20),
            (k = "y", w = 21)
        ]
        let n = _fold(a |> _join(b,
                                 $(l, r) => l.k == r.k,
                                 $(l, r) => (l.v, r.w))
                        |> count())
        t |> equal(n, 3)
    }
    t |> run("_join — empty srcb yields 0 / empty array") @(t : T?) {
        let cars <- aj_cars()
        var empty_dealers : array<AJDealer>
        let n = _fold(cars |> _join(empty_dealers,
                                    $(c : AJCar, d : AJDealer) => c.dealer_id == d.id,
                                    $(c : AJCar, d : AJDealer) => (c.name, d.brand))
                           |> count())
        t |> equal(n, 0)
        let rows <- _fold(cars |> _join(empty_dealers,
                                        $(c : AJCar, d : AJDealer) => c.dealer_id == d.id,
                                        $(c : AJCar, d : AJDealer) => (CarName = c.name, Brand = d.brand)))
        t |> equal(length(rows), 0)
    }
    t |> run("_join + count — many-to-one (multi-bucket fast path)") @(t : T?) {
        let cars <- aj_cars()
        let dealers <- aj_dealers()
        // Dealer 10 has 2 cars; dealer 20 has 1; dealer 30 has 1; orphan car has 0. Bucket count: 2+1+1+0 = 4. Fast path = no per-pair body, just `cnt += length(arr)`.
        let n = _fold(cars |> _join(dealers,
                                    $(c : AJCar, d : AJDealer) => c.dealer_id == d.id,
                                    $(c : AJCar, d : AJDealer) => (c.name, d.brand))
                           |> count())
        t |> equal(n, 4)
    }
    // Phase 2 — cross-arm `_join |> _group_by` (ArrayJoin SourceAdapter routed through plan_group_by_core)
    t |> run("_join + _group_by + count — bucket counts per Brand") @(t : T?) {
        let cars <- aj_cars()
        let dealers <- aj_dealers()
        // Dealer brands: 10=Acme, 20=Beta, 30=Acme. Cars: 2+1+1+orphan into 3 dealers → Acme bucket = 3 (2 via dealer 10 + 1 via dealer 30), Beta bucket = 1 (via dealer 20). Group count = 2.
        let groups <- _fold(cars |> _join(dealers,
                                          $(c : AJCar, d : AJDealer) => c.dealer_id == d.id,
                                          $(c : AJCar, d : AJDealer) => (Brand = d.brand, CarName = c.name))
                                 |> _group_by(_.Brand)
                                 |> _select((Brand = _._0, N = _._1 |> count())))
        t |> equal(length(groups), 2)
        var acme_n = 0
        var beta_n = 0
        for (g in groups) {
            if (g.Brand == "Acme") {
                acme_n = g.N
            }
            if (g.Brand == "Beta") {
                beta_n = g.N
            }
        }
        t |> equal(acme_n, 3)
        t |> equal(beta_n, 1)
    }
    t |> run("_join + _group_by + sum — per-Brand totals via select+sum aggregate") @(t : T?) {
        let cars <- aj_cars()
        let dealers <- aj_dealers()
        let groups <- _fold(cars |> _join(dealers,
                                          $(c : AJCar, d : AJDealer) => c.dealer_id == d.id,
                                          $(c : AJCar, d : AJDealer) => (Brand = d.brand, Price = c.price))
                                 |> _group_by(_.Brand)
                                 |> _select((Brand = _._0, Total = _._1 |> _select(_.Price) |> sum())))
        t |> equal(length(groups), 2)
        var acme_total = 0
        var beta_total = 0
        for (g in groups) {
            if (g.Brand == "Acme") {
                acme_total = g.Total
            }
            if (g.Brand == "Beta") {
                beta_total = g.Total
            }
        }
        // Acme rows: cars 1+2 (dealer 10) + car 4 (dealer 30) → 1000+2000+4000 = 7000
        // Beta rows: car 3 (dealer 20) → 3000
        t |> equal(acme_total, 7000)
        t |> equal(beta_total, 3000)
    }
    t |> run("_join + _group_by + min/max — per-Brand min/max via select+min/max") @(t : T?) {
        let cars <- aj_cars()
        let dealers <- aj_dealers()
        let groups <- _fold(cars |> _join(dealers,
                                          $(c : AJCar, d : AJDealer) => c.dealer_id == d.id,
                                          $(c : AJCar, d : AJDealer) => (Brand = d.brand, Price = c.price))
                                 |> _group_by(_.Brand)
                                 |> _select((Brand = _._0,
                                             MinP = _._1 |> _select(_.Price) |> min(),
                                             MaxP = _._1 |> _select(_.Price) |> max())))
        t |> equal(length(groups), 2)
        for (g in groups) {
            if (g.Brand == "Acme") {
                t |> equal(g.MinP, 1000)
                t |> equal(g.MaxP, 4000)
            }
            if (g.Brand == "Beta") {
                t |> equal(g.MinP, 3000)
                t |> equal(g.MaxP, 3000)
            }
        }
    }
    t |> run("_join + _group_by — empty srcb yields empty groups") @(t : T?) {
        let cars <- aj_cars()
        var empty_dealers : array<AJDealer>
        let groups <- _fold(cars |> _join(empty_dealers,
                                          $(c : AJCar, d : AJDealer) => c.dealer_id == d.id,
                                          $(c : AJCar, d : AJDealer) => (Brand = d.brand, CarName = c.name))
                                 |> _group_by(_.Brand)
                                 |> _select((Brand = _._0, N = _._1 |> count())))
        t |> equal(length(groups), 0)
    }
    t |> run("_join — non-primitive key cascades to tier-2 (still produces correct result)") @(t : T?) {
        // is_primitive_join_key_type rejects struct keys → splice returns null → fold_linq_default takes over → join_impl_const path with _::unique_key wrapping handles the struct key.
        let left <- [
            (k = AJKey(a = 1, b = 1), v = 10),
            (k = AJKey(a = 2, b = 2), v = 20),
            (k = AJKey(a = 3, b = 3), v = 30)
        ]
        let right <- [
            (k = AJKey(a = 1, b = 1), w = 100),
            (k = AJKey(a = 2, b = 2), w = 200),
            (k = AJKey(a = 3, b = 3), w = 300)
        ]
        let n = _fold(left |> _join(right,
                                    $(l, r) => l.k == r.k,
                                    $(l, r) => (l.v, r.w))
                           |> count())
        t |> equal(n, 3)
    }
}

// Regression — `_join` followed by a materializing op (`_order_by` / `_group_by_lazy`), and a
// scalar `_join` result, over `each(...)` sources. These fall to the tier-2 `fold_linq_default`
// fallback: the bare `each(srcB)` argument used to trip error[31013] ("each unsafe outside a
// for-loop"), and a scalar join result used to build a const buffer tripping error[30913].
[test]
def test_join_fused_regressions(t : T?) {
    t |> run("each |> _join |> _order_by — was error[31013]") @(t : T?) {
        let cars <- aj_cars()
        let dealers <- aj_dealers()
        let rows <- _fold(each(cars) |> _join(each(dealers),
                                              $(c : AJCar, d : AJDealer) => c.dealer_id == d.id,
                                              $(c : AJCar, d : AJDealer) => (Name = c.name, Price = c.price))
                                     |> _order_by($(r) => r.Price)
                                     |> to_array())
        t |> equal(length(rows), 4, "4 matched pairs (orphan dropped)")
        t |> equal(rows[0].Name, "C1")   // 1000
        t |> equal(rows[3].Name, "C4")   // 4000
    }
    t |> run("each |> _join |> _order_by_descending |> _select") @(t : T?) {
        let cars <- aj_cars()
        let dealers <- aj_dealers()
        let names <- _fold(each(cars) |> _join(each(dealers),
                                               $(c : AJCar, d : AJDealer) => c.dealer_id == d.id,
                                               $(c : AJCar, d : AJDealer) => (Name = c.name, Price = c.price))
                                      |> _order_by_descending($(r) => r.Price)
                                      |> _select($(r) => r.Name)
                                      |> to_array())
        t |> equal(length(names), 4)
        t |> equal(names[0], "C4")
        t |> equal(names[3], "C1")
    }
    t |> run("each |> _join |> _group_by_lazy — IGrouping after join, was error[31013]") @(t : T?) {
        let cars <- aj_cars()
        let dealers <- aj_dealers()
        // brands: D10=Acme(C1,C2), D20=Beta(C3), D30=Acme(C4) → Acme:3, Beta:1
        let groups <- _fold(each(cars) |> _join(each(dealers),
                                                $(c : AJCar, d : AJDealer) => c.dealer_id == d.id,
                                                $(c : AJCar, d : AJDealer) => (Brand = d.brand, Name = c.name))
                                       |> _group_by_lazy($(r) => r.Brand)
                                       |> to_array())
        t |> equal(length(groups), 2, "Acme + Beta buckets")
        for (g in groups) {
            if (g._0 == "Acme") { t |> equal(length(g._1), 3, "Acme: C1,C2,C4") }
            if (g._0 == "Beta") { t |> equal(length(g._1), 1, "Beta: C3") }
        }
    }
    t |> run("each |> _join scalar result |> to_array — was error[30913] const push") @(t : T?) {
        let cars <- aj_cars()
        let dealers <- aj_dealers()
        let names <- _fold(each(cars) |> _join(each(dealers),
                                               $(c : AJCar, d : AJDealer) => c.dealer_id == d.id,
                                               $(c : AJCar, d : AJDealer) => c.name)
                                      |> to_array())
        t |> equal(length(names), 4, "scalar (string) join result materializes")
        var hasC1 = false
        for (nm in names) {
            if (nm == "C1") { hasC1 = true }
        }
        t |> success(hasC1)
    }
    t |> run("each |> _join scalar result |> _order_by — scalar + materializing op") @(t : T?) {
        let cars <- aj_cars()
        let dealers <- aj_dealers()
        let names <- _fold(each(cars) |> _join(each(dealers),
                                               $(c : AJCar, d : AJDealer) => c.dealer_id == d.id,
                                               $(c : AJCar, d : AJDealer) => c.name)
                                      |> _order_by($(n) => n)
                                      |> to_array())
        t |> equal(length(names), 4)
        t |> equal(names[0], "C1")
    }
}
