options gen2
require dastest/testing_boost
require daslib/strings_boost
require daslib/faker
require daslib/fuzzer
require daslib/regex


[test]
def test_split_and_join(t : T?) {
    let spl <- split("12,34,56", ",")
    t |> equal(length(spl), 3)
    t |> equal(spl[0], "12")
    t |> equal(spl[1], "34")
    t |> equal(spl[2], "56")
    t |> run("fuzz split") @@(t : T?) {
        let limit = int(re_gen_get_rep_limit())
        var fake <- Faker()
        fuzz() {
            let txt = fake |> any_string
            let delim = fake |> any_string
            var foo <- split(txt, delim)
            t |> success(length(foo) <= limit)
            delete foo
        }
    }

    split("12,34,56", ",") $(splb) {
        t |> equal(length(splb), 3)
        t |> equal(splb[0], "12")
        t |> equal(splb[1], "34")
        t |> equal(splb[2], "56")
    }
    t |> run("fuzz split") @@(t : T?) {
        let limit = int(re_gen_get_rep_limit())
        var fake <- Faker()
        fuzz() {
            let txt = fake |> any_string
            let delim = fake |> any_string
            split(txt, delim) $(foo) {
                t |> success(length(foo) <= limit)
            }
        }
    }

    let splbc <- split_by_chars("12,34;56", ",;")
    t |> equal(length(splbc), 3)
    t |> equal(splbc[0], "12")
    t |> equal(splbc[1], "34")
    t |> equal(splbc[2], "56")
    t |> run("fuzz split_by_chars") @@(t : T?) {
        let limit = int(re_gen_get_rep_limit())
        var fake <- Faker()
        fuzz() {
            let txt = fake |> any_string
            let delim = fake |> any_string
            var foo <- split(txt, delim)
            t |> success(length(foo) <= limit)
            delete foo
        }
    }

    split_by_chars("12,34;56", ",;") $(splb) {
        t |> equal(length(splb), 3)
        t |> equal(splb[0], "12")
        t |> equal(splb[1], "34")
        t |> equal(splb[2], "56")
    }
    t |> run("fuzz split_by_chars") @@(t : T?) {
        let limit = int(re_gen_get_rep_limit())
        var fake <- Faker()
        fuzz() {
            let txt = fake |> any_string
            let delim = fake |> any_string
            split_by_chars(txt, delim) $(foo) {
                t |> success(length(foo) <= limit)
            }
        }
    }

    t |> equal(join([ "1", "2", "3"], ","), "1,2,3")
    t |> run("fuzz join") @@(t : T?) {
        let limit = int(re_gen_get_rep_limit())
        var fake <- Faker()
        fuzz() {
            let elem = fake |> random_uint % 10u
            var arr <- [for (x in urange(elem)); fake |> any_string]
            t |> accept(join(arr, fake |> any_string))
        }
    }
}


[test]
def tricky_test_split_and_join(t : T?) {
    let splStr = ",,12,,34,56,,"
    let spl <- split(splStr, ",")
    t |> equal(length(spl), 8)
    t |> equal(spl[0], "")
    t |> equal(spl[1], "")
    t |> equal(spl[2], "12")
    t |> equal(spl[3], "")
    t |> equal(spl[4], "34")
    t |> equal(spl[5], "56")
    t |> equal(spl[6], "")
    t |> equal(spl[7], "")
    t |> equal(join(spl, ","), splStr)


    split(splStr, ",") $(splb) {
        t |> equal(length(spl), 8)
        t |> equal(spl[0], "")
        t |> equal(spl[1], "")
        t |> equal(spl[2], "12")
        t |> equal(spl[3], "")
        t |> equal(spl[4], "34")
        t |> equal(spl[5], "56")
        t |> equal(spl[6], "")
        t |> equal(spl[7], "")
        t |> equal(join(spl, ","), splStr)
    }

    let splbcStr = ";;12,,34;56,,"
    let splbc <- split_by_chars(splbcStr, ",;")
    t |> equal(length(spl), 8)
    t |> equal(splbc[0], "")
    t |> equal(splbc[1], "")
    t |> equal(splbc[2], "12")
    t |> equal(splbc[3], "")
    t |> equal(splbc[4], "34")
    t |> equal(splbc[5], "56")
    t |> equal(splbc[6], "")
    t |> equal(splbc[7], "")

    split_by_chars(splbcStr, ",;") $(splb) {
        t |> equal(length(spl), 8)
        t |> equal(splb[0], "")
        t |> equal(splb[1], "")
        t |> equal(splb[2], "12")
        t |> equal(splb[3], "")
        t |> equal(splb[4], "34")
        t |> equal(splb[5], "56")
        t |> equal(splb[6], "")
        t |> equal(splb[7], "")
    }
}


