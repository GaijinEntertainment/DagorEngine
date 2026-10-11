// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "jsonDumpCommon.h"

namespace dafg::jsondump
{

extern Writer w;


void dump_state_deltas_to_json(const intermediate::Graph &graph, const sd::NodeStateDeltas &state_deltas)
{
  w.Key("nodeStateDeltas");
  {
    OBJECT_SCOPED
    for (const auto &[nodeIdx, stateDelta] : state_deltas.enumerate())
    {
      w.Key(get_name(graph, nodeIdx));
      {
        OBJECT_SCOPED

        w.Key("asyncPipelines");
        {
          OBJECT_SCOPED
          if (stateDelta.asyncPipelines)
          {
            w.Key("value");
            w.Bool(*stateDelta.asyncPipelines);
          }
        }

        w.Key("wire");
        {
          OBJECT_SCOPED
          if (stateDelta.wire)
          {
            w.Key("value");
            w.Bool(*stateDelta.wire);
          }
        }

        w.Key("vrs");
        {
          OBJECT_SCOPED
          if (stateDelta.vrs)
            print_intermediate_vrs_state(*stateDelta.vrs);
        }

        w.Key("pass");
        {
          OBJECT_SCOPED
          if (stateDelta.pass)
          {
            w.Key("variant");
            eastl::visit(
              [&](const auto &pass) {
                using T = eastl::decay_t<decltype(pass)>;

                if constexpr (std::is_same_v<T, sd::PassChange>)
                {
                  w.String("PassChange");
                  const sd::PassChange &passChange = pass;

                  w.Key("endAction");
                  {
                    OBJECT_SCOPED

                    w.Key("variant");
                    eastl::visit(
                      [&](const auto &end_action) {
                        using T = eastl::decay_t<decltype(end_action)>;
                        if constexpr (std::is_same_v<T, eastl::monostate>)
                          w.String("<null>");
                        else if constexpr (std::is_same_v<T, sd::FinishRenderPass>)
                          w.String("FinishRenderPass");
                        else if constexpr (std::is_same_v<T, sd::LegacyBackbufferPass>)
                          w.String("LegacyBackbufferPass");
                        else
                          w.String("<unknown>");
                      },
                      passChange.endAction);
                  }

                  w.Key("beginAction");
                  {
                    OBJECT_SCOPED

                    w.Key("variant");
                    eastl::visit(
                      [&](const auto &begin_action) {
                        using T = eastl::decay_t<decltype(begin_action)>;

                        if constexpr (std::is_same_v<T, eastl::monostate>)
                        {
                          w.String("<null>");
                        }

                        else if constexpr (std::is_same_v<T, sd::BeginRenderPass>)
                        {
                          w.String("BeginRenderPass");
                          const sd::BeginRenderPass &beginRP = begin_action;

                          w.Key("rp");
                          w.Uint64(uint64_t(beginRP.rp));

                          w.Key("attachments");
                          {
                            ARRAY_SCOPED
                            for (const auto &att : beginRP.attachments)
                            {
                              OBJECT_SCOPED

                              w.Key("res");
                              w.String(get_name(graph, att.res));

                              w.Key("mipLevel");
                              w.Uint(att.mipLevel);

                              w.Key("layer");
                              w.Uint(att.layer);

                              w.Key("clearValue");
                              {
                                OBJECT_SCOPED

                                w.Key("variant");
                                eastl::visit(
                                  [&](const auto &clear_value) {
                                    using T = eastl::decay_t<decltype(clear_value)>;

                                    if constexpr (std::is_same_v<T, ResourceClearValue>)
                                    {
                                      w.String("ResourceClearValue");
                                      const ResourceClearValue &clearVal = clear_value;

                                      w.Key("asUint");
                                      {
                                        ARRAY_SCOPED
                                        w.Uint(clearVal.asUint[0]);
                                        w.Uint(clearVal.asUint[1]);
                                        w.Uint(clearVal.asUint[2]);
                                        w.Uint(clearVal.asUint[3]);
                                      }
                                    }

                                    else if constexpr (std::is_same_v<T, intermediate::DynamicParameter>)
                                    {
                                      w.String("DynamicParameter");
                                      const intermediate::DynamicParameter &dynParam = clear_value;

                                      w.Key("resource");
                                      w.String(get_name(graph, dynParam.resource));
                                    }

                                    else
                                    {
                                      w.String("<unknown>");
                                    }
                                  },
                                  att.clearValue);
                              }
                            }
                          }

                          w.Key("useVrs");
                          w.Bool(beginRP.useVrs);
                        }

                        else if constexpr (std::is_same_v<T, sd::LegacyBackbufferPass>)
                        {
                          w.String("LegacyBackbufferPass");
                        }

                        else
                        {
                          w.String("<unknown>");
                        }
                      },
                      passChange.beginAction);
                  }
                }

                else if constexpr (std::is_same_v<T, sd::NextSubpass>)
                {
                  w.String("NextSubpass");
                }

                else
                {
                  w.String("<unknown>");
                }
              },
              *stateDelta.pass);
          }
        }

        w.Key("shaderOverrides");
        {
          OBJECT_SCOPED
          if (stateDelta.shaderOverrides)
            print_shaders_override_state(shaders::overrides::get(*stateDelta.shaderOverrides));
        }

        w.Key("vertexSources");
        {
          OBJECT_SCOPED
          for (uint32_t i = 0; i < stateDelta.vertexSources.size(); ++i)
          {
            w.Key(String(0, "%d", i));
            {
              OBJECT_SCOPED
              if (stateDelta.vertexSources[i])
                print_intermediate_vertex_source(*stateDelta.vertexSources[i], graph);
            }
          }
        }

        w.Key("indexSource");
        {
          OBJECT_SCOPED
          if (stateDelta.indexSource)
            print_intermediate_index_source(*stateDelta.indexSource, graph);
        }

        w.Key("bindings");
        {
          OBJECT_SCOPED
          for (const auto &[index, binding] : stateDelta.bindings)
          {
            w.Key(String(0, "%d", index));
            {
              OBJECT_SCOPED

              w.Key("type");
              write_string(w, detail::get_enum_name(binding.type));

              w.Key("resource");
              if (binding.resource)
                w.String(get_name(graph, *binding.resource));
              else
                w.String("");

              w.Key("history");
              w.Bool(binding.history);

              w.Key("reset");
              w.Bool(binding.reset);

              w.Key("optional");
              w.Bool(binding.optional);
            }
          }
        }

        w.Key("shaderBlockLayers");
        {
          OBJECT_SCOPED

          w.Key("objectLayer");
          {
            OBJECT_SCOPED
            if (stateDelta.shaderBlockLayers.objectLayer)
            {
              w.Key("value");
              w.Int(*stateDelta.shaderBlockLayers.objectLayer);
            }
          }

          w.Key("frameLayer");
          {
            OBJECT_SCOPED
            if (stateDelta.shaderBlockLayers.frameLayer)
            {
              w.Key("value");
              w.Int(*stateDelta.shaderBlockLayers.frameLayer);
            }
          }

          w.Key("sceneLayer");
          {
            OBJECT_SCOPED
            if (stateDelta.shaderBlockLayers.sceneLayer)
            {
              w.Key("value");
              w.Int(*stateDelta.shaderBlockLayers.sceneLayer);
            }
          }
        }
      }
    }
  }
}

} // namespace dafg::jsondump
