"use strict";

var GE_nodeDescriptionsAdditional =
[
  {
    name:"density output",
    category:"Output",
    synonyms:"density, output, result",
    pins:[
      {name:"value", caption:"density", types:["float"], singleConnect:true, role:"in", data:{code:"density = $value$"}},
    ],
    properties:[],
    allowLoop:false,
    width:200
  },
   {
    name:"height fraction",
    category:"Input",
    synonyms:"height, fraction, altitude",
    pins:[
      {name:"value", caption:"height fraction", types:["float"], singleConnect:false, role:"out", data:{code:"height_fraction"}}
    ],
    properties:[],
    allowLoop:false,
    width:200
  },
  {
    name:"sample weather",
    category:"Clouds",
    synonyms:"weather, coverage, type",
    pins:[
      {name:"world_xz", caption:"world_xz", types:["float2"], singleConnect:true, role:"in", data:{def_val:"world_pos.xz"}},
      {name:"covarage__rain", caption:"covarage__rain", types:["float3"], singleConnect:true, role:"in", hidden:true, makeInternalVar:true},
      {name:"cloud_type", caption:"cloud type", types:["float"], singleConnect:false, role:"out", data:{code:"nbs_sample_weather($world_xz$, $covarage__rain$)"}},
      {name:"rain", caption:"rain", types:["float"], singleConnect:false, role:"out", data:{code:"$covarage__rain$.z"}},
      {name:"covarage", caption:"covarage", types:["float2"], singleConnect:false, role:"out", data:{code:"$covarage__rain$.xy"}}
    ],
    properties:[],
    allowLoop:false,
    width:220
  },
  {
    name:"sample cloud shape",
    category:"Clouds",
    synonyms:"shape, perlin, worley, base",
    pins:[
      {name:"world_pos", caption:"world_pos", types:["float3"], singleConnect:true, role:"in", data:{def_val:"world_pos"}},
      {name:"scale", caption:"scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"clouds_shape_scale"}},
      {name:"value", caption:"shape", types:["float"], singleConnect:false, role:"out", data:{code:"nbs_sample_cloud_shape($world_pos$, $scale$)"}}
    ],
    properties:[],
    allowLoop:false,
    width:220
  },
  {
    name:"sample cloud detail",
    category:"Clouds",
    synonyms:"detail, erosion, worley",
    pins:[
      {name:"world_pos", caption:"world_pos", types:["float3"], singleConnect:true, role:"in", data:{def_val:"world_pos"}},
      {name:"scale", caption:"scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"1.0"}},
      {name:"value", caption:"detail", types:["float"], singleConnect:false, role:"out", data:{code:"nbs_sample_cloud_detail($world_pos$, $scale$)"}}
    ],
    properties:[],
    allowLoop:false,
    width:220
  },
  {
    name:"sample curl 2d",
    category:"Clouds",
    synonyms:"curl, turbulence, swirl",
    pins:[
      {name:"world_xz", caption:"world_pos", types:["float3"], singleConnect:true, role:"in", data:{def_val:"world_pos.xz"}},
      {name:"scale", caption:"scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"clouds_turbulence_freq"}},
      {name:"value", caption:"curl", types:["float2"], singleConnect:false, role:"out", data:{code:"nbs_sample_curl_2d($world_xz$, $scale$)"}}
    ],
    properties:[],
    allowLoop:false,
    width:220
  },
  {
    name:"sample types lut",
    category:"Clouds",
    synonyms:"lut, type, height gradient, erosion",
    pins:[
      {name:"height_fraction", caption:"height_frac", types:["float"], singleConnect:true, role:"in", data:{def_val:"height_fraction"}},
      {name:"cloud_type", caption:"cloud_type", types:["float"], singleConnect:true, role:"in", data:{def_val:"0.0"}},
      {name:"grad", caption:"grad", types:["float2"], singleConnect:false, role:"out", data:{code:"nbs_sample_types_lut($height_fraction$, $cloud_type$)"}}
    ],
    properties:[],
    allowLoop:false,
    width:220
  },
  {
    name:"remap shape by coverage",
    category:"Clouds",
    synonyms:"remap, coverage, shape",
    pins:[
      {name:"shape", caption:"shape", types:["float"], singleConnect:true, role:"in", data:{def_val:"0.0"}},
      {name:"coverage", caption:"coverage", types:["float"], singleConnect:true, role:"in", data:{def_val:"0.5"}},
      {name:"value", caption:"result", types:["float"], singleConnect:false, role:"out", data:{code:"nbs_remap_shape_coverage($shape$, $coverage$)"}}
    ],
    properties:[],
    allowLoop:false,
    width:220
  },
  {
    name:"remap",
    category:"Clouds",
    synonyms:"remap, range",
    pins:[
      {name:"value", caption:"value", types:["float"], singleConnect:true, role:"in", data:{def_val:"0.0"}},
      {name:"old_min", caption:"old_min", types:["float"], singleConnect:true, role:"in", data:{def_val:"0.0"}},
      {name:"old_max", caption:"old_max", types:["float"], singleConnect:true, role:"in", data:{def_val:"1.0"}},
      {name:"new_min", caption:"new_min", types:["float"], singleConnect:true, role:"in", data:{def_val:"0.0"}},
      {name:"new_max", caption:"new_max", types:["float"], singleConnect:true, role:"in", data:{def_val:"1.0"}},
      {name:"value_out", caption:"result", types:["float"], singleConnect:false, role:"out", data:{code:"nbs_remap($value$, $old_min$, $old_max$, $new_min$, $new_max$)"}}
    ],
    properties:[],
    allowLoop:false,
    width:220
  },
  {
    name:"clouds form variable",
    category:"Clouds",
    synonyms:"variable, form, density, layer",
    pins:[
      {name:"value", caption:"%name%", types:["float"], singleConnect:false, role:"out", data:{code:"%name%"}}
    ],
    properties:[
      {name:"name", type:"combobox", items:["clouds_first_layer_density","clouds_second_layer_density","cloud_shape_meter_scale",
      "cloud_cumulonimbus_shape_meter_scale","clouds_turbulence_freq","clouds_turbulence_amplitude","clouds_thickness2","clouds_start_altitude2",
       "weather_size", "inv_weather_size", "height_fraction", "clouds_turbulence_scale"], val:"clouds_first_layer_density"}
    ],
    allowLoop:false,
    width:240
  },
];

var GE_defaultExternalsAdditional = [];
