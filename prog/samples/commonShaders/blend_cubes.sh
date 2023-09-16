include "shader_global.sh"

texture dynamic_cube_tex_1;
texture dynamic_cube_tex_2;
float4 blend_cubes_params;
float dynamic_cube_tex_blend = 0;
texture dynamic_cube_tex;


shader blend_cubes, blur_cubes
{
  //supports global_frame;// because we likely call render scene right before it
  cull_mode  = none;
  z_test = false;
  z_write = false;
  no_ablend;


  channel float2 pos = pos;


  (vs){
    blend_cubes_params@f4 = blend_cubes_params;
  }

  USE_HDR_SH()

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float3 texcoord : TEXCOORD0;
    };
  }


  hlsl(vs) {

    VsOutput blend_cubes_vs(float2 pos : POSITION)
    {
      VsOutput output;
      output.pos = float4(pos.x, pos.y, 1, 1);
      float2 texcoord2 = pos.xy + blend_cubes_params.xy;
      if (blend_cubes_params.z < 0.5)
        output.texcoord = float3(1, texcoord2.y, -texcoord2.x);
      else if (blend_cubes_params.z < 1.5)
        output.texcoord = float3(-1, texcoord2.y, texcoord2.x);
      else if (blend_cubes_params.z < 2.5)
        output.texcoord = float3(texcoord2.x, 1, -texcoord2.y);
      else if (blend_cubes_params.z < 3.5)
        output.texcoord = float3(texcoord2.x, -1, texcoord2.y);
      else if (blend_cubes_params.z < 4.5)
        output.texcoord = float3(texcoord2.x, texcoord2.y, 1);
      else
        output.texcoord = float3(-texcoord2.x, texcoord2.y, -1);

      return output;
    }
  }

  (ps){
    blend_cubes_params@f4 = blend_cubes_params;
    dynamic_cube_tex_1@smpCube = dynamic_cube_tex_1;
    dynamic_cube_tex_2@smpCube = dynamic_cube_tex_2;
    dynamic_cube_tex_blend@f1 = (dynamic_cube_tex_blend);
  }

  hlsl(ps) {

    fixed4 blend_cubes_ps(VsOutput input) : SV_Target
    {
##if shader == blur_cubes
      half4 result = 0;

      float3 texcoord = input.texcoord + float3(blend_cubes_params.w, 0, 0);
      half4 cube1 = gamma_to_linear_rgba(texCUBE(dynamic_cube_tex_1, texcoord));
      half4 cube2 = gamma_to_linear_rgba(texCUBE(dynamic_cube_tex_2, texcoord));
      result += lerp(cube1, cube2, dynamic_cube_tex_blend);

      texcoord = input.texcoord + float3(-blend_cubes_params.w, 0, 0);
      cube1 = gamma_to_linear_rgba(texCUBE(dynamic_cube_tex_1, texcoord));
      cube2 = gamma_to_linear_rgba(texCUBE(dynamic_cube_tex_2, texcoord));
      result += lerp(cube1, cube2, dynamic_cube_tex_blend);

      texcoord = input.texcoord + float3(0, blend_cubes_params.w, 0);
      cube1 = gamma_to_linear_rgba(texCUBE(dynamic_cube_tex_1, texcoord));
      cube2 = gamma_to_linear_rgba(texCUBE(dynamic_cube_tex_2, texcoord));
      result += lerp(cube1, cube2, dynamic_cube_tex_blend);

      texcoord = input.texcoord + float3(0, -blend_cubes_params.w, 0);
      cube1 = gamma_to_linear_rgba(texCUBE(dynamic_cube_tex_1, texcoord));
      cube2 = gamma_to_linear_rgba(texCUBE(dynamic_cube_tex_2, texcoord));
      result += lerp(cube1, cube2, dynamic_cube_tex_blend);

      texcoord = input.texcoord + float3(0, 0, blend_cubes_params.w);
      cube1 = gamma_to_linear_rgba(texCUBE(dynamic_cube_tex_1, texcoord));
      cube2 = gamma_to_linear_rgba(texCUBE(dynamic_cube_tex_2, texcoord));
      result += lerp(cube1, cube2, dynamic_cube_tex_blend);

      texcoord = input.texcoord + float3(0, 0, -blend_cubes_params.w);
      cube1 = gamma_to_linear_rgba(texCUBE(dynamic_cube_tex_1, texcoord));
      cube2 = gamma_to_linear_rgba(texCUBE(dynamic_cube_tex_2, texcoord));
      result += lerp(cube1, cube2, dynamic_cube_tex_blend);

      return linear_to_gamma_rgba(result / 6.);
##else
      fixed4 cube1 = gamma_to_linear_rgba(texCUBE(dynamic_cube_tex_1, input.texcoord));
      fixed4 cube2 = gamma_to_linear_rgba(texCUBE(dynamic_cube_tex_2, input.texcoord));
      return linear_to_gamma_rgba(lerp(cube1, cube2, (fixed) dynamic_cube_tex_blend));
##endif
    }
  }
  compile("target_vs", "blend_cubes_vs");
  compile("target_ps", "blend_cubes_ps");
}


