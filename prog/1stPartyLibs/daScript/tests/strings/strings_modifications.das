options gen2
require dastest/testing_boost
require daslib/strings_boost
require daslib/faker
require daslib/fuzzer
require daslib/regex


[test]
def test_modifications(t : T?) {
    t |> equal(repeat("ab", 3), "ababab")
    t |> run("fuzz repeat") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let str = fake |> any_string
            let count = fake |> random_int % 10
            t |> accept(repeat(str, count))
        }
    }

    t |> equal(strip(" ab "), "ab")
    t |> run("fuzz strip") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let str = fake |> any_string
            t |> accept(strip(str))
        }
    }

    t |> equal(strip_right(" ab "), " ab")
    t |> run("fuzz strip_right") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let str = fake |> any_string
            t |> accept(strip_right(str))
        }
    }

    t |> equal(rtrim(" ab "), " ab")
    t |> run("fuzz rtrim") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let str = fake |> any_string
            t |> accept(rtrim(str))
        }
    }

    t |> equal(strip_left(" ab "), "ab ")
    t |> run("fuzz strip_left") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let str = fake |> any_string
            t |> accept(strip_left(str))
        }
    }

    t |> equal(chop(" ab ", 1, 2), "ab")
    t |> run("fuzz chop") @@(t : T?) {
        let limit = int(re_gen_get_rep_limit()) * 2
        var fake <- Faker()
        fuzz() {
            let str = fake |> any_string
            let start = fake |> random_int % limit
            let len = fake |> random_int % limit
            t |> accept(chop(str, start, len))
        }
    }

    t |> equal(slice(" ab ", 1), "ab ")
    t |> run("fuzz slice") @@(t : T?) {
        let limit = int(re_gen_get_rep_limit()) * 2
        var fake <- Faker()
        fuzz() {
            let str = fake |> any_string
            let start = fake |> random_int % limit
            t |> accept(slice(str, start))
        }
    }

    t |> equal(slice(" ab ", 1, 3), "ab")
    t |> run("fuzz slice2") @@(t : T?) {
        let limit = int(re_gen_get_rep_limit()) * 2
        var fake <- Faker()
        fuzz() {
            let str = fake |> any_string
            let start = fake |> random_int % limit
            let end = fake |> random_int % limit
            t |> accept(slice(str, start, end))
        }
    }

    t |> equal(reverse("1234"), "4321")
    t |> run("fuzz reverse") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let str = fake |> any_string
            t |> accept(reverse(str))
        }
    }

    t |> equal(to_upper("abCD"), "ABCD")
    t |> run("fuzz to_upper") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let str = to_upper(fake |> any_string)
            for (ch in str) {
                t |> success(!(ch >= 'a' && ch <= 'z'))
            }
        }
    }

    t |> equal(to_lower("abCD"), "abcd")
    t |> run("fuzz to_lower") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let str = to_lower(fake |> any_string)
            for (ch in str) {
                t |> success(!(ch >= 'A' && ch <= 'Z'))
            }
        }
    }

    t |> equal(escape("\n"), "\\n")
    t |> run("fuzz escape") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let str = fake |> any_string
            t |> accept(escape(str))
        }
    }

    t |> equal(unescape("\\n"), "\n")
    t |> run("fuzz unescape") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let str = fake |> any_string
            t |> accept(unescape(str))
        }
    }

    t |> equal(replace("1234", "23", "a"), "1a4")
    t |> run("fuzz replace") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let str = fake |> any_string
            let what = fake |> any_string
            let to = fake |> any_string
            t |> accept(replace(str, what, to))
        }
    }
}