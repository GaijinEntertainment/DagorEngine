// "Protoplanetary disk" by Duke
// https://www.shadertoy.com/view/MdtGRl
//-------------------------------------------------------------------------------------
// Based on "Dusty nebula 1" (https://www.shadertoy.com/view/4lSXD1) 
// and Shane's "Cheap Cloud Flythrough" (https://www.shadertoy.com/view/Xsc3R4) shaders
// Some ideas came from other shaders from this wonderful site
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0
//-------------------------------------------------------------------------------------

//#define MOUSE_CAMERA_CONTROL

// comment this string to see fluffy clouds
#define DENSE_DUST

//#define DITHERING
#define BACKGROUND

//-------------------
#define pi 3.14159265
#define R(p, a) p=cos(a)*p+sin(a)*vec2(p.y, -p.x)

float Noise2d( in vec2 x )
{
    float xhash = cos( x.x * 37.0 );
    float yhash = cos( x.y * 57.0 );
    return fract( 415.92653 * ( xhash + yhash ) );
}

// Convert Noise2d() into a "star field" by stomping everthing below fThreshhold to zero.
float NoisyStarField( in vec2 vSamplePos, float fThreshhold )
{
    float StarVal = Noise2d( vSamplePos );
    if ( StarVal >= fThreshhold )
        StarVal = pow( (StarVal - fThreshhold)/(1.0 - fThreshhold), 6.0 );
    else
        StarVal = 0.0;
    return StarVal;
}

// Stabilize NoisyStarField() by only sampling at integer values.
float StableStarField( in vec2 vSamplePos, float fThreshhold )
{
    // Linear interpolation between four samples.
    // Note: This approach has some visual artifacts.
    // There must be a better way to "anti alias" the star field.
    float fractX = fract( vSamplePos.x );
    float fractY = fract( vSamplePos.y );
    vec2 floorSample = floor( vSamplePos );    
    float v1 = NoisyStarField( floorSample, fThreshhold );
    float v2 = NoisyStarField( floorSample + vec2( 0.0, 1.0 ), fThreshhold );
    float v3 = NoisyStarField( floorSample + vec2( 1.0, 0.0 ), fThreshhold );
    float v4 = NoisyStarField( floorSample + vec2( 1.0, 1.0 ), fThreshhold );

    float StarVal =   v1 * ( 1.0 - fractX ) * ( 1.0 - fractY )
        			+ v2 * ( 1.0 - fractX ) * fractY
        			+ v3 * fractX * ( 1.0 - fractY )
        			+ v4 * fractX * fractY;
	return StarVal;
}
mat2 Spin(float angle)
{
	return mat2(cos(angle),-sin(angle),sin(angle),cos(angle));
}

// iq's noise
float pn( in vec3 x )
{
    vec3 p = floor(x);
    vec3 f = fract(x);
	f = f*f*(3.0-2.0*f);
	vec2 uv = (p.xy+vec2(37.0,17.0)*p.z) + f.xy;
	vec2 rg = float2(sampleNoise0((uv*float2(7.11,3.1) +71.13+ 0.5)/256.0), sampleNoise0((uv + 0.5)/256.0));
	return -1.0+2.4*mix( rg.x, rg.y, f.z );
}

float fpn(vec3 p)
{
   return pn(p*.06125)*.5 + pn(p*.125)*.25 + pn(p*.25)*.125;// + pn(p*.5)*.625;
}

float rand(vec2 co)
{
	return fract(sin(dot(co*0.123,vec2(12.9898,78.233))) * 43758.5453);
}

float Ring(vec3 p)
{
  vec2 q = vec2(length(p.xy)-2.3,p.z);
  return length(q)-0.01;
}

float length2( vec2 p )
{
	return sqrt( p.x*p.x + p.y*p.y );
}

float length8( vec2 p )
{
	p = p*p; p = p*p; p = p*p;
	return pow( p.x + p.y, 1.0/8.0 );
}


float Disk( vec3 p, vec3 t )
{
    vec2 q = vec2(length2(p.xy)-t.x,p.z*0.5);
    return max(length8(q)-t.y, abs(p.z) - t.z);
}

float smin( float a, float b, float k )
{
	float h = clamp( 0.5 + 0.5*(b-a)/k, 0.0, 1.0 );
	return mix( b, a, h ) - k*h*(1.0-h);
}

float map(vec3 p)
{
    float t=0.07*iGlobalTime;
	float d1 = Disk(p, vec3(2.0, 1., 0.005)) + fpn(vec3(mul_m2(Spin(t*0.25+p.z*.10), p.xy)*20.,p.z*20.-t)*4.0) * 0.545;
    float d2 = Ring(p);
    return smin(d1,d2,1.0);

}

// assign color to the media
vec3 computeColor( float density, float radius )
{
	// color based on density alone, gives impression of occlusion within
	// the media
	vec3 result = mix( 1.1*vec3(1.0,0.9,0.8), vec3(0.4,0.15,0.1), density );
	
	// color added for disk
	vec3 colCenter = 6.*vec3(0.8,1.0,1.0);
	vec3 colEdge = 2.*vec3(0.48,0.53,0.5);
	result *= mix( colCenter, colEdge, min( (radius+.5)/2.0, 1.15 ) );
	
	return result;
}

bool Raycylinderintersect(vec3 org, vec3 dir, out float near, out float far)
{
	// quadratic x^2 + y^2 = 0.5^2 => (org.x + t*dir.x)^2 + (org.y + t*dir.y)^2 = 0.5
	float a = dot(dir.xy, dir.xy);
	float b = dot(org.xy, dir.xy);
	float c = dot(org.xy, org.xy) - 12.;

	float delta = b * b - a * c;
	if( delta < 0.0 )
		return false;

	// 2 roots
	float deltasqrt = sqrt(delta);
	float arcp = 1.0 / a;
	near = (-b - deltasqrt) * arcp;
	far = (-b + deltasqrt) * arcp;
	
	// order roots
	float temp = min(far, near);
	far = max(far, near);
	near = temp;

	float znear = org.z + near * dir.z;
	float zfar = org.z + far * dir.z;

	// top, bottom
	vec2 zcap = vec2(1.85, -1.85);
	vec2 cap = (zcap - org.z) / dir.z;

	if ( znear < zcap.y )
		near = cap.y;
	else if ( znear > zcap.x )
		near = cap.x;

	if ( zfar < zcap.y )
		far = cap.y;
	else if ( zfar > zcap.x )
		far = cap.x;
	
	return far > 0.0 && far > near;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{  
    float key = 0.0;

	// ro: ray origin
	// rd: direction of the ray
    fragCoord.y = iResolution.y -1 - fragCoord.y;
	vec3 rd = normalize(vec3((fragCoord.xy-0.5*iResolution.xy)/iResolution.y, 1.));
	vec3 ro = vec3(0., 0., -6.+key*1.6);

    R(rd.yz, -pi*3.65);
    R(rd.xz, pi*3.2);
    R(ro.yz, -pi*3.65);
   	R(ro.xz, pi*3.2);    
    
	// ld, td: local, total density 
	// w: weighting factor
	float ld=0., td=0., w=0.;

	// t: length of the ray
	// d: distance function
	float d=1., t=0.;
   
	vec4 sum = 0.0;
   
    float min_dist=0.0, max_dist=0.0;

    if(Raycylinderintersect(ro, rd, min_dist, max_dist))
    {
       
	t = min_dist*step(t,min_dist);
   
	// raymarch loop
    LOOP
	for (int i=0; i<128; i++) 
	{
	 
		vec3 pos = ro + t*rd;

		float fld = 0.0;
       
		// Loop break conditions.
        if(td>(1.-1./80.) || d<0.008*t || t>10. || sum.a > 0.99 || t>max_dist) break;
        //if(sum.a > 0.99 || t>max_dist) break;
	  
		// evaluate distance function
		d = map(pos); 
       
		// direction to center
        vec3 center = 0;
		vec3 stardir = normalize(center-pos);
      
		// change this string to control density 
		d = max(d,0.08);
      
		td += 1./65.;
       
        // point light calculations
        vec3 ldst = center-pos;
        float lDist = max(length(ldst), 0.0001);

        // star in center
        vec3 lightColor=vec3(1.0,0.5,0.25);
        sum.rgb+=lightColor/(lDist*lDist*lDist*27.);//*10.); //add a bloom around the light

        // using the light distance to perform some falloff
        //float atten = 1./(1. + lDist*0.125 + lDist*lDist*0.4);
        // accumulating the color
        //sum += w*atten*fld;
       
        t +=  max(d * 0.23, 0.02);
      
	}
    
    //scattering test
	//sum *= 1. / exp( ld * 0.2 ) * 1.05;
        
   	sum = clamp( sum, 0.0, 1.0 );
   
    sum.xyz = sum.xyz*sum.xyz*(3.0-2.0*sum.xyz);
    
	}

    #ifdef BACKGROUND
    // stars background
    if (td<.8)
    {
        //vec3 stars = pn(rd*300.0)*0.4+0.5;
        vec3 stars = pn(float3(fragCoord.xy*0.41, rd.z*300.0))*0.4+0.5;
        vec3 starbg = 0.0;
        starbg = mix(starbg, vec3(0.8,0.9,1.0), smoothstep(0.99, 1.0, stars)*clamp(dot(0.0,rd)+0.75,0.0,1.0));
        starbg = clamp(starbg, 0.0, 1.0);
        //starbg = StableStarField( fragCoord, 0.97 );
        sum.xyz += starbg; 
    }
	#endif
    
   fragColor = vec4(sum.xyz,1.0);
}