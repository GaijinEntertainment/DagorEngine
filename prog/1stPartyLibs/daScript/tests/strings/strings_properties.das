options gen2
require dastest/testing_boost
require daslib/strings_boost
require daslib/faker
require daslib/fuzzer
require daslib/regex

[test]
def test_properties(t : T?) {
    t |> run("length") @@(t : T?) {
        t |> equal(length(""), 0)
        t |> equal(length("123"), 3)
        var fake <- Faker()
        let limit = re_gen_get_rep_limit()
        fuzz() {
            let str = fake |> any_string
            let len = str |> length
            t |> success(len >= 0 && len < int(limit))
        }
        fuzz(100) {
            let str = fake |> long_string
            let len = str |> length
            t |> success(len >= 0 && len < int(fake.max_long_string))
        }
    }

    t |> run("empty") @@(t : T?) {
        t |> success(empty(""))
        t |> success(!empty("123"))
        var fake <- Faker()
        let limit = re_gen_get_rep_limit()
        fuzz() {
            let str = fake |> any_string
            t |> accept(str |> empty)
        }
    }

    t |> run("starts_with") @@(t : T?) {
        t |> success(starts_with("123", "12"))
        t |> success(starts_with("123", "123"))
        t |> success(!starts_with("123", "23"))
        t |> success(!starts_with("123", "1234"))
        t |> success(starts_with("123", 1, "23"))
        t |> success(starts_with("123", 1, "23", 2u))
        t |> success(starts_with("123", 1, "2", 1u))
        t |> success(starts_with("123", 2, "3"))
        t |> success(starts_with("123", 2, "3", 2u)) // yes, it's ok
        t |> success(!starts_with("123", 3, "4"))
        t |> success(!starts_with("123", 3, "4", 2u))
        var fake <- Faker()
        let limit = re_gen_get_rep_limit()
        fuzz() {
            let str = fake |> any_string
            let cmp = fake |> any_string
            t |> accept(starts_with(str, cmp))
        }
    }

    t |> run("ends_with") @@(t : T?) {
        t |> success(ends_with("123", "23"))
        t |> success(ends_with("123", "123"))
        t |> success(!ends_with("123", "12"))
        t |> success(!ends_with("123", "0123"))
        var fake <- Faker()
        let limit = re_gen_get_rep_limit()
        fuzz() {
            let str = fake |> any_string
            let cmp = fake |> any_string
            t |> accept(ends_with(str, cmp))
        }
    }
}

