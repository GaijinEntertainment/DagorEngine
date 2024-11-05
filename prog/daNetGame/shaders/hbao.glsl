// number of directions to sample in UV space
#define NUM_SAMPLE_DIRECTIONS {{ numSampleDirections }}

// number of sample steps when raymarching along a direction
#define NUM_SAMPLE_STEPS      {{ numSampleSteps }}

#define APPLY_ATTENUATION     {{ attenuation }}

#define USE_ACTUAL_NORMALS    {{ useActualNormals }}

uniform sampler2D sGBuffer;
uniform sampler2D sNoise;

uniform float uFOV;
uniform float uSampleRadius;
uniform float uAngleBias;
uniform float uIntensity;
uniform vec2  uNoiseScale;

/*vec2 snapUV(vec2 uv) {
  vec2 temp = uv * viewportResolution;
  temp = floor(temp) + step(vec2(0.5), ceil(temp));
  
  return temp * (1.0 / viewportResolution);
}*/

// reconstructs view-space unit normal from view-space position
vec3 reconstructNormalVS(vec3 positionVS) {
  return normalize(cross(dFdx(positionVS), dFdy(positionVS)));
}

vec3 getPositionViewSpace(vec2 uv) {
  float depth = decodeGBufferDepth(sGBuffer, uv, clipFar);
  
  vec2 uv2  = uv * 2.0 - vec2(1.0);
  vec4 temp = viewProjectionInverseMatrix * vec4(uv2, -1.0, 1.0);
  vec3 cameraFarPlaneWS = (temp / temp.w).xyz;
  
  vec3 cameraToPositionRay = normalize(cameraFarPlaneWS - cameraPositionWorldSpace);
  vec3 originWS = cameraToPositionRay * depth + cameraPositionWorldSpace;
  vec3 originVS = (viewMatrix * vec4(originWS, 1.0)).xyz;
  
  return originVS;
}

void main() {
  const float TWO_PI = 2.0 * PI;
  
  vec3 originVS = getPositionViewSpace(vUV);
  
#if USE_ACTUAL_NORMALS
  gBufferGeomComponents gBufferValue = decodeGBufferGeom(sGBuffer, vUV, clipFar);
  
  vec3 normalVS = gBufferValue.normal;
#else
  vec3 normalVS = reconstructNormalVS(originVS);
#endif
  
  float radiusSS = 0.0; // radius of influence in screen space
  float radiusWS = 0.0; // radius of influence in world space
  
  radiusSS = uSampleRadius;
  vec4 temp0 = viewProjectionInverseMatrix * vec4(0.0, 0.0, -1.0, 1.0);
  vec3 out0  = temp0.xyz;
  vec4 temp1 = viewProjectionInverseMatrix * vec4(radiusSS, 0.0, -1.0, 1.0);
  vec3 out1  = temp1.xyz;
  
  // NOTE (travis): empirically, the main introduction of artifacts with HBAO 
  // is having too large of a world-space radius; attempt to combat this issue by 
  // clamping the world-space radius based on the screen-space radius' projection
  radiusWS = min(tan(radiusSS * uFOV / 2.0) * originVS.y / 2.0, length(out1 - out0));
  
  // early exit if the radius of influence is smaller than one fragment
  // since all samples would hit the current fragment.
  /*if (radiusSS < 1.0 / viewportResolution.x) {
    gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    return;
  }*/
  
  const float theta = TWO_PI / float(NUM_SAMPLE_DIRECTIONS);
  float cosTheta = cos(theta);
  float sinTheta = sin(theta);
  
  // matrix to create the sample directions
  mat2 deltaRotationMatrix = mat2(cosTheta, -sinTheta, sinTheta, cosTheta);
  
  // step vector in view space
  vec2 deltaUV = vec2(1.0, 0.0) * (radiusSS / (float(NUM_SAMPLE_DIRECTIONS * NUM_SAMPLE_STEPS) + 1.0));
  
  // we don't want to sample to the perimeter of R since those samples would be 
  // omitted by the distance attenuation (W(R) = 0 by definition)
  // Therefore we add a extra step and don't use the last sample.
  vec3 sampleNoise    = texture2D(sNoise, vUV * uNoiseScale).xyz;
  sampleNoise.xy      = sampleNoise.xy * 2.0 - vec2(1.0);
  mat2 rotationMatrix = mat2(sampleNoise.x, -sampleNoise.y, 
                             sampleNoise.y,  sampleNoise.x);
  
  // apply a random rotation to the base step vector
  deltaUV = rotationMatrix * deltaUV;
  
  float jitter = sampleNoise.z;
  float occlusion = 0.0;
  
  for (int i = 0; i < NUM_SAMPLE_DIRECTIONS; ++i) {
    // incrementally rotate sample direction
    deltaUV = deltaRotationMatrix * deltaUV;
    
    vec2 sampleDirUV = deltaUV;
    float oldAngle   = uAngleBias;
    
    for (int j = 0; j < NUM_SAMPLE_STEPS; ++j) {
      vec2 sampleUV     = vUV + (jitter + float(j)) * sampleDirUV;
      vec3 sampleVS     = getPositionViewSpace(sampleUV);
      vec3 sampleDirVS  = (sampleVS - originVS);
      
      // angle between fragment tangent and the sample
      float gamma = (PI / 2.0) - acos(dot(normalVS, normalize(sampleDirVS)));
      
      if (gamma > oldAngle) {
        float value = sin(gamma) - sin(oldAngle);
        
#if APPLY_ATTENUATION
        // distance between original and sample points
        float attenuation = clamp(1.0 - pow(length(sampleDirVS) / radiusWS, 2.0), 0.0, 1.0);
        occlusion += attenuation * value;
#else
        occlusion += value;
#endif
        oldAngle = gamma;
      }
    }
  }
  
  occlusion = 1.0 - occlusion / float(NUM_SAMPLE_DIRECTIONS);
  occlusion = clamp(pow(occlusion, 1.0 + uIntensity), 0.0, 1.0);
  gl_FragColor = vec4(occlusion, occlusion, occlusion, 1.0);
}