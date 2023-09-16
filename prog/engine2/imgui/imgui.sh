include "shader_global.sh"

float4 imgui_mvp_0;
float4 imgui_mvp_1;
float4 imgui_mvp_2;
float4 imgui_mvp_3;
texture imgui_tex;

shader imgui
{
  blend_src = sa;
  blend_dst = isa;
  blend_asrc = isa;
  blend_adst = zero;
  cull_mode = none;
  z_test = false;
  z_write = false;

  (vs) { mvp@f44 = {imgui_mvp_0, imgui_mvp_1, imgui_mvp_2, imgui_mvp_3}; }
  (ps) { tex@smp2d = imgui_tex; }

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float2 uv  : TEXCOORD0;
      float4 col : COLOR0;
    };
  }

  channel float2 pos=pos;
  channel float2 tc[0]=tc[0];
  channel color8 vcol=vcol;

  hlsl(vs) {
    struct VsInput
    {
      float2 pos : POSITION;
      float2 uv  : TEXCOORD0;
      float4 col : COLOR0;
    };

    VsOutput imgui_vs(VsInput v)
    {
      VsOutput o;
      o.pos = mul(mvp, float4(v.pos.xy, 0.f, 1.f));
      o.uv = v.uv;
      o.col = v.col.bgra; // dagor supports vertex color in BGRA format, but ImGui supplies it as RGBA
      return o;
    }
  }
  compile("target_vs", "imgui_vs");

  hlsl(ps) {
    float4 imgui_ps(VsOutput i) : SV_Target
    {
      return i.col * tex2D(tex, i.uv);
    }
  }
  compile("target_ps", "imgui_ps");
}