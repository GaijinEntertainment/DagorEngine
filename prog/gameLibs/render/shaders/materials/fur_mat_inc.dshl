texture fur_tangent_alpha_tex;
texture fur_diffuse_tex;
float4 fur_diffuse_color = float4(1, 1, 1, 1);

macro INIT_FUR_MAT_STATIC()
  static float thickness = 0.15;
  static float curl = 2;
  static float softness = 1;
  static float density = 0.6;
  static float tile = 120;
  static float dist_tile = 70;
  static float dist_power = 0.05;

  (vs) {
    fur_params@f4 = (thickness, curl, softness, density);
  }
  (ps) {
    fur_params@f4 = (thickness, curl, softness, density);
    fur_params2@f4 = (tile, dist_tile, dist_power, 0);
  }

  (ps) {
    fur_diffuse_color@f4 = fur_diffuse_color;
  }
  if (fur_tangent_alpha_tex != NULL)
  {
    (ps) {
      fur_tangent_alpha_tex@smpArray = fur_tangent_alpha_tex;
    }
  }
  if (fur_diffuse_tex != NULL)
  {
    (ps) {
      fur_diffuse_tex@smp2d = fur_diffuse_tex;
    }
  }

  hlsl {
    #define furDepth get_fur_params().x
    #define furCurl get_fur_params().y
    #define furSoft get_fur_params().z
    #define furThreshold (1.0 - get_fur_params().w)
    #define furTile get_fur_params2().x
    #define furDistTile get_fur_params2().y
    #define furDistPower get_fur_params2().z
    #define furDistThreshold 0.2
    #define furDistLen 0.3
    #define furDiffuseColor fur_diffuse_color

  ##if fur_tangent_alpha_tex != NULL
    #define furTangentAlphaTex fur_tangent_alpha_tex
  ##endif
  ##if fur_diffuse_tex != NULL
    #define furDiffuseTex fur_diffuse_tex
  ##endif
  }
endmacro

macro INIT_FUR_MAT_PARAMS(thickness, curl, softness, density, tile, dist_tile, dist_power, diffuse_color, tangent_alpha_tex, diffuse_tex)
  hlsl {
    #define furDepth thickness
    #define furCurl curl
    #define furSoft softness
    #define furThreshold (1.0 - density)
    #define furTile tile
    #define furDistTile dist_tile
    #define furDistPower dist_power
    #define furDistThreshold 0.2
    #define furDistLen 0.3
    #define furDiffuseColor diffuse_color
    #define furTangentAlphaTex tangent_alpha_tex
    #define furDiffuseTex diffuse_tex
  }
endmacro

macro FUR_MAT_COMMON()
  hlsl(ps) {
    #define SPECULAR_SHIFT 0

    #include "noise/Perlin2D.hlsl"
    #include "noise/Perlin3D.hlsl"
    #include "noise/Value3D.hlsl"
    #include "noise/Value2D.hlsl"

  #if SPECULAR_SHIFT
    float3 ShiftTangent(float3 T, float3 N, float shift)
    {
        float3 shiftedT = T + (shift * N);
        return normalize(shiftedT);
    }

    float StrandSpecular(float3 T, float3 V, float3 L, float exponent)
    {
        float3 H = normalize(L + V);
        float dotTH = dot(T, H);
        float sinTH = sqrt(1.0 - dotTH*dotTH);
        float dirAtten = smoothstep(-1.0, 0.0, dot(T, H));

        return dirAtten * pow(sinTH, exponent);
    }

    float3 HairLighting(float3 tangent, float3 normal, float3 lightVec,
      float3 viewVec, float2 uv, float ambOcc)
    {
      const float primaryShift = 0;
      const float secondaryShift = 0;
      const float3 specularColor1 = float3(1, 1, 1);
      const float3 specularColor2 = float3(1, 1, 1);
      const float3 lightColor = float3(1, 1, 1);
      const float specExp1 = 15;
      const float specExp2 = 15;

      // shift tangents
      float shiftTex = 0;//tex2D(tSpecShift, uv) - 0.5;
      float3 t1 = ShiftTangent(tangent, normal, primaryShift + shiftTex);
      float3 t2 = ShiftTangent(tangent, normal, secondaryShift + shiftTex);

      // diffuse lighting
      float3 diffuse = saturate(lerp(0.25, 1.0, dot(normal, lightVec)));

      // specular lighting
      float3 specular = specularColor1 * StrandSpecular(t1, viewVec, lightVec, specExp1);
      // add second specular term
      float specMask = 1;//tex2D(tSpecMask, uv);
      specular += specularColor2 * specMask * StrandSpecular(t2, viewVec, lightVec, specExp2);

      // Final color
      float3 o;
      o.rgb = (diffuse + specular) * lightColor;
      o.rgb *= ambOcc;
      return o;
    }
  #else
    float3 hairLightingTBN(float3 view, float3 light_dir, float2 uv, int shell_index, int num_shells,
      float3 ambient, float3 diffuse, float3 specular)
    {
      float3 CombVector = 0;
      float3 tangentVector = float3(0, 1, 0); //normalize((tangent_vector.rgb - 0.5f) * 2.0f); //this is the tangent to the strand of fur

      float kd = 0.7;
      float ks = 0.2;
      float specPower = 20.0;
      float backLight = 0.5;

      //kajiya and kay lighting
      tangentVector = normalize(tangentVector + CombVector);
      float TdotL = dot(tangentVector, light_dir);
      float TdotE = dot(tangentVector, view);
      float sinTL = sqrt(1 - TdotL * TdotL);
      float sinTE = sqrt(1 - TdotE * TdotE);
      float3 outputColor = kd * sinTL * diffuse.rgb +
          ks * pow(abs((TdotL * TdotE + sinTL * sinTE)), specPower).xxx * specular;

      //banks selfshadowing:
      float minShadow = 0.6;
      float shadow = (float(shell_index) / max(float(num_shells) - 1, 1)) * (1 - minShadow) + minShadow;
      outputColor.xyz *= shadow;
      outputColor = outputColor * saturate((light_dir.z + backLight) / (1.0 - backLight));

      outputColor += ambient * shadow;

      return outputColor;
    }
  #endif

    float4 furColor(float2 uv, int shell_index, int num_shells)
    {
      float4 furTex = furDiffuseColor;
    #if furTangentAlphaTex
      float4 tangentAlpha = tex3D(furTangentAlphaTex, float3(uv, shell_index), 0);
      furTex.a *= tangentAlpha.a;
    #else
      furTex.a *= saturate((noise_Value2D(uv * furTile) * 0.5 + 0.5));
      furTex.a = smoothstep(furThreshold, 1.0, furTex.a);

      float4 furTex2 = saturate((noise_Perlin3D(float3(uv * furDistTile, furDistPower * shell_index)) * 0.5 + 0.5));
      furTex.a *= saturate((furTex2.a - furDistThreshold) / furDistLen);

      float cutoff = shell_index / float(num_shells);
      furTex.a = saturate((furTex.a - cutoff) / ((1.0 - cutoff) * furSoft));
    #endif

    #if furDiffuseTex
      furTex *= tex2D(furDiffuseTex, uv);
    #else
      float colorNoise = noise_Perlin2D(15 + uv * 2) * 0.5 + 0.5;
      colorNoise = 0.2 + colorNoise * 0.8;
      furTex.rgb *= colorNoise.xxx;
    #endif

      return furTex;
    }
  }
endmacro

macro USE_FUR_MAT_MULTIPASS()
  FUR_MAT_COMMON()
  USE_PIXEL_TANGENT_SPACE()

  hlsl(vs) {
    void furVs(inout float3 local_pos, float3 normal, int shell_index, int num_shells)
    {
      local_pos = local_pos + normal * furDepth * shell_index / float(num_shells);

      float3 tangent;
      if (abs(normal.y) > 0.99)
      {
        tangent = float3(0, 0, 0);
      }
      else
      {
        float3 binormal = normalize(cross(normal, float3(0, 1, 0)));
        tangent = cross(normal, binormal);
      }

      float t = furDepth * shell_index / float(num_shells);
      local_pos += tangent * t * t * furCurl;	// curl down
    }
  }

  hlsl(ps) {
    float4 furPs(float3 view, float3 light_dir, float3 point_to_eye, float3 norm, float2 uv, int shell_index, int num_shells,
      float3 ambient, float3 diffuse, float3 specular)
    {
      view = normalize(view);
      norm = normalize(norm);
      light_dir = normalize(light_dir);

    #if SPECULAR_SHIFT
      float3 du, dv;
      get_du_dv(norm, point_to_eye, uv, du, dv);
      float3 light = HairLighting(dv, norm, -light_dir, -view, uv, 0.2);
      float3 baseColor = furBaseColor(uv);
      float4 color = furColor(uv, shell_index, num_shells);
      color.rgb *= baseColor * light;
      return color;
    #else
      float3 du, dv;
      get_du_dv(norm, point_to_eye, uv, du, dv);
      float3x3 tbn;
      tbn[0] = du;
      tbn[1] = dv;
      tbn[2] = norm;

      float4 baseColor = furColor(uv, shell_index, num_shells);

      return float4(
        hairLightingTBN(
          mul(tbn, view),
          mul(tbn, -normalize(light_dir)),
          uv,
          shell_index,
          num_shells,
          baseColor.rgb * ambient,
          baseColor.rgb * diffuse,
          specular
        ),
        baseColor.a
      );
    #endif
    }
  }
endmacro

macro USE_FUR_MAT_PARALLAX()
  FUR_MAT_COMMON()
  USE_PIXEL_TANGENT_SPACE()

  hlsl(ps) {
    float4 furPs(float3 view, float3 point_to_eye, float3 norm, float2 uv, int num_shells,
      out float ao)
    {
      float3 du, dv;
      get_du_dv(norm, point_to_eye, uv, du, dv);

      float3 viewDir;
      viewDir.x = dot(view, du);
      viewDir.y = dot(view, norm);
      viewDir.z = dot(view, dv);

      int numSteps = num_shells;
      half step = 1.0 / numSteps;
      float3 viewDirNrm = normalize(viewDir);
      half2 delta = half2(viewDirNrm.x, viewDirNrm.z) * furDepth / (-viewDirNrm.y * numSteps);
      half4 offset = float4(uv, 0, 0);
      float4 c = 0;
      int i;
      ao = 0;

      for (i = 0; i < numSteps; i++)
      {
        float4 furTex = furColor(offset.xy, numSteps - i, numSteps);
        if (i == numSteps)
          furTex.a = 1.0;

        float3 colorTex = furTex.rgb;
        float newAO = 1.0 - max(float(i) / max(float(numSteps) - 1, 1), 0.0);
        ao = ao + furTex.a * newAO * (1.0 - c.a);
        c = c + float4(colorTex * furTex.a, furTex.a) * (1.0 - c.a);
        if (c.a > 0.99)
          break;

        offset.xy += delta;
      }

      c.rgb /= max(c.a, 0.001);
      ao = 0.1 + 0.9 * ao;
      return c;
    }
  }
endmacro