options gen2
require dastest/testing_boost
require daslib/faker
require daslib/fuzzer

require fake_numeric

require math

[test]
def test_min_max(t : T?) {
    fake_numeric2(min)
    fake_numeric2(max)
}

[test]
def test_float_and_double(t : T?) {
    fake_numeric(sin)
    fake_numeric(cos)
    fake_numeric(tan)
    fake_numeric(atan)
    fake_numeric(asin)
    fake_numeric(acos)
    fake_numeric2(atan2)
    fake_numeric(exp)
    fake_numeric(log)
    fake_numeric(exp2)
    fake_numeric(log2)
    fake_numeric(rcp)
    fake_numeric2(pow)
    fake_numeric(sqrt)
    fake_numeric(abs)
    fake_numeric(sign)

    fake_float(ceil)
    fake_float(floor)
    fake_float(saturate)

    t |> run("fuzz is_finit") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(is_finite(fake |> random_float))
        }
        fuzz() {
            t |> accept(is_finite(fake |> random_double))
        }
    }

    t |> run("fuzz is_nan") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(is_nan(fake |> random_float))
        }
        fuzz() {
            t |> accept(is_nan(fake |> random_double))
        }
    }

    t |> run("fuzz sincos") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            var s, c : float
            sincos(fake |> random_float, s, c)
        }
        fuzz() {
            var s, c : double
            sincos(fake |> random_double, s, c)
        }
    }
}

[test]
def test_float_only(t : T?) {
    fake_float(atan_est)
    fake_float2(atan2_est)
    fake_float(rcp_est)
    fake_float(rsqrt)
    fake_float(rsqrt_est)
    fake_float(ceili)
    fake_float(roundi)
    fake_float(trunci)
    fake_float(floori)
}


[test]
def test_float3_only(t : T?) {
    t |> run("fuzz cross") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(cross(fake |> random_float3, fake |> random_float3))
        }
    }

    t |> run("fuzz distance") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(distance(fake |> random_float3, fake |> random_float3))
        }
    }

    t |> run("fuzz distance_sq") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(distance_sq(fake |> random_float3, fake |> random_float3))
        }
    }

    t |> run("fuzz inv_distance") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(inv_distance(fake |> random_float3, fake |> random_float3))
        }
    }

    t |> run("fuzz inv_distance_sq") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(inv_distance_sq(fake |> random_float3, fake |> random_float3))
        }
    }

    t |> run("fuzz reflect") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(reflect(fake |> random_float3, fake |> random_float3))
        }
    }
}

[test]
def test_float234(t : T?) {
    fake_float2_234(dot)
    fake_float_234(fast_normalize)
    fake_float_234(normalize)
    fake_float_234(length)
    fake_float_234(inv_length)
    fake_float_234(length_sq)
    fake_float_234(inv_length_sq)
}

[test]
def test_lerp_mad_clamp(t : T?) {
    fake_float3(lerp)
    fake_float3(mad)
    fake_numeric3(clamp)
}



