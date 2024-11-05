"use strict";


var GE_volfog_early_exit_node_name = "volfog early exit"
var GE_volfog_early_exit_var_name = "_volfog_early_exit"

var GE_typeColors =
  {
    "bool": "#777",
    "int": "#18f",
    "float": "#0a0",
    "float2": "#ff0",
    "float3": "#0ff",
    "float4": "#f0f",
    "texture2D": "#f81",
    "texture3D": "#f81",
    "texture2D_shdArray": "#f81",
    "BiomeData": "#f90"
  };


var GE_conversions =
  [// from      to        user data
    ["int",    "float",  "float($)"],
    ["float",  "int",    "int($)"],
    ["float",  "float2", "float2($, $)"],
    ["float",  "float3", "float3($, $, $)"],
    ["float",  "float4", "float4($, $, $, $)"],
    ["float4", "float3", "$.xyz"],
    ["float4", "float2", "$.xy"],
    ["float3", "float4", "float4($, 0.0)"],
    ["float3", "float2", "$.xy"],
    ["float2", "float4", "float4($, 0.0, 0.0)"],
    ["float2", "float3", "float3($, 0.0)"]
  ];

var GE_defaultValuesZero =
  {// type     user data
    "bool":   "false",
    "int":    "0",
    "float":  "0.0",
    "float2": "float2(0.0, 0.0)",
    "float3": "float3(0.0, 0.0, 0.0)",
    "float4": "float4(0.0, 0.0, 0.0, 0.0)",
    "polynom4":   "float4(0.0, 0.0, 0.0, 0.0)",
    "linear4":    "linear4_zero",
    "monotonic4": "monotonic4_zero",
  };

var GE_defaultValuesOne =
  {// type     user data
    "bool":   "true",
    "int":    "1",
    "float":  "1.0",
    "float2": "float2(1.0, 1.0)",
    "float3": "float3(1.0, 1.0, 1.0)",
    "float4": "float4(1.0, 1.0, 1.0, 1.0)",
    "polynom4":   "float4(1.0, 0.0, 0.0, 0.0)",
    "linear4":    "linear4_one",
    "monotonic4": "monotonic4_one",
  };

var GE_defaultValuesZeroBlk =
  {// type     user data
    "bool":   "false",
    "int":    "0",
    "float":  "0.0",
    "float2": "0.0, 0.0",
    "float3": "0.0, 0.0, 0.0",
    "float4": "0.0, 0.0, 0.0, 0.0",
    "polynom4":   "0.0, 0.0, 0.0, 0.0",
//    "linear4":    "linear4_zero",
//    "monotonic4": "monotonic4_zero",
  };

var GE_defaultValuesOneBlk =
  {// type     user data
    "bool":   "true",
    "int":    "1",
    "float":  "1.0",
    "float2": "1.0, 1.0",
    "float3": "1.0, 1.0, 1.0",
    "float4": "1.0, 1.0, 1.0, 1.0",
    "polynom4": "1.0, 0.0, 0.0, 0.0",
//    "linear4":    "linear4_one",
//    "monotonic4": "monotonic4_one",
  };


var GE_implicitGroupConversionOrder =
  ["bool", "int", "float", "float2", "float3", "float4"];

var curvesHlslCode = {

  "linear":
    "float _curve_fn_%#%(float x)\n" +
    "{\n" +
    "  const float a[] = {%curve%};\n" +
    "  int count = %countof(curve)%;\n" +
    "\n" +
    "  if (count < 2)\n" +
    "    return 0.f;\n" +
    "  if (count < 3)\n" +
    "    return a[1];\n" +
    "\n" +
    "  x = saturate(x);\n" +
    "  int idx = 0;\n" +
    "  for (int i = 0; i < count; i += 3)\n" +
    "    if (a[i] < x)\n" +
    "      idx = i;\n" +
    "\n" +
    "  return a[idx + 1] + a[idx + 2] * (x - a[idx]);\n" +
    "}\n",

  "monotonic":
    "float _curve_fn_%#%(float x)\n" +
    "{\n" +
    "  const float a[] = {%curve%};\n" +
    "  int count = %countof(curve)%;\n" +
    "\n" +
    "  if (count < 2)\n" +
    "    return 0.f;\n" +
    "  if (count < 5)\n" +
    "    return a[1];\n" +
    "\n" +
    "  x = saturate(x);\n" +
    "  int i = 0;\n" +
    "  for (int k = 0; k < count; k += 5)\n" +
    "    if (a[k] < x)\n" +
    "      i = k;\n" +
    "  \n" +
    "  float diff = x - a[i];\n" +
    "  float diffSq = diff * diff;\n" +
    "  return a[1 + i] + a[2 + i] * diff + a[3 + i] * diffSq + a[4 + i] * diff * diffSq;\n" +
    "}",


  "steps":
    "float _curve_fn_%#%(float x)\n" +
    "{\n" +
    "  const float a[] = {%curve%};\n" +
    "  int count = %countof(curve)%;\n" +
    "\n" +
    "  if (count < 2)\n" +
    "    return 0.f;\n" +
    "\n" +
    "  x = saturate(x);\n" +
    "  int idx = 0;\n" +
    "  for (int i = 0; i < count; i += 2)\n" +
    "    if (a[i] < x)\n" +
    "      idx = i;\n" +
    "\n" +
    "  return a[idx + 1];\n" +
    "}\n",


  "polynom":
    "float _curve_fn_%#%(float x)\n" +
    "{\n" +
    "  const float a[] = {%curve%};\n" +
    "  int count = %countof(curve)%;\n" +
    "\n" +
    "  if (count == 0)\n" +
    "    return 0.f;\n" +
    "\n" +
    "  x = saturate(x);\n" +
    "  float xp = 1.0f;\n" +
    "  float sum = 0.0f;\n" +
    "  for (int i = 0; i < count; i++)\n" +
    "  {\n" +
    "    sum += xp * a[i];\n" +
    "    xp *= x;\n" +
    "  }\n" +
    "\n" +
    "  return sum;\n" +
    "}\n",

};



var GE_nodeDescriptions =
  [
/*    {
      name:"combobox",
      category:"Input",
      pins:[
        {name:"value", caption:"%value%", types:["int"], singleConnect:false, role:"out", data:{code:"%value%"}}
      ],
      properties:[
        {name:"value", type:"combobox", items:["value1", "value2", "value3", "value4"], val:"value1"}
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"string",
      category:"Input",
      pins:[
        {name:"value", caption:"\"%value%\"", types:["int"], singleConnect:false, role:"out", data:{code:"%value%"}}
      ],
      properties:[
        {name:"value", type:"string", val:"default"}
      ],
      allowLoop:false,
      width:120
    },

*/

    {
      name:"const bool",
      category:"Input",
      pins:[
        {name:"value", caption:"%value%\n(%external name%)", types:["bool"], singleConnect:false, role:"out", data:{code:"%value%", blk_code:"%value%"}}
      ],
      properties:[
        {name:"value", type:"bool", val:"true"},
        {name:"external name", type:"string", val:""},
      ],
      allowLoop:false,
      width:120,
      addHeight:20
    },

    {
      name:"const int",
      category:"Input",
      pins:[
        {name:"value", caption:"%value%\n(%external name%)", types:["int"], singleConnect:false, role:"out", data:{code:"%value%", blk_code:"%value%"}}
      ],
      properties:[
        {name:"value", type:"int", minVal:"-65535", maxVal:"65535", step:"1", val:"1"},
        {name:"external name", type:"string", val:""},
      ],
      allowLoop:false,
      width:120,
      addHeight:20
    },

    {
      name:"const float",
      category:"Input",
      synonyms:"scalar,1",
      pins:[
        {name:"value", caption:"%value%\n(%external name%)", types:["float"], singleConnect:false, role:"out", data:{code:"%value%", blk_code:"%value%"}}
      ],
      properties:[
        {name:"value", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"1.0"},
        {name:"external name", type:"string", val:""},
      ],
      allowLoop:false,
      width:120,
      addHeight:20
    },

    {
      name:"const float2",
      category:"Input",
      synonyms:"vector,2",
      pins:[
        {name:"value", caption:"%x%\n%y%\n(%external name%)", types:["float2"], singleConnect:false, role:"out", data:{code:"float2(%x%, %y%)", blk_code:"%x%, %y%"}}
      ],
      properties:[
        {name:"x", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"1.0"},
        {name:"y", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"1.0"},
        {name:"external name", type:"string", val:""},
      ],
      allowLoop:false,
      width:120,
      addHeight:20*2
    },

    {
      name:"const float3",
      category:"Input",
      synonyms:"vector,3",
      pins:[
        {name:"value", caption:"%x%\n%y%\n%z%\n(%external name%)", types:["float3"], singleConnect:false, role:"out", data:{code:"float3(%x%, %y%, %z%)", blk_code:"%x%, %y%, %z%"}}
      ],
      properties:[
        {name:"x", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"1.0"},
        {name:"y", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"1.0"},
        {name:"z", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"1.0"},
        {name:"external name", type:"string", val:""},
      ],
      allowLoop:false,
      width:120,
      addHeight:20*3
    },

    {
      name:"const float4",
      category:"Input",
      synonyms:"vector,4",
      pins:[
        {name:"value", caption:"%x%\n%y%\n%z%\n%w%\n(%external name%)", types:["float4"], singleConnect:false, role:"out", data:{code:"float4(%x%, %y%, %z%, %w%)", blk_code:"%x%, %y%, %z%, %w%"}}
      ],
      properties:[
        {name:"x", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"1.0"},
        {name:"y", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"1.0"},
        {name:"z", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"1.0"},
        {name:"w", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"1.0"},
        {name:"external name", type:"string", val:""},
      ],
      allowLoop:false,
      width:120,
      addHeight:20*4
    },

    {
      name:"const rgba",
      category:"Input",
      synonyms:"color,4,float4",
      pins:[
        {name:"rgba", types:["float4"], singleConnect:false, role:"out", data:{code:"float4(%color%)", blk_code:"%color%"}},
        {name:"r", types:["float"], singleConnect:false, role:"out", data:{code:"float4(%color%).x"}},
        {name:"g", types:["float"], singleConnect:false, role:"out", data:{code:"float4(%color%).y"}},
        {name:"b", types:["float"], singleConnect:false, role:"out", data:{code:"float4(%color%).z"}},
        {name:"a", caption: "a\n(%external name%)", types:["float"], singleConnect:false, role:"out", data:{code:"float4(%color%).w"}}
      ],
      properties:[
        {name:"color", type:"color", minVal:"0", maxVal:"1", step:"0.001", val:"0.5,0.5,0.5,1"},
        {name:"external name", type:"string", val:""},
      ],
      allowLoop:false,
      width:140,
      addHeight:20
    },

    {
      name:"const texture",
      category:"Input",
      synonyms:"",
      pins:[
        {name:"texture", caption: "(%external name%)", types:["texture2D"], singleConnect:false, role:"out", data:{}}
      ],
      properties:[
        {name:"external name", type:"string", val:""},
      ],
      allowLoop:false,
      width:120,
    },


    {
      name:"external texture",
      category:"Input",
      synonyms:"variable",
      pins:[
        {name:"texture", caption: "(%name%)", types:["texture2D"], singleConnect:false, role:"out", data:{}}
      ],
      properties:[
        {name:"name", type:"combobox", items:["--to be replaced--"], val:""}
      ],
      isExternal:true,
      allowLoop:false,
      width:220,
    },

    {
      name:"external float4",
      category:"Input",
      synonyms:"variable,vector",
      pins:[
        {name:"value", caption:"(%name%)", types:["float4"], singleConnect:false, role:"out", data:{code:"float4(%name%)"}}
      ],
      properties:[
        {name:"name", type:"combobox", items:["--to be replaced--"], val:""}
      ],
      isExternal:true,
      allowLoop:false,
      width:220,
    },

    {
      name:"external float3",
      category:"Input",
      synonyms:"variable,vector",
      pins:[
        {name:"value", caption:"(%name%)", types:["float3"], singleConnect:false, role:"out", data:{code:"float3(%name%)"}}
      ],
      properties:[
        {name:"name", type:"combobox", items:["--to be replaced--"], val:""}
      ],
      isExternal:true,
      allowLoop:false,
      width:220,
    },

    {
      name:"external float2",
      category:"Input",
      synonyms:"variable,vector",
      pins:[
        {name:"value", caption:"(%name%)", types:["float2"], singleConnect:false, role:"out", data:{code:"float2(%name%)"}}
      ],
      properties:[
        {name:"name", type:"combobox", items:["--to be replaced--"], val:""}
      ],
      isExternal:true,
      allowLoop:false,
      width:220,
    },

    {
      name:"external float",
      category:"Input",
      synonyms:"variable",
      pins:[
        {name:"value", caption:"(%name%)", types:["float"], singleConnect:false, role:"out", data:{code:"float(%name%)"}}
      ],
      properties:[
        {name:"name", type:"combobox", items:["--to be replaced--"], val:""}
      ],
      isExternal:true,
      allowLoop:false,
      width:220,
    },

    {
      name:"external int",
      category:"Input",
      synonyms:"variable",
      pins:[
        {name:"value", caption:"(%name%)", types:["int"], singleConnect:false, role:"out", data:{code:"int(%name%)"}}
      ],
      properties:[
        {name:"name", type:"combobox", items:["--to be replaced--"], val:""}
      ],
      isExternal:true,
      allowLoop:false,
      width:220,
    },


    {
      name:"world pos",
      category:"Input",
      synonyms:"coordinates",
      pins:[
        {name:"pos", types:["float3"], singleConnect:false, role:"out", data:{code:"world_pos"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"prefered biome indices",
      category:"Input",
      synonyms:"indices",
      pins:[
        {name:"indices", types:["int4"], singleConnect:false, role:"out", data:{code:"float4(gpu_objects_biom1, gpu_objects_biom2, gpu_objects_biom3, gpu_objects_biom4)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"scale range",
      category:"Input",
      synonyms:"range",
      pins:[
        {name:"range", types:["float2"], singleConnect:false, role:"out", data:{code:"scale_range"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"rotate range",
      category:"Input",
      synonyms:"range",
      pins:[
        {name:"range", types:["float2"], singleConnect:false, role:"out", data:{code:"rotate_range"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"seed",
      category:"Input",
      synonyms:"seed",
      pins:[
        {name:"seed", types:["float"], singleConnect:false, role:"out", data:{code:"seed"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"up vector",
      category:"Input",
      synonyms:"direction",
      pins:[
        {name:"up", types:["float3"], singleConnect:false, role:"out", data:{code:"up_vector.xyz"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"incline delta",
      category:"Input",
      synonyms:"delta",
      pins:[
        {name:"delta", types:["float"], singleConnect:false, role:"out", data:{code:"incline_delta"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"map size",
      category:"Input",
      synonyms:"size",
      pins:[
        {name:"size", types:["float2"], singleConnect:false, role:"out", data:{code:"map_size"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"map offset",
      category:"Input",
      synonyms:"offset",
      pins:[
        {name:"offset", types:["float2"], singleConnect:false, role:"out", data:{code:"map_offset"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"from color",
      category:"Input",
      synonyms:"color",
      pins:[
        {name:"color", types:["float4"], singleConnect:false, role:"out", data:{code:"from_color"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"to color",
      category:"Input",
      synonyms:"color",
      pins:[
        {name:"color", types:["float4"], singleConnect:false, role:"out", data:{code:"to_color"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name: "time",
      category: "Input",
      synonyms: "time",
      pins: [
        { name: "time", types: ["float"], singleConnect: false, role: "out", data: { code: "global_time_phase" } }
      ],
      properties: [
      ],
      allowLoop: false,
      width: 120
    },

    {
      name: "time of day",
      category: "Input",
      synonyms: "time of day, clock time, hour",
      pins: [
        { name: "time of day", types: ["float"], singleConnect: false, role: "out", data: { code: "daskies_time" } }
      ],
      properties: [
      ],
      allowLoop: false,
      width: 120
    },

    {
      name: "screen tc",
      category: "Input",
      synonyms: "coordinates",
      pins: [
        { name: "pos", types: ["float3"], singleConnect: false, role: "out", data: { code: "screenTcJittered" } }
      ],
      properties: [
      ],
      allowLoop: false,
      width: 120
    },

    {
      name: "depth above tc",
      category: "Input",
      synonyms: "coordinates",
      pins: [
        { name: "pos", types: ["float2"], singleConnect: false, role: "out", data: { code: "depthAboveTc" } }
      ],
      properties: [
      ],
      allowLoop: false,
      width: 120
    },

    {
      name: "depth above vignette",
      category: "Input",
      synonyms: "",
      pins: [
        { name: "pos", types: ["float"], singleConnect: false, role: "out", data: { code: "depthAboveVignette" } }
      ],
      properties: [
      ],
      allowLoop: false,
      width: 140
    },

    {
      name: "ground height",
      category: "Input",
      synonyms: "",
      pins: [
        { name: "height", types: ["float"], singleConnect: false, role: "out", data: { code: "get_ground_height(world_pos)" } }
      ],
      properties: [
      ],
      allowLoop: false,
      width: 120
    },

    {
      name: "ground height at",
      category: "Input",
      synonyms: "",
      pins: [
        { name: "world pos", types: ["float3"], singleConnect: true, role: "in" },
        { name: "height", types: ["float"], singleConnect: false, role: "out", data: { code: "get_ground_height($world pos$)" } }
      ],
      properties: [
      ],
      allowLoop: false,
      width: 120
    },

    {
      name: "ground height lod",
      category: "Input",
      synonyms: "",
      pins: [
        { name: "lod", types: ["float"], singleConnect: true, role: "in" },
        { name: "height", types: ["float"], singleConnect: false, role: "out", data: { code: "get_ground_height_lod(world_pos, $lod$)" } }
      ],
      properties: [
      ],
      allowLoop: false,
      width: 120
    },

    {
      name: "ground height lod at",
      category: "Input",
      synonyms: "",
      pins: [
        { name: "world pos", types: ["float3"], singleConnect: true, role: "in" },
        { name: "lod", types: ["float"], singleConnect: true, role: "in" },
        { name: "height", types: ["float"], singleConnect: false, role: "out", data: { code: "get_ground_height_lod($world pos$, $lod$)" } }
      ],
      properties: [
      ],
      allowLoop: false,
      width: 120
    },

    {
      name: "ground normal",
      category: "Input",
      synonyms: "",
      pins: [
        { name: "height", types: ["float3"], singleConnect: false, role: "out", data: { code: "getWorldNormal(world_pos)" } }
      ],
      properties: [
      ],
      allowLoop: false,
      width: 120
    },

    {
      name: "ground normal at",
      category: "Input",
      synonyms: "",
      pins: [
        { name: "world pos", types: ["float3"], singleConnect: true, role: "in" },
        { name: "height", types: ["float3"], singleConnect: false, role: "out", data: { code: "getWorldNormal($world pos$)" } }
      ],
      properties: [
      ],
      allowLoop: false,
      width: 120
    },

    // TODO: deprecate it (remove from levels first)
    {
      name: "biome indices",
      category: "Input",
      synonyms: "indices",
      pins: [
        { name: "world pos", types: ["float3"], singleConnect: true, role: "in" },
        { name: "index", types: ["int4"], singleConnect: false, role: "out", data: { code: "getBiomeIndices($world pos$)" } }
      ],
      properties: [
      ],
      allowLoop: false,
      width: 120
    },

    // TODO: deprecate it (remove from levels first)
    {
      name: "biome influence",
      category: "Input",
      synonyms: "biome",
      pins: [
        { name: "world pos", types: ["float3"], singleConnect: true, role: "in" },
        { name: "index", types: ["int"], singleConnect: true, role: "in" },
        { name: "biome influence", types: ["float"], singleConnect: false, role: "out", data: { code: "getBiomeInfluence($world pos$, $index$)" } }
      ],
      properties: [
      ],
      allowLoop: false,
      width: 200
    },

    {
      name: "biome data",
      category: "Input",
      synonyms: "biome",
      pins: [
        { name: "world pos", types: ["float3"], singleConnect: true, role: "in" },
        { name: "biome data", types: ["BiomeData"], singleConnect: false, role: "out", data: { code: "getBiomeData($world pos$)" } }
      ],
      properties: [
      ],
      allowLoop: false,
      width: 200
    },
    {
      name: "biome influence from data",
      category: "Input",
      synonyms: "biome",
      pins: [
        { name: "biome data", types: ["BiomeData"], singleConnect: true, role: "in" },
        { name: "index", types: ["int"], singleConnect: true, role: "in" },
        { name: "biome influence", types: ["float"], singleConnect: false, role: "out", data: { code: "calcBiomeInfluence($biome data$, $index$)" } }
      ],
      properties: [
        {name:"index", type:"int", minVal:"0", maxVal:"255", step:"1", val:"0"},
      ],
      allowLoop: false,
      width: 200
    },
    {
      name: "extract biome data",
      category: "Input",
      synonyms: "biome",
      pins: [
        { name: "biome data", types: ["BiomeData"], singleConnect: true, role: "in" },
        { name: "indices", types: ["int4"], singleConnect: false, role: "out", data: { code: "$biome data$.indices" } },
        { name: "weights", types: ["float4"], singleConnect: false, role: "out", data: { code: "$biome data$.weights" } }
      ],
      properties: [
      ],
      allowLoop: false,
      width: 200
    },

    {
      name: "is biome active",
      category: "Input",
      synonyms: "biome",
      pins: [
        { name: "is active", types: ["bool"], singleConnect: false, role: "out", data: { code: "placeAllowed" } }
      ],
      properties: [
      ],
      allowLoop: false,
      width: 200
    },

    {
      name: "volfog mask",
      category: "Input",
      synonyms: "",
      pins: [
        { name: "mask", types: ["float"], singleConnect: false, role: "out", data: { code: "sample_volfog_mask(world_pos.xz)" } }
      ],
      properties: [
      ],
      allowLoop: false,
      width: 120
    },

    {
      name: "volfog mask at",
      category: "Input",
      synonyms: "",
      pins: [
        { name: "world pos", types: ["float3"], singleConnect: true, role: "in" },
        { name: "mask", types: ["float"], singleConnect: false, role: "out", data: { code: "sample_volfog_mask(($world pos$).xz)" } }
      ],
      properties: [
      ],
      allowLoop: false,
      width: 120
    },


    {
      name:"rand",
      category:"Input",
      synonyms:"noise,random",
      pins:[
        {name:"seed", types:["float"], singleConnect:true, role:"in"},
        {name:"rand", types:["float"], singleConnect:false, role:"out", data:{code:"rand($seed$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"int_noise1d",
      category:"Input",
      synonyms:"noise,random",
      pins:[
        {name:"seed", types:["int"], singleConnect:true, role:"in"},
        {name:"position", types:["int"], singleConnect:true, role:"in"},
        {name:"outer seed", types:["int"], singleConnect:false, role:"out", data:{code:"uint_noise1D($position$, $seed$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"int_noise2d",
      category:"Input",
      synonyms:"noise,random",
      pins:[
        {name:"seed", types:["int"], singleConnect:true, role:"in"},
        {name:"positionX", types:["int"], singleConnect:true, role:"in"},
        {name:"positionY", types:["int"], singleConnect:true, role:"in"},
        {name:"outer seed", types:["int"], singleConnect:false, role:"out", data:{code:"uint_noise2D($positionX$, $positionY$, $seed$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"int_noise3d",
      category:"Input",
      synonyms:"noise,random",
      pins:[
        {name:"seed", types:["int"], singleConnect:true, role:"in"},
        {name:"positionX", types:["int"], singleConnect:true, role:"in"},
        {name:"positionY", types:["int"], singleConnect:true, role:"in"},
        {name:"positionZ", types:["int"], singleConnect:true, role:"in"},
        {name:"outer seed", types:["int"], singleConnect:false, role:"out", data:{code:"uint_noise3D($positionX$, $positionY$, $positionZ$, $seed$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"random",
      category:"Input",
      synonyms:"noise",
      pins:[
        {name:"rand", types:["float"], singleConnect:false, role:"out", data:{code:"rand($outer seed$ + %seed% + input.tc.x + input.tc.y * 231.321)"}},
        {name:"outer seed", types:["float"], singleConnect:true, role:"in"}
      ],
      properties:[
        {name:"seed", type:"float", minVal:"-1e10", maxVal:"1e10", step:"1.2321", val:"0.0"}
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"noise2d",
      category:"Input",
      synonyms:"noise,random",
      pins:[
        {name:"pos", types:["float2"], singleConnect:true, role:"in"},
        {name:"rand", types:["float2"], singleConnect:false, role:"out", data:{code:"noise_Value2D($pos$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"PI",
      category:"Input",
      synonyms:"3.1415",
      pins:[
        {name:"3.14159265", types:["float"], singleConnect:false, role:"out", data:{code:"3.14159265"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },


    {
      name:"texValue",
      category:"Input",
      synonyms:"texture,fetch,2d",
      pins:[
        {name:"texture", types:["texture2D"], singleConnect:true, role:"in"},
        {name:"rgba", types:["float4"], singleConnect:false, role:"out", data:{code:"$texture$.SampleLevel($texture$_samplerstate, input.tc, 0)"}}
        ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },


    {
      name:"tex2D",
      category:"Input",
      synonyms:"texture,fetch,2d",
      pins:[
        {name:"texture", types:["texture2D"], singleConnect:true, role:"in"},
        {name:"coord", types:["float2"], singleConnect:true, role:"in"},
        {name:"rgba", types:["float4"], singleConnect:false, role:"out", data:{code:"$texture$.SampleLevel($texture$_samplerstate, $coord$, 0)"}}
        ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"vec to float",
      category:"Conversion",
      synonyms:"swizzle,component",
      pins:[
        {name:"vec", types:["float4"], singleConnect:true, role:"in"},
        {name:"x", types:["float"], singleConnect:false, role:"out", data:{code:"$vec$.x"}},
        {name:"y", types:["float"], singleConnect:false, role:"out", data:{code:"$vec$.y"}},
        {name:"z", types:["float"], singleConnect:false, role:"out", data:{code:"$vec$.z"}},
        {name:"w", types:["float"], singleConnect:false, role:"out", data:{code:"$vec$.w"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:80
    },

    {
      name:"float to vec",
      category:"Conversion",
      synonyms:"swizzle,component",
      pins:[
        {name:"x", types:["float"], singleConnect:true, role:"in"},
        {name:"y", types:["float"], singleConnect:true, role:"in"},
        {name:"z", types:["float"], singleConnect:true, role:"in"},
        {name:"w", types:["float"], singleConnect:true, role:"in"},
        {name:"vec", types:["float4"], singleConnect:false, role:"out", data:{code:"float4($x$, $y$, $z$, $w$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:80
    },

    {
      name:"vec to int",
      category:"Conversion",
      synonyms:"swizzle,component",
      pins:[
        {name:"vec", types:["int4"], singleConnect:true, role:"in"},
        {name:"x", types:["int"], singleConnect:false, role:"out", data:{code:"$vec$.x"}},
        {name:"y", types:["int"], singleConnect:false, role:"out", data:{code:"$vec$.y"}},
        {name:"z", types:["int"], singleConnect:false, role:"out", data:{code:"$vec$.z"}},
        {name:"w", types:["int"], singleConnect:false, role:"out", data:{code:"$vec$.w"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:80
    },

    {
      name:"int to vec",
      category:"Conversion",
      synonyms:"swizzle,component",
      pins:[
        {name:"x", types:["int"], singleConnect:true, role:"in"},
        {name:"y", types:["int"], singleConnect:true, role:"in"},
        {name:"z", types:["int"], singleConnect:true, role:"in"},
        {name:"w", types:["int"], singleConnect:true, role:"in"},
        {name:"vec", types:["int4"], singleConnect:false, role:"out", data:{code:"int4($x$, $y$, $z$, $w$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:80
    },

    {
      name:"is finite",
      category:"Conversion",
      synonyms:"nan,isnan",
      pins:[
        {name:"x", types:["float"], singleConnect:true, typeGroup:"group1", role:"in", data:{def_1:false}},
        {name:"isfinite(x)", types:["float"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"isfinite($x$) ? 1.0 : 0.0"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:80
    },

    {
      name:"+",
      category:"Math",
      synonyms:"add,summ,increase,offset,plus",
      pins:[
        {name:"A", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"B", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"A+B", types:["int", "float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"($A$ + $B$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"-",
      category:"Math",
      synonyms:"subtract,decrease,offset,minus",
      pins:[
        {name:"A", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"B", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"A-B", types:["int", "float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"($A$ - $B$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"*",
      category:"Math",
      synonyms:"multiply,product,scale",
      pins:[
        {name:"A", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in", data:{def_1:true}},
        {name:"B", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in", data:{def_1:true}},
        {name:"A*B", types:["int", "float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"($A$ * $B$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"/",
      category:"Math",
      synonyms:"divide,scale",
      pins:[
        {name:"A", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in", data:{def_1:true}},
        {name:"B", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in", data:{def_1:true}},
        {name:"A/B", types:["int", "float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"($A$ / $B$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"scale",
      category:"Math",
      synonyms:"multiply",
      pins:[
        {name:"A", types:["float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"res", caption:"A * %scale%", types:["float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"$A$ * (%scale%)"}}
      ],
      properties:[
        {name:"scale", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"1.0"}
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"expression",
      category:"Math",
      synonyms:"calculate",
      pins:[
        {name:"n", types:["float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"res", caption:"%res=%", types:["float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"_EXPRESSION_%#%($n$)", localFunction:"#define _EXPRESSION_%#%(n) (%res=%)" }}
      ],
      properties:[
        {name:"res=", type:"string", val:"n"}
      ],
      allowLoop:false,
      width:300
    },

    {
      name:"dot",
      category:"Math",
      synonyms:"product",
      pins:[
        {name:"A", types:["float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in", data:{def_1:true}},
        {name:"B", types:["float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in", data:{def_1:true}},
        {name:"dot(A,B)", types:["float"], singleConnect:false, role:"out", data:{code:"dot($A$, $B$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"cross",
      category:"Math",
      synonyms:"product",
      pins:[
        {name:"A", types:["float3"], singleConnect:true, role:"in", data:{def_1:true}},
        {name:"B", types:["float3"], singleConnect:true, role:"in", data:{def_1:true}},
        {name:"cross(A,B)", types:["float3"], singleConnect:false, role:"out", data:{code:"cross($A$, $B$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"mod",
      category:"Math",
      synonyms:"divide,%",
      pins:[
        {name:"A", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"B", types:["float"], singleConnect:true, role:"in", data:{def_1:true}},
        {name:"A mod B", types:["int", "float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"fmod($A$, $B$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"imod",
      category:"Math",
      synonyms:"divide,%",
      pins:[
        {name:"A", types:["int"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"B", types:["int"], singleConnect:true, role:"in", data:{def_1:true}},
        {name:"A mod B", types:["int"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"($A$ % $B$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"idiv",
      category:"Math",
      synonyms:"divide,%",
      pins:[
        {name:"A", types:["int"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"B", types:["int"], singleConnect:true, role:"in", data:{def_1:true}},
        {name:"A / B", types:["int"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"($A$ / ($B$ == 0 ? 1 : $B$))"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"frac",
      category:"Math",
      synonyms:"int,round",
      pins:[
        {name:"A",       types:["float", "float2", "float3", "float4"], singleConnect:true,  typeGroup:"group1", role:"in"},
        {name:"frac(A)", types:["float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"frac($A$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"floor",
      category:"Math",
      synonyms:"int,round",
      pins:[
        {name:"A",        types:["float", "float2", "float3", "float4"], singleConnect:true,  typeGroup:"group1", role:"in"},
        {name:"floor(A)", types:["float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"floor($A$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"ceil",
      category:"Math",
      synonyms:"int,round",
      pins:[
        {name:"A",       types:["float", "float2", "float3", "float4"], singleConnect:true,  typeGroup:"group1", role:"in"},
        {name:"ceil(A)", types:["float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"ceil($A$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"sqrt",
      category:"Math",
      synonyms:"square,root",
      pins:[
        {name:"A",       types:["float", "float2", "float3", "float4"], singleConnect:true,  typeGroup:"group1", role:"in"},
        {name:"sqrt(A)", types:["float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"sqrt($A$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"1/sqrt",
      category:"Math",
      synonyms:"inverse,square,root",
      pins:[
        {name:"A",         types:["float", "float2", "float3", "float4"], singleConnect:true,  typeGroup:"group1", role:"in", data:{def_1:true}},
        {name:"1/sqrt(A)", types:["float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"rsqrt($A$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"abs",
      category:"Math",
      pins:[
        {name:"A",      types:["int", "float", "float2", "float3", "float4"], singleConnect:true,  typeGroup:"group1", role:"in"},
        {name:"abs(A)", types:["int", "float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"abs($A$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"all",
      category:"Math",
      pins:[
        {name:"value", types:["int", "int2", "int3", "int4", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"all", types:["bool"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"all($value$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"any",
      category:"Math",
      pins:[
        {name:"value", types:["int", "int2", "int3", "int4", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"any", types:["bool"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"any($value$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"sign",
      category:"Math",
      pins:[
        {name:"x", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"sign", types:["int", "float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"sign($x$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"max",
      category:"Math",
      pins:[
        {name:"A", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"B", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"max", types:["int", "float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"max($A$, $B$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"min",
      category:"Math",
      pins:[
        {name:"A", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"B", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"max", types:["int", "float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"min($A$, $B$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"clamp",
      category:"Math",
      synonyms:"range,saturate",
      pins:[
        {name:"x", types:["float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"minV", types:["float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"maxV", types:["float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"res", types:["float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"clamp($x$, $minV$, $maxV$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"saturate",
      category:"Math",
      synonyms:"range,clamp",
      pins:[
        {name:"A", types:["float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"res", types:["float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"saturate($A$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"lerp",
      category:"Math",
      synonyms:"linear,interpolation,mix,blend",
      pins:[
        {name:"A", types:["float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"B", types:["float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"t", types:["float", "float2", "float3", "float4"], singleConnect:true, role:"in"},
        {name:"res", types:["float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"lerp($A$, $B$, $t$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"A*B+C",
      category:"Math",
      synonyms:"madd,multiply,add",
      pins:[
        {name:"A", types:["float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in", data:{def_1:true}},
        {name:"B", types:["float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in", data:{def_1:true}},
        {name:"C", types:["float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"res", types:["float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"($A$ * $B$ + $C$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"length",
      category:"Math",
      synonyms:"distance",
      pins:[
        {name:"A", types:["float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"res", types:["float"], singleConnect:false, role:"out", data:{code:"length($A$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"normalize",
      category:"Math",
      pins:[
        {name:"v",    types:["float2", "float3", "float4"], typeGroup:"group1", singleConnect:true,  role:"in"},
        {name:"norm", types:["float2", "float3", "float4"], typeGroup:"group1", singleConnect:false, role:"out", data:{code:"safe_normalize($v$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"smoothstep",
      category:"Math",
      synonyms:"easing",
      pins:[
        {name:"x", types:["float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"sstep(x)", types:["float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"smoothstep(0.0, 1.0, $x$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"map range to 0..1",
      category:"Math",
      synonyms:"clamp,saturate,remap",
      pins:[
        {name:"x", types:["float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"min_v", types:["float"], singleConnect:true, role:"in", data:{def_1:false}},
        {name:"max_v", types:["float"], singleConnect:true, role:"in", data:{def_1:true}},
        {name:"res", types:["float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"saturate(($x$ - $min_v$) / ($max_v$ - $min_v$))"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"map 0..1 to range",
      category:"Math",
      synonyms:"clamp,saturate,remap",
      pins:[
        {name:"x", types:["float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"min_v", types:["float"], singleConnect:true, role:"in", data:{def_1:false}},
        {name:"max_v", types:["float"], singleConnect:true, role:"in", data:{def_1:true}},
        {name:"res", types:["float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"saturate($x$) * ($max_v$ - $min_v$) + $min_v$"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"bright contrast",
      category:"Math",
      synonyms:"map",
      pins:[
        {name:"x", types:["float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"bright", types:["float"], singleConnect:true, role:"in", data:{def_val:"0.0", def_val_blk:"0.0"}},
        {name:"contrast", types:["float"], singleConnect:true, role:"in", data:{def_val:"1.0", def_val_blk:"1.0"}},
        {name:"res", types:["float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"saturate(($x$ - 0.5) * $contrast$ + $bright$ + 0.5)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"bright contrast const",
      category:"Math",
      synonyms:"map",
      pins:[
        {name:"x", types:["float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"res", types:["float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"saturate(($x$ - 0.5) * %contrast% + %bright% + 0.5)"}}
      ],
      properties:[
        {name:"bright", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"0.0"},
        {name:"contrast", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"1.0"}
      ],
      allowLoop:false,
      width:150
    },

    {
      name:"1-x",
      category:"Math",
      synonyms:"negate,inverse",
      pins:[
        {name:"x", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"1-x", types:["int", "float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"1.0 - $x$"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:80
    },

    {
      name: "fog quality filter",
      category: "Fog Filtering",
      synonyms:"fog,filter,compare,branch,check,test",
      pins: [
        {name:"close", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in", data:{def_val:"0.0", def_val_blk:"0.0"}},
        {name:"far", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in", data:{def_val:"0.0", def_val_blk:"0.0"}},
        {name: "result", types:["int", "float", "float2", "float3", "float4"], singleConnect: false, typeGroup:"group1", role: "out",
          data: { code: "((volfog_froxel_range_params.w) ? ($far$) : ($close$))"} }
      ],
      properties: [
      ],
      allowLoop: false,
      width: 120
    },

    {
      name: "fog type filter",
      category: "Fog Filtering",
      synonyms:"fog,filter,compare,branch,check,test",
      pins: [
        {name:"froxel fog", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in", data:{def_val:"0.0", def_val_blk:"0.0"}},
        {name:"distant fog", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in", data:{def_val:"0.0", def_val_blk:"0.0"}},
        {name: "result", types:["int", "float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out",
          data: { code: "((IS_DISTANT_FOG) ? ($distant fog$) : ($froxel fog$))"} }
      ],
      properties: [
      ],
      allowLoop: false,
      width: 120
    },

    {
      name:"close fog filter",
      category:"Fog Filtering",
      synonyms:"fog,filter,compare,branch,check,test",
      pins:[
        {name:"input", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in", data:{def_1:true}},
        {name: "result", types:["int", "float", "float2", "float3", "float4"], singleConnect: false, typeGroup:"group1", role: "out",
          data:{code:"((IS_DISTANT_FOG) ? 0.0 : ($input$))"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name: "distant fog mul",
      category: "Fog Filtering",
      synonyms:"fog,filter,multiply,product,scale",
      pins: [
        {name:"input", types:["float4"], singleConnect:true, typeGroup:"group1", role:"in", data:{def_1:true}},
        { name: "mul", types: ["float4"], singleConnect: true, role: "in", data: {def_val:"%mul%"} },
        {name: "result", types:["float4"], singleConnect:false, typeGroup:"group1", role:"out",
          data: { code: "((IS_DISTANT_FOG) ? ($mul$) : 1.0) * ($input$)"} }
      ],
      properties: [
        { name: "mul", type: "color", minVal: "0", maxVal: "1", step: "0.001", val: "1,1,1,1" },
      ],
      allowLoop: false,
      width: 120
    },

    {
      name:"if",
      category:"Branching",
      synonyms:"compare,branch,max,min,check,test",
      pins:[
        {name:"condition", types:["bool"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"true", types:["int", "int2", "int3", "int4", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group2", role:"in"},
        {name:"false", types:["int", "int2", "int3", "int4", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group2", role:"in"},
        {name:"res", types:["int", "int2", "int3", "int4", "float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group2", role:"out", data:{code:"(($condition$) ? ($true$) : ($false$))"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"if A==B",
      category:"Branching",
      synonyms:"compare,branch,max,min,check,test",
      pins:[
        {name:"A", types:["int", "float"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"B", types:["int", "float"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"A==B", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group2", role:"in"},
        {name:"A!=B", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group2", role:"in"},
        {name:"result", types:["int", "int2", "int3", "int4", "float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group2", role:"out", data:{code:"(($A$ == $B$) ? ($A==B$) : ($A!=B$))"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"if A>B",
      category:"Branching",
      synonyms:"compare,branch,max,min,check,test",
      pins:[
        {name:"A", types:["int", "float"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"B", types:["int", "float"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"A>B", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group2", role:"in"},
        {name:"A<=B", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group2", role:"in"},
        {name:"res", types:["int", "float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group2", role:"out", data:{code:"(($A$ > $B$) ? ($A>B$) : ($A<=B$))"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"if A>=B",
      category:"Branching",
      synonyms:"compare,branch,max,min,check,test",
      pins:[
        {name:"A", types:["int", "float"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"B", types:["int", "float"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"A>=B", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group2", role:"in"},
        {name:"A<B", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group2", role:"in"},
        {name:"res", types:["int", "float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group2", role:"out", data:{code:"(($A$ >= $B$) ? ($A>=B$) : ($A<B$))"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },


    {
      name:"pow",
      category:"Math",
      synonyms:"square,^,exp",
      pins:[
        {name:"a", types:["float", "float2", "float3", "float4"], typeGroup:"group1", singleConnect:true, role:"in"},
        {name:"b", types:["float", "float2", "float3", "float4"], typeGroup:"group1", singleConnect:true, role:"in"},
        {name:"a^b", types:["float", "float2", "float3", "float4"], typeGroup:"group1", singleConnect:false, role:"out", data:{code:"pow($a$, $b$)"}}
      ],
      properties:[
      ],
      typeBypass:true,
      allowLoop:false,
      width:120
    },

    {
      name:"exp",
      category:"Math",
      synonyms:"exp,pow,2.7182",
      pins:[
        {name:"a", types:["float", "float2", "float3", "float4"], typeGroup:"group1", singleConnect:true, role:"in"},
        {name:"exp(a)", types:["float", "float2", "float3", "float4"], typeGroup:"group1", singleConnect:false, role:"out", data:{code:"exp($a$)"}}
      ],
      properties:[
      ],
      typeBypass:true,
      allowLoop:false,
      width:120
    },

    {
      name:"cos",
      category:"Periodic",
      pins:[
        {name:"a", types:["float", "float2", "float3", "float4"], typeGroup:"group1", singleConnect:true, role:"in"},
        {name:"cos(a)", types:["float", "float2", "float3", "float4"], typeGroup:"group1", singleConnect:false, role:"out", data:{code:"cos($a$)"}}
      ],
      properties:[
      ],
      typeBypass:true,
      allowLoop:false,
      width:120
    },

    {
      name:"sin",
      category:"Periodic",
      pins:[
        {name:"a", types:["float", "float2", "float3", "float4"], typeGroup:"group1", singleConnect:true, role:"in"},
        {name:"sin(a)", types:["float", "float2", "float3", "float4"], typeGroup:"group1", singleConnect:false, role:"out", data:{code:"sin($a$)"}}
      ],
      properties:[
      ],
      typeBypass:true,
      allowLoop:false,
      width:120
    },

    {
      name:"tan",
      category:"Periodic",
      pins:[
        {name:"a", types:["float", "float2", "float3", "float4"], typeGroup:"group1", singleConnect:true, role:"in"},
        {name:"tan(a)", types:["float", "float2", "float3", "float4"], typeGroup:"group1", singleConnect:false, role:"out", data:{code:"tan($a$)"}}
      ],
      properties:[
      ],
      typeBypass:true,
      allowLoop:false,
      width:120
    },

    {
      name:"acos",
      category:"Periodic",
      synonyms:"arccos",
      pins:[
        {name:"a", types:["float", "float2", "float3", "float4"], typeGroup:"group1", singleConnect:true, role:"in"},
        {name:"acos(a)", types:["float", "float2", "float3", "float4"], typeGroup:"group1", singleConnect:false, role:"out", data:{code:"acos(saturate($a$))"}}
      ],
      properties:[
      ],
      typeBypass:true,
      allowLoop:false,
      width:120
    },

    {
      name:"asin",
      category:"Periodic",
      synonyms:"arcsin",
      pins:[
        {name:"a", types:["float", "float2", "float3", "float4"], typeGroup:"group1", singleConnect:true, role:"in"},
        {name:"asin(a)", types:["float", "float2", "float3", "float4"], typeGroup:"group1", singleConnect:false, role:"out", data:{code:"asin(saturate($a$))"}}
      ],
      properties:[
      ],
      typeBypass:true,
      allowLoop:false,
      width:120
    },

    {
      name:"atan",
      category:"Periodic",
      synonyms:"arctan",
      pins:[
        {name:"a", types:["float", "float2", "float3", "float4"], typeGroup:"group1", singleConnect:true, role:"in"},
        {name:"atan(a)", types:["float", "float2", "float3", "float4"], typeGroup:"group1", singleConnect:false, role:"out", data:{code:"atan($a$)"}}
      ],
      properties:[
      ],
      typeBypass:true,
      allowLoop:false,
      width:120
    },

    {
      name:"atan2",
      category:"Periodic",
      synonyms:"arctan",
      pins:[
        {name:"y", types:["float"], singleConnect:true, role:"in", data:{def_1:true}},
        {name:"x", types:["float"], singleConnect:true, role:"in", data:{def_1:true}},
        {name:"atan2(y,x)", types:["float"], singleConnect:false, role:"out", data:{code:"atan2($y$, $x$)"}}
      ],
      properties:[
      ],
      typeBypass:true,
      allowLoop:false,
      width:120
    },

    {
      name:"triangle",
      category:"Periodic",
      synonyms:"sin,cos,wave",
      pins:[
        {name:"a", types:["float", "float2", "float3", "float4"], typeGroup:"group1", singleConnect:true, role:"in"},
        {name:"triangle(a)", types:["float", "float2", "float3", "float4"], typeGroup:"group1", singleConnect:false, role:"out", data:{code:"abs(frac($a$) - 0.5) * 2.0"}}
      ],
      properties:[
      ],
      typeBypass:true,
      allowLoop:false,
      width:120
    },

    {
      name:"hills",
      category:"Periodic",
      synonyms:"sin,cos,wave",
      pins:[
        {name:"a", types:["float", "float2", "float3", "float4"], typeGroup:"group1", singleConnect:true, role:"in"},
        {name:"hills(a)", types:["float", "float2", "float3", "float4"], typeGroup:"group1", singleConnect:false, role:"out", data:{code:"pow2(1.0 - pow2((frac($a$) - 0.5) * 2.0))"}}
      ],
      properties:[
      ],
      typeBypass:true,
      allowLoop:false,
      width:120
    },


    {
      name:"result",
      category:"Output",
      synonyms:"output,color",
      pins:[
        {name:"color", caption:"col%MRT index%", types:["float4"], singleConnect:true, role:"in", data:{def_val:"float4(0.5, 0.5, 0.5, 0.5)", def_val_blk:"0.5, 0.5, 0.5, 0.5", code:"result.col%MRT index% += $color$", result_color:true}},
        {name:"out", types:["texture2D"], singleConnect:false, role:"out", hidden:true},
      ],
      properties:[
        {name:"MRT index", type:"int", minVal:"0", maxVal:"8", step:"1", val:"0", hidden:true},
      ],
      allowLoop:false,
      width:120,
      addHeight:20
    },

    {
      name:GE_volfog_early_exit_node_name,
      category:"Output",
      synonyms:"output,early_exit,exit,occlusion,branch",
      pins:[
        {name:"active", caption:"active", types:["bool", "int", "float"], singleConnect:true, role:"in", data:{def_val:"1.0", def_val_blk:"1.0", code:GE_volfog_early_exit_var_name+" = (float)$active$ > 0.00001"}},
        {name:"out", types:["bool"], singleConnect:false, role:"out", hidden:true},
      ],
      properties:[
      ],
      allowLoop:false,
      width:120,
      addHeight:20
    },

    {
      name:"GPU object parameters",
      category:"Metadata",
      synonyms:"metadata,parameters",
      pins: [],
      properties:[
        {name:"grid_tiling", type:"int", minVal:"0", maxVal:"1024", step:"1", val:"4"},
        {name:"grid_size", type:"int", minVal:"0", maxVal:"1024", step:"1", val:"16"},
        {name:"cell_size", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"8.0"},
        {name:"seed", type:"int", minVal:"-1e10", maxVal:"1e10", step:"1", val:"0"},
        {name:"up_vector_x", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"0.0"},
        {name:"up_vector_y", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"1.0"},
        {name:"up_vector_z", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"0.0"},
        {name:"scale_range_x", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"1.0"},
        {name:"scale_range_y", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"1.0"},
        {name:"biom_indexes_x", type:"float", minVal:"-1.0", maxVal:"1e10", step:"1.0", val:"-1.0"},
        {name:"biom_indexes_y", type:"float", minVal:"-1.0", maxVal:"1e10", step:"1.0", val:"-1.0"},
        {name:"biom_indexes_z", type:"float", minVal:"-1.0", maxVal:"1e10", step:"1.0", val:"-1.0"},
        {name:"biom_indexes_w", type:"float", minVal:"-1.0", maxVal:"1e10", step:"1.0", val:"-1.0"},
        {name:"is_using_normal", type:"bool", val:"false"},
        {name:"incline_delta", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"0.0"},
        {name:"rotate_range_x", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"0.0"},
        {name:"rotate_range_y", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"0.0"},
        {name:"map", type:"string", val:""},
        {name:"map_size_x", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"512.0"},
        {name:"map_size_y", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"512.0"},
        {name:"map_offset_x", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"0.0"},
        {name:"map_offset_y", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"0.0"},
        {name:"color_from", type:"color", minVal:"0.0", maxVal:"1.0", step:"0,00392", val:"0.498,0.498,0.498,0.498"},
        {name:"color_to", type:"color", minVal:"0.0", maxVal:"1.0", step:"0,00392", val:"0.498,0.498,0.498,0.498"},
        {name:"max_placed_on_ri", type:"int", minVal:"0", maxVal:"100", step:"1", val:"0"},
        {name:"slope_factor_x", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"1.0"},
        {name:"slope_factor_y", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"0.0"},
        {name:"hardness", type:"float", minVal:"-1e10", maxVal:"1e10", step:"0.001", val:"1.0"},
      ],
      allowLoop:false,
      width:160,
      addHeight:20
    },

    {
      name:"instance",
      category:"Output",
      synonyms:"output,instance",
      pins:[
        {name:"active", caption:"active", types:["bool"], singleConnect:true, role:"in", data:{def_val:"false", def_val_blk:"false", code:"instanceData.active = $active$"}},
        {name:"position", caption:"position", types:["float3"], singleConnect:true, role:"in", data:{code:"instanceData.position = $position$"}},
        {name:"direction", caption:"direction", types:["float3"], singleConnect:true, role:"in", data:{def_val:"float3(0,1,0)", def_val_blk:"0,1,0", code:"instanceData.direction = $direction$"}},
        {name:"rotation", caption:"rotation", types:["float"], singleConnect:true, role:"in", data:{def_val:"0", def_val_blk:"0", code:"instanceData.rotation = $rotation$"}},
        {name:"scale", caption:"scale", types:["float"], singleConnect:true, role:"in", data:{def_val:"1", def_val_blk:"1", code:"instanceData.scale = $scale$"}},
        {name:"color", caption:"color", types:["float4"], singleConnect:true, role:"in", data:{def_val:"float4(1,1,1,1)", def_val_blk:"1,1,1,1", code:"instanceData.color = $color$"}},
        {name:"out", types:["InstanceData"], singleConnect:false, role:"out", hidden:true},
      ],
      properties:[
      ],
      allowLoop:false,
      width:120,
      addHeight:20
    },

    {
      name:"tex size",
      category:"Input",
      synonyms:"texture,dimension,width,height",
      pins:[
        {name:"texture", types:["texture2D"], singleConnect:true, role:"in"},
        {name:"size", types:["float2"], singleConnect:false, role:"out", data:{code:"get_dimensions($texture$)"}}
        ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"get gradient",
      category:"Input",
      synonyms:"get_normal,normal,gradient",
      pins:[
        {name:"texture", types:["texture2D"], singleConnect:true, role:"in"},
        {name:"gradient", types:["float2"], singleConnect:false, role:"out", data:{code:"get2DGradient($texture$, wrap_t$indexof(texture)$, input.tc)"}}
        ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"get gradient uv",
      category:"Input",
      synonyms:"get_normal,normal,gradient",
      pins:[
        {name:"texture", types:["texture2D"], singleConnect:true, role:"in"},
        {name:"coord", types:["float2"], singleConnect:true, role:"in"},
        {name:"gradient", types:["float2"], singleConnect:false, role:"out", data:{code:"getNormal($texture$, wrap_t$indexof(texture)$, $coord$, 1).xy"}}
        ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },


//
//    {
//      name:"polynom",
//      synonyms:"polynom,curve",
//      category:"Curves",
//      pins:[
//        {name:"linear",  types:["polynom2"], singleConnect:false, role:"out", data:{code:"%linear%"}},
//        {name:"quadric", types:["polynom3"], singleConnect:false, role:"out", data:{code:"%quadric%"}},
//        {name:"cubic",   types:["polynom4"], singleConnect:false, role:"out", data:{code:"%cubic%"}},
//        {name:"6-th",    types:["polynom7"], singleConnect:false, role:"out", data:{code:"%6-th%"}}
//      ],
//      properties:[
//        {name:"linear",  type:"polynom2", val:"1, 0 /* 0,1, 1,1 */"},
//        {name:"quadric", type:"polynom3", val:"1, 0, 0 /* 0,1, 0.5,1, 1,1 */"},
//        {name:"cubic",   type:"polynom4", val:"1, 0, 0, 0 /* 0,1, 0.333,1, 0.666,1, 1,1 */"},
//        {name:"6-th",    type:"polynom7", val:"1, 0, 0, 0, 0, 0, 0 /* 0,1, 0.1666,1, 0.333,1, 0.4666,1, 0.666,1, 0.8,1, 1,1 */"}
//      ],
//      allowLoop:false,
//      width:120
//    },
//

    {
      name:"steps curve map",
      category:"Input",
      synonyms:"",
      pins:[
        {name:"x", types:["float"], singleConnect:true, role:"in"},
        {name:"out", types:["float"], singleConnect:false, role:"out", data:{code:"_curve_fn_%#%($x$)", localFunction:curvesHlslCode["steps"] }}
      ],
      properties:[
        {name:"curve", style:"gray", type:"steps_curve", val:"0.0,0.0, 0.333, 0.0, 0.666, 0.0, 1.0, 0.0  /*0.0,0.0, 0.333, 0.0, 0.666, 0.0, 1.0, 0.0 S*/", background:[]},
        {name:"preview", type:"gradient_preview", background:["curve", "curve", "curve"]},
      ],
      allowLoop:false,
      width:180
    },

    {
      name:"linear curve map",
      category:"Input",
      synonyms:"",
      pins:[
        {name:"x", types:["float"], singleConnect:true, role:"in"},
        {name:"out", types:["float"], singleConnect:false, role:"out", data:{code:"_curve_fn_%#%($x$)", localFunction:curvesHlslCode["linear"] }}
      ],
      properties:[
        {name:"curve", style:"gray", type:"linear_curve", val:"0,0,0,0,0,0,0,0,0  /*0.0,0.0, 0.333, 0.0, 0.666, 0.0, 1.0, 0.0 L*/", background:[]},
        {name:"preview", type:"gradient_preview", background:["curve", "curve", "curve"]},
      ],
      allowLoop:false,
      width:180
    },

    {
      name:"polynom curve map",
      category:"Input",
      synonyms:"",
      pins:[
        {name:"x", types:["float"], singleConnect:true, role:"in"},
        {name:"out", types:["float"], singleConnect:false, role:"out", data:{code:"_curve_fn_%#%($x$)", localFunction:curvesHlslCode["polynom"] }}
      ],
      properties:[
        {name:"curve", style:"gray", type:"polynom_curve", val:"0,0,0,0  /*0.0,0.0, 0.333, 0.0, 0.666, 0.0, 1.0, 0.0 P*/", background:[]},
        {name:"preview", type:"gradient_preview", background:["curve", "curve", "curve"]},
      ],
      allowLoop:false,
      width:180
    },

    {
      name:"monotonic curve map",
      category:"Input",
      synonyms:"",
      pins:[
        {name:"x", types:["float"], singleConnect:true, role:"in"},
        {name:"out", types:["float"], singleConnect:false, role:"out", data:{code:"_curve_fn_%#%($x$)", localFunction:curvesHlslCode["monotonic"] }}
      ],
      properties:[
        {name:"curve", style:"gray", type:"monotonic_curve", val:"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  /*0.0,0.0, 0.333, 0.0, 0.666, 0.0, 1.0, 0.0 M*/", background:[]},
        {name:"preview", type:"gradient_preview", background:["curve", "curve", "curve"]},
      ],
      allowLoop:false,
      width:180
    },




    {
      name:"scalar curve cubic",
      synonyms:"polynom",
      category:"Curves",
      pins:[
        {name:"cubic", types:["polynom4"], singleConnect:false, role:"out", data:{code:"float4(%curve%)"}},
      ],
      properties:[
        {name:"curve", style:"gray", type:"polynom4", val:"1, 0, 0, 0 /* 0,1, 0.333,1, 0.666,1, 1,1 */", background:[]},
        {name:"preview", type:"gradient_preview", background:["curve", "curve", "curve"]},
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"color curve cubic",
      synonyms:"polynom,rgba",
      category:"Curves",
      pins:[
        {name:"cubic R", types:["polynom4"], singleConnect:false, role:"out", data:{code:"float4(%R%)"}},
        {name:"cubic G", types:["polynom4"], singleConnect:false, role:"out", data:{code:"float4(%G%)"}},
        {name:"cubic B", types:["polynom4"], singleConnect:false, role:"out", data:{code:"float4(%B%)"}},
        {name:"cubic A", types:["polynom4"], singleConnect:false, role:"out", data:{code:"float4(%A%)"}}
      ],
      properties:[
        {name:"R", style:"red",   type:"polynom4", val:"1, 0, 0, 0 /* 0,1, 0.333,1, 0.666,1, 1,1 */", background:["G", "B"]},
        {name:"G", style:"green", type:"polynom4", val:"1, 0, 0, 0 /* 0,1, 0.333,1, 0.666,1, 1,1 */", background:["R", "B"]},
        {name:"B", style:"blue",  type:"polynom4", val:"1, 0, 0, 0 /* 0,1, 0.333,1, 0.666,1, 1,1 */", background:["R", "G"]},
        {name:"A", style:"gray",  type:"polynom4", val:"1, 0, 0, 0 /* 0,1, 0.333,1, 0.666,1, 1,1 */", background:["R", "G", "B"]},
        {name:"preview", type:"gradient_preview", background:["R", "G", "B"]},
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"calc cubic value",
      category:"Conversion",
      synonyms:"get,polynom,curve",
      pins:[
        {name:"curve", types:["polynom4"], singleConnect:true, role:"in", data:{def_1:true}},
        {name:"t", types:["float"], singleConnect:true, role:"in", data:{def_1:false}},
        {name:"res", types:["float"], singleConnect:false, role:"out", data:{code:"calc_polynom4($curve$, $t$)"}}
      ],
      properties:[
      ],
      typeBypass:true,
      allowLoop:false,
      width:120
    },

    {
      name:"calc cubic vector",
      category:"Conversion",
      synonyms:"get,polynom,curve",
      pins:[
        {name:"curve X", types:["polynom4"], singleConnect:true, role:"in", data:{def_1:true}},
        {name:"curve Y", types:["polynom4"], singleConnect:true, role:"in", data:{def_1:true}},
        {name:"curve Z", types:["polynom4"], singleConnect:true, role:"in", data:{def_1:true}},
        {name:"curve W", types:["polynom4"], singleConnect:true, role:"in", data:{def_1:true}},
        {name:"t", types:["float"], singleConnect:true, role:"in", data:{def_1:false}},
        {name:"res", types:["float4"], singleConnect:false, role:"out", data:{code:"float4(calc_polynom4($curve X$, $t$), calc_polynom4($curve Y$, $t$), calc_polynom4($curve Z$, $t$), calc_polynom4($curve W$, $t$))"}}
      ],
      properties:[
      ],
      typeBypass:true,
      allowLoop:false,
      width:120
    },


    {
      name:"comment",
      synonyms:"remark,text",
      category:"Comment",
      pins:[],
      properties:[
        {name:"comment string", type:"string", val:"//" },
        {name:"font size", type:"int", minVal:"10", maxVal:"120", step:"10", val:"35" }
      ],
      width:100
    },


    {
      name: "decode depth above",
      synonyms: "",
      category: "Built-in",
      pins: [
        { name: "depthHt", types: ["float"], singleConnect: true, role: "in" },
        {
          name: "depth above", types: ["float"], singleConnect: false, role: "out", data: {
            code: "decode_depth_above($depthHt$)"
          }
        },
      ],
      properties: [],
      allowLoop: false,
      width: 180
    },


    {
      name: "smooth noise 3d",
      synonyms: "",
      category: "Built-in",
      pins: [
        { name: "pos", types: ["float3"], singleConnect: true, role: "in" },
        {
          name: "noise_value", types: ["float"], singleConnect: false, role: "out", data: {
            code: "get_smooth_noise3d($pos$)"
          }
        },
      ],
      properties: [],
      allowLoop: false,
      width: 180
    },


    {
      name: "layered fog",
      synonyms: "",
      category: "Built-in",
      pins: [
        {
          name: "fog", types: ["float4"], singleConnect: false, role: "out", data: {
            code: "height_fog_node(make_height_fog_node("+
              "float4($color$).xyz, $world noise freq$, $wind strength$, $constant density$, $perlin density$,"+
              "$height fog scale$, $height fog layer max$, $ground fog density$, $ground fog offset$, $perlin threshold$"+
              "), world_pos, screenTcJittered.z, $wind dir$, $effect$"+
              ")"
          }
        },
        { name: "color", types: ["float4"], singleConnect: true, role: "in", data: {def_val:"%color%"} },
        { name: "wind dir", types: ["float3"], singleConnect: true, role: "in", data: {def_val:"float3(%wind dir.x%, %wind dir.y%, %wind dir.z%)"} },
        { name: "world noise freq", types: ["float3"], singleConnect: true, role: "in", data: {def_val:"float3(%world noise freq.x%, %world noise freq.y%, %world noise freq.z%)"} },
        { name: "wind strength", types: ["float"], singleConnect: true, role: "in", data: {def_val:"%wind strength%"} },
        { name: "constant density", types: ["float"], singleConnect: true, role: "in", data: {def_val:"%constant density%"} },
        { name: "perlin density", types: ["float"], singleConnect: true, role: "in", data: {def_val:"%perlin density%"} },
        { name: "perlin threshold", types: ["float"], singleConnect: true, role: "in", data: {def_val:"%perlin threshold%"} },
        { name: "height fog scale", types: ["float"], singleConnect: true, role: "in", data: {def_val:"%height fog scale%"} },
        { name: "height fog layer max", types: ["float"], singleConnect: true, role: "in", data: {def_val:"%height fog layer max%"} },
        { name: "ground fog density", types: ["float"], singleConnect: true, role: "in", data: {def_val:"%ground fog density%"} },
        { name: "ground fog offset", types: ["float"], singleConnect: true, role: "in", data: {def_val:"%ground fog offset%"} },
        { name: "effect", types: ["float"], singleConnect: true, role: "in", data: {def_val:"%effect%"} },
      ],
      properties: [
        { name: "color", type: "color", minVal: "0", maxVal: "1", step: "0.001", val: "0.5, 0.5, 0.5, 1" },
        { name: "wind dir.x", type: "float", minVal: "-10.0", maxVal: "10.0", step: "0.01", val: "0.2" },
        { name: "wind dir.y", type: "float", minVal: "-10.0", maxVal: "10.0", step: "0.01", val: "0.4" },
        { name: "wind dir.z", type: "float", minVal: "-10.0", maxVal: "10.0", step: "0.01", val: "0.2" },
        { name: "world noise freq.x", type: "float", minVal: "0.001", maxVal: "10.0", step: "0.01", val: "0.2" },
        { name: "world noise freq.y", type: "float", minVal: "0.001", maxVal: "10.0", step: "0.01", val: "0.4" },
        { name: "world noise freq.z", type: "float", minVal: "0.001", maxVal: "10.0", step: "0.01", val: "0.2" },
        { name: "wind strength", type: "float", minVal: "0.0", maxVal: "10.0", step: "0.01", val: "0.4" },
        { name: "constant density", type: "float", minVal: "0.0", maxVal: "1.0", step: "0.0001", val: "0.02" },
        { name: "perlin density", type: "float", minVal: "0.0", maxVal: "10.0", step: "0.01", val: "0.3" },
        { name: "perlin threshold", type: "float", minVal: "0.0", maxVal: "10.0", step: "0.01", val: "0.7" },
        { name: "height fog scale", type: "float", minVal: "0.0", maxVal: "10.0", step: "0.01", val: "0.8" },
        { name: "height fog layer max", type: "float", minVal: "-5000.0", maxVal: "5000.0", step: "0.1", val: "13" },
        { name: "ground fog density", type: "float", minVal: "0.0", maxVal: "100.0", step: "0.1", val: "3.0" },
        { name: "ground fog offset", type: "float", minVal: "-5000.0", maxVal: "5000.0", step: "0.1", val: "-0.5" },
        { name: "effect", type: "float", minVal: "-10.0", maxVal: "10.0", step: "0.1", val: "0.5" },
      ],
      allowLoop: false,
      width: 160
    },

    {
      name: "ambient wind",
      synonyms: "",
      category: "Built-in",
      pins: [
        { name: "dir", types: ["float3"], singleConnect: false, role: "out", data: { code: "sample_ambient_wind_dir( $worldPos$ )" } },
        { name: "strength", types: ["float"], singleConnect: false, role: "out", data: { code: "get_ambient_wind_speed( )" } },
        { name: "worldPos", types: ["float3"], singleConnect: true, role: "in" }
      ],
      properties: [],
      allowLoop: false,
      width: 160
    },

    {
      name:"sample spheres density",
      category:"Input",
      synonyms:"sphere",
      pins:[
        { name: "worldPos", types: ["float3"], singleConnect: true, role: "in" },
        {
          name: "spheres density", types: ["float"], singleConnect: false, role: "out", data: {
            code: "sample_spheres_density(%buffer_name%, %buffer_name%_counter, $worldPos$)"
          }
        },
      ],
      properties:[
        {name:"buffer_name", type:"combobox", items:["--to be replaced--"], val:""}
      ],
      isExternal:true,
      allowLoop:false,
      width:220,
    },

  ];


var GE_defaultExternals =
  [
    // type - int, float, float2, float3, float4
    // template: {type: "float3", name: "camera_pos"},
    {type: "float4", name:"view_vecLB"},
    {type: "float4", name:"view_vecLT"},
    {type: "float4", name:"view_vecRB"},
    {type: "float4", name:"view_vecRT"},
    {type: "float4", name:"jitter_ray_offset"},
    {type: "float4", name:"world_view_pos"},
    {type: "float4", name:"world_to_hmap_low"},
    {type: "float4", name:"heightmap_scale"},
    {type: "float4", name:"tex_hmap_inv_sizes"},
    {type: "float4", name:"vf_wind_dir"},
    {type: "float4", name:"world_to_depth_ao"},
    {type: "float4", name:"depth_ao_heights"},
    {type: "float4", name:"land_detail_mul_offset"},
    {type: "float",  name:"global_time_phase"},
    {type: "float4", name:"gpu_objects_world_coord"},
    {type: "float",  name:"gpu_objects_biomes_count"},
    {type: "float4", name:"gpu_objects_up_vector"},
    {type: "float4", name:"gpu_objects_scale_rotate"},
    {type: "float4", name:"gpu_objects_map_size_offset"},
    {type: "float4", name:"gpu_objects_color_from"},
    {type: "float4", name:"gpu_objects_color_to"},
    {type: "float4", name:"gpu_objects_weights"},
    {type: "float",  name:"gpu_objects_bounding_radius"},
    {type: "float",  name:"gpu_objects_groups_count"},
    {type: "float",  name:"gpu_objects_groups_bbox_offset"},
    {type: "float",  name:"gpu_objects_cell_buffer_offset"},
    {type: "float",  name:"gpu_objects_group_idx"},
    {type: "float",  name:"gpu_objects_seed"},
    {type: "float4", name:"biom_attributes[32]"},
    {type: "texture2D", name:"noise_64_tex_l8"},
    {type: "texture2D_nosampler", name:"volfog_occlusion"},
    {type: "texture2D", name:"land_heightmap_tex"},
    {type: "texture2D", name:"biomeIndicesTex"},
    {type: "texture2D", name:"gpu_objects_map"},
    {type: "texture2D_nosampler", name:"tex_hmap_low"},
    {type: "float4", name:"volfog_froxel_volume_res"},
    {type: "float4", name:"inv_volfog_froxel_volume_res"},
    {type: "float4", name:"volfog_froxel_range_params"},
    {type: "float4", name:"biome_indices_tex_size"},
    {type: "texture2D_nosampler", name:"volfog_poisson_samples"},
    {type: "float", name:"water_level"},
    {type: "float", name:"nbs_clouds_start_altitude2_meters"},
    {type: "texture2D", name:"ambient_wind_tex"},
    {type: "float4",  name:"ambient_wind_map_scale__offset"},
    {type: "float4",  name:"ambient_wind__speed__current_time__previous_time"},
    {type: "texture2D_nosampler", name:"distant_heightmap_tex"},
    {type: "float4",  name:"distant_world_to_hmap_low"},
    {type: "float4",  name:"distant_heightmap_scale"},
    {type: "float4",  name:"distant_heightmap_target_box"},
    {type: "texture2D_nosampler", name:"volfog_mask_tex"},
    {type: "float4",  name:"world_to_volfog_mask"},
    {type: "float4",  name:"froxel_fog_fading_params"},
    {type: "float", name:"daskies_time"},

    {type: "texture3D_nosampler", name:"prev_volfog_shadow"},
    {type: "texture2D_nosampler", name:"volfog_shadow_occlusion"},
    {type: "float4", name:"from_sun_direction"},
    {type: "float4", name:"volfog_shadow_res"},
    {type: "float", name:"volfog_shadow_accumulation_factor"},
    {type: "float", name:"volfog_prev_range_ratio"},


    // for distant fog:
    {type: "texture2D", name:"downsampled_far_depth_tex"},
    {type: "texture2D_nosampler", name:"downsampled_checkerboard_depth_tex"},
    {type: "texture2D_nosampler", name:"prev_downsampled_far_depth_tex"},
    {type: "texture2D_nosampler", name:"downsampled_close_depth_tex"},
    {type: "texture2D", name:"prev_distant_fog_raymarch_start_weights"},
    {type: "float4",  name:"distant_fog_raymarch_resolution"},
    {type: "float4",  name:"distant_fog_reconstruction_resolution"},
    {type: "float4",  name:"distant_fog_raymarch_params_0"},
    {type: "float4",  name:"distant_fog_raymarch_params_1"},
    {type: "float4",  name:"distant_fog_raymarch_params_2"},
    {type: "float",  name:"volfog_media_fog_input_mul"},
    {type: "int",  name:"fog_raymarch_frame_id"},
    {type: "float",  name:"volfog_blended_slice_start_depth"},
    {type: "float4", name:"prev_globtm_psf_0"},
    {type: "float4", name:"prev_globtm_psf_1"},
    {type: "float4", name:"prev_globtm_psf_2"},
    {type: "float4", name:"prev_globtm_psf_3"},
    {type: "float4", name:"sun_color_0"},
    {type: "float4", name:"skylight_params"},
    {type: "float4",  name:"zn_zfar"},
    {type: "float4",  name:"enviSPH0"},
    {type: "float4",  name:"enviSPH1"},
    {type: "float4",  name:"enviSPH2"},
    {type: "float4",  name:"nbs_world_pos_to_clouds_alt__inv_clouds_weather_size__neg_clouds_thickness_m"},
    {type: "float4",  name:"skies_primary_sun_light_dir"},
    {type: "float4",  name:"clouds_origin_offset"},
    {type: "texture2D", name:"clouds_shadows_2d"},
    {type: "float4",  name:"clouds_hole_pos"},
    {type: "float4",  name:"distant_fog_local_view_z"},

    {type: "texture2D_shdArray", name:"static_shadow_tex"},
    {type: "float4", name:"static_shadow_matrix_0_0"},
    {type: "float4", name:"static_shadow_matrix_1_0"},
    {type: "float4", name:"static_shadow_matrix_2_0"},
    {type: "float4", name:"static_shadow_matrix_3_0"},
    {type: "float4", name:"static_shadow_matrix_0_1"},
    {type: "float4", name:"static_shadow_matrix_1_1"},
    {type: "float4", name:"static_shadow_matrix_2_1"},
    {type: "float4", name:"static_shadow_matrix_3_1"},
    {type: "float4", name:"static_shadow_cascade_0_scale_ofs_z_tor"},
    {type: "float4", name:"static_shadow_cascade_1_scale_ofs_z_tor"},
    {type: "int", name:"distant_fog_use_static_shadows"},
    {type: "float", name:"debug_disable_static_pcf_center_sample"},
  ];


function GE_verifyDescriptions()
{
  // verify conversions:
  for (var i = 0; i < GE_conversions.length; i++)
  {
    if (GE_conversions[i].length != 3)
      alert("ERROR: Invalid length of GE_conversions[" + i + "]");

    var testType = function(t)
    {
      if (GE_implicitGroupConversionOrder.indexOf(t) < 0)
        alert("ERROR: type '" + t + "' not found in GE_implicitGroupConversionOrder");
    }
    testType(GE_conversions[i][0]);
    testType(GE_conversions[i][1]);
  }

  if (!GE_nodeDescriptions || GE_nodeDescriptions.length == 0)
    alert("ERROR: GE_nodeDescriptions is empty");

  var names = [];

  for (var i = 0; i < GE_nodeDescriptions.length; i++)
  {
    var desc = GE_nodeDescriptions[i];
    if (!desc.name || desc.name == "")
      alert("ERROR: GE_nodeDescriptions: 'name' must be defined (index=" + i + ")");

    if (names.indexOf(desc.name) >= 0)
      alert("ERROR: GE_nodeDescriptions: duplicate of name '" + desc.name + "'");

    names.push(desc.name);

    if (!desc.category || desc.category == "")
      alert("ERROR: GE_nodeDescriptions: 'category' must be defined (index=" + i + ")");

    if (!desc.pins)
      alert("ERROR: GE_nodeDescriptions: 'pins[]' must be defined (index=" + i + ")");

    if (!desc.properties)
      alert("ERROR: GE_nodeDescriptions: 'properties[]' must be defined (index=" + i + ")");

    if (!desc.width)
      alert("ERROR: GE_nodeDescriptions: 'width' must be defined (index=" + i + ")");


    var pinNames = [];
    var propNames = [];

    for (var j = 0; j < desc.pins.length; j++)
    {
      var pin = desc.pins[j];

      if (!pin.name || pin.name == "" || pin.name.indexOf("%") >= 0 || pin.name.indexOf("$") >= 0)
        alert("ERROR: invalid pin name '" + pin.name + "' in '" + desc.name + "'");

      if (pinNames.indexOf(pin.name) >= 0)
        alert("ERROR: duplicate of pin name '" + pin.name + "' in '" + desc.name + "'");

      pinNames.push(pin.name);

      if (pin.caption && pin.caption.indexOf("$") >= 0)
        alert("ERROR: only properties can be used in pin caption (pin '" + pin.name + "' in '" + desc.name + "')");

      if (pin.role != "out" && pin.role != "in" && pin.role != "any")
        alert("ERROR: pin role can be only 'in', 'out' or 'any' (pin '" + pin.name + "' in '" + desc.name + "')");
    }

    for (var j = 0; j < desc.properties.length; j++)
    {
      var prop = desc.properties[j];

      if (!prop.name || prop.name == "" || prop.name.indexOf("%") >= 0 || prop.name.indexOf("$") >= 0)
        alert("ERROR: invalid property name '" + prop.name + "' in '" + desc.name + "'");

      if (propNames.indexOf(prop.name) >= 0)
        alert("ERROR: duplicate of property name '" + prop.name + "' in '" + desc.name + "'");

      propNames.push(prop.name);

      if (["combobox", "bool", "int", "float", "string", "color", "gradient_preview",
           "polynom2", "polynom3", "polynom4", "polynom5", "polynom6", "polynom7", "polynom8",
           "linear2", "linear3", "linear4", "linear5", "linear6", "linear7", "linear8",
           "monotonic2", "monotonic3", "monotonic4", "monotonic5", "monotonic6", "monotonic7", "monotonic8",
           "steps_curve", "linear_curve", "monotonic_curve", "polynom_curve"
           ].indexOf(prop.type) < 0)
        alert("ERROR: invalid property type = '" + prop.type + "' ('" + prop.name + "' in '" + desc.name + "')");

      if (prop.type == "combobox")
      {
        if (!prop.items || prop.items.length == 0)
          alert("ERROR: missed 'items' for combobox ('" + prop.name + "' in '" + desc.name + "')");
      }
    }
  }
}
GE_verifyDescriptions();



var GE_beforeStringifyGraph = function(graph)
{
  generateAdditionalText(graph, false);
}


function generateAdditionalText(graph, useVarPool)
{
  var newLine = "\n";
  var singleIndent = "  ";
  var indent = singleIndent;
  var hlsl = "";
  var blk = newLine;

  for (var i = 0; i < graph.edgeCount; i++)
  {
    var edge = graph.edges[i];
    if (!edge)
      continue;

    var from = graph.elems[edge.elemA];
    var to = graph.elems[edge.elemB];
    if (from.pins[edge.pinA].role === "in")
    {
      var p = graph.elems[edge.elemA].pins[edge.pinA];
      p.connectElem = edge.elemB;
      p.connectPin = edge.pinB;
      p.connectFromType = graph.elems[p.connectElem].pins[p.connectPin].types[0];
      p.connectToType = p.types[0];
      p.connected = true;
      graph.elems[edge.elemB].pins[edge.pinB].connected = true;
    }
    if (to.pins[edge.pinB].role === "in")
    {
      var p = graph.elems[edge.elemB].pins[edge.pinB];
      p.connectElem = edge.elemA;
      p.connectPin = edge.pinA;
      p.connectFromType = graph.elems[p.connectElem].pins[p.connectPin].types[0];
      p.connectToType = p.types[0];
      p.connected = true;
      graph.elems[edge.elemA].pins[edge.pinA].connected = true;
    }
  }

  var hlslTypes =
  {
    "bool":   "bool",
    "int":    "int",
    "float":  "float",
    "float2": "float2",
    "float3": "float3",
    "float4": "float4",
    "polynom4":   "float4",
    "linear4":    "linear4_t",
    "monotonic4": "monotonic4_t",
  };

  var blkTypes =
  {
    "bool":   "b",
    "int":    "i",
    "float":  "r",
    "float2": "p2",
    "float3": "p3",
    "float4": "p4",
    "string": "t",
    "polynom4": "p4",
   // "linear4":  "linear4", // not implemented
   // "monotonic4": "monotonic4", // not implemented
  };


  var makeSafeFilename = function(name)
  {
    var code_0 = "0".charCodeAt(0);
    var code_9 = "9".charCodeAt(0);
    var code_a = "a".charCodeAt(0);
    var code_z = "z".charCodeAt(0);
    var code_A = "A".charCodeAt(0);
    var code_Z = "Z".charCodeAt(0);
    var code_underscore = "_".charCodeAt(0);

    for (var j = name.length - 1; j >= 0; j--)
    {
      var ch = name.charCodeAt(j);
      var good = (ch >= code_0 && ch <= code_9) || (ch >= code_a && ch <= code_z) ||
                 (ch >= code_A && ch <= code_Z) || ch == code_underscore;

      if (!good)
      {
        name = name.slice(0, j) + "_" + name.slice(j + 1);
      }
    }

    return name;
  }


  var pinTypeToCodeType = function(type)
  {
    return hlslTypes[type] ? hlslTypes[type] : type;
  }

  var pinTypeToBlkType = function(type)
  {
    return blkTypes[type] ? blkTypes[type] : type;
  }


  var convertType = function(typeFrom, typeTo, expression)
  {
    if (typeFrom === typeTo)
      return expression;

    for (var i = 0; i < GE_conversions.length; i++)
      if (GE_conversions[i][0] === typeFrom && GE_conversions[i][1] === typeTo)
        return GE_conversions[i][2].split("$").join(expression);

    return expression; // error
  }

  var hasAnyConnection = function(elem)
  {
    var anyConnection = false;
    for (var j = 0; j < elem.pins.length; j++)
      if (elem.pins[j].connected)
      {
        return true;
      }

    return false;
  }

  var getPinType = function(elem, pin)
  {
    if (pin.group && elem.groupTypes[pin.group])
      return elem.groupTypes[pin.group];
    else
      return pin.types[0];
  }

  var getDefaultValueBlk = function(elem, pin)
  {
    if (pin.data && pin.data.def_val_blk)
      return pin.data.def_val_blk;

    var type = getPinType(elem, pin);

    var res = (pin.data && pin.data.def_1) ? GE_defaultValuesOneBlk[type] : GE_defaultValuesZeroBlk[type];
    if (!res)
      return res = "0 /*unknown type*/"; // error

    return res;
  }

  var getDefaultValue = function(elem, pin)
  {
    if (pin.data && pin.data.def_val)
      return pin.data.def_val;

    var type = getPinType(elem, pin);

    var res = (pin.data && pin.data.def_1) ? GE_defaultValuesOne[type] : GE_defaultValuesZero[type];
    if (!res)
      return res = "0 /*unknown type*/"; // error

    return res;
  }

  var count_numbers_in_string = function(s)
  {
    return JSON.parse('{"a":[' + s.replace(/(\/\*([\s\S]*?)\*\/)/gm, "") + ']}').a.length;
  }

  var get_final_digits = function(s)
  {
    var res = "";
    for (var i = s.length - 1; i >= 0; i--)
    {
      var code = s.charCodeAt(i);
      if (code >= 48 && code <= 57) // 0..9
        res = s[i] + res;
      else
        break;
    }

    return res;
  }

  var substitute = function(code, elem)
  {
    for (var i = 0; i < elem.pins.length; i++)
    {
      var p = elem.pins[i];
      var replaceFrom = "$" + p.name + "$";
      var replaceTo = "";

      var type = p.connectToType;
      if (type)
      {
        var toPin = graph.elems[p.connectElem].pins[p.connectPin];
        var toVar = toPin.customVarPoolName ? toPin.customVarPoolName :
           (toPin.customVarName ? "" + toPin.customVarName : "_v_" + p.connectElem + "_" + p.connectPin);
        replaceTo = convertType(p.connectFromType, p.connectToType, toVar);
      }
      else
      {
        if (p.customPinInput)
          replaceTo = "" + p.customPinInput;
        else if (p.data && p.data.def_val)
          replaceTo = p.data.def_val;
        else
        {
          type = getPinType(elem, p);

          replaceTo = (p.data && p.data.def_1) ? GE_defaultValuesOne[type] : GE_defaultValuesZero[type];
          if (!replaceTo)
            replaceTo = "0"; // error
        }
      }

      if (p.group)
      {
        var groupType = elem.groupTypes[p.group];
        replaceTo = convertType(type, groupType, replaceTo);
      }


      if (code.indexOf("$indexof(" + p.name) >= 0)
      {
        code = code.split("$indexof(" + p.name + ")$").join(get_final_digits(replaceTo));
      }

      code = code.split(replaceFrom).join(replaceTo);
    }

    for (var i = 0; i < elem.properties.length; i++)
    {
      var v = elem.properties[i];
      var replaceFrom = "%" + v.name + "%";
      var replaceTo = v.value;
      code = code.split(replaceFrom).join(replaceTo);

      if (code.indexOf("%countof(" + v.name) >= 0)
      {
        var replaceFrom = "%countof(" + v.name + ")%";
        var replaceTo = count_numbers_in_string(v.value);
        code = code.split(replaceFrom).join(replaceTo);
      }
    }

    {
      var replaceFrom = "%#%";
      var replaceTo = elem.id;
      code = code.split(replaceFrom).join(replaceTo);
    }


    return code;
  }

  var setProperty = function(elem, propName, propValue)
  {
    var props = elem.properties;
    for (var i = 0; i < props.length; i++)
      if (props[i].name === propName)
      {
        props[i].value = propValue;
        return true;
      }

    return false;
  }


  var getProperty = function(elem, propName, def)
  {
    var props = elem.properties;
    for (var i = 0; i < props.length; i++)
      if (props[i].name === propName)
        return props[i].value;

    return def;
  }


  var hashCode31 = function(s)
  {
    var hash = 0;
    if (s.length == 0)
      return hash;

    var char = 0;

    for (var i = 0; i < s.length; i++)
    {
      char = s.charCodeAt(i);
      hash = ((hash << 5) - hash) + char;
      hash = hash & hash;
    }
    return hash;
  }


  var sortedRemap = new Array(graph.elemCount);
  for (var i = 0; i < sortedRemap.length; i++)
    sortedRemap[i] = i;

  sortedRemap.sort(function (a, b) {
    var aElem = graph.elems[a];
    var ay = aElem ? aElem.view.y : a;
    var bElem = graph.elems[b];
    var by = bElem ? bElem.view.y : b;
    return ay - by;
  })


  var externalNamePrefix = ""; // "_"


  var resultColors = 0;

  var params = [];
  var inputs = {
    "int":    [],
    "int4":   [],
    "float":  [],
    "float2": [],
    "float3": [],
    "float4": [],
    "texture2D": [],
    "texture3D": [],
    "texture2D_nosampler": [],
    "texture3D_nosampler": [],
    "texture2D_shdArray": [],
    "Buffer": [],
  };


  for (var i = 0; i < GE_defaultExternals.length; i++)
  {
    var d = GE_defaultExternals[i];
    inputs[d.type].push({
      elem: -1,
      pin: -1,
      externalName: d.name
    });

    if (d.type === "texture2D" || d.type === "texture3D" || d.type === "texture2D_shdArray" || d.type === "texture2D_nosampler" || d.type === "texture3D_nosampler")
      inputs[d.type][inputs[d.type].length - 1].customSamplerName = d.name;

    if (d.type === "Buffer")
      inputs["int"].push({
        elem: -1,
        pin: -1,
        externalName: d.name + "_counter"
      });
  }


  var outputs = [];
  for (var ii = 0; ii < graph.elemCount; ii++)
  {
    var i = sortedRemap[ii];
    var e = graph.elems[i];
    if (e)
    {
      for (var j = 0; j < e.pins.length; j++)
      {
        var p = e.pins[j];
        var pinType = getPinType(e, p);

        if (p.role === "in" && p.data && p.data.result_color)
        {
          if (!setProperty(e, "MRT index", resultColors))
            alert("Cannot find property 'MRT index' in '" + e.name + "'");
        //  resultColors++; - not needed for fog
        }


        if (e.isExternal && p.role === "out" && !p.hidden && e.pins.length === 1)
        {
          var name = getProperty(e, "name", null);
          var foundInExternals = false;

          for (var k = 0; k < GE_defaultExternals.length; k++)
            if (GE_defaultExternals[k].name === name
              && GE_defaultExternals[k].type === pinType
              && (pinType === "texture2D" || pinType === "texture3D"  || pinType === "texture2D_shdArray"|| pinType === "texture2D_nosampler" || pinType === "texture3D_nosampler")
              )
            {
              p.customVarName = name;
              foundInExternals = true;
              break;
            }

          if (!foundInExternals)
          {
            if (pinType === "texture2D" || pinType === "texture3D" || pinType === "texture2D_shdArray" || pinType === "texture2D_nosampler" || pinType === "texture3D_nosampler")
              p.customVarName = "sampler" + inputs[pinType].length;

            inputs[pinType].push({
              elem: i,
              pin: j,
              externalName: name
            });
          }
        }

        if (p.role === "in" && !p.connected && (["bool", "int", "float", "float2", "float3",
          "float4"].indexOf(pinType) >= 0))
        {
          var comment = graph.elems[i].pinComments ? graph.elems[i].pinComments[j] : null;
          var n = (comment && comment.length > 0) ? comment : p.name;

          //p.customPinInput = externalNamePrefix + makeSafeFilename(n);

          var exists = false;
          for (var k = 0; k < params.length; k++)
            if (p.customPinInput === params[k].safeName)
            {
              exists = true;
              break;
            }

          if (!exists)
          {
            params.push({
              safeName: p.customPinInput,
              originalName: n,
              type: pinType,
              blk_value: getDefaultValueBlk(e, p),
              value: getDefaultValue(e, p),
            });
          }
        }



        if (p.role === "out" && e.externalName && e.externalName !== "" && (["bool", "int", "float", "float2", "float3",
          "float4"].indexOf(pinType) >= 0) && p.data && p.data.blk_code)
        {
          p.customVarName = externalNamePrefix + makeSafeFilename(e.externalName);
          p.connected = false;

          var exists = false;
          for (var k = 0; k < params.length; k++)
            if (p.customVarName === params[k].safeName)
            {
              exists = true;
              break;
            }
          
          if (!exists)
          {
            //print("PAARM 2: " + e.name + " " + p.customVarName + " " + e.externalName + " " + pinType);
            params.push({
              safeName: p.customVarName,
              originalName: e.externalName,
              type: pinType,
              blk_value: substitute(p.data.blk_code, e),
              value: substitute(p.data.code, e),
            });
          }
        }
      }

      for (var j = 0; j < e.properties.length; j++)
      {
        if (e.properties[j].name !== "buffer_name")
          continue;

        var name = getProperty(e, "buffer_name", null);

        inputs["Buffer"].push({
          elem: i,
          pin: j,
          externalName: name
        });
        inputs["int"].push({
          elem: i,
          pin: j,
          externalName: name + "_counter"
        });
      }
    }
  }

  for (var i = 0; i < params.length; i++)
  {
    var descPin = null;
    for (var j = 0; j < graph.description.pins.length; j++)
    {
      var p = graph.description.pins[j];
      if (!descPin && p.name === params[i].originalName && p.types[0] === params[i].type && p.role === "in")
      {
        descPin = p;
        break;
      }
    }


    if (params[i].type === "bool")
    {
      graph.description.properties.push({
        name: params[i].originalName,
        type: params[i].type,
        val: params[i].blk_value.trim(),
      });
      if (descPin)
      {
        descPin.data.blk_code = "%" + params[i].originalName + "%";
        descPin.data.code = descPin.data.blk_code;
      }
    }

    if (params[i].type === "int")
    {
      graph.description.properties.push({
        name: params[i].originalName,
        type: params[i].type, minVal:"-65535", maxVal:"65535", step:"1",
        val: params[i].blk_value.trim(),
      });
      if (descPin)
      {
        descPin.data.blk_code = "%" + params[i].originalName + "%";
        descPin.data.code = descPin.data.blk_code;
      }
    }

    if (params[i].type === "float")
    {
      graph.description.properties.push({
        name: params[i].originalName,
        type: params[i].type, minVal:"-1e9", maxVal:"1e9", step:"0.001",
        val: params[i].blk_value.trim(),
      });
      if (descPin)
      {
        descPin.data.blk_code = "%" + params[i].originalName + "%";
        descPin.data.code = descPin.data.blk_code;
      }
    }

    if (params[i].type === "float2")
    {
      graph.description.properties.push({
        name: params[i].originalName + ".x",
        type: "float", minVal:"-1e9", maxVal:"1e9", step:"0.001",
        val: params[i].blk_value.split(",")[0].trim(),
      });
      graph.description.properties.push({
        name: params[i].originalName + ".y",
        type: "float", minVal:"-1e9", maxVal:"1e9", step:"0.001",
        val: params[i].blk_value.split(",")[1].trim(),
      });
      if (descPin)
      {
        descPin.data.blk_code = "%" + params[i].originalName + ".x%, %" + params[i].originalName + ".y%";
        descPin.data.code = "float2(" + descPin.data.blk_code + ")";
      }
    }

    if (params[i].type === "float3")
    {
      graph.description.properties.push({
        name: params[i].originalName + ".x",
        type: "float", minVal:"-1e9", maxVal:"1e9", step:"0.001",
        val: params[i].blk_value.split(",")[0].trim(),
      });
      graph.description.properties.push({
        name: params[i].originalName + ".y",
        type: "float", minVal:"-1e9", maxVal:"1e9", step:"0.001",
        val: params[i].blk_value.split(",")[1].trim(),
      });
      graph.description.properties.push({
        name: params[i].originalName + ".z",
        type: "float", minVal:"-1e9", maxVal:"1e9", step:"0.001",
        val: params[i].blk_value.split(",")[2].trim(),
      });
      if (descPin)
      {
        descPin.data.blk_code = "%" + params[i].originalName + ".x%, %" + params[i].originalName + ".y%, %" + params[i].originalName + ".z%";
        descPin.data.code = "float3(" + descPin.data.blk_code + ")";
      }
    }

    if (params[i].type === "float4")
    {
      graph.description.properties.push({
        name: params[i].originalName + ".x",
        type: "float", minVal:"-1e9", maxVal:"1e9", step:"0.001",
        val: params[i].blk_value.split(",")[0].trim(),
      });
      graph.description.properties.push({
        name: params[i].originalName + ".y",
        type: "float", minVal:"-1e9", maxVal:"1e9", step:"0.001",
        val: params[i].blk_value.split(",")[1].trim(),
      });
      graph.description.properties.push({
        name: params[i].originalName + ".z",
        type: "float", minVal:"-1e9", maxVal:"1e9", step:"0.001",
        val: params[i].blk_value.split(",")[2].trim(),
      });
      graph.description.properties.push({
        name: params[i].originalName + ".w",
        type: "float", minVal:"-1e9", maxVal:"1e9", step:"0.001",
        val: params[i].blk_value.split(",")[3].trim(),
      });
      if (descPin)
      {
        descPin.data.blk_code = "%" + params[i].originalName + ".x%, %" + params[i].originalName + ".y%, %" + params[i].originalName + ".z%, %" + params[i].originalName + ".w%";
        descPin.data.code = "float4(" + descPin.data.blk_code + ")";
      }
    }
  }


  var shaderLocalFunctions = "";
  var outputStructCopy = "";
  var mrtOutputsDecl = "";
  var mrtOutputsInit = "";

  blk += "params {" + newLine;
  for (var i = 0; i < params.length; i++)
    blk += indent + params[i].safeName + ":" + pinTypeToBlkType(params[i].type) + " = " + params[i].blk_value + newLine;

  blk += "}" + newLine + newLine;

  var inputTypes = ["int", "float", "float2", "float3", "float4",
    "texture2D", "texture3D", "texture2D_shdArray", "texture2D_nosampler", "texture3D_nosampler", "Buffer"];
  var cbufferTypes = ["int", "float", "float2", "float3", "float4"];

  var addedExtNames = [];

  var cbuffer = indent + "cbuffer global : register(b0) {" + newLine;

  for (var j = 0; j < inputTypes.length; j++)
  {
    var key = inputTypes[j];
    var isCbuffer = cbufferTypes.indexOf(key) >= 0;
    blk += "inputs_" + key + " {" + newLine;

    var cnt = 0;
    for (var i = 0; i < inputs[key].length; i++)
    {
      var extName = inputs[key][i].externalName;
      if (addedExtNames.indexOf(extName) < 0)
      {
        blk += indent + 'name:t="' + extName + '" // index=' + cnt + newLine;
        cnt++;
        addedExtNames.push(extName);

        if (isCbuffer)
        {
          cbuffer += indent + indent + key + " " + inputs[key][i].externalName + ";" + newLine;
        }
      }
    }

    blk += "}" + newLine + newLine;
  }

  cbuffer += indent + "};" + newLine + newLine;


  if (resultColors > 1)
    blk += "outputs:i=" + resultColors + newLine;

  blk += newLine + 'input_declaration:t="""' + newLine + newLine;

  blk += cbuffer;

  mrtOutputsDecl += indent + "struct MRTOutput {";

  for (var i = 0; i < Math.max(resultColors, 1); i++)
  {
    mrtOutputsDecl += " float4 col" + i + ";";
    mrtOutputsInit += indent + "result.col" + i + " = float4(0.0, 0.0, 0.0, 0.0);";
  }

  mrtOutputsDecl += "};" + newLine;


  var fillBlkTexture = function(typeStr, codeStr, registerOffset, samplerType)
  {
    for (var i = 0; i < inputs[typeStr].length; i++)
    {
      var d = inputs[typeStr][i];
      var registerId = registerOffset + i;
      if (d.customSamplerName)
      {
        blk += indent + codeStr + " " + d.customSamplerName + ":register(t" + registerId + ");" + newLine;
        if (samplerType == 1)
          blk += indent + "SamplerState " + d.customSamplerName + "_samplerstate:register(s" + registerId + ");" + newLine;
        else if (samplerType == 2)
          blk += indent + "SamplerComparisonState " + d.customSamplerName + "_cmpSampler:register(s" + registerId + ");" + newLine;
      }
      else
      {
        blk += indent + codeStr + " sampler" + i + ":register(t" + registerId + ");" + newLine;
        if (samplerType == 1)
          blk += indent + "SamplerState sampler" + i + "_samplerstate:register(s" + registerId + ");" + newLine;
        else if (samplerType == 2)
          blk += indent + "SamplerComparisonState sampler" + i + "_cmpSampler:register(s" + registerId + ");" + newLine;
      }
    }
    return inputs[typeStr].length;
  }

  var registerOffset = 0;
  registerOffset += fillBlkTexture("texture2D", "Texture2D", registerOffset, 1);
  registerOffset += fillBlkTexture("texture3D", "Texture3D", registerOffset, 1);
  registerOffset += fillBlkTexture("texture2D_shdArray", "Texture2DArray", registerOffset, 2);
  registerOffset += fillBlkTexture("texture2D_nosampler", "Texture2D", registerOffset, 0);
  registerOffset += fillBlkTexture("texture3D_nosampler", "Texture3D", registerOffset, 0);

  for (var i = 0; i < inputs["Buffer"].length; i++)
  {
    var d = inputs["Buffer"][i];
    blk += indent + "StructuredBuffer<float4> " + d.externalName + ":register(t" + (registerOffset + i) + ");" + newLine;
  }


  blk += newLine + '"""' + newLine + newLine;


  var processed = new Array(graph.elemCount);
  for (var i = 0; i < processed.length; i++)
    processed[i] = false;

  var earlyExitNode = -1;
  for (var i = 0; i < graph.elemCount; i++)
  {
    var e = graph.elems[i];
    if (e && e.descName === GE_volfog_early_exit_node_name)
    {
      if (earlyExitNode >= 0)
      {
        alert("ERROR: multiple early exit nodes are present!");
        break;
      }
      earlyExitNode = i;
    }
  }
  var earlyExitConnections = new Array();
  var earlyExitStack = new Array();
  if (earlyExitNode >= 0)
  {
    earlyExitStack.push(earlyExitNode);
    for (var iteration = 0; earlyExitStack.length > 0 && iteration < graph.elemCount; iteration++)
    {
      var elemIndex = earlyExitStack.pop();
      earlyExitConnections.push(elemIndex);
      processed[elemIndex] = true;

      var e = graph.elems[elemIndex];
      for (var j = 0; j < e.pins.length; j++)
      {
        var p = e.pins[j];
        if (p.connectFromType && p.role === "in" && !processed[p.connectElem])
        {
          earlyExitStack.push(p.connectElem);
        }
      }
    }
  }


  var processed = new Array(graph.elemCount);
  for (var i = 0; i < processed.length; i++)
    processed[i] = false;

  function processGraphElem(elemIndex)
  {
    var e = graph.elems[elemIndex];

    if (e && !processed[elemIndex])
    {
      var allInProcessed = true;
      for (var pinIndex = 0; pinIndex < e.pins.length; pinIndex++)
      {
        var p = e.pins[pinIndex];
        if (p.connectFromType && p.role === "in" && !processed[p.connectElem])
        {
          allInProcessed = false;
          break;
        }
      }

      if (allInProcessed)
      {
        processed[elemIndex] = true;

        for (var j = 0; j < e.pins.length; j++)
        {
          var p = e.pins[j];

          if (p.data && p.data.localFunction)
          {
            shaderLocalFunctions += substitute(p.data.localFunction, e) + newLine;
          }

          if (p.data && p.data.code)
          {
            if (p.role === "in")
              hlsl += indent + substitute(p.data.code, e) + ";" + newLine;
            else // out
            {
              var t = p.types[0];

              if (p.types.length > 1 && p.group)
              {
                for (var k = 0; k < e.pins.length; k++)
                  if (k != j && e.pins[k].group === p.group && e.pins[k].types.length == 1)
                  {
                    t = e.pins[k].types[0];
                    break;
                  }
              }

              if (p.connected)
              {
                var varName = p.customVarPoolName ? p.customVarPoolName : (p.customVarName ? "" + p.customVarName : "_v_" + elemIndex + "_" + j);
                hlsl += indent + pinTypeToCodeType(t) +
                  " " + varName + " = " + substitute(p.data.code, e) + ";" + newLine;
              }
              else if (p.customPinOutput)
              {
              // hlsl += indent + "_out." + p.customPinOutput + " = " + substitute(p.data.code, e) + ";" + newLine;
              }
            }
          }
        }
        return true;
      }
    }
    return false;
  }


  if (earlyExitNode >= 0)
  {
    hlsl += indent + "bool "+GE_volfog_early_exit_var_name+" = true;" + newLine;
    var changed = true;
    for (var iteration = 0; changed && iteration < earlyExitConnections.length; iteration++)
    {
      changed = false;
      for (var i = 0; i < earlyExitConnections.length; i++)
      {
        if (processGraphElem(earlyExitConnections[i]))
        {
          changed = true;
        }
      }
    }
    hlsl += indent + "BRANCH" + newLine;
    hlsl += indent + "if ("+GE_volfog_early_exit_var_name+")" + newLine;
    hlsl += indent + "{" + newLine;
    indent = singleIndent + singleIndent;
  }

  var changed = true;
  for (var iteration = 0; changed && iteration < graph.elemCount; iteration++)
  {
    changed = false;
    for (var i = 0; i < graph.elemCount; i++)
    {
      if (processGraphElem(i))
      {
        changed = true;
      }
    }
  }

  if (earlyExitNode >= 0)
  {
    indent = singleIndent;
    hlsl += indent + "}" + newLine;
  }



  blk += 'mrt_outputs_decl:t="""' + newLine + newLine +
         mrtOutputsDecl + newLine +
         '"""' + newLine + newLine;

  blk += 'mrt_outputs_init:t="""' + newLine + newLine +
         mrtOutputsInit + newLine +
         '"""' + newLine + newLine;

  blk += 'shader_local_functions:t="""' + newLine + newLine +
          shaderLocalFunctions + newLine +
         '"""' + newLine + newLine;

  blk += 'shader_code:t="""' + newLine + newLine +
         hlsl + newLine +
         '"""' + newLine + newLine;

  var metadata_consts = "";
  for (var ii = 0; ii < graph.elemCount; ii++)
  {
    var e = graph.elems[ii];
    if (e && e.descName == "GPU object parameters")
    {
      metadata_consts +=
        "grid_tiling: i = %grid_tiling%\n" +
        "grid_size: i = %grid_size%\n" +
        "cell_size: r = %cell_size%\n" +
        "seed: i = %seed%\n" +
        "up_vector: p3 = %up_vector_x%, %up_vector_y%, %up_vector_z%\n" +
        "scale_range: p2 = %scale_range_x%, %scale_range_y%\n" +
        "biom_indexes: p4 = %biom_indexes_x%, %biom_indexes_y%, %biom_indexes_z%, %biom_indexes_w%\n" +
        "is_using_normal: b = %is_using_normal%\n" +
        "incline_delta: r = %incline_delta%\n" +
        "rotate_range: p2 = %rotate_range_x%, %rotate_range_y%\n" +
        'map: t = "%map%"' +
        "map_size: p2 = %map_size_x%, %map_size_y%\n" +
        "map_offset: p2 = %map_offset_x%, %map_offset_y%\n" +
        "color_from: p4 = %color_from%\n" +
        "color_to: p4 = %color_to%\n" +
        "max_placed_on_ri: i = %max_placed_on_ri%\n" +
        "slope_factor: p2 = %slope_factor_x%, %slope_factor_y%\n" +
        "hardness: r = %hardness%\n";
      for (var j = 0; j < e.properties.length; j++)
      {
        var v = e.properties[j];
        var replaceFrom = "%" + v.name + "%";
        var replaceTo = v.value;
        metadata_consts = metadata_consts.split(replaceFrom).join(replaceTo);
      }
    }
  }

  blk = "\nmetadata {\n" + metadata_consts + "}\n" + blk;

  blk = "/*SHADER_BLK_START*/" + blk + "/*SHADER_BLK_END*/";

  graph["code"] = blk;
  graph["hashOfCode"] = "/*HASH_OF_CODE_START*/" + hashCode31(blk) + "/*HASH_OF_CODE_END*/";
}

function GE_setExternalNames(nodeName, items)
{
  if (items[0] !== "")
    items = [""].concat(items);

  for (var i = 0; i < GE_nodeDescriptions.length; i++)
    if (GE_nodeDescriptions[i].name === nodeName)
    {
      var props = GE_nodeDescriptions[i].properties;
      for (var j = 0; j < props.length; j++)
        if (props[j].name === "name" || props[j].name === "buffer_name")
        {
          props[j].items = items;
          break;
        }

      break;
    }
}
