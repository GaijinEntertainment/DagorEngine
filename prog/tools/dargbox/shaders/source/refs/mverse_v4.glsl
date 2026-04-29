// PS3 XMB Wave Knot v2 — ShaderToy Version
// Paste into shadertoy.com Image tab
//
// ── Customize these ──
#define KNOT_POS   vec2(0.88, 0.60) // Knot center (UV 0-1)
#define FLOW_DIR   1.5708            // Radians: 0=horizontal, 1.5708=vertical (90°)
#define NUM_WAVES  14.0              // Ribbon count
#define SPEED      0.37              // Animation speed
#define SPARK_INT  0.0               // Spark intensity 0-1 (0 = off, saves perf)
#define WAVE_WIDTH 0.64              // Band width 0.05-1.0 (lower = narrower)
#define WAVE_HUE   0.07              // Wave color hue 0-1 (0.07≈orange, 0.0≈red, 0.15≈gold, 0.55≈cyan)
#define BG_HUE     1.0               // Background hue (0-1, 1.0≈red, 0.61≈blue, 0.08≈orange)
#define BG_AMT     0.39              // Background gradient strength 0-1

#define PI 3.14159265359

float hash11(float p){
  p = fract(p * 0.1031);
  p *= p + 33.33;
  p *= p + p;
  return fract(p);
}
float hash21(vec2 p){
  vec3 p3 = fract(vec3(p.xyx) * 0.1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.x + p3.y) * p3.z);
}
vec3 hsv2rgb(vec3 c){
  vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord){
  vec2 uv = fragCoord / iResolution.xy;
  float aspect = iResolution.x / iResolution.y;

  // ── Background gradient ──
  vec2 bgUV = uv - KNOT_POS;
  bgUV.x *= aspect;
  float bgDist = length(bgUV);
  vec3 bgDark  = hsv2rgb(vec3(BG_HUE, 0.6, 0.02)) * BG_AMT;
  vec3 bgMid   = hsv2rgb(vec3(BG_HUE, 0.5, 0.07)) * BG_AMT;
  vec3 bgLight = hsv2rgb(vec3(BG_HUE + 0.05, 0.4, 0.12)) * BG_AMT;
  vec3 bg = mix(bgMid, bgDark, smoothstep(0.0, 0.8, bgDist));
  vec2 bg2UV = uv - vec2(KNOT_POS.x + 0.2, KNOT_POS.y - 0.15);
  bg2UV.x *= aspect;
  bg += bgLight * exp(-dot(bg2UV, bg2UV) * 3.0) * 0.5;
  float neb = hash21(floor(fragCoord * 0.15)) * 0.3 + hash21(floor(fragCoord * 0.05)) * 0.7;
  bg += hsv2rgb(vec3(BG_HUE + 0.1, 0.3, 0.015 * BG_AMT)) * neb;
  vec3 col = bg;

  // ── Coords ──
  vec2 p = uv - 0.5;  p.x *= aspect;
  vec2 knot = KNOT_POS - 0.5;  knot.x *= aspect;
  float ca = cos(FLOW_DIR), sa = sin(FLOW_DIR);
  mat2 rot = mat2(ca, -sa, sa, ca);
  vec2 rp = rot * p;  vec2 rk = rot * knot;
  float along = rp.x, perp = rp.y, kAlong = rk.x, kPerp = rk.y;
  float t = iTime * SPEED;

  // ── Ribbons ──
  for(float i = 0.0; i < 40.0; i++){
    if(i >= NUM_WAVES) break;
    float id = i / NUM_WAVES;
    float s1 = hash11(i*17.31), s2 = hash11(i*23.71+7.0), s3 = hash11(i*11.13+3.0);
    float spread = (id - 0.5)*2.0 + (s1 - 0.5)*0.5;
    float dAlong = along - kAlong;
    float pinch = 1.0 - exp(-2.2*dAlong*dAlong);
    float tightPinch = exp(-10.0*dAlong*dAlong);

    float ribbonBase = kPerp + spread*(0.06*WAVE_WIDTH + 0.35*WAVE_WIDTH*pinch);

    float phase = along*(3.0+s1*2.5) - t*(0.25+s2*0.35);
    float wave = sin(phase)*0.035*(0.4+pinch)*WAVE_WIDTH
               + sin(phase*2.1+s1*5.0-t*0.18)*0.018*(0.3+pinch)*WAVE_WIDTH
               + sin(phase*0.6+s3*4.0+t*0.12)*0.022*pinch*WAVE_WIDTH
               + sin(along*9.0+i*PI/NUM_WAVES+t*0.4)*0.065*tightPinch*WAVE_WIDTH;
    float ribbonY = ribbonBase + wave;
    float d = abs(perp - ribbonY);

    float coreW = 0.0012 + 0.0006*tightPinch;
    float core = smoothstep(coreW, coreW*0.2, d);
    float haloR = 0.003*(1.0+0.8*tightPinch);
    float halo = exp(-d*d/(haloR*haloR))*0.35;

    vec3 rCol = hsv2rgb(vec3(WAVE_HUE + s1*0.06 - 0.03, 0.85 - 0.15*tightPinch, 1.0));
    float kB = 1.0 + 1.2*tightPinch;
    float dots = pow(max(sin(along*(40.0+s1*30.0)-t*(3.0+s2*4.0)),0.0),8.0);
    float dotI = 0.25 + 0.75*dots;

    col += rCol*(core*1.2+halo)*kB*dotI;

    // Sparks (skipped when SPARK_INT ≈ 0)
    #if SPARK_INT > 0.01
    {
      float sparkA = 0.0;
      for(float si=0.0; si<5.0; si++){
        float ss1 = hash11(i*100.0+si*37.0+5.0);
        float ss2 = hash11(i*100.0+si*53.0+11.0);
        float sPA = along-(t*(0.15+ss1*0.35)+ss2*10.0);
        float per = 0.8+ss1*1.2;
        float sL = mod(sPA,per)-per*0.5;
        float sD2 = sL*sL + d*d;
        float sR = 0.0006+0.0004*SPARK_INT;
        sparkA += (exp(-sD2/(sR*sR))*2.5 + exp(-(pow(sL+0.012+0.008*ss1,2.0)*8.0+d*d*400.0))*step(0.0,sL+0.012)*0.8)*(0.5+0.5*ss1);
      }
      col += mix(rCol, vec3(1.0,0.92,0.7), 0.55) * sparkA * SPARK_INT;
    }
    #endif

    // Nodes
    float nF = 3.5+s1*2.5;
    float nP = along*nF - t*(0.08+s2*0.12);
    float nOn = smoothstep(0.94,0.97,sin(nP));
    float nS = smoothstep(0.008,0.0024,abs(mod(along/(1.0/nF),1.0)-0.5))*smoothstep(0.003,0.0009,d);
    col += rCol*0.75*nOn*nS;
  }

  // ── Vignette + tonemap ──
  col *= 1.0-0.45*pow(length(uv-0.5)*1.4,2.0);
  col = 1.0-exp(-col*2.0);
  col.b += 0.008*(1.0-col.b);

  fragColor = vec4(col,1.0);
}
