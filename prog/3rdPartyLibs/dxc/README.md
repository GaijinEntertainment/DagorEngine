# dxc

API header for the DirectX Shader Compiler (DXC), used to invoke the DXC
compiler DLL for HLSL to DXIL/SPIR-V shader compilation.

Upstream: Microsoft DirectXShaderCompiler,
https://github.com/microsoft/DirectXShaderCompiler
Vendored file: include/dxc/dxcapi.h, matches release tag v1.7.2207
(minor local adjustments: Apple UUID emulation define, WinAdapter.h include path).
