options gen2
options indenting = 4

require dastest/testing_boost
require strudel/strudel
require strudel/strudel_synth
require strudel/strudel_scheduler
require strudel/strudel_samples
require math

// ─── Setter propagation (bare aliases + scalar + Pattern) ───

[test]
def test_crush_alias(t : T?) {
    let pat <- atom("bd") |> crush(8.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].value.crush - 8.0) < 0.001)
}

[test]
def test_coarse_alias(t : T?) {
    let pat <- atom("bd") |> coarse(4)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(haps[0].value.coarse, 4)
}

[test]
def test_shape_alias(t : T?) {
    let pat <- atom("bd") |> shape(0.5)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.shape - 0.5) < 0.001)
}

[test]
def test_djf_alias(t : T?) {
    let pat <- atom("bd") |> djf(0.2)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.djf - 0.2) < 0.001)
}

[test]
def test_bpf_alias(t : T?) {
    let pat <- atom("bd") |> bpf(1200.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.bpf - 1200.0) < 0.001)
}

[test]
def test_bpq_alias(t : T?) {
    let pat <- atom("bd") |> bpq(5.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.bpq - 5.0) < 0.001)
}

[test]
def test_effects_chain(t : T?) {
    // all six setters stack onto one pattern
    let pat <- atom("bd") |> crush(10.0) |> coarse(2) |> shape(0.3) |> djf(0.7) |> bpf(800.0) |> bpq(3.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    let e = haps[0].value
    t |> success(abs(e.crush - 10.0) < 0.001)
    t |> equal(e.coarse, 2)
    t |> success(abs(e.shape - 0.3) < 0.001)
    t |> success(abs(e.djf - 0.7) < 0.001)
    t |> success(abs(e.bpf - 800.0) < 0.001)
    t |> success(abs(e.bpq - 3.0) < 0.001)
}

[test]
def test_crush_pattern_modulator(t : T?) {
    // Pattern-valued modulator: a signal sampled at the event's onset
    let pat <- atom("bd") |> crush(signal_saw() |> signal_range(4.0, 12.0))
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(length(haps) >= 1)
    // signal_saw at t=0 → 0 → mapped to 4.0
    t |> success(abs(haps[0].value.crush - 4.0) < 0.1, "crush sampled from modulator = {haps[0].value.crush}")
}

// ─── VoiceFX behavior ───

[test]
def test_voicefx_default_is_inactive(t : T?) {
    var fx : VoiceFX
    let e = Event()
    fx_init_from_event(fx, e, 48000.0)
    t |> success(!fx.active, "default event has no active FX")
    t |> success(!fx.has_crush)
    t |> success(!fx.has_shape)
    t |> success(!fx.has_djf)
    t |> success(!fx.has_bpf)
}

[test]
def test_voicefx_crush_activates(t : T?) {
    var fx : VoiceFX
    var e = Event()
    e.crush = 8.0
    fx_init_from_event(fx, e, 48000.0)
    t |> success(fx.active)
    t |> success(fx.has_crush)
    t |> success(!fx.has_shape)
}

[test]
def test_voicefx_coarse_alone_activates(t : T?) {
    var fx : VoiceFX
    var e = Event()
    e.coarse = 4
    fx_init_from_event(fx, e, 48000.0)
    t |> success(fx.has_crush, "coarse alone routes through the bitcrusher")
    t |> success(fx.active)
}

[test]
def test_voicefx_djf_bypass_when_half(t : T?) {
    var fx : VoiceFX
    let e = Event()
    // djf default is 0.5 (bypass) — should not activate
    fx_init_from_event(fx, e, 48000.0)
    t |> success(!fx.has_djf, "djf at 0.5 is bypass — FX should not activate")
}

[test]
def test_voicefx_djf_offcenter_activates(t : T?) {
    var fx : VoiceFX
    var e = Event()
    e.djf = 0.2
    fx_init_from_event(fx, e, 48000.0)
    t |> success(fx.has_djf)
    t |> success(fx.active)
}

[test]
def test_voicefx_bpf_activates(t : T?) {
    var fx : VoiceFX
    var e = Event()
    e.bpf = 1000.0
    e.bpq = 2.0
    fx_init_from_event(fx, e, 48000.0)
    t |> success(fx.has_bpf)
    t |> success(fx.active)
}

[test]
def test_fx_apply_bypass_is_noop(t : T?) {
    var fx : VoiceFX
    let e = Event()
    fx_init_from_event(fx, e, 48000.0)
    var buf : array<float>
    buf |> resize(64)
    for (i in range(64)) {
        buf[i] = float(i) * 0.01
    }
    var orig : array<float>
    orig |> resize(64)
    for (i in range(64)) {
        orig[i] = buf[i]
    }
    fx_apply(fx, buf, 32)
    for (i in range(64)) {
        t |> success(abs(buf[i] - orig[i]) < 1.0e-6, "buf[{i}] unchanged when FX inactive")
    }
    delete orig
    delete buf
}

[test]
def test_fx_apply_crush_quantizes(t : T?) {
    var fx : VoiceFX
    var e = Event()
    e.crush = 2.0   // very aggressive — 4 levels total (2^2=4)
    fx_init_from_event(fx, e, 48000.0)
    var buf : array<float>
    buf |> resize(64)
    for (i in range(32)) {
        buf[i * 2]     = 0.37
        buf[i * 2 + 1] = -0.63
    }
    fx_apply(fx, buf, 32)
    // 2-bit quantization with round-to-nearest should collapse many samples to the same level
    var unique_vals : array<float>
    for (i in range(64)) {
        var found = false
        for (u in unique_vals) {
            if (abs(u - buf[i]) < 1.0e-4) {
                found = true
                break
            }
        }
        if (!found) {
            unique_vals |> push(buf[i])
        }
    }
    t |> success(length(unique_vals) <= 8, "2-bit crush collapses to a small number of levels, got {length(unique_vals)}")
    delete unique_vals
    delete buf
}

[test]
def test_fx_apply_shape_clamps(t : T?) {
    var fx : VoiceFX
    var e = Event()
    e.shape = 5.0   // strong drive
    fx_init_from_event(fx, e, 48000.0)
    var buf : array<float>
    buf |> resize(64)
    for (i in range(32)) {
        buf[i * 2]     = 10.0    // extreme input
        buf[i * 2 + 1] = -10.0
    }
    fx_apply(fx, buf, 32)
    for (i in range(64)) {
        t |> success(abs(buf[i]) <= 1.01, "waveshaper output bounded |{buf[i]}|")
    }
    delete buf
}

// ─── Scheduler integration: effects-on-osc-voice renders differently from dry ───

def private tick_chunks(var sched : Scheduler; var pat : Pattern; bank : SampleBank; numChunks : int; chunkSec : float = 0.05) : array<float> {
    let chunkFrames = int(chunkSec * float(SAMPLE_RATE))
    var result : array<float>
    result |> reserve(numChunks * chunkFrames * 2)
    for (c in range(numChunks)) {
        let wall = double(c) * double(chunkSec)
        tick(sched, pat, bank, wall, chunkSec)
        for (s in sched.output) {
            result |> push(s)
        }
    }
    return <- result
}

def private rms(samples : array<float>) : float {
    var sum = 0.0
    for (s in samples) {
        sum += s * s
    }
    if (length(samples) == 0) {
        return 0.0
    }
    return sqrt(sum / float(length(samples)))
}

def private diff_rms(a, b : array<float>) : float {
    let n = min(length(a), length(b))
    var sum = 0.0
    for (i in range(n)) {
        let d = a[i] - b[i]
        sum += d * d
    }
    if (n == 0) {
        return 0.0
    }
    return sqrt(sum / float(n))
}

[test]
def test_scheduler_osc_crush_differs_from_dry(t : T?) {
    t |> run("crush applied to osc voice changes output") @(t : T?) {
        var inscope dry_sched : Scheduler
        dry_sched.cps = 1.0lf
        var inscope wet_sched : Scheduler
        wet_sched.cps = 1.0lf
        let bank : SampleBank
        let dry_pat <- atom("sawtooth") |> note(60.0)
        let wet_pat <- atom("sawtooth") |> note(60.0) |> crush(3.0)
        var dry <- tick_chunks(dry_sched, dry_pat, bank, 4)
        var wet <- tick_chunks(wet_sched, wet_pat, bank, 4)
        let dry_rms = rms(dry)
        let wet_rms = rms(wet)
        let delta = diff_rms(dry, wet)
        t |> success(dry_rms > 1.0e-4, "dry output has signal (rms={dry_rms})")
        t |> success(wet_rms > 1.0e-4, "wet output has signal (rms={wet_rms})")
        t |> success(delta > 1.0e-4, "crush produces audibly different output (delta rms={delta})")
        delete dry
        delete wet
    }
}

[test]
def test_scheduler_osc_bpf_reduces_rms(t : T?) {
    t |> run("narrow bandpass reduces overall energy") @(t : T?) {
        var inscope dry_sched : Scheduler
        dry_sched.cps = 1.0lf
        var inscope wet_sched : Scheduler
        wet_sched.cps = 1.0lf
        let bank : SampleBank
        let dry_pat <- atom("sawtooth") |> note(60.0)
        // Narrow BPF around a frequency unrelated to the note's partials
        let wet_pat <- atom("sawtooth") |> note(60.0) |> bpf(4000.0) |> bpq(8.0)
        var dry <- tick_chunks(dry_sched, dry_pat, bank, 4)
        var wet <- tick_chunks(wet_sched, wet_pat, bank, 4)
        let dry_rms = rms(dry)
        let wet_rms = rms(wet)
        t |> success(dry_rms > 1.0e-4, "dry has energy")
        t |> success(wet_rms < dry_rms, "bpf reduces total energy (dry={dry_rms} wet={wet_rms})")
        delete dry
        delete wet
    }
}

[test]
def test_scheduler_osc_voicefx_marked_active(t : T?) {
    t |> run("osc voice gets fx.active when event has effect params") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 1.0lf
        let bank : SampleBank
        let pat <- atom("sawtooth") |> note(60.0) |> crush(6.0) |> bpf(1000.0)
        tick(sched, pat, bank, 0.0lf, 0.01)
        t |> success(length(sched.osc_voices) >= 1)
        if (length(sched.osc_voices) >= 1) {
            t |> success(sched.osc_voices[0].fx.active, "osc voice should have active FX")
            t |> success(sched.osc_voices[0].fx.has_crush)
            t |> success(sched.osc_voices[0].fx.has_bpf)
        }
    }
}

[test]
def test_scheduler_drum_crush_differs_from_dry(t : T?) {
    t |> run("crush applied to pre-rendered drum changes output") @(t : T?) {
        var inscope dry_sched : Scheduler
        dry_sched.cps = 1.0lf
        var inscope wet_sched : Scheduler
        wet_sched.cps = 1.0lf
        let bank : SampleBank
        let dry_pat <- atom("bd")
        let wet_pat <- atom("bd") |> crush(3.0)
        var dry <- tick_chunks(dry_sched, dry_pat, bank, 6)
        var wet <- tick_chunks(wet_sched, wet_pat, bank, 6)
        let delta = diff_rms(dry, wet)
        t |> success(delta > 1.0e-4, "crush produces audibly different drum output (delta rms={delta})")
        delete dry
        delete wet
    }
}

[test]
def test_scheduler_djf_lpf_vs_bypass(t : T?) {
    t |> run("djf=0.1 (full LPF) differs from djf=0.5 (bypass)") @(t : T?) {
        var inscope dry_sched : Scheduler
        dry_sched.cps = 1.0lf
        var inscope wet_sched : Scheduler
        wet_sched.cps = 1.0lf
        let bank : SampleBank
        let dry_pat <- atom("sawtooth") |> note(60.0)  // djf default 0.5 = bypass
        let wet_pat <- atom("sawtooth") |> note(60.0) |> djf(0.1)
        var dry <- tick_chunks(dry_sched, dry_pat, bank, 4)
        var wet <- tick_chunks(wet_sched, wet_pat, bank, 4)
        let delta = diff_rms(dry, wet)
        t |> success(delta > 1.0e-4, "djf lowpass differs from bypass (delta rms={delta})")
        delete dry
        delete wet
    }
}

[test]
def test_scheduler_shape_distorts(t : T?) {
    t |> run("shape adds harmonic distortion to osc voice") @(t : T?) {
        var inscope dry_sched : Scheduler
        dry_sched.cps = 1.0lf
        var inscope wet_sched : Scheduler
        wet_sched.cps = 1.0lf
        let bank : SampleBank
        let dry_pat <- atom("sine") |> note(60.0)
        let wet_pat <- atom("sine") |> note(60.0) |> shape(5.0)
        var dry <- tick_chunks(dry_sched, dry_pat, bank, 4)
        var wet <- tick_chunks(wet_sched, wet_pat, bank, 4)
        let delta = diff_rms(dry, wet)
        t |> success(delta > 1.0e-4, "shape distorts sine wave (delta rms={delta})")
        delete dry
        delete wet
    }
}

// ─── Phase 5 effects: phaser, tremolo, compressor ───

[test]
def test_phaser_alias(t : T?) {
    let pat <- atom("sawtooth") |> phaser(2.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.phaser - 2.0) < 0.001)
}

[test]
def test_phaser_params(t : T?) {
    let pat <- atom("sawtooth") |> phaser(1.5) |> phaserdepth(0.5) |> phasercenter(800.0) |> phasersweep(1500.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    let e = haps[0].value
    t |> success(abs(e.phaser - 1.5) < 0.001)
    t |> success(abs(e.phaserdepth - 0.5) < 0.001)
    t |> success(abs(e.phasercenter - 800.0) < 0.001)
    t |> success(abs(e.phasersweep - 1500.0) < 0.001)
}

[test]
def test_phaser_short_aliases(t : T?) {
    let pat <- atom("sawtooth") |> ph(3.0) |> phd(0.25) |> phc(1200.0) |> phs(1800.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    let e = haps[0].value
    t |> success(abs(e.phaser - 3.0) < 0.001)
    t |> success(abs(e.phaserdepth - 0.25) < 0.001)
    t |> success(abs(e.phasercenter - 1200.0) < 0.001)
    t |> success(abs(e.phasersweep - 1800.0) < 0.001)
}

[test]
def test_tremolo_alias(t : T?) {
    let pat <- atom("sawtooth") |> tremolo(5.0) |> tremolodepth(0.7)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    let e = haps[0].value
    t |> success(abs(e.tremolo - 5.0) < 0.001)
    t |> success(abs(e.tremolodepth - 0.7) < 0.001)
}

[test]
def test_tremolo_short_aliases(t : T?) {
    let pat <- atom("sawtooth") |> trem(4.0) |> tremdepth(0.5)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    let e = haps[0].value
    t |> success(abs(e.tremolo - 4.0) < 0.001)
    t |> success(abs(e.tremolodepth - 0.5) < 0.001)
}

[test]
def test_compressor_alias(t : T?) {
    let pat <- atom("sawtooth") |> compressor(-20.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.compressor - (-20.0)) < 0.001)
}

[test]
def test_compressor_camel_case_aliases(t : T?) {
    // camelCase aliases
    let pat <- atom("sawtooth") |> compressor(-15.0) |> compressorRatio(8.0) |> compressorKnee(5.0) |> compressorAttack(0.003) |> compressorRelease(0.04)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    let e = haps[0].value
    t |> success(abs(e.compressor - (-15.0)) < 0.001)
    t |> success(abs(e.compressor_ratio - 8.0) < 0.001)
    t |> success(abs(e.compressor_knee - 5.0) < 0.001)
    t |> success(abs(e.compressor_attack - 0.003) < 0.0001)
    t |> success(abs(e.compressor_release - 0.04) < 0.0001)
}

[test]
def test_compressor_snake_case_aliases(t : T?) {
    // idiomatic daScript snake_case variants
    let pat <- atom("sawtooth") |> compressor(-10.0) |> compressor_ratio(6.0) |> compressor_knee(2.0) |> compressor_attack(0.001) |> compressor_release(0.03)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    let e = haps[0].value
    t |> success(abs(e.compressor_ratio - 6.0) < 0.001)
    t |> success(abs(e.compressor_knee - 2.0) < 0.001)
    t |> success(abs(e.compressor_attack - 0.001) < 0.0001)
    t |> success(abs(e.compressor_release - 0.03) < 0.0001)
}

[test]
def test_voicefx_phaser_activates(t : T?) {
    var fx : VoiceFX
    var e = Event()
    e.phaser = 2.0
    fx_init_from_event(fx, e, 48000.0)
    t |> success(fx.has_phaser, "phaser>0 activates")
    t |> success(fx.active)
}

[test]
def test_voicefx_phaser_off(t : T?) {
    var fx : VoiceFX
    var e = Event()
    // phaser=0 (default) should NOT activate even with depth set
    e.phaserdepth = 0.5
    fx_init_from_event(fx, e, 48000.0)
    t |> success(!fx.has_phaser, "phaser rate=0 stays inactive")
}

[test]
def test_voicefx_tremolo_activates(t : T?) {
    var fx : VoiceFX
    var e = Event()
    e.tremolo = 5.0
    e.tremolodepth = 1.0
    fx_init_from_event(fx, e, 48000.0)
    t |> success(fx.has_tremolo)
    t |> success(fx.active)
}

[test]
def test_voicefx_tremolo_zero_depth_inactive(t : T?) {
    var fx : VoiceFX
    var e = Event()
    e.tremolo = 5.0
    e.tremolodepth = 0.0
    fx_init_from_event(fx, e, 48000.0)
    t |> success(!fx.has_tremolo, "tremolo depth=0 stays inactive")
}

[test]
def test_voicefx_compressor_bypass_by_default(t : T?) {
    var fx : VoiceFX
    let e = Event()
    // compressor default is 1.0 (above 0 dB) = bypass
    fx_init_from_event(fx, e, 48000.0)
    t |> success(!fx.has_compressor, "compressor default (+1 dB) stays bypass")
}

[test]
def test_voicefx_compressor_activates(t : T?) {
    var fx : VoiceFX
    var e = Event()
    e.compressor = -20.0
    fx_init_from_event(fx, e, 48000.0)
    t |> success(fx.has_compressor)
    t |> success(fx.active)
}

[test]
def test_fx_apply_phaser_modulates(t : T?) {
    // Phaser should modify a static sine test signal (notch filter output differs from input).
    var fx : VoiceFX
    var e = Event()
    e.phaser = 2.0
    e.phaserdepth = 0.75
    fx_init_from_event(fx, e, 48000.0)
    var buf : array<float>
    buf |> resize(512)
    // fill with a 440Hz sine (roughly in phaser sweep range)
    for (i in range(256)) {
        let s = sin(float(i) * 440.0 * 2.0 * float(PI) / 48000.0) * 0.5
        buf[i * 2]     = s
        buf[i * 2 + 1] = s
    }
    var orig : array<float>
    orig |> resize(512)
    for (i in range(512)) { orig[i] = buf[i]; }
    fx_apply(fx, buf, 256)
    var diff = 0.0
    for (i in range(512)) { diff += abs(buf[i] - orig[i]); }
    t |> success(diff > 1.0e-3, "phaser alters signal (total diff={diff})")
    delete orig
    delete buf
}

[test]
def test_fx_apply_tremolo_envelopes(t : T?) {
    // Tremolo should produce an amplitude envelope — peaks and troughs vs dry constant.
    var fx : VoiceFX
    var e = Event()
    e.tremolo = 20.0   // 20 Hz — noticeable within a short buffer
    e.tremolodepth = 1.0
    fx_init_from_event(fx, e, 48000.0)
    var buf : array<float>
    buf |> resize(4800 * 2)  // 100ms at 48kHz
    for (i in range(4800)) {
        buf[i * 2]     = 0.5  // constant DC signal
        buf[i * 2 + 1] = 0.5
    }
    fx_apply(fx, buf, 4800)
    // find min and max amplitude — tremolo should sweep between them
    var mn = 1.0e9
    var mx = -1.0e9
    for (i in range(4800)) {
        if (buf[i * 2] < mn) { mn = buf[i * 2]; }
        if (buf[i * 2] > mx) { mx = buf[i * 2]; }
    }
    t |> success(mx - mn > 0.1, "tremolo creates envelope (range={mx - mn})")
    delete buf
}

[test]
def test_fx_apply_compressor_reduces_peaks(t : T?) {
    // Compressor with aggressive threshold should reduce peak amplitude of a loud signal.
    var fx : VoiceFX
    var e = Event()
    e.compressor = -20.0
    e.compressor_ratio = 20.0
    e.compressor_attack = 0.0001  // fast attack so the first samples get compressed
    e.compressor_release = 0.01
    e.compressor_knee = 0.0
    fx_init_from_event(fx, e, 48000.0)
    var buf : array<float>
    buf |> resize(4800 * 2)
    for (i in range(4800)) {
        buf[i * 2]     = 0.9
        buf[i * 2 + 1] = 0.9
    }
    fx_apply(fx, buf, 4800)
    // Peak near the end (after attack settles) should be well below the input 0.9
    var tail_peak = 0.0
    for (i in range(4000, 4800)) {
        if (abs(buf[i * 2]) > tail_peak) { tail_peak = abs(buf[i * 2]); }
    }
    t |> success(tail_peak < 0.9, "compressor reduces steady peak ({tail_peak} < 0.9)")
    delete buf
}

[test]
def test_scheduler_osc_phaser_differs_from_dry(t : T?) {
    t |> run("phaser changes osc voice output") @(t : T?) {
        var inscope dry_sched : Scheduler
        dry_sched.cps = 1.0lf
        var inscope wet_sched : Scheduler
        wet_sched.cps = 1.0lf
        let bank : SampleBank
        let dry_pat <- atom("sawtooth") |> note(60.0)
        let wet_pat <- atom("sawtooth") |> note(60.0) |> phaser(4.0) |> phaserdepth(0.9)
        var dry <- tick_chunks(dry_sched, dry_pat, bank, 4)
        var wet <- tick_chunks(wet_sched, wet_pat, bank, 4)
        let delta = diff_rms(dry, wet)
        t |> success(delta > 1.0e-4, "phaser produces audibly different output (delta rms={delta})")
        delete dry
        delete wet
    }
}

[test]
def test_scheduler_osc_tremolo_marks_fx(t : T?) {
    t |> run("tremolo on osc voice sets has_tremolo") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 1.0lf
        let bank : SampleBank
        let pat <- atom("sawtooth") |> note(60.0) |> tremolo(8.0)
        tick(sched, pat, bank, 0.0lf, 0.01)
        t |> success(length(sched.osc_voices) >= 1)
        if (length(sched.osc_voices) >= 1) {
            t |> success(sched.osc_voices[0].fx.has_tremolo, "osc voice should have tremolo")
            t |> success(sched.osc_voices[0].fx.active)
        }
    }
}
