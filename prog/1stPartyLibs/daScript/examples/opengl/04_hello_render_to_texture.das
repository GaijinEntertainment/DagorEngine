options gen2
require glfw/glfw_boost
require opengl/opengl_boost
require glsl/glsl_opengl
require daslib/defer
require math

require daslib/safe_addr

var @in @location v_position : float2
var @in @location v_color : float3
var @in @location v_texcoord : float2
var @uniform f_offset : float2
var @inout f_color : float3
var @inout f_texcoord : float2
var @uniform f_tex : sampler2D
var @out f_FragColor : float4

[vertex_program]
def vs_effect {
    f_color = v_color
    f_texcoord = v_texcoord
    gl_Position = float4(v_position, 0.0, 1.0)
}

def checker(uv : float2; checkSize : float = 8.0) {
    let fmodResult = (floor(checkSize * uv.x) + floor(checkSize * uv.y)) % 2.0
    return max(sign(fmodResult), 0.0)
}

def greyscale(color : float3) {
    return dot(color, float3(0.2126, 0.7152, 0.0722))
}

[fragment_program]
def fs_effect {
    let coord = f_texcoord + f_offset
    f_FragColor = texture(f_tex, coord) * float4(f_color, 1.0)
}

[fragment_program]
def fs_display {
    let f = checker(f_texcoord.xy)
    let trgb = texture(f_tex, float2(f_texcoord.x, 1. - f_texcoord.y)).xyz
    let tgrey = greyscale(trgb)
    f_FragColor.xyz = trgb * f + float3(tgrey) * (1.0 - f)
    f_FragColor.w = 1.
}

var program, display_program : uint
var vao : uint
var vbo : uint
var ebo : uint
var texture : uint
var fbo : uint
let fbo_width = 256
let fbo_height = 256
var target_texture : uint

[vertex_buffer]
struct EffectVertex {
    xy  : float2
    rgb : float3
    uv  : float2
}

let effect_vertices = [EffectVertex(
    xy=float2(-1.0, 1.0), rgb=float3(0.0, 0.0, 0.0), uv=float2(0.0, 0.0)), EffectVertex(
    xy=float2(1.0, 1.0), rgb=float3(1.0, 1.0, 0.0), uv=float2(1.0, 0.0)), EffectVertex(
    xy=float2(1.0, -1.0), rgb=float3(1.0, 0.0, 0.0), uv=float2(1.0, 1.0)), EffectVertex(
    xy=float2(-1.0, -1.0), rgb=float3(1.0, 1.0, 0.0), uv=float2(0.0, 1.0)
)];

let effect_indices = fixed_array<int>(0, 1, 2, 2, 3, 0)

def create_gl_objects {
    program = create_shader_program(@@vs_effect, @@fs_effect)
    display_program = create_shader_program(@@vs_effect, @@fs_display)
    // vao
    glGenVertexArrays(1, safe_addr(vao))
    glBindVertexArray(vao)
    // vbo
    glGenBuffers(1, safe_addr(vbo))
    glBindBuffer(GL_ARRAY_BUFFER, vbo)
    glBufferData(GL_ARRAY_BUFFER, effect_vertices, GL_STATIC_DRAW)
    bind_vertex_buffer(null, type<EffectVertex>)
    // ebo
    glGenBuffers(1, safe_addr(ebo))
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo)
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, effect_indices, GL_STATIC_DRAW)
    // image
    texture = load_image_from_file("image.png",
        "{get_das_root()}/examples/opengl",
        "{get_das_root()}/examples/opengl/media"
    )
    // fbo
    glGenFramebuffers(1, safe_addr(fbo))
    glBindFramebuffer(GL_FRAMEBUFFER, fbo)
    // texture bound to fbo
    glGenTextures(1, safe_addr(target_texture))
    glBindTexture(GL_TEXTURE_2D, target_texture)
    glTexImage2D(GL_TEXTURE_2D, 0, int(GL_RGB), fbo_width, fbo_height, 0, GL_RGB, GL_UNSIGNED_BYTE, null)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target_texture, 0)
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        panic("fbo failed")
    }
}

[export]
def main {
    if (glfwInit() == 0) {
        panic("can't init glfw")
    }
    defer <| $() {
        glfwTerminate()
    }
    glfwInitOpenGL(3, 3)
    var window = glfwCreateWindow(1280, 720, "OpenGL - Hello render to texture", null, null)
    if (window == null) {
        panic("can't create window")
    }
    defer <| $() {
        glfwDestroyWindow(window)
    }
    glfwMakeContextCurrent(window)
    create_gl_objects()
    eval_main_loop <| $() {
        if (glfwWindowShouldClose(window) != 0) {
            return false
        }
        glfwPollEvents()
        var display_w, display_h : int
        glfwGetFramebufferSize(window, display_w, display_h)
        let t = glfwGetTime()
        if (t > 1lf) {
            glfwSetTime(t - 1lf)
        }
        let ft = float(t)
        f_offset = float2(ft, sin(ft * PI * 2.) * 0.25)
        // bind fbo and render to it
        glBindFramebuffer(GL_FRAMEBUFFER, fbo)
        glViewport(0, 0, fbo_width, fbo_height)
        glUseProgram(program)
        f_tex := texture
        vs_effect_bind_uniform(program)
        fs_effect_bind_uniform(program)
        glBindVertexArray(vao)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo)
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, null)
        // bind back buffer, and render to it
        glBindFramebuffer(GL_FRAMEBUFFER, 0u)
        glViewport(0, 0, display_w, display_h)
        glUseProgram(display_program)
        f_tex := target_texture
        vs_effect_bind_uniform(display_program)
        fs_display_bind_uniform(display_program)
        glBindVertexArray(vao)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo)
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, null)
        // swap buffers
        glfwMakeContextCurrent(window)
        glfwSwapBuffers(window)
        return true
    }
}

