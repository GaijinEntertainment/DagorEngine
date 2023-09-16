vec3 rgbToColor(int rgb)
{
    return vec3((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, (rgb >> 0) & 0xff) / 255.0;
}

float fractNoise2(vec2 seed)
{
    const int NOISE_ITERS = 5;
    float noise = 0.0;
    float divisor = 0.5;
    float weights = 0.0;
    for (int i = 1; i <= NOISE_ITERS; ++i)
    {
        float val = tex2D(noise_128_tex_hash, seed / 128.0).x;
        noise += val * divisor;
        seed *= 2.0;
        seed = vec2(0.8 * seed.x - 0.6 * seed.y, 0.6 * seed.x + 0.8 * seed.y);
        weights += divisor;
        divisor *= 0.5;
    }
    return noise / weights;
}

float getCloudsLayer(vec3 camPos, vec3 view, float cloudsHeight, float clouds_speed, float scale)
{
    float t = max((cloudsHeight - camPos.y) / view.y, 1e-3);
    vec3 worldPos = camPos + view * t;

    return smoothstep(0.4, 0.9, fractNoise2(worldPos.xz * scale + vec2(0, iTime * clouds_speed)));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;

    vec3 camPos = vec3(0, 0, 0);
    vec3 viewVec = normalize(vec3(0, 0.85, 1));
    vec3 right = vec3(1, 0, 0);
    vec3 up = vec3(0, 1, 0);
    vec3 view = normalize(viewVec + right * (uv.x * 2.0 - 1.0) * 0.5 + up * (uv.y * 2.0 - 1.0) * -0.5);

    fragColor = vec4(rgbToColor(0x87ceeb), 0);
    float generalSpeed = 1.0;
    float layer1 = getCloudsLayer(camPos, view, 1000.0, 0.3 * generalSpeed, 0.003);
    float leyer2 = getCloudsLayer(camPos, view, 3000.0, 0.09 * generalSpeed, 0.0006);
    float layer3 = getCloudsLayer(camPos, view, 10000.0, 0.1 * generalSpeed, 0.00006);
    float clouds = layer1 + leyer2 - layer1 * leyer2;
    clouds = clouds + layer3 - layer3 * clouds;
    vec3 fogColor = rgbToColor(0xF2F8F7);
    float t = max((1000.0 - camPos.y) / view.y, 1e-3);
    fragColor = mix(fragColor, vec4(1, 1, 1, 1), clouds);
    fragColor.rgb = mix(fogColor, fragColor.rgb, exp(-0.0001 *  t));
}
