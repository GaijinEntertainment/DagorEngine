"use strict";

var GE_volfog_early_exit_node_name;


var gbuffer_type_dict =
{
  "emission_color":"float4",
  "albedo":"float3",
  "motion":"float3",
  "normal":"float3",
  "smoothness":"float",
  "metalness":"float",
  "translucency":"float",
  "emission_strength":"float",
  "reflectance":"float",
  "sheen":"float",
  "ao":"float",
  "shadow":"float",
  "reactive":"float",
  "material":"material_t",
  "dynamic":"bool",
  "autoMotionVecs":"bool",
  "sss_profile":"uint",
  "isLandscape":"bool",
  "isHeroCockpit":"bool",
}

var gbuffer_type_no_uint_dict =
{
  "emission_color":"float4",
  "albedo":"float3",
  "motion":"float3",
  "normal":"float3",
  "smoothness":"float",
  "metalness":"float",
  "translucency":"float",
  "emission_strength":"float",
  "reflectance":"float",
  "sheen":"float",
  "ao":"float",
  "shadow":"float",
  "reactive":"float",
}

var gbufferSSSProfiles = ["SSS_PROFILE_EYES", "SSS_PROFILE_CLOTH", "SSS_PROFILE_NEUTRAL_TRANSLUCENT"];

var materialDict = ["SHADING_NORMAL", "SHADING_SUBSURFACE", "SHADING_FOLIAGE", "SHADING_SELFILLUM"];

var GE_nodeDescriptionsAdditional =
[
  {
    name:"gbuffer output",
    category:"Output",
    synonyms:"gbuffer, output",
    pins:[
      {name:"gbuffer", caption:"Gbuffer", types:["NBSGbuffer"], singleConnect:true, role:"in", data:{code:"writeNBSGbuffer($gbuffer$, dtId)"}},
    ],
    properties:[],
    allowLoop:false,
    width:200
  },
  {
    name:"gbuffer input",
    category:"Input",
    synonyms:"gbuffer, input",
    pins:[
      {name:"gbuffer", caption:"Gbuffer", types:["NBSGbuffer"], singleConnect:false, role:"out", data:{code:"nbsGbuffer"}}
    ],
    properties:[],
    allowLoop:false,
    width:200
  },
  {
    name:"read gbuffer property",
    category:"Gbuffer",
    synonyms:"gbuffer, read",
    pins:[
      {name:"input", caption:"gbuffer", types:["NBSGbuffer"], singleConnect:true, role:"in"},
      {name:"value", caption:"%name%", selectableTypes:gbuffer_type_dict, singleConnect:false, role:"out", data:{code:"$input$.unpackedGbuffer.%name%"}}
    ],
    properties:[
      {name:"name", type:"multitype_combobox", items:Object.keys(gbuffer_type_dict), val:Object.keys(gbuffer_type_dict)[0], linkedPin:"value"}
    ],
    allowLoop:false,
    width:200
  },
  {
    name:"read layer property",
    category:"Gbuffer",
    synonyms:"gbuffer, read, layer",
    pins:[
      {name:"input", caption:"gbuffer", types:["Layer_t"], singleConnect:true, role:"in"},
      {name:"value", caption:"%name%", selectableTypes:gbuffer_type_dict, singleConnect:false, role:"out", data:{code:"$input$.%name%"}}
    ],
    properties:[
      {name:"name", type:"multitype_combobox", items:Object.keys(gbuffer_type_dict), val:Object.keys(gbuffer_type_dict)[0], linkedPin:"value"}
    ],
    allowLoop:false,
    width:200
  },
  {
    name:"metallic layer node",
    category:"Gbuffer",
    synonyms:"gbuffer, layer, metallic",
    pins:[
      {name:"albedo", caption:"albedo", types:["float3"], singleConnect:true, role:"in", data:{def_val:"%albedo%"}},
      {name:"normal", caption:"normal", types:["float3"], singleConnect:true, role:"in", data:{def_val:"%normal%"}},
      {name:"motion", caption:"motion", types:["float3"], singleConnect:true, role:"in", data:{def_val:"%motion%"}},
      {name:"reactive", caption:"reactive", types:["float"], singleConnect:true, role:"in", data:{def_val:"%reactive%"}},
      {name:"smoothness", caption:"smoothness", types:["float"], singleConnect:true, role:"in", data:{def_val:"%smoothness%"}},
      {name:"metalness", caption:"metalness", types:["float"], singleConnect:true, role:"in", data:{def_val:"%metalness%"}},
      {name:"reflectance", caption:"reflectance", types:["float"], singleConnect:true, role:"in", data:{def_val:"%reflectance%"}},
      {name:"ao", caption:"ao", types:["float"], singleConnect:true, role:"in", data:{def_val:"%ao%"}},
      {name:"shadow", caption:"shadow", types:["float"], singleConnect:true, role:"in", data:{def_val:"%shadow%"}},
      {name:"dynamic", caption:"dynamic", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%dynamic%"}},
      {name:"autoMotionVecs", caption:"autoMotionVecs", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%autoMotionVecs%"}},
      {name:"isLandscape", caption:"isLandscape", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%isLandscape%"}},
      {name:"isHeroCockpit", caption:"isHeroCockpit", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%isHeroCockpit%"}},
      {name:"layer", caption:"Layer", types:["Layer_t"], singleConnect:false, role:"out", data:{code:
        "createMetallicLayer(half3($albedo$), float3($normal$), half3($motion$), $smoothness$, $metalness$, $reflectance$, $ao$, $shadow$, $reactive$,"+
        "$dynamic$, $autoMotionVecs$, $isLandscape$, $isHeroCockpit$)"}},
    ],
    properties:[
      {name:"albedo", type:"float3", minVal:"0", maxVal:"1", step:"0.01", val:"0.5, 0.5, 0.5"},
      {name:"normal", type:"float3", minVal:"0", maxVal:"1", step:"0.01", val:"0.0, 1.0, 0.0"},
      {name:"motion", type:"float3", minVal:"0", maxVal:"1", step:"0.01", val:"0.0, 0.0, 0.0"},
      {name:"reactive", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0"},
      {name:"smoothness", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"},
      {name:"metalness", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"},
      {name:"reflectance", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"},
      {name:"ao", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"1"},
      {name:"shadow", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"1"},
      {name:"dynamic", type:"bool", val:"false"},
      {name:"autoMotionVecs", type:"bool", val:"true"},
      {name:"isLandscape", type:"bool", val:"false"},
      {name:"isHeroCockpit", type:"bool", val:"false"},
    ],
    allowLoop:false,
    width:200
  },
  {
    name:"foliage layer node",
    category:"Gbuffer",
    synonyms:"gbuffer, layer, foliage",
    pins:[
      {name:"albedo", caption:"albedo", types:["float3"], singleConnect:true, role:"in", data:{def_val:"%albedo%"}},
      {name:"normal", caption:"normal", types:["float3"], singleConnect:true, role:"in", data:{def_val:"%normal%"}},
      {name:"motion", caption:"motion", types:["float3"], singleConnect:true, role:"in", data:{def_val:"%motion%"}},
      {name:"reactive", caption:"reactive", types:["float"], singleConnect:true, role:"in", data:{def_val:"%reactive%"}},
      {name:"smoothness", caption:"smoothness", types:["float"], singleConnect:true, role:"in", data:{def_val:"%smoothness%"}},
      {name:"translucency", caption:"translucency", types:["float"], singleConnect:true, role:"in", data:{def_val:"%translucency%"}},
      {name:"reflectance", caption:"reflectance", types:["float"], singleConnect:true, role:"in", data:{def_val:"%reflectance%"}},
      {name:"ao", caption:"ao", types:["float"], singleConnect:true, role:"in", data:{def_val:"%ao%"}},
      {name:"shadow", caption:"shadow", types:["float"], singleConnect:true, role:"in", data:{def_val:"%shadow%"}},
      {name:"dynamic", caption:"dynamic", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%dynamic%"}},
      {name:"autoMotionVecs", caption:"autoMotionVecs", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%autoMotionVecs%"}},
      {name:"isLandscape", caption:"isLandscape", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%isLandscape%"}},
      {name:"isHeroCockpit", caption:"isHeroCockpit", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%isHeroCockpit%"}},
      {name:"layer", caption:"Layer", types:["Layer_t"], singleConnect:false, role:"out", data:{code:
        "createFoliageLayer(half3($albedo$), float3($normal$), half3($motion$), $smoothness$, $translucency$, $reflectance$, $ao$, $shadow$, $reactive$,"+
        "$dynamic$, $autoMotionVecs$, $isLandscape$, $isHeroCockpit$)"}},
    ],
    properties:[
      {name:"albedo", type:"float3", minVal:"0", maxVal:"1", step:"0.01", val:"0.5, 0.5, 0.5"},
      {name:"normal", type:"float3", minVal:"0", maxVal:"1", step:"0.01", val:"0.0, 1.0, 0.0"},
      {name:"motion", type:"float3", minVal:"0", maxVal:"1", step:"0.01", val:"0.0, 0.0, 0.0"},
      {name:"reactive", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0"},
      {name:"smoothness", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"},
      {name:"translucency", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"},
      {name:"reflectance", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"},
      {name:"ao", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"1"},
      {name:"shadow", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"1"},
      {name:"dynamic", type:"bool", val:"false"},
      {name:"autoMotionVecs", type:"bool", val:"true"},
      {name:"isLandscape", type:"bool", val:"false"},
      {name:"isHeroCockpit", type:"bool", val:"false"},
    ],
    allowLoop:false,
    width:200
  },
  {
    name:"subsurface layer node",
    category:"Gbuffer",
    synonyms:"gbuffer, layer, subsurface",
    pins:[
      {name:"albedo", caption:"albedo", types:["float3"], singleConnect:true, role:"in", data:{def_val:"%albedo%"}},
      {name:"normal", caption:"normal", types:["float3"], singleConnect:true, role:"in", data:{def_val:"%normal%"}},
      {name:"motion", caption:"motion", types:["float3"], singleConnect:true, role:"in", data:{def_val:"%motion%"}},
      {name:"reactive", caption:"reactive", types:["float"], singleConnect:true, role:"in", data:{def_val:"%reactive%"}},
      {name:"smoothness", caption:"smoothness", types:["float"], singleConnect:true, role:"in", data:{def_val:"%smoothness%"}},
      {name:"translucency", caption:"translucency", types:["float"], singleConnect:true, role:"in", data:{def_val:"%translucency%"}},
      {name:"reflectance", caption:"reflectance", types:["float"], singleConnect:true, role:"in", data:{def_val:"%reflectance%"}},
      {name:"ao", caption:"ao", types:["float"], singleConnect:true, role:"in", data:{def_val:"%ao%"}},
      {name:"sheen", caption:"sheen", types:["float"], singleConnect:true, role:"in", data:{def_val:"%sheen%"}},
      {name:"sss_profile", caption:"sss_profile", types:["uint"], singleConnect:true, role:"in", data:{def_val:"%sss_profile%"}},
      {name:"dynamic", caption:"dynamic", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%dynamic%"}},
      {name:"autoMotionVecs", caption:"autoMotionVecs", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%autoMotionVecs%"}},
      {name:"isLandscape", caption:"isLandscape", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%isLandscape%"}},
      {name:"isHeroCockpit", caption:"isHeroCockpit", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%isHeroCockpit%"}},
      {name:"layer", caption:"Layer", types:["Layer_t"], singleConnect:false, role:"out", data:{code:
        "createSubsurfaceLayer(half3($albedo$), float3($normal$), half3($motion$), $smoothness$, $translucency$, $reflectance$, $sheen$, $ao$, $reactive$,"+
        "$dynamic$, $autoMotionVecs$, $sss_profile$, $isLandscape$, $isHeroCockpit$)"}},
    ],
    properties:[
      {name:"albedo", type:"float3", minVal:"0", maxVal:"1", step:"0.01", val:"0.5, 0.5, 0.5"},
      {name:"normal", type:"float3", minVal:"0", maxVal:"1", step:"0.01", val:"0.0, 1.0, 0.0"},
      {name:"motion", type:"float3", minVal:"0", maxVal:"1", step:"0.01", val:"0.0, 0.0, 0.0"},
      {name:"reactive", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0"},
      {name:"smoothness", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"},
      {name:"translucency", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"},
      {name:"reflectance", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"},
      {name:"ao", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"1"},
      {name:"sheen", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"},
      {name:"sss_profile", type:"combobox", items:gbufferSSSProfiles, val:gbufferSSSProfiles[0]},
      {name:"dynamic", type:"bool", val:"false"},
      {name:"autoMotionVecs", type:"bool", val:"true"},
      {name:"isLandscape", type:"bool", val:"false"},
      {name:"isHeroCockpit", type:"bool", val:"false"},
    ],
    allowLoop:false,
    width:200
  },
  {
    name:"emissive layer node",
    category:"Gbuffer",
    synonyms:"gbuffer, layer, emissive",
    pins:[
      {name:"emission_color", caption:"emission_color", types:["float4"], singleConnect:true, role:"in", data:{def_val:"%emission_color%"}},
      {name:"albedo", caption:"albedo", types:["float3"], singleConnect:true, role:"in", data:{def_val:"%albedo%"}},
      {name:"normal", caption:"normal", types:["float3"], singleConnect:true, role:"in", data:{def_val:"%normal%"}},
      {name:"motion", caption:"motion", types:["float3"], singleConnect:true, role:"in", data:{def_val:"%motion%"}},
      {name:"reactive", caption:"reactive", types:["float"], singleConnect:true, role:"in", data:{def_val:"%reactive%"}},
      {name:"smoothness", caption:"smoothness", types:["float"], singleConnect:true, role:"in", data:{def_val:"%smoothness%"}},
      {name:"emission_strength", caption:"emission_strength", types:["float"], singleConnect:true, role:"in", data:{def_val:"%emission_strength%"}},
      {name:"reflectance", caption:"reflectance", types:["float"], singleConnect:true, role:"in", data:{def_val:"%reflectance%"}},
      {name:"shadow", caption:"shadow", types:["float"], singleConnect:true, role:"in", data:{def_val:"%shadow%"}},
      {name:"dynamic", caption:"dynamic", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%dynamic%"}},
      {name:"autoMotionVecs", caption:"autoMotionVecs", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%autoMotionVecs%"}},
      {name:"isLandscape", caption:"isLandscape", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%isLandscape%"}},
      {name:"isHeroCockpit", caption:"isHeroCockpit", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%isHeroCockpit%"}},
      {name:"layer", caption:"Layer", types:["Layer_t"], singleConnect:false, role:"out", data:{code:
        "createEmissiveLayer(half4($emission_color$), half3($albedo$), float3($normal$), half3($motion$), $smoothness$, $emission_strength$, $reflectance$, $shadow$, $reactive$,"+
        "$dynamic$, $autoMotionVecs$, $isLandscape$, $isHeroCockpit$)"}},
    ],
    properties:[
      {name:"emission_color", type:"float4", minVal:"0", maxVal:"1", step:"0.01", val:"0.5, 0.5, 0.5, 0.5"},
      {name:"albedo", type:"float3", minVal:"0", maxVal:"1", step:"0.01", val:"0.5, 0.5, 0.5"},
      {name:"normal", type:"float3", minVal:"0", maxVal:"1", step:"0.01", val:"0.0, 1.0, 0.0"},
      {name:"motion", type:"float3", minVal:"0", maxVal:"1", step:"0.01", val:"0.0, 0.0, 0.0"},
      {name:"reactive", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0"},
      {name:"smoothness", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"},
      {name:"emission_strength", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"},
      {name:"reflectance", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"},
      {name:"shadow", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"1"},
      {name:"dynamic", type:"bool", val:"false"},
      {name:"autoMotionVecs", type:"bool", val:"true"},
      {name:"isLandscape", type:"bool", val:"false"},
      {name:"isHeroCockpit", type:"bool", val:"false"},
    ],
    allowLoop:false,
    width:200
  },
  {
    name:"gbuffer mix mask",
    category:"Gbuffer",
    synonyms:"gbuffer, mix, mask",
    pins:[
      {name:"albedo", caption:"albedo", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%albedo%"}},
      {name:"normal", caption:"normal", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%normal%"}},
      {name:"motion", caption:"motion", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%motion%"}},
      {name:"reactive", caption:"reactive", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%reactive%"}},
      {name:"smoothness", caption:"smoothness", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%smoothness%"}},
      {name:"reflectance", caption:"reflectance", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%reflectance%"}},
      {name:"shadow", caption:"shadow", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%shadow%"}},
      {name:"ao", caption:"ao", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%ao%"}},
      {name:"emission_color", caption:"emission_color", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%emission_color%"}},
      {name:"metalness", caption:"metalness", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%metalness%"}},
      {name:"translucency", caption:"translucency", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%translucency%"}},
      {name:"emission_strength", caption:"emission_strength", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%emission_strength%"}},
      {name:"sss_profile", caption:"sss_profile", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%sss_profile%"}},
      {name:"sheen", caption:"sheen", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%sheen%"}},
      {name:"dynamic", caption:"dynamic", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%dynamic%"}},
      {name:"autoMotionVecs", caption:"autoMotionVecs", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%autoMotionVecs%"}},
      {name:"isLandscape", caption:"isLandscape", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%isLandscape%"}},
      {name:"isHeroCockpit", caption:"isHeroCockpit", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%isHeroCockpit%"}},
      {name:"material", caption:"material", types:["bool"], singleConnect:true, role:"in", data:{def_val:"%material%"}},
      {name:"mask", caption:"mask", types:["mask_t"],singleConnect:false, role:"out", data:{code: "createLayerMask($albedo$, $normal$, $motion$, $reactive$, $smoothness$, $reflectance$,"+
       "$shadow$, $ao$, $emission_color$, $metalness$, $translucency$, $emission_strength$, $sss_profile$, $sheen$, $dynamic$, $autoMotionVecs$, $isLandscape$, $isHeroCockpit$, $material$)"}}
    ],
    properties:[
      {name:"albedo", type:"bool", val:"true"},
      {name:"normal", type:"bool", val:"true"},
      {name:"motion", type:"bool", val:"false"},
      {name:"reactive", type:"bool", val:"false"},
      {name:"smoothness", type:"bool", val:"true"},
      {name:"reflectance", type:"bool", val:"true"},
      {name:"shadow", type:"bool", val:"false"},
      {name:"ao", type:"bool", val:"false"},
      {name:"emission_color", type:"bool", val:"false"},
      {name:"metalness", type:"bool", val:"false"},
      {name:"translucency", type:"bool", val:"false"},
      {name:"emission_strength", type:"bool", val:"false"},
      {name:"sss_profile", type:"bool", val:"false"},
      {name:"sheen", type:"bool", val:"false"},
      {name:"dynamic", type:"bool", val:"false"},
      {name:"autoMotionVecs", type:"bool", val:"false"},
      {name:"isLandscape", type:"bool", val:"false"},
      {name:"isHeroCockpit", type:"bool", val:"false"},
      {name:"material", type:"bool", val:"true"},
    ],
    allowLoop:false,
    width:200
  },
  {
    name:"overwrite gbuffer property",
    category:"Gbuffer",
    synonyms:"gbuffer, overwrite, mix",
    pins:[
      {name:"gbuffer", caption:"in-gbuffer", types:["NBSGbuffer"], singleConnect:true, role:"in"},
      {name:"value", caption:"%name%", selectableTypes:gbuffer_type_dict, singleConnect:true, role:"in", data:{code:"overwrite_%name%($gbuffer$, $value$)"}},
      {name:"output", caption:"out-gbuffer", types:["NBSGbuffer"], singleConnect:false, role:"out", data:{code:"$gbuffer$"}}
    ],
    properties:[
      {name:"name", type:"multitype_combobox", items:Object.keys(gbuffer_type_dict), val:Object.keys(gbuffer_type_dict)[0], linkedPin:"value"}
    ],
    allowLoop:false,
    width:200
  },
  {
    name:"lerp gbuffer property",
    category:"Gbuffer",
    synonyms:"gbuffer, lerp, mix",
    pins:[
      {name:"gbuffer", caption:"in-gbuffer", types:["NBSGbuffer"], singleConnect:true, role:"in"},
      {name:"value", caption:"%name%", selectableTypes:gbuffer_type_dict, singleConnect:true, role:"in"},
      {name:"weight", caption:"weight", types:["float"], singleConnect:true, role:"in", data:{def_val:"%weight%", code:"lerp_%name%($gbuffer$, $value$, $weight$)"}},
      {name:"output", caption:"out-gbuffer", types:["NBSGbuffer"], singleConnect:false, role:"out", data:{code:"$gbuffer$"}}
    ],
    properties:[
      {name:"name", type:"multitype_combobox", items:Object.keys(gbuffer_type_no_uint_dict), val:Object.keys(gbuffer_type_no_uint_dict)[0], linkedPin:"value"},
      {name:"weight", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"},
    ],
    allowLoop:false,
    width:200
  },
  {
    name:"overwrite gbuffer with layer",
    category:"Gbuffer",
    synonyms:"gbuffer, overwrite, mix, layer",
    pins:[
      {name:"gbuffer", caption:"in-gbuffer", types:["NBSGbuffer"], singleConnect:true, role:"in"},
      {name:"layer", caption:"layer", types:["Layer_t"], singleConnect:true, role:"in"},
      {name:"mask", caption:"mask", types:["mask_t"], singleConnect:true, role:"in"},
      {name:"overwrite", caption:"shouldOverwrite", types:["bool"], singleConnect:true, role:"in",  data:{def_val:"%shouldOverwrite%", code:"overwriteWithLayer($gbuffer$, $layer$, $mask$, $overwrite$)"}},
      {name:"output", caption:"out-gbuffer", types:["NBSGbuffer"], singleConnect:false, role:"out", data:{code:"$gbuffer$"}}
    ],
    properties:[
      {name:"shouldOverwrite", type:"bool", val:"true"},
    ],
    allowLoop:false,
    width:200
  },
  {
    name:"lerp gbuffer with layer",
    category:"Gbuffer",
    synonyms:"gbuffer, overwrite, mix, layer",
    pins:[
      {name:"gbuffer", caption:"in-gbuffer", types:["NBSGbuffer"], singleConnect:true, role:"in"},
      {name:"layer", caption:"layer", types:["Layer_t"], singleConnect:true, role:"in"},
      {name:"mask", caption:"mask", types:["mask_t"], singleConnect:true, role:"in"},
      {name:"weight", caption:"weight", types:["float"], singleConnect:true, role:"in",  data:{def_val:"%weight%"}},
      {name:"allowOverwrite", caption:"allowOverwrite", types:["bool"], singleConnect:true, role:"in",  data:{def_val:"%allowOverwrite%", code:"lerpWithLayer($gbuffer$, $layer$, $mask$, $weight$, $allowOverwrite$)"}},
      {name:"output", caption:"out-gbuffer", types:["NBSGbuffer"], singleConnect:false, role:"out", data:{code:"$gbuffer$"}}
    ],
    properties:[
      {name:"weight", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"},
      {name:"allowOverwrite", type:"bool", val:"true"},
    ],
    allowLoop:false,
    width:200
  },
  {
    name:"dither gbuffer with layer",
    category:"Gbuffer",
    synonyms:"gbuffer, dither, mix, layer",
    pins:[
      {name:"gbuffer", caption:"in-gbuffer", types:["NBSGbuffer"], singleConnect:true, role:"in"},
      {name:"layer", caption:"layer", types:["Layer_t"], singleConnect:true, role:"in"},
      {name:"mask", caption:"mask", types:["mask_t"], singleConnect:true, role:"in"},
      {name:"weight", caption:"weight", types:["float"], singleConnect:true, role:"in",  data:{def_val:"%weight%"}},
      {name:"bias", caption:"bias", types:["float"], singleConnect:true, role:"in",  data:{def_val:"%bias%", code:"ditherWithLayer($gbuffer$, $layer$, $mask$, $weight$, $bias$, dtId)"}},
      {name:"output", caption:"out-gbuffer", types:["NBSGbuffer"], singleConnect:false, role:"out", data:{code:"$gbuffer$"}}
    ],
    properties:[
      {name:"weight", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.5"},
      {name:"bias", type:"float", minVal:"0", maxVal:"1", step:"0.01", val:"0.2"},
    ],
    allowLoop:false,
    width:200
  },
  {
    name:"input material",
    category:"Gbuffer",
    synonyms:"gbuffer, read, input, material",
    pins:[
      {name:"value", caption:"%material%", types:["material_t"], singleConnect:false, role:"out", data:{code:"%material%"}}
    ],
    properties:[
      {name:"material", type:"combobox", items:materialDict, val:materialDict[0]}
    ],
    allowLoop:false,
    width:150
  },
  {
    name:"add sparkles",
    category:"Gbuffer",
    synonyms:"gbuffer, sparkles",
    pins:[
      {name:"gbuffer", caption:"in-gbuffer", types:["NBSGbuffer"], singleConnect:true, role:"in"},
      {name:"shouldApply", caption:"shouldApply", types:["bool"], singleConnect:true, role:"in"},
      {name:"albedo", caption:"sparkle albedo", types:["float3"], singleConnect:true, role:"in",  data:{def_val:"%sparkleAlbedo%"}},
      {name:"reflectance", caption:"sparkle reflectance", types:["float"], singleConnect:true, role:"in",  data:{def_val:"%sparkleReflectance%"}},
      {name:"smoothness", caption:"sparkle smoothness", types:["float"], singleConnect:true, role:"in",  data:{def_val:"%sparkleSmoothness%"}},
      {name:"size", caption:"size", types:["float"], singleConnect:true, role:"in",  data:{def_val:"%size%"}},
      {name:"density", caption:"density", types:["float"], singleConnect:true, role:"in",  data:{def_val:"%density%"}},
      {name:"alpha", caption:"max alpha", types:["float"], singleConnect:true, role:"in", data:{code:"applySparkles($gbuffer$, float3($albedo$), $reflectance$, $smoothness$, world_pos, world_view_pos.xyz, $alpha$, $size$, $density$, $shouldApply$, %applyToNormal%, %applyToSmoothness%, %applyToAlbedo%, %applyToReflectance%)"}},
      {name:"output", caption:"out-gbuffer", types:["NBSGbuffer"], singleConnect:false, role:"out", data:{code:"$gbuffer$"}}
    ],
    properties:[
      {name:"sparkleAlbedo", type:"float3", minVal:"0", maxVal:"100", step:"0.01", val:"1, 1, 1"},
      {name:"sparkleReflectance", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"},
      {name:"sparkleSmoothness", type:"float", minVal:"0", maxVal:"100", step:"0.01", val:"1"},
      {name:"size", type:"float", minVal:"0", maxVal:"50", step:"0.01", val:"1"},
      {name:"density", type:"float", minVal:"0", maxVal:"15", step:"0.01", val:"1"},
      {name:"applyToNormal", type:"bool", val:"false"},
      {name:"applyToAlbedo", type:"bool", val:"false"},
      {name:"applyToSmoothness", type:"bool", val:"true"},
      {name:"applyToReflectance", type:"bool", val:"false"},
    ],
    allowLoop:false,
    width:150
  },
];

var GE_defaultExternalsAdditional =
[
  {type:"float4x4", name:"jitteredCamPosToUnjitteredHistoryClip"},
  {type:"float4", name:"prev_zn_zfar"},
  {type:"Buffer", name:"emission_decode_color_map"},
  {type:"texture2D_nosampler", name:"depth_gbuf"},
  {type:"texture2D", name:"envi_cover_intensity_map"},
  {type:"float4", name:"rendering_res"},
  {type:"int", name:"envi_cover_is_temporal_aa"},
  {type:"int", name:"envi_cover_frame_idx"},

  {type:"float4", name:"camera_in_camera_prev_vp_ellipse_center"},
  {type:"float4", name:"camera_in_camera_prev_vp_ellipse_xy_axes"},
  {type:"float4", name:"camera_in_camera_vp_ellipse_center"},
  {type:"float4", name:"camera_in_camera_vp_ellipse_xy_axes"},
  {type:"float", name:"camera_in_camera_vp_y_scale"},
  {type:"float4", name:"camera_in_camera_vp_uv_remapping"},
  {type:"float", name:"camera_in_camera_postfx_lens_view"},
  {type:"float", name:"camera_in_camera_active"},
];