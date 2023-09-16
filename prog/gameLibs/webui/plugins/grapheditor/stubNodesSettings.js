"use strict";


var GE_typeColors =
  {
    "bool": "#777",
    "int": "#18f",
    "float": "#0a0",
    "float2": "#ff0",
    "float3": "#0ff",
    "float4": "#f0f",
    "texture2D": "#f81",
    "particles": "#85f",
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
  {
    // type     user data
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
  {
    // type     user data
    "int":    "1",
    "float":  "1.0",
    "float2": "float2(1.0, 1.0)",
    "float3": "float3(1.0, 1.0, 1.0)",
    "float4": "float4(1.0, 1.0, 1.0, 1.0)",
    "polynom4":   "float4(1.0, 0.0, 0.0, 0.0)",
    "linear4":    "linear4_one",
    "monotonic4": "monotonic4_one",
  };


var GE_implicitGroupConversionOrder =
  ["int", "float", "float2", "float3", "float4"];


var GE_nodeDescriptions =
  [
    {
      name:"node int",
      synonyms:"dummy,stub",
      category:"Nodes",
      pins:[
        {name:"A", types:["int"], singleConnect:true, role:"in", data:{}},
        {name:"B", types:["int"], singleConnect:false, role:"out", data:{}}
      ],
      properties:[
        {name:"value", type:"int", minVal:"-65535", maxVal:"65535", step:"1", val:"1"}
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"node float",
      synonyms:"dummy,stub,1",
      category:"Nodes",
      pins:[
        {name:"A", main:true, types:["float"], singleConnect:true, role:"in", data:{}},
        {name:"bool", types:["bool"], singleConnect:false, role:"out", data:{}},
        {name:"int", types:["int"], singleConnect:false, role:"out", data:{}},
        {name:"float", types:["float"], singleConnect:false, role:"out", data:{}},
        {name:"float2", types:["float2"], singleConnect:false, role:"out", data:{}},
        {name:"float3", types:["float3"], singleConnect:false, role:"out", data:{}},
        {name:"float4", types:["float4"], singleConnect:false, role:"out", data:{}},
        {name:"texture2D", types:["texture2D"], singleConnect:false, role:"out", data:{}},
        {name:"particles", types:["particles"], singleConnect:false, role:"out", data:{}},
      ],
      properties:[
        {name:"value", type:"float", minVal:"-65535", maxVal:"65535", step:"1", val:"1"}
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"node float2",
      synonyms:"dummy,stub,2",
      category:"Nodes",
      pins:[
        {name:"A", types:["float2"], singleConnect:true, role:"in", data:{}},
        {name:"B", types:["float2"], singleConnect:false, role:"out", data:{}}
      ],
      properties:[
        {name:"value", type:"int", minVal:"-65535", maxVal:"65535", step:"1", val:"1"}
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"+",
      category:"Math",
      synonyms:"add,summ,increase,offset,plus",
      pins:[
        {name:"A", types:["int", "float", "float2", "float3", "float4"], main:true, singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"B", types:["int", "float", "float2", "float3", "float4"], singleConnect:true, typeGroup:"group1", role:"in"},
        {name:"A+B", types:["int", "float", "float2", "float3", "float4"], singleConnect:false, typeGroup:"group1", role:"out", data:{code:"($A$ + $B$)"}}
      ],
      properties:[
      ],
      allowLoop:false,
      width:120
    },


    {
      name:"input int",
      synonyms:"1",
      category:"Input",
      pins:[
        {name:"A", caption:"%external name% (%value%)", types:["int"], singleConnect:true, role:"out", data:{}},
      ],
      properties:[
        {name:"value", type:"int", minVal:"-65535", maxVal:"65535", step:"1", val:"1"},
        {name:"external name", type:"string", val:""},
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"input bool",
      synonyms:"true,false",
      category:"Input",
      pins:[
        {name:"A", caption:"%value%", types:["bool"], singleConnect:true, role:"out", data:{}},
      ],
      properties:[
        {name:"value", type:"bool", val:"false"},
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"output int",
      synonyms:"1",
      category:"Output",
      pins:[
        {name:"A", caption:"%external name%", types:["int"], singleConnect:true, role:"in", data:{}},
      ],
      properties:[
        {name:"external name", type:"string", val:""},
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"dynamic pins",
      synonyms:"dummy,stub",
      category:"Nodes",
      pins:[
        {name:"A", uid:"413444-12331", types:["int"], singleConnect:true, role:"in", data:{}},
        {name:"B", uid:"413444-12332", types:["int"], singleConnect:false, role:"out", data:{}},
        {name:"C", uid:"413444-12333", types:["int"], singleConnect:false, role:"out", data:{}},
        {name:"D", uid:"413444-12334", types:["int"], singleConnect:false, role:"out", data:{}}
      ],
      properties:[
        {name:"value", type:"int", minVal:"-65535", maxVal:"65535", step:"1", val:"1"}
      ],
      allowLoop:false,
      width:120
    },

    {
      name:"create new shader",
      synonyms:"",
      category:"Creation",
      pins:[],
      properties:[],
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
      name:"curve",
      synonyms:"",
      category:"Curves",
      pins:[
        {name:"out", types:["texture2D"], singleConnect:false, role:"out", data:{code:"%curve%"}},
      ],
      properties:[
        {name:"curve", style:"gray", type:"monotonic_curve", val:"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  /*0.0,0.0, 0.333, 0.0, 0.666, 0.0, 1.0, 0.0*/", background:[]},
        {name:"preview", type:"gradient_preview", background:["curve", "curve", "curve"]},
      ],
      allowLoop:false,
      width:120
    },


  ];


function GE_verifyDescriptions()
{
}
GE_verifyDescriptions();

var GE_beforeStringifyGraph = function(graph)
{
}
