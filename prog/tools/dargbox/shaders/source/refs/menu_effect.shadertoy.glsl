//source https://www.shadertoy.com/view/tXy3DK
/*
    I'm too lazy to copy the MIT license so like just do whatever you want with this shader
    but like if you can leave a comment or something or hit me up, that'd be dope. thxxx

    -- int_45h
*/

//#define DAVE_HOSKINS
#define THRESHOLD .99
#define DUST
#define MIN_DIST .13
#define MAX_DIST 40.
#define MAX_DRAWS 40

// https://www.shadertoy.com/view/XdGfRR
#ifdef DAVE_HOSKINS
float hash12(vec2 p)
{
	uvec2 q = uvec2(ivec2(p)) * uvec2(1597334673U, 3812015801U);
	uint n = (q.x ^ q.y) * 1597334673U;
	return float(n) * 2.328306437080797e-10;
}
#else
float hash12(vec2 p)
{
    ivec2 pi = ivec2(
        mod(p.x, iChannelResolution[0].x),
        mod(p.y, iChannelResolution[0].y)
    );
    return texelFetch(iChannel0, pi,0).r;
}
#endif

float value2d(vec2 p);

// Based on xaot88's starfield: https://www.shadertoy.com/view/Md2SR3
float get_stars_rough(vec2 p)
{
    float s = smoothstep(THRESHOLD,1.,hash12(p));
    if (s >= THRESHOLD)
        s = pow((s-THRESHOLD) / (1.-THRESHOLD), 10.); // Get s in 0-1 range
    return s;
}

// Instead of linear interpolation like in the original, I used cubic hermite interpolation
float get_stars(vec2 p, float a, float t)
{
    vec2 pg=floor(p), pc=p-pg, k=vec2(0,1);
    pc *= pc*pc*(3.-2.*pc);
    
    float s = mix(
        mix(get_stars_rough(pg+k.xx), get_stars_rough(pg+k.yx), pc.x),
        mix(get_stars_rough(pg+k.xy), get_stars_rough(pg+k.yy), pc.x),
        pc.y
    );
    return smoothstep(a,a+t, s)*pow(value2d(p*.1 + iTime)*.5+.5,8.3);
}

// This is stupid but I needed another value noise function (for part of the wave effect)
#ifdef DAVE_HOSKINS
float value2d(vec2 p)
{
    vec2 pg=floor(p),pc=p-pg,k=vec2(0,1);
    pc*=pc*pc*(3.-2.*pc);
    return mix(
        mix(hash12(pg+k.xx),hash12(pg+k.yx),pc.x),
        mix(hash12(pg+k.xy),hash12(pg+k.yy),pc.x),
        pc.y
    );
}
#else
float value2d(vec2 p)
{
    return texture(iChannel0, p / iChannelResolution[0].xy).r;
}
#endif
// Shorthand for .5+.5*sin/cos(x)
float s5(float x) {return .5+.5*sin(x);}
float c5(float x) {return .5+.5*cos(x);}

// Sample the starfield at different sizes (to fake depth). What are stars other than really big balls of condensed dust...
float get_dust(vec2 p, vec2 size, float f)
{
    // Aspect ratio (so the stars look correct)
    vec2 ar = vec2(iResolution.x/iResolution.y,1);

    // Play with the power exponents to mess with the
    // intensity of the sin/cos waves (for the stars, NOT for)
    // the translucent wave
    vec2 pp=p*size*ar;
    return
        pow(   .64+.46*cos(p.x*6.28), 1.7)*    // keep stars at edges of the screen
        //pow(1.-c5(p.y*6.28+.2), 3.3)* // keep stars in middle row
        f*
    (
        get_stars(.1*pp+iTime*vec2(20.,-10.1),.11,.71)*4. + 
        get_stars(.2*pp+iTime*vec2(30.,-10.1),.1,.31)*5. + 
        get_stars(.32*pp+iTime*vec2(40.,-10.1),.1,.91)*2.
    );
}

float sdf(vec3 p)
{ 
    p*=2.;
    float o =
        4.2*sin(.05*p.x+iTime*.25)+ // Make the wave move up and down
        (.04*p.z)*            // Make waves more intense as they get further away
        sin(p.x*.11+iTime)*   // Add a sine wave in the x direction to make it more wavy
        2.*sin(p.z*.2+iTime)* // Add some waves in the z direction too, why not?
        value2d(              // Value noise (to make the waves more erratic and because i didn't use the sine waves at first)
            vec2(.03,.4)*p.xz+vec2(iTime*.5,0) // Stretch it out, make it longer in the y direction, then add movement
        );
    return abs(dot(p,normalize(vec3(0,1,0.05)))+2.5+o*.5);
}

vec3 norm(vec3 p)
{
    const vec2 k=vec2(1,-1);
    const float t=.001;
    return normalize(
        k.xyy*sdf(p+t*k.xyy) +
        k.yyx*sdf(p+t*k.yyx) +
        k.yxy*sdf(p+t*k.yxy) +
        k.xxx*sdf(p+t*k.xxx)
    );
}

// Since we only need the alpha, return a float.
vec2 raymarch(vec3 o, vec3 d, float omega)
{
    float   t =0.,
            a =0.;       // Alpha, since we're just returning a value from 0 to 1
    float   g =MAX_DIST, // Glow
            dt=0.,       // Delta
            sl=0.,       // Step length
            emin=0.03,   // Minimum epsilon
            ed=emin;     // Dynamic epsilon
    int     dr=0;  // Number of draws
    bool    hit=false; // Hit detector, tripped only once (for adding base color)

    // Raymarching method based on this paper: https://kev.town/raymarching/media/enhanced_sphere_tracing.pdf
    for (int i=0;i<100;i++)
    {
        vec3 p=o+d*t;

        float ndt=sdf(p);
        if (abs(dt)+abs(ndt) < sl)
        {
            sl -= omega*sl;
            omega = 1.;
        }
        else
            sl = ndt*omega;

        dt=ndt;
        t+=sl;

        // Glow
        g = (t > 10.) ? min(g,abs(dt)) : MAX_DIST;

        if ((t+=dt)>=MAX_DIST) break;
        if (dt<MIN_DIST) // Made the minimum distance .13 because anything shorter made it look wrong.
        {
            if(dr > MAX_DRAWS) break; // Number of times we can draw before it's time to bail.
            dr++;

            float f = smoothstep(0.09, 0.11, (p.z*.9)/100.); // Smoothly fade out things that are close to the camera
            //g = mix(g, MAX_DIST, 1.-f);
            if (!hit)
            {
                a=.01;
                hit=true;
            }
            ed=2.*max(emin,abs(ndt));
            a += .0135*f; // Default alpha of the wave is .012, multiply by the fade
            //a += .135*max(dot(norm(p),d),0.);
            t += ed; // Offset so we don't just sample the same point again and again
        }
    }

    g /= 3.;
    return vec2(a, max(1.-g, 0.));
}

void mainImage(out vec4 O, in vec2 U)
{
    vec2 ires=iResolution.xy, uv=U/ires;
    vec3 o=vec3(0),d=(vec3(
        (U-.5*ires)/ires.y,1
    ));
    vec2 mg = raymarch(o,d,1.2);
    float m = mg.x;

    vec3 c = mix( // Simple reddish 4 point gradient
        mix(vec3(.7,.2,.2),vec3(.4,.1,.1),uv.x),
        mix(vec3(.45,.1,.1),vec3(.8,.3,.5),uv.x),
        uv.y
    );
    // Blend between our color and white based on the wave alpha
    c = mix(c, vec3(1.), m);

    // Add dust (optional)
    #ifdef DUST
    c += get_dust(uv, vec2(2000.), mg.y)*.3;
    #endif
    O=vec4(c,1);
    //O=vec4(value2d(U/4.));
}