# Graphics Examples

ImGui and OpenGL examples using the `dasImgui` daspkg package.

## Setup

```bash
cd examples/graphics
daslang.exe ../../utils/daspkg/main.das -- install
```

This installs `dasImgui` into `modules/` and builds the C++ shared modules automatically.

## Run

```bash
daslang.exe -project_root . furier_opengl_imgui_example.das
```

## Requirements

- daslang SDK (built with `DAS_GLFW_DISABLED=OFF`)
- CMake 3.16+
- C++17 compiler
- OpenGL
