include "shader_global.sh"
include "gbuffer.sh"
include "wires_helper.sh"
include "static_shadow.sh"
include "vr_multiview.sh"

buffer cables_buf;
float pixel_scale;

int cables_render_pass = 1;
interval cables_render_pass : cables_render_pass_shadow < 1, cables_render_pass_opaque < 2, cables_render_pass_trans;

shader wires
{
  supports global_frame;
  supports global_const_block;
  cull_mode = none;

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float4 normal_blend : NORMAL;
    ##if cables_render_pass == cables_render_pass_trans
      float3 fogAdd : TEXCOORD0;
    ##else
      float3 pointToEye : TEXCOORD0;
    ##endif
    };
  }

  DISABLE_ASSERT_STAGE(vs)

  (vs) {
    globtm@f44 = globtm;
    pixel_scale@f1 = (pixel_scale, 0, 0, 0);
  }

  (vs) {
    cables_buf@buf = cables_buf hlsl {
      #include "render/cables.hlsli"
      StructuredBuffer<CableData> cables_buf@buf;
    }
  }

  if (cables_render_pass == cables_render_pass_trans)
  {
    CABLES_FOG_VS()
  }

  VR_MULTIVIEW_RENDERING(cables_render_pass == cables_render_pass_opaque, true)

  hlsl(vs) {
    VsOutput cables_vs(uint vertex_id : SV_VertexID USE_VIEW_ID)
    {
      uint instance_id = vertex_id / (TRIANGLES_PER_CABLE + 4);
      vertex_id = vertex_id % (TRIANGLES_PER_CABLE + 4);

      if (vertex_id == TRIANGLES_PER_CABLE + 3)
        vertex_id--;
      if (vertex_id != 0)
        vertex_id--;
      VsOutput output;
      float point_count = TRIANGLES_PER_CABLE * 0.5;
      float4 point1_rad = structuredBufferAt(cables_buf, instance_id).point1_rad;
      float4 point2_sag = structuredBufferAt(cables_buf, instance_id).point2_sag;
      float3 start = point1_rad.xyz;
      float3 end = point2_sag.xyz;
      float3 dir = end - start;
      float sag = point2_sag.w;
      int point_num = vertex_id >> 1;
      dir.y += sag * 4 * (point_num / point_count - 1);
      float3 pos = start + dir * point_num / point_count;
      float3 right = normalize(float3(dir.x, dir.y + sag * 4 * point_num / point_count, dir.z));

      // TODO: if (point1_rad.w <= 0) { /* some actions with ragged cables */ }
      float radius = abs(point1_rad.w); // ragged cable bit is a sign bit of cable radius
      float blend = 1;
      ##if cables_render_pass != cables_render_pass_shadow
        float w = dot(float4(pos, 1), float4(globtm[0].w, globtm[1].w, globtm[2].w, globtm[3].w));
        float pixel_rad = w * pixel_scale; //can be negative if point behind camera
        radius = max(radius, pixel_rad); //increase radius to at least one pixel size
        blend = abs(point1_rad.w / pixel_rad);
        float visibleDistance = 100; //meters
        ##if cables_render_pass == cables_render_pass_opaque
          if (w > visibleDistance)
            radius = 0; //to not render far cables in opaque pass
        ##else //cables_render_pass == cables_render_pass_trans
          blend = 2 - w / visibleDistance; //blend when farther than visibleDistance;
        ##endif
        output.normal_blend.w = blend;
      ##endif

      float3 view;
      ##if cables_render_pass == cables_render_pass_shadow
        view = from_sun_direction;
      ##else
        view = normalize(pos - world_view_pos);
      ##endif
      float3 up = normalize(cross(right, view));
      float3 normal = normalize(cross(right, up));
      if (vertex_id & 1)
        up = -up;
      pos += up * radius;
      normal += up * saturate(blend - 1); //to imitate rounded normal at close distance

      float3 pointToEye = world_view_pos - pos;

      output.pos = mulPointTm(pos, globtm);
      output.normal_blend.xyz = normal;

      ##if cables_render_pass == cables_render_pass_trans
        half3 fogMul, fogAdd;
        get_cables_fog(pointToEye, output.pos, fogMul, fogAdd);
        output.fogAdd = fogAdd;
      ##else
        output.pointToEye = pointToEye;
      ##endif

      VR_MULTIVIEW_REPROJECT(output.pos);

      return output;
    }
  }

  if (cables_render_pass == cables_render_pass_opaque || cables_render_pass == cables_render_pass_shadow)
  {
    INIT_STATIC_SHADOW_PS()
    WRITE_GBUFFER()
    hlsl(ps) {
      GBUFFER_OUTPUT cables_ps(VsOutput input HW_USE_SCREEN_POS)
      {
        UnpackedGbuffer result;
        init_gbuffer(result);
        init_albedo(result, float3(0.0, 0.0, 0.0));
        init_normal(result, normalize(input.normal_blend.xyz));
        //init_dynamic(result, 1);
        init_smoothness(result, 0.25);
        init_metalness(result, 0);
        return encode_gbuffer3(result, input.pointToEye, GET_SCREEN_POS(input.pos));
      }
    }
  }
  else //if (cables_render_pass == cables_render_pass_trans)
  {
    SKY_HDR()
    z_write = false;
    blend_src = sa; blend_dst = isa;
    blend_asrc = 0; blend_adst = 1;
    hlsl(ps) {
      half4 cables_ps(VsOutput input HW_USE_SCREEN_POS) : SV_Target0
      {
        ##if in_editor_assume == no
          if (input.normal_blend.w > 1)
            discard; //to avoid render transparent cable on opaque one
        ##endif
        return half4(pack_hdr(input.fogAdd).rgb, saturate(input.normal_blend.w)); //return blended black color with fog and scattered light applied
      }
    }
  }

  compile("target_vs", "cables_vs");
  compile("target_ps", "cables_ps");
}