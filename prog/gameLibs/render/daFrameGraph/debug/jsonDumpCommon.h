// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <frontend/internalRegistry.h>
#include <backend/intermediateRepresentation.h>
#include <backend/nodeStateDeltas.h>
#include <backend/resourceScheduling/barrierScheduler.h>
#include <backend/resourceScheduling/heapLayout.h>

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>


namespace dafg::jsondump
{

void dump_registry_to_json(const InternalRegistry &registry);

void dump_ir_graph_to_json(const InternalRegistry &registry, const intermediate::Graph &graph);

void dump_state_deltas_to_json(const intermediate::Graph &graph, const sd::NodeStateDeltas &state_deltas);

void dump_events_to_json(const intermediate::Graph &graph, const BarrierScheduler::EventsCollection &events);

void dump_layout_to_json(const intermediate::Graph &graph, const Layout &layout);

void print_shaders_override_state(const shaders::OverrideState &state);

void print_intermediate_vrs_state(const intermediate::VrsState &vrs);

void print_intermediate_vertex_source(const intermediate::VertexSource &vtx_src, const intermediate::Graph &graph);

void print_intermediate_index_source(const intermediate::IndexSource &inx_src, const intermediate::Graph &graph);


template <class EnumType>
inline const char *get_name(const InternalRegistry &reg, EnumType id)
{
  return reg.knownNames.getName(id);
}

inline const char *get_name(const InternalRegistry &reg, ShaderNameId id) { return reg.knownShaders.getNameCstr(id); }

inline const char *get_name(const intermediate::Graph &graph, const intermediate::NodeIndex idx)
{
  return graph.nodeNames.all().isMapped(idx) ? graph.nodeNames.all()[idx].c_str() : "NOT_MAPPED";
}

inline const char *get_name(const intermediate::Graph &graph, const intermediate::ResourceIndex idx)
{
  return graph.resourceNames.isMapped(idx) ? graph.resourceNames[idx].c_str() : "NOT_MAPPED";
}


using Buffer = rapidjson::StringBuffer;
using Writer = rapidjson::PrettyWriter<Buffer>;

struct ObjectHandler
{
  Writer &writer;
  ObjectHandler(Writer &w) : writer(w) { writer.StartObject(); }
  ~ObjectHandler() { writer.EndObject(); }
};
struct ArrayHandler
{
  Writer &writer;
  ArrayHandler(Writer &w) : writer(w) { writer.StartArray(); }
  ~ArrayHandler() { writer.EndArray(); }
};
#define OBJECT_SCOPED auto DAG_CONCAT(oh, __LINE__) = ObjectHandler(w);
#define ARRAY_SCOPED  auto DAG_CONCAT(ah, __LINE__) = ArrayHandler(w);

inline bool write_string(Writer &writer, const String &str) { return writer.String(str.data(), str.length()); }
inline bool write_string(Writer &writer, const eastl::string_view &str) { return writer.String(str.data(), str.size()); }

void write_to_file(const String &filename, const Buffer &buffer);

} // namespace dafg::jsondump
