# Graphics Examples

ImGui and OpenGL examples using the in-tree `dasImgui` module. It ships built-in
with the daslang tree — no package install needed, a bare build suffices.

## Run

```bash
cd examples/graphics
daslang.exe -project_root . furier_opengl_imgui_example.das
```

## Requirements

- daslang SDK (built with `DAS_GLFW_DISABLED=OFF`)
- CMake 3.16+
- C++17 compiler
- OpenGL
