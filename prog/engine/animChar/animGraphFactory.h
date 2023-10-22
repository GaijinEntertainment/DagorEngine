#pragma once

namespace AnimV20
{
class GenericAnimStatesGraph;
class ITranslator;
} // namespace AnimV20

AnimV20::GenericAnimStatesGraph *make_graph_by_res_name(const char *graphname);

extern AnimV20::ITranslator *anim_iface_trans;
