float g_occlusion_radius;
float g_occlusion_max_distance;

float4x4 g_mat_view_inv;
float2 g_resolution;

sampler2D smp_occlusion;
sampler2D smp_position;
sampler2D smp_normal;
sampler2D smp_noise;

float4 dssdo_accumulate(float2 tex)
{
	float3 points[] =
	{
		float3(-0.134, 0.044, -0.825),
		float3(0.045, -0.431, -0.529),
		float3(-0.537, 0.195, -0.371),
		float3(0.525, -0.397, 0.713),
		float3(0.895, 0.302, 0.139),
		float3(-0.613, -0.408, -0.141),
		float3(0.307, 0.822, 0.169),
		float3(-0.819, 0.037, -0.388),
		float3(0.376, 0.009, 0.193),
		float3(-0.006, -0.103, -0.035),
		float3(0.098, 0.393, 0.019),
		float3(0.542, -0.218, -0.593),
		float3(0.526, -0.183, 0.424),
		float3(-0.529, -0.178, 0.684),
		float3(0.066, -0.657, -0.570),
		float3(-0.214, 0.288, 0.188),
		float3(-0.689, -0.222, -0.192),
		float3(-0.008, -0.212, -0.721),
		float3(0.053, -0.863, 0.054),
		float3(0.639, -0.558, 0.289),
		float3(-0.255, 0.958, 0.099),
		float3(-0.488, 0.473, -0.381),
		float3(-0.592, -0.332, 0.137),
		float3(0.080, 0.756, -0.494),
		float3(-0.638, 0.319, 0.686),
		float3(-0.663, 0.230, -0.634),
		float3(0.235, -0.547, 0.664),
		float3(0.164, -0.710, 0.086),
		float3(-0.009, 0.493, -0.038),
		float3(-0.322, 0.147, -0.105),
		float3(-0.554, -0.725, 0.289),
		float3(0.534, 0.157, -0.250),
	};

	const int num_samples = 32;

	float2 noise_texture_size = float2(4,4);
	float3  center_pos  = tex2D(smp_position, tex).xyz;
	float3 eye_pos = g_mat_view_inv[3].xyz;

	float  center_depth  = distance(eye_pos, center_pos);

	float radius = g_occlusion_radius / center_depth;
	float max_distance_inv = 1.f / g_occlusion_max_distance;
	float attenuation_angle_threshold = 0.1;

	float3 noise = tex2D(smp_noise, tex*g_resolution.xy/noise_texture_size).xyz*2-1;

	//radius = min(radius, 0.1);

	float3 center_normal = tex2D(smp_normal, tex).xyz;

	float4 occlusion_sh2 = 0;

	const float fudge_factor_l0 = 2.0;
	const float fudge_factor_l1 = 10.0;

	const float sh2_weight_l0 = fudge_factor_l0 * 0.28209; //0.5*sqrt(1.0/pi);
	const float3 sh2_weight_l1 = fudge_factor_l1 * 0.48860; //0.5*sqrt(3.0/pi);

	const float4 sh2_weight = float4(sh2_weight_l1, sh2_weight_l0) / num_samples;

	[unroll] // compiler wants to branch here by default and this makes it run nearly 2x slower on PC and 1.5x slower on 360!
	for( int i=0; i<num_samples; ++i )
	{
	    float2 textureOffset = reflect( points[ i ].xy, noise.xy ).xy * radius;
		float2 sample_tex = tex + textureOffset;
		float3 sample_pos = tex2Dlod(smp_position, float4(sample_tex,0,0)).xyz;
		float3 center_to_sample = sample_pos - center_pos;
		float dist = length(center_to_sample);
		float3 center_to_sample_normalized = center_to_sample / dist;
		float attenuation = 1-saturate(dist * max_distance_inv);
		float dp = dot(center_normal, center_to_sample_normalized);

		attenuation = attenuation*attenuation * step(attenuation_angle_threshold, dp);

		occlusion_sh2 += attenuation * sh2_weight*float4(center_to_sample_normalized,1);
	}

	return occlusion_sh2;
}


float4 dssdo_blur(float2 tex, float2 dir) : COLOR
{
	float weights[9] =
	{
		0.013519569015984728,
		0.047662179108871855,
		0.11723004402070096,
		0.20116755999375591,
		0.240841295721373,
		0.20116755999375591,
		0.11723004402070096,
		0.047662179108871855,
		0.013519569015984728
	};

	float indices[9] = {-4, -3, -2, -1, 0, +1, +2, +3, +4};

	float2 step = dir/g_resolution.xy;

	float3 normal[9];

	normal[0] = tex2D(smp_normal, tex + indices[0]*step).xyz;
	normal[1] = tex2D(smp_normal, tex + indices[1]*step).xyz;
	normal[2] = tex2D(smp_normal, tex + indices[2]*step).xyz;
	normal[3] = tex2D(smp_normal, tex + indices[3]*step).xyz;
	normal[4] = tex2D(smp_normal, tex + indices[4]*step).xyz;
	normal[5] = tex2D(smp_normal, tex + indices[5]*step).xyz;
	normal[6] = tex2D(smp_normal, tex + indices[6]*step).xyz;
	normal[7] = tex2D(smp_normal, tex + indices[7]*step).xyz;
	normal[8] = tex2D(smp_normal, tex + indices[8]*step).xyz;

	float total_weight = 1.0;
	float discard_threshold = 0.85;

	int i;

	for( i=0; i<9; ++i )
	{
		if( dot(normal[i], normal[4]) < discard_threshold )
		{
			total_weight -= weights[i];
			weights[i] = 0;
		}
	}

	//

	float4 res = 0;

	for( i=0; i<9; ++i )
	{
		res += tex2D(smp_occlusion, tex + indices[i]*step) * weights[i];
	}

	res /= total_weight;

	return res;
}