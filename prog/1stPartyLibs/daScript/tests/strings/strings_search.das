options gen2
require dastest/testing_boost
require daslib/strings_boost
require daslib/faker
require daslib/fuzzer
require daslib/regex

[test]
def test_search(t : T?) {

    t |> equal(find("123", "23"), 1)
    t |> equal(find("123", "22"), -1)
    t |> run("fuzz find") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let str = fake |> any_string
            let substr = fake |> any_string
            t |> accept(find(str, substr))
        }
    }

    t |> equal(find("123", '2'), 1)
    t |> equal(find("123", '4'), -1)
    t |> run("fuzz find char") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let str = fake |> any_string
            let substr = fake |> random_int % 256
            t |> accept(find(str, substr))
        }
    }

    t |> equal(find("123", "23", 1), 1)
    t |> equal(find("123", "23", 2), -1)
    t |> run("fuzz find1") @@(t : T?) {
        let limit = int(re_gen_get_rep_limit()) * 2
        var fake <- Faker()
        fuzz() {
            let str = fake |> any_string
            let substr = fake |> any_string
            let start = fake |> random_int % limit
            t |> accept(find(str, substr, start))
        }
    }

    t |> equal(find("123", '2', 1), 1)
    t |> equal(find("123", '2', 2), -1)
    t |> run("fuzz find1 char") @@(t : T?) {
        let limit = int(re_gen_get_rep_limit()) * 2
        var fake <- Faker()
        fuzz() {
            let str = fake |> any_string
            let substr = fake |> random_int % 255
            let start = fake |> random_int % limit
            t |> accept(find(str, substr, start))
        }
    }

    // rfind — reverse find
    t |> equal(rfind("hello/world/foo", "/"), 11)
    t |> equal(rfind("hello/world/foo", "world"), 6)
    t |> equal(rfind("hello", "/"), -1)
    t |> equal(rfind("", "/"), -1)
    t |> equal(rfind("abc", ""), -1)
    t |> equal(rfind("aaa", "a"), 2)
    t |> run("fuzz rfind") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let str = fake |> any_string
            let substr = fake |> any_string
            t |> accept(rfind(str, substr))
        }
    }

    // rfind with start position
    t |> equal(rfind("hello/world/foo", "/", 10), 5)
    t |> equal(rfind("hello/world/foo", "/", 11), 11)
    t |> equal(rfind("hello/world/foo", "/", 4), -1)
    t |> equal(rfind("aaa", "a", 1), 1)
    t |> equal(rfind("aaa", "a", 0), 0)
    t |> run("fuzz rfind1") @@(t : T?) {
        let limit = int(re_gen_get_rep_limit()) * 2
        var fake <- Faker()
        fuzz() {
            let str = fake |> any_string
            let substr = fake |> any_string
            let start = fake |> random_int % limit
            t |> accept(rfind(str, substr, start))
        }
    }
}
