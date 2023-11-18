void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float t = iTime * 0.2;
    float param = exp(cos(t)) - 2.0 * cos(4.0 * t) - pow5(sin(t / 12.0));
    vec2 uv = fragCoord/iResolution.xy - 0.5 + vec2(sin(t), cos(t)) * param * 0.3;

    vec3 colors[4] = {
        vec3(0x60, 0x6c, 0x38) / 255.0,
        vec3(0x28, 0x36, 0x18) / 255.0,
        vec3(0xdd, 0xa1, 0x5e) / 255.0,
        vec3(0xbc, 0x6c, 0x25) / 255.0
    };
    vec3 col = mix(
        mix(colors[0], colors[1], uv.x),
        mix(colors[2], colors[3], uv.x),
        uv.y);

    fragColor = vec4(col,1.0);
}
