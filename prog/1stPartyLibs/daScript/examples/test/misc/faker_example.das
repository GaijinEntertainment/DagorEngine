require daslib/faker
require daslib/fuzzer

require daslib/regex_boost
require daslib/random

[export]
def main
    var fake <- Faker() // note - we can specify different RNG source
    fuzz(10) <|
        print("number {fake |> number}\n")
    fuzz(10) <|
        print("month {fake |> month}\n")
    fuzz(10) <|
        print("day {fake |> day}\n")
    fuzz(10) <|
        print("date {fake |> date}\n")
    fuzz(10) <|
        print("positive int {fake |> positive_int}\n")
    fuzz(10) <|
        print("int {fake |> any_int}\n")
