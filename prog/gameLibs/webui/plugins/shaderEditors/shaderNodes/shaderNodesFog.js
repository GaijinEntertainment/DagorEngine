"use strict";


var GE_volfog_early_exit_node_name = "volfog early exit"
var GE_volfog_early_exit_var_name = "_volfog_early_exit"



var GE_nodeDescriptionsAdditional =
  [
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


  ];


var GE_defaultExternalsAdditional =
  [
    // type - int, float, float2, float3, float4
    // template: {type: "float3", name: "camera_pos"},
    {type: "texture2D_nosampler", name:"volfog_occlusion"},
    {type: "texture2D_nosampler", name:"volfog_mask_tex"},
    {type: "float4",  name:"world_to_volfog_mask"},
    {type: "float4",  name:"froxel_fog_fading_params"},
    {type: "float", name:"daskies_time"},
    {type: "float4", name:"volfog_shadow_res"},
    {type: "float", name:"volfog_shadow_accumulation_factor"},
    {type: "float", name:"volfog_prev_range_ratio"},
    {type: "texture3D_nosampler", name:"prev_volfog_shadow"},
    {type: "texture2D_nosampler", name:"volfog_shadow_occlusion"},
    {type: "float", name:"nbs_clouds_start_altitude2_meters"},
    {type: "float4", name:"volfog_froxel_volume_res"},
    {type: "float4", name:"inv_volfog_froxel_volume_res"},
    {type: "float4", name:"volfog_froxel_range_params"},
    {type: "texture2D_nosampler", name:"volfog_poisson_samples"},
    {type: "float4", name:"jitter_ray_offset"},
    {type: "float4", name:"vf_wind_dir"},

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
    {type: "float", name:"static_shadow_scrolled_depth_min_0"},
    {type: "float", name:"static_shadow_scrolled_depth_max_0"},
    {type: "int", name:"distant_fog_use_static_shadows"},
    {type: "float", name:"debug_disable_static_pcf_center_sample"},

    {type:"float4", name:"camera_in_camera_prev_vp_ellipse_center"},
    {type:"float4", name:"camera_in_camera_prev_vp_ellipse_xy_axes"},
    {type:"float4", name:"camera_in_camera_vp_ellipse_center"},
    {type:"float4", name:"camera_in_camera_vp_ellipse_xy_axes"},
    {type:"float", name:"camera_in_camera_vp_y_scale"},
    {type:"float4", name:"camera_in_camera_vp_uv_remapping"},
    {type:"float", name:"camera_in_camera_postfx_lens_view"},
    {type:"float", name:"camera_in_camera_active"},
  ];


