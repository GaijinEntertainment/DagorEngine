options gen2
require dastest/testing_boost
require daslib/strings_boost
require daslib/faker
require daslib/fuzzer

[test]
def test_string_as_array(t : T?) {
    peek_data("123") $(arr) {
        t |> equal(length(arr), 3)
        t |> equal(arr[0], uint8('1'))
        t |> equal(arr[1], uint8('2'))
        t |> equal(arr[2], uint8('3'))
    }
    t |> run("fuzz peek_data") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let st = fake |> any_string
            peek_data(st) $(arr) {
                t |> equal(length(st), length(arr))
            }
        }
    }

    var SST = "123"
    let SSTR = modify_data(SST) $(arr) {
        t |> equal(length(arr), 3)
        t |> equal(arr[0], uint8('1'))
        t |> equal(arr[1], uint8('2'))
        t |> equal(arr[2], uint8('3'))
        arr[0] = uint8('4')
        arr[1] = uint8('5')
        arr[2] = uint8('6')
    }
    t |> equal(SSTR, "456")
    t |> run("fuzz modify_data") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            var st = fake |> any_string
            modify_data(st) $(arr) {
                t |> equal(length(st), length(arr))
                for (Ch in arr) {
                    Ch = uint8(fake |> random_int)
                }
            }
        }
    }
}
