// PS3 XMB Wave Knot — ShaderToy Version
// Paste this into shadertoy.com as a new shader (Image tab)
//
// Controls via constants below:
#define KNOT_POS vec2(0.5, 0.5)   // Knot position (0-1 UV space)
#define FLOW_DIR 0.0               // Direction angle in radians (0 = horizontal, PI/2 = vertical)
#define NUM_WAVES 18.0             // Number of ribbon waves
#define SPEED 0.8                  // Animation speed
#define GLOW_INTENSITY 0.5         // Glow amount (0.1 - 1.0)

#define PI 3.14159265359

float hash(float n){ return fract(sin(n)*43758.5453123); }

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;
    float aspect = iResolution.x / iResolution.y;
    
    vec2 p = uv - 0.5;
    p.x *= aspect;
    
    vec2 knot = KNOT_POS - 0.5;
    knot.x *= aspect;
    
    float ca = cos(FLOW_DIR), sa = sin(FLOW_DIR);
    mat2 rot = mat2(ca, -sa, sa, ca);
    vec2 rp = rot * p;
    vec2 rk = rot * knot;
    
    float along = rp.x;
    float perp  = rp.y;
    float kAlong = rk.x;
    float kPerp  = rk.y;
    
    float t = iTime * SPEED;
    
    vec3 col = vec3(0.0);
    
    for(float i = 0.0; i < 40.0; i++){
        if(i >= NUM_WAVES) break;
        
        float id = i / NUM_WAVES;
        float seed  = hash(i * 17.31);
        float seed2 = hash(i * 23.71 + 7.0);
        float seed3 = hash(i * 11.13 + 3.0);
        
        float spread = (id - 0.5) * 2.0 + (seed - 0.5) * 0.6;
        
        float dAlong = along - kAlong;
        float pinch = 1.0 - exp(-1.8 * dAlong * dAlong);
        float tightPinch = exp(-8.0 * dAlong * dAlong);
        
        float ribbonBase = kPerp + spread * (0.15 + 0.55 * pinch);
        
        float flowPhase = along * (2.5 + seed * 2.0) - t * (0.3 + seed2 * 0.4);
        float wave = sin(flowPhase) * 0.04 * (0.5 + pinch);
        wave += sin(flowPhase * 2.3 + seed * 6.0 - t * 0.2) * 0.02 * (0.3 + pinch);
        wave += sin(flowPhase * 0.7 + seed3 * 4.0 + t * 0.15) * 0.03 * pinch;
        
        float crossWave = sin(along * 8.0 + i * PI / NUM_WAVES + t * 0.5) * 0.08 * tightPinch;
        wave += crossWave;
        
        float ribbonY = ribbonBase + wave;
        float d = abs(perp - ribbonY);
        
        float thickness = 0.0018 + 0.001 * tightPinch;
        float core = smoothstep(thickness, 0.0, d);
        
        float glowRadius = (0.008 + 0.015 * GLOW_INTENSITY) * (1.0 + 1.5 * tightPinch);
        float ribbonGlow = exp(-d * d / (glowRadius * glowRadius));
        
        vec3 baseColor;
        baseColor.r = 1.0;
        baseColor.g = 0.35 + seed * 0.12 + 0.15 * tightPinch;
        baseColor.b = 0.02 + seed2 * 0.08;
        
        float knotBrightness = 1.0 + 2.0 * tightPinch;
        
        float particlePhase = along * (30.0 + seed * 20.0) - t * (2.0 + seed * 3.0);
        float particles = pow(0.5 + 0.5 * sin(particlePhase), 3.0);
        float particleIntensity = 0.3 + 0.7 * particles;
        
        float nodePhase = along * (4.0 + seed * 3.0) - t * (0.1 + seed2 * 0.15);
        float node = smoothstep(0.92, 0.96, sin(nodePhase));
        float nodeGlow = node * exp(-d * d / (0.0003 + 0.001 * GLOW_INTENSITY));
        
        float intensity = (core * 1.5 + ribbonGlow * 0.6) * knotBrightness * particleIntensity;
        intensity += nodeGlow * 2.0;
        
        col += baseColor * intensity;
    }
    
    // Knot bloom
    float knotDist = length(vec2(along - kAlong, perp - kPerp));
    col += vec3(1.0, 0.7, 0.2) * exp(-knotDist * knotDist / (0.003 * GLOW_INTENSITY + 0.001)) * 0.8;
    col += vec3(0.6, 0.25, 0.02) * exp(-knotDist * knotDist / (0.02 * GLOW_INTENSITY + 0.005)) * 0.3;
    
    // Stars
    vec2 starUV = fragCoord * 0.01;
    float star = pow(hash(floor(starUV.x * 100.0) + floor(starUV.y * 100.0) * 317.0), 20.0) * 0.15;
    col += vec3(star * 0.7, star * 0.8, star);
    
    // Vignette
    col *= 1.0 - 0.4 * length(uv - 0.5);
    
    // Tonemap
    col = 1.0 - exp(-col * 1.5);
    col += vec3(-0.01, -0.005, 0.015) * (1.0 - col);
    
    fragColor = vec4(col, 1.0);
}
