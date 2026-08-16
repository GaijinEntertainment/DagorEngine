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
  {
    name: "sample perlin_worley 3d noise (low frequency)",
    category: "Noise",
    synonyms: "perlin, worley, noise, 3d, low",
    pins:[
      {name: "uvw", types:["float3"], singleConnect:true, role:"in"},
      {name: "noise value", types:["float"], singleConnect:false, role:"out", data:{code:"tex3Dlod(gen_cloud_shape, float4($uvw$, 0)).x"}}
    ],
    properties:[],
    allowLoop:false,
    width:200
  },
  {
    name: "sample worley 3d noise (high frequency)",
    category: "Noise",
    synonyms: "worley, noise, 3d, high",
    pins:[
      {name: "uvw", types:["float3"], singleConnect:true, role:"in"},
      {name: "noise value", types:["float"], singleConnect:false, role:"out", data:{code:"tex3Dlod(gen_cloud_detail, float4($uvw$, 0)).x"}}
    ],
    properties:[],
    allowLoop:false,
    width:200
  },
  {
    name: "sample vector 2d noise",
    category: "Noise",
    synonyms: "vector, noise, 2d, high",
    pins:[
      {name: "uv", types:["float2"], singleConnect:true, role:"in"},
      {name: "noise vector", types:["float2"], singleConnect:false, role:"out", data:{code:"tex2Dlod(clouds_curl_2d, float4($uv$, 0, 0)).xy"}}
    ],
    properties:[],
    allowLoop:false,
    width:200
  },
  {
    name: "perlin 3d noise",
    category: "Noise",
    synonyms: "perlin, noise, 3d, procedural, signed",
    pins:[
      {name: "uvw", types:["float3"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "noise value", types:["float"], singleConnect:false, role:"out", data:{code:"perlin($uvw$, $scale$, bool3(%tile x%, %tile y%, %tile z%))"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"},
      {name:"tile x", type:"bool", val:"false"},
      {name:"tile y", type:"bool", val:"false"},
      {name:"tile z", type:"bool", val:"false"}
    ],
    allowLoop:false,
    width:220
  },
  {
    name: "perlin 2d noise",
    category: "Noise",
    synonyms: "perlin, noise, 2d, procedural, signed",
    pins:[
      {name: "uv", types:["float2"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "noise value", types:["float"], singleConnect:false, role:"out", data:{code:"perlin($uv$, $scale$, bool2(%tile x%, %tile y%))"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"},
      {name:"tile x", type:"bool", val:"false"},
      {name:"tile y", type:"bool", val:"false"}
    ],
    allowLoop:false,
    width:220
  },
  {
    name: "perlin 3d noise with derivative",
    category: "Noise",
    synonyms: "perlin, noise, 3d, procedural, derivative, gradient",
    pins:[
      {name: "uvw", types:["float3"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "result", caption:"x: value, yzw: deriv", types:["float4"], singleConnect:false, role:"out", data:{code:"perlin_deriv($uvw$, $scale$, bool3(%tile x%, %tile y%, %tile z%))"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"},
      {name:"tile x", type:"bool", val:"false"},
      {name:"tile y", type:"bool", val:"false"},
      {name:"tile z", type:"bool", val:"false"}
    ],
    allowLoop:false,
    width:240
  },
  {
    name: "perlin 2d noise with derivative",
    category: "Noise",
    synonyms: "perlin, noise, 2d, procedural, derivative, gradient",
    pins:[
      {name: "uv", types:["float2"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "result", caption:"x: value, yz: deriv", types:["float3"], singleConnect:false, role:"out", data:{code:"perlin_deriv($uv$, $scale$, bool2(%tile x%, %tile y%))"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"},
      {name:"tile x", type:"bool", val:"false"},
      {name:"tile y", type:"bool", val:"false"}
    ],
    allowLoop:false,
    width:240
  },
  {
    name: "voronoi 3d noise",
    category: "Noise",
    synonyms: "voronoi, worley, cellular, noise, 3d, procedural",
    pins:[
      {name: "uvw", types:["float3"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "result", caption:"x: F1sq, y: F2sq, z: id", types:["float3"], singleConnect:false, role:"out", data:{code:"voronoi($uvw$, $scale$, %inverted%)"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"},
      {name:"inverted", type:"bool", val:"false"}
    ],
    allowLoop:false,
    width:240
  },
  {
    name: "voronoi 2d noise",
    category: "Noise",
    synonyms: "voronoi, worley, cellular, noise, 2d, procedural",
    pins:[
      {name: "uv", types:["float2"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "result", caption:"x: F1sq, y: F2sq, z: id", types:["float3"], singleConnect:false, role:"out", data:{code:"voronoi($uv$, $scale$, %inverted%)"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"},
      {name:"inverted", type:"bool", val:"false"}
    ],
    allowLoop:false,
    width:240
  },
  {
    name: "worley 3d noise",
    category: "Noise",
    synonyms: "worley, cellular, octaves, noise, 3d, procedural",
    pins:[
      {name: "uvw", types:["float3"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "noise value", types:["float"], singleConnect:false, role:"out", data:{code:"get_worley($uvw$, $scale$)"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"}
    ],
    allowLoop:false,
    width:220
  },
  {
    name: "worley 2d noise",
    category: "Noise",
    synonyms: "worley, cellular, octaves, noise, 2d, procedural",
    pins:[
      {name: "uv", types:["float2"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "noise value", types:["float"], singleConnect:false, role:"out", data:{code:"get_worley($uv$, $scale$)"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"}
    ],
    allowLoop:false,
    width:220
  },
  {
    name: "worley 3d - 2 octaves",
    category: "Noise",
    synonyms: "worley, cellular, octaves, noise, 3d, procedural",
    pins:[
      {name: "uvw", types:["float3"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "noise value", types:["float"], singleConnect:false, role:"out", data:{code:"get_worley_2_octaves($uvw$, $scale$)"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"}
    ],
    allowLoop:false,
    width:220
  },
  {
    name: "worley 2d - 2 octaves",
    category: "Noise",
    synonyms: "worley, cellular, octaves, noise, 2d, procedural",
    pins:[
      {name: "uv", types:["float2"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "noise value", types:["float"], singleConnect:false, role:"out", data:{code:"get_worley_2_octaves($uv$, $scale$)"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"}
    ],
    allowLoop:false,
    width:220
  },
  {
    name: "worley 3d - 3 octaves",
    category: "Noise",
    synonyms: "worley, cellular, octaves, noise, 3d, procedural",
    pins:[
      {name: "uvw", types:["float3"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "noise value", types:["float"], singleConnect:false, role:"out", data:{code:"get_worley_3_octaves($uvw$, $scale$)"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"}
    ],
    allowLoop:false,
    width:220
  },
  {
    name: "worley 2d - 3 octaves",
    category: "Noise",
    synonyms: "worley, cellular, octaves, noise, 2d, procedural",
    pins:[
      {name: "uv", types:["float2"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "noise value", types:["float"], singleConnect:false, role:"out", data:{code:"get_worley_3_octaves($uv$, $scale$)"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"}
    ],
    allowLoop:false,
    width:220
  },
  {
    name: "perlin fbm 3 octaves",
    category: "Noise",
    synonyms: "perlin, fbm, octaves, noise, 3d, procedural",
    pins:[
      {name: "uvw", types:["float3"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "frequency factor", types:["float"], singleConnect:true, role:"in", data:{def_val:"%frequency factor%"}},
      {name: "amplitude factor", types:["float"], singleConnect:true, role:"in", data:{def_val:"%amplitude factor%"}},
      {name: "noise value", types:["float"], singleConnect:false, role:"out", data:{code:"get_perlin_3_octaves($uvw$, $scale$, bool3(%tile x%, %tile y%, %tile z%), $frequency factor$, $amplitude factor$)"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"},
      {name:"frequency factor", type:"float", minVal:"0", maxVal:"8", step:"0.01", val:"2"},
      {name:"amplitude factor", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"},
      {name:"tile x", type:"bool", val:"false"},
      {name:"tile y", type:"bool", val:"false"},
      {name:"tile z", type:"bool", val:"false"}
    ],
    allowLoop:false,
    width:240
  },
  {
    name: "perlin fbm 3 octaves 2d",
    category: "Noise",
    synonyms: "perlin, fbm, octaves, noise, 2d, procedural",
    pins:[
      {name: "uv", types:["float2"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "frequency factor", types:["float"], singleConnect:true, role:"in", data:{def_val:"%frequency factor%"}},
      {name: "amplitude factor", types:["float"], singleConnect:true, role:"in", data:{def_val:"%amplitude factor%"}},
      {name: "noise value", types:["float"], singleConnect:false, role:"out", data:{code:"get_perlin_3_octaves($uv$, $scale$, bool2(%tile x%, %tile y%), $frequency factor$, $amplitude factor$)"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"},
      {name:"frequency factor", type:"float", minVal:"0", maxVal:"8", step:"0.01", val:"2"},
      {name:"amplitude factor", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"},
      {name:"tile x", type:"bool", val:"false"},
      {name:"tile y", type:"bool", val:"false"}
    ],
    allowLoop:false,
    width:240
  },
  {
    name: "perlin fbm 5 octaves",
    category: "Noise",
    synonyms: "perlin, fbm, octaves, noise, 3d, procedural",
    pins:[
      {name: "uvw", types:["float3"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "frequency factor", types:["float"], singleConnect:true, role:"in", data:{def_val:"%frequency factor%"}},
      {name: "amplitude factor", types:["float"], singleConnect:true, role:"in", data:{def_val:"%amplitude factor%"}},
      {name: "noise value", types:["float"], singleConnect:false, role:"out", data:{code:"get_perlin_5_octaves($uvw$, $scale$, bool3(%tile x%, %tile y%, %tile z%), $frequency factor$, $amplitude factor$)"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"},
      {name:"frequency factor", type:"float", minVal:"0", maxVal:"8", step:"0.01", val:"2"},
      {name:"amplitude factor", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"},
      {name:"tile x", type:"bool", val:"false"},
      {name:"tile y", type:"bool", val:"false"},
      {name:"tile z", type:"bool", val:"false"}
    ],
    allowLoop:false,
    width:240
  },
  {
    name: "perlin fbm 5 octaves 2d",
    category: "Noise",
    synonyms: "perlin, fbm, octaves, noise, 2d, procedural",
    pins:[
      {name: "uv", types:["float2"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "frequency factor", types:["float"], singleConnect:true, role:"in", data:{def_val:"%frequency factor%"}},
      {name: "amplitude factor", types:["float"], singleConnect:true, role:"in", data:{def_val:"%amplitude factor%"}},
      {name: "noise value", types:["float"], singleConnect:false, role:"out", data:{code:"get_perlin_5_octaves($uv$, $scale$, bool2(%tile x%, %tile y%), $frequency factor$, $amplitude factor$)"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"},
      {name:"frequency factor", type:"float", minVal:"0", maxVal:"8", step:"0.01", val:"2"},
      {name:"amplitude factor", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"},
      {name:"tile x", type:"bool", val:"false"},
      {name:"tile y", type:"bool", val:"false"}
    ],
    allowLoop:false,
    width:240
  },
  {
    name: "perlin fbm 5 octaves with derivative",
    category: "Noise",
    synonyms: "perlin, fbm, octaves, noise, 3d, procedural, derivative, gradient",
    pins:[
      {name: "uvw", types:["float3"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "result", caption:"x: value, yzw: deriv", types:["float4"], singleConnect:false, role:"out", data:{code:"get_perlin_5_octaves_deriv($uvw$, $scale$, bool3(%tile x%, %tile y%, %tile z%))"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"},
      {name:"tile x", type:"bool", val:"false"},
      {name:"tile y", type:"bool", val:"false"},
      {name:"tile z", type:"bool", val:"false"}
    ],
    allowLoop:false,
    width:240
  },
  {
    name: "perlin fbm 5 octaves with derivative 2d",
    category: "Noise",
    synonyms: "perlin, fbm, octaves, noise, 2d, procedural, derivative, gradient",
    pins:[
      {name: "uv", types:["float2"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "result", caption:"x: value, yz: deriv", types:["float3"], singleConnect:false, role:"out", data:{code:"get_perlin_5_octaves_deriv($uv$, $scale$, bool2(%tile x%, %tile y%))"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"},
      {name:"tile x", type:"bool", val:"false"},
      {name:"tile y", type:"bool", val:"false"}
    ],
    allowLoop:false,
    width:240
  },
  {
    name: "perlin fbm 7 octaves",
    category: "Noise",
    synonyms: "perlin, fbm, octaves, noise, 3d, procedural, tiled",
    pins:[
      {name: "uvw", types:["float3"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "frequency factor", types:["float"], singleConnect:true, role:"in", data:{def_val:"%frequency factor%"}},
      {name: "amplitude factor", types:["float"], singleConnect:true, role:"in", data:{def_val:"%amplitude factor%"}},
      {name: "noise value", types:["float"], singleConnect:false, role:"out", data:{code:"get_perlin_7_octaves($uvw$, $scale$, $frequency factor$, $amplitude factor$)"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"},
      {name:"frequency factor", type:"float", minVal:"0", maxVal:"8", step:"0.01", val:"2"},
      {name:"amplitude factor", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"}
    ],
    allowLoop:false,
    width:240
  },
  {
    name: "perlin fbm 7 octaves 2d",
    category: "Noise",
    synonyms: "perlin, fbm, octaves, noise, 2d, procedural, tiled",
    pins:[
      {name: "uv", types:["float2"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "frequency factor", types:["float"], singleConnect:true, role:"in", data:{def_val:"%frequency factor%"}},
      {name: "amplitude factor", types:["float"], singleConnect:true, role:"in", data:{def_val:"%amplitude factor%"}},
      {name: "noise value", types:["float"], singleConnect:false, role:"out", data:{code:"get_perlin_7_octaves($uv$, $scale$, $frequency factor$, $amplitude factor$)"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"},
      {name:"frequency factor", type:"float", minVal:"0", maxVal:"8", step:"0.01", val:"2"},
      {name:"amplitude factor", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"}
    ],
    allowLoop:false,
    width:240
  },
  {
    name: "curl noise 3d",
    category: "Noise",
    synonyms: "curl, turbulence, swirl, noise, 3d, procedural, divergence free",
    pins:[
      {name: "uvw", types:["float3"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "curl", types:["float3"], singleConnect:false, role:"out", data:{code:"curl_noise($uvw$, $scale$, bool3(%tile x%, %tile y%, %tile z%))"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"},
      {name:"tile x", type:"bool", val:"true"},
      {name:"tile y", type:"bool", val:"true"},
      {name:"tile z", type:"bool", val:"false"}
    ],
    allowLoop:false,
    width:220
  },
  {
    name: "curl noise 2d",
    category: "Noise",
    synonyms: "curl, turbulence, swirl, noise, 2d, procedural, divergence free",
    pins:[
      {name: "uv", types:["float2"], singleConnect:true, role:"in"},
      {name: "scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"%scale%"}},
      {name: "curl", types:["float2"], singleConnect:false, role:"out", data:{code:"curl_noise($uv$, $scale$, bool2(%tile x%, %tile y%))"}}
    ],
    properties:[
      {name:"scale", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"},
      {name:"tile x", type:"bool", val:"true"},
      {name:"tile y", type:"bool", val:"true"}
    ],
    allowLoop:false,
    width:220
  },
  {
    name: "fbm 5 octaves normalization",
    category: "Noise",
    synonyms: "normalize, fbm, octaves, amplitude, factor",
    pins:[
      {name: "amplitude factor", types:["float"], singleConnect:true, role:"in", data:{def_val:"%amplitude factor%"}},
      {name: "factor", types:["float"], singleConnect:false, role:"out", data:{code:"normalize_5_octaves($amplitude factor$)"}}
    ],
    properties:[
      {name:"amplitude factor", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"}
    ],
    allowLoop:false,
    width:220
  },
];

var GE_defaultExternalsAdditional = [];
