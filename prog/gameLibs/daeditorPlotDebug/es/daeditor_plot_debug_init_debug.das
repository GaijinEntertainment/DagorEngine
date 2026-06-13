options no_aot

require ecs
require ecs.common

[es(on_event=EventLevelLoaded, tag=dev)]
def daeditor_plot_debug_ensure(evt : Event)
  if !(find_query() <| $ [es(REQUIRE=daeditor_plot_debug__signalCount)] () => true)
    createEntity("daeditor_plot_debug")

[es(on_appear, tag=dev)]
def daeditor_plot_debug_on_appear(evt : Event;
                                  daeditor_plot_debug__signalCount : int;
                                  var daeditor_plot_debug__signals : FloatList)
  daeditor_plot_debug__signals |> resize(daeditor_plot_debug__signalCount)
