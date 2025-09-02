"use strict";

var GE_typeColors =
  {
    "bool": "#777",
    "int": "#18f",
    "uint":"#00a",
    "float": "#0a0",
    "float2": "#ff0",
    "float3": "#0ff",
    "float4": "#f0f",
    "texture2D": "#f81",
    "texture3D": "#f81",
    "texture2DArray": "#f81",
    "texture2D_shdArray": "#f81",
    "BiomeData": "#f90",
    "NBSGbuffer": "#a1a",
    "material_t": "#864",
    "Layer_t":"#f40",
    "mask_t":"#4f0"
  };

var texTypes =
{
  "texture2D":"texture2D",
  "texture3D":"texture3D",
  "texture2DArray":"texture2DArray"
}


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

var genColorGradFunctions = function()
{
  var baseCurveFunction =
    "float color_curve_!VECTOR_COMPONENT!_%#%(float4 input)\n" +
    "{\n" +
    "  const float a[] = {%gradient_!VECTOR_COMPONENT!%};\n" +
    "  int count = %countof(gradient_!VECTOR_COMPONENT!)%;\n" +
    "\n" +
    "  if (count < 2)\n" +
    "    return 0.f;\n" +
    "  if (count < 3)\n" +
    "    return a[1];\n" +
    "\n" +
    "  float x = saturate(input.!VECTOR_COMPONENT!);\n" +
    "  int idx = 0;\n" +
    "  for (int i = 0; i < count; i += 3)\n" +
    "    if (a[i] < x)\n" +
    "      idx = i;\n" +
    "\n" +
    "  return a[idx + 1] + a[idx + 2] * (x - a[idx]);\n" +
    "}\n\n"

  return baseCurveFunction.split("!VECTOR_COMPONENT!").join("r")
          + baseCurveFunction.split("!VECTOR_COMPONENT!").join("g")
          + baseCurveFunction.split("!VECTOR_COMPONENT!").join("b")
          + baseCurveFunction.split("!VECTOR_COMPONENT!").join("a");

}

var GE_implicitGroupConversionOrder =
  ["bool", "int", "float", "float2", "float3", "float4"];

var GE_nodeDescriptionsCommon = [
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
      {name:"texture", caption: "(%name%)", selectableTypes:texTypes, singleConnect:false, role:"out", data:{}}
    ],
    properties:[
      {name:"name", type:"combobox", items:["--to be replaced--"], val:""},
      {name:"type", type:"multitype_combobox", items: Object.keys(texTypes), value:Object.keys(texTypes)[0], linkedPin: "texture"}
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
    category: "Conversion",
    synonyms: "select, vec, component, swizzle, conversion",
    pins: [
      { name: "vec", selectableTypes:{"2":"float2", "3":"float3", "4":"float4"}, singleConnect: true, role: "in"},
      { name: "x", types:["float"], singleConnect:false, role:"out", data:{code:"$vec$.x"}},
      { name: "y", types:["float"], singleConnect:false, role:"out", data:{code:"$vec$.y"}},
      { name: "z", types:["float"], singleConnect:false, role:"out", optionalIdentifier:{idx:3, prop:"vec length"}, data:{code:"$vec$.z"}},
      { name: "w", types:["float"], singleConnect:false, role:"out", optionalIdentifier:{idx:4, prop:"vec length"}, data:{code:"$vec$.w"}},

    ],
    properties: [
      {name:"vec length", type:"multitype_combobox", items:["2", "3", "4"], val:"4", linkedPin:"vec"}
    ],
    allowLoop: false,
    width: 80
  },

  {
    name:"float to vec",
    category: "Conversion",
    synonyms: "select, vec, component, swizzle, conversion",
    pins: [
      { name: "x", types:["float"], singleConnect:true, role:"in"},
      { name: "y", types:["float"], singleConnect:true, role:"in"},
      { name: "z", types:["float"], singleConnect:true, role:"in", optionalIdentifier:{idx:3, prop:"vec length"}},
      { name: "w", types:["float"], singleConnect:true, role:"in", optionalIdentifier:{idx:4, prop:"vec length"}},
      { name: "vec", selectableTypes:{"2":"float2", "3":"float3", "4":"float4"}, singleConnect: false, role: "out", data:{code:"float%vec length%($x$, $y$, $z$, $w$)"}},
    ],
    properties: [
      {name:"vec length", type:"multitype_combobox", items:["2", "3", "4"], val:"4", linkedPin:"vec"}
    ],
    allowLoop: false,
    width: 80
  },

  {
    name:"vec to int",
    category: "Conversion",
    synonyms: "select, vec, component, swizzle, conversion",
    pins: [
      { name: "vec", selectableTypes:{"2":"int2", "3":"int3", "4":"int4"}, singleConnect: true, role: "in"},
      { name: "x", types:["int"], singleConnect:false, role:"out", data:{code:"$vec$.x"}},
      { name: "y", types:["int"], singleConnect:false, role:"out", data:{code:"$vec$.y"}},
      { name: "z", types:["int"], singleConnect:false, role:"out", optionalIdentifier:{idx:3, prop:"vec length"}, data:{code:"$vec$.z"}},
      { name: "w", types:["int"], singleConnect:false, role:"out", optionalIdentifier:{idx:4, prop:"vec length"}, data:{code:"$vec$.w"}},

    ],
    properties: [
      {name:"vec length", type:"multitype_combobox", items:["2", "3", "4"], val:"4", linkedPin:"vec"}
    ],
    allowLoop: false,
    width: 80
  },

  {
    name:"int to vec",
    category: "Conversion",
    synonyms: "select, vec, component, swizzle, conversion",
    pins: [
      { name: "x", types:["int"], singleConnect:true, role:"in"},
      { name: "y", types:["int"], singleConnect:true, role:"in"},
      { name: "z", types:["int"], singleConnect:true, role:"in", optionalIdentifier:{idx:3, prop:"vec length"}},
      { name: "w", types:["int"], singleConnect:true, role:"in", optionalIdentifier:{idx:4, prop:"vec length"}},
      { name: "vec", selectableTypes:{"2":"int2", "3":"int3", "4":"int4"}, singleConnect: false, role: "out", data:{code:"int%vec length%($x$, $y$, $z$, $w$)"}},
    ],
    properties: [
      {name:"vec length", type:"multitype_combobox", items:["2", "3", "4"], val:"4", linkedPin:"vec"}
    ],
    allowLoop: false,
    width: 80
  },
  {
    name: "vec to uint",
    category: "Conversion",
    synonyms: "select, vec, component",
    pins: [
      { name: "vec", selectableTypes:{"2":"uint2", "3":"uint3", "4":"uint4"}, singleConnect: true, role: "in"},
      { name: "x", types:["uint"], singleConnect:false, role:"out", data:{code:"$vec$.x"}},
      { name: "y", types:["uint"], singleConnect:false, role:"out", data:{code:"$vec$.y"}},
      { name: "z", types:["uint"], singleConnect:false, role:"out", optionalIdentifier:{idx:3, prop:"vec length"}, data:{code:"$vec$.z"}},
      { name: "w", types:["uint"], singleConnect:false, role:"out", optionalIdentifier:{idx:4, prop:"vec length"}, data:{code:"$vec$.w"}},

    ],
    properties: [
      {name:"vec length", type:"multitype_combobox", items:["2", "3", "4"], val:"4", linkedPin:"vec"}
    ],
    allowLoop: false,
    width: 80
  },
  {
    name: "uint to vec",
    category: "Conversion",
    synonyms: "select, vec, component",
    pins: [
      { name: "x", types:["uint"], singleConnect:true, role:"in"},
      { name: "y", types:["uint"], singleConnect:true, role:"in"},
      { name: "z", types:["uint"], singleConnect:true, role:"in", optionalIdentifier:{idx:3, prop:"vec length"}},
      { name: "w", types:["uint"], singleConnect:true, role:"in", optionalIdentifier:{idx:4, prop:"vec length"}},
      { name: "vec", selectableTypes:{"2":"uint2", "3":"uint3", "4":"uint4"}, singleConnect: false, role: "out", data:{code:"uint%vec length%($x$, $y$, $z$, $w$)"}},
    ],
    properties: [
      {name:"vec length", type:"multitype_combobox", items:["2", "3", "4"], val:"4", linkedPin:"vec"}
    ],
    allowLoop: false,
    width: 80
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
    name:"if",
    category:"Branching",
    synonyms:"compare,branch,max,min,check,test",
    pins:[
      {name:"condition", types:["bool"], singleConnect:true, typeGroup:"group1", role:"in"},
      {name:"true", types:["uint", "int", "int2", "int3", "int4", "float", "float2", "float3", "float4", "material_t"], singleConnect:true, typeGroup:"group2", role:"in"},
      {name:"false", types:["uint", "int", "int2", "int3", "int4", "float", "float2", "float3", "float4", "material_t"], singleConnect:true, typeGroup:"group2", role:"in"},
      {name:"res", types:["uint", "int", "int2", "int3", "int4", "float", "float2", "float3", "float4", "material_t"], singleConnect:false, typeGroup:"group2", role:"out", data:{code:"(($condition$) ? ($true$) : ($false$))"}}
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
      {name:"A", types:["int", "float", "uint"], singleConnect:true, typeGroup:"group1", role:"in"},
      {name:"B", types:["int", "float", "uint"], singleConnect:true, typeGroup:"group1", role:"in"},
      {name:"A==B", types:["int", "float", "float2", "float3", "float4", "material_t"], singleConnect:true, typeGroup:"group2", role:"in"},
      {name:"A!=B", types:["int", "float", "float2", "float3", "float4", "material_t"], singleConnect:true, typeGroup:"group2", role:"in"},
      {name:"result", types:["int", "int2", "int3", "int4", "float", "float2", "float3", "float4", "material_t"], singleConnect:false, typeGroup:"group2", role:"out", data:{code:"(($A$ == $B$) ? ($A==B$) : ($A!=B$))"}}
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
      {name:"A", types:["int", "float", "uint"], singleConnect:true, typeGroup:"group1", role:"in"},
      {name:"B", types:["int", "float", "uint"], singleConnect:true, typeGroup:"group1", role:"in"},
      {name:"A>B", types:["uint", "int", "float", "float2", "float3", "float4", "material_t"], singleConnect:true, typeGroup:"group2", role:"in"},
      {name:"A<=B", types:["uint", "int", "float", "float2", "float3", "float4", "material_t"], singleConnect:true, typeGroup:"group2", role:"in"},
      {name:"res", types:["uint", "int", "float", "float2", "float3", "float4", "material_t"], singleConnect:false, typeGroup:"group2", role:"out", data:{code:"(($A$ > $B$) ? ($A>B$) : ($A<=B$))"}}
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
      {name:"A", types:["int", "float", "uint"], singleConnect:true, typeGroup:"group1", role:"in"},
      {name:"B", types:["int", "float", "uint"], singleConnect:true, typeGroup:"group1", role:"in"},
      {name:"A>=B", types:["uint", "int", "float", "float2", "float3", "float4", "material_t"], singleConnect:true, typeGroup:"group2", role:"in"},
      {name:"A<B", types:["uint", "int", "float", "float2", "float3", "float4", "material_t"], singleConnect:true, typeGroup:"group2", role:"in"},
      {name:"res", types:["uint", "int", "float", "float2", "float3", "float4", "material_t"], singleConnect:false, typeGroup:"group2", role:"out", data:{code:"(($A$ >= $B$) ? ($A>=B$) : ($A<B$))"}}
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
    name:"color gradient map",
    synonyms: "color, gradient, map, curve",
    category:"Curves",
    pins:[
      {name:"x", types:["float"], singleConnect:true, role:"in",  data:{localFunction:genColorGradFunctions()}},
      {name:"rgb", types:["float3"], optionalIdentifier:{idx:1, prop:"useRgb"}, role:"out", singleConnect:false, data:{code:"float3(color_curve_r_%#%($x$), color_curve_g_%#%($x$), color_curve_b_%#%($x$))"}},
      {name:"rgba", types:["float4"], optionalIdentifier:{idx:1, prop:"useAlpha"}, singleConnect:false, role:"out", data:{code:"float4(color_curve_r_%#%($x$), color_curve_g_%#%($x$), color_curve_b_%#%($x$), color_curve_a_%#%($x$))"}},
    ],
    properties:[
      {name:"gradient", type:"gradient_editor", val:"0, 0, 0, 0, 1, 1, 1, 1, 1, 1"},
      {name:"useAlpha", type:"int", minVal:0, maxVal:1, val:"1"},
      {name:"useRgb", type:"int", minVal:0, maxVal:1, val:"0"}
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
    name: "==",
    category: "Boolean",
    synonyms: "bool, ==, equal",
    pins: [
      { name: "A", types: ["bool", "uint", "float", "int", "material_t"], singleConnect: true, role: "in"},
      { name: "B", types: ["bool", "uint", "float", "int", "material_t"], singleConnect: true, role: "in"},
      { name: "A==B", types: ["bool"], singleConnect: false, role: "out", data:{code:"$A$ == $B$"}},
    ],
    properties: [
    ],
    allowLoop: false,
    width: 200
  },
  {
    name: "!=",
    category: "Boolean",
    synonyms: "bool, !=, notequal",
    pins: [
      { name: "A", types: ["bool", "uint", "float", "int", "material_t"], singleConnect: true, role: "in"},
      { name: "B", types: ["bool", "uint", "float", "int", "material_t"], singleConnect: true, role: "in"},
      { name: "A!=B", types: ["bool"], singleConnect: false, role: "out", data:{code:"$A$ != $B$"}},
    ],
    properties: [
    ],
    allowLoop: false,
    width: 200
  },
  {
    name: ">",
    category: "Boolean",
    synonyms: "bool, >",
    pins: [
      { name: "A", types: ["bool", "uint", "float", "int"], singleConnect: true, role: "in"},
      { name: "B", types: ["bool", "uint", "float", "int"], singleConnect: true, role: "in"},
      { name: "A > B", types: ["bool"], singleConnect: false, role: "out", data:{code:"$A$ > $B$"}},
    ],
    properties: [
    ],
    allowLoop: false,
    width: 200
  },
  {
    name: "<",
    category: "Boolean",
    synonyms: "bool, <",
    pins: [
      { name: "A", types: ["bool", "uint", "float", "int"], singleConnect: true, role: "in"},
      { name: "B", types: ["bool", "uint", "float", "int"], singleConnect: true, role: "in"},
      { name: "A < B", types: ["bool"], singleConnect: false, role: "out", data:{code:"$A$ < $B$"}},
    ],
    properties: [
    ],
    allowLoop: false,
    width: 200
  },
  {
    name: ">=",
    category: "Boolean",
    synonyms: "bool, >=",
    pins: [
      { name: "A", types: ["bool", "uint", "float", "int"], singleConnect: true, role: "in"},
      { name: "B", types: ["bool", "uint", "float", "int"], singleConnect: true, role: "in"},
      { name: "A >= B", types: ["bool"], singleConnect: false, role: "out", data:{code:"$A$ >= $B$"}},
    ],
    properties: [
    ],
    allowLoop: false,
    width: 200
  },
  {
    name: "<=",
    category: "Boolean",
    synonyms: "bool, <=",
    pins: [
      { name: "A", types: ["bool", "uint", "float", "int"], singleConnect: true, role: "in"},
      { name: "B", types: ["bool", "uint", "float", "int"], singleConnect: true, role: "in"},
      { name: "A <= B", types: ["bool"], singleConnect: false, role: "out", data:{code:"$A$ <= $B$"}},
    ],
    properties: [
    ],
    allowLoop: false,
    width: 200
  },
  {
    name: "AND",
    category: "Boolean",
    synonyms: "bool, and, &&",
    pins: [
      { name: "A", types: ["bool"], singleConnect: true, role: "in"},
      { name: "B", types: ["bool"], singleConnect: true, role: "in"},
      { name: "A AND B", types: ["bool"], singleConnect: false, role: "out", data:{code:"$A$ && $B$"}},
    ],
    properties: [
    ],
    allowLoop: false,
    width: 200
  },
  {
    name: "OR",
    category: "Boolean",
    synonyms: "bool, or, ||",
    pins: [
      { name: "A", types: ["bool"], singleConnect: true, role: "in"},
      { name: "B", types: ["bool"], singleConnect: true, role: "in"},
      { name: "A OR B", types: ["bool"], singleConnect: false, role: "out", data:{code:"$A$ || $B$"}},
    ],
    properties: [
    ],
    allowLoop: false,
    width: 200
  },
  {
    name: "NOT",
    category: "Boolean",
    synonyms: "bool, not, !",
    pins: [
      { name: "A", types: ["bool"], singleConnect: true, role: "in"},
      { name: "NOT A", types: ["bool"], singleConnect: false, role: "out", data:{code:"!$A$"}},
    ],
    properties: [
    ],
    allowLoop: false,
    width: 200
  },
  {
    name:"get depth above",
    category:"Depth Above",
    synonyms:"get, depth, above",
    pins:[
      {name:"world pos", types:["float3"], singleConnect:true, role:"in"},
      {name:"texture", types:["texture2DArray"], singleConnect:true, role:"in"},
      {name:"vig", caption:"vig", types:["float"], singleConnect:true, role:"in", hidden:true, makeInternalVar:true},
      {name:"depth above", types:["float"], singleConnect:false, role:"out", data:{code:"get_depth_above_impl($world pos$, %precise%, $texture$, $texture$_samplerstate, $vig$)"}},
      {name:"vignette", types:["float"], singleConnect:false, role:"out", data:{code:"$vig$"}}
    ],
    properties:[
      {name:"precise", type:"bool", val:"false"},
    ],
    allowLoop:false,
    width:160
  },
  {
    name:"get depth above clamped",
    category:"Depth Above",
    synonyms:"get, depth, above, blurred",
    pins:[
      {name:"world pos", types:["float3"], singleConnect:true, role:"in"},
      {name:"texture", types:["texture2DArray"], singleConnect:true, role:"in"},
      {name:"min", caption:"clamp min", types:["float"], singleConnect:true, role:"in", data:{def_val:"%clamp min%"}},
      {name:"max", caption:"clamp max", types:["float"], singleConnect:true, role:"in", data:{def_val:"%clamp max%"}},
      {name:"vig", caption:"vig", types:["float"], singleConnect:true, role:"in", hidden:true, makeInternalVar:true},
      {name:"depth above", types:["float"], singleConnect:false, role:"out", data:{code:"get_depth_above_precise_with_clamps($world pos$, $texture$, $texture$_samplerstate, $min$, $max$, $vig$)"}},
      {name:"vignette", types:["float"], singleConnect:false, role:"out", data:{code:"$vig$"}}
    ],
    properties:[
      {name:"clamp min", type:"float", minVal:"-10000", maxVal:"10000", step:"0.01", val:"-10000"},
      {name:"clamp max", type:"float", minVal:"-10000", maxVal:"10000", step:"0.01", val:"10000"},
    ],
    allowLoop:false,
    width:160
  },
  {
    name:"get water heightmap",
    category:"Input",
    synonyms:"water, heightmap",
    pins:[
      {name:"height", types:["float"], singleConnect:true, makeInternalVar:true, hidden:true, role:"in"},
      {name:"worldPos", types:["float3"], singleConnect: true, role:"in", data:{code:"sample_water_heightmap_pages($worldPos$.xz, $height$)"}},
      {name:"water height", types:["float"], singleConnect:false, role:"out", data:{code:"$height$"}}
    ],
    properties:[],
    allowLoop:false,
    width:160
  },
  {
    name:"get SDF occlusion",
    category:"Input",
    synonyms:"get, SDF, occlusion",
    pins:[
      {name:"world pos", types:["float3"], singleConnect:true, role:"in"},
      {name:"normal", types:["float3"], singleConnect:true, role:"in"},
      {name:"range", types:["float"], singleConnect:true, role:"in", data:{def_val:"%range%"}},
      {name:"occlusion", types:["float"], singleConnect:false, role:"out", data:{code:"compute_simple_sdf_occlusion($world pos$, $normal$, $range$)"}}
    ],
    properties:[
      {name:"range", type:"float", minVal:"0", maxVal:"50", step:"0.01", val:"1"},
    ],
    allowLoop:false,
    width:160
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
  {
    name:"get gravity dir",
    category:"Input",
    synonyms:"gravity, get, dir",
    pins:[
      { name: "worldPos", types: ["float3"], singleConnect: true, role: "in" },
      { name: "dummy", types: ["float"], singleConnect: true, makeInternalVar:true, hidden:true, role: "in" },
      { name: "gravity dir", types: ["float3"], singleConnect: false, role: "out", data:
        {code: "get_gravity_dir_impl($worldPos$, grav_zone_grid, grav_zone_grid_samplerstate, grav_grid_offset, grav_grid_extra_offset, $dummy$)"}},
    ],
    properties:[
      {name:"approximate far", type:"bool", val:"true"},
    ],
    isExternal:true,
    allowLoop:false,
    width:220,
  },
]

var GE_defaultExternalsCommon =
[
  // type - int, float, float2, float3, float4
  // template: {type: "float3", name: "camera_pos"},
  {type: "float4", name:"skylight_params"},
  {type: "float4",  name:"zn_zfar"},
  {type: "float4", name:"view_vecLB"},
  {type: "float4", name:"view_vecLT"},
  {type: "float4", name:"view_vecRB"},
  {type: "float4", name:"view_vecRT"},
  {type: "float4", name:"world_view_pos"},
  {type: "float4", name:"world_to_hmap_low"},
  {type: "float4", name:"heightmap_scale"},
  {type: "float4", name:"tex_hmap_inv_sizes"},
  {type: "float4", name:"world_to_depth_ao"},
  {type: "float4", name:"depth_ao_heights"},
  {type:"int", name:"depth_ao_extra_enabled"},
  {type:"float4", name:"world_to_depth_ao_extra"},
  {type:"float4", name:"depth_ao_heights_extra"},
  {type:"float", name:"depth_ao_texture_size"},
  {type:"float", name:"depth_ao_texture_size_inv"},
  {type: "float4", name:"land_detail_mul_offset"},
  {type: "float",  name:"global_time_phase"},
  {type: "texture2D", name:"noise_64_tex_l8"},
  {type: "texture2D", name:"land_heightmap_tex"},
  {type: "texture2D_nosampler", name:"tex_hmap_low"},
  {type: "float", name:"water_level"},
  {type: "texture2D_nosampler", name:"distant_heightmap_tex"},
  {type: "float4",  name:"distant_world_to_hmap_low"},
  {type: "float4",  name:"distant_heightmap_scale"},
  {type: "float4",  name:"distant_heightmap_target_box"},
  {type: "float4", name:"from_sun_direction"},
  {type: "texture2D", name:"ambient_wind_tex"},
  {type: "float4",  name:"ambient_wind_map_scale__offset"},
  {type: "float4",  name:"ambient_wind__speed__current_time__previous_time"},
  {type: "texture2D", name:"biomeIndicesTex"},
  {type: "float4", name:"biome_indices_tex_size"},
  {type: "float4", name:"biom_attributes[32]"},
  {type: "float4", name:"water_heightmap_page_count__det_scale"},
  {type: "float4", name:"water_height_offset_scale__page_padding"},
  {type: "float4", name:"world_to_water_heightmap"},
  {type: "texture2D", name:"water_heightmap_grid"},
  {type: "texture2D_nosampler", name:"water_heightmap_pages"},
  {type: "int4", name:"world_sdf_res"},
  {type: "CBuffer", name:"world_sdf_params"},
  {type: "float4", name:"world_sdf_to_atlas_decode__gradient_offset"},
  {type: "texture3D", name:"world_sdf_clipmap"},

]

var GE_defaultExternals = GE_defaultExternalsCommon.concat(GE_defaultExternalsAdditional);
var GE_nodeDescriptions = GE_nodeDescriptionsCommon.concat(GE_nodeDescriptionsAdditional);

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

      if (pin.makeInternalVar && (pin.role != "in" || !pin.hidden))
        alert("ERROR: pin attribute makeInternalVar can be only used on pins with role 'in', and hidden:'true'! Problematic node and pin: '" + desc.name + "': " + pin.name);

      if (pin.optionalIdentifier)
      {
        if (!pin.optionalIdentifier.idx)
          alert("ERROR: pin with optionalIdentifier is missing optionalIdentifier.idx, pin:" + pin.name);
        if (!pin.optionalIdentifier.prop)
        {
          alert("ERROR: pin with optionalIdentifier is missing optionalIdentifier.prop, pin:" + pin.name);
          var found = false;
          for (var z = 0; z < desc.prop.length; z++)
          {
            if(desc.prop[z] === pin.optionalIdentifier.prop)
              found = true;
          }
          if(!found)
            alert("ERROR: pin '" + pin.name + "' is refefering to a non existant property with optionalIdentifier.prop: " + desc.prop[z].name);
        }
      }
    }

    for (var j = 0; j < desc.properties.length; j++)
    {
      var prop = desc.properties[j];

      if (!prop.name || prop.name == "" || prop.name.indexOf("%") >= 0 || prop.name.indexOf("$") >= 0)
        alert("ERROR: invalid property name '" + prop.name + "' in '" + desc.name + "'");

      if (propNames.indexOf(prop.name) >= 0)
        alert("ERROR: duplicate of property name '" + prop.name + "' in '" + desc.name + "'");

      propNames.push(prop.name);

      if (["combobox", "multitype_combobox", "bool", "int", "float", "float2", "float3", "float4", "string", "color", "gradient_preview",
           "polynom2", "polynom3", "polynom4", "polynom5", "polynom6", "polynom7", "polynom8",
           "linear2", "linear3", "linear4", "linear5", "linear6", "linear7", "linear8",
           "monotonic2", "monotonic3", "monotonic4", "monotonic5", "monotonic6", "monotonic7", "monotonic8",
           "steps_curve", "linear_curve", "monotonic_curve", "polynom_curve", "gradient_editor"
           ].indexOf(prop.type) < 0)
        alert("ERROR: invalid property type = '" + prop.type + "' ('" + prop.name + "' in '" + desc.name + "')");

      if (prop.type == "combobox" || prop.type == "multitype_combobox")
      {
        if (!prop.items || prop.items.length == 0)
          alert("ERROR: missed 'items' for combobox ('" + prop.name + "' in '" + desc.name + "')");
      }
      if (prop.type == "multitype_combobox")
      {
        if (!prop.linkedPin)
        {
          alert("ERROR: property '" + prop.name + "' with type multitype_combobox, is missing attribute: linkedPin");
          continue;
        }
        var found = false;
        for (var z = 0; z < desc.pins.length; z++)
        {
          var lPin = desc.pins[z];
          if (lPin.name == prop.linkedPin)
          {
            found = true;
            if (!lPin.selectableTypes)
            {
              alert("ERROR: pin '" + lPin.name + "' linked by multitype_combobox `" + prop.name + "', is missing attribute: selectableTypes");
            }
            for(var it = 0; it < prop.items.length; it++)
            {
              if(lPin.selectableTypes[prop.items[it]] == null)
                alert("ERROR: in multitype_combobox '" + prop.name + "' item: " + prop.items[it] + " is missing the selectable type, which are defined for: [" + Object.keys(lPin.selectableTypes) +"]");
            }
          }
        }
        if (!found)
        {
          alert("ERROR: multitype_combobox '" + prop.name + "' referenced a non existant pin on the element: " + prop.linkedPin);
        }
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
      if (p.optional)
      {
        replaceFrom = ", " + replaceFrom;
      }
      var replaceTo = "";

      var type = p.connectToType;
      if (type && !p.optional)
      {
        var toPin = graph.elems[p.connectElem].pins[p.connectPin];
        var toVar = toPin.customVarPoolName ? toPin.customVarPoolName :
           (toPin.customVarName ? "" + toPin.customVarName : "_v_" + p.connectElem + "_" + p.connectPin);
        replaceTo = convertType(p.connectFromType, p.connectToType, toVar);
      }
      else if (!p.optional)
      {
        if (p.customPinInput)
          replaceTo = "" + p.customPinInput;
        else if (p.data && p.data.def_val)
          replaceTo = p.data.def_val;
        else if (p.makeInternalVar)
          replaceTo = "_v_" + elem.id + "_" + i;
        else
        {
          type = getPinType(elem, p);

          replaceTo = (p.data && p.data.def_1) ? GE_defaultValuesOne[type] : GE_defaultValuesZero[type];
          if (!replaceTo)
            replaceTo = "0"; // error
        }
      }

      if (p.group && !p.optional)
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
      if (code.indexOf("%" + v.name + "_r") >= 0)
      {
        if(v.value.length === 0)
          break;

        var componentMap = {1:"r", 2:"g", 3:"b", 4:"a"};
        var valueArray = v.value.split(", ");
        for (var j = 0; j < valueArray.length; j++)
          valueArray[j] = Number(valueArray[j].trim())

        for (var j = 1; j < 5; j++)
        {
          var complexName = v.name + "_" + componentMap[j];
          var replaceValFrom = "%" + complexName + "%";
          var replaceValTo = "";

          // For every point
          for (var z = 0; z < valueArray.length / 5 - 1; z++)
          {
            var x1 = valueArray[z * 5];
            var x2 = valueArray[(z + 1) * 5];
            var y1 = valueArray[z * 5 + j];
            var y2 = valueArray[(z + 1) * 5 + j];

            // First value should not have a comma before
            replaceValTo += z == 0 ? "" : ", ";
            replaceValTo += String(x1);
            replaceValTo += ", " + String(y1);
            // Slope
            replaceValTo += ", " + String((y2 - y1) / (x2 - x1 + 1e-9));
          }
          code = code.split(replaceValFrom).join(replaceValTo);

          var replaceCountOfFrom = "%countof(" + complexName + ")%";
          var replaceCountOfTo = count_numbers_in_string(replaceValTo);
          code = code.split(replaceCountOfFrom).join(replaceCountOfTo);
        }
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
    "float4x4":[],
    "texture2D": [],
    "texture3D": [],
    "texture2D_nosampler": [],
    "texture3D_nosampler": [],
    "texture2DArray": [],
    "texture2D_shdArray": [],
    "Buffer": [],
    "CBuffer":[],
  };

  var isTextureType = function(type) {
    return type === "texture2D" || type === "texture3D" || type === "texture2DArray" || type === "texture2D_shdArray" || type === "texture2D_nosampler" || type === "texture3D_nosampler";
  }

  for (var i = 0; i < GE_defaultExternals.length; i++)
  {
    var d = GE_defaultExternals[i];
    inputs[d.type].push({
      elem: -1,
      pin: -1,
      externalName: d.name
    });

    if (typeof d.buffer_struct != "undefined")
      inputs[d.type][inputs[d.type].length - 1].customBufferStruct = d.buffer_struct;

    if (typeof d.texture_type != "undefined")
      inputs[d.type][inputs[d.type].length - 1].texture_type = d.texture_type;

    if (isTextureType(d.type))
      inputs[d.type][inputs[d.type].length - 1].customSamplerName = d.name;

  }


  var outputs = [];
  var gravZonesUsed = false;
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
              && isTextureType(pinType)
              )
            {
              p.customVarName = name;
              foundInExternals = true;
              break;
            }

          if (!foundInExternals)
          {
            if (isTextureType(pinType))
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
        if ((p.name === "gravity dir" || p.name ===  "gravity grid") && !gravZonesUsed)
        {
          gravZonesUsed = true;

          inputs["CBuffer"].push({
            elem:i,
            pin:j,
            externalName:"gravity_zones",
          });

          inputs["float4"].push({
            elem:i,
            pin:j,
            externalName:"grav_grid_offset"
          });
          inputs["float4"].push({
            elem:i,
            pin:j,
            externalName:"grav_grid_extra_offset"
          });
          inputs["texture3D"].push({
            elem:i,
            pin:j,
            externalName:"grav_zone_grid",
            customSamplerName:"grav_zone_grid",
            texture_type:"float4"
          });
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

  var inputTypes = ["int", "float", "int4", "float2", "float3", "float4", "float4x4",
    "texture2D", "texture3D", "texture2DArray", "texture2D_shdArray", "texture2D_nosampler", "texture3D_nosampler", "Buffer", "CBuffer"];
  var cbufferTypes = ["int", "float", "int4", "float2", "float3", "float4", "float4x4"];

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

  // fillBlkExtraCBuffer
  var cbufferOffset = 1;
  for (var i = 0; i < inputs["CBuffer"].length; i++)
  {
    var d = inputs["CBuffer"][i];
    blk += indent + "cbuffer " + d.externalName + ":register(b" + cbufferOffset++ + ")" + newLine;
    blk += indent + "{" + newLine;
    blk += indent + indent + "CBUFF_STRUCTURE_" + d.externalName + newLine
    blk += indent + "};" + newLine;
  }

  var fillBlkTexture = function(typeStr, codeStr, registerOffset, samplerType)
  {
    for (var i = 0; i < inputs[typeStr].length; i++)
    {
      var d = inputs[typeStr][i];
      var registerId = registerOffset + i;
      var texTypeStr = "";
      if (typeof d.texture_type != "undefined")
        texTypeStr = "<" + d.texture_type + ">"
      if (d.customSamplerName)
      {
        blk += indent + codeStr + texTypeStr + " " + d.customSamplerName + ":register(t" + registerId + ");" + newLine;
        if (samplerType == 1)
          blk += indent + "SamplerState " + d.customSamplerName + "_samplerstate:register(s" + registerId + ");" + newLine;
        else if (samplerType == 2)
          blk += indent + "SamplerComparisonState " + d.customSamplerName + "_cmpSampler:register(s" + registerId + ");" + newLine;
      }
      else
      {
        blk += indent + codeStr + texTypeStr + " sampler" + i + ":register(t" + registerId + ");" + newLine;
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
  registerOffset += fillBlkTexture("texture2DArray", "Texture2DArray", registerOffset, 1);
  registerOffset += fillBlkTexture("texture2D_shdArray", "Texture2DArray", registerOffset, 2);
  registerOffset += fillBlkTexture("texture2D_nosampler", "Texture2D", registerOffset, 0);
  registerOffset += fillBlkTexture("texture3D_nosampler", "Texture3D", registerOffset, 0);

  for (var i = 0; i < inputs["Buffer"].length; i++)
  {
    var d = inputs["Buffer"][i];
    var bufferStructure = d.customBufferStruct ? d.customBufferStruct : "float4";
    blk += indent + "StructuredBuffer<" + bufferStructure + "> " + d.externalName + ":register(t" + (registerOffset + i) + ");" + newLine;
  }


  blk += newLine + '"""' + newLine + newLine;


  var processed = new Array(graph.elemCount);
  for (var i = 0; i < processed.length; i++)
    processed[i] = false;

  var earlyExitNode = -1;
  if (typeof GE_volfog_early_exit_node_name != "undefined")
  {
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
          else if (p.makeInternalVar)
          {
            var varName = "_v_" + e.id + "_" + j;
            hlsl += indent + pinTypeToCodeType(p.types[0]) + " " + varName + " = " + GE_defaultValuesZero[p.types[0]] + ";" + newLine;
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

  blk = "/*SHADER_BLK_START*/" + blk + "/*SHADER_BLK_END*/";

  graph["code"] = blk;
  graph["hashOfCode"] = "/*HASH_OF_CODE_START*/" + hashCode31(blk) + "/*HASH_OF_CODE_END*/";
}

function GE_setExternalNames(nodeName, items)
{
  if (items[0] !== "")
    items = [""].concat(items);

  if (nodeName === "external texture")
  {
    var extraItems = [];
    for (var i = 0; i < GE_defaultExternals.length; i++)
    {
      var ext = GE_defaultExternals[i];
      if (ext.type === "texture2D")
        extraItems.push(ext.name);
    }
    items = items.concat(extraItems);
  }

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
