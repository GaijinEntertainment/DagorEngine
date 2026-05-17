options gen2
options indenting = 4

require dastest/testing_boost
require strudel/strudel
require math

// Helper: query a pattern at cycle 0 and return the first event.
def private query_first(var pat : Pattern) : Event {
    var haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    let ev = haps[0].value
    delete haps
    return ev
}

// ─── Float setters ───

[test]
def test_set_room(t : T?) {
    let pat <- atom("bd") |> set_room(0.7)
    let ev = query_first(pat)
    t |> success(abs(ev.room - 0.7) < 0.001)
}

[test]
def test_set_roomsize(t : T?) {
    let pat <- atom("bd") |> set_roomsize(0.9)
    let ev = query_first(pat)
    t |> success(abs(ev.roomsize - 0.9) < 0.001)
}

[test]
def test_set_delay(t : T?) {
    let pat <- atom("bd") |> set_delay(0.6)
    let ev = query_first(pat)
    t |> success(abs(ev.delay_amount - 0.6) < 0.001)
}

[test]
def test_set_delaytime(t : T?) {
    let pat <- atom("bd") |> set_delaytime(0.25)
    let ev = query_first(pat)
    t |> success(abs(ev.delaytime - 0.25) < 0.001)
}

[test]
def test_set_delayfeedback(t : T?) {
    let pat <- atom("bd") |> set_delayfeedback(0.8)
    let ev = query_first(pat)
    t |> success(abs(ev.delayfeedback - 0.8) < 0.001)
}

[test]
def test_set_orbit(t : T?) {
    let pat <- atom("bd") |> set_orbit(3)
    let ev = query_first(pat)
    t |> equal(ev.orbit, 3)
}

[test]
def test_set_fm(t : T?) {
    let pat <- atom("sine") |> set_fm(2.0)
    let ev = query_first(pat)
    t |> success(abs(ev.fm - 2.0) < 0.001)
}

[test]
def test_set_fmh(t : T?) {
    let pat <- atom("sine") |> set_fmh(1.5)
    let ev = query_first(pat)
    t |> success(abs(ev.fmh - 1.5) < 0.001)
}

// ─── Chaining ───

[test]
def test_setter_chaining(t : T?) {
    let pat <- atom("sawtooth") |> set_gain(0.5) |> set_lpf(1000.0) |> set_room(0.3) |> set_delay(0.4)
    let ev = query_first(pat)
    t |> success(abs(ev.gain - 0.5) < 0.001)
    t |> success(abs(ev.lpf - 1000.0) < 0.1)
    t |> success(abs(ev.room - 0.3) < 0.001)
    t |> success(abs(ev.delay_amount - 0.4) < 0.001)
}

// ─── Pattern-valued setters ───

[test]
def test_set_room_pattern(t : T?) {
    // atom("bd") onset is at cycle start (0.0); signal_saw at t=0.0 → 0.0
    let pat <- atom("bd") |> set_room(signal_saw())
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].value.room) < 0.05)
}

[test]
def test_set_fm_pattern(t : T?) {
    // signal_saw at t=0.0 → 0.0, scaled to 0..4 → fm=0
    let pat <- atom("sine") |> set_fm(signal_saw() |> signal_range(0.0, 4.0))
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 0.001lf))
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].value.fm) < 0.1)
}

// ─── Default values ───

[test]
def test_event_defaults(t : T?) {
    let ev = Event()
    t |> success(abs(ev.room) < 0.001)
    t |> success(abs(ev.roomsize - 2.0) < 0.001)
    t |> success(abs(ev.delay_amount) < 0.001)
    t |> success(ev.delaytime < 0.0)  // sentinel: unset → resolver derives from delaysync + cps
    t |> success(abs(ev.delaysync - 0.1875) < 0.001)  // 3/16 cycle default
    t |> success(abs(ev.delayfeedback - 0.5) < 0.001)
    t |> equal(ev.orbit, 0)
    t |> success(abs(ev.fm) < 0.001)
    t |> success(abs(ev.fmh - 1.0) < 0.001)
}
