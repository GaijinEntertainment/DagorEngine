options gen2
require dastest/testing_boost
require daslib/strings_boost
require daslib/faker
require daslib/fuzzer

[test]
def test_traits(t : T?) {
    t |> run("traits") @@(t : T?) {
        t |> success(is_alpha('A'))
        t |> success(!is_alpha('9'))
        t |> success(is_new_line('\n'))
        t |> success(!is_new_line(' '))
        t |> success(is_white_space('\n'))
        t |> success(!is_white_space('#'))
        t |> success(is_number('3'))
        t |> success(!is_number('A'))
    }
    t |> run("traits fuzz is_alpha") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let Ch = fake |> any_char
            t |> accept(is_alpha(Ch))
        }
    }
    t |> run("traits fuzz is_new_line") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let Ch = fake |> any_char
            t |> accept(is_new_line(Ch))
        }
    }
    t |> run("traits fuzz is_white_space") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let Ch = fake |> any_char
            t |> accept(is_white_space(Ch))
        }
    }
    t |> run("traits fuzz is_number") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let Ch = fake |> any_char
            t |> accept(is_number(Ch))
        }
    }
}
