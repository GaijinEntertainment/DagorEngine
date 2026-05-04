options gen2
require dastest/testing_boost
require daslib/bool_array

// Helper: verify BoolArray matches array<bool> element by element
def verify_match(t : T?; a : BoolArray; b : array<bool>; context_msg : string) {
    t |> equal(a.length(), length(b))
    for (i in range(length(b))) {
        if (!t |> equal(a[i], b[i])) {
            t |> failure("{context_msg}: mismatch at index {i}, expected {b[i]}, got {a[i]}, len={length(b)}")
            return
        }
    }
}

[test]
def test_basic(t : T?) {
    t |> run("clear_and_length") @@(t : T?) {
        var a : BoolArray
        t |> equal(a.length(), 0)
        a.push(true)
        a.push(false)
        t |> equal(a.length(), 2)
        a.clear()
        t |> equal(a.length(), 0)
    }
    t |> run("push_and_index") @@(t : T?) {
        var a : BoolArray
        for (i in range(100)) {
            a.push(i % 3 == 0)
        }
        t |> equal(a.length(), 100)
        for (i in range(100)) {
            t |> equal(a[i], i % 3 == 0)
        }
    }
    t |> run("set_index") @@(t : T?) {
        var a : BoolArray
        a.resize(64)
        for (i in range(64)) {
            t |> equal(a[i], false)
        }
        for (i in range(64)) {
            a[i] = i % 3 == 0
        }
        for (i in range(64)) {
            t |> equal(a[i], i % 3 == 0)
        }
        for (i in range(64)) {
            a[i] = !(i % 3 == 0)
        }
        for (i in range(64)) {
            t |> equal(a[i], !(i % 3 == 0))
        }
    }
    t |> run("xor_assign") @@(t : T?) {
        var a : BoolArray
        a.resize(10)
        for (i in range(10)) {
            a[i] = i < 5
        }
        a[0] ^^= true
        a[5] ^^= true
        a[1] ^^= false
        a[6] ^^= false
        t |> equal(a[0], false)
        t |> equal(a[5], true)
        t |> equal(a[1], true)
        t |> equal(a[6], false)
    }
    t |> run("and_assign") @@(t : T?) {
        var a : BoolArray
        a.resize(4)
        a[0] = true;  a[1] = true;  a[2] = false; a[3] = false
        a[0] &&= true
        a[1] &&= false
        a[2] &&= true
        a[3] &&= false
        t |> equal(a[0], true)
        t |> equal(a[1], false)
        t |> equal(a[2], false)
        t |> equal(a[3], false)
    }
    t |> run("or_assign") @@(t : T?) {
        var a : BoolArray
        a.resize(4)
        a[0] = true;  a[1] = true;  a[2] = false; a[3] = false
        a[0] ||= true
        a[1] ||= false
        a[2] ||= true
        a[3] ||= false
        t |> equal(a[0], true)
        t |> equal(a[1], true)
        t |> equal(a[2], true)
        t |> equal(a[3], false)
    }
    t |> run("pop") @@(t : T?) {
        var a : BoolArray
        a.push(true)
        a.push(false)
        a.push(true)
        t |> equal(a.pop(), true)
        t |> equal(a.length(), 2)
        t |> equal(a.pop(), false)
        t |> equal(a.length(), 1)
        t |> equal(a.pop(), true)
        t |> equal(a.length(), 0)
    }
    t |> run("to_string") @@(t : T?) {
        var a : BoolArray
        t |> equal(a.to_string(), "[]")
        a.push(true)
        t |> equal(a.to_string(), "[true]")
        a.push(false)
        t |> equal(a.to_string(), "[true, false]")
    }
}

[test]
def test_resize(t : T?) {
    t |> run("resize_grow_zeros") @@(t : T?) {
        var a : BoolArray
        for (size in range(0, 130)) {
            a.clear()
            a.resize(size)
            t |> equal(a.length(), size)
            for (j in range(size)) {
                if (!t |> equal(a[j], false)) {
                    t |> failure("resize({size}): expected false at index {j}")
                    return
                }
            }
        }
    }
    t |> run("resize_preserves_data") @@(t : T?) {
        var a : BoolArray
        for (initial_size in range(1, 66)) {
            a.clear()
            a.resize(initial_size)
            for (j in range(initial_size)) {
                a[j] = j % 2 == 0
            }
            let new_size = initial_size + 33
            a.resize(new_size)
            t |> equal(a.length(), new_size)
            for (j in range(initial_size)) {
                if (!t |> equal(a[j], j % 2 == 0)) {
                    t |> failure("resize_preserves: initial={initial_size}, new={new_size}, index {j}")
                    return
                }
            }
            for (j in range(initial_size, new_size)) {
                if (!t |> equal(a[j], false)) {
                    t |> failure("resize_preserves: new region not false at {j}, initial={initial_size}")
                    return
                }
            }
        }
    }
    t |> run("resize_shrink_and_grow") @@(t : T?) {
        var a : BoolArray
        for (size in range(1, 130)) {
            a.clear()
            a.resize(size)
            for (j in range(size)) {
                a[j] = true
            }
            let shrunk = size / 2
            a.resize(shrunk)
            a.resize(size)
            t |> equal(a.length(), size)
            for (j in range(shrunk)) {
                if (!t |> equal(a[j], true)) {
                    t |> failure("shrink_grow({size}): lost data at {j}")
                    return
                }
            }
            for (j in range(shrunk, size)) {
                if (!t |> equal(a[j], false)) {
                    t |> failure("shrink_grow({size}): not false at {j} after grow")
                    return
                }
            }
        }
    }
    t |> run("resize_boundary_31_32_33") @@(t : T?) {
        var a : BoolArray
        var boundaries : array<int>
        boundaries |> push(31); boundaries |> push(32); boundaries |> push(33)
        boundaries |> push(63); boundaries |> push(64); boundaries |> push(65)
        for (bi in range(length(boundaries))) {
            let boundary = boundaries[bi]
            a.clear()
            a.resize(boundary)
            for (j in range(boundary)) {
                a[j] = true
            }
            for (j in range(boundary)) {
                t |> equal(a[j], true)
            }
            a.resize(boundary + 1)
            t |> equal(a[boundary], false)
            for (j in range(boundary)) {
                t |> equal(a[j], true)
            }
        }
    }
}

[test]
def test_erase(t : T?) {
    t |> run("erase_vs_reference") @@(t : T?) {
        var a : BoolArray
        var b : array<bool>
        for (len in range(1, 40)) {
            for (erase_i in range(len)) {
                a.clear()
                b |> clear()
                for (i in range(len)) {
                    a.push(i % 2 == 0)
                    b |> push(i % 2 == 0)
                }
                a.erase(erase_i)
                b |> erase(erase_i)
                verify_match(t, a, b, "erase len={len} at={erase_i}")
            }
        }
    }
    t |> run("erase_all_true") @@(t : T?) {
        var a : BoolArray
        var b : array<bool>
        var lens : array<int>
        lens |> push(1); lens |> push(31); lens |> push(32); lens |> push(33)
        lens |> push(63); lens |> push(64); lens |> push(65)
        for (li in range(length(lens))) {
            let len = lens[li]
            for (erase_i in range(len)) {
                a.clear()
                b |> clear()
                for (i in range(len)) {
                    a.push(true)
                    b |> push(true)
                }
                a.erase(erase_i)
                b |> erase(erase_i)
                verify_match(t, a, b, "erase_all_true len={len} at={erase_i}")
            }
        }
    }
    t |> run("erase_all_false") @@(t : T?) {
        var a : BoolArray
        var b : array<bool>
        var lens : array<int>
        lens |> push(1); lens |> push(31); lens |> push(32); lens |> push(33)
        lens |> push(63); lens |> push(64); lens |> push(65)
        for (li in range(length(lens))) {
            let len = lens[li]
            for (erase_i in range(len)) {
                a.clear()
                b |> clear()
                for (i in range(len)) {
                    a.push(false)
                    b |> push(false)
                }
                a.erase(erase_i)
                b |> erase(erase_i)
                verify_match(t, a, b, "erase_all_false len={len} at={erase_i}")
            }
        }
    }
    t |> run("erase_single_true_bit") @@(t : T?) {
        var a : BoolArray
        var b : array<bool>
        var lens : array<int>
        lens |> push(2); lens |> push(31); lens |> push(32); lens |> push(33)
        for (li in range(length(lens))) {
            let len = lens[li]
            for (true_pos in range(len)) {
                for (erase_i in range(len)) {
                    a.clear()
                    b |> clear()
                    for (i in range(len)) {
                        a.push(i == true_pos)
                        b |> push(i == true_pos)
                    }
                    a.erase(erase_i)
                    b |> erase(erase_i)
                    verify_match(t, a, b, "erase_single_true len={len} true_pos={true_pos} erase={erase_i}")
                }
            }
        }
    }
    t |> run("erase_repeated_to_empty") @@(t : T?) {
        var a : BoolArray
        var b : array<bool>
        let len = 67
        for (i in range(len)) {
            a.push(i % 3 == 0)
            b |> push(i % 3 == 0)
        }
        for (i in range(len)) {
            a.erase(0)
            b |> erase(0)
            verify_match(t, a, b, "erase_repeated step={i}")
        }
        t |> equal(a.length(), 0)
    }
    t |> run("erase_from_back_repeatedly") @@(t : T?) {
        var a : BoolArray
        var b : array<bool>
        let len = 67
        for (i in range(len)) {
            a.push(i % 5 == 0)
            b |> push(i % 5 == 0)
        }
        for (i in range(len)) {
            let idx = a.length() - 1
            a.erase(idx)
            b |> erase(idx)
            verify_match(t, a, b, "erase_back step={i}")
        }
        t |> equal(a.length(), 0)
    }
    t |> run("erase_at_dword_boundary") @@(t : T?) {
        var a : BoolArray
        var b : array<bool>
        let len = 96
        var positions : array<int>
        positions |> push(0); positions |> push(30); positions |> push(31); positions |> push(32)
        positions |> push(33); positions |> push(62); positions |> push(63); positions |> push(64)
        positions |> push(65)
        for (pi in range(length(positions))) {
            let erase_at = positions[pi]
            a.clear()
            b |> clear()
            for (i in range(len)) {
                a.push(i % 2 == 0)
                b |> push(i % 2 == 0)
            }
            a.erase(erase_at)
            b |> erase(erase_at)
            verify_match(t, a, b, "erase_boundary at={erase_at}")
        }
    }
}

[test]
def test_insert(t : T?) {
    t |> run("insert_vs_reference") @@(t : T?) {
        var a : BoolArray
        var b : array<bool>
        for (len in range(1, 40)) {
            for (insert_i in range(len)) {
                // test both true and false
                a.clear()
                b |> clear()
                for (i in range(len)) {
                    a.push(i % 2 == 0)
                    b |> push(i % 2 == 0)
                }
                a.insert(insert_i, true)
                b |> push(true, insert_i)
                verify_match(t, a, b, "insert len={len} at={insert_i} val=true")

                a.clear()
                b |> clear()
                for (i in range(len)) {
                    a.push(i % 2 == 0)
                    b |> push(i % 2 == 0)
                }
                a.insert(insert_i, false)
                b |> push(false, insert_i)
                verify_match(t, a, b, "insert len={len} at={insert_i} val=false")
            }
        }
    }
    t |> run("insert_all_true") @@(t : T?) {
        var a : BoolArray
        var b : array<bool>
        var lens : array<int>
        lens |> push(1); lens |> push(31); lens |> push(32); lens |> push(33)
        lens |> push(63); lens |> push(64); lens |> push(65)
        for (li in range(length(lens))) {
            let len = lens[li]
            for (insert_i in range(len)) {
                a.clear()
                b |> clear()
                for (i in range(len)) {
                    a.push(true)
                    b |> push(true)
                }
                a.insert(insert_i, false)
                b |> push(false, insert_i)
                verify_match(t, a, b, "insert_all_true len={len} at={insert_i}")
            }
        }
    }
    t |> run("insert_at_zero_repeatedly") @@(t : T?) {
        var a : BoolArray
        var b : array<bool>
        a.push(false)
        b |> push(false)
        for (i in range(1, 67)) {
            let val = i % 3 == 0
            a.insert(0, val)
            b |> push(val, 0)
            verify_match(t, a, b, "insert_at_zero step={i}")
        }
    }
    t |> run("insert_at_dword_boundary") @@(t : T?) {
        var a : BoolArray
        var b : array<bool>
        let len = 96
        var positions : array<int>
        positions |> push(0); positions |> push(30); positions |> push(31); positions |> push(32)
        positions |> push(33); positions |> push(62); positions |> push(63); positions |> push(64)
        positions |> push(65)
        for (pi in range(length(positions))) {
            let insert_at = positions[pi]
            // test true
            a.clear()
            b |> clear()
            for (i in range(len)) {
                a.push(i % 2 == 0)
                b |> push(i % 2 == 0)
            }
            a.insert(insert_at, true)
            b |> push(true, insert_at)
            verify_match(t, a, b, "insert_boundary at={insert_at} val=true")
            // test false
            a.clear()
            b |> clear()
            for (i in range(len)) {
                a.push(i % 2 == 0)
                b |> push(i % 2 == 0)
            }
            a.insert(insert_at, false)
            b |> push(false, insert_at)
            verify_match(t, a, b, "insert_boundary at={insert_at} val=false")
        }
    }
    t |> run("insert_carry_propagation") @@(t : T?) {
        var a : BoolArray
        var b : array<bool>
        let len = 96
        for (i in range(len)) {
            a.push(true)
            b |> push(true)
        }
        a.insert(0, false)
        b |> push(false, 0)
        verify_match(t, a, b, "insert_carry_all_true")
    }
    t |> run("insert_then_erase_roundtrip") @@(t : T?) {
        var a : BoolArray
        var b : array<bool>
        var lens : array<int>
        lens |> push(1); lens |> push(31); lens |> push(32); lens |> push(33)
        lens |> push(64); lens |> push(65)
        for (li in range(length(lens))) {
            let len = lens[li]
            a.clear()
            b |> clear()
            for (i in range(len)) {
                a.push(i % 3 == 0)
                b |> push(i % 3 == 0)
            }
            for (idx in range(len)) {
                a.insert(idx, true)
                a.erase(idx)
                verify_match(t, a, b, "insert_erase_roundtrip len={len} idx={idx}")
            }
        }
    }
    t |> run("erase_then_insert_roundtrip") @@(t : T?) {
        var a : BoolArray
        var b : array<bool>
        var lens : array<int>
        lens |> push(2); lens |> push(31); lens |> push(32); lens |> push(33)
        lens |> push(64); lens |> push(65)
        for (li in range(length(lens))) {
            let len = lens[li]
            a.clear()
            b |> clear()
            for (i in range(len)) {
                a.push(i % 3 == 0)
                b |> push(i % 3 == 0)
            }
            for (idx in range(len)) {
                let saved = a[idx]
                a.erase(idx)
                a.insert(idx, saved)
                verify_match(t, a, b, "erase_insert_roundtrip len={len} idx={idx}")
            }
        }
    }
}

[test]
def test_push(t : T?) {
    t |> run("push_at_position") @@(t : T?) {
        var a : BoolArray
        var b : array<bool>
        for (len in range(1, 40)) {
            for (at_pos in range(len)) {
                // test true
                a.clear()
                b |> clear()
                for (i in range(len)) {
                    a.push(i % 2 == 0)
                    b |> push(i % 2 == 0)
                }
                a.push(true, at_pos)
                b |> push(true, at_pos)
                verify_match(t, a, b, "push_at len={len} at={at_pos} val=true")
                // test false
                a.clear()
                b |> clear()
                for (i in range(len)) {
                    a.push(i % 2 == 0)
                    b |> push(i % 2 == 0)
                }
                a.push(false, at_pos)
                b |> push(false, at_pos)
                verify_match(t, a, b, "push_at len={len} at={at_pos} val=false")
            }
        }
    }
    t |> run("push_at_end") @@(t : T?) {
        var a : BoolArray
        var b : array<bool>
        for (i in range(67)) {
            let val = i % 4 == 0
            a.push(val, a.length())
            b |> push(val)
            verify_match(t, a, b, "push_at_end step={i}")
        }
    }
    t |> run("push_pop_cycle") @@(t : T?) {
        var a : BoolArray
        for (cycle in range(3)) {
            for (i in range(100)) {
                a.push(i % 2 == 0)
            }
            t |> equal(a.length(), 100)
            for (i in range(100)) {
                let expected = (99 - i) % 2 == 0
                t |> equal(a.pop(), expected)
            }
            t |> equal(a.length(), 0)
        }
    }
}

[test]
def test_iterator(t : T?) {
    t |> run("iterator_empty") @@(t : T?) {
        var a : BoolArray
        var count = 0
        for (b in a) {
            count += 1
        }
        t |> equal(count, 0)
    }
    t |> run("iterator_various_sizes") @@(t : T?) {
        var a : BoolArray
        var sizes : array<int>
        sizes |> push(1); sizes |> push(2); sizes |> push(31); sizes |> push(32)
        sizes |> push(33); sizes |> push(63); sizes |> push(64); sizes |> push(65)
        sizes |> push(100)
        for (si in range(length(sizes))) {
            let size = sizes[si]
            a.clear()
            for (i in range(size)) {
                a.push(i % 3 == 0)
            }
            var idx = 0
            for (b in a) {
                t |> equal(b, idx % 3 == 0)
                idx += 1
            }
            t |> equal(idx, size)
        }
    }
    t |> run("iterator_all_true") @@(t : T?) {
        var a : BoolArray
        for (i in range(65)) {
            a.push(true)
        }
        var count = 0
        for (b in a) {
            t |> equal(b, true)
            count += 1
        }
        t |> equal(count, 65)
    }
    t |> run("iterator_all_false") @@(t : T?) {
        var a : BoolArray
        for (i in range(65)) {
            a.push(false)
        }
        var count = 0
        for (b in a) {
            t |> equal(b, false)
            count += 1
        }
        t |> equal(count, 65)
    }
}

[test]
def test_stress(t : T?) {
    t |> run("build_with_insert_only") @@(t : T?) {
        var a : BoolArray
        var b : array<bool>
        a.push(false)
        b |> push(false)
        for (i in range(1, 100)) {
            let val = i % 3 == 0
            a.insert(0, val)
            b |> push(val, 0)
        }
        verify_match(t, a, b, "build_with_insert_only")
    }
    t |> run("alternating_insert_erase") @@(t : T?) {
        var a : BoolArray
        var b : array<bool>
        for (i in range(50)) {
            a.push(i % 2 == 0)
            b |> push(i % 2 == 0)
        }
        for (step in range(200)) {
            let pos = step % a.length()
            if (step % 3 == 0) {
                let val = step % 5 == 0
                a.insert(pos, val)
                b |> push(val, pos)
            } else {
                a.erase(pos)
                b |> erase(pos)
                if (a.length() == 0) {
                    a.push(true)
                    b |> push(true)
                }
            }
            verify_match(t, a, b, "alt_insert_erase step={step}")
        }
    }
    t |> run("random_pattern") @@(t : T?) {
        var a : BoolArray
        var b : array<bool>
        var seed = 12345u
        for (i in range(200)) {
            seed = seed * 1103515245u + 12345u
            let val = (seed & 0x10000u) != 0u
            a.push(val)
            b |> push(val)
        }
        verify_match(t, a, b, "random_push")
        for (i in range(100)) {
            seed = seed * 1103515245u + 12345u
            let pos = int(seed % uint(a.length()))
            a.erase(pos)
            b |> erase(pos)
            verify_match(t, a, b, "random_erase step={i}")
        }
        for (i in range(100)) {
            seed = seed * 1103515245u + 12345u
            let pos = int(seed % uint(a.length()))
            let val = (seed & 0x80000u) != 0u
            a.insert(pos, val)
            b |> push(val, pos)
            verify_match(t, a, b, "random_insert step={i}")
        }
    }
}

