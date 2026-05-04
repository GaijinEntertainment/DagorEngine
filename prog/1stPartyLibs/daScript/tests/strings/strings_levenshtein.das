options gen2
require dastest/testing_boost
require daslib/strings_boost
require daslib/faker
require daslib/fuzzer
require daslib/regex

require daslib/strings_boost
require math

[test]
def test_levenshtein_distance_fast(t : T?) {
    t |> equal(levenshtein_distance_fast("kitten", "sitting"), 3)
    t |> equal(levenshtein_distance_fast("Saturday", "Sunday"), 3)
    let limit = int(re_gen_get_rep_limit())
    var fake <- Faker()
    fuzz() {
        let ss = fake |> any_string
        let tt = fake |> any_string
        let dist = levenshtein_distance_fast(ss, tt)
        t |> success(dist <= limit)
        t |> equal(dist, levenshtein_distance(ss, tt))
    }
}
