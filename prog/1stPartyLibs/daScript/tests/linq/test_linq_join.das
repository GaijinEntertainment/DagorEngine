options gen2
require daslib/linq
require dastest/testing_boost public
require math

require daslib/linq
require daslib/linq_boost

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
        var expected = [
            (4., 2, 4.3, 4.9),
            (1., 1, 1.5, 1.5),
            (8., 1, 8.3, 8.3)
        ]
        for (c, e in query, expected) {
            t |> equal(c._0, e._0)
            t |> equal(c._1, e._1)
            t |> equal(c._2, e._2)
            t |> equal(c._3, e._3)
        }
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
        var expected = [
            (4., 2, 4.3, 4.9),
            (1., 1, 1.5, 1.5),
            (8., 1, 8.3, 8.3)
        ]
        for (c, e in query, expected) {
            t |> equal(c._0, e._0)
            t |> equal(c._1, e._1)
            t |> equal(c._2, e._2)
            t |> equal(c._3, e._3)
        }
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
        var expected = [
            (4., 2, 4.3, 4.9),
            (1., 1, 1.5, 1.5),
            (8., 1, 8.3, 8.3)
        ]
        for (c, e in query, expected) {
            t |> equal(c._0, e._0)
            t |> equal(c._1, e._1)
            t |> equal(c._2, e._2)
            t |> equal(c._3, e._3)
        }
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
        var expected = [
            (4., 2, 4.3, 4.9),
            (1., 1, 1.5, 1.5),
            (8., 1, 8.3, 8.3)
        ]
        for (c, e in query, expected) {
            t |> equal(c._0, e._0)
            t |> equal(c._1, e._1)
            t |> equal(c._2, e._2)
            t |> equal(c._3, e._3)
        }
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


