// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>
#include <EASTL/algorithm.h>
#include <EASTL/vector.h>

namespace voicechat
{

// Hides the step between audio and silence in a played stream. The stream is delayed by the fade length, so the fader
// sees silence before the audio in front of it is played: audio ramps up from zero after silence and down to zero
// before it, whatever the chunk layout. Each ramp takes the fade length, or half of a run shorter than two fades.
class EdgeFader
{
public:
  explicit EdgeFader(unsigned fade_samples) : values(fade_samples, 0), gains(fade_samples, 0) {}

  // pcm holds one chunk: its first `got` samples are audio, the rest is silence. On return every sample of it holds the
  // delayed stream to play
  void apply(dag::Span<int16_t> pcm, unsigned got)
  {
    const unsigned fade = values.size();
    for (unsigned i = 0; i < pcm.size(); ++i)
    {
      if (i >= got && sinceSilence > 0)
        rampDownBefore(pos);
      const int16_t played = int16_t(int(values[pos]) * int(gains[pos]) / int(fade));
      if (i < got)
      {
        values[pos] = pcm[i];
        gains[pos] = eastl::min(sinceSilence++, fade);
      }
      else
      {
        values[pos] = 0;
        gains[pos] = 0;
        sinceSilence = 0;
      }
      pos = (pos + 1) % fade;
      pcm[i] = played;
    }
  }

private:
  // the audio still in the line, `at` included, ramps down to zero next to the silence that enters at `at`; a slot of
  // gain 0 is silence or the first sample of the run, where the ramp stops
  void rampDownBefore(unsigned at)
  {
    const unsigned fade = values.size();
    for (unsigned j = 1; j <= fade; ++j)
    {
      const unsigned idx = (at + fade - j) % fade;
      if (gains[idx] == 0)
        return;
      gains[idx] = eastl::min(gains[idx], j - 1);
    }
  }

  // one slot per sample of the delay; `pos` is the oldest, played and then replaced by the newest
  eastl::vector<int16_t> values;
  eastl::vector<unsigned> gains;
  unsigned pos = 0;
  unsigned sinceSilence = 0;
};

} // namespace voicechat
