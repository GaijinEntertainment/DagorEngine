options gen2
options persistent_heap
options gc
require glfw/glfw_boost
require opengl/opengl_boost
require opengl/opengl_gen
require math
require strings
require daslib/math_boost
require live/live_gc

var @in @location v_position : float3
var @in @location v_normal   : float3
var @in @location v_texcoord : float2
var @uniform v_model : float4x4
var @uniform v_view : float4x4
var @uniform v_projection : float4x4
var @inout f_texcoord : float2
var @inout f_normal : float3
var @inout f_position : float3
var @uniform f_camera_position : float3
var @uniform@stage f_tex : sampler2D
var @out f_FragColor : float4

[vertex_program]
def vs_preview {
    f_texcoord = v_texcoord
    f_normal = normalize(float3x3(v_model) * v_normal)
    let tpos = v_model * float4(v_position, 1.0)
    f_position = tpos.xyz
    gl_Position = v_projection * v_view * tpos
}

[fragment_program]
def fs_preview {
    let pS = 24.0
    let light_dir = normalize(float3(-1, -2, -3))
    let camera_dir = normalize(f_position)
    let normal = normalize(f_normal)
    let k = -dot(light_dir, normal)
    let diffuse = float3(saturate(k))
    let refl = normalize(reflect(camera_dir, normal))
    let kS = saturate(dot(-light_dir, refl))
    let specular = float3(pow(kS, pS) * kS)
    f_FragColor.xyz = texture(f_tex, f_texcoord).xyz * diffuse + specular
    f_FragColor.w = 1.
}

var window : GLFWwindow?
var program : uint
var sphere, cube, cylinder : OpenGLGeometryFragment
var checkerboard_texture : uint

[export]
def init() {
    if (glfwInit() == 0) {
        panic("can't init glfw")
    }
    glfwInitOpenGL(3, 3)
    window = glfwCreateWindow(1280, 720, "OpenGL - Hello generated geometry", null, null)
    if (window == null) {
        panic("can't create window")
    }
    glfwMakeContextCurrent(window)
    program = create_shader_program(@@vs_preview, @@fs_preview)
    sphere <- gen_sphere(32, 16, false) |> create_geometry_fragment
    cube <- gen_cube() |> create_geometry_fragment
    cylinder <- gen_cylinder(GenDirection.xz, 32) |> create_geometry_fragment
    checkerboard_texture = gen_image_checkerboard(16, 16, 0xff404040, 0xff808080) |> create_texture
    glBindTexture(GL_TEXTURE_2D, checkerboard_texture)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST)
    glBindTexture(GL_TEXTURE_2D, 0u)
}

[export]
def update() {
    glfwPollEvents()
    var display_w, display_h : int
    glfwGetFramebufferSize(window, display_w, display_h)
    let aspect = display_h != 0 ? float(display_w) / float(display_h) : 1.
    // render
    glViewport(0, 0, display_w, display_h)
    glClearColor(0.2, 0.2, 0.2, 1.0)
    glClearDepth(1.0lf)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
    glEnable(GL_DEPTH_TEST)
    glEnable(GL_CULL_FACE)
    glCullFace(GL_BACK)
    glDepthFunc(GL_LEQUAL)
    let t = glfwGetTime()
    if (t > 10lf) {
        glfwSetTime(t - 10lf)
    }
    f_camera_position = float3(0, 0, 8)
    v_view = look_at_rh(f_camera_position, float3(0, 0, 0), float3(0, 1, 0))
    let rot = quat_from_unit_vec_ang(normalize(float3(1., 1., 0.)), float(t) * PI * 2. * 0.1)
    v_projection = perspective_rh_opengl(45.0f * PI / 180., aspect, 0.1f, 50.0f)
    f_tex := checkerboard_texture
    glUseProgram(program)
    // cube
    v_model = compose(float3(-3, 0, 0), rot, float3(1.0))
    vs_preview_bind_uniform(program)
    fs_preview_bind_uniform(program)
    draw_geometry_fragment(cube)
    // sphere
    v_model = compose(float3(0, 0, 0), rot, float3(1.0))
    vs_preview_bind_uniform(program)
    fs_preview_bind_uniform(program)
    draw_geometry_fragment(sphere)
    // cylinder
    v_model = compose(float3(3, 0, 0), rot, float3(1.0))
    vs_preview_bind_uniform(program)
    fs_preview_bind_uniform(program)
    draw_geometry_fragment(cylinder)
    glUseProgram(0u)
    glfwMakeContextCurrent(window)
    glfwSwapBuffers(window)
}

[export]
def shutdown() {
    if (window != null) {
        glfwDestroyWindow(window)
        window = null
    }
    glfwTerminate()
}

var max_frame_limit = 0

def parse_args() {
    let args <- get_command_line_arguments()
    for (i in range(length(args) - 1)) {
        if (args[i] == "--max-frames") {
            max_frame_limit = to_int(args[i + 1])
        }
    }
}

// on web the wasm host auto-drives init/update/shutdown per frame and never calls main
[export]
def main() {
    parse_args()
    init()
    var frame_count = 0
    while (glfwWindowShouldClose(window) == 0) {
        update()
        maybe_collect_gc()
        frame_count ++
        if (max_frame_limit > 0 && frame_count >= max_frame_limit) {
            break
        }
    }
    shutdown()
}
