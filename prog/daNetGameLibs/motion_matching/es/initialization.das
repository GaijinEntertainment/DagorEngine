require ecs
require MotionMatching
require danetlibs/motion_matching/es/motion_matching_common
require danetlibs/motion_matching/es/mm_events

[es(before=init_animations_es, on_appear, on_event=InvalidateAnimationDataBase)]
def invalidate_anim_data_base(evt : Event;
                              var main_database__loaded : bool&)
  main_database__loaded = false

[es(before=init_animations_es, on_appear, on_event=InvalidateAnimationDataBase)]
def init_anim_data_base_weights(evt : Event;
                                var dataBase : AnimationDataBase;
                                weightsPath : string;
                                rootMotionPath : string;
                                var main_database__root_node : das_string;
                                var main_database__root_motion_a2d_node : das_string;
                                var main_database__direction_nodes : StringList;
                                var main_database__direction_weights : FloatList;
                                var main_database__center_of_mass_nodes : StringList;
                                var main_database__center_of_mass_params : Point4List;
                                var main_database__presetsTagsName : StringList)

  deserialize_presets(weightsPath, dataBase.tagsPresets, dataBase.nodesName, dataBase.predictionTimes, main_database__presetsTagsName)
  deserialize_root_motions(rootMotionPath, main_database__root_node, main_database__root_motion_a2d_node, main_database__direction_nodes,
                           main_database__direction_weights, main_database__center_of_mass_nodes, main_database__center_of_mass_params)

  dataBase.nodeCount = dataBase.nodesName |> length()
  dataBase.trajectorySize = dataBase.predictionTimes |> length()

  dataBase.featuresSize = get_features_sizes(dataBase.nodeCount, dataBase.trajectorySize)
