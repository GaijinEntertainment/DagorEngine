# HLSLcc
DirectX shader bytecode cross compiler.

Originally based on https://github.com/James-Jones/HLSLCrossCompiler.

This library takes DirectX bytecode as input, and translates it into the following languages:
- GLSL (OpenGL 3.2 and later)
- GLSL ES (OpenGL ES 3.0 and later)
- GLSL for Vulkan consumption (as input for Glslang to generate SPIR-V)
- Metal Shading Language

This library is used to generate all shaders in Unity for OpenGL, OpenGL ES 3.0+, Metal and Vulkan.

Changes from original HLSLCrossCompiler:
- Codebase changed to C++11, with major code reorganizations.
- Support for multiple language output backends (currently ToGLSL and ToMetal)
- Metal language output support
- Temp register type analysis: In DX bytecode the registers are typeless 32-bit 4-vectors. We do code analysis to infer the actual data types (to prevent the need for tons of bitcasts).
- Loop transformation: Detect constructs that look like for-loops and transform them back to their original form
- Support for partial precision variables in HLSL (min16float etc). Do extra analysis pass to infer the intended precision of samplers.
- Reflection interface to retrieve the shader inputs and their types.
- Lots of workarounds for various driver/shader compiler bugs.
- Lots of minor fixes and improvements for correctness
- Lots of Unity-specific tweaks to allow extending HLSL without having to change the D3D compiler itself.

## Note

This project is originally integrated into the Unity build systems. However, building this library should be fairly straightforward: just compile `src/*.cpp` (in C++11 mode!) and `src/cbstring/*.c` with the following include paths:

- include
- src/internal_includes
- src/cbstrinc
- src 

Alternatively, a CMakeLists.txt is provided to build the project using cmake.

The main entry point is TranslateHLSLFromMem() function in HLSLcc.cpp (taking DX bytecode as input).


## Contributors
- Mikko Strandborg
- Juho Oravainen
- David Rogers
- Marton Ekler
- Antti Tapaninen
- Florian Penzkofer
- Alexey Orlov
- Povilas Kanapickas

## License

MIT license for HLSLcc itself, BSD license for the bstring library. See license.txt.
