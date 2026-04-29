.. _tutorial_dasAudio_streaming:

==============================
AUDIO-06 — Streaming Audio
==============================

.. index::
    single: Tutorial; Streaming
    single: Tutorial; PCM Stream
    single: Tutorial; Real-time Audio

This tutorial covers real-time PCM streaming — generating and feeding
audio data in small chunks while it plays.  It also introduces the
collected audio system for long-running applications.

PCM Streaming
=============

Previous tutorials used ``play_sound_from_pcm``, which requires all
samples up front.  ``play_sound_from_pcm_stream`` creates an open channel
that you feed incrementally with ``append_to_pcm``.  This is essential for:

- Real-time synthesis (generating audio on the fly)
- Network audio (receiving chunks over time)
- Procedural music (composing while playing)

.. code-block:: das

    let sid = play_sound_from_pcm_stream(MA_SAMPLE_RATE, 1)

The returned sound ID is immediately valid — the audio engine starts
reading from the stream as soon as data is available.

Generating Chunks
=================

Feed audio by calling ``append_to_pcm`` repeatedly, passing the sound ID
and an ``array<float>`` of samples.  Each chunk can be any size; the audio
engine buffers them internally.  Maintain phase continuity between chunks
to avoid clicks at chunk boundaries:

.. code-block:: das

    var phase = 0.0
    for (i in range(num_chunks)) {
        let freq = 440.0
        var samples <- [for (x in range(chunk_samples));
            sin(phase + 2.0 * PI * freq * float(x) / float(MA_SAMPLE_RATE)) * 0.5
        ]
        phase += 2.0 * PI * freq * float(chunk_samples) / float(MA_SAMPLE_RATE)
        append_to_pcm(sid, samples)
        sleep(uint(chunk_duration * 1000.0))
    }

Frequency Sweep
===============

The tutorial demonstrates streaming with a sine wave that sweeps from
220 Hz to 880 Hz over 3 seconds.  The frequency is interpolated across
30 chunks of 100 ms each, and phase is accumulated so the waveform
remains continuous:

.. code-block:: das

    let start_freq = 220.0
    let end_freq = 880.0

    for (i in range(num_chunks)) {
        let t = float(i) / float(num_chunks)
        let freq = start_freq + (end_freq - start_freq) * t
        // ... generate chunk at `freq`, advance phase ...
        append_to_pcm(sid, samples)
    }

When the sweep is finished, ``stop(sid, 0.1)`` fades out and releases the
stream.

Collected Audio System
======================

``with_audio_system`` is the simplest way to initialise audio, but for
long-running applications (games, tools) where sounds are created and
destroyed over time, ``with_collected_audio_system`` adds garbage collection
for audio resources.  When using it, call ``audio_system_collect``
periodically to process deferred cleanup:

.. code-block:: das

    with_collected_audio_system() {
        let sid = play_sound_from_pcm_stream(MA_SAMPLE_RATE, 1)
        for (i in range(10)) {
            var samples <- [for (x in range(chunk_samples));
                sin(2.0 * PI * 440.0 * float(x) / float(MA_SAMPLE_RATE)) * 0.4
            ]
            append_to_pcm(sid, samples)
            audio_system_collect()
            sleep(100u)
        }
        stop(sid, 0.1)
        audio_system_collect()
    }

Running the Tutorial
====================

Run from the project root::

   daslang.exe tutorials/dasAudio/06_streaming.das

The tutorial plays a 3-second frequency sweep (220 Hz to 880 Hz), printing
the current frequency every 10 chunks, then demonstrates the collected
audio system with a short ascending tone sequence.

.. seealso::

   Full source: :download:`tutorials/dasAudio/06_streaming.das <../../../../tutorials/dasAudio/06_streaming.das>`

   Next tutorial: :ref:`tutorial_dasAudio_wav_io`
