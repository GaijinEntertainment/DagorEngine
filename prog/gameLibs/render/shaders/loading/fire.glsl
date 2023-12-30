float hash( float n ) { return fract(sin(n*12.9898)*1.456789); }

float snoiseAlu( in vec3 p )
{
    vec3 fl = floor( p );
    vec3 fr = fract( p );
    fr = fr * fr * ( 3.0 - 2.0 * fr );

    float n = fl.x + fl.y * 157.0 + 113.0 * fl.z;
    return mix( mix( mix( hash( n +   0.0), hash( n +   1.0 ), fr.x ),
                     mix( hash( n + 157.0), hash( n + 158.0 ), fr.x ), fr.y ),
                mix( mix( hash( n + 113.0), hash( n + 114.0 ), fr.x ),
                     mix( hash( n + 270.0), hash( n + 271.0 ), fr.x ), fr.y ), fr.z );
}

float snoiseTex( in float3 x )
{
  float3 i = floor(x);
  float3 f = frac(x);
  f = f * f * (3.0 - 2.0 * f);

  float2 uv = (i.xy + float2(37.0, 17.0) * i.z) + f.xy;
  float2 rg = tex2Dlod(noise_128_tex_hash, float4(uv*1./128.0 + 0.5/128.0, 0.0, 0.0)).yx;

  return lerp(rg.x, rg.y, f.z);
}

float noise( in vec2 p )
{
  //return snoiseAlu(vec3(p.x, p.y, 0.0));
  return snoiseTex(vec3(p.x, p.y, 0.0));
}

float noise( in vec3 p )
{
  //return snoiseAlu(p);
  return snoiseTex(p);
}

float intHash1(in ivec2 x)
{
  // The MIT License
  // Copyright © 2017 Inigo Quilez
  // Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  ivec2 q = 1103515245U * ( (x>>1U) ^ (x.yx   ) );
  uint  n = 1103515245U * ( (q.x  ) ^ (q.y>>3U) );
  return float(n) * (1.0/float(0xffffffffU));
}

vec3 flameCol(in float emission)
{
  float e3 = pow3(emission);
  return 1.5*vec3(emission, e3, pow2(e3));
}

float sdEgg( in vec2 p, in float ra, in float rb )
{
  const float k = sqrt(3.0);
  p.x = abs(p.x);
  float r = ra - rb;
  return ((p.y<0.0)       ? length(vec2(p.x,  p.y    )) - r :
          (k*(p.x+r)<p.y) ? length(vec2(p.x,  p.y-k*r)) :
                            length(vec2(p.x+r,p.y    )) - 2.0*r) - rb;
}

float noise_octaves(in vec3 uv)
{
  const mat3 rot1 = mat3(-0.37, 0.36, 0.85,-0.14,-0.93, 0.34,0.92, 0.01,0.4);
  const mat3 rot2 = mat3(-0.55,-0.39, 0.74, 0.33,-0.91,-0.24,0.77, 0.12,0.63);
  const mat3 rot3 = mat3(-0.71, 0.52,-0.47,-0.08,-0.72,-0.68,-0.7,-0.45,0.56);

  vec4 octaves = vec4(0.5000, 0.2500, 0.1250, 0.0625);

  float f = 0.0;

  f += octaves[0]*noise( 1.0*mul(uv, rot1) );
  f += octaves[1]*noise( 2.0*mul(uv, rot2) );
  f += octaves[2]*noise( 4.0*mul(uv, rot3) );
  f += octaves[3]*noise( 8.0*uv );
  return 0.5 + 0.5*f;
}

vec2 displacement(in vec3 uv)
{
  return vec2(noise_octaves(uv), noise_octaves(uv + vec3(213.5, 12.2, 155.0))) * 2.0 - 1.0;
}

float variance(float hash, in vec2 minMaxVariance)
{
  return minMaxVariance.x + (minMaxVariance.y - minMaxVariance.x) * (hash + 1.0) * 0.5f;
}

vec3 particles(in vec2 uv, in float scale, in float height)
{
  const vec2 sizeVar = vec2(0.03, 0.05);
  const vec2 animationSpeedVar = vec2(0.5, 4.0);
  const vec2 animationStrengthVar = vec2(0.1, 0.5);

  ivec2 ceil_no = ivec2(floor(uv));
  vec2 ceil_uv = fract(uv) - 0.5;
  float ceilHash = intHash1(ceil_no) ;

  float alpha = smoothstep(0.0, 0.6, noise(vec2(ceilHash* 1124.0, iTime * 0.2)) -0.2);

  float speed = variance(ceilHash, animationSpeedVar);
  float rot = intHash1(ceil_no) + iTime * speed;
  ceil_uv += vec2(sin(rot), cos(rot)) * variance(intHash1(ceil_no + 1), animationStrengthVar);
  float dist = length(vec2(ceil_uv.x  * 1.3, ceil_uv.y));

  float heightScale = 0.5 + smoothstep(0.4, 0.0, height) * 0.5;
  alpha *= heightScale;
  float particleSize = alpha * scale * variance(intHash1(ceil_no + 6), sizeVar);

  vec3 particleCol = flameCol(alpha);
  vec3 color = (1.0 - smoothstep(0.0, particleSize, dist)) * particleCol;
  color += pow2(1.0 - smoothstep(0.0, particleSize * 2.0, dist)) * particleCol * 0.6;

  return color;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
  fragCoord.y = iResolution.y - fragCoord.y;
  // Normalized pixel coordinates (from 0 to 1)
  vec2 uv = fragCoord/iResolution.xy;
  vec2 uv_n = fragCoord/iResolution.x;
  uv_n = uv_n * 2.0 - 1.0;

  vec3 col = vec3(0,0,0);

  vec2 cellSizeCoff = vec2(13.f, 1.2);
  vec2 offsetCoff = vec2(17.0, 3.1232);
  vec2 scrollSpeedCoff = vec2(2.5, 1.7);
  vec2 alphaCoff = vec2(1.0, 0.7);
  vec2 scaleCoff = vec2(1.0, 1.4);
  vec2 windCoff = vec2(6.0, 1.5);

  float wind = noise(vec2(uv.x * 0.1, iTime*0.05)) * (0.5+uv.y);
  float pulsation = noise(vec2(uv.x*0.2, iTime*0.7)) + 0.5 ;
  float attraction = mix(0.5,1.0, smoothstep(1.0, 0.2, uv.y) - smoothstep(0.4, 0.0, uv.y) * 0.2  );

  {
    vec2 uv2 = uv_n.xy;
    float wideFallof = 1.0 - length(uv * vec2(1.0,2.0)+ vec2(-0.5, -0.3));
    float falloff = 1.0 - length(uv * vec2(1.0,1.0)+ vec2(-0.5, -0.3)) * 2.0;
    falloff += smoothstep(0.25, 0.0, uv.y) *0.4;
    falloff *= smoothstep(0.3, 0.0, uv.y);
    falloff = max(falloff, 0.0);

    uv2.x += wind * 0.6;
    uv2.x /= attraction;
    uv2 *= 20.0;
    uv2.y -= iTime * 5.0;

    vec2 disp = displacement(vec3(uv2 * 2.0 + vec2(12,42), iTime));
    disp = vec2(pow2(disp.x), pow2(disp.y));

    float noise = noise_octaves(vec3(uv2.xy + disp, iTime *0.45)) * 1.20;
    float flame_e = pow(max(noise,0.0),0.4 + uv.y * 5.6 + abs(uv_n.x) * 1.0) - .05;

    col = flameCol((pow2(pow3(flame_e))+ mix(0.1,0.25,pulsation)) * falloff + mix(0.18,0.25,pulsation) * wideFallof );
  }

  float falloff_2 = 1.0 - sdEgg(uv_n * vec2(0.5,1.0) + vec2(0.0,0.7), 0.3,0.1);

  for(int i = 0; i < 5; i++)
  {
    vec2 uv2 = uv_n * cellSizeCoff.x;
    uv2.x += wind * windCoff.x;
    uv2.x /= attraction;

    col += particles(uv2 - vec2(offsetCoff.x , iTime * scrollSpeedCoff.x),
                      scaleCoff.x * falloff_2, uv.y) * alphaCoff.x * falloff_2;

    scrollSpeedCoff.x *= scrollSpeedCoff.y;
    alphaCoff.x *= alphaCoff.y;
    scaleCoff.x *= scaleCoff.y;
    offsetCoff.x *= offsetCoff.y;
    windCoff.x *= windCoff.y;
    cellSizeCoff *= cellSizeCoff.y;
  }

  fragColor = vec4(col,1.0);
}