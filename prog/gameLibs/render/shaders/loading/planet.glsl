/*
* created by dmytro rubalskyi (ruba)
*
* all values are in kilometers
*
* references: 
*
* http://nishitalab.org/user/nis/cdrom/sig93_nis.pdf
* 
* http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter16.html
*
* http://www-evasion.imag.fr/people/Eric.Bruneton/
*
* https://software.intel.com/en-us/blogs/2013/09/19/otdoor-light-scattering-sample-update
*
*/

#define M_MAX 1e9
#define KEY_M (float(77)+0.5)/256.0

static const float M_PI = 3.1415926535;
static const float M_4PI = 4.0 * M_PI;

///////////////////////////////////////
// planet
static const float earthRadius 	= 6360.0;
static const float atmoHeight 		= 60.0;
static const float atmoRadius 		= earthRadius + atmoHeight;
static const vec3 earthCenter 		= vec3(0.0, 0.0, 0.0);

///////////////////////////////////////
// sun
static const float distanceToSun = 1.496e8;
static const float sunRadius = 2.0 * 109.0 * earthRadius;
static const float sunIntensity = 10.0;

///////////////////////////////////////
// atmosphere
static const vec3 betaR 			= vec3(5.8e-4, 1.35e-3, 3.31e-3);
static const vec3 betaM 			= vec3(4.0e-3, 4.0e-3, 4.0e-3);

static const vec3 M_4PIbetaR	 	= M_4PI * betaR;
static const vec3 M_4PIbetaM 		= M_4PI * betaM;

static const float heightScaleRayleight = 6.0;
static const float heightScaleMie = 1.2;
static const float g = -0.76;

static const float NUM_DENSITY_SAMPLES = 8.0;
static const float NUM_VIEW_SAMPLES = 8.0;
static const int 	INT_NUM_DENSITY_SAMPLES = int(NUM_DENSITY_SAMPLES);
static const int	INT_NUM_VIEW_SAMPLES = int(NUM_VIEW_SAMPLES);

///////////////////////////////////////
// ray - sphere intersection
vec2 iSphere(vec3 ro, vec3 rd, vec4 sph)
{
    vec3 tmp = ro - sph.xyz;

    float b = dot(rd, tmp);
    float c = dot(tmp, tmp) - sph.w * sph.w;
    
    float disc = b * b - c;
    
    if(disc < 0.0) return vec2(-M_MAX, -M_MAX);
    
    float disc_sqrt = sqrt(disc);
	
    float t0 = -b - disc_sqrt;
    float t1 = -b + disc_sqrt;
    
    return vec2(t0, t1);
}

///////////////////////////////////////
// Henyey-Greenstein phase function
float phase(float nu, float g)
{
	return (3.0 * (1.0 - g * g) * (1.0 + nu * nu)) / (2.0 * (2.0 + g * g) * pow(1.0 + g * g - 2.0 * g * nu, 1.5));
}

///////////////////////////////////////
// density integral calculation from p0 to p1 
// for mie and rayleight
vec2 densityOverPath(vec3 p0, vec3 p1, vec2 prescaler)
{
    float l = length(p1 - p0);
    vec3  v = (p1 - p0) / l;
    
    l /= NUM_DENSITY_SAMPLES;
    
    vec2 density = 0.0;
    float t = 0.0;
    
	for(int i = 0; i < INT_NUM_DENSITY_SAMPLES; i++)
    {
        vec3 sp = p0 + v * (t + 0.5 * l);
        vec2 h = length(sp) - earthRadius;
        density += exp(-h / prescaler);
        
        t += l;
    }
    
    return l * density;
}

///////////////////////////////////////
// inscatter integral calculation
vec4 inscatter(vec3 cam, vec3 v, vec3 sun)
{    
    vec4 atmoSphere 	= vec4(earthCenter, atmoRadius);
    vec4 earthSphere 	= vec4(earthCenter, earthRadius);
        
	vec2 t0 = iSphere(cam, v, atmoSphere);
    vec2 t1 = iSphere(cam, v, earthSphere);
   
    bool bNoPlanetIntersection = t1.x < 0.0 && t1.y < 0.0;
    
    float farPoint = bNoPlanetIntersection ? t0.y : t1.x;
    float nearPoint = t0.x > 0.0 ? t0.x : 0.0;
    
    float l = (farPoint - nearPoint) / NUM_VIEW_SAMPLES;
	cam += nearPoint * v;  
    
    float t = 0.0;

    vec3 rayleight = 0.0;
	vec3 mie = 0.0;
    
    vec2 prescalers = vec2(heightScaleRayleight, heightScaleMie);
    
    vec2 densityPointToCam = 0.0;
    
    for(int i = 0; i < INT_NUM_VIEW_SAMPLES; i++)
    {
        vec3 sp = cam + v * (t + 0.5 * l);
        float tc = iSphere(sp, sun, vec4(earthCenter, atmoRadius)).y;
        
        vec3 pc = sp + tc * sun;
        
        vec2 densitySPCam = densityOverPath(sp, cam, prescalers);
        vec2 densities = densityOverPath(sp, pc, prescalers) + densitySPCam;
        
        vec2 h = length(sp) - earthRadius;
        vec2 expRM = exp(-h / prescalers);
        
        rayleight 	+= expRM.x * exp( -M_4PIbetaR * densities.x );
		mie 		+= expRM.y * exp( -M_4PIbetaM * densities.y );

		densityPointToCam += densitySPCam;
        
        t += l;
    }
    
	rayleight *= l;
    mie *= l;
    
    vec3 extinction = exp( - (M_4PIbetaR * densityPointToCam.x + M_4PIbetaM * densityPointToCam.y));
    
    float nu = dot(sun, -v);
    
    vec3 inscatter_ = sunIntensity * (betaM * mie * phase(nu, g) + betaR * phase(nu, 0.0) * rayleight);
    return vec4(inscatter_, extinction.r * float(bNoPlanetIntersection));
}

///////////////////////////////////////
// rotation around axis Y
vec3 rotate_y(vec3 v, float angle)
{
	vec3 vo = v; float cosa = cos(angle); float sina = sin(angle);
	v.y = cosa*vo.y - sina*vo.z;
	v.z = sina*vo.y + cosa*vo.z;
	return v;
}

///////////////////////////////////////
// noise from iq
float noise( in vec3 x )
{
    vec3 p = floor(x);
    vec3 f = fract(x);
	f = f*f*(3.0-2.0*f);
	
	vec2 uv = (p.xy+vec2(37.0,17.0) * p.z) + f.xy;
	vec2 rg = tex2Dlod( noise_64_tex_l8, float4((uv + 0.5)/256.0, 0, -100.0) ).x;
    rg.y = tex2Dlod( noise_64_tex_l8, float4((uv*float2(3.1,1.7) +71.1+ 0.5)/256.0, 0, -101.0) ).x;
	return mix( rg.x, rg.y, f.z );
}

float hash( float n ) { return fract(sin(n)*123.456789); }
float hash2(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); }

vec2 rotate( in vec2 uv, float a)
{
    float c = cos( a );
    float s = sin( a );
    return vec2( c * uv.x - s * uv.y, s * uv.x + c * uv.y );
}

float snoise( in vec3 p )
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

float doMainStar( in vec2 uv, in vec2 sp)
{
    float t = atan2( uv.x - sp.x, uv.y - sp.y );
    float n = 2.0 + snoise( vec3( 10.0 * t, iGlobalTime, 0.0 ) );
    float d = length( uv - sp ) * 25.0;
    return ( ( 1.0 + n ) / ( d * d * d ) );
}

float starplane(vec3 dir) { 
    // Project to a cube-map plane and scale with the resolution of the display
    vec2 basePos = iResolution.x * dir.xy / max(1e-3, abs(dir.z));
         
	const float largeStarSizePixels = 20.0;
    
	float color = 0.0;
	vec2 pos = floor(basePos / largeStarSizePixels);
	//float starValue = hash2(pos);
    
    // Stabilize stars under motion by locking to a grid
    basePos = floor(basePos);

    if (all(hash2(basePos.xy) > 0.999)) {
        float r = hash2(basePos.xy * 0.5);
        color += r;// * (0.3 * sin(time * (r * 5.0) + r) + 0.7) * 1.5;
    }
	
    // Weight by the z-plane
    return color * abs(dir.z);
}


float starbox(vec3 dir) {
	return starplane(dir.xyz) + starplane(dir.yzx) + starplane(dir.zxy);
}    

static const vec3 world_up = vec3( 0.0, 0.0, 1.0 );
struct CameraInfo
{
    vec3 pos;
    vec3 dir;
    mat3 m;
    mat3 mInv;
};
CameraInfo doCamera( in vec3 pos, in vec3 dir )
{
    CameraInfo ci;
    
    vec3 ww = dir;
    vec3 uu = normalize( cross( ww, world_up ) );
    vec3 vv = normalize( cross( uu, ww ) );
    mat3 m = mat3( uu, vv, ww );
    mat3 mInv = mat3( uu.x, vv.x, ww.x,
                      uu.y, vv.y, ww.y,
                      uu.z, vv.z, ww.z );
    
    ci.pos = pos;
    ci.dir = dir;
    ci.m = m;
    ci.mInv = mInv;
    
    return ci;
}

float doFlare( in vec2 uv, in vec2 dir, float s )
{
    float d = length( uv - dot( uv, dir ) * dir );
    float f = 0.0;
    f += max( pow( 1.0 - d, 128.0 ) * ( 1.0   * s - length( uv ) ), 0.0 );
    f += max( pow( 1.0 - d,  64.0 ) * ( 0.5   * s - length( uv ) ), 0.0 );
    f += max( pow( 1.0 - d,  32.0 ) * ( 0.25  * s - length( uv ) ), 0.0 );
    f += max( pow( 1.0 - d,  16.0 ) * ( 0.125 * s - length( uv ) ), 0.0 );
    return f;
}
static const vec3 sun_color = vec3(1,0.9,0.85);

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xy - 0.5;
    uv.x *= iResolution.x / iResolution.y;
    uv.y = -uv.y;

    vec2 sc = 2.0 * fragCoord.xy / iResolution.xy - 1.0;
    sc.x *= iResolution.x / iResolution.y;
    sc.y = -sc.y;
    
    vec3 ro = 0.0;
    
   	ro = vec3(-1000.0, earthRadius + 10.1, 1000.0 * abs(cos(iGlobalTime / 2.0)));
    vec3 rd;
    float baseTime;
    baseTime = iGlobalTime;
    float time = 1. - 1./(1.0 + 2.*(baseTime*0.07 + 0.07*pow3(baseTime/1.4)));
    //time = abs(cos(iGlobalTime / 2.0));
   	ro = vec3(0, 0, earthRadius + mix(4000.1, 0.05, time));
    //rd = normalize(rotate_y(vec3(sc, -1), M_PI/2.*time));
    //rd = normalize(rotate_y(vec3(sc, mix(-2., -2.5, time)), M_PI/2.*(0.1+time*0.7)));
    rd = normalize(rotate_y(vec3(sc, -2.), M_PI/2.*(0.6+time*0.2)));
    vec3 sun = normalize(vec3(0.8, 2.0, -0.05));
    
    vec4 col = inscatter(ro, rd, sun);
    
    vec3 sunPos = sun * distanceToSun;
    CameraInfo ci = doCamera( ro, rd );

    vec3 cp = normalize(mul((sunPos - ci.pos), ci.mInv));
    vec2 sp = cp.xy / cp.z;
    if( cp.z > 0. )
    {
        col.xyz += doMainStar( uv, sp )*sun_color*pow(col.a, 0.1);
    }
    
    vec3 stars = noise(rd * iResolution.y * 0.75);
    stars += noise(rd * iResolution.y * 0.5);
    stars += noise(rd * iResolution.y * 0.25);
    stars += noise(rd * iResolution.y * 0.1);
    stars = clamp(stars, 0.0, 1.0);
    stars = (1.0 - stars);
    
    col.xyz = col.xyz + stars * col.a;
    //col.xyz = pow(col.xyz, 1.0 / 2.2);
    
    // lens flare
    fragColor = col;
}