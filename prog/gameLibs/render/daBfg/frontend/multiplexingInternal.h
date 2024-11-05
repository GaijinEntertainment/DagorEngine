// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daBfg/multiplexing.h>
#include <backend/intermediateRepresentation.h>


namespace dabfg
{


intermediate::MultiplexingIndex multiplexing_index_to_ir(multiplexing::Index idx, multiplexing::Extents extents);
uint32_t multiplexing_extents_to_ir(multiplexing::Extents extents);
multiplexing::Index multiplexing_index_from_ir(intermediate::MultiplexingIndex idx, multiplexing::Extents extents);
namespace multiplexing
{
bool mode_has_flag(Mode mode, Mode flag);

bool operator==(const Extents &fst, const Extents &snd);
inline bool operator!=(const Extents &fst, const Extents &snd) { return !(fst == snd); }

bool operator==(const Index &fst, const Index &snd);
inline bool operator!=(const Index &fst, const Index &snd) { return !(fst == snd); }

bool index_inside_extents(Index idx, Extents extents);
// Advanced the index within a multidimensional box of dimensions `extents`
Index next_index(Index idx, Extents extents);
Extents extents_for_node(Mode mode, Extents total_extents);
Index clamp(Index idx, Extents extents);
// Wrap around sub/supersample and clamp viewport, this produces more reasonable history indices when up-multiplexing
Index clamp_and_wrap(Index idx, Extents extents);
bool less_multiplexed(Mode fst, Mode snd, Extents extents);

} // namespace multiplexing

} // namespace dabfg
