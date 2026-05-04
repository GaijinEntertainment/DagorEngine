options gen2
options indenting = 4

require dastest/testing_boost
require math

[test]
def test_floor_double(t : T?) {
    t |> equal(floor(3.7lf), 3.0lf)
    t |> equal(floor(-1.2lf), -2.0lf)
    t |> equal(floor(0.0lf), 0.0lf)
    t |> equal(floor(5.0lf), 5.0lf)
}

[test]
def test_ceil_double(t : T?) {
    t |> equal(ceil(3.2lf), 4.0lf)
    t |> equal(ceil(-1.7lf), -1.0lf)
    t |> equal(ceil(0.0lf), 0.0lf)
    t |> equal(ceil(5.0lf), 5.0lf)
}

[test]
def test_round_double(t : T?) {
    t |> equal(round(3.4lf), 3.0lf)
    t |> equal(round(3.5lf), 4.0lf)
    t |> equal(round(-1.5lf), -2.0lf)
    t |> equal(round(0.0lf), 0.0lf)
}

[test]
def test_fract_double(t : T?) {
    t |> success(abs(fract(3.75lf) - 0.75lf) < 0.0001lf)
    t |> success(abs(fract(0.25lf) - 0.25lf) < 0.0001lf)
    t |> equal(fract(5.0lf), 0.0lf)
}

[test]
def test_saturate_double(t : T?) {
    t |> equal(saturate(0.5lf), 0.5lf)
    t |> equal(saturate(-1.0lf), 0.0lf)
    t |> equal(saturate(2.0lf), 1.0lf)
    t |> equal(saturate(0.0lf), 0.0lf)
    t |> equal(saturate(1.0lf), 1.0lf)
}

[test]
def test_min_max_double(t : T?) {
    t |> equal(min(3.0lf, 5.0lf), 3.0lf)
    t |> equal(max(3.0lf, 5.0lf), 5.0lf)
    t |> equal(min(-1.0lf, 1.0lf), -1.0lf)
    t |> equal(max(-1.0lf, 1.0lf), 1.0lf)
}

[test]
def test_clamp_double(t : T?) {
    t |> equal(clamp(5.0lf, 0.0lf, 1.0lf), 1.0lf)
    t |> equal(clamp(-1.0lf, 0.0lf, 1.0lf), 0.0lf)
    t |> equal(clamp(0.5lf, 0.0lf, 1.0lf), 0.5lf)
}

[test]
def test_abs_double(t : T?) {
    t |> equal(abs(-3.5lf), 3.5lf)
    t |> equal(abs(3.5lf), 3.5lf)
    t |> equal(abs(0.0lf), 0.0lf)
}

[test]
def test_sign_double(t : T?) {
    t |> equal(sign(5.0lf), 1.0lf)
    t |> equal(sign(-5.0lf), -1.0lf)
    t |> equal(sign(0.0lf), 0.0lf)
}

[test]
def test_sqrt_double(t : T?) {
    t |> success(abs(sqrt(4.0lf) - 2.0lf) < 0.0001lf)
    t |> success(abs(sqrt(2.0lf) - 1.41421356lf) < 0.0001lf)
    t |> equal(sqrt(0.0lf), 0.0lf)
}

[test]
def test_large_cycle_precision(t : T?) {
    // at cycle 16 million, double should still have sub-cycle precision
    let big = 16777216.0lf
    t |> success(floor(big + 0.25lf) == big)
    t |> success(fract(big + 0.75lf) - 0.75lf < 0.0001lf)
}
