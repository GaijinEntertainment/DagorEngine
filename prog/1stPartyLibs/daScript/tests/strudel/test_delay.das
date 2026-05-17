options gen2
options indenting = 4

require dastest/testing_boost
require strudel/strudel
require strudel/strudel_synth
require strudel/strudel_scheduler
require strudel/strudel_samples
require math

// Helper: tick a scheduler with a pattern for N chunks and return concatenated output.
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

// Helper: compute RMS of a range of stereo samples.
def private rms_range(samples : array<float>; startSample, endSample : int) : float {
    var sum = 0.0
    var count = 0
    let start2 = startSample * 2
    let end2 = min(endSample * 2, length(samples))
    for (i in range(start2, end2)) {
        sum += samples[i] * samples[i]
        count ++
    }
    if (count == 0) {
        return 0.0
    }
    return sqrt(sum / float(count))
}

[test]
def test_delay_auto_init(t : T?) {
    t |> run("delay auto-initializes when delay_amount > 0") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 1.0lf
        t |> success(get_orbit_delay(sched) == null)
        let pat <- atom("sine") |> set_delay(0.5) |> set_delaytime(0.25)
        let bank : SampleBank
        tick(sched, pat, bank, 0.0lf, 0.05)
        t |> success(get_orbit_delay(sched) != null, "delay should auto-init when delay_amount > 0")
    }
}

[test]
def test_no_delay_when_zero(t : T?) {
    t |> run("delay stays null when delay_amount = 0") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 1.0lf
        let pat <- atom("sine")  // delay_amount defaults to 0.0
        let bank : SampleBank
        tick(sched, pat, bank, 0.0lf, 0.05)
        t |> success(get_orbit_delay(sched) == null, "delay should not init when delay_amount = 0")
    }
}

[test]
def test_osc_voice_delay_send_field(t : T?) {
    t |> run("OscVoice delay_send populated from Event.delay_amount") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 1.0lf
        let pat <- atom("sine") |> set_delay(0.7) |> set_delaytime(0.25)
        let bank : SampleBank
        tick(sched, pat, bank, 0.0lf, 0.05)
        t |> equal(length(sched.osc_voices), 1)
        t |> success(abs(sched.osc_voices[0].delay_send - 0.7) < 0.001, "delay_send should match Event.delay_amount")
    }
}

[test]
def test_delay_produces_echo(t : T?) {
    t |> run("delay_amount > 0 produces echo after voice ends") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 0.5lf
        init_delay(sched)
        // Short percussive sine with delay. delaytime=0.25s should produce audible echo.
        let pat <- atom("sine") |> set_delay(1.0) |> set_delaytime(0.25) |> set_delayfeedback(0.5) |> set_gain(1.0) |> set_sustain(0.0) |> set_release(0.05) |> set_decay(0.1)
        let bank : SampleBank
        let output <- tick_chunks(sched, pat, bank, 40, 0.05)  // 2 seconds total
        let sr = 48000
        // Voice ends at ~0.15s. At 0.3-0.5s the first echo should be audible.
        let echoRms = rms_range(output, sr / 4, sr / 2)
        t |> success(echoRms > 0.0001, "delay echo should be audible after voice ends")
    }
}

[test]
def test_no_delay_tail_when_dry(t : T?) {
    t |> run("delay_amount = 0 produces no echo tail") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 0.5lf
        let pat <- atom("sine") |> set_sustain(0.0) |> set_release(0.05) |> set_decay(0.05)
        let bank : SampleBank
        let output <- tick_chunks(sched, pat, bank, 40, 0.05)
        let sr = 48000
        // Voice ends quickly. At 1.0s there should be silence.
        let tailRms = rms_range(output, sr, sr + sr / 4)
        t |> success(tailRms < 0.0001, "dry voice should have no delay tail")
    }
}

[test]
def test_delay_feedback_decay(t : T?) {
    t |> run("higher feedback produces longer echo trail") @(t : T?) {
        // Low feedback
        var inscope sched1 : Scheduler
        sched1.cps = 0.5lf
        init_delay(sched1)
        let pat1 <- atom("sine") |> set_delay(1.0) |> set_delaytime(0.2) |> set_delayfeedback(0.2) |> set_sustain(0.0) |> set_release(0.05) |> set_decay(0.05)
        let bank : SampleBank
        let out1 <- tick_chunks(sched1, pat1, bank, 40, 0.05)

        // High feedback
        var inscope sched2 : Scheduler
        sched2.cps = 0.5lf
        init_delay(sched2)
        let pat2 <- atom("sine") |> set_delay(1.0) |> set_delaytime(0.2) |> set_delayfeedback(0.9) |> set_sustain(0.0) |> set_release(0.05) |> set_decay(0.05)
        let out2 <- tick_chunks(sched2, pat2, bank, 40, 0.05)

        // At 1.5s, higher feedback should have louder tail
        let sr = 48000
        let rms1 = rms_range(out1, sr + sr / 2, sr + sr / 2 + sr / 4)
        let rms2 = rms_range(out2, sr + sr / 2, sr + sr / 2 + sr / 4)
        t |> success(rms2 > rms1, "higher feedback should have louder tail at 1.5s")
    }
}

[test]
def test_delay_on_drums(t : T?) {
    t |> run("pre-rendered drum with delay has echo") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 0.5lf
        init_delay(sched)
        let pat <- atom("bd") |> set_delay(0.8) |> set_delaytime(0.3) |> set_delayfeedback(0.5) |> set_sustain(0.0)
        let bank : SampleBank
        let output <- tick_chunks(sched, pat, bank, 40, 0.05)
        let sr = 48000
        // Kick is short, delay echo should be audible at 0.5s
        let echoRms = rms_range(output, sr / 2, sr / 2 + sr / 4)
        t |> success(echoRms > 0.0001, "drum delay echo should be non-zero")
    }
}

[test]
def test_delay_supersaw(t : T?) {
    t |> run("supersaw with delay produces echo") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 0.5lf
        init_delay(sched)
        let pat <- atom("supersaw") |> set_delay(1.0) |> set_delaytime(0.3) |> set_delayfeedback(0.5) |> set_gain(1.0) |> set_sustain(0.0) |> set_release(0.05) |> set_decay(0.1) |> set_note(48.0)
        let bank : SampleBank
        let output <- tick_chunks(sched, pat, bank, 20, 0.05)
        let sr = 48000
        let echoRms = rms_range(output, sr / 2, sr / 2 + sr / 4)
        t |> success(echoRms > 0.00001, "supersaw delay echo should be non-zero")
    }
}

[test]
def test_delay_plus_reverb(t : T?) {
    t |> run("delay and reverb can coexist") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 0.5lf
        init_delay(sched)
        init_reverb(sched)
        let pat <- atom("sine") |> set_delay(0.8) |> set_delaytime(0.25) |> set_delayfeedback(0.5) |> set_room(0.6) |> set_sustain(0.0) |> set_release(0.05) |> set_decay(0.1)
        let bank : SampleBank
        let output <- tick_chunks(sched, pat, bank, 40, 0.05)
        let sr = 48000
        // Both delay echo and reverb tail should be present at 0.5s
        let tailRms = rms_range(output, sr / 2, sr / 2 + sr / 4)
        t |> success(tailRms > 0.0001, "delay+reverb combined tail should be audible")
    }
}
