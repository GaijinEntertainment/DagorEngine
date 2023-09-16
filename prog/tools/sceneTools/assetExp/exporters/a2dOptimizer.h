#pragma once

#include "a2dKeyTypes.h"

void optimize_keys(dag::Span<ChannelData> chan, float pos_eps, float rot_eps, float scale_eps, int rot_resample_freq);
