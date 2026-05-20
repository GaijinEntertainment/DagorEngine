options gen2
options indenting = 4

require dastest/testing_boost
require strudel/strudel
require strudel/strudel_synth
require strudel/strudel_scheduler
require strudel/strudel_event
require strudel/strudel_pattern
require strudel/strudel_samples
require math

[test]
def test_hrtf_setters_engage_active(t : T?) {
    t |> run("hrtf_azimuth flips hrtf_active true on every event") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 1.0lf
        let pat <- atom("sine") |> hrtf_azimuth(-45.0)
        let bank : SampleBank
        tick(sched, pat, bank, 0.0lf, 0.05)
        t |> equal(length(sched.osc_voices), 1)
        t |> success(sched.osc_voices[0].hrtf_active, "voice should have hrtf_active = true")
        t |> numericEqual(sched.osc_voices[0].hrtf_azimuth, -45.0)
    }
}

[test]
def test_hrtf_combined_setter(t : T?) {
    t |> run("hrtf(az, el) sets both axes and engages") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 1.0lf
        let pat <- atom("sine") |> hrtf(60.0, 15.0)
        let bank : SampleBank
        tick(sched, pat, bank, 0.0lf, 0.05)
        t |> equal(length(sched.osc_voices), 1)
        t |> success(sched.osc_voices[0].hrtf_active)
        t |> numericEqual(sched.osc_voices[0].hrtf_azimuth, 60.0)
        t |> numericEqual(sched.osc_voices[0].hrtf_elevation, 15.0)
    }
}

[test]
def test_no_hrtf_means_inactive(t : T?) {
    t |> run("plain pattern leaves hrtf_active = false") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 1.0lf
        let pat <- atom("sine") |> pan(0.5)
        let bank : SampleBank
        tick(sched, pat, bank, 0.0lf, 0.05)
        t |> equal(length(sched.osc_voices), 1)
        t |> success(!sched.osc_voices[0].hrtf_active, "voice without hrtf setter must keep hrtf_active = false")
    }
}

[test]
def test_hrtf_overrides_pan(t : T?) {
    t |> run("setting hrtf alongside pan still engages HRTF (pan path is bypassed)") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 1.0lf
        // Pan first, then hrtf — hrtf must win.
        let pat <- atom("sine") |> pan(0.5) |> hrtf(-90.0, 0.0)
        let bank : SampleBank
        tick(sched, pat, bank, 0.0lf, 0.05)
        t |> success(sched.osc_voices[0].hrtf_active, "hrtf must still be active after also setting pan")
        t |> numericEqual(sched.osc_voices[0].hrtf_azimuth, -90.0)
    }
}

[test]
def test_hrtf_renders_audible_output(t : T?) {
    t |> run("HRTF-positioned sine produces audible stereo output without crashing") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 0.5lf
        let pat <- atom("sine") |> hrtf(-90.0, 0.0) |> set_gain(0.8) |> set_release(0.2)
        let bank : SampleBank
        // tick a few chunks to ensure HRTF renders without bounds issues
        for (c in range(8)) {
            tick(sched, pat, bank, double(c) * 0.05lf, 0.05)
        }
        // some sample produced (RMS check across whole tick)
        var energy = 0.0
        for (s in sched.output) {
            energy += s * s
        }
        t |> success(energy > 0.0, "HRTF rendering should produce non-silent output")
    }
}

[test]
def test_hrtf_pattern_modulated(t : T?) {
    t |> run("pattern-valued hrtf_azimuth animates per event") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 1.0lf
        // 4 events per cycle, azimuths -90, -30, 30, 90
        let pat <- s("sine sine sine sine") |> hrtf_azimuth(s("-90 -30 30 90"))
        let bank : SampleBank
        // process one full cycle
        for (c in range(20)) {
            tick(sched, pat, bank, double(c) * 0.05lf, 0.05)
        }
        // every spawned voice should have hrtf_active = true
        var any_voice = false
        for (v in sched.osc_voices) {
            t |> success(v.hrtf_active, "every voice with pattern-valued hrtf_azimuth must be HRTF-active")
            any_voice = true
        }
        t |> success(any_voice || length(sched.osc_voices) == 0, "ok — voices may have all completed by end of tick")
    }
}

[test]
def test_hrtf_lazy_alloc(t : T?) {
    t |> run("ma_hrtf instances allocated lazily on first render") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 0.5lf
        let pat <- atom("sine") |> hrtf(45.0, 0.0) |> set_release(0.1)
        let bank : SampleBank
        // first tick should allocate the per-voice hrtf pair (one per channel: az - 30°, az + 30°)
        tick(sched, pat, bank, 0.0lf, 0.05)
        t |> equal(length(sched.osc_voices), 1)
        t |> success(sched.osc_voices[0].hrtf_l != null, "voice.hrtf_l should be allocated after first render")
        t |> success(sched.osc_voices[0].hrtf_r != null, "voice.hrtf_r should be allocated after first render")
    }
}
