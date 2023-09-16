include "postfx_inc.sh"
include "shader_global.sh"

int swizzled_texture_type = 0;
interval swizzled_texture_type : swizzled_texture_2d < 1, swizzled_texture_cube < 2, swizzled_texture_array < 3, swizzled_texture_3d < 4, swizzled_texture_2d_depth < 5, swizzled_texture_cube_array;

texture swizzled_texture;
float4 swizzled_texture_size;
float4 swizzled_texture_face_mip;

float4 tex_swizzling_r = (1, 0, 0, 0);
float4 tex_swizzling_g = (0, 1, 0, 0);
float4 tex_swizzling_b = (0, 0, 1, 0);
float4 tex_swizzling_a = (0, 0, 0, 1);

float4 tex_mod_r = (1, 0, 0, 0);
float4 tex_mod_g = (0, 0, 0, 0);
float4 tex_mod_b = (0, 1, 0, 0);
float4 tex_mod_a = (0, 0, 1, 0);

float cube_index = 0;

shader debug_tex_overlay
{
  cull_mode  = none;
  z_test = false;
  z_write = false;

  blend_src = sa; blend_dst = isa;

  if (swizzled_texture_type == swizzled_texture_2d)
  {
    (ps) { swizzled_texture@smp2d = swizzled_texture; }
  }
  else if (swizzled_texture_type == swizzled_texture_cube)
  {
    (ps) { swizzled_texture@smpCube = swizzled_texture; }
  }
  else if (swizzled_texture_type == swizzled_texture_3d)
  {
    (ps) { swizzled_texture@smp3d = swizzled_texture; }
  }
  else if (swizzled_texture_type == swizzled_texture_array)
  {
    (ps) { swizzled_texture@smpArray = swizzled_texture; }
  }
  else if (swizzled_texture_type == swizzled_texture_2d_depth)
  {
    (ps) { swizzled_texture@shd = swizzled_texture; }
  }
  else if ((swizzled_texture_type == swizzled_texture_cube_array) && (hardware.dx11 || hardware.vulkan))
  {
    (ps) {
      swizzled_texture@smpCubeArray = swizzled_texture;
      cube_index@f1 = (cube_index);
    }
  }

  (vs) {
    swizzled_texture_size@f4 = swizzled_texture_size;
    swizzled_texture_face_mip@f4 = swizzled_texture_face_mip;
  }

  (ps) {
    tex_swizzling_r@f4 = tex_swizzling_r;
    tex_swizzling_g@f4 = tex_swizzling_g;
    tex_swizzling_b@f4 = tex_swizzling_b;
    tex_swizzling_a@f4 = tex_swizzling_a;

    tex_mod_r@f2 = tex_mod_r;
    tex_mod_g@f2 = tex_mod_g;
    tex_mod_b@f2 = tex_mod_b;
    tex_mod_a@f2 = tex_mod_a;

    swizzled_texture_face_mip@f4 = swizzled_texture_face_mip;
  }


  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float4 texcoord : TEXCOORD0;
    };
  }

  USE_POSTFX_VERTEX_POSITIONS()

  hlsl(vs) {
    #include <get_cubemap_vector.hlsl>
    VsOutput draw_swizzled_texture(uint vertexId : SV_VertexID)
    {
      float2 pos = getPostfxVertexPositionById(vertexId);
      VsOutput output;
      output.pos = float4(pos.x, pos.y, 1, 1);
      output.pos.xy *= swizzled_texture_size.xy;
      output.pos.xy += swizzled_texture_size.zw;

      float2 tc = screen_to_texcoords(pos);
      ##if swizzled_texture_type == swizzled_texture_2d || swizzled_texture_type == swizzled_texture_2d_depth || swizzled_texture_type == swizzled_texture_array || swizzled_texture_type == swizzled_texture_3d
      output.texcoord = float4( tc, swizzled_texture_face_mip.xy );
      ##else

      float3 v = GetCubemapVector(tc*2-1, int(swizzled_texture_face_mip.x+0.01));

      output.texcoord = float4( v, swizzled_texture_face_mip.y );
      ##endif

      return output;
    }
  }

  hlsl(ps) {
    float4 xblend(float4 acc, float4 v, float cnt, int filter)
    {
      bool fz = filter & 4096;
      bool fo = filter & 8192;
      bool fa = filter & 16384;

      if (fz && max3(v.x,v.y,v.z) == 0)
        return acc;
      if (fo && min3(v.x,v.y,v.z) > 0.98)
        return acc;

      float alpha = fa ? v.a * 0.2 : 0.2;
      acc.xyz = acc.xyz * (1.-alpha) + v.xyz * alpha;
//      acc.a   = max(acc.a,  v.a);
//      acc.a   = acc.a * (1.-alpha) + v.a* alpha;
      acc.a = 1;
      return acc;
    }

    float4 draw_swizzled_texture(VsOutput input) : SV_Target
    {
      ##if swizzled_texture_type == swizzled_texture_2d_depth
        float4 dst = shadow2D(swizzled_texture, float3(input.texcoord.xy, 1));
        dst.a = 1;
      ##else
        float scale = 0.8;

        ##if swizzled_texture_type == swizzled_texture_2d
          float4 src = tex2Dlod( swizzled_texture, input.texcoord );
        ##elif swizzled_texture_type == swizzled_texture_array
          float4 src = tex3Dlod( swizzled_texture, input.texcoord );
        ##elif swizzled_texture_type == swizzled_texture_3d
          int filter = (int)swizzled_texture_face_mip.z;
          int face = filter & 4095;
          int numSlices = (int)swizzled_texture_face_mip.w;
          float4 src = 0;
          if (numSlices < 0)
            src = tex3Dlod( swizzled_texture, input.texcoord );
          else
          {
            uint4 dim;
            swizzled_texture.GetDimensions(0, dim.x, dim.y, dim.z, dim.w);
            if (numSlices == 0)
              numSlices = dim.z;
            int minz = face * numSlices;
            int maxz = minz + numSlices;
            minz = min(minz, (int)dim.z);
            maxz = min(maxz, (int)dim.z);
            float invscale = 1./scale;
            for (int i = minz; i < maxz; i++)
            {
              float zf = ((float)i + 0.5) / (float)dim.z;
              float4 newcoord = input.texcoord * invscale;
              newcoord.z = zf;
              newcoord.xy -= (1.-scale) * (1-zf);
              if (newcoord.x <= 1 && newcoord.y <= 1 && newcoord.x >=0 && newcoord.y >=0)
                src = xblend(src, tex3Dlod(swizzled_texture, newcoord ), (float)dim.z, filter);
            }
          }
        ##elif swizzled_texture_type == swizzled_texture_cube
          float4 src = texCUBElod( swizzled_texture, input.texcoord );
        ##elif swizzled_texture_type == swizzled_texture_cube_array
          ##if (hardware.dx11 || hardware.vulkan)
            float4 src = texCUBEArraylod(swizzled_texture, float4(input.texcoord.xyz, cube_index), input.texcoord.w);
          ##else
            float4 src = float4(0, 0, 0, 0);
          ##endif
        ##endif

        float4 dst;
        dst.r = dot( tex_swizzling_r, src ) * tex_mod_r.x + tex_mod_r.y;
        dst.g = dot( tex_swizzling_g, src ) * tex_mod_g.x + tex_mod_g.y;
        dst.b = dot( tex_swizzling_b, src ) * tex_mod_b.x + tex_mod_b.y;
        dst.a = dot( tex_swizzling_a, src ) * tex_mod_a.x + tex_mod_a.y;

      ##endif

      return dst;
    }
  }

  compile("target_vs", "draw_swizzled_texture");
  if (swizzled_texture_type == swizzled_texture_cube_array)
  {
    compile("ps_5_0", "draw_swizzled_texture");
  }
  else
  {
    compile("target_ps", "draw_swizzled_texture");
  }
}
