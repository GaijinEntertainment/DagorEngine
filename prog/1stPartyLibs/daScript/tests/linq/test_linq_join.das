options gen2
require daslib/linq
require dastest/testing_boost public
require math

require daslib/linq

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
        var arr1 = [1, 2, 3, 4, 5]
        var arr2 = [4, 5, 6, 7, 8]
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
        var arr1 = [1, 2, 3, 4, 5]
        var arr2 = [4, 5, 6, 7, 8]
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

