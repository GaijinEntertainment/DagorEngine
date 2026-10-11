options gen2
require dastest/testing_boost
require math
require UnitTest

[test]
def test_scalar_crt_twins(t : T?) {
    t |> run("switched scalar builtins are the CRT exactly") @@(t : T?) {
        // windows x86 keeps the vecmath exp, exp2, log2 and pow (sim_policy.h)
        if (get_platform_name() == "windows" && get_architecture_name() == "x86") {
            return
        }
        var y = 0.05f
        while (y < 20.) {
            t |> equal(exp(y), crt_expf(y), "exp({y})")
            t |> equal(exp2(y), crt_exp2f(y), "exp2({y})")
            t |> equal(log2(y), crt_log2f(y), "log2({y})")
            t |> equal(pow(y, 1.5f), crt_powf(y, 1.5f), "pow({y}, 1.5)")
            y += 0.37f
        }
    }
    t |> run("the measuring twins agree with the double CRT") @@(t : T?) {
        var y = 0.05f
        while (y < 20.) {
            t |> success(abs(crt_logf(y) - float(log(double(y)))) <= 4e-7f * max(1.0f, abs(crt_logf(y))), "crt_logf({y}) = {crt_logf(y)}")
            y += 0.37f
        }
        var z = -1.4f
        while (z < 1.4) {
            t |> success(abs(crt_sinf(z) - float(sin(double(z)))) <= 4e-7f, "crt_sinf({z}) = {crt_sinf(z)}")
            t |> success(abs(crt_cosf(z) - float(cos(double(z)))) <= 4e-7f, "crt_cosf({z}) = {crt_cosf(z)}")
            t |> success(abs(crt_tanf(z) - float(tan(double(z)))) <= 2e-6f * max(1.0f, abs(crt_tanf(z))), "crt_tanf({z}) = {crt_tanf(z)}")
            z += 0.11f
        }
    }
}
