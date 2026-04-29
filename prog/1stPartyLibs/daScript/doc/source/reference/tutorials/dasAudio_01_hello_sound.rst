.. _tutorial_dasAudio_hello_sound:

===========================
AUDIO-01 --- Hello Sound
===========================

.. index::
    single: Tutorial; Audio
    single: Tutorial; PCM
    single: Tutorial; dasAudio

This tutorial covers initializing the audio system, generating PCM samples
in daslang, and playing them back through the speakers.

Setup
=====

Import the ``audio_boost`` module and ``math`` for trigonometry::

   require audio/audio_boost
   require math

``audio_boost`` re-exports the low-level audio bindings and provides
convenience functions for playback, volume, panning, and 3D spatialization.

Audio System Lifecycle
======================

All audio operations must happen inside ``with_audio_system``.  This block
initializes the miniaudio engine on entry and shuts it down on exit:

.. code-block:: das

   with_audio_system() {
       // ... play sounds here ...
   }

Any ``play_*``, ``set_*``, or ``stop`` call outside this block is a no-op.

Generating PCM Samples
======================

The audio engine works with floating-point PCM data in the range **-1.0** to
**1.0**.  One second of audio at the system sample rate (``MA_SAMPLE_RATE``,
typically 48000 Hz) is simply an ``array<float>`` with that many elements.

A pure sine tone at 440 Hz (concert A):

.. code-block:: das

   var samples <- [for (x in range(MA_SAMPLE_RATE));
       sin(2.0 * PI * 440.0 * float(x) / float(MA_SAMPLE_RATE))]

The comprehension produces exactly one second of samples.  Each element is
the sine of the phase at that sample index, scaled to the desired frequency.

Playing Sound
=============

``play_sound_from_pcm`` takes ownership of the sample array (move semantics)
and returns a **SID** (sound identifier) --- an opaque ``uint64`` handle used
to control the sound later:

.. code-block:: das

   let sid = play_sound_from_pcm(MA_SAMPLE_RATE, 1, samples)
   //                            rate          channels  data (moved)

The sound begins playing immediately.  Once it reaches the end of the
buffer it stops automatically.

Frequency Variations
====================

Different musical notes correspond to different frequencies.  Here are
middle C (C4, 261 Hz) and C5 (523 Hz, one octave higher):

.. code-block:: das

   // Middle C
   var samples_c4 <- [for (x in range(MA_SAMPLE_RATE));
       sin(2.0 * PI * 261.0 * float(x) / float(MA_SAMPLE_RATE))]
   let sid_c4 = play_sound_from_pcm(MA_SAMPLE_RATE, 1, samples_c4)

   // One octave up
   var samples_c5 <- [for (x in range(MA_SAMPLE_RATE));
       sin(2.0 * PI * 523.0 * float(x) / float(MA_SAMPLE_RATE))]
   let sid_c5 = play_sound_from_pcm(MA_SAMPLE_RATE, 1, samples_c5)

Doubling the frequency raises the pitch by one octave.

Waveform Variations
===================

A **square wave** has a harsher, buzzier timbre than a sine wave.  Create
one by taking the sign of a sine wave.  Multiply by 0.3 to reduce volume
(square waves are perceptually louder at the same amplitude):

.. code-block:: das

   var samples <- [for (x in range(MA_SAMPLE_RATE));
       sign(sin(2.0 * PI * 440.0 * float(x) / float(MA_SAMPLE_RATE))) * 0.3]
   let sid = play_sound_from_pcm(MA_SAMPLE_RATE, 1, samples)

Quick Reference
===============

==============================  =========================================================
Function                        Description
==============================  =========================================================
``with_audio_system``           Initialize and shut down the audio engine
``play_sound_from_pcm``         Play a one-shot sound from an ``array<float>``
``MA_SAMPLE_RATE``              System sample rate (typically 48000)
``sleep``                       Wait for a duration (milliseconds, ``uint``)
==============================  =========================================================

Running the Tutorial
====================

::

   daslang.exe tutorials/dasAudio/01_hello_sound.das

You will hear three tones played sequentially: a 440 Hz sine wave, middle C
and C5, then a 440 Hz square wave.  Each tone lasts approximately one
second.

.. seealso::

   Full source: :download:`tutorials/dasAudio/01_hello_sound.das <../../../../tutorials/dasAudio/01_hello_sound.das>`

   Next tutorial: :ref:`tutorial_dasAudio_playing_files`
