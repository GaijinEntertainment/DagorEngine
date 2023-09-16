include "shader_global.sh"
include "dynamic_opaque_inc.sh"
include "cloud_mask.sh"
include "xray_destruction.sh"
include "gbuffer.sh"
include "xray_helper.sh"

texture outline_source_tex;
texture noise_128_tex_hash;
float4 outline_tc_scale_offset = float4(1, 1, 0, 0);
float4 outline_pos_scale_offset = float4(1, 1, 0, 0);
float4 outline_color_xray = (1, 1, 1, 1);
float4 surface_color = (1, 1, 1, 1);
float outline_armor_base_alpha = 0.2;
float outline_armor_alpha_slope = 7.0;
float outline_part_alpha = 1.0;
//x - surface alpha, z,w - hsv range
float4 outline_params;
float4 outline_point_to_eye;
float4 outline_screen_pos;
float outline_use_fog = 0;
float outline_special_vision = 0;
float outline_line_width = 1.0;
float4 outline_dissolve_spot = float4(0, 0, 100, 100);
buffer simulated_part_bbox_tm;
texture xray_destruction_color_scheme;

int outline_creases = 0;
interval outline_creases : outline_creases_off < 1, outline_creases_on;
int fill_outline = 0;
interval fill_outline : fill_outline_off < 1, fill_outline_on;
int fill_outline_lighting = 0;
interval fill_outline_lighting : fill_outline_lighting_off < 1, fill_outline_lighting_on;
int outline_alpha_mode = 0;
interval outline_alpha_mode : outline_alpha_none < 1, outline_alpha_node_color < 2, outline_alpha_hsv < 3, outline_alpha_on < 4, outline_alpha_sonar;

int xray_destruction = 0;
interval xray_destruction : off < 1, ps_only < 2, full_sim;

int xray_generate_tessellated_geometry = 0;
interval xray_generate_tessellated_geometry : off < 1, on;

int is_xray_normal_map_mode = 0;
interval is_xray_normal_map_mode : is_xray_normal_map_mode_off < 1, is_xray_normal_map_mode_on;

shader edge_detect, edge_detect_box
{
  supports global_frame;

  if (shader == edge_detect_box)
  {
    cull_mode = cw;
  }
  else
  {
    cull_mode = none;
  }
  z_test = false;
  z_write = false;

  if (outline_alpha_mode != outline_alpha_none ||
    outline_creases == outline_creases_on)
  {
    blend_src = sa; blend_dst = isa;
    blend_asrc = one; blend_adst = isa;
    NO_ATEST()
  }
  else
  {
    no_ablend;
    USE_ATEST_1()
  }

  (ps) {
    outline_source_tex@smp2d = outline_source_tex;
    outline_color@f4 = outline_color_xray;
    surface_color@f4 = surface_color;
    outline_params@f4 = outline_params;
    outline_point_to_eye@f4 = outline_point_to_eye;
    outline_line_width@f1 = (outline_line_width);
    outline_dissolve_spot@f4 = (outline_dissolve_spot);
    outline_part_alpha@f1 = (outline_part_alpha);
    outline_armor_base_alpha@f1 = (outline_armor_base_alpha);
    outline_armor_alpha_slope@f1 = (outline_armor_alpha_slope);
    xray_destruction_color_scheme@smp2d = xray_destruction_color_scheme;
  }
  (vs) {
    outline_tc_scale_offset@f4 = outline_tc_scale_offset;
    simulated_part_bbox_tm@cbuf = simulated_part_bbox_tm hlsl {
      cbuffer simulated_part_bbox_tm@cbuf
      {
        float4 simulated_part_bbox_tm[3];
      }
    };
    globtm@f44 = globtm;
  }

  USE_POSTFX_VERTEX_POSITIONS()

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      noperspective float2 texcoord : TEXCOORD0;
    };

    half3 hsv_to_rgb(half h, half s, half v)
    {
      int i;
      half f, p, q, t;

      if ( s <= 0.0 )
        return half3(v, v, v);

      h /= 60;
      i = floor( h );
      f = h - i;
      p = v * ( 1 - s );
      q = v * ( 1 - s * f );
      t = v * ( 1 - s * ( 1 - f ) );

      if ( i == 0)
        return half3(v, t, p);
      else if ( i == 1 )
        return half3(q, v, p);
      else if ( i == 2 )
        return half3(p, v, t);
      else if ( i == 3 )
        return half3(p, q, v);
      else if ( i == 4 )
        return half3(t, p, v);
      else
        return half3(v, p, q);
    }
  }

  hlsl(vs) {
    VsOutput edge_detect_vs(uint vertex_id : SV_VertexID)
    {
      VsOutput output;
      ##if shader == edge_detect_box
        float4 vecX = simulated_part_bbox_tm[0];
        float4 vecY = simulated_part_bbox_tm[1];
        float4 vecZ = simulated_part_bbox_tm[2];

        float3 pos = float3(vertex_id / 4, vertex_id / 2 % 2 == 1, (vertex_id % 2) != 0);

        float3 worldPos = pos.x*vecX.xyz + pos.y*vecY.xyz +
                          pos.z*vecZ.xyz + float3(vecX.w, vecY.w, vecZ.w);
        output.pos = mulPointTm(worldPos, globtm);
        pos = output.pos.xyz / output.pos.w;
      ##else
        float2 pos = getPostfxVertexPositionById(vertex_id);
        output.pos = float4(pos.x, pos.y, 1, 1);
      ##endif

      float2 texcoord = screen_to_texcoords(pos.xy);
      texcoord = outline_tc_scale_offset.zw + texcoord.xy * outline_tc_scale_offset.xy;
      output.texcoord = texcoord;
      return output;
    }
  }

  hlsl(ps) {
    half4 get_destruction_color(half sunDotN, half deform)
    {
      if (deform == 0 && sunDotN == 0)
        return half4(0, 0, 0, 0);

      half lighting = 0.8 + 0.2 * (sunDotN * 2 - 1);
      half4 destructionColor = half4(tex2D(xray_destruction_color_scheme, float2(deform, 0)).xyz * lighting, outline_part_alpha);
      destructionColor.a *= lerp(outline_armor_base_alpha, saturate(5 * outline_armor_base_alpha), outline_armor_alpha_slope * deform);
      return destructionColor;
    }

    half4 edge_detect_ps(VsOutput input) : SV_Target
    {
      uint2 dim;
      outline_source_tex.GetDimensions(dim.x, dim.y);
      float2 offs = outline_line_width / dim;

      float2 rvec = input.texcoord.xy - outline_dissolve_spot.xy;
      rvec.y = rvec.y * dim.y / dim.x;
      float distRad = dot(rvec.xy, rvec.xy);
      // dissolve clip
      if (distRad > outline_dissolve_spot.w)
      {
        clip_alpha(-1);
        return 0;
      }

    const half deformationThreshold = 0.2;
    half totalDeformation = 0;
    half outline_intensity_sonar = 0.0;
    ##if outline_creases == outline_creases_on
      half4 nc = tex2Dlod(outline_source_tex, float4(input.texcoord, 0, 0));
      half nci0 = all(nc == 0.0);
      nc.xyz = nc.xyz * 2.0 - 1.0;
      half4 nd, nd1, nd2;
      // Normal discontinuity filter
      #define _CE(s, t, d) nd = t; \
        s = all(nd == 0.0); \
        nd.xyz = nd.xyz * 2.0 - 1.0; \
        d = dot(nc.xyz, nd.xyz); \
        nc.w += nd.w;
      #define _CE1(s, t, i) _CE(s, t, nd1[i])
      #define _CE2(s, t, i) _CE(s, t, nd2[i])
    ##elif outline_alpha_mode == outline_alpha_node_color || outline_alpha_mode == outline_alpha_hsv
      half4 nc = half4(0, 0, 0, 0);
      half4 nd;
      ##if xray_destruction != off
        #define _CE1(s, t, i) nd = t; \
        totalDeformation += nd.y; \
        nd = get_destruction_color(nd.x, nd.y); \
        s = all(nd == 0.0); \
        nc += nd;
        #define _CE2(s, t, i) _CE1(s, t, i);
      ##else
        #define _CE1(s, t, i) nd = t; \
          s = all(nd == 0.0); \
          nc += nd;
        #define _CE2(s, t, i) _CE1(s, t, i);
      ##endif
    ##else
      ##if outline_alpha_mode == outline_alpha_sonar
        half4 c;
        #define _CE1(s, t, i) c = t; \
        s = all(c == 0.0); \
        outline_intensity_sonar = max(c.w, outline_intensity_sonar);
      ##else
        #define _CE1(s, t, i) s = all(t == 0.0);
      ##endif
      #define _CE2(s, t, i) _CE1(s, t, i);
    ##endif

      // Color edge detection
      half4 sample0, sample1;
      _CE1(sample0.x, tex2Dlod(outline_source_tex, float4(input.texcoord.xy + float2(offs.x, 0), 0, 0)), 0)
      _CE1(sample0.y, tex2Dlod(outline_source_tex, float4(input.texcoord.xy + float2(offs.x, offs.y), 0, 0)), 1)
      _CE1(sample0.z, tex2Dlod(outline_source_tex, float4(input.texcoord.xy + float2(0, offs.y), 0, 0)), 2)
      _CE1(sample0.w, tex2Dlod(outline_source_tex, float4(input.texcoord.xy + float2(-offs.x, offs.y), 0, 0)), 3)
      _CE2(sample1.x, tex2Dlod(outline_source_tex, float4(input.texcoord.xy + float2(-offs.x, 0), 0, 0)), 0)
      _CE2(sample1.y, tex2Dlod(outline_source_tex, float4(input.texcoord.xy + float2(-offs.x, -offs.y), 0, 0)), 1)
      _CE2(sample1.z, tex2Dlod(outline_source_tex, float4(input.texcoord.xy + float2(0, -offs.y), 0, 0)), 2)
      _CE2(sample1.w, tex2Dlod(outline_source_tex, float4(input.texcoord.xy + float2(offs.x, -offs.y), 0, 0)), 3)

      half nci = dot(sample0 + sample1, half4(1, 1, 1, 1));
      half outline = 1 - (nci == 0 || nci == 8);
      // dissolve border
      if (distRad > outline_dissolve_spot.z && distRad < outline_dissolve_spot.w)
        outline = nci < 8;

      // Composition

    ##if outline_creases == outline_creases_on
      nd1 = saturate(1.0 - nd1);
      nd2 = saturate(1.0 - nd2);
      float dv = dot(nd1 + nd2, 1.0);
      dv *= step(0.1, dv);
      outline = saturate(outline + dv);
      half nw = (nci + nci0) < 9;
      nc.w = nw ? nc.w / (9 - nci - nci0) : 0;
    ##elif outline_alpha_mode == outline_alpha_node_color || outline_alpha_mode == outline_alpha_hsv
      half nw = nci < 8;
      nc = nw ? nc / (8 - nci) : 0;
    ##else
      half nw = nci < 8;
    ##endif

    // Shading

    half4 color = float4(1, 1, 1, 1);
    ##if xray_destruction != off
      half2 destrData = tex2D(outline_source_tex, input.texcoord.xy).xy;
      half4 destrColor = get_destruction_color(destrData.x, destrData.y);
    ##endif

    ##if outline_alpha_mode == outline_alpha_hsv
      half3 hsv = hsv_to_rgb(lerp(outline_params.z, outline_params.w, nc.w), 1, 1);
      color.rgb *= hsv;
    ##elif outline_alpha_mode == outline_alpha_node_color
      ##if xray_destruction != off
        color *= (totalDeformation > deformationThreshold) ? destrColor
                                                           : outline ? nc : destrColor;
      ##else
        color *= nc;
      ##endif
    ##endif

    ##if fill_outline == fill_outline_on
      ##if fill_outline_lighting == fill_outline_lighting_on && outline_creases == outline_creases_on
        float lighting = 1.0 - dot(nc.xyz, outline_point_to_eye.xyz);
        lighting = saturate(1.0 - pow2(lighting));
      ##else
        float lighting = 1.0;
      ##endif
      color.a *= saturate(outline + nw);
      float4 outlineColor = outline_color;
      ##if xray_destruction != off
        outlineColor *= totalDeformation > deformationThreshold ? destrColor
                                                                : float4(1, 1, 1, lerp(color.a, 0, 10 * totalDeformation));
      ##else
        outlineColor *= color;
      ##endif
        color = lerp(surface_color * color * lighting, outlineColor, outline);
    ##else
      ##if outline_alpha_mode == outline_alpha_sonar
        color.a *= (outline*0.8 + 0.2) * outline_intensity_sonar;
      ##else
        color.a *= outline;
      ##endif
      color *= outline_color;
    ##endif

      clip_alpha(color.a);
      return half4(color);
    }
  }

  compile("target_vs", "edge_detect_vs");
  compile("target_ps", "edge_detect_ps");
}

shader hatching, hatching_vcolor, hatching_simple, hatching_sphere
{
  hlsl {
    #pragma spir-v compiler hlslcc
  }

  if (shader == hatching_sphere)
  {
    z_test = false;
    z_write = false;
  }
  else
  {
    z_test = true;
    z_write = true;
  }

  if (shader == hatching_sphere)
  {
    cull_mode = cw;
  }

  if (is_xray_normal_map_mode == is_xray_normal_map_mode_on)
  {
    no_ablend;
    USE_ATEST_1()
  }
  else if (shader == hatching_sphere)
  {
    NO_ATEST()
  }
  else
  {
    blend_src = sa; blend_dst = zero;
    blend_asrc = one; blend_adst = zero;
    NO_ATEST()
  }

  if (shader == hatching_simple)
  {
    USE_INDIRECT_DRAW()
  }

  DYNAMIC_BLOCK_XRAY()

  if (shader == hatching || shader == hatching_vcolor)
  {
    channel color8 norm = norm unsigned_pack;
  }
  if (shader == hatching_sphere)
  {
    channel float4 norm = norm;
  }
  if (shader == hatching_vcolor)
  {
    channel color8 vcol = vcol;
  }

  if (shader == hatching || shader == hatching_vcolor)
  {
    static int num_bones = 0;
    interval num_bones: no_bones<1, four_bones;
    if (num_bones != no_bones)
    {
      channel color8 tc[4] = extra[0];
      channel color8 tc[5] = extra[1];
    }
    INIT_OPTIONAL_SKINNING()
  }

  hlsl(vs) {
    struct VsInput
    {
      float3 pos : POSITION;
      ##if shader == hatching_simple
      int id: TEXCOORD0;
      ##endif
    ##if shader == hatching || shader == hatching_vcolor || shader == hatching_sphere
      float4 normal : NORMAL;
    ##endif
    ##if shader == hatching || shader == hatching_vcolor
      INIT_BONES_VSINPUT(TEXCOORD4, TEXCOORD5)
    ##endif
    ##if shader == hatching_vcolor
      float4 color : COLOR;
    ##endif
    };
  }

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      int id: TEXCOORD0;
      float3 pointToEye : TEXCOORD1;
      ##if shader == hatching_vcolor
      float4 color : TEXCOORD2;
      ##endif
      ##if shader == hatching || shader == hatching_vcolor || shader == hatching_sphere
      float3 worldNormal : TEXCOORD4;
      ##endif
      ##if xray_destruction != off && shader != hatching_sphere
      float4 deformation : TEXCOORD6;
      ##endif
      float4 screenPos : TEXCOORD3;
      ##if xray_generate_tessellated_geometry == on && shader == hatching_simple
      float3 localPos : TEXCOORD2;
      ##endif
    };

    float3 get_world_normal(VsOutput input)
    {
    ##if shader == hatching || shader == hatching_vcolor || shader == hatching_sphere
      return input.worldNormal;
    ##else
      return cross(normalize(ddx(input.pointToEye)), normalize(ddy(input.pointToEye)));
    ##endif
    }

  }
  if (shader == hatching || shader == hatching_vcolor)
  {
    OPTIONAL_SKINNING_SHADER()
    XRAY_PACKED_RENDERING_HELPER()
  }
  hlsl (vs)
  {
    ##if shader == hatching_simple
    #define PARAMS_ID input.id
    ##elif shader == hatching_sphere
    #define PARAMS_ID instance_id
    ##else
    #define PARAMS_ID get_id(GET_ID_ARGS)
    ##endif
  }
  hlsl (ps)
  {
    #define PARAMS_ID input.id
  }

  (vs) { globtm@f44 = globtm; }

  hlsl(vs) {
      void prepareXrayVSOutput(in VsOutput output, XrayPartParams part_params, out float4 deformation, out float3 pToEye, out float4 pos, uint vertex_id);

      VsOutput hatching_vs(VsInput input, uint vertex_id : SV_VertexID HW_USE_INSTANCE_ID)
      {
      ##if shader == hatching || shader == hatching_vcolor
        SETUP_RENDER_VARS(input.normal.w);
      ##endif
      ##if shader == hatching || shader == hatching_vcolor
        input.pos.xyz = unpack_pos(input.pos);
      ##endif
        uint params_id = min(PARAMS_ID, MAX_XRAY_PARTS - 1);

      ##if xray_destruction == full_sim && shader == hatching_simple
        params_id = xray_part_no;
      ##endif

        VsOutput output;
        float3 worldPos;
        float3 worldNorm;
    ##if shader == hatching || shader == hatching_vcolor
        float3 localNormal = BGR_SWIZZLE(input.normal).xyz * 2 - 1;
        float3 worldDu;
        float3 worldDv;
        instance_skinning(
          input,
          input.pos.xyz,
          localNormal,
          float3(0, 0, 0),
          float3(0, 0, 0),
          worldPos,
          output.pos,
          worldNorm,
          worldDu,
          worldDv);
    ##else
        XrayPartParams part_params = partParams[params_id];
        float3 world_local_x = float3(part_params.world_local[0].x, part_params.world_local[1].x, part_params.world_local[2].x);
        float3 world_local_y = float3(part_params.world_local[0].y, part_params.world_local[1].y, part_params.world_local[2].y);
        float3 world_local_z = float3(part_params.world_local[0].z, part_params.world_local[1].z, part_params.world_local[2].z);
        float3 world_local_pos = float3(part_params.world_local[0].w, part_params.world_local[1].w, part_params.world_local[2].w);
        worldPos = float3(input.pos.x * world_local_x + input.pos.y * world_local_y + input.pos.z * world_local_z + world_local_pos);
        output.pos = mul(float4(worldPos.xyz, 1), globtm);
      ##if shader == hatching_sphere
        worldNorm = input.normal.xyz;
      ##else
        worldNorm = float3(0, 1, 0);
      ##endif
    ##endif

      output.pointToEye = world_view_pos - worldPos;
      output.id = params_id;

      ##if xray_destruction != off && shader != hatching_sphere
        float4 dPos;
        float3 dPToEye;
        float4 deformation;
        prepareXrayVSOutput(output, partParams[params_id], deformation, dPToEye, dPos, vertex_id);
        output.pointToEye = dPToEye;
        output.pos = dPos;
        output.deformation = deformation;
      ##endif

      ##if xray_generate_tessellated_geometry == on && shader == hatching_simple
        output.localPos = input.pos;
      ##endif

      ##if shader == hatching_vcolor
        output.color = BGRA_SWIZZLE(input.color);
      ##endif
      ##if shader == hatching || shader == hatching_vcolor || shader == hatching_sphere
        output.worldNormal = worldNorm;
      ##endif
        output.screenPos = output.pos;
        return output;
      }
  }
  compile("target_vs", "hatching_vs");

  if (xray_destruction != off && shader != hatching_sphere)
  {
    INIT_XRAY_DESTRUCTION()
    if (xray_destruction == full_sim && xray_generate_tessellated_geometry == on)
    {
      USE_XRAY_DESTRUCTION()
    }
    cull_mode = none;
    XRAY_DESTRUCTION_PS()
  }
  else
  {
    if (shader == hatching_vcolor)
    {
      hlsl(ps) {
        #define XRAY_RENDER_SHADER_MUL_INPUT_COLOR 1
      }
    }

    XRAY_RENDER_SHADER(input.pointToEye.xyz, get_world_normal(input), true)

    if (shader == hatching_sphere)
    {
      blend_src = sa; blend_dst = isa;
      blend_asrc = one; blend_adst = isa;
    }
  }
}


int outline_sprite_type = 0;
interval outline_sprite_type : outline_sprite_circle < 1, outline_sprite_silhouette < 2, outline_sprite_patch ;

shader outline_sprite
{
  supports global_frame;
  supports global_const_block;
  USE_POSTFX_VERTEX_POSITIONS()
  INIT_READ_DEPTH_GBUFFER()
  USE_READ_DEPTH_GBUFFER()

  if (outline_sprite_type == outline_sprite_patch)
  {
    blend_src = one; blend_dst = one;
    blend_asrc = one; blend_adst = one;
  }
  else
  {
    blend_src = sa; blend_dst = isa;
    blend_asrc = one; blend_adst = isa;
  }
  NO_ATEST()

  if (outline_sprite_type == outline_sprite_silhouette)
  {
    cull_mode = none;
    z_test = true;
    z_write = false;

    //USE_ALPHA_HDR()
    if (compatibility_mode == compatibility_mode_off)
    {
      INIT_SKY()
    } else
    {
      INIT_SKY_COMPAT()
    }
    USE_SKY_DIFFUSE()

    (vs) { outline_screen_pos@f4 = outline_screen_pos; }
  }
  else if (outline_sprite_type == outline_sprite_patch)
  {
    cull_mode = cw;
    z_test = true;
    z_write = false;
    INIT_ZNZFAR()
  }
  else
  {
    cull_mode = none;
    z_test = false;
    z_write = false;
  }

  (ps) {
    outline_source_tex@smp2d = outline_source_tex;
  }
  (vs) {
    outline_pos_scale_offset@f4 = outline_pos_scale_offset;
    outline_tc_scale_offset@f4 = outline_tc_scale_offset;
  }
  if (outline_sprite_type == outline_sprite_silhouette)
  {
    (ps) {
      outline_use_fog@f1 = (outline_use_fog);
    }
  }
  hlsl {
    #include "pixelPacking/ColorSpaceUtility.hlsl"
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float2 texcoord : TEXCOORD0;
      ##if outline_sprite_type == outline_sprite_patch
      float4 screenPos : TEXCOORD1;
      ##endif
    };
  }

  //INIT_WORLD_LOCAL_MATRIX(vs)
  (vs) { globtm@f44 = globtm; }

  hlsl(vs) {
    VsOutput outline_sprite_vs(uint vertexId : SV_VertexID)
    {
      VsOutput output;

      float2 pos = float2(vertexId / 2, vertexId % 3 ? 1 : 0);
      output.texcoord = outline_tc_scale_offset.zw + pos * outline_tc_scale_offset.xy;
      pos.xy = outline_pos_scale_offset.zw + pos * outline_pos_scale_offset.xy;
      pos.xy = pos.xy * 2 - 1;
      pos.y *= HALF_RT_TC_YSCALE * 2;

      ##if outline_sprite_type == outline_sprite_silhouette
      output.pos = float4(pos.xy, outline_screen_pos.z/outline_screen_pos.w, 1.0);
      ##elif outline_sprite_type == outline_sprite_patch
      output.screenPos = output.pos = mul(float4(pos.x, 0.0, pos.y, 1), globtm);
      ##else
      output.pos = float4(pos.xy, 1.0, 1.0);
      ##endif

      return output;
    }
  }

  if (outline_sprite_type == outline_sprite_circle || outline_sprite_type == outline_sprite_patch)
  {
    (ps) {
      outline_params@f4 = outline_params;
    }
    if (outline_sprite_type != outline_sprite_patch)
    {
      (ps) {
        outline_color@f4 = outline_color_xray;
        surface_color@f4 = surface_color;
      }
    }
    XRAY_HDR_PS_OUTPUT(SPRITE_OUTPUT, half4)

    hlsl(ps) {
      #define DEPTH_BIAS 0.1

      SPRITE_OUTPUT outline_sprite_ps(VsOutput input)
      {
        ##if outline_sprite_type == outline_sprite_patch && compatibility_mode == compatibility_mode_off
        half3 screenTc = input.screenPos.xyz / input.screenPos.w;
        screenTc.xy = screenTc.xy * half2(0.5, -0.5) + half2(0.5, 0.5);
        float depthScene = linearize_z(readGbufferDepth(screenTc.xy), zn_zfar.zw);
        if (linearize_z(screenTc.z, zn_zfar.zw) - DEPTH_BIAS > depthScene)
          discard;
        ##endif

        half4 color = tex2Dlod(outline_source_tex, float4(input.texcoord, 0, 0));
        half2 tc = (input.texcoord - 0.5);
        half r = tc.x * tc.x + tc.y * tc.y;
        half fade = saturate(r * outline_params.x + outline_params.y);

        ##if outline_sprite_type == outline_sprite_patch
        half thickness = lerp(outline_params.z, outline_params.w, fade);
        half r2 = (1 - outline_params.y) / outline_params.x;
        half alpha = 1.0 - saturate((r - r2) / (0.25 - r2));
        half4 result_base = half4(thickness.xxx, alpha);
        ##else
        color *= lerp(outline_color, surface_color, fade);
        clip_alpha(color.a);
        half4 result_base = color;
        ##endif
        return getXrayHdrOutput(result_base);
      }
    }
  }
  else if (outline_sprite_type == outline_sprite_silhouette)
  {
    (ps) {
      outline_params@f4 = outline_params;
      outline_color@f4 = outline_color_xray;
      outline_screen_pos@f4 = outline_screen_pos;
      outline_point_to_eye@f4 = outline_point_to_eye;
      outline_special_vision@f1 = (outline_special_vision, 0, 0, 0);
    }

    GET_CLOUD_VOLUME_MASK()
    if (compatibility_mode == compatibility_mode_off)
    {
      INIT_BRUNETON_FOG(ps)
    } else {
      INIT_BRUNETON_FOG_COMPAT(ps)
    }
    USE_BRUNETON_FOG()

    hlsl(ps) {
      #define pack_hdr(a) (a)
      #define SILHOUETTE_MIN_LAMBERT 0.3

      struct SPRITE_OUTPUT
      {
        half4 color : SV_Target0;
        half4 motion : SV_Target1;
      };

      SPRITE_OUTPUT outline_sprite_ps(VsOutput input)
      {
        #define SAMPLE_TEX(offs) \
          color = tex2Dlod(outline_source_tex, float4(input.texcoord + offs * outline_params.zw, 0, 0)); \
          finalColor += color; \
          weight += any(color.rgba > 0) ? 1 : 0;

        #define SAMPLE_TEX_QUAD(dir) \
          SAMPLE_TEX(half2(1.0, 1.0) * dir) \
          SAMPLE_TEX(half2(3.0, 1.0) * dir) \
          SAMPLE_TEX(half2(3.0, 3.0) * dir) \
          SAMPLE_TEX(half2(1.0, 3.0) * dir)

        half4 finalColor = half4(0, 0, 0, 0);
        half weight = 0;
        half4 color;

        SAMPLE_TEX_QUAD(half2(1, 1))
        SAMPLE_TEX_QUAD(half2(-1, 1))
        SAMPLE_TEX_QUAD(half2(-1, -1))
        SAMPLE_TEX_QUAD(half2(1, -1))

        finalColor.rgb *= weight > 0 ? rcp(weight) : 0;
        finalColor.a = weight * 0.0625;

        float motionAlpha = 1;
        float3 normal = normalize(outline_point_to_eye.xyz);
        float NdotL = dot(normal, -from_sun_direction.xyz);
        finalColor.rgb *= outline_color.rgb;
        BRANCH
        if (outline_special_vision == 0)
          finalColor.rgb *= sun_color_0.rgb * max(NdotL, SILHOUETTE_MIN_LAMBERT) + GetSkySHDiffuse(normal);
        else
        {
          finalColor.rgb = float3(0.4, 0, 0);
        }
        finalColor.a *= outline_color.a;

        float4 screenTc = outline_screen_pos;
        screenTc.xy = screenTc.xy * RT_SCALE_HALF + float2(0.50001, 0.50001) * screenTc.w;
        finalColor.a *= get_cloud_volume_mask(screenTc);

        float2 motion = finalColor.a > 0 ? float2(outline_params.x, outline_params.y) : float2(0, 1);

        BRANCH
        if (outline_use_fog > 0.f && outline_special_vision == 0)
          finalColor.rgb = apply_fog(finalColor.rgb, outline_point_to_eye.xyz);

        finalColor.rgb = pack_hdr(finalColor).rgb;
        clip_alpha(finalColor.a);
        SPRITE_OUTPUT output;
        output.color = finalColor;
        output.motion = half4(motion.x, motion.y, 0, motionAlpha);
        return output;
      }
    }
  }

  compile("target_vs", "outline_sprite_vs");
  compile("target_ps", "outline_sprite_ps");
}