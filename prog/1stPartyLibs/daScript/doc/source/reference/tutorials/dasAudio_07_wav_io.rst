.. _tutorial_dasAudio_wav_io:

=============================
AUDIO-07 — WAV File I/O
=============================

.. index::
    single: Tutorial; WAV
    single: Tutorial; File I/O
    single: Tutorial; Audio Processing

This tutorial covers reading and writing WAV files using the
``audio/audio_wav`` module, and demonstrates simple offline audio
processing (reversing and amplifying).

Writing WAV Files
=================

``write_wav`` saves an ``array<float>`` of PCM samples as a 16-bit WAV
file.  Parameters are the output path, sample data, sample rate, and
channel count:

.. code-block:: das

    require audio/audio_wav

    var samples <- [for (x in range(MA_SAMPLE_RATE));
        sin(2.0 * PI * 440.0 * float(x) / float(MA_SAMPLE_RATE)) * 0.8
    ]
    write_wav("tone.wav", samples, uint(MA_SAMPLE_RATE), 1)

The samples are stored as floating-point values in the range -1.0 to 1.0
and quantized to 16-bit integers on disk.

Reading WAV Files
=================

``read_wav`` loads a WAV file back into an ``array<float>``, returning the
sample rate and channel count via out-parameters.  It returns ``true`` on
success:

.. code-block:: das

    var samples : array<float>
    var sample_rate : int
    var channels : int
    if (read_wav("tone.wav", samples, sample_rate, channels)) {
        let duration = float(length(samples)) / float(sample_rate * channels)
        print("{length(samples)} samples, {duration:.2f} seconds\n")
    }

Because the file stores 16-bit integers, the round-tripped float values
will have small quantization differences from the originals.

Simple Processing
=================

With PCM data in an ``array<float>``, any processing is just a loop over
samples.

**Reversing** swaps samples from both ends toward the middle:

.. code-block:: das

    let n = length(samples)
    for (i in range(n / 2)) {
        let tmp = samples[i]
        samples[i] = samples[n - 1 - i]
        samples[n - 1 - i] = tmp
    }
    write_wav("reversed.wav", samples, uint(sample_rate), channels)

**Amplifying** (or attenuating) multiplies every sample by a gain factor:

.. code-block:: das

    for (i in range(length(samples))) {
        samples[i] *= 0.5   // halve volume
    }
    write_wav("quiet.wav", samples, uint(sample_rate), channels)

The tutorial reads a gong sample from the media directory, applies each
transformation, writes the result to a temporary WAV, plays both the
original and processed versions for comparison, then cleans up the
temporary files.

Running the Tutorial
====================

Run from the project root::

   daslang.exe tutorials/dasAudio/07_wav_io.das

The tutorial generates a 440 Hz tone and saves it to WAV, reads it back
and verifies the sample values, then demonstrates reversing and volume
reduction on a gong sample — playing each version so you can hear the
difference.

.. seealso::

   Full source: :download:`tutorials/dasAudio/07_wav_io.das <../../../../tutorials/dasAudio/07_wav_io.das>`

   Next tutorial: :ref:`tutorial_dasAudio_midi`
