// total number of samples at each fragment
#define NUM_SAMPLES           {{ numSamples }}

#define NUM_SPIRAL_TURNS      {{ numSpiralTurns }}

#define USE_ACTUAL_NORMALS    {{ useActualNormals }}

#define VARIATION             {{ variation }}

uniform sampler2D sGBuffer;
uniform sampler2D sNoise;

uniform float uFOV;
uniform float uIntensity;
uniform vec2  uNoiseScale;
uniform float uSampleRadiusWS;
uniform float uBias;

// reconstructs view-space unit normal from view-space position
vec3 reconstructNormalVS(vec3 positionVS) {
  return normalize(cross(dFdx(positionVS), dFdy(positionVS)));
}

vec3 getPositionVS(vec2 uv) {
  float depth = decodeGBufferDepth(sGBuffer, uv, clipFar);
  
  vec2 uv2  = uv * 2.0 - vec2(1.0);
  vec4 temp = viewProjectionInverseMatrix * vec4(uv2, -1.0, 1.0);
  vec3 cameraFarPlaneWS = (temp / temp.w).xyz;
  
  vec3 cameraToPositionRay = normalize(cameraFarPlaneWS - cameraPositionWorldSpace);
  vec3 originWS = cameraToPositionRay * depth + cameraPositionWorldSpace;
  vec3 originVS = (viewMatrix * vec4(originWS, 1.0)).xyz;
  
  return originVS;
}

// returns a unit vector and a screen-space radius for the tap on a unit disk 
// (the caller should scale by the actual disk radius)
vec2 tapLocation(int sampleNumber, float spinAngle, out float radiusSS) {
  // radius relative to radiusSS
  float alpha = (float(sampleNumber) + 0.5) * (1.0 / float(NUM_SAMPLES));
  float angle = alpha * (float(NUM_SPIRAL_TURNS) * 6.28) + spinAngle;
  
  radiusSS = alpha;
  return vec2(cos(angle), sin(angle));
}

vec3 getOffsetPositionVS(vec2 uv, vec2 unitOffset, float radiusSS) {
  uv = uv + radiusSS * unitOffset * (1.0 / viewportResolution);
  
  return getPositionVS(uv);
}

float sampleAO(vec2 uv, vec3 positionVS, vec3 normalVS, float sampleRadiusSS, 
               int tapIndex, float rotationAngle)
{
  const float epsilon = 0.01;
  float radius2 = uSampleRadiusWS * uSampleRadiusWS;
  
  // offset on the unit disk, spun for this pixel
  float radiusSS;
  vec2 unitOffset = tapLocation(tapIndex, rotationAngle, radiusSS);
  radiusSS *= sampleRadiusSS;
  
  vec3 Q = getOffsetPositionVS(uv, unitOffset, radiusSS);
  vec3 v = Q - positionVS;
  
  float vv = dot(v, v);
  float vn = dot(v, normalVS) - uBias;
  
#if VARIATION == 0
  
  // (from the HPG12 paper)
  // Note large epsilon to avoid overdarkening within cracks
  return float(vv < radius2) * max(vn / (epsilon + vv), 0.0);
  
#elif VARIATION == 1 // default / recommended
  
  // Smoother transition to zero (lowers contrast, smoothing out corners). [Recommended]
  float f = max(radius2 - vv, 0.0) / radius2;
  return f * f * f * max(vn / (epsilon + vv), 0.0);
  
#elif VARIATION == 2
  
  // Medium contrast (which looks better at high radii), no division.  Note that the 
  // contribution still falls off with radius^2, but we've adjusted the rate in a way that is
  // more computationally efficient and happens to be aesthetically pleasing.
  float invRadius2 = 1.0 / radius2;
  return 4.0 * max(1.0 - vv * invRadius2, 0.0) * max(vn, 0.0);
  
#else
  
  // Low contrast, no division operation
  return 2.0 * float(vv < radius2) * max(vn, 0.0);
  
#endif
}

void main() {
  vec3 originVS = getPositionVS(vUV);
  
#if USE_ACTUAL_NORMALS
  gBufferGeomComponents gBufferValue = decodeGBufferGeom(sGBuffer, vUV, clipFar);
  
  vec3 normalVS = gBufferValue.normal;
#else
  vec3 normalVS = reconstructNormalVS(originVS);
#endif
  
  vec3 sampleNoise = texture2D(sNoise, vUV * uNoiseScale).xyz;
  
  float randomPatternRotationAngle = 2.0 * PI * sampleNoise.x;
  
  float radiusSS  = 0.0; // radius of influence in screen space
  float radiusWS  = 0.0; // radius of influence in world space
  float occlusion = 0.0;
  
  // TODO (travis): don't hardcode projScale
  float projScale = 40.0;//1.0 / (2.0 * tan(uFOV * 0.5));
  radiusWS = uSampleRadiusWS;
  radiusSS = projScale * radiusWS / originVS.y;
  
  for (int i = 0; i < NUM_SAMPLES; ++i) {
    occlusion += sampleAO(vUV, originVS, normalVS, radiusSS, i, 
                          randomPatternRotationAngle);
  }
  
  occlusion = 1.0 - occlusion / (4.0 * float(NUM_SAMPLES));
  occlusion = clamp(pow(occlusion, 1.0 + uIntensity), 0.0, 1.0);
  gl_FragColor = vec4(occlusion, occlusion, occlusion, 1.0);
}