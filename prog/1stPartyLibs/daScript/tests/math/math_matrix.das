options gen2
require dastest/testing_boost
require daslib/faker
require daslib/fuzzer

require math

[test]
def test_matrix_op(t : T?) {
    t |> run("fuzz *") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            var m1 = fake |> random_float3x3
            var m2 = fake |> random_float3x3
            t |> accept(m1 * m2)
        }
        fuzz() {
            var m1 = fake |> random_float3x4
            var m2 = fake |> random_float3x4
            t |> accept(m1 * m2)
        }
        fuzz() {
            var m1 = fake |> random_float4x4
            var m2 = fake |> random_float4x4
            t |> accept(m1 * m2)
        }
    }

    t |> run("fuzz ==") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            var m1 = fake |> random_float3x3
            var m2 = fake |> random_float3x3
            t |> accept(m1 == m2)
        }
        fuzz() {
            var m1 = fake |> random_float3x4
            var m2 = fake |> random_float3x4
            t |> accept(m1 == m2)
        }
        fuzz() {
            var m1 = fake |> random_float4x4
            var m2 = fake |> random_float4x4
            t |> accept(m1 == m2)
        }
    }

    t |> run("fuzz !=") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            var m1 = fake |> random_float3x3
            var m2 = fake |> random_float3x3
            t |> accept(m1 != m2)
        }
        fuzz() {
            var m1 = fake |> random_float3x4
            var m2 = fake |> random_float3x4
            t |> accept(m1 != m2)
        }
        fuzz() {
            var m1 = fake |> random_float4x4
            var m2 = fake |> random_float4x4
            t |> accept(m1 != m2)
        }
    }
}

[test]
def test_matrix_manip(t : T?) {
    t |> run("fuzz translation") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(translation(fake |> random_float3))
        }
    }

    t |> run("fuzz transpose") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(transpose(fake |> random_float4x4))
        }
    }

    t |> run("fuzz persp_forward") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(persp_forward(fake |> random_float, fake |> random_float, fake |> random_float, fake |> random_float))
        }
    }

    t |> run("fuzz persp_reverse") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(persp_reverse(fake |> random_float, fake |> random_float, fake |> random_float, fake |> random_float))
        }
    }

    t |> run("fuzz look_at") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(look_at(fake |> random_float3, fake |> random_float3, fake |> random_float3))
        }
    }

    t |> run("fuzz compose") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(compose(fake |> random_float3, fake |> random_float4, fake |> random_float3))
        }
    }

    t |> run("fuzz decompose") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            var pos, scale : float3
            var rot : float4
            decompose(fake |> random_float4x4, pos, rot, scale)
        }
    }

    t |> run("fuzz inverse") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(inverse(fake |> random_float3x4))
        }
        fuzz() {
            t |> accept(inverse(fake |> random_float4x4))
        }
    }

    t |> run("fuzz determinant") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(determinant(fake |> random_float3x3))
        }
        fuzz() {
            t |> accept(determinant(fake |> random_float3x4))
        }
        fuzz() {
            t |> accept(determinant(fake |> random_float4x4))
        }
    }

    t |> run("fuzz orthonormal_inverse") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(orthonormal_inverse(fake |> random_float3x3))
        }
        fuzz() {
            t |> accept(orthonormal_inverse(fake |> random_float3x4))
        }
    }
}
