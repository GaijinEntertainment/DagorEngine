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

def roughly_equal(t : T?; expected, actual, tolerance : float; msg : string = "") {
    let diff = abs(expected - actual)
    if (diff > tolerance) {
        t |> failure("{msg}expected {expected} +/- {tolerance}, got {actual} (diff={diff})")
    } else {
        t |> success(true)
    }
}

// ─── Envelope Tests ───

[test]
def test_envelope_stages(t : T?) {
    t |> run("delay then attack") <| @(t : T?) {
        var env = SF2Envelope()
        env.delay_sec = 0.001  // ~48 samples at 48kHz
        env.attack_sec = 0.01
        env.hold_sec = 0.0
        env.decay_sec = 0.1
        env.sustain_level = 0.5
        env.release_sec = 0.1
        env.is_amp_env = true
        env.midi_velocity = 127
        sf2_env_start(env, 48000.0)
        // tick through delay
        var level = 0.0
        for (_ in range(100)) {
            level = sf2_env_tick(env, false, 48000.0)
        }
        // should be in attack, level > 0
        t |> success(level > 0.0)
        t |> success(env.stage == SF2EnvStage.env_attack || env.stage == SF2EnvStage.env_hold || env.stage == SF2EnvStage.env_decay)
    }
    t |> run("attack reaches 1.0") <| @(t : T?) {
        var env = SF2Envelope()
        env.delay_sec = 0.0
        env.attack_sec = 0.01
        env.hold_sec = 0.0
        env.decay_sec = 1.0
        env.sustain_level = 0.5
        env.release_sec = 0.1
        env.is_amp_env = false
        sf2_env_start(env, 48000.0)
        var level = 0.0
        for (_ in range(1000)) {
            level = sf2_env_tick(env, false, 48000.0)
        }
        // should have passed through attack, now in decay
        t |> success(env.stage == SF2EnvStage.env_decay || env.stage == SF2EnvStage.env_sustain)
    }
    t |> run("decay reaches sustain level") <| @(t : T?) {
        var env = SF2Envelope()
        env.delay_sec = 0.0
        env.attack_sec = 0.001
        env.hold_sec = 0.0
        env.decay_sec = 0.01
        env.sustain_level = 0.3
        env.release_sec = 0.1
        env.is_amp_env = false
        sf2_env_start(env, 48000.0)
        var level = 0.0
        for (_ in range(2000)) {
            level = sf2_env_tick(env, false, 48000.0)
        }
        t |> roughly_equal(0.3, level, 0.05, "sustain: ")
        t |> success(env.stage == SF2EnvStage.env_sustain)
    }
    t |> run("release drops to zero") <| @(t : T?) {
        var env = SF2Envelope()
        env.delay_sec = 0.0
        env.attack_sec = 0.001
        env.hold_sec = 0.0
        env.decay_sec = 0.001
        env.sustain_level = 1.0
        env.release_sec = 0.01
        env.is_amp_env = true
        env.midi_velocity = 127
        sf2_env_start(env, 48000.0)
        // reach sustain
        for (_ in range(500)) {
            sf2_env_tick(env, false, 48000.0)
        }
        // release
        for (_ in range(2000)) {
            sf2_env_tick(env, true, 48000.0)
        }
        t |> success(env.stage == SF2EnvStage.env_finished)
        t |> success(env.level < 0.001)
    }
    t |> run("instantaneous delay (-32768 timecents)") <| @(t : T?) {
        var env = SF2Envelope()
        env.delay_sec = timecents_to_seconds(-32768)  // 0.0
        env.attack_sec = 0.001
        sf2_env_start(env, 48000.0)
        // should skip delay immediately
        t |> success(env.stage == SF2EnvStage.env_attack)
    }
}

// ─── LFO Tests ───

[test]
def test_lfo(t : T?) {
    t |> run("LFO outputs zero during delay") <| @(t : T?) {
        var lfo = SF2LFO()
        lfo.delay_sec = 1.0
        lfo.freq = 5.0
        let val = sf2_lfo_tick(lfo, 0.01)
        t |> equal(0.0, val)
    }
    t |> run("LFO oscillates after delay") <| @(t : T?) {
        var lfo = SF2LFO()
        lfo.delay_sec = 0.0
        lfo.freq = 10.0
        // tick for a while and check we get values in [-1, 1]
        var min_val = 1.0
        var max_val = -1.0
        for (_ in range(4800)) {  // 0.1 seconds = 1 full cycle at 10Hz
            let val = sf2_lfo_tick(lfo, 1.0 / 48000.0)
            if (val < min_val) { min_val = val; }
            if (val > max_val) { max_val = val; }
        }
        t |> success(min_val < -0.5)
        t |> success(max_val > 0.5)
    }
}

// ─── Voice Creation Tests ───

[test]
def test_voice_pitch(t : T?) {
    var st : FStat
    if (!stat("{SF2_DIR}/FluidR3_GM.sf2", st)) {
        t->skip("FluidR3_GM.sf2 not available")
        return
    }
    var sf2 = sf2_load("{SF2_DIR}/FluidR3_GM.sf2")
    if (sf2 == null) {
        t->skip("FluidR3_GM.sf2 failed to load")
        return
    }
    t |> run("piano C4 on root key has phase_inc near 1.0") <| @(t : T?) {
        let idx = sf2_find_preset(*sf2, 0, 0)
        let zones = sf2_find_zones(*sf2, idx, 60, 100)
        if (length(zones) > 0) {
            let preset = sf2.presets[idx]
            var pzone : SF2Zone const?
            for (pz in preset.zones) {
                if (get_generator(*pz, int(SF2GenOper.instrument), -1) == zones[0].x) {
                    pzone = pz
                    break
                }
            }
            var cc_def : int[128]; sf2_default_cc_values(cc_def)
            var cv : ma_sf2_voice; var as_ = SF2VoiceAttenState()
            if (sf2_create_c_voice(*sf2, zones[0].x, zones[0].y, pzone, preset.global_zone, 60, 100, 0, cc_def, cv, as_)) {
                t |> success(cv.phase_inc > 0.1lf)
                t |> success(cv.phase_inc < 4.0lf)
            }
        }
    }
    t |> run("12 semitones apart on same zone doubles phase_inc") <| @(t : T?) {
        let idx = sf2_find_preset(*sf2, 0, 0)
        if (idx >= 0) {
            let zones_lo = sf2_find_zones(*sf2, idx, 55, 100)
            if (length(zones_lo) > 0) {
                let preset = sf2.presets[idx]
                let z = zones_lo[0]
                var pzone : SF2Zone const?
                for (pz in preset.zones) {
                    if (get_generator(*pz, int(SF2GenOper.instrument), -1) == z.x) {
                        pzone = pz
                        break
                    }
                }
                var cc_def2 : int[128]; sf2_default_cc_values(cc_def2)
                var cv_lo : ma_sf2_voice; var asl = SF2VoiceAttenState()
                var cv_hi : ma_sf2_voice; var ash = SF2VoiceAttenState()
                if (sf2_create_c_voice(*sf2, z.x, z.y, pzone, preset.global_zone, 55, 100, 0, cc_def2, cv_lo, asl) && sf2_create_c_voice(*sf2, z.x, z.y, pzone, preset.global_zone, 67, 100, 0, cc_def2, cv_hi, ash)) {
                    let ratio = cv_hi.phase_inc / cv_lo.phase_inc
                    t |> roughly_equal(2.0, float(ratio), 0.01, "octave ratio: ")
                }
            }
        }
    }
    unsafe { delete sf2; }
}

// ─── Loop Mode Tests ───

def private test_loop_mode_0(t : T?) {
    var sample_data : array<int16>
    sample_data |> resize(1000)
    for (i in range(1000)) {
        sample_data[i] = int16(sin(float(i) * 0.1) * 16000.0)
    }
    var cv : ma_sf2_voice
    unsafe { ma_sf2_voice_init(addr(cv), 48000.0); }
    cv.sample_start = 0
    cv.sample_end = 100
    cv.loop_start = 0
    cv.loop_end = 100
    cv.loop_mode = 0
    cv.phase_inc = 1.0lf
    cv.attenuation = 1.0
    cv.vol_env.stage = 4 // sustain
    cv.vol_env.level = 1.0
    cv.vol_env.sustain_level = 1.0
    cv.vol_env.release_sec = 0.1
    var output : array<float>
    output |> resize(400)
    unsafe { ma_sf2_voice_render(addr(cv), addr(sample_data[0]), length(sample_data), addr(output[0]), 0, 200); }
    t |> success(cv.finished != 0)
}

def private test_loop_mode_1(t : T?) {
    var sample_data : array<int16>
    sample_data |> resize(1000)
    for (i in range(1000)) {
        sample_data[i] = int16(sin(float(i) * 0.1) * 16000.0)
    }
    var cv : ma_sf2_voice
    unsafe { ma_sf2_voice_init(addr(cv), 48000.0); }
    cv.sample_start = 0
    cv.sample_end = 500
    cv.loop_start = 100
    cv.loop_end = 200
    cv.loop_mode = 1
    cv.phase_inc = 1.0lf
    cv.attenuation = 1.0
    cv.vol_env.stage = 4 // sustain
    cv.vol_env.level = 1.0
    cv.vol_env.sustain_level = 1.0
    cv.vol_env.release_sec = 0.1
    var output : array<float>
    output |> resize(1000)
    unsafe { ma_sf2_voice_render(addr(cv), addr(sample_data[0]), length(sample_data), addr(output[0]), 0, 500); }
    t |> success(cv.finished == 0)
    t |> success(cv.position < 200.0lf)
}

[test]
def test_loop_modes(t : T?) {
    t |> run("loop mode 0: voice finishes at sample end") <| @(t : T?) {
        test_loop_mode_0(t)
    }
    t |> run("loop mode 1: voice loops continuously") <| @(t : T?) {
        test_loop_mode_1(t)
    }
}

// ─── C Voice Tests ───

[test]
def test_c_voice_render(t : T?) {
    var st : FStat
    if (!stat("{SF2_DIR}/FluidR3_GM.sf2", st)) {
        t->skip("FluidR3_GM.sf2 not available")
        return
    }
    var sf2 = sf2_load("{SF2_DIR}/FluidR3_GM.sf2")
    if (sf2 == null) {
        t->skip("FluidR3_GM.sf2 failed to load")
        return
    }
    t |> run("C voice produces non-zero output") <| @(t : T?) {
        let idx = sf2_find_preset(*sf2, 0, 0)
        let zones = sf2_find_zones(*sf2, idx, 60, 127)
        if (length(zones) > 0) {
            let preset = sf2.presets[idx]
            var pzone : SF2Zone const?
            for (pz in preset.zones) {
                if (get_generator(*pz, int(SF2GenOper.instrument), -1) == zones[0].x) {
                    pzone = pz
                    break
                }
            }
            var cc3 : int[128]; sf2_default_cc_values(cc3)
            var cv : ma_sf2_voice; var as3 = SF2VoiceAttenState()
            if (sf2_create_c_voice(*sf2, zones[0].x, zones[0].y, pzone, preset.global_zone, 60, 127, 0, cc3, cv, as3)) {
                var output : array<float>
                output |> resize(2000)
                unsafe {
                    ma_sf2_voice_render(addr(cv), addr(sf2.sample_data[0]), length(sf2.sample_data), addr(output[0]), 0, 1000)
                }
                var max_val = 0.0
                for (v in output) {
                    if (abs(v) > max_val) { max_val = abs(v); }
                }
                t |> success(max_val > 0.01)
            }
        }
    }
    t |> run("C voice note-off triggers release and finishes") <| @(t : T?) {
        let idx = sf2_find_preset(*sf2, 0, 0)
        let zones = sf2_find_zones(*sf2, idx, 60, 127)
        if (length(zones) > 0) {
            let preset = sf2.presets[idx]
            var pzone : SF2Zone const?
            for (pz in preset.zones) {
                if (get_generator(*pz, int(SF2GenOper.instrument), -1) == zones[0].x) {
                    pzone = pz
                    break
                }
            }
            var cc4 : int[128]; sf2_default_cc_values(cc4)
            var cv : ma_sf2_voice; var as4 = SF2VoiceAttenState()
            if (sf2_create_c_voice(*sf2, zones[0].x, zones[0].y, pzone, preset.global_zone, 60, 127, 0, cc4, cv, as4)) {
                var output : array<float>
                output |> resize(192000 * 2)  // 4 seconds
                // render 1 second, then note-off, render 3 more seconds
                unsafe {
                    ma_sf2_voice_render(addr(cv), addr(sf2.sample_data[0]), length(sf2.sample_data), addr(output[0]), 0, 48000)
                    ma_sf2_voice_note_off(addr(cv))
                    ma_sf2_voice_render(addr(cv), addr(sf2.sample_data[0]), length(sf2.sample_data), addr(output[0]), 48000 * 2, 144000)
                }
                t |> success(cv.finished != 0)
            }
        }
    }
    t |> run("C voice end_quick kills fast") <| @(t : T?) {
        let idx = sf2_find_preset(*sf2, 128, 0)  // drums
        let zones = sf2_find_zones(*sf2, idx, 36, 127)
        if (length(zones) > 0) {
            let preset = sf2.presets[idx]
            var pzone : SF2Zone const?
            for (pz in preset.zones) {
                if (get_generator(*pz, int(SF2GenOper.instrument), -1) == zones[0].x) {
                    pzone = pz
                    break
                }
            }
            var cc5 : int[128]; sf2_default_cc_values(cc5)
            var cv : ma_sf2_voice; var as5 = SF2VoiceAttenState()
            if (sf2_create_c_voice(*sf2, zones[0].x, zones[0].y, pzone, preset.global_zone, 36, 127, 9, cc5, cv, as5)) {
                unsafe { ma_sf2_voice_end_quick(addr(cv)); }
                var output : array<float>
                output |> resize(96000)  // 1 second
                unsafe {
                    ma_sf2_voice_render(addr(cv), addr(sf2.sample_data[0]), length(sf2.sample_data), addr(output[0]), 0, 48000)
                }
                // should be finished after 10ms quick release + render
                t |> success(cv.finished != 0)
            }
        }
    }
    unsafe { delete sf2; }
}

// ─── Pan Tests ───

[test]
def test_pan(t : T?) {
    t |> run("pan -500 = full left") <| @(t : T?) {
        let pan = clamp(-500.0 / 500.0, -1.0, 1.0)
        let pan05 = pan * 0.5
        t |> roughly_equal(1.0, sqrt(0.5 - pan05), 0.001, "left gain: ")
        t |> roughly_equal(0.0, sqrt(0.5 + pan05), 0.001, "right gain: ")
    }
    t |> run("pan 0 = center, equal power") <| @(t : T?) {
        let pan05 = 0.0
        let l = sqrt(0.5 - pan05)
        let r = sqrt(0.5 + pan05)
        t |> roughly_equal(l, r, 0.001, "center balance: ")
        t |> roughly_equal(0.707, l, 0.01, "center gain: ")
    }
    t |> run("pan +500 = full right") <| @(t : T?) {
        let pan = clamp(500.0 / 500.0, -1.0, 1.0)
        let pan05 = pan * 0.5
        t |> roughly_equal(0.0, sqrt(0.5 - pan05), 0.001, "left gain: ")
        t |> roughly_equal(1.0, sqrt(0.5 + pan05), 0.001, "right gain: ")
    }
}
