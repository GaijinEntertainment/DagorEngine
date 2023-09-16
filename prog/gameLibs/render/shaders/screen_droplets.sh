include "sky_shader_global.sh"
include "viewVecVS.sh"
include "shader_global.sh"

//Begin time, End time, Rain force, Fadeout R
float4 screen_droplets_setup = (0.0, 0.0, -1.0, 0.4);
texture frame_tex;
float screen_droplets_intensity;
int screen_droplets_has_leaks;

shader screen_droplets
{
  supports global_frame;

  cull_mode = none;
  z_write = false;
  z_test = false;

  hlsl
  {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float4 texcoord : TEXCOORD0;
    };
  }

  USE_POSTFX_VERTEX_POSITIONS()

  INIT_RENDERING_RESOLUTION(vs)
  hlsl(vs)
  {
    VsOutput screen_droplets_vs(uint vertexId : SV_VertexID)
    {
      VsOutput output;
      float2 inpos = getPostfxVertexPositionById(vertexId);

      output.pos = float4(inpos, 0, 1);
      output.texcoord.xy = screen_to_texcoords(inpos);
      output.texcoord.zw = output.texcoord.xy * float2(rendering_res.x/rendering_res.y, 1.0);

      return output;
    }
  }
  USE_AND_INIT_VIEW_VEC_PS()

  (ps) {
    current_time@f1 = (time_phase(0, 0));
    screen_droplets_setup@f4 = (screen_droplets_setup);
    frame_tex@smp2d = frame_tex;
    screen_droplets_intensity@f1 = (screen_droplets_intensity);
    screen_droplets_has_leaks@i1 = (screen_droplets_has_leaks);
  }

  hlsl(ps)
  {
    #define rainStarted (screen_droplets_setup.x)
    #define rainEnded (screen_droplets_setup.y)
    #define rainForce (screen_droplets_setup.z)
    #define rainFadeout (screen_droplets_setup.w)

    #define MAX_DROP_R 0.15
    #define MAX_DROP_RI (1.0 / MAX_DROP_R)
    #define MAX_TRAIL_R 0.15
    #define GRID_X 5.4
    #define GRID_Y 0.9
    #define SHRINK_Y 0.059
    #define MOVE_BOARD_K 0.75

    #define STATIC_DROP_R 0.1
    #define STATIC_DROP_RI (1.0 / STATIC_DROP_R)

    #define TIME_K (0.35)

    float S(float a, float b, float t)
    {
      //Smoothstep is undefined when a>=b
      //Makes it defined
      float s = smoothstep(min(a,b),max(a,b),t);
      if (a == b) return 0.0;
      else if (a > b) return 1. - s;
      else return s;
    }

    //Fast noise
    half3 N13(float p)
    {
      half3 p3 = frac(half3(p, p, p) * half3(0.1031, 0.11369, 0.13787));
      p3 += dot(p3, p3.yzx + 19.19);
      return frac(half3((p3.x + p3.y) * p3.z, (p3.x + p3.z) * p3.y, (p3.y + p3.z) * p3.x));
    }
    float N(float t)
    {
      return frac(sin((t * 12.9898)) * 7658.76);
    }
    float3 smoothUnion(float3 d1, float3 d2, float k)
    {
      float h = clamp(.5 + .5*(d2.x-d1.x)/k, 0.0, 1.0);
      return lerp(d2,d1,h) - k*h*(1.0-h);
    }

    float Saw(float b, float t)
    {
      return S(0.0, b, t) * S(1.0, b, t);
    }
    half4 WaterDrops(half2 uv, float t)
    {
      if (!screen_droplets_has_leaks) return half4(0,0,0,0);
      half2 uv_f = uv;

      half2 UV = uv;
      uv.y -= t * MOVE_BOARD_K;

      half2 gs = half2(GRID_X, 1.0);                    //Grid size (Y Is always eq to 1)
      half2 gs2 = half2(GRID_X, GRID_Y) * 2.0;

      half2 id = floor(uv * gs2);
      uv.y += N(id.x);                                  // Y Shift
      id = floor(uv * gs2);                             // Drop block id

      //constants
      half end_id = floor((N(id.x) + 0.0 - rainEnded * TIME_K * MOVE_BOARD_K) * gs2.y);
      half begin_id = floor((N(id.x) + 0.0 - rainStarted * TIME_K * MOVE_BOARD_K) * gs2.y);

      if (begin_id >= end_id)
      {
        if (id.y < end_id) return half4(0.0, 0.0, 0.0, 0.0);
        if (id.y > begin_id) return half4(0.0, 0.0, 0.0, 0.0);
      }else
      {
        if (id.y < end_id && id.y > begin_id) return half4(0.0, 0.0, 0.0, 0.0);
      }

      float drop_rk = rainForce > 0 ? frac(rainForce) : 1.0;

      half3 rnd = N13(id.x * 78.34 + id.y * 1768.98);   // Drop Random vec
      float rndRow = N(id.x);

      half2 st = frac(uv * gs2) - half2(0.5, 0.0);

      float x = (rnd.x - 0.5);
      float y = UV.y * 10.0;

      float wiggle = sin(y);                            // X wiggling
      x += (abs(x) - 0.5) * wiggle;                     // Apply wiggling with bounds

      x *= (1.0 - MAX_DROP_R * 2.0);
      float dx = abs(x - st.x);

      //return half2(S(MAX_DROP_R,0.0,dx),st.y).xyxy;   // Visual Debug Wiggling lines

      float ti = frac(t * GRID_Y + rnd.z);
      y = Saw(0.15, ti) * (1.0 - SHRINK_Y * 2.0) + SHRINK_Y;

      half2 n = (st - half2(x, y)) * gs.yx * MAX_DROP_RI / drop_rk;
      float d = length(n);

      float maskUp = S(-0.0001, y, st.y);
      float maskDown = S(y + 0.02, y - 0.01, st.y);
      float fadeOut = S(0.0, 0.5, y);

      float mainTrail = S(MAX_TRAIL_R * drop_rk * maskUp, MAX_TRAIL_R * drop_rk * 0.5 * maskUp, dx) *
                        maskUp * maskDown;
      mainTrail *= (0.5 + 0.5 * fadeOut);

      float droplets = frac(UV.y * gs2.x + rndRow) + (st.y - 0.5);
      half2 n_droplets = (st - half2(x, droplets)) /
                        (MAX_DROP_R * drop_rk * 0.8 * maskUp) * half2(1.0, -1.0);
      float d_droplets = length(n_droplets);

      half3 dnr = smoothUnion(half3(d, n), half3(d_droplets+(1-maskUp)+(1-maskDown), n_droplets), 1.0);

      return half4(1. - dnr.x, mainTrail, dnr.yz);
    }

    half3 StaticDrops(half2 uv, float t)
    {
      uv *= 10.0;

      float drop_rk = screen_droplets_intensity;

      half2 id = floor(uv);
      uv = frac(uv) - 0.5;
      float rnd_t = N(id.x * 1237.45 + id.y * 33.654).x;
      float ti = t + rnd_t * 5.23423;
      float to = floor(ti);
      float tf = frac(ti);

      half3 rnd = N13(id.x * 107.45 + id.y * 343.654 + to * 5.0);

      half2 p = (rnd.xy - 0.5) * (1.0 - STATIC_DROP_R * 2.0);

      half2 n = (uv - p) * STATIC_DROP_RI / drop_rk;
      float d = length(n);

      float fade = Saw(0.025, tf);
      float c = 1.0 * frac(rnd.z * 10.0) * fade - d;

      return half3(c, n);
    }

    half4 get_droplets(half2 screen_uv, half2 effect_uv)
    {
      float t = current_time * TIME_K;

      float fadeout_k = S(0.0, 0.2, length(screen_uv - 0.5) - rainFadeout);
      BRANCH
      if(fadeout_k < 0.00001) return half4(0,0,0.5,0.5);

      half4 wd = WaterDrops(effect_uv, t);
      half3 sd = StaticDrops(effect_uv, t);
      //Endtime dryout
      sd.x -= 1.0 - S(0, 1.5, current_time - rainStarted); //fadein
      if (rainStarted < rainEnded)
        sd.x -= 1.0 - S(1.5, 0.0, current_time - rainEnded); //dryout

      half3 dnr = smoothUnion(half3((1 - wd.x), wd.zw), half3((1 - sd.x), sd.yz), 1.0);
      wd.zw = dnr.yz * smoothstep(-0.03, 0.5, (1. - dnr.x));
      wd.x = 1-dnr.x;

      wd.yzw *= fadeout_k;
      wd.x = (wd.x - .1) * 1.1111;

      return wd;
    }

    float2 get_tc_offset(float4 drops)
    {
      float drops_r = length(drops.zw);
      float drops_r2 = drops_r * drops_r;
      float2 drops_n = (30.0 * drops_r2 * drops_r2 - 60.0 * drops_r * drops_r2 + 30.0 * drops_r2) * drops.zw * -0.1;
      return drops_n;
    }

    void paint_waterdroplets(inout float3 col, float4 drops)
    {
      half3 n3 = half3(drops.zw, sqrt(abs(1.0 - dot(drops.zw, drops.zw))));
      half diff_c = dot(n3.yz, half2(-0.447, 0.894));
      half3 diff = half3(diff_c, diff_c, diff_c);
      float3 paint = 1.0 - diff + col * diff;
      if (drops.x > 0.0) col = lerp(col, 0.8 * paint,  pow2(0.063));
      //Paint water drops trails
      col = lerp(col, 0.8 * paint,  pow2(drops.y * 0.3));
    }

    half4 screen_droplets_ps(VsOutput input) : SV_Target
    {
      float2 screenUV = input.texcoord.xy;
      float4 drops = get_droplets(screenUV, input.texcoord.zw);

      float blendingStrength = 0, mipBias = 1;
      BRANCH
      if (drops.x > 0)
      {
        blendingStrength = 1; mipBias = 0;
        screenUV += get_tc_offset(drops);
      }
      float3 frame = frame_tex.SampleBias(frame_tex_samplerstate, screenUV, mipBias).xyz;
      BRANCH
      if (drops.x > 0 || drops.y > 0)
        paint_waterdroplets(frame, drops);
      return float4(frame, blendingStrength);
    }
  }

  compile("target_vs", "screen_droplets_vs");
  compile("target_ps", "screen_droplets_ps");
}
