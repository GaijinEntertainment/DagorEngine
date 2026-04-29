options gen2
require dastest/testing_boost
require daslib/faker
require daslib/fuzzer

require math

[test]
def test_quaternions(t : T?) {
    t |> run("fuzz quat_from_unit_arc") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(quat_from_unit_arc(fake |> random_float3, fake |> random_float3))
        }
    }

    t |> run("fuzz quat_from_unit_vec_ang") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(quat_from_unit_vec_ang(fake |> random_float3, fake |> random_float))
        }
    }

    t |> run("fuzz quat_from_float3x4") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(quat(fake |> random_float3x4))
        }
    }

    t |> run("fuzz quat_mul") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(quat_mul(fake |> random_float4, fake |> random_float4))
        }
    }

    t |> run("fuzz quat_mul_vec") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(quat_mul_vec(fake |> random_float4, fake |> random_float3))
        }
    }

    t |> run("fuzz quat_conjugate") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(quat_conjugate(fake |> random_float4))
        }
    }
}
