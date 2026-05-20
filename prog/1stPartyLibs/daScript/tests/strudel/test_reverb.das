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
def test_osc_voice_reverb_send_field(t : T?) {
    t |> run("OscVoice reverb_send populated from Event.room") @(t : T?) {
        // Spawn an osc voice with room=0.5 via the scheduler
        var inscope sched : Scheduler
        sched.cps = 1.0lf
        init_reverb(sched)
        let pat <- atom("sine") |> set_room(0.5) |> set_sustain(1.0)
        let bank : SampleBank
        tick(sched, pat, bank, 0.0lf, 0.05)
        // Should have spawned an osc voice with reverb_send = 0.5
        t |> equal(length(sched.osc_voices), 1)
        t |> success(abs(sched.osc_voices[0].reverb_send - 0.5) < 0.001)
    }
}

[test]
def test_reverb_auto_init(t : T?) {
    t |> run("reverb auto-initializes when room > 0") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 1.0lf
        // reverb is null initially
        t |> success(get_orbit_reverb(sched) == null)
        let pat <- atom("sine") |> set_room(0.3)
        let bank : SampleBank
        tick(sched, pat, bank, 0.0lf, 0.05)
        // After ticking with room > 0, reverb should be initialized
        t |> success(get_orbit_reverb(sched) != null)
    }
}

[test]
def test_no_reverb_when_room_zero(t : T?) {
    t |> run("reverb stays null when room = 0") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 1.0lf
        let pat <- atom("sine")  // room defaults to 0.0
        let bank : SampleBank
        tick(sched, pat, bank, 0.0lf, 0.05)
        t |> success(get_orbit_reverb(sched) == null)
    }
}

[test]
def test_reverb_send_produces_output(t : T?) {
    t |> run("room > 0 produces reverb tail after voice ends") @(t : T?) {
        // Short pluck with reverb — slow cps so only one event fires
        var inscope sched : Scheduler
        sched.cps = 0.5lf
        init_reverb(sched)
        // Short note with room=1.0, gain=1.0 to maximize send level (osc voices have OSC_TURN_DOWN=0.3)
        let pat <- atom("sine") |> set_room(1.0) |> set_gain(1.0) |> set_sustain(0.0) |> set_release(0.05) |> set_decay(0.1)
        let bank : SampleBank
        // Tick enough chunks for the voice to end + reverb tail
        let output <- tick_chunks(sched, pat, bank, 20, 0.05)  // 1 second total
        // Voice is audible for ~0.15s. Check at 0.5s — reverb tail should be present.
        let sr = 48000
        let tailRms = rms_range(output, sr / 2, sr / 2 + sr / 4)
        t |> success(tailRms > 0.00001, "reverb tail should be non-zero after voice ends")
    }
}

[test]
def test_no_reverb_tail_when_dry(t : T?) {
    t |> run("room = 0 produces no reverb tail") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 0.5lf  // slow: one event in 2s
        // Same note but dry (room=0)
        let pat <- atom("sine") |> set_sustain(0.0) |> set_release(0.05) |> set_decay(0.05)
        let bank : SampleBank
        let output <- tick_chunks(sched, pat, bank, 40, 0.05)
        // Voice ends quickly (~0.1s). At 1.5s there should be silence.
        let sr = 48000
        let tailRms = rms_range(output, sr + sr / 2, sr + sr / 2 + sr / 4)
        t |> success(tailRms < 0.0001, "dry voice should have no reverb tail")
    }
}

[test]
def test_reverb_on_drums(t : T?) {
    t |> run("pre-rendered drum with room > 0 has reverb send") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 0.5lf
        init_reverb(sched)
        let pat <- atom("bd") |> set_room(0.8) |> set_sustain(0.0)
        let bank : SampleBank
        let output <- tick_chunks(sched, pat, bank, 40, 0.05)
        // Kick is short, reverb tail should be audible at 1.0s
        let sr = 48000
        let tailRms = rms_range(output, sr, sr + sr / 4)
        t |> success(tailRms > 0.0001, "drum reverb tail should be non-zero")
    }
}

[test]
def test_reverb_roomsize_decay(t : T?) {
    t |> run("larger roomsize produces longer reverb tail") @(t : T?) {
        // Small room (short decay)
        var inscope sched1 : Scheduler
        sched1.cps = 0.5lf
        init_reverb(sched1)
        let pat1 <- atom("sine") |> set_room(0.8) |> set_roomsize(0.5) |> set_sustain(0.0) |> set_release(0.05) |> set_decay(0.05)
        let bank : SampleBank
        let out1 <- tick_chunks(sched1, pat1, bank, 40, 0.05)

        // Large room (long decay)
        var inscope sched2 : Scheduler
        sched2.cps = 0.5lf
        init_reverb(sched2)
        let pat2 <- atom("sine") |> set_room(0.8) |> set_roomsize(8.0) |> set_sustain(0.0) |> set_release(0.05) |> set_decay(0.05)
        let out2 <- tick_chunks(sched2, pat2, bank, 40, 0.05)

        // At 1.5s, the long-decay reverb should be louder than the short-decay one
        let sr = 48000
        let rms1 = rms_range(out1, sr + sr / 2, sr + sr / 2 + sr / 4)
        let rms2 = rms_range(out2, sr + sr / 2, sr + sr / 2 + sr / 4)
        t |> success(rms2 > rms1, "longer roomsize should have louder tail at 1.5s")
    }
}

[test]
def test_reverb_supersaw(t : T?) {
    t |> run("supersaw with room produces reverb tail") @(t : T?) {
        var inscope sched : Scheduler
        sched.cps = 0.5lf
        init_reverb(sched)
        let pat <- atom("supersaw") |> set_room(1.0) |> set_gain(1.0) |> set_sustain(0.0) |> set_release(0.05) |> set_decay(0.1) |> set_note(48.0)
        let bank : SampleBank
        let output <- tick_chunks(sched, pat, bank, 20, 0.05)
        let sr = 48000
        let tailRms = rms_range(output, sr / 2, sr / 2 + sr / 4)
        t |> success(tailRms > 0.00001, "supersaw reverb tail should be non-zero")
    }
}
