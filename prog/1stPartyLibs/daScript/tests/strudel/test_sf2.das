options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require strudel/strudel_sf2
require dastest/testing_boost
require daslib/fio
require math
require strings

let SF2_DIR = "{get_das_root()}/examples/daStrudel/midi_sf2_demo"

def roughly_equal(t : T?; expected, actual, tolerance : float) {
    let diff = abs(expected - actual)
    if (diff > tolerance) {
        t |> failure("expected {expected} +/- {tolerance}, got {actual} (diff={diff})")
    } else {
        t |> success(true)
    }
}

[test]
def test_timecent_conversion(t : T?) {
    t |> run("zero timecents = 1 second") <| @(t : T?) {
        t |> equal(1.0, timecents_to_seconds(0))
    }
    t |> run("1200 timecents = 2 seconds") <| @(t : T?) {
        t |> roughly_equal(2.0, timecents_to_seconds(1200), 0.001)
    }
    t |> run("-1200 timecents = 0.5 seconds") <| @(t : T?) {
        t |> roughly_equal(0.5, timecents_to_seconds(-1200), 0.001)
    }
    t |> run("-32768 timecents = instantaneous") <| @(t : T?) {
        t |> equal(0.0, timecents_to_seconds(-32768))
    }
}

[test]
def test_cents_to_hz(t : T?) {
    t |> run("6900 cents = 440 Hz (A4)") <| @(t : T?) {
        t |> roughly_equal(440.0, cents_to_hz(6900), 1.0)
    }
    t |> run("0 cents = 8.176 Hz") <| @(t : T?) {
        t |> roughly_equal(8.176, cents_to_hz(0), 0.01)
    }
}

[test]
def test_centibels_to_linear(t : T?) {
    t |> run("0 cb = 1.0") <| @(t : T?) {
        t |> equal(1.0, centibels_to_linear(0))
    }
    t |> run("200 cb ~ 0.1") <| @(t : T?) {
        t |> roughly_equal(0.1, centibels_to_linear(200), 0.001)
    }
}

[test]
def test_load_fluidr3(t : T?) {
    var st : FStat
    if (!stat("{SF2_DIR}/FluidR3_GM.sf2", st)) {
        t->skip("FluidR3_GM.sf2 not available")
        return
    }
    var sf2 = sf2_load("{SF2_DIR}/FluidR3_GM.sf2")
    if (sf2 == null) {
        t->skip("FluidR3_GM.sf2 not available")
        return
    }
    t |> run("has presets") <| @(t : T?) {
        t |> success(length(sf2.presets) >= 128)
    }
    t |> run("has instruments") <| @(t : T?) {
        t |> success(length(sf2.instruments) > 0)
    }
    t |> run("has samples") <| @(t : T?) {
        t |> success(length(sf2.samples) > 0)
    }
    t |> run("has sample data") <| @(t : T?) {
        t |> success(length(sf2.sample_data) > 1000000)
    }
    t |> run("preset 0 is piano") <| @(t : T?) {
        let idx = sf2_find_preset(*sf2, 0, 0)
        t |> success(idx >= 0)
        if (idx >= 0) {
            let name = sf2.presets[idx].name
            t |> success(!empty(name))
        }
    }
    t |> run("preset lookup for guitar (bank 0 program 25)") <| @(t : T?) {
        let idx = sf2_find_preset(*sf2, 0, 25)
        t |> success(idx >= 0)
    }
    t |> run("drum bank 128 exists") <| @(t : T?) {
        let idx = sf2_find_preset(*sf2, 128, 0)
        t |> success(idx >= 0)
    }
    t |> run("zone lookup for piano C4") <| @(t : T?) {
        let idx = sf2_find_preset(*sf2, 0, 0)
        if (idx >= 0) {
            let zones = sf2_find_zones(*sf2, idx, 60, 127)
            t |> success(length(zones) > 0)
        }
    }
    t |> run("zone lookup for drum kick") <| @(t : T?) {
        let idx = sf2_find_preset(*sf2, 128, 0)
        if (idx >= 0) {
            let zones = sf2_find_zones(*sf2, idx, 36, 127)
            t |> success(length(zones) > 0)
        }
    }
    t |> run("sample headers have valid data") <| @(t : T?) {
        if (length(sf2.samples) > 0) {
            let s = sf2.samples[0]
            t |> success(s.sample_rate > 0u)
            t |> success(s.end > s.start)
            t |> success(!empty(s.name))
        }
    }
    unsafe { delete sf2; }
}
