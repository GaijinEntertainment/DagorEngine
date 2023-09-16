// Star Nest by Pablo RomA?n Andrioli

// This content is under the MIT License.

#define iterations 17
#define formuparam 0.53

#define volsteps 8
#define stepsize 0.1

#define zoom   0.800
#define tile   0.850
#define speed  0.002

#define brightness 0.0015
#define darkmatter 0.300
#define distfading 0.730
#define saturation 0.850


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
  //get coords and direction
  vec2 uv=fragCoord.xy/iResolution.xy-.5;
  uv.y*=iResolution.y/iResolution.x;
  vec3 dir=vec3(uv*zoom,1.);
  float time=(iGlobalTime+59)*speed+0.25;

  //mouse rotation
  float a1=.5+iMouse.x/iResolution.x*2.;
  float a2=.8+iMouse.y/iResolution.y*2.;
  mat2 rot1=mat2(cos(a1),sin(a1),-sin(a1),cos(a1));
  mat2 rot2=mat2(cos(a2),sin(a2),-sin(a2),cos(a2));
  dir.xz=mul_m2(dir.xz, rot1);
  dir.xy=mul_m2(dir.xy, rot2);
  vec3 from=vec3(1.,.5,0.5);
  from+=vec3(time*2.,time,-2.);
  from.xz=mul_m2(from.xz, rot1);
  from.xy=mul_m2(from.xy, rot2);
  
  //volumetric rendering
  float s=0.1,fade=0.01;
  vec3 v=0.;
  for (int r=0; r<volsteps; r++) {
  	vec3 p=from+s*dir*.5;
  	p = abs(tile.xxx-mod(p, tile*2.)); // tiling fold
  	float pa,a=pa=0.;
  	for (int i=0; i<iterations; i++) { 
  		p=abs(p)/dot(p,p)-formuparam; // the magic formula
  		a+=abs(length(p)-pa); // absolute sum of average change
  		pa=length(p);
  	}
  	float dm=max(0.,darkmatter-a*a*.001); //dark matter
  	a*=a*a; // add contrast
  	if (r>6) fade*=1.-dm; // dark matter, don't render near
  	//v+=vec3(dm,dm*.5,0.);
  	v+=fade;
  	v+=vec3(s,s*s,s*s*s*s)*a*brightness*fade; // coloring based on distance
  	fade*=distfading; // distance fading
  	s+=stepsize;
  }
  v = min(v, 1.2);
  //v=mix(vec3(length(v)),v,saturation); //color adjust

  float intensity = min(v.r + v.g + v.b, 0.7);

  int2 sgn = (int2(fragCoord.xy) & 1) * 2 - 1;
  float2 gradient = float2(ddx(intensity) * sgn.x, ddy(intensity) * sgn.y);
  float cutoff = max(max(gradient.x, gradient.y) - 0.1, 0.0);
  v.rgb *= max(1.0 - cutoff * 6.0, 0.3);
 
  fragColor = vec4(v,1.);
}