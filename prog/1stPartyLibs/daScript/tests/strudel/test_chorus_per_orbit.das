options gen2
options indenting = 4

require dastest/testing_boost
require strudel/strudel
require strudel/strudel_synth
require strudel/strudel_scheduler
require strudel/strudel_samples
require math

[test]
def test_chorus_orbit_lazy_init(t : T?) {
    t |> run("chorus on orbit allocated only when event uses chorus") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 1.0lf
        // Dry orbit 0 — no chorus should be created
        let pat <- atom("sine") |> set_orbit(0)
        let bank : SampleBank
        tick(sched, pat, bank, 0.0lf, 0.05)
        t |> success(get_orbit_chorus(sched, 0) == null, "dry orbit should have no chorus instance")
    }
}

[test]
def test_chorus_orbit_init_on_send(t : T?) {
    t |> run("chorus instance created when an event with chorus arrives") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 1.0lf
        let pat <- atom("sine") |> chorus(0.6) |> set_orbit(2)
        let bank : SampleBank
        tick(sched, pat, bank, 0.0lf, 0.05)
        t |> success(get_orbit_chorus(sched, 2) != null, "orbit 2 should have chorus instance")
        t |> success(get_orbit_chorus(sched, 0) == null, "orbit 0 should remain dry")
    }
}

[test]
def test_chorus_per_orbit_independence(t : T?) {
    t |> run("two orbits with chorus get independent chorus instances") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 1.0lf
        let pat <- stack([
            atom("sine") |> chorus(0.5) |> set_orbit(0),
            atom("sine") |> chorus(0.5) |> set_orbit(1)
        ])
        let bank : SampleBank
        tick(sched, pat, bank, 0.0lf, 0.05)
        t |> success(get_orbit_chorus(sched, 0) != null, "orbit 0 should have chorus")
        t |> success(get_orbit_chorus(sched, 1) != null, "orbit 1 should have chorus")
        t |> success(get_orbit_chorus(sched, 0) != get_orbit_chorus(sched, 1), "orbits should have different chorus instances")
    }
}

[test]
def test_chorus_isolation_between_orbits(t : T?) {
    t |> run("chorus on orbit 0 leaves orbit 1 dry") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 1.0lf
        // Only orbit 0 has chorus; orbit 1 plays the same note dry.
        let pat <- stack([
            atom("sine") |> chorus(0.7) |> set_orbit(0),
            atom("sine") |> set_orbit(1)
        ])
        let bank : SampleBank
        tick(sched, pat, bank, 0.0lf, 0.05)
        t |> success(get_orbit_chorus(sched, 0) != null, "orbit 0 should have chorus")
        t |> success(get_orbit_chorus(sched, 1) == null, "orbit 1 should NOT have chorus (regression guard for the old shared-chorus behaviour)")
    }
}

[test]
def test_chorus_voice_field(t : T?) {
    t |> run("OscVoice carries chorus_send from Event.chorus") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 1.0lf
        let pat <- atom("sine") |> chorus(0.4)
        let bank : SampleBank
        tick(sched, pat, bank, 0.0lf, 0.05)
        t |> equal(length(sched.osc_voices), 1)
        t |> numericEqual(sched.osc_voices[0].chorus_send, 0.4)
    }
}
