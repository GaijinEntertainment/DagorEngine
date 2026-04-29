.. _tutorial_dasAudio_midi:

==========================
AUDIO-08 — MIDI Files
==========================

.. index::
    single: Tutorial; MIDI
    single: Tutorial; SoundFont
    single: Tutorial; Cross-fade

This tutorial covers MIDI file parsing, playback with the built-in sample
banks, and cross-fading between two simultaneously playing MIDI tracks.
It is split into three parts that mirror the structure of the source file.

Part A: MIDI Parsing
====================

``load_midi`` reads a ``.mid`` file and returns a ``MidiFile`` struct:

.. code-block:: das

    require strudel/strudel_midi

    let midi = load_midi("fur_elise.mid")
    print("Format: {midi.format}\n")         // 0 = single track, 1 = multi-track
    print("Ticks/QN: {midi.ticks_per_qn}\n") // timing resolution
    print("Tracks: {length(midi.tracks)}\n")

Each track is a ``MidiTrack`` containing an array of ``MidiEvent``.
Events carry a tick timestamp, a ``MidiEventKind`` (note-on, note-off,
control change, tempo, etc.), a channel number, and two data bytes.

``merge_tracks`` combines all tracks into a single sorted event list,
which is easier to iterate for analysis or playback:

.. code-block:: das

    var merged <- merge_tracks(midi)
    for (evt in merged) {
        if (evt.kind == MidiEventKind.midi_tempo) {
            let bpm = 60000000.0 / float(evt.tempo)
            print("Tempo change at tick {evt.tick}: {bpm:.1f} BPM\n")
        }
    }

To compute the total duration in seconds, walk the merged events and
accumulate time using the current tempo and ``ticks_per_qn``:

.. code-block:: das

    var tempo_us = 500000   // default 120 BPM
    var total_time = 0.0
    var prev_tick = 0
    for (evt in merged) {
        if (evt.tick > prev_tick) {
            let sec_per_tick = float(tempo_us) / 1000000.0 / float(midi.ticks_per_qn)
            total_time += float(evt.tick - prev_tick) * sec_per_tick
        }
        prev_tick = evt.tick
        if (evt.kind == MidiEventKind.midi_tempo && evt.tempo > 0) {
            tempo_us = evt.tempo
        }
    }

Part B: MIDI Playback
=====================

The MIDI player uses the audio system's streaming infrastructure and
built-in piano and drum sample banks.  Three functions manage its
lifecycle:

- ``midi_load_samples(media_path)`` — loads the sample banks from the
  media directory
- ``midi_init()`` — starts the MIDI playback thread
- ``midi_shutdown()`` — stops the thread and releases resources

``midi_play`` starts a named MIDI track.  The name is an arbitrary string
that identifies the track for later control.  Multiple tracks can play
simultaneously:

.. code-block:: das

    require strudel/strudel_midi_player

    midi_load_samples(media_path)
    midi_init()
    midi_play("music", "fur_elise.mid", [gain = 0.8, looping = true])
    sleep(5000u)
    midi_stop("music")
    midi_shutdown()

Part C: Cross-fading
====================

Because tracks are named and independent, you can run two MIDI files at
once and cross-fade between them.  ``midi_set_volume`` smoothly
transitions a track's volume over a specified fade time in seconds:

.. code-block:: das

    // Start both — track_a at full volume, track_b silent
    midi_play("track_a", fur_elise, [gain = 1.0, looping = true])
    midi_play("track_b", bach_air, [gain = 0.0, looping = true])

    // Cross-fade A -> B over 3 seconds
    midi_set_volume("track_a", 0.0, 3.0)
    midi_set_volume("track_b", 1.0, 3.0)
    sleep(5000u)

    // Cross-fade back B -> A
    midi_set_volume("track_a", 1.0, 3.0)
    midi_set_volume("track_b", 0.0, 3.0)
    sleep(5000u)

    midi_stop()   // stop all tracks
    midi_shutdown()

This pattern is common in games for transitioning between exploration and
combat music without an audible cut.

Running the Tutorial
====================

Run from the project root::

   daslang.exe tutorials/dasAudio/08_midi.das

Part A prints the MIDI file structure, the first 20 merged events, and the
computed duration.  Part B plays Fur Elise for 5 seconds.  Part C
cross-fades between Fur Elise and Bach's Air on the G String, fading each
direction over 3 seconds.

.. seealso::

   Full source: :download:`tutorials/dasAudio/08_midi.das <../../../../tutorials/dasAudio/08_midi.das>`

   Audio module reference: :ref:`stdlib_audio`
