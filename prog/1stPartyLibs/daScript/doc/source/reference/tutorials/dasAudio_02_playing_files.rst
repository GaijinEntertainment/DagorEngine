.. _tutorial_dasAudio_playing_files:

====================================
AUDIO-02 --- Playing Audio Files
====================================

.. index::
    single: Tutorial; Audio Files
    single: Tutorial; MP3
    single: Tutorial; Decoding

This tutorial covers loading audio from files on disk, decoding compressed
formats to PCM, and looping playback.

Playing from File
=================

``play_sound_from_file`` loads and decodes a file directly on the audio
thread.  It supports WAV, MP3, and FLAC.  You specify the expected sample
rate and channel count:

.. code-block:: das

   let path = "{get_das_root()}/examples/media/audio/gong.wav"
   let sid = play_sound_from_file(path, MA_SAMPLE_RATE, 2)

The function returns a SID immediately.  Decoding happens in the background
--- there may be a brief delay before audio is audible for large files.

Decoding Audio
==============

For more control, decode the file to an ``array<float>`` of interleaved PCM
samples using ``decode_audio``.  This lets you inspect properties (channels,
sample rate, duration) before playback, or process the data.

The file must first be loaded into memory with ``fopen`` / ``fmap``:

.. code-block:: das

   var channels : int
   var rate : int
   var inscope samples : array<float>
   fopen(path, "rb") <| $(f) {
       if (f == null) {
           print("ERROR: Could not open file\n")
           return
       }
       fmap(f) <| $(data) {
           samples <- decode_audio(data, channels, rate)
       }
   }

``decode_audio`` takes the raw file bytes (via ``fmap``) and writes the
channel count and sample rate to out-parameters.  The returned array
contains interleaved samples --- for stereo audio, samples alternate
left/right.

After decoding you can compute the duration:

.. code-block:: das

   let num_frames = length(samples) / channels
   let duration = float(num_frames) / float(rate)

Then play with ``play_sound_from_pcm`` as in the previous tutorial:

.. code-block:: das

   let sid = play_sound_from_pcm(rate, channels, samples)

Looping Playback
================

``play_sound_loop_from_pcm`` plays samples in an infinite loop until
explicitly stopped.  This is useful for background music, ambient sounds,
or drones:

.. code-block:: das

   let half_second = MA_SAMPLE_RATE / 2
   var samples <- [for (x in range(half_second));
       sin(2.0 * PI * 330.0 * float(x) / float(MA_SAMPLE_RATE))]
   let sid = play_sound_loop_from_pcm(MA_SAMPLE_RATE, 1, samples)

   // Let it loop for 2 seconds, then stop
   sleep(2000u)
   stop(sid, 0.0)

``stop(sid, 0.0)`` stops the sound immediately.  A non-zero second argument
fades out over that many seconds (covered in the next tutorial).

Running the Tutorial
====================

::

   daslang.exe tutorials/dasAudio/02_playing_files.das

The tutorial plays a WAV file from disk, decodes and plays an MP3 (first 3
seconds), then loops a short generated tone for 2 seconds before stopping.

.. seealso::

   Full source: :download:`tutorials/dasAudio/02_playing_files.das <../../../../tutorials/dasAudio/02_playing_files.das>`

   Next tutorial: :ref:`tutorial_dasAudio_sound_control`
