module ecs_plot_registry_common shared
options no_global_variables = false

typedef PlotSampleFn = function<(data : void?;
                                 var dim : int&; var comp_idx : int&;
                                 var rows : int&; var row_idx : int&;
                                 var val : float&) : void>

typedef PlotSetFn    = function<(data : void?;
                                 comp_idx, row_idx : int;
                                 new_val : float) : void>

var private @ignore_heap_usage plot_samplers : table<string; PlotSampleFn>
var private @ignore_heap_usage plot_setters  : table<string; PlotSetFn>

def add_plot_sampler(type_name : string; fn : PlotSampleFn)
  plot_samplers[type_name] = fn

def add_plot_setter(type_name : string; fn : PlotSetFn)
  plot_setters[type_name] = fn

def try_sample_typed(data : void?; type_name : string;
                     var dim : int&; var comp_idx : int&;
                     var rows : int&; var row_idx : int&;
                     var val : float&) : bool
  if data == null
    return false
  rows = 0
  if !(plot_samplers |> key_exists(type_name))
    return false
  invoke(plot_samplers[type_name], data, dim, comp_idx, rows, row_idx, val)
  return true

def try_set_typed(data : void?; type_name : string;
                  comp_idx, row_idx : int; new_val : float) : bool
  if data == null
    return false
  if !(plot_setters |> key_exists(type_name))
    return false
  invoke(plot_setters[type_name], data, comp_idx, row_idx, new_val)
  return true
