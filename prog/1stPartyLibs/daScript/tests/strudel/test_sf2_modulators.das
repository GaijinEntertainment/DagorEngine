options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require strudel/strudel_sf2
require strudel/strudel_sf2_voice
require audio
require dastest/testing_boost
require daslib/fio
require math

let SF2_DIR = "{get_das_root()}/examples/daStrudel/midi_sf2_demo"

def try_load_sf2(path : string; t : T?) : SF2File const? {
    var st : FStat
    if (!stat(path, st)) {
        t->skip("{path} not available")
        return null
    }
    let sf2 = sf2_load(path)
    if (sf2 == null) {
        t->skip("failed to load {path}")
    }
    return sf2
}

def roughly_equal(t : T?; expected, actual, tolerance : float; msg : string = "") {
    let diff = abs(expected - actual)
    if (diff > tolerance) {
        t |> failure("{msg}expected {expected} +/- {tolerance}, got {actual} (diff={diff})")
    } else {
        t |> success(true)
    }
}

// ─── Modulator Source Unpacking ───

[test]
def test_mod_source_unpacking(t : T?) {
    t |> run("CC flag") @(t : T?) {
        // CC1 (mod wheel), positive, unipolar, linear = 0x0001 | 0x0080 = 0x0081 = 129
        let src = int(0x81)
        t |> equal(sf2_mod_src_index(src), 1)
        t |> success(sf2_mod_src_cc(src))
        t |> success(!sf2_mod_src_negative(src))
        t |> success(!sf2_mod_src_bipolar(src))
        t |> equal(sf2_mod_src_type(src), 0)  // linear
    }
    t |> run("general controller velocity, concave, negative, unipolar") @(t : T?) {
        // velocity=2, GC=0, concave=1<<10, negative=1<<8, unipolar=0
        // 0x0002 | 0x0100 | 0x0400 = 0x0502
        let src = int(0x502)
        t |> equal(sf2_mod_src_index(src), 2)  // velocity
        t |> success(!sf2_mod_src_cc(src))
        t |> success(sf2_mod_src_negative(src))
        t |> success(!sf2_mod_src_bipolar(src))
        t |> equal(sf2_mod_src_type(src), 1)  // concave
    }
    t |> run("pitch wheel, bipolar, positive, linear") @(t : T?) {
        // pitchWheel=14, GC=0, bipolar=0x200, positive=0, linear=0
        let src = int(0x20E)
        t |> equal(sf2_mod_src_index(src), 14)  // pitch wheel
        t |> success(!sf2_mod_src_cc(src))
        t |> success(!sf2_mod_src_negative(src))
        t |> success(sf2_mod_src_bipolar(src))
        t |> equal(sf2_mod_src_type(src), 0)  // linear
    }
    t |> run("NONE source") @(t : T?) {
        let src = 0
        t |> equal(sf2_mod_src_index(src), 0)
        t |> success(!sf2_mod_src_cc(src))
    }
}

// ─── Modulator Evaluation ───

[test]
def test_mod_evaluation(t : T?) {
    var cc_values : int[128]
    cc_values[7] = 127   // volume
    cc_values[10] = 64   // pan
    cc_values[11] = 127  // expression
    cc_values[1] = 0     // mod wheel

    t |> run("NONE src always produces amount") @(t : T?) {
        // src1=NONE (0), src2=NONE (0), amount=100
        let m = SF2Modulator(src_oper = 0, amt_src_oper = 0, dest_oper = 0, amount = 100, transform = 0)
        let val = sf2_mod_get_value(m, 127, 60, cc_values)
        t |> roughly_equal(100.0, val, 0.1)
    }
    t |> run("velocity src, linear positive unipolar, full velocity") @(t : T?) {
        // src1 = velocity(2), GC, linear, positive, unipolar = 0x0002
        // src2 = NONE
        let m = SF2Modulator(src_oper = 2, amt_src_oper = 0, dest_oper = 0, amount = 960, transform = 0)
        let val = sf2_mod_get_value(m, 127, 60, cc_values)
        // velocity 127 / range 128 = 0.9921875, * 1.0 (NONE) * 960 = ~952.5
        t |> roughly_equal(952.5, val, 1.0)
    }
    t |> run("velocity src, linear positive unipolar, low velocity") @(t : T?) {
        let m = SF2Modulator(src_oper = 2, amt_src_oper = 0, dest_oper = 0, amount = 960, transform = 0)
        let val = sf2_mod_get_value(m, 32, 60, cc_values)
        // velocity 32 / 128 = 0.25, * 960 = 240
        t |> roughly_equal(240.0, val, 1.0)
    }
    t |> run("CC7 volume, concave negative unipolar (default modulator #5)") @(t : T?) {
        // CC7, CC flag=0x80, concave=0x400, negative=0x100, unipolar=0
        // src_oper = 7 | 0x80 | 0x100 | 0x400 = 0x587
        let m = SF2Modulator(src_oper = int(0x587), amt_src_oper = 0, dest_oper = int(SF2GenOper.initialAttenuation), amount = 960, transform = 0)
        // CC7=127: negative unipolar → inv_norm = 1 - 1/128 - 127/128 = 0
        // concave(0) = 0 → result = 960 * 0 * 1 = 0
        let val_max = sf2_mod_get_value(m, 127, 60, cc_values)
        t |> roughly_equal(0.0, val_max, 1.0, "CC7=127: ")
        // CC7=0: inv_norm = 1 - 1/128 - 0 ≈ 0.992
        // concave(128*0.992) ≈ concave(127) = 1.0 → result ≈ 960
        var cc0 : int[128]
        cc0[7] = 0
        let val_min = sf2_mod_get_value(m, 127, 60, cc0)
        t |> roughly_equal(960.0, val_min, 10.0, "CC7=0: ")
    }
    t |> run("absolute value transform") @(t : T?) {
        // bipolar source can produce negative, abs transform makes it positive
        // src = velocity, bipolar(0x200), positive, linear
        let m = SF2Modulator(src_oper = int(0x202), amt_src_oper = 0, dest_oper = 0, amount = -100, transform = 2)
        let val = sf2_mod_get_value(m, 64, 60, cc_values)
        t |> success(val >= 0.0)
    }
    t |> run("two-source modulator (pitch bend * sensitivity)") @(t : T?) {
        // src1 = pitchWheel(14), GC, bipolar(0x200), positive, linear = 0x20E
        // src2 = pitchWheelSens(16), GC, unipolar, positive, linear = 0x0010
        let m = SF2Modulator(src_oper = int(0x20E), amt_src_oper = int(0x10), dest_oper = 0, amount = 12700, transform = 0)
        // pitchWheel=0 (center) → bipolar → 0 (or near 0), result ≈ 0
        let val_center = sf2_mod_get_value(m, 127, 60, cc_values)
        t |> roughly_equal(0.0, val_center, 200.0, "center: ")
    }
}

// ─── Parse Count Tests ───

[test]
def test_modulator_parse_counts(t : T?) {
    let sf2 = try_load_sf2("{SF2_DIR}/FluidR3_GM.sf2", t)
    if (sf2 == null) {
        return
    }
    t |> run("FluidR3 has 746 instrument modulators") @(t : T?) {
        var total_imods = 0
        for (inst in sf2.instruments) {
            if (inst.global_zone != null) {
                total_imods += length(inst.global_zone.modulators)
            }
            for (z in inst.zones) {
                total_imods += length(z.modulators)
            }
        }
        t |> equal(total_imods, 746)
    }
    t |> run("FluidR3 has 0 preset modulators") @(t : T?) {
        var total_pmods = 0
        for (p in sf2.presets) {
            if (p.global_zone != null) {
                total_pmods += length(p.global_zone.modulators)
            }
            for (z in p.zones) {
                total_pmods += length(z.modulators)
            }
        }
        t |> equal(total_pmods, 0)
    }
}

// ─── Modulator Effect on Voice ───

[test]
def test_modulators_affect_voice(t : T?) {
    let sf2 = try_load_sf2("{SF2_DIR}/FluidR3_GM.sf2", t)
    if (sf2 == null) { return ; }
    t |> run("modulators change voice attenuation") @(t : T?) {
        // find piano preset
        let idx = sf2_find_preset(*sf2, 0, 0)
        if (idx < 0) {
            t->skip("no piano preset")
            return
        }
        let zones = sf2_find_zones(*sf2, idx, 60, 127)
        if (length(zones) == 0) {
            t->skip("no zones for piano C4")
            return
        }
        let z = zones[0]
        let preset = sf2.presets[idx]
        var pzone : SF2Zone const?
        for (pz in preset.zones) {
            let inst_gen = get_generator(*pz, int(SF2GenOper.instrument), -1)
            if (inst_gen == z.x) {
                pzone = pz
                break
            }
        }
        // create voice at high velocity
        var voice_loud : ma_sf2_voice
        var cc_defaults_1 : int[128]; sf2_default_cc_values(cc_defaults_1); var as_1 = SF2VoiceAttenState()
        sf2_create_c_voice(*sf2, z.x, z.y, pzone, preset.global_zone, 60, 127, 0, cc_defaults_1, voice_loud, as_1)
        // create voice at low velocity
        var voice_soft : ma_sf2_voice
        var as_2 = SF2VoiceAttenState()
        sf2_create_c_voice(*sf2, z.x, z.y, pzone, preset.global_zone, 60, 30, 0, cc_defaults_1, voice_soft, as_2)
        // with 2445 modulators, velocity should significantly affect attenuation
        t |> success(voice_loud.attenuation > voice_soft.attenuation)
        if (voice_loud.attenuation > 0.001 && voice_soft.attenuation > 0.001) {
            let ratio = voice_loud.attenuation / voice_soft.attenuation
            // loud should be notably louder (higher attenuation = more signal)
            t |> success(ratio > 1.5)
        }
    }
}

// ─── Default Modulator Tests ───

[test]
def test_default_modulators(t : T?) {
    let sf2 = try_load_sf2("{SF2_DIR}/FluidR3_GM.sf2", t)
    if (sf2 == null) { return ; }
    t |> run("default velocity→attenuation produces dynamics without hardcoded curve") @(t : T?) {
        let idx = sf2_find_preset(*sf2, 0, 0)
        if (idx < 0) { t->skip("no piano"); return ; }
        let zones = sf2_find_zones(*sf2, idx, 60, 100)
        if (length(zones) == 0) { t->skip("no zones"); return ; }
        let z = zones[0]
        let preset = sf2.presets[idx]
        var pzone : SF2Zone const?
        for (pz in preset.zones) {
            if (get_generator(*pz, int(SF2GenOper.instrument), -1) == z.x) {
                pzone = pz
                break
            }
        }
        var cc_def : int[128]; sf2_default_cc_values(cc_def)
        var v_loud : ma_sf2_voice; var asl = SF2VoiceAttenState()
        sf2_create_c_voice(*sf2, z.x, z.y, pzone, preset.global_zone, 60, 127, 0, cc_def, v_loud, asl)
        var v_mid : ma_sf2_voice; var asm_ = SF2VoiceAttenState()
        sf2_create_c_voice(*sf2, z.x, z.y, pzone, preset.global_zone, 60, 64, 0, cc_def, v_mid, asm_)
        var v_soft : ma_sf2_voice; var ass_ = SF2VoiceAttenState()
        sf2_create_c_voice(*sf2, z.x, z.y, pzone, preset.global_zone, 60, 20, 0, cc_def, v_soft, ass_)
        // velocity 127 > 64 > 20 in attenuation
        t |> success(v_loud.attenuation > v_mid.attenuation)
        t |> success(v_mid.attenuation > v_soft.attenuation)
        // the range should be significant (concave curve over 960 cB)
        if (v_soft.attenuation > 0.0001) {
            let dynamic_range = v_loud.attenuation / v_soft.attenuation
            t |> success(dynamic_range > 5.0)
        }
    }
    t |> run("default modulator overridden by zone modulator") @(t : T?) {
        // If a zone has a modulator with same identity as a default,
        // the default should NOT be applied (zone takes precedence).
        var cc_values : int[128]
        cc_values[7] = 127   // volume max → 0 attenuation from default #5
        cc_values[11] = 127  // expression max → 0 attenuation from default #7
        var gen_offsets : float[64]
        // Create a zone with a custom vel→atten modulator (amount=0 = disable)
        var zone = SF2Zone()
        zone.modulators |> push(SF2Modulator(
            src_oper = int(0x0502),  // same as default #1: velocity, concave, negative, unipolar
            amt_src_oper = 0,
            dest_oper = int(SF2GenOper.initialAttenuation),
            amount = 0,  // zone disables it
            transform = 0
        ))
        sf2_apply_modulators(zone, 127, 60, cc_values, gen_offsets)
        sf2_apply_default_modulators(zone, null, null, null, 127, 60, cc_values, gen_offsets)
        // Default #1 should be overridden (amount=0), so attenuation offset should be ~0
        // (only the zone's amount=0 contributes, not the default's 960)
        t |> roughly_equal(0.0, gen_offsets[int(SF2GenOper.initialAttenuation)], 1.0)
    }
}
