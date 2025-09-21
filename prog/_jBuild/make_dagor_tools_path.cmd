@echo off
rem detect windows host architecture and build path to proper tools in DAGOR_CDK_DIR
set ARCH_SUFFIX=x86_64
if "%PROCESSOR_ARCHITECTURE%" == "ARM64" (
  set ARCH_SUFFIX=arm64
) else (
  if not "%PROCESSOR_IDENTIFIER:(64-bit)=%" == "%PROCESSOR_IDENTIFIER%" (
    if not "%PROCESSOR_IDENTIFIER:ARMv=%" == "%PROCESSOR_IDENTIFIER%" (
      set ARCH_SUFFIX=arm64
    )
  )
)
set DAGOR_CDK_DIR=%~dp0..\..\tools\dagor_cdk\windows-%ARCH_SUFFIX%
