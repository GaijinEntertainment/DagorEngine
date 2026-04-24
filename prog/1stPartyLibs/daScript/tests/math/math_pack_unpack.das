options gen2
require dastest/testing_boost
require daslib/faker
require daslib/fuzzer

require math

[test]
def test_matrix_op(t : T?) {
    t |> run("fuzz pack_float_to_byte") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(pack_float_to_byte(fake |> random_float4))
        }
        fuzz() {
            t |> accept(unpack_byte_to_float(fake |> random_uint))
        }
    }
}
