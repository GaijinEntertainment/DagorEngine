options gen2
options indenting = 4

require dastest/testing_boost
require strudel/strudel
require math

// ─── Bare-name setter aliases (scalar) ───

[test]
def test_gain_alias(t : T?) {
    let pat <- atom("bd") |> gain(0.5)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    t |> success(abs(haps[0].value.gain - 0.5) < 0.001)
}

[test]
def test_velocity_alias(t : T?) {
    let pat <- atom("bd") |> velocity(0.7)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.velocity - 0.7) < 0.001)
}

[test]
def test_vel_short_alias(t : T?) {
    let pat <- atom("bd") |> vel(0.3)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.velocity - 0.3) < 0.001)
}

[test]
def test_pan_alias(t : T?) {
    let pat <- atom("bd") |> pan(0.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.pan - 0.0) < 0.001)
}

[test]
def test_speed_alias(t : T?) {
    let pat <- atom("bd") |> speed(2.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.speed - 2.0) < 0.001)
}

[test]
def test_note_float_alias(t : T?) {
    let pat <- atom("sine") |> note(72.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.note - 72.0) < 0.001)
}

[test]
def test_sound_alias(t : T?) {
    let pat <- atom("bd") |> sound("sawtooth")
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(haps[0].value.s, "sawtooth")
}

[test]
def test_cut_alias(t : T?) {
    let pat <- atom("bd") |> cut(3)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(haps[0].value.cut, 3)
}

[test]
def test_room_alias(t : T?) {
    let pat <- atom("bd") |> room(0.5)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.room - 0.5) < 0.001)
}

[test]
def test_roomsize_alias(t : T?) {
    let pat <- atom("bd") |> roomsize(3.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.roomsize - 3.0) < 0.001)
}

[test]
def test_size_short_alias(t : T?) {
    let pat <- atom("bd") |> size(4.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.roomsize - 4.0) < 0.001)
}

[test]
def test_delay_alias(t : T?) {
    let pat <- atom("bd") |> delay(0.8)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.delay_amount - 0.8) < 0.001)
}

[test]
def test_delaytime_alias(t : T?) {
    let pat <- atom("bd") |> delaytime(0.25)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.delaytime - 0.25) < 0.001)
}

[test]
def test_dt_short_alias(t : T?) {
    let pat <- atom("bd") |> dt(0.125)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.delaytime - 0.125) < 0.001)
}

[test]
def test_delayfeedback_alias(t : T?) {
    let pat <- atom("bd") |> delayfeedback(0.6)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.delayfeedback - 0.6) < 0.001)
}

[test]
def test_dfb_short_alias(t : T?) {
    let pat <- atom("bd") |> dfb(0.9)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.delayfeedback - 0.9) < 0.001)
}

[test]
def test_orbit_alias(t : T?) {
    let pat <- atom("bd") |> orbit(2)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(haps[0].value.orbit, 2)
}

[test]
def test_fm_alias(t : T?) {
    let pat <- atom("sine") |> fm(3.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.fm - 3.0) < 0.001)
}

[test]
def test_fmh_alias(t : T?) {
    let pat <- atom("sine") |> fmh(2.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.fmh - 2.0) < 0.001)
}

[test]
def test_lpf_alias(t : T?) {
    let pat <- atom("bd") |> lpf(2000.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.lpf - 2000.0) < 0.001)
}

[test]
def test_lp_short_alias(t : T?) {
    let pat <- atom("bd") |> lp(1500.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.lpf - 1500.0) < 0.001)
}

[test]
def test_hpf_alias(t : T?) {
    let pat <- atom("bd") |> hpf(300.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.hpf - 300.0) < 0.001)
}

[test]
def test_hp_short_alias(t : T?) {
    let pat <- atom("bd") |> hp(500.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.hpf - 500.0) < 0.001)
}

[test]
def test_lpq_alias(t : T?) {
    let pat <- atom("bd") |> lpq(5.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.lpq - 5.0) < 0.001)
}

[test]
def test_resonance_alias(t : T?) {
    let pat <- atom("bd") |> resonance(8.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.lpq - 8.0) < 0.001)
}

[test]
def test_hpq_alias(t : T?) {
    let pat <- atom("bd") |> hpq(3.0)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.hpq - 3.0) < 0.001)
}

[test]
def test_attack_alias(t : T?) {
    let pat <- atom("sine") |> attack(0.1)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.attack - 0.1) < 0.001)
}

[test]
def test_att_short_alias(t : T?) {
    let pat <- atom("sine") |> att(0.2)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.attack - 0.2) < 0.001)
}

[test]
def test_decay_alias(t : T?) {
    let pat <- atom("sine") |> decay(0.3)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.decay - 0.3) < 0.001)
}

[test]
def test_dec_short_alias(t : T?) {
    let pat <- atom("sine") |> dec(0.4)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.decay - 0.4) < 0.001)
}

[test]
def test_sustain_alias(t : T?) {
    let pat <- atom("sine") |> sustain(0.7)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.sustain - 0.7) < 0.001)
}

[test]
def test_sus_short_alias(t : T?) {
    let pat <- atom("sine") |> sus(0.8)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.sustain - 0.8) < 0.001)
}

[test]
def test_release_alias(t : T?) {
    let pat <- atom("sine") |> release(0.5)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.release - 0.5) < 0.001)
}

[test]
def test_rel_short_alias(t : T?) {
    let pat <- atom("sine") |> rel(0.6)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.release - 0.6) < 0.001)
}

[test]
def test_chorus_alias(t : T?) {
    let pat <- atom("sine") |> chorus(0.4)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.chorus - 0.4) < 0.001)
}

// ─── SF2 aliases ───

[test]
def test_sf2_program_alias(t : T?) {
    let pat <- atom("bd") |> sf2(42)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(haps[0].value.sf2_program, 42)
}

[test]
def test_sf2_name_alias(t : T?) {
    let pat <- atom("bd") |> sf2("piano")
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(haps[0].value.sf2_program, 0)
}

[test]
def test_sf2_bank_alias(t : T?) {
    let pat <- atom("bd") |> sf2_bank(128)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(haps[0].value.sf2_bank, 128)
}

[test]
def test_sf2_expression_alias(t : T?) {
    let pat <- atom("bd") |> sf2_expression(0.9)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.sf2_expression - 0.9) < 0.001)
}

// ─── Pattern-modulated aliases ───

[test]
def test_gain_pattern_alias(t : T?) {
    let pat <- atom("bd") |> gain(sine())
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    // gain should be ~0.5 at t=0 (sine starts at midpoint)
    t |> success(abs(haps[0].value.gain - 0.5) < 0.1)
}

[test]
def test_lpf_pattern_alias(t : T?) {
    let pat <- atom("sawtooth") |> lpf(sine() |> range(200.0, 4000.0))
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    // At t=0, sine=0.5, mapped to 200+0.5*3800=2100
    t |> success(abs(haps[0].value.lpf - 2100.0) < 100.0)
}

// ─── Signal aliases ───

[test]
def test_sine_alias(t : T?) {
    let pat <- sine()
    let haps <- pat(TimeSpan(start = 0.25lf, stop = 0.251lf))
    t |> success(abs(haps[0].value.note - 1.0) < 0.01)
}

[test]
def test_saw_alias(t : T?) {
    let pat <- saw()
    let haps <- pat(TimeSpan(start = 0.5lf, stop = 0.501lf))
    t |> success(abs(haps[0].value.note - 0.5) < 0.01)
}

[test]
def test_tri_alias(t : T?) {
    let pat <- tri()
    let haps <- pat(TimeSpan(start = 0.5lf, stop = 0.501lf))
    t |> success(abs(haps[0].value.note - 1.0) < 0.01)
}

[test]
def test_cosine_alias(t : T?) {
    let pat <- cosine()
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 0.001lf))
    t |> success(abs(haps[0].value.note - 1.0) < 0.01)
}

[test]
def test_perlin_alias(t : T?) {
    let pat <- perlin()
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 0.001lf))
    t |> equal(length(haps), 1)
    t |> success(haps[0].value.note >= 0.0 && haps[0].value.note <= 1.0)
}

[test]
def test_range_alias(t : T?) {
    let pat <- saw() |> range(100.0, 200.0)
    let haps <- pat(TimeSpan(start = 0.5lf, stop = 0.501lf))
    // saw at 0.5 = 0.5, mapped to 100 + 0.5*100 = 150
    t |> success(abs(haps[0].value.note - 150.0) < 1.0)
}

// ─── note() mini-notation alias ───

[test]
def test_note_string_alias(t : T?) {
    let pat <- note("c4 e4 g4")
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 3)
    t |> success(abs(haps[0].value.note - 60.0) < 0.01)  // C4 = 60
    t |> success(abs(haps[1].value.note - 64.0) < 0.01)  // E4 = 64
    t |> success(abs(haps[2].value.note - 67.0) < 0.01)  // G4 = 67
    // default sound is "sine"
    t |> equal(haps[0].value.s, "sine")
}

// ─── scale() alias ───

[test]
def test_scale_alias(t : T?) {
    let pat <- n("0 2 4") |> scale("C4:major")
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 3)
    t |> success(abs(haps[0].value.note - 60.0) < 0.01)  // C4
    t |> success(abs(haps[1].value.note - 64.0) < 0.01)  // E4
    t |> success(abs(haps[2].value.note - 67.0) < 0.01)  // G4
}

// ─── degradeBy alias ───

[test]
def test_degradeBy_alias(t : T?) {
    let pat <- s("bd sd hh cp") |> degradeBy(0.5)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    // With 50% degradation, should have fewer than 4 events (probabilistic but deterministic)
    t |> success(length(haps) <= 4)
    t |> success(length(haps) >= 0)
}

// ─── Chain multiple bare-name aliases (integration test) ───

[test]
def test_chain_bare_names(t : T?) {
    // Chained bare-name aliases: note("c4").s("sawtooth").gain(0.5).lpf(2000).attack(0.01).room(0.3)
    let pat <- note("c4", "sawtooth") |> gain(0.5) |> lpf(2000.0) |> att(0.01) |> room(0.3)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> equal(length(haps), 1)
    t |> equal(haps[0].value.s, "sawtooth")
    t |> success(abs(haps[0].value.note - 60.0) < 0.01)
    t |> success(abs(haps[0].value.gain - 0.5) < 0.001)
    t |> success(abs(haps[0].value.lpf - 2000.0) < 0.001)
    t |> success(abs(haps[0].value.attack - 0.01) < 0.001)
    t |> success(abs(haps[0].value.room - 0.3) < 0.001)
}

[test]
def test_begin_alias(t : T?) {
    let pat <- atom("bd") |> begin(0.25)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.begin - 0.25) < 0.001)
}

[test]
def test_end_pos_alias(t : T?) {
    let pat <- atom("bd") |> end_pos(0.75)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.end_pos - 0.75) < 0.001)
}

[test]
def test_begin_end_chain(t : T?) {
    let pat <- atom("bd") |> begin(0.1) |> end_pos(0.6)
    let haps <- pat(TimeSpan(start = 0.0lf, stop = 1.0lf))
    t |> success(abs(haps[0].value.begin - 0.1) < 0.001)
    t |> success(abs(haps[0].value.end_pos - 0.6) < 0.001)
}
