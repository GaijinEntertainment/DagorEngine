.. _tutorial_dasAudio_reverb:

================================
AUDIO-05 — Reverb and Effects
================================

.. index::
    single: Tutorial; Reverb
    single: Tutorial; Chorus
    single: Tutorial; Effects
    single: Tutorial; I3DL2

This tutorial covers applying reverb and chorus effects to sounds.
A short impulse burst is played dry, then with several reverb presets,
then with chorus configurations, and finally with both combined.

Reverb Basics
=============

Reverb simulates the acoustic reflections of a physical space.  The dasAudio
module implements the `I3DL2 <https://en.wikipedia.org/wiki/I3DL2>`_ standard,
which defines a set of named presets modelling environments from a small
bathroom to a vast cave.

To hear the effect clearly, the tutorial generates a very short (0.1-second)
decaying sine burst at 880 Hz.  The rapid decay leaves silence for the reverb
tail to ring out:

.. code-block:: das

    def make_impulse() : array<float> {
        let num_samples = MA_SAMPLE_RATE / 10  // 0.1 seconds
        return <- [for (x in range(num_samples));
            sin(2.0 * PI * 880.0 * float(x) / float(MA_SAMPLE_RATE)) * exp(-float(x) / float(num_samples) * 6.0)
        ]
    }

Without reverb the impulse is a short "tick" that ends abruptly.

Applying Presets
================

``set_reverb_preset`` attaches an I3DL2 reverb to a playing sound,
identified by the sound ID returned from ``play_sound_from_pcm``.  The
second argument is a value from the ``ReverbPreset`` enumeration:

.. code-block:: das

    var samples <- make_impulse()
    let sid = play_sound_from_pcm(MA_SAMPLE_RATE, 1, samples)
    set_reverb_preset(sid, ReverbPreset.ConcertHall)

Each preset models a different acoustic space with its own decay time,
high-frequency damping, and early-reflection pattern.

Preset Reference
================

The ``ReverbPreset`` enumeration contains 23 presets:

====================  ============================================
Preset                Character
====================  ============================================
``Generic``           Neutral, medium-sized room
``PaddedCell``        Very small, heavily damped
``Room``              Average room
``Bathroom``          Small, bright reflections, short tail
``LivingRoom``        Soft furnishings, moderate decay
``StoneRoom``         Hard surfaces, clear reflections
``Auditorium``        Large hall, smooth decay
``ConcertHall``       Spacious, warm, long smooth decay
``Cave``              Massive, dark reverb, very long tail
``Arena``             Large open space
``Hangar``            Huge metallic space
``CarpetedHallway``   Damped corridor
``Hallway``           Narrow reflective corridor
``StoneCorridor``     Hard-walled corridor
``Alley``             Outdoor narrow passage
``Forest``            Diffuse outdoor scattering
``City``              Urban reflections
``Mountains``         Distant echoes
``Quarry``            Large outdoor pit
``Plain``             Open flat ground
``ParkingLot``        Concrete enclosure
``SewerPipe``         Cylindrical metallic tube
``Underwater``        Heavy low-pass filter, muffled
====================  ============================================

Chorus
======

Chorus adds a modulated delay to simulate multiple detuned copies of the
sound, producing width and shimmer.  ``set_chorus_default`` applies sensible
defaults; ``set_chorus`` accepts a ``ma_chorus_config`` for fine control:

.. code-block:: das

    // Default chorus
    let sid = play_sound_from_pcm(MA_SAMPLE_RATE, 1, samples)
    set_chorus_default(sid)

    // Custom chorus — slower, deeper
    var cfg = chorus_config_default()
    cfg.rate = 0.5       // LFO rate in Hz
    cfg.depth = 0.8      // modulation depth
    cfg.delay_ms = 15.0  // base delay
    cfg.wet = 0.7        // wet/dry mix
    set_chorus(sid, cfg)

The ``ma_chorus_config`` fields are:

==============  ============================================
Field           Description
==============  ============================================
``rate``        LFO modulation rate in Hz
``depth``       LFO modulation depth (0.0--1.0)
``delay_ms``    Base delay in milliseconds
``feedback``    Feedback amount (0.0--1.0)
``wet``         Wet/dry mix (0.0 = dry, 1.0 = fully wet)
==============  ============================================

Combining Effects
=================

Reverb and chorus can be applied to the same sound — chorus runs after
reverb in the signal chain:

.. code-block:: das

    var samples <- make_impulse()
    let sid = play_sound_from_pcm(MA_SAMPLE_RATE, 1, samples)
    set_reverb_preset(sid, ReverbPreset.ConcertHall)
    set_chorus_default(sid)

Running the Tutorial
====================

Run from the project root::

   daslang.exe tutorials/dasAudio/05_reverb.das

The tutorial plays the impulse dry, with reverb presets, with chorus
variations, and finally with reverb and chorus combined.

.. seealso::

   Full source: :download:`tutorials/dasAudio/05_reverb.das <../../../../tutorials/dasAudio/05_reverb.das>`

   Next tutorial: :ref:`tutorial_dasAudio_streaming`
