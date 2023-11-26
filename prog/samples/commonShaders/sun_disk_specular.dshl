macro USE_SUN_DISK_SPECULAR()
  hlsl(ps){
    void sunDiskSpecular( float3 specularColor, float NoV, half ggx_alpha, float3 lightDir, float3 view, half3 normal, out float D, out float G, out float3 F )
    {
      //ggx_alpha = max(roughness, 0.03);
      //float sunAngularRadius = 0.01;//depends on time of day!
      //float r = sin( sunAngularRadius ); // Disk radius
      //float d = cos( sunAngularRadius ); // Distance to disk
      float r = 0.009999833334166664;
      float d = 0.9999500004166653;

      // Closest point to a disk ( since the radius is small , this is
      // a good approximation
      float NdotV = dot(normal, view);
      float3 R = (2 * NdotV * normal - view);
      float DdotR = dot (lightDir, R);
      float3 S = R - DdotR * lightDir;
      float3 L = DdotR < d ? normalize (d * lightDir + normalize (S) * r) : R;


      float3 H = normalize(view + L);
      float NoH = saturate( dot(normal, H) );
      float VoH = saturate( dot(view, H) );

      
      // Generalized microfacet specular
      D = BRDF_distribution( ggx_alpha, NoH );
      G = BRDF_geometricVisibility( ggx_alpha, NoV, saturate(dot(normal, L)), VoH );
      F = BRDF_fresnel( specularColor, VoH ).x;
    }
  }
endmacro