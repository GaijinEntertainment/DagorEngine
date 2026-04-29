.. _tutorial_dasAudio_sound_control:

==============================
AUDIO-03 --- Sound Control
==============================

.. index::
    single: Tutorial; Volume
    single: Tutorial; Pan
    single: Tutorial; Pitch

This tutorial covers runtime control of playing sounds: volume, pan, pitch,
pause/resume, command batching, and stopping with a fade.

All examples operate on a looping 440 Hz tone created at startup:

.. code-block:: das

   var tone <- [for (x in range(MA_SAMPLE_RATE));
       sin(2.0 * PI * 440.0 * float(x) / float(MA_SAMPLE_RATE))]
   let sid = play_sound_loop_from_pcm(MA_SAMPLE_RATE, 1, tone)

Volume
======

``set_volume(sid, volume, time)`` sets the volume of a playing sound.
Volume **1.0** is full and **0.0** is silent.

When ``time`` is **0.0** the change is instant:

.. code-block:: das

   set_volume(sid, 0.3, 0.0)   // quiet, instant
   set_volume(sid, 1.0, 0.0)   // full, instant

When ``time`` is greater than zero the volume fades smoothly over that many
seconds --- useful for crossfades and fade-outs:

.. code-block:: das

   set_volume(sid, 0.1, 2.0)   // fade to 0.1 over 2 seconds
   set_volume(sid, 1.0, 0.5)   // fade back to full over 0.5 seconds

Pan
===

``set_pan(sid, pan)`` positions the sound in the stereo field:

- **-1.0** = full left
- **0.0** = center (default)
- **1.0** = full right

.. code-block:: das

   set_pan(sid, -1.0)   // left
   set_pan(sid,  1.0)   // right
   set_pan(sid,  0.0)   // center

Pitch
=====

``set_pitch(sid, pitch)`` changes the playback speed and pitch
simultaneously.  The value is a multiplier:

- **1.0** = normal
- **2.0** = one octave up (double speed)
- **0.5** = one octave down (half speed)

.. code-block:: das

   set_pitch(sid, 2.0)   // one octave up
   set_pitch(sid, 0.5)   // one octave down
   set_pitch(sid, 1.0)   // normal

Pause and Resume
================

``set_pause(sid, paused)`` pauses or resumes a sound.  The sound picks up
exactly where it left off:

.. code-block:: das

   set_pause(sid, true)    // pause
   set_pause(sid, false)   // resume

Command Batching
================

``batch`` groups multiple audio commands into a single atomic update.  All
changes inside the block take effect together on the audio thread, avoiding
audible glitches from applying them one by one:

.. code-block:: das

   batch() {
       set_volume(sid, 0.5, 0.0)
       set_pan(sid, -0.5)
       set_pitch(sid, 1.5)
   }

Stop with Fade
==============

``stop(sid, time)`` stops a sound.  When ``time`` is greater than zero the
sound fades out over that many seconds before stopping, avoiding audible
clicks:

.. code-block:: das

   stop(sid, 1.0)   // fade out over 1 second, then stop

Quick Reference
===============

==============================  =========================================================
Function                        Description
==============================  =========================================================
``set_volume(sid, vol, time)``  Set volume (0.0--1.0); fade over ``time`` seconds
``set_pan(sid, pan)``           Stereo position (-1.0 left, 0.0 center, 1.0 right)
``set_pitch(sid, pitch)``       Playback speed multiplier (1.0 = normal)
``set_pause(sid, paused)``      Pause (``true``) or resume (``false``)
``batch``                       Group commands into one atomic audio-thread update
``stop(sid, time)``             Stop a sound; fade out over ``time`` seconds
==============================  =========================================================

Running the Tutorial
====================

::

   daslang.exe tutorials/dasAudio/03_sound_control.das

The tutorial creates a looping tone and walks through each control in
sequence: volume changes, fades, panning left/right, pitch shifts,
pause/resume, batching, and a final fade-out stop.

.. seealso::

   Full source: :download:`tutorials/dasAudio/03_sound_control.das <../../../../tutorials/dasAudio/03_sound_control.das>`

   Next tutorial: :ref:`tutorial_dasAudio_spatial_audio`
