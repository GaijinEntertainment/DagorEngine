// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// Volumetric clouds. It performs level of detail (LOD) for faster rendering

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

float snoiseTex(in float3 x)
{
  float3 i = floor(x);
  float3 f = frac(x);
  f = f * f * (3.0 - 2.0 * f);

  float2 uv = (i.xy + float2(37.0, 17.0) * i.z) + f.xy;
  float2 rg = tex2Dlod(noise_128_tex_hash, float4(uv*1./128.0 + 0.5/128.0, 0.0, 0.0)).yx;

  return lerp(rg.x, rg.y, f.z);
}

float noise( in vec3 p )
{
  //return tex3Dlod(perlin_noise3d, float4(p*0.33,0));
  return snoiseTex(p*2.1);
}
float snoise( in vec3 p )
{
  //return snoiseAlu(p);
  return snoiseTex(p);
}

float map5( in vec3 p )
{
	vec3 q = p - vec3(0.0,0.1,1.0)*iTime;
	float f;
    f  = 0.50000*snoise( q ); q = q*2.02;
    f += 0.25000*noise( q ); q = q*2.03;
    f += 0.12500*noise( q ); q = q*2.01;
    f += 0.06250*noise( q ); q = q*2.02;
    f += 0.03125*noise( q );
	return clamp( 1.5 - p.y - 2.0 + 1.75*f, 0.0, 1.0 );
}

float map4( in vec3 p )
{
	vec3 q = p - vec3(0.0,0.1,1.0)*iTime;
	float f;
    f  = 0.50000*snoise( q ); q = q*2.02;
    f += 0.25000*noise( q ); q = q*2.03;
    f += 0.12500*noise( q ); q = q*2.01;
    f += 0.06250*noise( q );
	return clamp( 1.5 - p.y - 2.0 + 1.75*f, 0.0, 1.0 );
}
float map3( in vec3 p )
{
	vec3 q = p - vec3(0.0,0.1,1.0)*iTime;
	float f;
    f  = 0.50000*snoise( q ); q = q*2.02;
    f += 0.25000*noise( q ); q = q*2.03;
    f += 0.12500*noise( q );
	return clamp( 1.5 - p.y - 2.0 + 1.75*f, 0.0, 1.0 );
}
float map2( in vec3 p )
{
	vec3 q = p - vec3(0.0,0.1,1.0)*iTime;
	float f;
    f  = 0.50000*snoise( q ); q = q*2.02;
    f += 0.25000*noise( q );;
	return clamp( 1.5 - p.y - 2.0 + 1.75*f, 0.0, 1.0 );
}

static vec3 sundir = normalize( vec3(-1.0,0.0,-1.0) );

vec4 integrate( in vec4 sum, in float dif, in float den, in vec3 bgcol, in float t )
{
    // lighting
    vec3 lin = vec3(0.65,0.7,0.75)*1.4 + vec3(1.0, 0.6, 0.3)*dif;        
    vec4 col = vec4( mix( vec3(1.0,0.95,0.8), vec3(0.25,0.3,0.35), den ), den );
    col.xyz *= lin;
    col.xyz = mix( col.xyz, bgcol, 1.0-exp(-0.003*t*t) );
    // front to back blending    
    col.a *= 0.4;
    col.rgb *= col.a;
    return sum + col*(1.0-sum.a);
}

#define MARCH(STEPS,MAPLOD) \
for(i=0; i<STEPS; i++) { vec3  pos = ro + t*rd; if( pos.y<-3.0 || pos.y>2.0 || sum.a > 0.99 ) break; float den = MAPLOD( pos ); if( den>0.01 ) { float dif =  clamp((den - MAPLOD(pos+0.3*sundir))/0.6, 0.0, 1.0 ); sum = integrate( sum, dif, den, bgcol, t ); } t += max(0.06,0.05*t); }\
BRANCH \
if (sum.w>0.99)\
  return saturate(sum);

vec4 raymarch( in vec3 ro, in vec3 rd, in vec3 bgcol, in ivec2 px )
{
	vec4 sum = 0.0;

	float t = 0.0;//0.05*texelFetch( iChannel0, px&255, 0 ).x;
    int i = 0;
    MARCH(20,map5);
    MARCH(20,map4);
    //MARCH(20,map3);
    MARCH(80,map2);
    return clamp( sum, 0.0, 1.0 );
}

mat3 setCamera( in vec3 ro, in vec3 ta, float cr )
{
	vec3 cw = normalize(ta-ro);
	vec3 cp = vec3(sin(cr), cos(cr),0.0);
	vec3 cu = normalize( cross(cw,cp) );
	vec3 cv = normalize( cross(cu,cw) );
    return mat3( cu, cv, cw );
}

vec4 render( in vec3 ro, in vec3 rd, in ivec2 px )
{
    // background sky     
	float sun = clamp( dot(sundir,rd), 0.0, 1.0 );
	vec3 col = vec3(0.6,0.71,0.75) - rd.y*0.2*vec3(1.0,0.5,1.0) + 0.15*0.5;
	col += 0.2*vec3(1.0,.6,0.1)*pow( sun, 8.0 );

    // clouds    
    vec4 res = raymarch( ro, rd, col, px );
    col = col*(1.0-res.w) + res.xyz;
    
    // sun glare    
	col += 0.2*vec3(1.0,0.4,0.2)*pow( sun, 3.0 );

    return vec4( col, 1.0 );
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 p = (-iResolution.xy + 2.0*fragCoord.xy)/ iResolution.y;
    p.y = -p.y;

    vec2 m = iMouse.xy/iResolution.xy;
    
    // camera
    vec3 ro = 4.0*normalize(vec3(sin(3.0*m.x), 0.1+0.4*m.y, cos(3.0*m.x)));
	vec3 ta = vec3(0.0, -0.5, 0.0);
    mat3 ca = setCamera( ro, ta, 0.0 );
    // ray
    vec3 rd = mul_m3(ca, normalize( vec3(p.xy,1.5)));
    
    fragColor = render( ro, rd, ivec2(fragCoord-0.5) );
}

void mainVR( out vec4 fragColor, in vec2 fragCoord, in vec3 fragRayOri, in vec3 fragRayDir )
{
    fragColor = render( fragRayOri, fragRayDir, ivec2(fragCoord-0.5) );
}
