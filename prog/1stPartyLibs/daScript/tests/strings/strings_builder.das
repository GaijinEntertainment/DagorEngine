options gen2
require dastest/testing_boost
require daslib/strings_boost
require daslib/faker
require daslib/fuzzer

[test]
def test_builder(t : T?) {
    // TODO: manually test builder

    t |> run("fuzz write") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            build_string() $(writer) {
                let str = fake |> any_string
                writer |> write(str)
            }
        }
    }

    t |> run("fuzz write_char") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            build_string() $(writer) {
                // TODO: implement all different formats?
                let Ch = fake |> random_int
                writer |> write_char(Ch)
            }
        }
    }

    t |> run("fuzz write_chars") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            build_string() $(writer) {
                let Ch = fake |> random_int
                let Count = fake |> random_int % 100
                writer |> write_chars(Ch, Count)
            }
        }
    }

    t |> run("fuzz write_escape_string") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            build_string() $(writer) {
                let str = fake |> any_string
                writer |> write_escape_string(str)
            }
        }
    }

    /*
    t |> run("fuzz format") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            build_string() $(writer) {
                let str = fake |> any_string
                writer |> format(str, 0)
            }
        }
    }
    */
}

