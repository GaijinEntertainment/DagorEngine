// Copyright (C) Gaijin Games KFT.  All rights reserved.

// auto generated, do not modify
//-V::1020
#pragma once
#include "../publicInclude/spirv/traits_table.h"
namespace spirv
{ // binary decoders
template <typename LC, typename ECB>
inline bool loadExtendedGLSLstd450(IdResult result_id, IdResultType result_type, GLSLstd450 opcode, detail::Operands &operands,
  LC load_context, ECB on_error)
{
  switch (opcode)
  {
    default:
      if (on_error("unknown opcode for extended grammar 'GLSL.std.450'"))
        return false;
      break;
    case GLSLstd450::Round:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Round"))
        return false;
      load_context.onGLSLstd450Round(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::RoundEven:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:RoundEven"))
        return false;
      load_context.onGLSLstd450RoundEven(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Trunc:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Trunc"))
        return false;
      load_context.onGLSLstd450Trunc(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::FAbs:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:FAbs"))
        return false;
      load_context.onGLSLstd450FAbs(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::SAbs:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:SAbs"))
        return false;
      load_context.onGLSLstd450SAbs(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::FSign:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:FSign"))
        return false;
      load_context.onGLSLstd450FSign(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::SSign:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:SSign"))
        return false;
      load_context.onGLSLstd450SSign(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Floor:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Floor"))
        return false;
      load_context.onGLSLstd450Floor(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Ceil:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Ceil"))
        return false;
      load_context.onGLSLstd450Ceil(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Fract:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Fract"))
        return false;
      load_context.onGLSLstd450Fract(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Radians:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Radians"))
        return false;
      load_context.onGLSLstd450Radians(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Degrees:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Degrees"))
        return false;
      load_context.onGLSLstd450Degrees(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Sin:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Sin"))
        return false;
      load_context.onGLSLstd450Sin(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Cos:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Cos"))
        return false;
      load_context.onGLSLstd450Cos(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Tan:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Tan"))
        return false;
      load_context.onGLSLstd450Tan(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Asin:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Asin"))
        return false;
      load_context.onGLSLstd450Asin(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Acos:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Acos"))
        return false;
      load_context.onGLSLstd450Acos(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Atan:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Atan"))
        return false;
      load_context.onGLSLstd450Atan(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Sinh:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Sinh"))
        return false;
      load_context.onGLSLstd450Sinh(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Cosh:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Cosh"))
        return false;
      load_context.onGLSLstd450Cosh(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Tanh:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Tanh"))
        return false;
      load_context.onGLSLstd450Tanh(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Asinh:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Asinh"))
        return false;
      load_context.onGLSLstd450Asinh(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Acosh:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Acosh"))
        return false;
      load_context.onGLSLstd450Acosh(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Atanh:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Atanh"))
        return false;
      load_context.onGLSLstd450Atanh(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Atan2:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Atan2"))
        return false;
      load_context.onGLSLstd450Atan2(opcode, result_id, result_type, c0, c1);
      break;
    }
    case GLSLstd450::Pow:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Pow"))
        return false;
      load_context.onGLSLstd450Pow(opcode, result_id, result_type, c0, c1);
      break;
    }
    case GLSLstd450::Exp:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Exp"))
        return false;
      load_context.onGLSLstd450Exp(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Log:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Log"))
        return false;
      load_context.onGLSLstd450Log(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Exp2:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Exp2"))
        return false;
      load_context.onGLSLstd450Exp2(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Log2:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Log2"))
        return false;
      load_context.onGLSLstd450Log2(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Sqrt:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Sqrt"))
        return false;
      load_context.onGLSLstd450Sqrt(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::InverseSqrt:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:InverseSqrt"))
        return false;
      load_context.onGLSLstd450InverseSqrt(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Determinant:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Determinant"))
        return false;
      load_context.onGLSLstd450Determinant(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::MatrixInverse:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:MatrixInverse"))
        return false;
      load_context.onGLSLstd450MatrixInverse(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Modf:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Modf"))
        return false;
      load_context.onGLSLstd450Modf(opcode, result_id, result_type, c0, c1);
      break;
    }
    case GLSLstd450::ModfStruct:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:ModfStruct"))
        return false;
      load_context.onGLSLstd450ModfStruct(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::FMin:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:FMin"))
        return false;
      load_context.onGLSLstd450FMin(opcode, result_id, result_type, c0, c1);
      break;
    }
    case GLSLstd450::UMin:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:UMin"))
        return false;
      load_context.onGLSLstd450UMin(opcode, result_id, result_type, c0, c1);
      break;
    }
    case GLSLstd450::SMin:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:SMin"))
        return false;
      load_context.onGLSLstd450SMin(opcode, result_id, result_type, c0, c1);
      break;
    }
    case GLSLstd450::FMax:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:FMax"))
        return false;
      load_context.onGLSLstd450FMax(opcode, result_id, result_type, c0, c1);
      break;
    }
    case GLSLstd450::UMax:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:UMax"))
        return false;
      load_context.onGLSLstd450UMax(opcode, result_id, result_type, c0, c1);
      break;
    }
    case GLSLstd450::SMax:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:SMax"))
        return false;
      load_context.onGLSLstd450SMax(opcode, result_id, result_type, c0, c1);
      break;
    }
    case GLSLstd450::FClamp:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      auto c2 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:FClamp"))
        return false;
      load_context.onGLSLstd450FClamp(opcode, result_id, result_type, c0, c1, c2);
      break;
    }
    case GLSLstd450::UClamp:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      auto c2 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:UClamp"))
        return false;
      load_context.onGLSLstd450UClamp(opcode, result_id, result_type, c0, c1, c2);
      break;
    }
    case GLSLstd450::SClamp:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      auto c2 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:SClamp"))
        return false;
      load_context.onGLSLstd450SClamp(opcode, result_id, result_type, c0, c1, c2);
      break;
    }
    case GLSLstd450::FMix:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      auto c2 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:FMix"))
        return false;
      load_context.onGLSLstd450FMix(opcode, result_id, result_type, c0, c1, c2);
      break;
    }
    case GLSLstd450::IMix:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      auto c2 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:IMix"))
        return false;
      load_context.onGLSLstd450IMix(opcode, result_id, result_type, c0, c1, c2);
      break;
    }
    case GLSLstd450::Step:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Step"))
        return false;
      load_context.onGLSLstd450Step(opcode, result_id, result_type, c0, c1);
      break;
    }
    case GLSLstd450::SmoothStep:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      auto c2 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:SmoothStep"))
        return false;
      load_context.onGLSLstd450SmoothStep(opcode, result_id, result_type, c0, c1, c2);
      break;
    }
    case GLSLstd450::Fma:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      auto c2 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Fma"))
        return false;
      load_context.onGLSLstd450Fma(opcode, result_id, result_type, c0, c1, c2);
      break;
    }
    case GLSLstd450::Frexp:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Frexp"))
        return false;
      load_context.onGLSLstd450Frexp(opcode, result_id, result_type, c0, c1);
      break;
    }
    case GLSLstd450::FrexpStruct:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:FrexpStruct"))
        return false;
      load_context.onGLSLstd450FrexpStruct(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Ldexp:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Ldexp"))
        return false;
      load_context.onGLSLstd450Ldexp(opcode, result_id, result_type, c0, c1);
      break;
    }
    case GLSLstd450::PackSnorm4x8:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:PackSnorm4x8"))
        return false;
      load_context.onGLSLstd450PackSnorm4x8(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::PackUnorm4x8:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:PackUnorm4x8"))
        return false;
      load_context.onGLSLstd450PackUnorm4x8(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::PackSnorm2x16:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:PackSnorm2x16"))
        return false;
      load_context.onGLSLstd450PackSnorm2x16(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::PackUnorm2x16:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:PackUnorm2x16"))
        return false;
      load_context.onGLSLstd450PackUnorm2x16(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::PackHalf2x16:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:PackHalf2x16"))
        return false;
      load_context.onGLSLstd450PackHalf2x16(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::PackDouble2x32:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:PackDouble2x32"))
        return false;
      load_context.onGLSLstd450PackDouble2x32(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::UnpackSnorm2x16:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:UnpackSnorm2x16"))
        return false;
      load_context.onGLSLstd450UnpackSnorm2x16(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::UnpackUnorm2x16:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:UnpackUnorm2x16"))
        return false;
      load_context.onGLSLstd450UnpackUnorm2x16(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::UnpackHalf2x16:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:UnpackHalf2x16"))
        return false;
      load_context.onGLSLstd450UnpackHalf2x16(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::UnpackSnorm4x8:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:UnpackSnorm4x8"))
        return false;
      load_context.onGLSLstd450UnpackSnorm4x8(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::UnpackUnorm4x8:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:UnpackUnorm4x8"))
        return false;
      load_context.onGLSLstd450UnpackUnorm4x8(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::UnpackDouble2x32:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:UnpackDouble2x32"))
        return false;
      load_context.onGLSLstd450UnpackDouble2x32(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Length:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Length"))
        return false;
      load_context.onGLSLstd450Length(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::Distance:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Distance"))
        return false;
      load_context.onGLSLstd450Distance(opcode, result_id, result_type, c0, c1);
      break;
    }
    case GLSLstd450::Cross:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Cross"))
        return false;
      load_context.onGLSLstd450Cross(opcode, result_id, result_type, c0, c1);
      break;
    }
    case GLSLstd450::Normalize:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Normalize"))
        return false;
      load_context.onGLSLstd450Normalize(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::FaceForward:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      auto c2 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:FaceForward"))
        return false;
      load_context.onGLSLstd450FaceForward(opcode, result_id, result_type, c0, c1, c2);
      break;
    }
    case GLSLstd450::Reflect:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Reflect"))
        return false;
      load_context.onGLSLstd450Reflect(opcode, result_id, result_type, c0, c1);
      break;
    }
    case GLSLstd450::Refract:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      auto c2 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:Refract"))
        return false;
      load_context.onGLSLstd450Refract(opcode, result_id, result_type, c0, c1, c2);
      break;
    }
    case GLSLstd450::FindILsb:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:FindILsb"))
        return false;
      load_context.onGLSLstd450FindILsb(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::FindSMsb:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:FindSMsb"))
        return false;
      load_context.onGLSLstd450FindSMsb(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::FindUMsb:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:FindUMsb"))
        return false;
      load_context.onGLSLstd450FindUMsb(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::InterpolateAtCentroid:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:InterpolateAtCentroid"))
        return false;
      load_context.onGLSLstd450InterpolateAtCentroid(opcode, result_id, result_type, c0);
      break;
    }
    case GLSLstd450::InterpolateAtSample:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:InterpolateAtSample"))
        return false;
      load_context.onGLSLstd450InterpolateAtSample(opcode, result_id, result_type, c0, c1);
      break;
    }
    case GLSLstd450::InterpolateAtOffset:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:InterpolateAtOffset"))
        return false;
      load_context.onGLSLstd450InterpolateAtOffset(opcode, result_id, result_type, c0, c1);
      break;
    }
    case GLSLstd450::NMin:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:NMin"))
        return false;
      load_context.onGLSLstd450NMin(opcode, result_id, result_type, c0, c1);
      break;
    }
    case GLSLstd450::NMax:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:NMax"))
        return false;
      load_context.onGLSLstd450NMax(opcode, result_id, result_type, c0, c1);
      break;
    }
    case GLSLstd450::NClamp:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      auto c2 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "GLSL.std.450:NClamp"))
        return false;
      load_context.onGLSLstd450NClamp(opcode, result_id, result_type, c0, c1, c2);
      break;
    }
  };
  return true;
}
template <typename LC, typename ECB>
inline bool loadExtendedAMDGcnShader(IdResult result_id, IdResultType result_type, AMDGcnShader opcode, detail::Operands &operands,
  LC load_context, ECB on_error)
{
  switch (opcode)
  {
    default:
      if (on_error("unknown opcode for extended grammar 'SPV_AMD_gcn_shader'"))
        return false;
      break;
    case AMDGcnShader::CubeFaceIndexAMD:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "SPV_AMD_gcn_shader:CubeFaceIndexAMD"))
        return false;
      load_context.onAMDGcnShaderCubeFaceIndex(opcode, result_id, result_type, c0);
      break;
    }
    case AMDGcnShader::CubeFaceCoordAMD:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "SPV_AMD_gcn_shader:CubeFaceCoordAMD"))
        return false;
      load_context.onAMDGcnShaderCubeFaceCoord(opcode, result_id, result_type, c0);
      break;
    }
    case AMDGcnShader::TimeAMD:
    {
      if (!operands.finishOperands(on_error, "SPV_AMD_gcn_shader:TimeAMD"))
        return false;
      load_context.onAMDGcnShaderTime(opcode, result_id, result_type);
      break;
    }
  };
  return true;
}
template <typename LC, typename ECB>
inline bool loadExtendedAMDShaderBallot(IdResult result_id, IdResultType result_type, AMDShaderBallot opcode,
  detail::Operands &operands, LC load_context, ECB on_error)
{
  switch (opcode)
  {
    default:
      if (on_error("unknown opcode for extended grammar 'SPV_AMD_shader_ballot'"))
        return false;
      break;
    case AMDShaderBallot::SwizzleInvocationsAMD:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "SPV_AMD_shader_ballot:SwizzleInvocationsAMD"))
        return false;
      load_context.onAMDShaderBallotSwizzleInvocations(opcode, result_id, result_type, c0, c1);
      break;
    }
    case AMDShaderBallot::SwizzleInvocationsMaskedAMD:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "SPV_AMD_shader_ballot:SwizzleInvocationsMaskedAMD"))
        return false;
      load_context.onAMDShaderBallotSwizzleInvocationsMasked(opcode, result_id, result_type, c0, c1);
      break;
    }
    case AMDShaderBallot::WriteInvocationAMD:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      auto c2 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "SPV_AMD_shader_ballot:WriteInvocationAMD"))
        return false;
      load_context.onAMDShaderBallotWriteInvocation(opcode, result_id, result_type, c0, c1, c2);
      break;
    }
    case AMDShaderBallot::MbcntAMD:
    {
      auto c0 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "SPV_AMD_shader_ballot:MbcntAMD"))
        return false;
      load_context.onAMDShaderBallotMbcnt(opcode, result_id, result_type, c0);
      break;
    }
  };
  return true;
}
template <typename LC, typename ECB>
inline bool loadExtendedAMDShaderExplicitVertexParameter(IdResult result_id, IdResultType result_type,
  AMDShaderExplicitVertexParameter opcode, detail::Operands &operands, LC load_context, ECB on_error)
{
  switch (opcode)
  {
    default:
      if (on_error("unknown opcode for extended grammar 'SPV_AMD_shader_explicit_vertex_parameter'"))
        return false;
      break;
    case AMDShaderExplicitVertexParameter::InterpolateAtVertexAMD:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "SPV_AMD_shader_explicit_vertex_parameter:InterpolateAtVertexAMD"))
        return false;
      load_context.onAMDShaderExplicitVertexParameterInterpolateAtVertex(opcode, result_id, result_type, c0, c1);
      break;
    }
  };
  return true;
}
template <typename LC, typename ECB>
inline bool loadExtendedAMDShaderTrinaryMinmax(IdResult result_id, IdResultType result_type, AMDShaderTrinaryMinmax opcode,
  detail::Operands &operands, LC load_context, ECB on_error)
{
  switch (opcode)
  {
    default:
      if (on_error("unknown opcode for extended grammar 'SPV_AMD_shader_trinary_minmax'"))
        return false;
      break;
    case AMDShaderTrinaryMinmax::FMin3AMD:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      auto c2 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "SPV_AMD_shader_trinary_minmax:FMin3AMD"))
        return false;
      load_context.onAMDShaderTrinaryMinmaxFMin3(opcode, result_id, result_type, c0, c1, c2);
      break;
    }
    case AMDShaderTrinaryMinmax::UMin3AMD:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      auto c2 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "SPV_AMD_shader_trinary_minmax:UMin3AMD"))
        return false;
      load_context.onAMDShaderTrinaryMinmaxUMin3(opcode, result_id, result_type, c0, c1, c2);
      break;
    }
    case AMDShaderTrinaryMinmax::SMin3AMD:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      auto c2 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "SPV_AMD_shader_trinary_minmax:SMin3AMD"))
        return false;
      load_context.onAMDShaderTrinaryMinmaxSMin3(opcode, result_id, result_type, c0, c1, c2);
      break;
    }
    case AMDShaderTrinaryMinmax::FMax3AMD:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      auto c2 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "SPV_AMD_shader_trinary_minmax:FMax3AMD"))
        return false;
      load_context.onAMDShaderTrinaryMinmaxFMax3(opcode, result_id, result_type, c0, c1, c2);
      break;
    }
    case AMDShaderTrinaryMinmax::UMax3AMD:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      auto c2 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "SPV_AMD_shader_trinary_minmax:UMax3AMD"))
        return false;
      load_context.onAMDShaderTrinaryMinmaxUMax3(opcode, result_id, result_type, c0, c1, c2);
      break;
    }
    case AMDShaderTrinaryMinmax::SMax3AMD:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      auto c2 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "SPV_AMD_shader_trinary_minmax:SMax3AMD"))
        return false;
      load_context.onAMDShaderTrinaryMinmaxSMax3(opcode, result_id, result_type, c0, c1, c2);
      break;
    }
    case AMDShaderTrinaryMinmax::FMid3AMD:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      auto c2 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "SPV_AMD_shader_trinary_minmax:FMid3AMD"))
        return false;
      load_context.onAMDShaderTrinaryMinmaxFMid3(opcode, result_id, result_type, c0, c1, c2);
      break;
    }
    case AMDShaderTrinaryMinmax::UMid3AMD:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      auto c2 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "SPV_AMD_shader_trinary_minmax:UMid3AMD"))
        return false;
      load_context.onAMDShaderTrinaryMinmaxUMid3(opcode, result_id, result_type, c0, c1, c2);
      break;
    }
    case AMDShaderTrinaryMinmax::SMid3AMD:
    {
      auto c0 = operands.read<IdRef>();
      auto c1 = operands.read<IdRef>();
      auto c2 = operands.read<IdRef>();
      if (!operands.finishOperands(on_error, "SPV_AMD_shader_trinary_minmax:SMid3AMD"))
        return false;
      load_context.onAMDShaderTrinaryMinmaxSMid3(opcode, result_id, result_type, c0, c1, c2);
      break;
    }
  };
  return true;
}
template <typename HCB, typename LC, typename ECB>
inline bool load(const Id *words, Id word_count, HCB on_header, LC load_context, ECB on_error)
{
  if (word_count * sizeof(Id) < sizeof(FileHeader))
  {
    on_error("Input is not a spir-v file, no enough words to process");
    return false;
  }
  if (!on_header(*reinterpret_cast<const FileHeader *>(words)))
    return true;
  auto at = reinterpret_cast<const Id *>(reinterpret_cast<const FileHeader *>(words) + 1);
  auto ed = words + word_count;
  while (at < ed)
  {
    auto op = static_cast<Op>(*at & OpCodeMask);
    auto len = *at >> WordCountShift;
    load_context.setAt(at - words);
    detail::Operands operands{at + 1, len - 1};
    switch (op)
    {
      default:
        // if on_error returns true we stop
        if (on_error("unknown op code encountered"))
          return false;
        break;
      case Op::OpNop:
      {
        if (!operands.finishOperands(on_error, "OpNop"))
          return false;
        load_context.onNop(Op::OpNop);
        break;
      }
      case Op::OpUndef:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpUndef"))
          return false;
        load_context.onUndef(Op::OpUndef, c1, c0);
        break;
      }
      case Op::OpSourceContinued:
      {
        auto c0 = operands.read<LiteralString>();
        if (!operands.finishOperands(on_error, "OpSourceContinued"))
          return false;
        load_context.onSourceContinued(Op::OpSourceContinued, c0);
        break;
      }
      case Op::OpSource:
      {
        auto c0 = operands.read<SourceLanguage>();
        auto c1 = operands.read<LiteralInteger>();
        auto c2 = operands.read<Optional<IdRef>>();
        auto c3 = operands.read<Optional<LiteralString>>();
        if (!operands.finishOperands(on_error, "OpSource"))
          return false;
        load_context.onSource(Op::OpSource, c0, c1, c2, c3);
        break;
      }
      case Op::OpSourceExtension:
      {
        auto c0 = operands.read<LiteralString>();
        if (!operands.finishOperands(on_error, "OpSourceExtension"))
          return false;
        load_context.onSourceExtension(Op::OpSourceExtension, c0);
        break;
      }
      case Op::OpName:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<LiteralString>();
        if (!operands.finishOperands(on_error, "OpName"))
          return false;
        load_context.onName(Op::OpName, c0, c1);
        break;
      }
      case Op::OpMemberName:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<LiteralInteger>();
        auto c2 = operands.read<LiteralString>();
        if (!operands.finishOperands(on_error, "OpMemberName"))
          return false;
        load_context.onMemberName(Op::OpMemberName, c0, c1, c2);
        break;
      }
      case Op::OpString:
      {
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<LiteralString>();
        if (!operands.finishOperands(on_error, "OpString"))
          return false;
        load_context.onString(Op::OpString, c0, c1);
        break;
      }
      case Op::OpLine:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<LiteralInteger>();
        auto c2 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpLine"))
          return false;
        load_context.onLine(Op::OpLine, c0, c1, c2);
        break;
      }
      case Op::OpExtension:
      {
        // enable extension / extended grammar related features
        auto c0 = operands.read<LiteralString>();
        auto extId = extension_name_to_id(c0.asString(), c0.length);
        if (!operands.finishOperands(on_error, "0"))
          return false;
        load_context.enableExtension(extId, c0.asString(), c0.length);
        break;
      }
      case Op::OpExtInstImport:
      {
        // load extended grammar functions into id
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<LiteralString>();
        auto extId = extended_grammar_name_to_id(c1.asString(), c1.length);
        if (!operands.finishOperands(on_error, "OpExtInstImport"))
          return false;
        load_context.loadExtendedGrammar(c0, extId, c1.asString(), c1.length);
        break;
      }
      case Op::OpExtInst:
      {
        // call extended grammar function
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralExtInstInteger>();
        auto extId = load_context.getExtendedGrammarIdentifierFromRefId(c2);
        switch (extId)
        {
          default:
            if (on_error("error while looking up extended grammar identifier"))
              return false;
            break;
          case ExtendedGrammar::GLSL_std_450:
            if (!loadExtendedGLSLstd450(c1, c0, static_cast<GLSLstd450>(c3.value), operands, load_context, on_error))
              return false;
            break;
          case ExtendedGrammar::AMD_gcn_shader:
            if (!loadExtendedAMDGcnShader(c1, c0, static_cast<AMDGcnShader>(c3.value), operands, load_context, on_error))
              return false;
            break;
          case ExtendedGrammar::AMD_shader_ballot:
            if (!loadExtendedAMDShaderBallot(c1, c0, static_cast<AMDShaderBallot>(c3.value), operands, load_context, on_error))
              return false;
            break;
          case ExtendedGrammar::AMD_shader_explicit_vertex_parameter:
            if (!loadExtendedAMDShaderExplicitVertexParameter(c1, c0, static_cast<AMDShaderExplicitVertexParameter>(c3.value),
                  operands, load_context, on_error))
              return false;
            break;
          case ExtendedGrammar::AMD_shader_trinary_minmax:
            if (!loadExtendedAMDShaderTrinaryMinmax(c1, c0, static_cast<AMDShaderTrinaryMinmax>(c3.value), operands, load_context,
                  on_error))
              return false;
            break;
        };
        break;
      }
      case Op::OpMemoryModel:
      {
        auto c0 = operands.read<AddressingModel>();
        auto c1 = operands.read<MemoryModel>();
        if (!operands.finishOperands(on_error, "OpMemoryModel"))
          return false;
        load_context.onMemoryModel(Op::OpMemoryModel, c0, c1);
        break;
      }
      case Op::OpEntryPoint:
      {
        auto c0 = operands.read<ExecutionModel>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<LiteralString>();
        auto c3 = operands.read<Multiple<IdRef>>();
        if (!operands.finishOperands(on_error, "OpEntryPoint"))
          return false;
        load_context.onEntryPoint(Op::OpEntryPoint, c0, c1, c2, c3);
        break;
      }
      case Op::OpExecutionMode:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<ExecutionMode>();
        if (!operands.finishOperands(on_error, "OpExecutionMode"))
          return false;
        load_context.onExecutionMode(Op::OpExecutionMode, c0, c1);
        break;
      }
      case Op::OpCapability:
      {
        auto c0 = operands.read<Capability>();
        if (!operands.finishOperands(on_error, "OpCapability"))
          return false;
        load_context.onCapability(Op::OpCapability, c0);
        break;
      }
      case Op::OpTypeVoid:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeVoid"))
          return false;
        load_context.onTypeVoid(Op::OpTypeVoid, c0);
        break;
      }
      case Op::OpTypeBool:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeBool"))
          return false;
        load_context.onTypeBool(Op::OpTypeBool, c0);
        break;
      }
      case Op::OpTypeInt:
      {
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<LiteralInteger>();
        auto c2 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpTypeInt"))
          return false;
        load_context.onTypeInt(Op::OpTypeInt, c0, c1, c2);
        break;
      }
      case Op::OpTypeFloat:
      {
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpTypeFloat"))
          return false;
        load_context.onTypeFloat(Op::OpTypeFloat, c0, c1);
        break;
      }
      case Op::OpTypeVector:
      {
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpTypeVector"))
          return false;
        load_context.onTypeVector(Op::OpTypeVector, c0, c1, c2);
        break;
      }
      case Op::OpTypeMatrix:
      {
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpTypeMatrix"))
          return false;
        load_context.onTypeMatrix(Op::OpTypeMatrix, c0, c1, c2);
        break;
      }
      case Op::OpTypeImage:
      {
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<Dim>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<ImageFormat>();
        auto c8 = operands.read<Optional<AccessQualifier>>();
        if (!operands.finishOperands(on_error, "OpTypeImage"))
          return false;
        load_context.onTypeImage(Op::OpTypeImage, c0, c1, c2, c3, c4, c5, c6, c7, c8);
        break;
      }
      case Op::OpTypeSampler:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeSampler"))
          return false;
        load_context.onTypeSampler(Op::OpTypeSampler, c0);
        break;
      }
      case Op::OpTypeSampledImage:
      {
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpTypeSampledImage"))
          return false;
        load_context.onTypeSampledImage(Op::OpTypeSampledImage, c0, c1);
        break;
      }
      case Op::OpTypeArray:
      {
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpTypeArray"))
          return false;
        load_context.onTypeArray(Op::OpTypeArray, c0, c1, c2);
        break;
      }
      case Op::OpTypeRuntimeArray:
      {
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpTypeRuntimeArray"))
          return false;
        load_context.onTypeRuntimeArray(Op::OpTypeRuntimeArray, c0, c1);
        break;
      }
      case Op::OpTypeStruct:
      {
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<Multiple<IdRef>>();
        if (!operands.finishOperands(on_error, "OpTypeStruct"))
          return false;
        load_context.onTypeStruct(Op::OpTypeStruct, c0, c1);
        break;
      }
      case Op::OpTypeOpaque:
      {
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<LiteralString>();
        if (!operands.finishOperands(on_error, "OpTypeOpaque"))
          return false;
        load_context.onTypeOpaque(Op::OpTypeOpaque, c0, c1);
        break;
      }
      case Op::OpTypePointer:
      {
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<StorageClass>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpTypePointer"))
          return false;
        load_context.onTypePointer(Op::OpTypePointer, c0, c1, c2);
        break;
      }
      case Op::OpTypeFunction:
      {
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<Multiple<IdRef>>();
        if (!operands.finishOperands(on_error, "OpTypeFunction"))
          return false;
        load_context.onTypeFunction(Op::OpTypeFunction, c0, c1, c2);
        break;
      }
      case Op::OpTypeEvent:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeEvent"))
          return false;
        load_context.onTypeEvent(Op::OpTypeEvent, c0);
        break;
      }
      case Op::OpTypeDeviceEvent:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeDeviceEvent"))
          return false;
        load_context.onTypeDeviceEvent(Op::OpTypeDeviceEvent, c0);
        break;
      }
      case Op::OpTypeReserveId:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeReserveId"))
          return false;
        load_context.onTypeReserveId(Op::OpTypeReserveId, c0);
        break;
      }
      case Op::OpTypeQueue:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeQueue"))
          return false;
        load_context.onTypeQueue(Op::OpTypeQueue, c0);
        break;
      }
      case Op::OpTypePipe:
      {
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<AccessQualifier>();
        if (!operands.finishOperands(on_error, "OpTypePipe"))
          return false;
        load_context.onTypePipe(Op::OpTypePipe, c0, c1);
        break;
      }
      case Op::OpTypeForwardPointer:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<StorageClass>();
        if (!operands.finishOperands(on_error, "OpTypeForwardPointer"))
          return false;
        load_context.onTypeForwardPointer(Op::OpTypeForwardPointer, c0, c1);
        break;
      }
      case Op::OpConstantTrue:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpConstantTrue"))
          return false;
        load_context.onConstantTrue(Op::OpConstantTrue, c1, c0);
        break;
      }
      case Op::OpConstantFalse:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpConstantFalse"))
          return false;
        load_context.onConstantFalse(Op::OpConstantFalse, c1, c0);
        break;
      }
      case Op::OpConstant:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        operands.setLiteralContextSize(load_context.getContextDependentSize(c0));
        auto c2 = operands.read<LiteralContextDependentNumber>();
        if (!operands.finishOperands(on_error, "OpConstant"))
          return false;
        load_context.onConstant(Op::OpConstant, c1, c0, c2);
        break;
      }
      case Op::OpConstantComposite:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<Multiple<IdRef>>();
        if (!operands.finishOperands(on_error, "OpConstantComposite"))
          return false;
        load_context.onConstantComposite(Op::OpConstantComposite, c1, c0, c2);
        break;
      }
      case Op::OpConstantSampler:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<SamplerAddressingMode>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<SamplerFilterMode>();
        if (!operands.finishOperands(on_error, "OpConstantSampler"))
          return false;
        load_context.onConstantSampler(Op::OpConstantSampler, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpConstantNull:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpConstantNull"))
          return false;
        load_context.onConstantNull(Op::OpConstantNull, c1, c0);
        break;
      }
      case Op::OpSpecConstantTrue:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpSpecConstantTrue"))
          return false;
        load_context.onSpecConstantTrue(Op::OpSpecConstantTrue, c1, c0);
        break;
      }
      case Op::OpSpecConstantFalse:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpSpecConstantFalse"))
          return false;
        load_context.onSpecConstantFalse(Op::OpSpecConstantFalse, c1, c0);
        break;
      }
      case Op::OpSpecConstant:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        operands.setLiteralContextSize(load_context.getContextDependentSize(c0));
        auto c2 = operands.read<LiteralContextDependentNumber>();
        if (!operands.finishOperands(on_error, "OpSpecConstant"))
          return false;
        load_context.onSpecConstant(Op::OpSpecConstant, c1, c0, c2);
        break;
      }
      case Op::OpSpecConstantComposite:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<Multiple<IdRef>>();
        if (!operands.finishOperands(on_error, "OpSpecConstantComposite"))
          return false;
        load_context.onSpecConstantComposite(Op::OpSpecConstantComposite, c1, c0, c2);
        break;
      }
      case Op::OpSpecConstantOp:
      {
        // number of operands depend on the opcode it uses
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<LiteralSpecConstantOpInteger>();
        switch (static_cast<Op>(c2.value))
        {
          default:
            if (on_error("unexpected spec-op operation"))
              return false;
          case Op::OpAccessChain:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<Multiple<IdRef>>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpAccessChain"))
              return false;
            load_context.onSpecAccessChain(Op::OpAccessChain, c1, c0, c4, c5);
            break;
          }
          case Op::OpInBoundsAccessChain:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<Multiple<IdRef>>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpInBoundsAccessChain"))
              return false;
            load_context.onSpecInBoundsAccessChain(Op::OpInBoundsAccessChain, c1, c0, c4, c5);
            break;
          }
          case Op::OpPtrAccessChain:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            auto c6 = operands.read<Multiple<IdRef>>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpPtrAccessChain"))
              return false;
            load_context.onSpecPtrAccessChain(Op::OpPtrAccessChain, c1, c0, c4, c5, c6);
            break;
          }
          case Op::OpInBoundsPtrAccessChain:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            auto c6 = operands.read<Multiple<IdRef>>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpInBoundsPtrAccessChain"))
              return false;
            load_context.onSpecInBoundsPtrAccessChain(Op::OpInBoundsPtrAccessChain, c1, c0, c4, c5, c6);
            break;
          }
          case Op::OpVectorShuffle:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            auto c6 = operands.read<Multiple<LiteralInteger>>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpVectorShuffle"))
              return false;
            load_context.onSpecVectorShuffle(Op::OpVectorShuffle, c1, c0, c4, c5, c6);
            break;
          }
          case Op::OpCompositeExtract:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<Multiple<LiteralInteger>>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpCompositeExtract"))
              return false;
            load_context.onSpecCompositeExtract(Op::OpCompositeExtract, c1, c0, c4, c5);
            break;
          }
          case Op::OpCompositeInsert:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            auto c6 = operands.read<Multiple<LiteralInteger>>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpCompositeInsert"))
              return false;
            load_context.onSpecCompositeInsert(Op::OpCompositeInsert, c1, c0, c4, c5, c6);
            break;
          }
          case Op::OpConvertFToU:
          {
            auto c4 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpConvertFToU"))
              return false;
            load_context.onSpecConvertFToU(Op::OpConvertFToU, c1, c0, c4);
            break;
          }
          case Op::OpConvertFToS:
          {
            auto c4 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpConvertFToS"))
              return false;
            load_context.onSpecConvertFToS(Op::OpConvertFToS, c1, c0, c4);
            break;
          }
          case Op::OpConvertSToF:
          {
            auto c4 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpConvertSToF"))
              return false;
            load_context.onSpecConvertSToF(Op::OpConvertSToF, c1, c0, c4);
            break;
          }
          case Op::OpConvertUToF:
          {
            auto c4 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpConvertUToF"))
              return false;
            load_context.onSpecConvertUToF(Op::OpConvertUToF, c1, c0, c4);
            break;
          }
          case Op::OpUConvert:
          {
            auto c4 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpUConvert"))
              return false;
            load_context.onSpecUConvert(Op::OpUConvert, c1, c0, c4);
            break;
          }
          case Op::OpSConvert:
          {
            auto c4 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpSConvert"))
              return false;
            load_context.onSpecSConvert(Op::OpSConvert, c1, c0, c4);
            break;
          }
          case Op::OpFConvert:
          {
            auto c4 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpFConvert"))
              return false;
            load_context.onSpecFConvert(Op::OpFConvert, c1, c0, c4);
            break;
          }
          case Op::OpQuantizeToF16:
          {
            auto c4 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpQuantizeToF16"))
              return false;
            load_context.onSpecQuantizeToF16(Op::OpQuantizeToF16, c1, c0, c4);
            break;
          }
          case Op::OpConvertPtrToU:
          {
            auto c4 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpConvertPtrToU"))
              return false;
            load_context.onSpecConvertPtrToU(Op::OpConvertPtrToU, c1, c0, c4);
            break;
          }
          case Op::OpConvertUToPtr:
          {
            auto c4 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpConvertUToPtr"))
              return false;
            load_context.onSpecConvertUToPtr(Op::OpConvertUToPtr, c1, c0, c4);
            break;
          }
          case Op::OpPtrCastToGeneric:
          {
            auto c4 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpPtrCastToGeneric"))
              return false;
            load_context.onSpecPtrCastToGeneric(Op::OpPtrCastToGeneric, c1, c0, c4);
            break;
          }
          case Op::OpGenericCastToPtr:
          {
            auto c4 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpGenericCastToPtr"))
              return false;
            load_context.onSpecGenericCastToPtr(Op::OpGenericCastToPtr, c1, c0, c4);
            break;
          }
          case Op::OpBitcast:
          {
            auto c4 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpBitcast"))
              return false;
            load_context.onSpecBitcast(Op::OpBitcast, c1, c0, c4);
            break;
          }
          case Op::OpSNegate:
          {
            auto c4 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpSNegate"))
              return false;
            load_context.onSpecSNegate(Op::OpSNegate, c1, c0, c4);
            break;
          }
          case Op::OpFNegate:
          {
            auto c4 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpFNegate"))
              return false;
            load_context.onSpecFNegate(Op::OpFNegate, c1, c0, c4);
            break;
          }
          case Op::OpIAdd:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpIAdd"))
              return false;
            load_context.onSpecIAdd(Op::OpIAdd, c1, c0, c4, c5);
            break;
          }
          case Op::OpFAdd:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpFAdd"))
              return false;
            load_context.onSpecFAdd(Op::OpFAdd, c1, c0, c4, c5);
            break;
          }
          case Op::OpISub:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpISub"))
              return false;
            load_context.onSpecISub(Op::OpISub, c1, c0, c4, c5);
            break;
          }
          case Op::OpFSub:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpFSub"))
              return false;
            load_context.onSpecFSub(Op::OpFSub, c1, c0, c4, c5);
            break;
          }
          case Op::OpIMul:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpIMul"))
              return false;
            load_context.onSpecIMul(Op::OpIMul, c1, c0, c4, c5);
            break;
          }
          case Op::OpFMul:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpFMul"))
              return false;
            load_context.onSpecFMul(Op::OpFMul, c1, c0, c4, c5);
            break;
          }
          case Op::OpUDiv:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpUDiv"))
              return false;
            load_context.onSpecUDiv(Op::OpUDiv, c1, c0, c4, c5);
            break;
          }
          case Op::OpSDiv:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpSDiv"))
              return false;
            load_context.onSpecSDiv(Op::OpSDiv, c1, c0, c4, c5);
            break;
          }
          case Op::OpFDiv:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpFDiv"))
              return false;
            load_context.onSpecFDiv(Op::OpFDiv, c1, c0, c4, c5);
            break;
          }
          case Op::OpUMod:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpUMod"))
              return false;
            load_context.onSpecUMod(Op::OpUMod, c1, c0, c4, c5);
            break;
          }
          case Op::OpSRem:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpSRem"))
              return false;
            load_context.onSpecSRem(Op::OpSRem, c1, c0, c4, c5);
            break;
          }
          case Op::OpSMod:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpSMod"))
              return false;
            load_context.onSpecSMod(Op::OpSMod, c1, c0, c4, c5);
            break;
          }
          case Op::OpFRem:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpFRem"))
              return false;
            load_context.onSpecFRem(Op::OpFRem, c1, c0, c4, c5);
            break;
          }
          case Op::OpFMod:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpFMod"))
              return false;
            load_context.onSpecFMod(Op::OpFMod, c1, c0, c4, c5);
            break;
          }
          case Op::OpLogicalEqual:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpLogicalEqual"))
              return false;
            load_context.onSpecLogicalEqual(Op::OpLogicalEqual, c1, c0, c4, c5);
            break;
          }
          case Op::OpLogicalNotEqual:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpLogicalNotEqual"))
              return false;
            load_context.onSpecLogicalNotEqual(Op::OpLogicalNotEqual, c1, c0, c4, c5);
            break;
          }
          case Op::OpLogicalOr:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpLogicalOr"))
              return false;
            load_context.onSpecLogicalOr(Op::OpLogicalOr, c1, c0, c4, c5);
            break;
          }
          case Op::OpLogicalAnd:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpLogicalAnd"))
              return false;
            load_context.onSpecLogicalAnd(Op::OpLogicalAnd, c1, c0, c4, c5);
            break;
          }
          case Op::OpLogicalNot:
          {
            auto c4 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpLogicalNot"))
              return false;
            load_context.onSpecLogicalNot(Op::OpLogicalNot, c1, c0, c4);
            break;
          }
          case Op::OpSelect:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            auto c6 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpSelect"))
              return false;
            load_context.onSpecSelect(Op::OpSelect, c1, c0, c4, c5, c6);
            break;
          }
          case Op::OpIEqual:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpIEqual"))
              return false;
            load_context.onSpecIEqual(Op::OpIEqual, c1, c0, c4, c5);
            break;
          }
          case Op::OpINotEqual:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpINotEqual"))
              return false;
            load_context.onSpecINotEqual(Op::OpINotEqual, c1, c0, c4, c5);
            break;
          }
          case Op::OpUGreaterThan:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpUGreaterThan"))
              return false;
            load_context.onSpecUGreaterThan(Op::OpUGreaterThan, c1, c0, c4, c5);
            break;
          }
          case Op::OpSGreaterThan:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpSGreaterThan"))
              return false;
            load_context.onSpecSGreaterThan(Op::OpSGreaterThan, c1, c0, c4, c5);
            break;
          }
          case Op::OpUGreaterThanEqual:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpUGreaterThanEqual"))
              return false;
            load_context.onSpecUGreaterThanEqual(Op::OpUGreaterThanEqual, c1, c0, c4, c5);
            break;
          }
          case Op::OpSGreaterThanEqual:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpSGreaterThanEqual"))
              return false;
            load_context.onSpecSGreaterThanEqual(Op::OpSGreaterThanEqual, c1, c0, c4, c5);
            break;
          }
          case Op::OpULessThan:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpULessThan"))
              return false;
            load_context.onSpecULessThan(Op::OpULessThan, c1, c0, c4, c5);
            break;
          }
          case Op::OpSLessThan:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpSLessThan"))
              return false;
            load_context.onSpecSLessThan(Op::OpSLessThan, c1, c0, c4, c5);
            break;
          }
          case Op::OpULessThanEqual:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpULessThanEqual"))
              return false;
            load_context.onSpecULessThanEqual(Op::OpULessThanEqual, c1, c0, c4, c5);
            break;
          }
          case Op::OpSLessThanEqual:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpSLessThanEqual"))
              return false;
            load_context.onSpecSLessThanEqual(Op::OpSLessThanEqual, c1, c0, c4, c5);
            break;
          }
          case Op::OpShiftRightLogical:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpShiftRightLogical"))
              return false;
            load_context.onSpecShiftRightLogical(Op::OpShiftRightLogical, c1, c0, c4, c5);
            break;
          }
          case Op::OpShiftRightArithmetic:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpShiftRightArithmetic"))
              return false;
            load_context.onSpecShiftRightArithmetic(Op::OpShiftRightArithmetic, c1, c0, c4, c5);
            break;
          }
          case Op::OpShiftLeftLogical:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpShiftLeftLogical"))
              return false;
            load_context.onSpecShiftLeftLogical(Op::OpShiftLeftLogical, c1, c0, c4, c5);
            break;
          }
          case Op::OpBitwiseOr:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpBitwiseOr"))
              return false;
            load_context.onSpecBitwiseOr(Op::OpBitwiseOr, c1, c0, c4, c5);
            break;
          }
          case Op::OpBitwiseXor:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpBitwiseXor"))
              return false;
            load_context.onSpecBitwiseXor(Op::OpBitwiseXor, c1, c0, c4, c5);
            break;
          }
          case Op::OpBitwiseAnd:
          {
            auto c4 = operands.read<IdRef>();
            auto c5 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpBitwiseAnd"))
              return false;
            load_context.onSpecBitwiseAnd(Op::OpBitwiseAnd, c1, c0, c4, c5);
            break;
          }
          case Op::OpNot:
          {
            auto c4 = operands.read<IdRef>();
            if (!operands.finishOperands(on_error, "OpSpecConstantOp->OpNot"))
              return false;
            load_context.onSpecNot(Op::OpNot, c1, c0, c4);
            break;
          }
        };
        break;
      }
      case Op::OpFunction:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<FunctionControlMask>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFunction"))
          return false;
        load_context.onFunction(Op::OpFunction, c1, c0, c2, c3);
        break;
      }
      case Op::OpFunctionParameter:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpFunctionParameter"))
          return false;
        load_context.onFunctionParameter(Op::OpFunctionParameter, c1, c0);
        break;
      }
      case Op::OpFunctionEnd:
      {
        if (!operands.finishOperands(on_error, "OpFunctionEnd"))
          return false;
        load_context.onFunctionEnd(Op::OpFunctionEnd);
        break;
      }
      case Op::OpFunctionCall:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<Multiple<IdRef>>();
        if (!operands.finishOperands(on_error, "OpFunctionCall"))
          return false;
        load_context.onFunctionCall(Op::OpFunctionCall, c1, c0, c2, c3);
        break;
      }
      case Op::OpVariable:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<StorageClass>();
        auto c3 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpVariable"))
          return false;
        load_context.onVariable(Op::OpVariable, c1, c0, c2, c3);
        break;
      }
      case Op::OpImageTexelPointer:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpImageTexelPointer"))
          return false;
        load_context.onImageTexelPointer(Op::OpImageTexelPointer, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpLoad:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<Optional<MemoryAccessMask>>();
        if (!operands.finishOperands(on_error, "OpLoad"))
          return false;
        load_context.onLoad(Op::OpLoad, c1, c0, c2, c3);
        break;
      }
      case Op::OpStore:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<Optional<MemoryAccessMask>>();
        if (!operands.finishOperands(on_error, "OpStore"))
          return false;
        load_context.onStore(Op::OpStore, c0, c1, c2);
        break;
      }
      case Op::OpCopyMemory:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<Optional<MemoryAccessMask>>();
        auto c3 = operands.read<Optional<MemoryAccessMask>>();
        if (!operands.finishOperands(on_error, "OpCopyMemory"))
          return false;
        load_context.onCopyMemory(Op::OpCopyMemory, c0, c1, c2, c3);
        break;
      }
      case Op::OpCopyMemorySized:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<Optional<MemoryAccessMask>>();
        auto c4 = operands.read<Optional<MemoryAccessMask>>();
        if (!operands.finishOperands(on_error, "OpCopyMemorySized"))
          return false;
        load_context.onCopyMemorySized(Op::OpCopyMemorySized, c0, c1, c2, c3, c4);
        break;
      }
      case Op::OpAccessChain:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<Multiple<IdRef>>();
        if (!operands.finishOperands(on_error, "OpAccessChain"))
          return false;
        load_context.onAccessChain(Op::OpAccessChain, c1, c0, c2, c3);
        break;
      }
      case Op::OpInBoundsAccessChain:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<Multiple<IdRef>>();
        if (!operands.finishOperands(on_error, "OpInBoundsAccessChain"))
          return false;
        load_context.onInBoundsAccessChain(Op::OpInBoundsAccessChain, c1, c0, c2, c3);
        break;
      }
      case Op::OpPtrAccessChain:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<Multiple<IdRef>>();
        if (!operands.finishOperands(on_error, "OpPtrAccessChain"))
          return false;
        load_context.onPtrAccessChain(Op::OpPtrAccessChain, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpArrayLength:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArrayLength"))
          return false;
        load_context.onArrayLength(Op::OpArrayLength, c1, c0, c2, c3);
        break;
      }
      case Op::OpGenericPtrMemSemantics:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGenericPtrMemSemantics"))
          return false;
        load_context.onGenericPtrMemSemantics(Op::OpGenericPtrMemSemantics, c1, c0, c2);
        break;
      }
      case Op::OpInBoundsPtrAccessChain:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<Multiple<IdRef>>();
        if (!operands.finishOperands(on_error, "OpInBoundsPtrAccessChain"))
          return false;
        load_context.onInBoundsPtrAccessChain(Op::OpInBoundsPtrAccessChain, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpDecorate:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<Decoration>();
        if (!operands.finishOperands(on_error, "OpDecorate"))
          return false;
        load_context.onDecorate(Op::OpDecorate, c0, c1);
        break;
      }
      case Op::OpMemberDecorate:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<LiteralInteger>();
        auto c2 = operands.read<Decoration>();
        if (!operands.finishOperands(on_error, "OpMemberDecorate"))
          return false;
        load_context.onMemberDecorate(Op::OpMemberDecorate, c0, c1, c2);
        break;
      }
      case Op::OpDecorationGroup:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpDecorationGroup"))
          return false;
        load_context.onDecorationGroup(Op::OpDecorationGroup, c0);
        break;
      }
      case Op::OpGroupDecorate:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<Multiple<IdRef>>();
        if (!operands.finishOperands(on_error, "OpGroupDecorate"))
          return false;
        load_context.onGroupDecorate(Op::OpGroupDecorate, c0, c1);
        break;
      }
      case Op::OpGroupMemberDecorate:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<Multiple<PairIdRefLiteralInteger>>();
        if (!operands.finishOperands(on_error, "OpGroupMemberDecorate"))
          return false;
        load_context.onGroupMemberDecorate(Op::OpGroupMemberDecorate, c0, c1);
        break;
      }
      case Op::OpVectorExtractDynamic:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpVectorExtractDynamic"))
          return false;
        load_context.onVectorExtractDynamic(Op::OpVectorExtractDynamic, c1, c0, c2, c3);
        break;
      }
      case Op::OpVectorInsertDynamic:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpVectorInsertDynamic"))
          return false;
        load_context.onVectorInsertDynamic(Op::OpVectorInsertDynamic, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpVectorShuffle:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<Multiple<LiteralInteger>>();
        if (!operands.finishOperands(on_error, "OpVectorShuffle"))
          return false;
        load_context.onVectorShuffle(Op::OpVectorShuffle, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpCompositeConstruct:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<Multiple<IdRef>>();
        if (!operands.finishOperands(on_error, "OpCompositeConstruct"))
          return false;
        load_context.onCompositeConstruct(Op::OpCompositeConstruct, c1, c0, c2);
        break;
      }
      case Op::OpCompositeExtract:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<Multiple<LiteralInteger>>();
        if (!operands.finishOperands(on_error, "OpCompositeExtract"))
          return false;
        load_context.onCompositeExtract(Op::OpCompositeExtract, c1, c0, c2, c3);
        break;
      }
      case Op::OpCompositeInsert:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<Multiple<LiteralInteger>>();
        if (!operands.finishOperands(on_error, "OpCompositeInsert"))
          return false;
        load_context.onCompositeInsert(Op::OpCompositeInsert, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpCopyObject:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpCopyObject"))
          return false;
        load_context.onCopyObject(Op::OpCopyObject, c1, c0, c2);
        break;
      }
      case Op::OpTranspose:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpTranspose"))
          return false;
        load_context.onTranspose(Op::OpTranspose, c1, c0, c2);
        break;
      }
      case Op::OpSampledImage:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSampledImage"))
          return false;
        load_context.onSampledImage(Op::OpSampledImage, c1, c0, c2, c3);
        break;
      }
      case Op::OpImageSampleImplicitLod:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<Optional<ImageOperandsMask>>();
        if (!operands.finishOperands(on_error, "OpImageSampleImplicitLod"))
          return false;
        load_context.onImageSampleImplicitLod(Op::OpImageSampleImplicitLod, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpImageSampleExplicitLod:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<ImageOperandsMask>();
        if (!operands.finishOperands(on_error, "OpImageSampleExplicitLod"))
          return false;
        load_context.onImageSampleExplicitLod(Op::OpImageSampleExplicitLod, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpImageSampleDrefImplicitLod:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<ImageOperandsMask>>();
        if (!operands.finishOperands(on_error, "OpImageSampleDrefImplicitLod"))
          return false;
        load_context.onImageSampleDrefImplicitLod(Op::OpImageSampleDrefImplicitLod, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpImageSampleDrefExplicitLod:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<ImageOperandsMask>();
        if (!operands.finishOperands(on_error, "OpImageSampleDrefExplicitLod"))
          return false;
        load_context.onImageSampleDrefExplicitLod(Op::OpImageSampleDrefExplicitLod, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpImageSampleProjImplicitLod:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<Optional<ImageOperandsMask>>();
        if (!operands.finishOperands(on_error, "OpImageSampleProjImplicitLod"))
          return false;
        load_context.onImageSampleProjImplicitLod(Op::OpImageSampleProjImplicitLod, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpImageSampleProjExplicitLod:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<ImageOperandsMask>();
        if (!operands.finishOperands(on_error, "OpImageSampleProjExplicitLod"))
          return false;
        load_context.onImageSampleProjExplicitLod(Op::OpImageSampleProjExplicitLod, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpImageSampleProjDrefImplicitLod:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<ImageOperandsMask>>();
        if (!operands.finishOperands(on_error, "OpImageSampleProjDrefImplicitLod"))
          return false;
        load_context.onImageSampleProjDrefImplicitLod(Op::OpImageSampleProjDrefImplicitLod, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpImageSampleProjDrefExplicitLod:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<ImageOperandsMask>();
        if (!operands.finishOperands(on_error, "OpImageSampleProjDrefExplicitLod"))
          return false;
        load_context.onImageSampleProjDrefExplicitLod(Op::OpImageSampleProjDrefExplicitLod, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpImageFetch:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<Optional<ImageOperandsMask>>();
        if (!operands.finishOperands(on_error, "OpImageFetch"))
          return false;
        load_context.onImageFetch(Op::OpImageFetch, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpImageGather:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<ImageOperandsMask>>();
        if (!operands.finishOperands(on_error, "OpImageGather"))
          return false;
        load_context.onImageGather(Op::OpImageGather, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpImageDrefGather:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<ImageOperandsMask>>();
        if (!operands.finishOperands(on_error, "OpImageDrefGather"))
          return false;
        load_context.onImageDrefGather(Op::OpImageDrefGather, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpImageRead:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<Optional<ImageOperandsMask>>();
        if (!operands.finishOperands(on_error, "OpImageRead"))
          return false;
        load_context.onImageRead(Op::OpImageRead, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpImageWrite:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<Optional<ImageOperandsMask>>();
        if (!operands.finishOperands(on_error, "OpImageWrite"))
          return false;
        load_context.onImageWrite(Op::OpImageWrite, c0, c1, c2, c3);
        break;
      }
      case Op::OpImage:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpImage"))
          return false;
        load_context.onImage(Op::OpImage, c1, c0, c2);
        break;
      }
      case Op::OpImageQueryFormat:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpImageQueryFormat"))
          return false;
        load_context.onImageQueryFormat(Op::OpImageQueryFormat, c1, c0, c2);
        break;
      }
      case Op::OpImageQueryOrder:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpImageQueryOrder"))
          return false;
        load_context.onImageQueryOrder(Op::OpImageQueryOrder, c1, c0, c2);
        break;
      }
      case Op::OpImageQuerySizeLod:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpImageQuerySizeLod"))
          return false;
        load_context.onImageQuerySizeLod(Op::OpImageQuerySizeLod, c1, c0, c2, c3);
        break;
      }
      case Op::OpImageQuerySize:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpImageQuerySize"))
          return false;
        load_context.onImageQuerySize(Op::OpImageQuerySize, c1, c0, c2);
        break;
      }
      case Op::OpImageQueryLod:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpImageQueryLod"))
          return false;
        load_context.onImageQueryLod(Op::OpImageQueryLod, c1, c0, c2, c3);
        break;
      }
      case Op::OpImageQueryLevels:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpImageQueryLevels"))
          return false;
        load_context.onImageQueryLevels(Op::OpImageQueryLevels, c1, c0, c2);
        break;
      }
      case Op::OpImageQuerySamples:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpImageQuerySamples"))
          return false;
        load_context.onImageQuerySamples(Op::OpImageQuerySamples, c1, c0, c2);
        break;
      }
      case Op::OpConvertFToU:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpConvertFToU"))
          return false;
        load_context.onConvertFToU(Op::OpConvertFToU, c1, c0, c2);
        break;
      }
      case Op::OpConvertFToS:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpConvertFToS"))
          return false;
        load_context.onConvertFToS(Op::OpConvertFToS, c1, c0, c2);
        break;
      }
      case Op::OpConvertSToF:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpConvertSToF"))
          return false;
        load_context.onConvertSToF(Op::OpConvertSToF, c1, c0, c2);
        break;
      }
      case Op::OpConvertUToF:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpConvertUToF"))
          return false;
        load_context.onConvertUToF(Op::OpConvertUToF, c1, c0, c2);
        break;
      }
      case Op::OpUConvert:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpUConvert"))
          return false;
        load_context.onUConvert(Op::OpUConvert, c1, c0, c2);
        break;
      }
      case Op::OpSConvert:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSConvert"))
          return false;
        load_context.onSConvert(Op::OpSConvert, c1, c0, c2);
        break;
      }
      case Op::OpFConvert:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFConvert"))
          return false;
        load_context.onFConvert(Op::OpFConvert, c1, c0, c2);
        break;
      }
      case Op::OpQuantizeToF16:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpQuantizeToF16"))
          return false;
        load_context.onQuantizeToF16(Op::OpQuantizeToF16, c1, c0, c2);
        break;
      }
      case Op::OpConvertPtrToU:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpConvertPtrToU"))
          return false;
        load_context.onConvertPtrToU(Op::OpConvertPtrToU, c1, c0, c2);
        break;
      }
      case Op::OpSatConvertSToU:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSatConvertSToU"))
          return false;
        load_context.onSatConvertSToU(Op::OpSatConvertSToU, c1, c0, c2);
        break;
      }
      case Op::OpSatConvertUToS:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSatConvertUToS"))
          return false;
        load_context.onSatConvertUToS(Op::OpSatConvertUToS, c1, c0, c2);
        break;
      }
      case Op::OpConvertUToPtr:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpConvertUToPtr"))
          return false;
        load_context.onConvertUToPtr(Op::OpConvertUToPtr, c1, c0, c2);
        break;
      }
      case Op::OpPtrCastToGeneric:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpPtrCastToGeneric"))
          return false;
        load_context.onPtrCastToGeneric(Op::OpPtrCastToGeneric, c1, c0, c2);
        break;
      }
      case Op::OpGenericCastToPtr:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGenericCastToPtr"))
          return false;
        load_context.onGenericCastToPtr(Op::OpGenericCastToPtr, c1, c0, c2);
        break;
      }
      case Op::OpGenericCastToPtrExplicit:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<StorageClass>();
        if (!operands.finishOperands(on_error, "OpGenericCastToPtrExplicit"))
          return false;
        load_context.onGenericCastToPtrExplicit(Op::OpGenericCastToPtrExplicit, c1, c0, c2, c3);
        break;
      }
      case Op::OpBitcast:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpBitcast"))
          return false;
        load_context.onBitcast(Op::OpBitcast, c1, c0, c2);
        break;
      }
      case Op::OpSNegate:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSNegate"))
          return false;
        load_context.onSNegate(Op::OpSNegate, c1, c0, c2);
        break;
      }
      case Op::OpFNegate:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFNegate"))
          return false;
        load_context.onFNegate(Op::OpFNegate, c1, c0, c2);
        break;
      }
      case Op::OpIAdd:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpIAdd"))
          return false;
        load_context.onIAdd(Op::OpIAdd, c1, c0, c2, c3);
        break;
      }
      case Op::OpFAdd:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFAdd"))
          return false;
        load_context.onFAdd(Op::OpFAdd, c1, c0, c2, c3);
        break;
      }
      case Op::OpISub:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpISub"))
          return false;
        load_context.onISub(Op::OpISub, c1, c0, c2, c3);
        break;
      }
      case Op::OpFSub:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFSub"))
          return false;
        load_context.onFSub(Op::OpFSub, c1, c0, c2, c3);
        break;
      }
      case Op::OpIMul:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpIMul"))
          return false;
        load_context.onIMul(Op::OpIMul, c1, c0, c2, c3);
        break;
      }
      case Op::OpFMul:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFMul"))
          return false;
        load_context.onFMul(Op::OpFMul, c1, c0, c2, c3);
        break;
      }
      case Op::OpUDiv:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpUDiv"))
          return false;
        load_context.onUDiv(Op::OpUDiv, c1, c0, c2, c3);
        break;
      }
      case Op::OpSDiv:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSDiv"))
          return false;
        load_context.onSDiv(Op::OpSDiv, c1, c0, c2, c3);
        break;
      }
      case Op::OpFDiv:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFDiv"))
          return false;
        load_context.onFDiv(Op::OpFDiv, c1, c0, c2, c3);
        break;
      }
      case Op::OpUMod:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpUMod"))
          return false;
        load_context.onUMod(Op::OpUMod, c1, c0, c2, c3);
        break;
      }
      case Op::OpSRem:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSRem"))
          return false;
        load_context.onSRem(Op::OpSRem, c1, c0, c2, c3);
        break;
      }
      case Op::OpSMod:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSMod"))
          return false;
        load_context.onSMod(Op::OpSMod, c1, c0, c2, c3);
        break;
      }
      case Op::OpFRem:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFRem"))
          return false;
        load_context.onFRem(Op::OpFRem, c1, c0, c2, c3);
        break;
      }
      case Op::OpFMod:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFMod"))
          return false;
        load_context.onFMod(Op::OpFMod, c1, c0, c2, c3);
        break;
      }
      case Op::OpVectorTimesScalar:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpVectorTimesScalar"))
          return false;
        load_context.onVectorTimesScalar(Op::OpVectorTimesScalar, c1, c0, c2, c3);
        break;
      }
      case Op::OpMatrixTimesScalar:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpMatrixTimesScalar"))
          return false;
        load_context.onMatrixTimesScalar(Op::OpMatrixTimesScalar, c1, c0, c2, c3);
        break;
      }
      case Op::OpVectorTimesMatrix:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpVectorTimesMatrix"))
          return false;
        load_context.onVectorTimesMatrix(Op::OpVectorTimesMatrix, c1, c0, c2, c3);
        break;
      }
      case Op::OpMatrixTimesVector:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpMatrixTimesVector"))
          return false;
        load_context.onMatrixTimesVector(Op::OpMatrixTimesVector, c1, c0, c2, c3);
        break;
      }
      case Op::OpMatrixTimesMatrix:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpMatrixTimesMatrix"))
          return false;
        load_context.onMatrixTimesMatrix(Op::OpMatrixTimesMatrix, c1, c0, c2, c3);
        break;
      }
      case Op::OpOuterProduct:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpOuterProduct"))
          return false;
        load_context.onOuterProduct(Op::OpOuterProduct, c1, c0, c2, c3);
        break;
      }
      case Op::OpDot:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpDot"))
          return false;
        load_context.onDot(Op::OpDot, c1, c0, c2, c3);
        break;
      }
      case Op::OpIAddCarry:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpIAddCarry"))
          return false;
        load_context.onIAddCarry(Op::OpIAddCarry, c1, c0, c2, c3);
        break;
      }
      case Op::OpISubBorrow:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpISubBorrow"))
          return false;
        load_context.onISubBorrow(Op::OpISubBorrow, c1, c0, c2, c3);
        break;
      }
      case Op::OpUMulExtended:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpUMulExtended"))
          return false;
        load_context.onUMulExtended(Op::OpUMulExtended, c1, c0, c2, c3);
        break;
      }
      case Op::OpSMulExtended:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSMulExtended"))
          return false;
        load_context.onSMulExtended(Op::OpSMulExtended, c1, c0, c2, c3);
        break;
      }
      case Op::OpAny:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpAny"))
          return false;
        load_context.onAny(Op::OpAny, c1, c0, c2);
        break;
      }
      case Op::OpAll:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpAll"))
          return false;
        load_context.onAll(Op::OpAll, c1, c0, c2);
        break;
      }
      case Op::OpIsNan:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpIsNan"))
          return false;
        load_context.onIsNan(Op::OpIsNan, c1, c0, c2);
        break;
      }
      case Op::OpIsInf:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpIsInf"))
          return false;
        load_context.onIsInf(Op::OpIsInf, c1, c0, c2);
        break;
      }
      case Op::OpIsFinite:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpIsFinite"))
          return false;
        load_context.onIsFinite(Op::OpIsFinite, c1, c0, c2);
        break;
      }
      case Op::OpIsNormal:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpIsNormal"))
          return false;
        load_context.onIsNormal(Op::OpIsNormal, c1, c0, c2);
        break;
      }
      case Op::OpSignBitSet:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSignBitSet"))
          return false;
        load_context.onSignBitSet(Op::OpSignBitSet, c1, c0, c2);
        break;
      }
      case Op::OpLessOrGreater:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpLessOrGreater"))
          return false;
        load_context.onLessOrGreater(Op::OpLessOrGreater, c1, c0, c2, c3);
        break;
      }
      case Op::OpOrdered:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpOrdered"))
          return false;
        load_context.onOrdered(Op::OpOrdered, c1, c0, c2, c3);
        break;
      }
      case Op::OpUnordered:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpUnordered"))
          return false;
        load_context.onUnordered(Op::OpUnordered, c1, c0, c2, c3);
        break;
      }
      case Op::OpLogicalEqual:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpLogicalEqual"))
          return false;
        load_context.onLogicalEqual(Op::OpLogicalEqual, c1, c0, c2, c3);
        break;
      }
      case Op::OpLogicalNotEqual:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpLogicalNotEqual"))
          return false;
        load_context.onLogicalNotEqual(Op::OpLogicalNotEqual, c1, c0, c2, c3);
        break;
      }
      case Op::OpLogicalOr:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpLogicalOr"))
          return false;
        load_context.onLogicalOr(Op::OpLogicalOr, c1, c0, c2, c3);
        break;
      }
      case Op::OpLogicalAnd:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpLogicalAnd"))
          return false;
        load_context.onLogicalAnd(Op::OpLogicalAnd, c1, c0, c2, c3);
        break;
      }
      case Op::OpLogicalNot:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpLogicalNot"))
          return false;
        load_context.onLogicalNot(Op::OpLogicalNot, c1, c0, c2);
        break;
      }
      case Op::OpSelect:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSelect"))
          return false;
        load_context.onSelect(Op::OpSelect, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpIEqual:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpIEqual"))
          return false;
        load_context.onIEqual(Op::OpIEqual, c1, c0, c2, c3);
        break;
      }
      case Op::OpINotEqual:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpINotEqual"))
          return false;
        load_context.onINotEqual(Op::OpINotEqual, c1, c0, c2, c3);
        break;
      }
      case Op::OpUGreaterThan:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpUGreaterThan"))
          return false;
        load_context.onUGreaterThan(Op::OpUGreaterThan, c1, c0, c2, c3);
        break;
      }
      case Op::OpSGreaterThan:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSGreaterThan"))
          return false;
        load_context.onSGreaterThan(Op::OpSGreaterThan, c1, c0, c2, c3);
        break;
      }
      case Op::OpUGreaterThanEqual:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpUGreaterThanEqual"))
          return false;
        load_context.onUGreaterThanEqual(Op::OpUGreaterThanEqual, c1, c0, c2, c3);
        break;
      }
      case Op::OpSGreaterThanEqual:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSGreaterThanEqual"))
          return false;
        load_context.onSGreaterThanEqual(Op::OpSGreaterThanEqual, c1, c0, c2, c3);
        break;
      }
      case Op::OpULessThan:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpULessThan"))
          return false;
        load_context.onULessThan(Op::OpULessThan, c1, c0, c2, c3);
        break;
      }
      case Op::OpSLessThan:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSLessThan"))
          return false;
        load_context.onSLessThan(Op::OpSLessThan, c1, c0, c2, c3);
        break;
      }
      case Op::OpULessThanEqual:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpULessThanEqual"))
          return false;
        load_context.onULessThanEqual(Op::OpULessThanEqual, c1, c0, c2, c3);
        break;
      }
      case Op::OpSLessThanEqual:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSLessThanEqual"))
          return false;
        load_context.onSLessThanEqual(Op::OpSLessThanEqual, c1, c0, c2, c3);
        break;
      }
      case Op::OpFOrdEqual:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFOrdEqual"))
          return false;
        load_context.onFOrdEqual(Op::OpFOrdEqual, c1, c0, c2, c3);
        break;
      }
      case Op::OpFUnordEqual:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFUnordEqual"))
          return false;
        load_context.onFUnordEqual(Op::OpFUnordEqual, c1, c0, c2, c3);
        break;
      }
      case Op::OpFOrdNotEqual:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFOrdNotEqual"))
          return false;
        load_context.onFOrdNotEqual(Op::OpFOrdNotEqual, c1, c0, c2, c3);
        break;
      }
      case Op::OpFUnordNotEqual:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFUnordNotEqual"))
          return false;
        load_context.onFUnordNotEqual(Op::OpFUnordNotEqual, c1, c0, c2, c3);
        break;
      }
      case Op::OpFOrdLessThan:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFOrdLessThan"))
          return false;
        load_context.onFOrdLessThan(Op::OpFOrdLessThan, c1, c0, c2, c3);
        break;
      }
      case Op::OpFUnordLessThan:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFUnordLessThan"))
          return false;
        load_context.onFUnordLessThan(Op::OpFUnordLessThan, c1, c0, c2, c3);
        break;
      }
      case Op::OpFOrdGreaterThan:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFOrdGreaterThan"))
          return false;
        load_context.onFOrdGreaterThan(Op::OpFOrdGreaterThan, c1, c0, c2, c3);
        break;
      }
      case Op::OpFUnordGreaterThan:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFUnordGreaterThan"))
          return false;
        load_context.onFUnordGreaterThan(Op::OpFUnordGreaterThan, c1, c0, c2, c3);
        break;
      }
      case Op::OpFOrdLessThanEqual:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFOrdLessThanEqual"))
          return false;
        load_context.onFOrdLessThanEqual(Op::OpFOrdLessThanEqual, c1, c0, c2, c3);
        break;
      }
      case Op::OpFUnordLessThanEqual:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFUnordLessThanEqual"))
          return false;
        load_context.onFUnordLessThanEqual(Op::OpFUnordLessThanEqual, c1, c0, c2, c3);
        break;
      }
      case Op::OpFOrdGreaterThanEqual:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFOrdGreaterThanEqual"))
          return false;
        load_context.onFOrdGreaterThanEqual(Op::OpFOrdGreaterThanEqual, c1, c0, c2, c3);
        break;
      }
      case Op::OpFUnordGreaterThanEqual:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFUnordGreaterThanEqual"))
          return false;
        load_context.onFUnordGreaterThanEqual(Op::OpFUnordGreaterThanEqual, c1, c0, c2, c3);
        break;
      }
      case Op::OpShiftRightLogical:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpShiftRightLogical"))
          return false;
        load_context.onShiftRightLogical(Op::OpShiftRightLogical, c1, c0, c2, c3);
        break;
      }
      case Op::OpShiftRightArithmetic:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpShiftRightArithmetic"))
          return false;
        load_context.onShiftRightArithmetic(Op::OpShiftRightArithmetic, c1, c0, c2, c3);
        break;
      }
      case Op::OpShiftLeftLogical:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpShiftLeftLogical"))
          return false;
        load_context.onShiftLeftLogical(Op::OpShiftLeftLogical, c1, c0, c2, c3);
        break;
      }
      case Op::OpBitwiseOr:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpBitwiseOr"))
          return false;
        load_context.onBitwiseOr(Op::OpBitwiseOr, c1, c0, c2, c3);
        break;
      }
      case Op::OpBitwiseXor:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpBitwiseXor"))
          return false;
        load_context.onBitwiseXor(Op::OpBitwiseXor, c1, c0, c2, c3);
        break;
      }
      case Op::OpBitwiseAnd:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpBitwiseAnd"))
          return false;
        load_context.onBitwiseAnd(Op::OpBitwiseAnd, c1, c0, c2, c3);
        break;
      }
      case Op::OpNot:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpNot"))
          return false;
        load_context.onNot(Op::OpNot, c1, c0, c2);
        break;
      }
      case Op::OpBitFieldInsert:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpBitFieldInsert"))
          return false;
        load_context.onBitFieldInsert(Op::OpBitFieldInsert, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpBitFieldSExtract:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpBitFieldSExtract"))
          return false;
        load_context.onBitFieldSExtract(Op::OpBitFieldSExtract, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpBitFieldUExtract:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpBitFieldUExtract"))
          return false;
        load_context.onBitFieldUExtract(Op::OpBitFieldUExtract, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpBitReverse:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpBitReverse"))
          return false;
        load_context.onBitReverse(Op::OpBitReverse, c1, c0, c2);
        break;
      }
      case Op::OpBitCount:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpBitCount"))
          return false;
        load_context.onBitCount(Op::OpBitCount, c1, c0, c2);
        break;
      }
      case Op::OpDPdx:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpDPdx"))
          return false;
        load_context.onDPdx(Op::OpDPdx, c1, c0, c2);
        break;
      }
      case Op::OpDPdy:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpDPdy"))
          return false;
        load_context.onDPdy(Op::OpDPdy, c1, c0, c2);
        break;
      }
      case Op::OpFwidth:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFwidth"))
          return false;
        load_context.onFwidth(Op::OpFwidth, c1, c0, c2);
        break;
      }
      case Op::OpDPdxFine:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpDPdxFine"))
          return false;
        load_context.onDPdxFine(Op::OpDPdxFine, c1, c0, c2);
        break;
      }
      case Op::OpDPdyFine:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpDPdyFine"))
          return false;
        load_context.onDPdyFine(Op::OpDPdyFine, c1, c0, c2);
        break;
      }
      case Op::OpFwidthFine:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFwidthFine"))
          return false;
        load_context.onFwidthFine(Op::OpFwidthFine, c1, c0, c2);
        break;
      }
      case Op::OpDPdxCoarse:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpDPdxCoarse"))
          return false;
        load_context.onDPdxCoarse(Op::OpDPdxCoarse, c1, c0, c2);
        break;
      }
      case Op::OpDPdyCoarse:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpDPdyCoarse"))
          return false;
        load_context.onDPdyCoarse(Op::OpDPdyCoarse, c1, c0, c2);
        break;
      }
      case Op::OpFwidthCoarse:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFwidthCoarse"))
          return false;
        load_context.onFwidthCoarse(Op::OpFwidthCoarse, c1, c0, c2);
        break;
      }
      case Op::OpEmitVertex:
      {
        if (!operands.finishOperands(on_error, "OpEmitVertex"))
          return false;
        load_context.onEmitVertex(Op::OpEmitVertex);
        break;
      }
      case Op::OpEndPrimitive:
      {
        if (!operands.finishOperands(on_error, "OpEndPrimitive"))
          return false;
        load_context.onEndPrimitive(Op::OpEndPrimitive);
        break;
      }
      case Op::OpEmitStreamVertex:
      {
        auto c0 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpEmitStreamVertex"))
          return false;
        load_context.onEmitStreamVertex(Op::OpEmitStreamVertex, c0);
        break;
      }
      case Op::OpEndStreamPrimitive:
      {
        auto c0 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpEndStreamPrimitive"))
          return false;
        load_context.onEndStreamPrimitive(Op::OpEndStreamPrimitive, c0);
        break;
      }
      case Op::OpControlBarrier:
      {
        auto c0 = operands.read<IdScope>();
        auto c1 = operands.read<IdScope>();
        auto c2 = operands.read<IdMemorySemantics>();
        if (!operands.finishOperands(on_error, "OpControlBarrier"))
          return false;
        load_context.onControlBarrier(Op::OpControlBarrier, c0, c1, c2);
        break;
      }
      case Op::OpMemoryBarrier:
      {
        auto c0 = operands.read<IdScope>();
        auto c1 = operands.read<IdMemorySemantics>();
        if (!operands.finishOperands(on_error, "OpMemoryBarrier"))
          return false;
        load_context.onMemoryBarrier(Op::OpMemoryBarrier, c0, c1);
        break;
      }
      case Op::OpAtomicLoad:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdScope>();
        auto c4 = operands.read<IdMemorySemantics>();
        if (!operands.finishOperands(on_error, "OpAtomicLoad"))
          return false;
        load_context.onAtomicLoad(Op::OpAtomicLoad, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpAtomicStore:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdScope>();
        auto c2 = operands.read<IdMemorySemantics>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpAtomicStore"))
          return false;
        load_context.onAtomicStore(Op::OpAtomicStore, c0, c1, c2, c3);
        break;
      }
      case Op::OpAtomicExchange:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdScope>();
        auto c4 = operands.read<IdMemorySemantics>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpAtomicExchange"))
          return false;
        load_context.onAtomicExchange(Op::OpAtomicExchange, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpAtomicCompareExchange:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdScope>();
        auto c4 = operands.read<IdMemorySemantics>();
        auto c5 = operands.read<IdMemorySemantics>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpAtomicCompareExchange"))
          return false;
        load_context.onAtomicCompareExchange(Op::OpAtomicCompareExchange, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpAtomicCompareExchangeWeak:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdScope>();
        auto c4 = operands.read<IdMemorySemantics>();
        auto c5 = operands.read<IdMemorySemantics>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpAtomicCompareExchangeWeak"))
          return false;
        load_context.onAtomicCompareExchangeWeak(Op::OpAtomicCompareExchangeWeak, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpAtomicIIncrement:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdScope>();
        auto c4 = operands.read<IdMemorySemantics>();
        if (!operands.finishOperands(on_error, "OpAtomicIIncrement"))
          return false;
        load_context.onAtomicIIncrement(Op::OpAtomicIIncrement, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpAtomicIDecrement:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdScope>();
        auto c4 = operands.read<IdMemorySemantics>();
        if (!operands.finishOperands(on_error, "OpAtomicIDecrement"))
          return false;
        load_context.onAtomicIDecrement(Op::OpAtomicIDecrement, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpAtomicIAdd:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdScope>();
        auto c4 = operands.read<IdMemorySemantics>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpAtomicIAdd"))
          return false;
        load_context.onAtomicIAdd(Op::OpAtomicIAdd, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpAtomicISub:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdScope>();
        auto c4 = operands.read<IdMemorySemantics>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpAtomicISub"))
          return false;
        load_context.onAtomicISub(Op::OpAtomicISub, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpAtomicSMin:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdScope>();
        auto c4 = operands.read<IdMemorySemantics>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpAtomicSMin"))
          return false;
        load_context.onAtomicSMin(Op::OpAtomicSMin, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpAtomicUMin:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdScope>();
        auto c4 = operands.read<IdMemorySemantics>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpAtomicUMin"))
          return false;
        load_context.onAtomicUMin(Op::OpAtomicUMin, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpAtomicSMax:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdScope>();
        auto c4 = operands.read<IdMemorySemantics>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpAtomicSMax"))
          return false;
        load_context.onAtomicSMax(Op::OpAtomicSMax, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpAtomicUMax:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdScope>();
        auto c4 = operands.read<IdMemorySemantics>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpAtomicUMax"))
          return false;
        load_context.onAtomicUMax(Op::OpAtomicUMax, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpAtomicAnd:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdScope>();
        auto c4 = operands.read<IdMemorySemantics>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpAtomicAnd"))
          return false;
        load_context.onAtomicAnd(Op::OpAtomicAnd, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpAtomicOr:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdScope>();
        auto c4 = operands.read<IdMemorySemantics>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpAtomicOr"))
          return false;
        load_context.onAtomicOr(Op::OpAtomicOr, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpAtomicXor:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdScope>();
        auto c4 = operands.read<IdMemorySemantics>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpAtomicXor"))
          return false;
        load_context.onAtomicXor(Op::OpAtomicXor, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpPhi:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<Multiple<PairIdRefIdRef>>();
        if (!operands.finishOperands(on_error, "OpPhi"))
          return false;
        load_context.onPhi(Op::OpPhi, c1, c0, c2);
        break;
      }
      case Op::OpLoopMerge:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<LoopControlMask>();
        if (!operands.finishOperands(on_error, "OpLoopMerge"))
          return false;
        load_context.onLoopMerge(Op::OpLoopMerge, c0, c1, c2);
        break;
      }
      case Op::OpSelectionMerge:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<SelectionControlMask>();
        if (!operands.finishOperands(on_error, "OpSelectionMerge"))
          return false;
        load_context.onSelectionMerge(Op::OpSelectionMerge, c0, c1);
        break;
      }
      case Op::OpLabel:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpLabel"))
          return false;
        load_context.onLabel(Op::OpLabel, c0);
        break;
      }
      case Op::OpBranch:
      {
        auto c0 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpBranch"))
          return false;
        load_context.onBranch(Op::OpBranch, c0);
        break;
      }
      case Op::OpBranchConditional:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<Multiple<LiteralInteger>>();
        if (!operands.finishOperands(on_error, "OpBranchConditional"))
          return false;
        load_context.onBranchConditional(Op::OpBranchConditional, c0, c1, c2, c3);
        break;
      }
      case Op::OpSwitch:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<Multiple<PairLiteralIntegerIdRef>>();
        if (!operands.finishOperands(on_error, "OpSwitch"))
          return false;
        load_context.onSwitch(Op::OpSwitch, c0, c1, c2);
        break;
      }
      case Op::OpKill:
      {
        if (!operands.finishOperands(on_error, "OpKill"))
          return false;
        load_context.onKill(Op::OpKill);
        break;
      }
      case Op::OpReturn:
      {
        if (!operands.finishOperands(on_error, "OpReturn"))
          return false;
        load_context.onReturn(Op::OpReturn);
        break;
      }
      case Op::OpReturnValue:
      {
        auto c0 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpReturnValue"))
          return false;
        load_context.onReturnValue(Op::OpReturnValue, c0);
        break;
      }
      case Op::OpUnreachable:
      {
        if (!operands.finishOperands(on_error, "OpUnreachable"))
          return false;
        load_context.onUnreachable(Op::OpUnreachable);
        break;
      }
      case Op::OpLifetimeStart:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpLifetimeStart"))
          return false;
        load_context.onLifetimeStart(Op::OpLifetimeStart, c0, c1);
        break;
      }
      case Op::OpLifetimeStop:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpLifetimeStop"))
          return false;
        load_context.onLifetimeStop(Op::OpLifetimeStop, c0, c1);
        break;
      }
      case Op::OpGroupAsyncCopy:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupAsyncCopy"))
          return false;
        load_context.onGroupAsyncCopy(Op::OpGroupAsyncCopy, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpGroupWaitEvents:
      {
        auto c0 = operands.read<IdScope>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupWaitEvents"))
          return false;
        load_context.onGroupWaitEvents(Op::OpGroupWaitEvents, c0, c1, c2);
        break;
      }
      case Op::OpGroupAll:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupAll"))
          return false;
        load_context.onGroupAll(Op::OpGroupAll, c1, c0, c2, c3);
        break;
      }
      case Op::OpGroupAny:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupAny"))
          return false;
        load_context.onGroupAny(Op::OpGroupAny, c1, c0, c2, c3);
        break;
      }
      case Op::OpGroupBroadcast:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupBroadcast"))
          return false;
        load_context.onGroupBroadcast(Op::OpGroupBroadcast, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupIAdd:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupIAdd"))
          return false;
        load_context.onGroupIAdd(Op::OpGroupIAdd, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupFAdd:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupFAdd"))
          return false;
        load_context.onGroupFAdd(Op::OpGroupFAdd, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupFMin:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupFMin"))
          return false;
        load_context.onGroupFMin(Op::OpGroupFMin, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupUMin:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupUMin"))
          return false;
        load_context.onGroupUMin(Op::OpGroupUMin, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupSMin:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupSMin"))
          return false;
        load_context.onGroupSMin(Op::OpGroupSMin, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupFMax:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupFMax"))
          return false;
        load_context.onGroupFMax(Op::OpGroupFMax, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupUMax:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupUMax"))
          return false;
        load_context.onGroupUMax(Op::OpGroupUMax, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupSMax:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupSMax"))
          return false;
        load_context.onGroupSMax(Op::OpGroupSMax, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpReadPipe:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpReadPipe"))
          return false;
        load_context.onReadPipe(Op::OpReadPipe, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpWritePipe:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpWritePipe"))
          return false;
        load_context.onWritePipe(Op::OpWritePipe, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpReservedReadPipe:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpReservedReadPipe"))
          return false;
        load_context.onReservedReadPipe(Op::OpReservedReadPipe, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpReservedWritePipe:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpReservedWritePipe"))
          return false;
        load_context.onReservedWritePipe(Op::OpReservedWritePipe, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpReserveReadPipePackets:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpReserveReadPipePackets"))
          return false;
        load_context.onReserveReadPipePackets(Op::OpReserveReadPipePackets, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpReserveWritePipePackets:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpReserveWritePipePackets"))
          return false;
        load_context.onReserveWritePipePackets(Op::OpReserveWritePipePackets, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpCommitReadPipe:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpCommitReadPipe"))
          return false;
        load_context.onCommitReadPipe(Op::OpCommitReadPipe, c0, c1, c2, c3);
        break;
      }
      case Op::OpCommitWritePipe:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpCommitWritePipe"))
          return false;
        load_context.onCommitWritePipe(Op::OpCommitWritePipe, c0, c1, c2, c3);
        break;
      }
      case Op::OpIsValidReserveId:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpIsValidReserveId"))
          return false;
        load_context.onIsValidReserveId(Op::OpIsValidReserveId, c1, c0, c2);
        break;
      }
      case Op::OpGetNumPipePackets:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGetNumPipePackets"))
          return false;
        load_context.onGetNumPipePackets(Op::OpGetNumPipePackets, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGetMaxPipePackets:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGetMaxPipePackets"))
          return false;
        load_context.onGetMaxPipePackets(Op::OpGetMaxPipePackets, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupReserveReadPipePackets:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupReserveReadPipePackets"))
          return false;
        load_context.onGroupReserveReadPipePackets(Op::OpGroupReserveReadPipePackets, c1, c0, c2, c3, c4, c5, c6);
        break;
      }
      case Op::OpGroupReserveWritePipePackets:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupReserveWritePipePackets"))
          return false;
        load_context.onGroupReserveWritePipePackets(Op::OpGroupReserveWritePipePackets, c1, c0, c2, c3, c4, c5, c6);
        break;
      }
      case Op::OpGroupCommitReadPipe:
      {
        auto c0 = operands.read<IdScope>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupCommitReadPipe"))
          return false;
        load_context.onGroupCommitReadPipe(Op::OpGroupCommitReadPipe, c0, c1, c2, c3, c4);
        break;
      }
      case Op::OpGroupCommitWritePipe:
      {
        auto c0 = operands.read<IdScope>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupCommitWritePipe"))
          return false;
        load_context.onGroupCommitWritePipe(Op::OpGroupCommitWritePipe, c0, c1, c2, c3, c4);
        break;
      }
      case Op::OpEnqueueMarker:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpEnqueueMarker"))
          return false;
        load_context.onEnqueueMarker(Op::OpEnqueueMarker, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpEnqueueKernel:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        auto c8 = operands.read<IdRef>();
        auto c9 = operands.read<IdRef>();
        auto c10 = operands.read<IdRef>();
        auto c11 = operands.read<IdRef>();
        auto c12 = operands.read<Multiple<IdRef>>();
        if (!operands.finishOperands(on_error, "OpEnqueueKernel"))
          return false;
        load_context.onEnqueueKernel(Op::OpEnqueueKernel, c1, c0, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12);
        break;
      }
      case Op::OpGetKernelNDrangeSubGroupCount:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGetKernelNDrangeSubGroupCount"))
          return false;
        load_context.onGetKernelNDrangeSubGroupCount(Op::OpGetKernelNDrangeSubGroupCount, c1, c0, c2, c3, c4, c5, c6);
        break;
      }
      case Op::OpGetKernelNDrangeMaxSubGroupSize:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGetKernelNDrangeMaxSubGroupSize"))
          return false;
        load_context.onGetKernelNDrangeMaxSubGroupSize(Op::OpGetKernelNDrangeMaxSubGroupSize, c1, c0, c2, c3, c4, c5, c6);
        break;
      }
      case Op::OpGetKernelWorkGroupSize:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGetKernelWorkGroupSize"))
          return false;
        load_context.onGetKernelWorkGroupSize(Op::OpGetKernelWorkGroupSize, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpGetKernelPreferredWorkGroupSizeMultiple:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGetKernelPreferredWorkGroupSizeMultiple"))
          return false;
        load_context.onGetKernelPreferredWorkGroupSizeMultiple(Op::OpGetKernelPreferredWorkGroupSizeMultiple, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpRetainEvent:
      {
        auto c0 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRetainEvent"))
          return false;
        load_context.onRetainEvent(Op::OpRetainEvent, c0);
        break;
      }
      case Op::OpReleaseEvent:
      {
        auto c0 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpReleaseEvent"))
          return false;
        load_context.onReleaseEvent(Op::OpReleaseEvent, c0);
        break;
      }
      case Op::OpCreateUserEvent:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpCreateUserEvent"))
          return false;
        load_context.onCreateUserEvent(Op::OpCreateUserEvent, c1, c0);
        break;
      }
      case Op::OpIsValidEvent:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpIsValidEvent"))
          return false;
        load_context.onIsValidEvent(Op::OpIsValidEvent, c1, c0, c2);
        break;
      }
      case Op::OpSetUserEventStatus:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSetUserEventStatus"))
          return false;
        load_context.onSetUserEventStatus(Op::OpSetUserEventStatus, c0, c1);
        break;
      }
      case Op::OpCaptureEventProfilingInfo:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpCaptureEventProfilingInfo"))
          return false;
        load_context.onCaptureEventProfilingInfo(Op::OpCaptureEventProfilingInfo, c0, c1, c2);
        break;
      }
      case Op::OpGetDefaultQueue:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpGetDefaultQueue"))
          return false;
        load_context.onGetDefaultQueue(Op::OpGetDefaultQueue, c1, c0);
        break;
      }
      case Op::OpBuildNDRange:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpBuildNDRange"))
          return false;
        load_context.onBuildNDRange(Op::OpBuildNDRange, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpImageSparseSampleImplicitLod:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<Optional<ImageOperandsMask>>();
        if (!operands.finishOperands(on_error, "OpImageSparseSampleImplicitLod"))
          return false;
        load_context.onImageSparseSampleImplicitLod(Op::OpImageSparseSampleImplicitLod, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpImageSparseSampleExplicitLod:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<ImageOperandsMask>();
        if (!operands.finishOperands(on_error, "OpImageSparseSampleExplicitLod"))
          return false;
        load_context.onImageSparseSampleExplicitLod(Op::OpImageSparseSampleExplicitLod, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpImageSparseSampleDrefImplicitLod:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<ImageOperandsMask>>();
        if (!operands.finishOperands(on_error, "OpImageSparseSampleDrefImplicitLod"))
          return false;
        load_context.onImageSparseSampleDrefImplicitLod(Op::OpImageSparseSampleDrefImplicitLod, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpImageSparseSampleDrefExplicitLod:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<ImageOperandsMask>();
        if (!operands.finishOperands(on_error, "OpImageSparseSampleDrefExplicitLod"))
          return false;
        load_context.onImageSparseSampleDrefExplicitLod(Op::OpImageSparseSampleDrefExplicitLod, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpImageSparseSampleProjImplicitLod:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<Optional<ImageOperandsMask>>();
        if (!operands.finishOperands(on_error, "OpImageSparseSampleProjImplicitLod"))
          return false;
        load_context.onImageSparseSampleProjImplicitLod(Op::OpImageSparseSampleProjImplicitLod, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpImageSparseSampleProjExplicitLod:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<ImageOperandsMask>();
        if (!operands.finishOperands(on_error, "OpImageSparseSampleProjExplicitLod"))
          return false;
        load_context.onImageSparseSampleProjExplicitLod(Op::OpImageSparseSampleProjExplicitLod, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpImageSparseSampleProjDrefImplicitLod:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<ImageOperandsMask>>();
        if (!operands.finishOperands(on_error, "OpImageSparseSampleProjDrefImplicitLod"))
          return false;
        load_context.onImageSparseSampleProjDrefImplicitLod(Op::OpImageSparseSampleProjDrefImplicitLod, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpImageSparseSampleProjDrefExplicitLod:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<ImageOperandsMask>();
        if (!operands.finishOperands(on_error, "OpImageSparseSampleProjDrefExplicitLod"))
          return false;
        load_context.onImageSparseSampleProjDrefExplicitLod(Op::OpImageSparseSampleProjDrefExplicitLod, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpImageSparseFetch:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<Optional<ImageOperandsMask>>();
        if (!operands.finishOperands(on_error, "OpImageSparseFetch"))
          return false;
        load_context.onImageSparseFetch(Op::OpImageSparseFetch, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpImageSparseGather:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<ImageOperandsMask>>();
        if (!operands.finishOperands(on_error, "OpImageSparseGather"))
          return false;
        load_context.onImageSparseGather(Op::OpImageSparseGather, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpImageSparseDrefGather:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<ImageOperandsMask>>();
        if (!operands.finishOperands(on_error, "OpImageSparseDrefGather"))
          return false;
        load_context.onImageSparseDrefGather(Op::OpImageSparseDrefGather, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpImageSparseTexelsResident:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpImageSparseTexelsResident"))
          return false;
        load_context.onImageSparseTexelsResident(Op::OpImageSparseTexelsResident, c1, c0, c2);
        break;
      }
      case Op::OpNoLine:
      {
        if (!operands.finishOperands(on_error, "OpNoLine"))
          return false;
        load_context.onNoLine(Op::OpNoLine);
        break;
      }
      case Op::OpAtomicFlagTestAndSet:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdScope>();
        auto c4 = operands.read<IdMemorySemantics>();
        if (!operands.finishOperands(on_error, "OpAtomicFlagTestAndSet"))
          return false;
        load_context.onAtomicFlagTestAndSet(Op::OpAtomicFlagTestAndSet, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpAtomicFlagClear:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdScope>();
        auto c2 = operands.read<IdMemorySemantics>();
        if (!operands.finishOperands(on_error, "OpAtomicFlagClear"))
          return false;
        load_context.onAtomicFlagClear(Op::OpAtomicFlagClear, c0, c1, c2);
        break;
      }
      case Op::OpImageSparseRead:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<Optional<ImageOperandsMask>>();
        if (!operands.finishOperands(on_error, "OpImageSparseRead"))
          return false;
        load_context.onImageSparseRead(Op::OpImageSparseRead, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpSizeOf:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSizeOf"))
          return false;
        load_context.onSizeOf(Op::OpSizeOf, c1, c0, c2);
        break;
      }
      case Op::OpTypePipeStorage:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypePipeStorage"))
          return false;
        load_context.onTypePipeStorage(Op::OpTypePipeStorage, c0);
        break;
      }
      case Op::OpConstantPipeStorage:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<LiteralInteger>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpConstantPipeStorage"))
          return false;
        load_context.onConstantPipeStorage(Op::OpConstantPipeStorage, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpCreatePipeFromPipeStorage:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpCreatePipeFromPipeStorage"))
          return false;
        load_context.onCreatePipeFromPipeStorage(Op::OpCreatePipeFromPipeStorage, c1, c0, c2);
        break;
      }
      case Op::OpGetKernelLocalSizeForSubgroupCount:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGetKernelLocalSizeForSubgroupCount"))
          return false;
        load_context.onGetKernelLocalSizeForSubgroupCount(Op::OpGetKernelLocalSizeForSubgroupCount, c1, c0, c2, c3, c4, c5, c6);
        break;
      }
      case Op::OpGetKernelMaxNumSubgroups:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGetKernelMaxNumSubgroups"))
          return false;
        load_context.onGetKernelMaxNumSubgroups(Op::OpGetKernelMaxNumSubgroups, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpTypeNamedBarrier:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeNamedBarrier"))
          return false;
        load_context.onTypeNamedBarrier(Op::OpTypeNamedBarrier, c0);
        break;
      }
      case Op::OpNamedBarrierInitialize:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpNamedBarrierInitialize"))
          return false;
        load_context.onNamedBarrierInitialize(Op::OpNamedBarrierInitialize, c1, c0, c2);
        break;
      }
      case Op::OpMemoryNamedBarrier:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdScope>();
        auto c2 = operands.read<IdMemorySemantics>();
        if (!operands.finishOperands(on_error, "OpMemoryNamedBarrier"))
          return false;
        load_context.onMemoryNamedBarrier(Op::OpMemoryNamedBarrier, c0, c1, c2);
        break;
      }
      case Op::OpModuleProcessed:
      {
        auto c0 = operands.read<LiteralString>();
        if (!operands.finishOperands(on_error, "OpModuleProcessed"))
          return false;
        load_context.onModuleProcessed(Op::OpModuleProcessed, c0);
        break;
      }
      case Op::OpExecutionModeId:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<ExecutionMode>();
        if (!operands.finishOperands(on_error, "OpExecutionModeId"))
          return false;
        load_context.onExecutionModeId(Op::OpExecutionModeId, c0, c1);
        break;
      }
      case Op::OpDecorateId:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<Decoration>();
        if (!operands.finishOperands(on_error, "OpDecorateId"))
          return false;
        load_context.onDecorateId(Op::OpDecorateId, c0, c1);
        break;
      }
      case Op::OpGroupNonUniformElect:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformElect"))
          return false;
        load_context.onGroupNonUniformElect(Op::OpGroupNonUniformElect, c1, c0, c2);
        break;
      }
      case Op::OpGroupNonUniformAll:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformAll"))
          return false;
        load_context.onGroupNonUniformAll(Op::OpGroupNonUniformAll, c1, c0, c2, c3);
        break;
      }
      case Op::OpGroupNonUniformAny:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformAny"))
          return false;
        load_context.onGroupNonUniformAny(Op::OpGroupNonUniformAny, c1, c0, c2, c3);
        break;
      }
      case Op::OpGroupNonUniformAllEqual:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformAllEqual"))
          return false;
        load_context.onGroupNonUniformAllEqual(Op::OpGroupNonUniformAllEqual, c1, c0, c2, c3);
        break;
      }
      case Op::OpGroupNonUniformBroadcast:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformBroadcast"))
          return false;
        load_context.onGroupNonUniformBroadcast(Op::OpGroupNonUniformBroadcast, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupNonUniformBroadcastFirst:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformBroadcastFirst"))
          return false;
        load_context.onGroupNonUniformBroadcastFirst(Op::OpGroupNonUniformBroadcastFirst, c1, c0, c2, c3);
        break;
      }
      case Op::OpGroupNonUniformBallot:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformBallot"))
          return false;
        load_context.onGroupNonUniformBallot(Op::OpGroupNonUniformBallot, c1, c0, c2, c3);
        break;
      }
      case Op::OpGroupNonUniformInverseBallot:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformInverseBallot"))
          return false;
        load_context.onGroupNonUniformInverseBallot(Op::OpGroupNonUniformInverseBallot, c1, c0, c2, c3);
        break;
      }
      case Op::OpGroupNonUniformBallotBitExtract:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformBallotBitExtract"))
          return false;
        load_context.onGroupNonUniformBallotBitExtract(Op::OpGroupNonUniformBallotBitExtract, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupNonUniformBallotBitCount:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformBallotBitCount"))
          return false;
        load_context.onGroupNonUniformBallotBitCount(Op::OpGroupNonUniformBallotBitCount, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupNonUniformBallotFindLSB:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformBallotFindLSB"))
          return false;
        load_context.onGroupNonUniformBallotFindLSB(Op::OpGroupNonUniformBallotFindLSB, c1, c0, c2, c3);
        break;
      }
      case Op::OpGroupNonUniformBallotFindMSB:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformBallotFindMSB"))
          return false;
        load_context.onGroupNonUniformBallotFindMSB(Op::OpGroupNonUniformBallotFindMSB, c1, c0, c2, c3);
        break;
      }
      case Op::OpGroupNonUniformShuffle:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformShuffle"))
          return false;
        load_context.onGroupNonUniformShuffle(Op::OpGroupNonUniformShuffle, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupNonUniformShuffleXor:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformShuffleXor"))
          return false;
        load_context.onGroupNonUniformShuffleXor(Op::OpGroupNonUniformShuffleXor, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupNonUniformShuffleUp:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformShuffleUp"))
          return false;
        load_context.onGroupNonUniformShuffleUp(Op::OpGroupNonUniformShuffleUp, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupNonUniformShuffleDown:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformShuffleDown"))
          return false;
        load_context.onGroupNonUniformShuffleDown(Op::OpGroupNonUniformShuffleDown, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupNonUniformIAdd:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformIAdd"))
          return false;
        load_context.onGroupNonUniformIAdd(Op::OpGroupNonUniformIAdd, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpGroupNonUniformFAdd:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformFAdd"))
          return false;
        load_context.onGroupNonUniformFAdd(Op::OpGroupNonUniformFAdd, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpGroupNonUniformIMul:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformIMul"))
          return false;
        load_context.onGroupNonUniformIMul(Op::OpGroupNonUniformIMul, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpGroupNonUniformFMul:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformFMul"))
          return false;
        load_context.onGroupNonUniformFMul(Op::OpGroupNonUniformFMul, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpGroupNonUniformSMin:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformSMin"))
          return false;
        load_context.onGroupNonUniformSMin(Op::OpGroupNonUniformSMin, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpGroupNonUniformUMin:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformUMin"))
          return false;
        load_context.onGroupNonUniformUMin(Op::OpGroupNonUniformUMin, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpGroupNonUniformFMin:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformFMin"))
          return false;
        load_context.onGroupNonUniformFMin(Op::OpGroupNonUniformFMin, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpGroupNonUniformSMax:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformSMax"))
          return false;
        load_context.onGroupNonUniformSMax(Op::OpGroupNonUniformSMax, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpGroupNonUniformUMax:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformUMax"))
          return false;
        load_context.onGroupNonUniformUMax(Op::OpGroupNonUniformUMax, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpGroupNonUniformFMax:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformFMax"))
          return false;
        load_context.onGroupNonUniformFMax(Op::OpGroupNonUniformFMax, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpGroupNonUniformBitwiseAnd:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformBitwiseAnd"))
          return false;
        load_context.onGroupNonUniformBitwiseAnd(Op::OpGroupNonUniformBitwiseAnd, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpGroupNonUniformBitwiseOr:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformBitwiseOr"))
          return false;
        load_context.onGroupNonUniformBitwiseOr(Op::OpGroupNonUniformBitwiseOr, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpGroupNonUniformBitwiseXor:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformBitwiseXor"))
          return false;
        load_context.onGroupNonUniformBitwiseXor(Op::OpGroupNonUniformBitwiseXor, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpGroupNonUniformLogicalAnd:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformLogicalAnd"))
          return false;
        load_context.onGroupNonUniformLogicalAnd(Op::OpGroupNonUniformLogicalAnd, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpGroupNonUniformLogicalOr:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformLogicalOr"))
          return false;
        load_context.onGroupNonUniformLogicalOr(Op::OpGroupNonUniformLogicalOr, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpGroupNonUniformLogicalXor:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformLogicalXor"))
          return false;
        load_context.onGroupNonUniformLogicalXor(Op::OpGroupNonUniformLogicalXor, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpGroupNonUniformQuadBroadcast:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformQuadBroadcast"))
          return false;
        load_context.onGroupNonUniformQuadBroadcast(Op::OpGroupNonUniformQuadBroadcast, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupNonUniformQuadSwap:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformQuadSwap"))
          return false;
        load_context.onGroupNonUniformQuadSwap(Op::OpGroupNonUniformQuadSwap, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpCopyLogical:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpCopyLogical"))
          return false;
        load_context.onCopyLogical(Op::OpCopyLogical, c1, c0, c2);
        break;
      }
      case Op::OpPtrEqual:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpPtrEqual"))
          return false;
        load_context.onPtrEqual(Op::OpPtrEqual, c1, c0, c2, c3);
        break;
      }
      case Op::OpPtrNotEqual:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpPtrNotEqual"))
          return false;
        load_context.onPtrNotEqual(Op::OpPtrNotEqual, c1, c0, c2, c3);
        break;
      }
      case Op::OpPtrDiff:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpPtrDiff"))
          return false;
        load_context.onPtrDiff(Op::OpPtrDiff, c1, c0, c2, c3);
        break;
      }
      case Op::OpColorAttachmentReadEXT:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpColorAttachmentReadEXT"))
          return false;
        load_context.onColorAttachmentReadEXT(Op::OpColorAttachmentReadEXT, c1, c0, c2, c3);
        break;
      }
      case Op::OpDepthAttachmentReadEXT:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpDepthAttachmentReadEXT"))
          return false;
        load_context.onDepthAttachmentReadEXT(Op::OpDepthAttachmentReadEXT, c1, c0, c2);
        break;
      }
      case Op::OpStencilAttachmentReadEXT:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpStencilAttachmentReadEXT"))
          return false;
        load_context.onStencilAttachmentReadEXT(Op::OpStencilAttachmentReadEXT, c1, c0, c2);
        break;
      }
      case Op::OpTerminateInvocation:
      {
        if (!operands.finishOperands(on_error, "OpTerminateInvocation"))
          return false;
        load_context.onTerminateInvocation(Op::OpTerminateInvocation);
        break;
      }
      case Op::OpSubgroupBallotKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupBallotKHR"))
          return false;
        load_context.onSubgroupBallotKHR(Op::OpSubgroupBallotKHR, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupFirstInvocationKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupFirstInvocationKHR"))
          return false;
        load_context.onSubgroupFirstInvocationKHR(Op::OpSubgroupFirstInvocationKHR, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAllKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAllKHR"))
          return false;
        load_context.onSubgroupAllKHR(Op::OpSubgroupAllKHR, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAnyKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAnyKHR"))
          return false;
        load_context.onSubgroupAnyKHR(Op::OpSubgroupAnyKHR, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAllEqualKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAllEqualKHR"))
          return false;
        load_context.onSubgroupAllEqualKHR(Op::OpSubgroupAllEqualKHR, c1, c0, c2);
        break;
      }
      case Op::OpGroupNonUniformRotateKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformRotateKHR"))
          return false;
        load_context.onGroupNonUniformRotateKHR(Op::OpGroupNonUniformRotateKHR, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpSubgroupReadInvocationKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupReadInvocationKHR"))
          return false;
        load_context.onSubgroupReadInvocationKHR(Op::OpSubgroupReadInvocationKHR, c1, c0, c2, c3);
        break;
      }
      case Op::OpExtInstWithForwardRefsKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralExtInstInteger>();
        auto c4 = operands.read<Multiple<IdRef>>();
        if (!operands.finishOperands(on_error, "OpExtInstWithForwardRefsKHR"))
          return false;
        load_context.onExtInstWithForwardRefsKHR(Op::OpExtInstWithForwardRefsKHR, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpTraceRayKHR:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        auto c8 = operands.read<IdRef>();
        auto c9 = operands.read<IdRef>();
        auto c10 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpTraceRayKHR"))
          return false;
        load_context.onTraceRayKHR(Op::OpTraceRayKHR, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10);
        break;
      }
      case Op::OpExecuteCallableKHR:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpExecuteCallableKHR"))
          return false;
        load_context.onExecuteCallableKHR(Op::OpExecuteCallableKHR, c0, c1);
        break;
      }
      case Op::OpConvertUToAccelerationStructureKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpConvertUToAccelerationStructureKHR"))
          return false;
        load_context.onConvertUToAccelerationStructureKHR(Op::OpConvertUToAccelerationStructureKHR, c1, c0, c2);
        break;
      }
      case Op::OpIgnoreIntersectionKHR:
      {
        if (!operands.finishOperands(on_error, "OpIgnoreIntersectionKHR"))
          return false;
        load_context.onIgnoreIntersectionKHR(Op::OpIgnoreIntersectionKHR);
        break;
      }
      case Op::OpTerminateRayKHR:
      {
        if (!operands.finishOperands(on_error, "OpTerminateRayKHR"))
          return false;
        load_context.onTerminateRayKHR(Op::OpTerminateRayKHR);
        break;
      }
      case Op::OpSDot:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<Optional<PackedVectorFormat>>();
        if (!operands.finishOperands(on_error, "OpSDot"))
          return false;
        load_context.onSDot(Op::OpSDot, c1, c0, c2, c3, c4);
        break;
      }
      // OpSDotKHR alias of OpSDot
      case Op::OpUDot:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<Optional<PackedVectorFormat>>();
        if (!operands.finishOperands(on_error, "OpUDot"))
          return false;
        load_context.onUDot(Op::OpUDot, c1, c0, c2, c3, c4);
        break;
      }
      // OpUDotKHR alias of OpUDot
      case Op::OpSUDot:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<Optional<PackedVectorFormat>>();
        if (!operands.finishOperands(on_error, "OpSUDot"))
          return false;
        load_context.onSUDot(Op::OpSUDot, c1, c0, c2, c3, c4);
        break;
      }
      // OpSUDotKHR alias of OpSUDot
      case Op::OpSDotAccSat:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<PackedVectorFormat>>();
        if (!operands.finishOperands(on_error, "OpSDotAccSat"))
          return false;
        load_context.onSDotAccSat(Op::OpSDotAccSat, c1, c0, c2, c3, c4, c5);
        break;
      }
      // OpSDotAccSatKHR alias of OpSDotAccSat
      case Op::OpUDotAccSat:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<PackedVectorFormat>>();
        if (!operands.finishOperands(on_error, "OpUDotAccSat"))
          return false;
        load_context.onUDotAccSat(Op::OpUDotAccSat, c1, c0, c2, c3, c4, c5);
        break;
      }
      // OpUDotAccSatKHR alias of OpUDotAccSat
      case Op::OpSUDotAccSat:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<PackedVectorFormat>>();
        if (!operands.finishOperands(on_error, "OpSUDotAccSat"))
          return false;
        load_context.onSUDotAccSat(Op::OpSUDotAccSat, c1, c0, c2, c3, c4, c5);
        break;
      }
      // OpSUDotAccSatKHR alias of OpSUDotAccSat
      case Op::OpTypeCooperativeMatrixKHR:
      {
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpTypeCooperativeMatrixKHR"))
          return false;
        load_context.onTypeCooperativeMatrixKHR(Op::OpTypeCooperativeMatrixKHR, c0, c1, c2, c3, c4, c5);
        break;
      }
      case Op::OpCooperativeMatrixLoadKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<Optional<IdRef>>();
        auto c5 = operands.read<Optional<MemoryAccessMask>>();
        if (!operands.finishOperands(on_error, "OpCooperativeMatrixLoadKHR"))
          return false;
        load_context.onCooperativeMatrixLoadKHR(Op::OpCooperativeMatrixLoadKHR, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpCooperativeMatrixStoreKHR:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<Optional<IdRef>>();
        auto c4 = operands.read<Optional<MemoryAccessMask>>();
        if (!operands.finishOperands(on_error, "OpCooperativeMatrixStoreKHR"))
          return false;
        load_context.onCooperativeMatrixStoreKHR(Op::OpCooperativeMatrixStoreKHR, c0, c1, c2, c3, c4);
        break;
      }
      case Op::OpCooperativeMatrixMulAddKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<CooperativeMatrixOperandsMask>>();
        if (!operands.finishOperands(on_error, "OpCooperativeMatrixMulAddKHR"))
          return false;
        load_context.onCooperativeMatrixMulAddKHR(Op::OpCooperativeMatrixMulAddKHR, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpCooperativeMatrixLengthKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpCooperativeMatrixLengthKHR"))
          return false;
        load_context.onCooperativeMatrixLengthKHR(Op::OpCooperativeMatrixLengthKHR, c1, c0, c2);
        break;
      }
      case Op::OpConstantCompositeReplicateEXT:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpConstantCompositeReplicateEXT"))
          return false;
        load_context.onConstantCompositeReplicateEXT(Op::OpConstantCompositeReplicateEXT, c1, c0, c2);
        break;
      }
      case Op::OpSpecConstantCompositeReplicateEXT:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSpecConstantCompositeReplicateEXT"))
          return false;
        load_context.onSpecConstantCompositeReplicateEXT(Op::OpSpecConstantCompositeReplicateEXT, c1, c0, c2);
        break;
      }
      case Op::OpCompositeConstructReplicateEXT:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpCompositeConstructReplicateEXT"))
          return false;
        load_context.onCompositeConstructReplicateEXT(Op::OpCompositeConstructReplicateEXT, c1, c0, c2);
        break;
      }
      case Op::OpTypeRayQueryKHR:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeRayQueryKHR"))
          return false;
        load_context.onTypeRayQueryKHR(Op::OpTypeRayQueryKHR, c0);
        break;
      }
      case Op::OpRayQueryInitializeKHR:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryInitializeKHR"))
          return false;
        load_context.onRayQueryInitializeKHR(Op::OpRayQueryInitializeKHR, c0, c1, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpRayQueryTerminateKHR:
      {
        auto c0 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryTerminateKHR"))
          return false;
        load_context.onRayQueryTerminateKHR(Op::OpRayQueryTerminateKHR, c0);
        break;
      }
      case Op::OpRayQueryGenerateIntersectionKHR:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryGenerateIntersectionKHR"))
          return false;
        load_context.onRayQueryGenerateIntersectionKHR(Op::OpRayQueryGenerateIntersectionKHR, c0, c1);
        break;
      }
      case Op::OpRayQueryConfirmIntersectionKHR:
      {
        auto c0 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryConfirmIntersectionKHR"))
          return false;
        load_context.onRayQueryConfirmIntersectionKHR(Op::OpRayQueryConfirmIntersectionKHR, c0);
        break;
      }
      case Op::OpRayQueryProceedKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryProceedKHR"))
          return false;
        load_context.onRayQueryProceedKHR(Op::OpRayQueryProceedKHR, c1, c0, c2);
        break;
      }
      case Op::OpRayQueryGetIntersectionTypeKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryGetIntersectionTypeKHR"))
          return false;
        load_context.onRayQueryGetIntersectionTypeKHR(Op::OpRayQueryGetIntersectionTypeKHR, c1, c0, c2, c3);
        break;
      }
      case Op::OpImageSampleWeightedQCOM:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpImageSampleWeightedQCOM"))
          return false;
        load_context.onImageSampleWeightedQCOM(Op::OpImageSampleWeightedQCOM, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpImageBoxFilterQCOM:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpImageBoxFilterQCOM"))
          return false;
        load_context.onImageBoxFilterQCOM(Op::OpImageBoxFilterQCOM, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpImageBlockMatchSSDQCOM:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpImageBlockMatchSSDQCOM"))
          return false;
        load_context.onImageBlockMatchSSDQCOM(Op::OpImageBlockMatchSSDQCOM, c1, c0, c2, c3, c4, c5, c6);
        break;
      }
      case Op::OpImageBlockMatchSADQCOM:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpImageBlockMatchSADQCOM"))
          return false;
        load_context.onImageBlockMatchSADQCOM(Op::OpImageBlockMatchSADQCOM, c1, c0, c2, c3, c4, c5, c6);
        break;
      }
      case Op::OpImageBlockMatchWindowSSDQCOM:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpImageBlockMatchWindowSSDQCOM"))
          return false;
        load_context.onImageBlockMatchWindowSSDQCOM(Op::OpImageBlockMatchWindowSSDQCOM, c1, c0, c2, c3, c4, c5, c6);
        break;
      }
      case Op::OpImageBlockMatchWindowSADQCOM:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpImageBlockMatchWindowSADQCOM"))
          return false;
        load_context.onImageBlockMatchWindowSADQCOM(Op::OpImageBlockMatchWindowSADQCOM, c1, c0, c2, c3, c4, c5, c6);
        break;
      }
      case Op::OpImageBlockMatchGatherSSDQCOM:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpImageBlockMatchGatherSSDQCOM"))
          return false;
        load_context.onImageBlockMatchGatherSSDQCOM(Op::OpImageBlockMatchGatherSSDQCOM, c1, c0, c2, c3, c4, c5, c6);
        break;
      }
      case Op::OpImageBlockMatchGatherSADQCOM:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpImageBlockMatchGatherSADQCOM"))
          return false;
        load_context.onImageBlockMatchGatherSADQCOM(Op::OpImageBlockMatchGatherSADQCOM, c1, c0, c2, c3, c4, c5, c6);
        break;
      }
      case Op::OpGroupIAddNonUniformAMD:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupIAddNonUniformAMD"))
          return false;
        load_context.onGroupIAddNonUniformAMD(Op::OpGroupIAddNonUniformAMD, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupFAddNonUniformAMD:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupFAddNonUniformAMD"))
          return false;
        load_context.onGroupFAddNonUniformAMD(Op::OpGroupFAddNonUniformAMD, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupFMinNonUniformAMD:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupFMinNonUniformAMD"))
          return false;
        load_context.onGroupFMinNonUniformAMD(Op::OpGroupFMinNonUniformAMD, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupUMinNonUniformAMD:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupUMinNonUniformAMD"))
          return false;
        load_context.onGroupUMinNonUniformAMD(Op::OpGroupUMinNonUniformAMD, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupSMinNonUniformAMD:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupSMinNonUniformAMD"))
          return false;
        load_context.onGroupSMinNonUniformAMD(Op::OpGroupSMinNonUniformAMD, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupFMaxNonUniformAMD:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupFMaxNonUniformAMD"))
          return false;
        load_context.onGroupFMaxNonUniformAMD(Op::OpGroupFMaxNonUniformAMD, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupUMaxNonUniformAMD:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupUMaxNonUniformAMD"))
          return false;
        load_context.onGroupUMaxNonUniformAMD(Op::OpGroupUMaxNonUniformAMD, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupSMaxNonUniformAMD:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupSMaxNonUniformAMD"))
          return false;
        load_context.onGroupSMaxNonUniformAMD(Op::OpGroupSMaxNonUniformAMD, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpFragmentMaskFetchAMD:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFragmentMaskFetchAMD"))
          return false;
        load_context.onFragmentMaskFetchAMD(Op::OpFragmentMaskFetchAMD, c1, c0, c2, c3);
        break;
      }
      case Op::OpFragmentFetchAMD:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFragmentFetchAMD"))
          return false;
        load_context.onFragmentFetchAMD(Op::OpFragmentFetchAMD, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpReadClockKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        if (!operands.finishOperands(on_error, "OpReadClockKHR"))
          return false;
        load_context.onReadClockKHR(Op::OpReadClockKHR, c1, c0, c2);
        break;
      }
      case Op::OpFinalizeNodePayloadsAMDX:
      {
        auto c0 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFinalizeNodePayloadsAMDX"))
          return false;
        load_context.onFinalizeNodePayloadsAMDX(Op::OpFinalizeNodePayloadsAMDX, c0);
        break;
      }
      case Op::OpFinishWritingNodePayloadAMDX:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFinishWritingNodePayloadAMDX"))
          return false;
        load_context.onFinishWritingNodePayloadAMDX(Op::OpFinishWritingNodePayloadAMDX, c1, c0, c2);
        break;
      }
      case Op::OpInitializeNodePayloadsAMDX:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdScope>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpInitializeNodePayloadsAMDX"))
          return false;
        load_context.onInitializeNodePayloadsAMDX(Op::OpInitializeNodePayloadsAMDX, c0, c1, c2, c3);
        break;
      }
      case Op::OpGroupNonUniformQuadAllKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformQuadAllKHR"))
          return false;
        load_context.onGroupNonUniformQuadAllKHR(Op::OpGroupNonUniformQuadAllKHR, c1, c0, c2);
        break;
      }
      case Op::OpGroupNonUniformQuadAnyKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformQuadAnyKHR"))
          return false;
        load_context.onGroupNonUniformQuadAnyKHR(Op::OpGroupNonUniformQuadAnyKHR, c1, c0, c2);
        break;
      }
      case Op::OpHitObjectRecordHitMotionNV:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        auto c8 = operands.read<IdRef>();
        auto c9 = operands.read<IdRef>();
        auto c10 = operands.read<IdRef>();
        auto c11 = operands.read<IdRef>();
        auto c12 = operands.read<IdRef>();
        auto c13 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectRecordHitMotionNV"))
          return false;
        load_context.onHitObjectRecordHitMotionNV(Op::OpHitObjectRecordHitMotionNV, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11,
          c12, c13);
        break;
      }
      case Op::OpHitObjectRecordHitWithIndexMotionNV:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        auto c8 = operands.read<IdRef>();
        auto c9 = operands.read<IdRef>();
        auto c10 = operands.read<IdRef>();
        auto c11 = operands.read<IdRef>();
        auto c12 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectRecordHitWithIndexMotionNV"))
          return false;
        load_context.onHitObjectRecordHitWithIndexMotionNV(Op::OpHitObjectRecordHitWithIndexMotionNV, c0, c1, c2, c3, c4, c5, c6, c7,
          c8, c9, c10, c11, c12);
        break;
      }
      case Op::OpHitObjectRecordMissMotionNV:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectRecordMissMotionNV"))
          return false;
        load_context.onHitObjectRecordMissMotionNV(Op::OpHitObjectRecordMissMotionNV, c0, c1, c2, c3, c4, c5, c6);
        break;
      }
      case Op::OpHitObjectGetWorldToObjectNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectGetWorldToObjectNV"))
          return false;
        load_context.onHitObjectGetWorldToObjectNV(Op::OpHitObjectGetWorldToObjectNV, c1, c0, c2);
        break;
      }
      case Op::OpHitObjectGetObjectToWorldNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectGetObjectToWorldNV"))
          return false;
        load_context.onHitObjectGetObjectToWorldNV(Op::OpHitObjectGetObjectToWorldNV, c1, c0, c2);
        break;
      }
      case Op::OpHitObjectGetObjectRayDirectionNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectGetObjectRayDirectionNV"))
          return false;
        load_context.onHitObjectGetObjectRayDirectionNV(Op::OpHitObjectGetObjectRayDirectionNV, c1, c0, c2);
        break;
      }
      case Op::OpHitObjectGetObjectRayOriginNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectGetObjectRayOriginNV"))
          return false;
        load_context.onHitObjectGetObjectRayOriginNV(Op::OpHitObjectGetObjectRayOriginNV, c1, c0, c2);
        break;
      }
      case Op::OpHitObjectTraceRayMotionNV:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        auto c8 = operands.read<IdRef>();
        auto c9 = operands.read<IdRef>();
        auto c10 = operands.read<IdRef>();
        auto c11 = operands.read<IdRef>();
        auto c12 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectTraceRayMotionNV"))
          return false;
        load_context.onHitObjectTraceRayMotionNV(Op::OpHitObjectTraceRayMotionNV, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11,
          c12);
        break;
      }
      case Op::OpHitObjectGetShaderRecordBufferHandleNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectGetShaderRecordBufferHandleNV"))
          return false;
        load_context.onHitObjectGetShaderRecordBufferHandleNV(Op::OpHitObjectGetShaderRecordBufferHandleNV, c1, c0, c2);
        break;
      }
      case Op::OpHitObjectGetShaderBindingTableRecordIndexNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectGetShaderBindingTableRecordIndexNV"))
          return false;
        load_context.onHitObjectGetShaderBindingTableRecordIndexNV(Op::OpHitObjectGetShaderBindingTableRecordIndexNV, c1, c0, c2);
        break;
      }
      case Op::OpHitObjectRecordEmptyNV:
      {
        auto c0 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectRecordEmptyNV"))
          return false;
        load_context.onHitObjectRecordEmptyNV(Op::OpHitObjectRecordEmptyNV, c0);
        break;
      }
      case Op::OpHitObjectTraceRayNV:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        auto c8 = operands.read<IdRef>();
        auto c9 = operands.read<IdRef>();
        auto c10 = operands.read<IdRef>();
        auto c11 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectTraceRayNV"))
          return false;
        load_context.onHitObjectTraceRayNV(Op::OpHitObjectTraceRayNV, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11);
        break;
      }
      case Op::OpHitObjectRecordHitNV:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        auto c8 = operands.read<IdRef>();
        auto c9 = operands.read<IdRef>();
        auto c10 = operands.read<IdRef>();
        auto c11 = operands.read<IdRef>();
        auto c12 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectRecordHitNV"))
          return false;
        load_context.onHitObjectRecordHitNV(Op::OpHitObjectRecordHitNV, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12);
        break;
      }
      case Op::OpHitObjectRecordHitWithIndexNV:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        auto c8 = operands.read<IdRef>();
        auto c9 = operands.read<IdRef>();
        auto c10 = operands.read<IdRef>();
        auto c11 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectRecordHitWithIndexNV"))
          return false;
        load_context.onHitObjectRecordHitWithIndexNV(Op::OpHitObjectRecordHitWithIndexNV, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10,
          c11);
        break;
      }
      case Op::OpHitObjectRecordMissNV:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectRecordMissNV"))
          return false;
        load_context.onHitObjectRecordMissNV(Op::OpHitObjectRecordMissNV, c0, c1, c2, c3, c4, c5);
        break;
      }
      case Op::OpHitObjectExecuteShaderNV:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectExecuteShaderNV"))
          return false;
        load_context.onHitObjectExecuteShaderNV(Op::OpHitObjectExecuteShaderNV, c0, c1);
        break;
      }
      case Op::OpHitObjectGetCurrentTimeNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectGetCurrentTimeNV"))
          return false;
        load_context.onHitObjectGetCurrentTimeNV(Op::OpHitObjectGetCurrentTimeNV, c1, c0, c2);
        break;
      }
      case Op::OpHitObjectGetAttributesNV:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectGetAttributesNV"))
          return false;
        load_context.onHitObjectGetAttributesNV(Op::OpHitObjectGetAttributesNV, c0, c1);
        break;
      }
      case Op::OpHitObjectGetHitKindNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectGetHitKindNV"))
          return false;
        load_context.onHitObjectGetHitKindNV(Op::OpHitObjectGetHitKindNV, c1, c0, c2);
        break;
      }
      case Op::OpHitObjectGetPrimitiveIndexNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectGetPrimitiveIndexNV"))
          return false;
        load_context.onHitObjectGetPrimitiveIndexNV(Op::OpHitObjectGetPrimitiveIndexNV, c1, c0, c2);
        break;
      }
      case Op::OpHitObjectGetGeometryIndexNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectGetGeometryIndexNV"))
          return false;
        load_context.onHitObjectGetGeometryIndexNV(Op::OpHitObjectGetGeometryIndexNV, c1, c0, c2);
        break;
      }
      case Op::OpHitObjectGetInstanceIdNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectGetInstanceIdNV"))
          return false;
        load_context.onHitObjectGetInstanceIdNV(Op::OpHitObjectGetInstanceIdNV, c1, c0, c2);
        break;
      }
      case Op::OpHitObjectGetInstanceCustomIndexNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectGetInstanceCustomIndexNV"))
          return false;
        load_context.onHitObjectGetInstanceCustomIndexNV(Op::OpHitObjectGetInstanceCustomIndexNV, c1, c0, c2);
        break;
      }
      case Op::OpHitObjectGetWorldRayDirectionNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectGetWorldRayDirectionNV"))
          return false;
        load_context.onHitObjectGetWorldRayDirectionNV(Op::OpHitObjectGetWorldRayDirectionNV, c1, c0, c2);
        break;
      }
      case Op::OpHitObjectGetWorldRayOriginNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectGetWorldRayOriginNV"))
          return false;
        load_context.onHitObjectGetWorldRayOriginNV(Op::OpHitObjectGetWorldRayOriginNV, c1, c0, c2);
        break;
      }
      case Op::OpHitObjectGetRayTMaxNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectGetRayTMaxNV"))
          return false;
        load_context.onHitObjectGetRayTMaxNV(Op::OpHitObjectGetRayTMaxNV, c1, c0, c2);
        break;
      }
      case Op::OpHitObjectGetRayTMinNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectGetRayTMinNV"))
          return false;
        load_context.onHitObjectGetRayTMinNV(Op::OpHitObjectGetRayTMinNV, c1, c0, c2);
        break;
      }
      case Op::OpHitObjectIsEmptyNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectIsEmptyNV"))
          return false;
        load_context.onHitObjectIsEmptyNV(Op::OpHitObjectIsEmptyNV, c1, c0, c2);
        break;
      }
      case Op::OpHitObjectIsHitNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectIsHitNV"))
          return false;
        load_context.onHitObjectIsHitNV(Op::OpHitObjectIsHitNV, c1, c0, c2);
        break;
      }
      case Op::OpHitObjectIsMissNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpHitObjectIsMissNV"))
          return false;
        load_context.onHitObjectIsMissNV(Op::OpHitObjectIsMissNV, c1, c0, c2);
        break;
      }
      case Op::OpReorderThreadWithHitObjectNV:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<Optional<IdRef>>();
        auto c2 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpReorderThreadWithHitObjectNV"))
          return false;
        load_context.onReorderThreadWithHitObjectNV(Op::OpReorderThreadWithHitObjectNV, c0, c1, c2);
        break;
      }
      case Op::OpReorderThreadWithHintNV:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpReorderThreadWithHintNV"))
          return false;
        load_context.onReorderThreadWithHintNV(Op::OpReorderThreadWithHintNV, c0, c1);
        break;
      }
      case Op::OpTypeHitObjectNV:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeHitObjectNV"))
          return false;
        load_context.onTypeHitObjectNV(Op::OpTypeHitObjectNV, c0);
        break;
      }
      case Op::OpImageSampleFootprintNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<Optional<ImageOperandsMask>>();
        if (!operands.finishOperands(on_error, "OpImageSampleFootprintNV"))
          return false;
        load_context.onImageSampleFootprintNV(Op::OpImageSampleFootprintNV, c1, c0, c2, c3, c4, c5, c6);
        break;
      }
      case Op::OpEmitMeshTasksEXT:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpEmitMeshTasksEXT"))
          return false;
        load_context.onEmitMeshTasksEXT(Op::OpEmitMeshTasksEXT, c0, c1, c2, c3);
        break;
      }
      case Op::OpSetMeshOutputsEXT:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSetMeshOutputsEXT"))
          return false;
        load_context.onSetMeshOutputsEXT(Op::OpSetMeshOutputsEXT, c0, c1);
        break;
      }
      case Op::OpGroupNonUniformPartitionNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupNonUniformPartitionNV"))
          return false;
        load_context.onGroupNonUniformPartitionNV(Op::OpGroupNonUniformPartitionNV, c1, c0, c2);
        break;
      }
      case Op::OpWritePackedPrimitiveIndices4x8NV:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpWritePackedPrimitiveIndices4x8NV"))
          return false;
        load_context.onWritePackedPrimitiveIndices4x8NV(Op::OpWritePackedPrimitiveIndices4x8NV, c0, c1);
        break;
      }
      case Op::OpFetchMicroTriangleVertexPositionNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFetchMicroTriangleVertexPositionNV"))
          return false;
        load_context.onFetchMicroTriangleVertexPositionNV(Op::OpFetchMicroTriangleVertexPositionNV, c1, c0, c2, c3, c4, c5, c6);
        break;
      }
      case Op::OpFetchMicroTriangleVertexBarycentricNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFetchMicroTriangleVertexBarycentricNV"))
          return false;
        load_context.onFetchMicroTriangleVertexBarycentricNV(Op::OpFetchMicroTriangleVertexBarycentricNV, c1, c0, c2, c3, c4, c5, c6);
        break;
      }
      case Op::OpReportIntersectionKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpReportIntersectionKHR"))
          return false;
        load_context.onReportIntersectionKHR(Op::OpReportIntersectionKHR, c1, c0, c2, c3);
        break;
      }
      // OpReportIntersectionNV alias of OpReportIntersectionKHR
      case Op::OpIgnoreIntersectionNV:
      {
        if (!operands.finishOperands(on_error, "OpIgnoreIntersectionNV"))
          return false;
        load_context.onIgnoreIntersectionNV(Op::OpIgnoreIntersectionNV);
        break;
      }
      case Op::OpTerminateRayNV:
      {
        if (!operands.finishOperands(on_error, "OpTerminateRayNV"))
          return false;
        load_context.onTerminateRayNV(Op::OpTerminateRayNV);
        break;
      }
      case Op::OpTraceNV:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        auto c8 = operands.read<IdRef>();
        auto c9 = operands.read<IdRef>();
        auto c10 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpTraceNV"))
          return false;
        load_context.onTraceNV(Op::OpTraceNV, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10);
        break;
      }
      case Op::OpTraceMotionNV:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        auto c8 = operands.read<IdRef>();
        auto c9 = operands.read<IdRef>();
        auto c10 = operands.read<IdRef>();
        auto c11 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpTraceMotionNV"))
          return false;
        load_context.onTraceMotionNV(Op::OpTraceMotionNV, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11);
        break;
      }
      case Op::OpTraceRayMotionNV:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        auto c8 = operands.read<IdRef>();
        auto c9 = operands.read<IdRef>();
        auto c10 = operands.read<IdRef>();
        auto c11 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpTraceRayMotionNV"))
          return false;
        load_context.onTraceRayMotionNV(Op::OpTraceRayMotionNV, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11);
        break;
      }
      case Op::OpRayQueryGetIntersectionTriangleVertexPositionsKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryGetIntersectionTriangleVertexPositionsKHR"))
          return false;
        load_context.onRayQueryGetIntersectionTriangleVertexPositionsKHR(Op::OpRayQueryGetIntersectionTriangleVertexPositionsKHR, c1,
          c0, c2, c3);
        break;
      }
      case Op::OpTypeAccelerationStructureKHR:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeAccelerationStructureKHR"))
          return false;
        load_context.onTypeAccelerationStructureKHR(Op::OpTypeAccelerationStructureKHR, c0);
        break;
      }
      // OpTypeAccelerationStructureNV alias of OpTypeAccelerationStructureKHR
      case Op::OpExecuteCallableNV:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpExecuteCallableNV"))
          return false;
        load_context.onExecuteCallableNV(Op::OpExecuteCallableNV, c0, c1);
        break;
      }
      case Op::OpTypeCooperativeMatrixNV:
      {
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpTypeCooperativeMatrixNV"))
          return false;
        load_context.onTypeCooperativeMatrixNV(Op::OpTypeCooperativeMatrixNV, c0, c1, c2, c3, c4);
        break;
      }
      case Op::OpCooperativeMatrixLoadNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<Optional<MemoryAccessMask>>();
        if (!operands.finishOperands(on_error, "OpCooperativeMatrixLoadNV"))
          return false;
        load_context.onCooperativeMatrixLoadNV(Op::OpCooperativeMatrixLoadNV, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpCooperativeMatrixStoreNV:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<Optional<MemoryAccessMask>>();
        if (!operands.finishOperands(on_error, "OpCooperativeMatrixStoreNV"))
          return false;
        load_context.onCooperativeMatrixStoreNV(Op::OpCooperativeMatrixStoreNV, c0, c1, c2, c3, c4);
        break;
      }
      case Op::OpCooperativeMatrixMulAddNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpCooperativeMatrixMulAddNV"))
          return false;
        load_context.onCooperativeMatrixMulAddNV(Op::OpCooperativeMatrixMulAddNV, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpCooperativeMatrixLengthNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpCooperativeMatrixLengthNV"))
          return false;
        load_context.onCooperativeMatrixLengthNV(Op::OpCooperativeMatrixLengthNV, c1, c0, c2);
        break;
      }
      case Op::OpBeginInvocationInterlockEXT:
      {
        if (!operands.finishOperands(on_error, "OpBeginInvocationInterlockEXT"))
          return false;
        load_context.onBeginInvocationInterlockEXT(Op::OpBeginInvocationInterlockEXT);
        break;
      }
      case Op::OpEndInvocationInterlockEXT:
      {
        if (!operands.finishOperands(on_error, "OpEndInvocationInterlockEXT"))
          return false;
        load_context.onEndInvocationInterlockEXT(Op::OpEndInvocationInterlockEXT);
        break;
      }
      case Op::OpDemoteToHelperInvocation:
      {
        if (!operands.finishOperands(on_error, "OpDemoteToHelperInvocation"))
          return false;
        load_context.onDemoteToHelperInvocation(Op::OpDemoteToHelperInvocation);
        break;
      }
      // OpDemoteToHelperInvocationEXT alias of OpDemoteToHelperInvocation
      case Op::OpIsHelperInvocationEXT:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpIsHelperInvocationEXT"))
          return false;
        load_context.onIsHelperInvocationEXT(Op::OpIsHelperInvocationEXT, c1, c0);
        break;
      }
      case Op::OpConvertUToImageNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpConvertUToImageNV"))
          return false;
        load_context.onConvertUToImageNV(Op::OpConvertUToImageNV, c1, c0, c2);
        break;
      }
      case Op::OpConvertUToSamplerNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpConvertUToSamplerNV"))
          return false;
        load_context.onConvertUToSamplerNV(Op::OpConvertUToSamplerNV, c1, c0, c2);
        break;
      }
      case Op::OpConvertImageToUNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpConvertImageToUNV"))
          return false;
        load_context.onConvertImageToUNV(Op::OpConvertImageToUNV, c1, c0, c2);
        break;
      }
      case Op::OpConvertSamplerToUNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpConvertSamplerToUNV"))
          return false;
        load_context.onConvertSamplerToUNV(Op::OpConvertSamplerToUNV, c1, c0, c2);
        break;
      }
      case Op::OpConvertUToSampledImageNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpConvertUToSampledImageNV"))
          return false;
        load_context.onConvertUToSampledImageNV(Op::OpConvertUToSampledImageNV, c1, c0, c2);
        break;
      }
      case Op::OpConvertSampledImageToUNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpConvertSampledImageToUNV"))
          return false;
        load_context.onConvertSampledImageToUNV(Op::OpConvertSampledImageToUNV, c1, c0, c2);
        break;
      }
      case Op::OpSamplerImageAddressingModeNV:
      {
        auto c0 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpSamplerImageAddressingModeNV"))
          return false;
        load_context.onSamplerImageAddressingModeNV(Op::OpSamplerImageAddressingModeNV, c0);
        break;
      }
      case Op::OpRawAccessChainNV:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<Optional<RawAccessChainOperandsMask>>();
        if (!operands.finishOperands(on_error, "OpRawAccessChainNV"))
          return false;
        load_context.onRawAccessChainNV(Op::OpRawAccessChainNV, c1, c0, c2, c3, c4, c5, c6);
        break;
      }
      case Op::OpSubgroupShuffleINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupShuffleINTEL"))
          return false;
        load_context.onSubgroupShuffleINTEL(Op::OpSubgroupShuffleINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupShuffleDownINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupShuffleDownINTEL"))
          return false;
        load_context.onSubgroupShuffleDownINTEL(Op::OpSubgroupShuffleDownINTEL, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpSubgroupShuffleUpINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupShuffleUpINTEL"))
          return false;
        load_context.onSubgroupShuffleUpINTEL(Op::OpSubgroupShuffleUpINTEL, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpSubgroupShuffleXorINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupShuffleXorINTEL"))
          return false;
        load_context.onSubgroupShuffleXorINTEL(Op::OpSubgroupShuffleXorINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupBlockReadINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupBlockReadINTEL"))
          return false;
        load_context.onSubgroupBlockReadINTEL(Op::OpSubgroupBlockReadINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupBlockWriteINTEL:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupBlockWriteINTEL"))
          return false;
        load_context.onSubgroupBlockWriteINTEL(Op::OpSubgroupBlockWriteINTEL, c0, c1);
        break;
      }
      case Op::OpSubgroupImageBlockReadINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupImageBlockReadINTEL"))
          return false;
        load_context.onSubgroupImageBlockReadINTEL(Op::OpSubgroupImageBlockReadINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupImageBlockWriteINTEL:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupImageBlockWriteINTEL"))
          return false;
        load_context.onSubgroupImageBlockWriteINTEL(Op::OpSubgroupImageBlockWriteINTEL, c0, c1, c2);
        break;
      }
      case Op::OpSubgroupImageMediaBlockReadINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupImageMediaBlockReadINTEL"))
          return false;
        load_context.onSubgroupImageMediaBlockReadINTEL(Op::OpSubgroupImageMediaBlockReadINTEL, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpSubgroupImageMediaBlockWriteINTEL:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupImageMediaBlockWriteINTEL"))
          return false;
        load_context.onSubgroupImageMediaBlockWriteINTEL(Op::OpSubgroupImageMediaBlockWriteINTEL, c0, c1, c2, c3, c4);
        break;
      }
      case Op::OpUCountLeadingZerosINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpUCountLeadingZerosINTEL"))
          return false;
        load_context.onUCountLeadingZerosINTEL(Op::OpUCountLeadingZerosINTEL, c1, c0, c2);
        break;
      }
      case Op::OpUCountTrailingZerosINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpUCountTrailingZerosINTEL"))
          return false;
        load_context.onUCountTrailingZerosINTEL(Op::OpUCountTrailingZerosINTEL, c1, c0, c2);
        break;
      }
      case Op::OpAbsISubINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpAbsISubINTEL"))
          return false;
        load_context.onAbsISubINTEL(Op::OpAbsISubINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpAbsUSubINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpAbsUSubINTEL"))
          return false;
        load_context.onAbsUSubINTEL(Op::OpAbsUSubINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpIAddSatINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpIAddSatINTEL"))
          return false;
        load_context.onIAddSatINTEL(Op::OpIAddSatINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpUAddSatINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpUAddSatINTEL"))
          return false;
        load_context.onUAddSatINTEL(Op::OpUAddSatINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpIAverageINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpIAverageINTEL"))
          return false;
        load_context.onIAverageINTEL(Op::OpIAverageINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpUAverageINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpUAverageINTEL"))
          return false;
        load_context.onUAverageINTEL(Op::OpUAverageINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpIAverageRoundedINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpIAverageRoundedINTEL"))
          return false;
        load_context.onIAverageRoundedINTEL(Op::OpIAverageRoundedINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpUAverageRoundedINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpUAverageRoundedINTEL"))
          return false;
        load_context.onUAverageRoundedINTEL(Op::OpUAverageRoundedINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpISubSatINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpISubSatINTEL"))
          return false;
        load_context.onISubSatINTEL(Op::OpISubSatINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpUSubSatINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpUSubSatINTEL"))
          return false;
        load_context.onUSubSatINTEL(Op::OpUSubSatINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpIMul32x16INTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpIMul32x16INTEL"))
          return false;
        load_context.onIMul32x16INTEL(Op::OpIMul32x16INTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpUMul32x16INTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpUMul32x16INTEL"))
          return false;
        load_context.onUMul32x16INTEL(Op::OpUMul32x16INTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpConstantFunctionPointerINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpConstantFunctionPointerINTEL"))
          return false;
        load_context.onConstantFunctionPointerINTEL(Op::OpConstantFunctionPointerINTEL, c1, c0, c2);
        break;
      }
      case Op::OpFunctionPointerCallINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<Multiple<IdRef>>();
        if (!operands.finishOperands(on_error, "OpFunctionPointerCallINTEL"))
          return false;
        load_context.onFunctionPointerCallINTEL(Op::OpFunctionPointerCallINTEL, c1, c0, c2);
        break;
      }
      case Op::OpAsmTargetINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<LiteralString>();
        if (!operands.finishOperands(on_error, "OpAsmTargetINTEL"))
          return false;
        load_context.onAsmTargetINTEL(Op::OpAsmTargetINTEL, c1, c0, c2);
        break;
      }
      case Op::OpAsmINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<LiteralString>();
        auto c5 = operands.read<LiteralString>();
        if (!operands.finishOperands(on_error, "OpAsmINTEL"))
          return false;
        load_context.onAsmINTEL(Op::OpAsmINTEL, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpAsmCallINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<Multiple<IdRef>>();
        if (!operands.finishOperands(on_error, "OpAsmCallINTEL"))
          return false;
        load_context.onAsmCallINTEL(Op::OpAsmCallINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpAtomicFMinEXT:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdScope>();
        auto c4 = operands.read<IdMemorySemantics>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpAtomicFMinEXT"))
          return false;
        load_context.onAtomicFMinEXT(Op::OpAtomicFMinEXT, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpAtomicFMaxEXT:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdScope>();
        auto c4 = operands.read<IdMemorySemantics>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpAtomicFMaxEXT"))
          return false;
        load_context.onAtomicFMaxEXT(Op::OpAtomicFMaxEXT, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpAssumeTrueKHR:
      {
        auto c0 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpAssumeTrueKHR"))
          return false;
        load_context.onAssumeTrueKHR(Op::OpAssumeTrueKHR, c0);
        break;
      }
      case Op::OpExpectKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpExpectKHR"))
          return false;
        load_context.onExpectKHR(Op::OpExpectKHR, c1, c0, c2, c3);
        break;
      }
      case Op::OpDecorateString:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<Decoration>();
        if (!operands.finishOperands(on_error, "OpDecorateString"))
          return false;
        load_context.onDecorateString(Op::OpDecorateString, c0, c1);
        break;
      }
      // OpDecorateStringGOOGLE alias of OpDecorateString
      case Op::OpMemberDecorateString:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<LiteralInteger>();
        auto c2 = operands.read<Decoration>();
        if (!operands.finishOperands(on_error, "OpMemberDecorateString"))
          return false;
        load_context.onMemberDecorateString(Op::OpMemberDecorateString, c0, c1, c2);
        break;
      }
      // OpMemberDecorateStringGOOGLE alias of OpMemberDecorateString
      case Op::OpVmeImageINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpVmeImageINTEL"))
          return false;
        load_context.onVmeImageINTEL(Op::OpVmeImageINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpTypeVmeImageINTEL:
      {
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpTypeVmeImageINTEL"))
          return false;
        load_context.onTypeVmeImageINTEL(Op::OpTypeVmeImageINTEL, c0, c1);
        break;
      }
      case Op::OpTypeAvcImePayloadINTEL:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeAvcImePayloadINTEL"))
          return false;
        load_context.onTypeAvcImePayloadINTEL(Op::OpTypeAvcImePayloadINTEL, c0);
        break;
      }
      case Op::OpTypeAvcRefPayloadINTEL:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeAvcRefPayloadINTEL"))
          return false;
        load_context.onTypeAvcRefPayloadINTEL(Op::OpTypeAvcRefPayloadINTEL, c0);
        break;
      }
      case Op::OpTypeAvcSicPayloadINTEL:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeAvcSicPayloadINTEL"))
          return false;
        load_context.onTypeAvcSicPayloadINTEL(Op::OpTypeAvcSicPayloadINTEL, c0);
        break;
      }
      case Op::OpTypeAvcMcePayloadINTEL:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeAvcMcePayloadINTEL"))
          return false;
        load_context.onTypeAvcMcePayloadINTEL(Op::OpTypeAvcMcePayloadINTEL, c0);
        break;
      }
      case Op::OpTypeAvcMceResultINTEL:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeAvcMceResultINTEL"))
          return false;
        load_context.onTypeAvcMceResultINTEL(Op::OpTypeAvcMceResultINTEL, c0);
        break;
      }
      case Op::OpTypeAvcImeResultINTEL:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeAvcImeResultINTEL"))
          return false;
        load_context.onTypeAvcImeResultINTEL(Op::OpTypeAvcImeResultINTEL, c0);
        break;
      }
      case Op::OpTypeAvcImeResultSingleReferenceStreamoutINTEL:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeAvcImeResultSingleReferenceStreamoutINTEL"))
          return false;
        load_context.onTypeAvcImeResultSingleReferenceStreamoutINTEL(Op::OpTypeAvcImeResultSingleReferenceStreamoutINTEL, c0);
        break;
      }
      case Op::OpTypeAvcImeResultDualReferenceStreamoutINTEL:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeAvcImeResultDualReferenceStreamoutINTEL"))
          return false;
        load_context.onTypeAvcImeResultDualReferenceStreamoutINTEL(Op::OpTypeAvcImeResultDualReferenceStreamoutINTEL, c0);
        break;
      }
      case Op::OpTypeAvcImeSingleReferenceStreaminINTEL:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeAvcImeSingleReferenceStreaminINTEL"))
          return false;
        load_context.onTypeAvcImeSingleReferenceStreaminINTEL(Op::OpTypeAvcImeSingleReferenceStreaminINTEL, c0);
        break;
      }
      case Op::OpTypeAvcImeDualReferenceStreaminINTEL:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeAvcImeDualReferenceStreaminINTEL"))
          return false;
        load_context.onTypeAvcImeDualReferenceStreaminINTEL(Op::OpTypeAvcImeDualReferenceStreaminINTEL, c0);
        break;
      }
      case Op::OpTypeAvcRefResultINTEL:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeAvcRefResultINTEL"))
          return false;
        load_context.onTypeAvcRefResultINTEL(Op::OpTypeAvcRefResultINTEL, c0);
        break;
      }
      case Op::OpTypeAvcSicResultINTEL:
      {
        auto c0 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpTypeAvcSicResultINTEL"))
          return false;
        load_context.onTypeAvcSicResultINTEL(Op::OpTypeAvcSicResultINTEL, c0);
        break;
      }
      case Op::OpSubgroupAvcMceGetDefaultInterBaseMultiReferencePenaltyINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceGetDefaultInterBaseMultiReferencePenaltyINTEL"))
          return false;
        load_context.onSubgroupAvcMceGetDefaultInterBaseMultiReferencePenaltyINTEL(
          Op::OpSubgroupAvcMceGetDefaultInterBaseMultiReferencePenaltyINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcMceSetInterBaseMultiReferencePenaltyINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceSetInterBaseMultiReferencePenaltyINTEL"))
          return false;
        load_context.onSubgroupAvcMceSetInterBaseMultiReferencePenaltyINTEL(Op::OpSubgroupAvcMceSetInterBaseMultiReferencePenaltyINTEL,
          c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcMceGetDefaultInterShapePenaltyINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceGetDefaultInterShapePenaltyINTEL"))
          return false;
        load_context.onSubgroupAvcMceGetDefaultInterShapePenaltyINTEL(Op::OpSubgroupAvcMceGetDefaultInterShapePenaltyINTEL, c1, c0, c2,
          c3);
        break;
      }
      case Op::OpSubgroupAvcMceSetInterShapePenaltyINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceSetInterShapePenaltyINTEL"))
          return false;
        load_context.onSubgroupAvcMceSetInterShapePenaltyINTEL(Op::OpSubgroupAvcMceSetInterShapePenaltyINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcMceGetDefaultInterDirectionPenaltyINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceGetDefaultInterDirectionPenaltyINTEL"))
          return false;
        load_context.onSubgroupAvcMceGetDefaultInterDirectionPenaltyINTEL(Op::OpSubgroupAvcMceGetDefaultInterDirectionPenaltyINTEL, c1,
          c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcMceSetInterDirectionPenaltyINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceSetInterDirectionPenaltyINTEL"))
          return false;
        load_context.onSubgroupAvcMceSetInterDirectionPenaltyINTEL(Op::OpSubgroupAvcMceSetInterDirectionPenaltyINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcMceGetDefaultIntraLumaShapePenaltyINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceGetDefaultIntraLumaShapePenaltyINTEL"))
          return false;
        load_context.onSubgroupAvcMceGetDefaultIntraLumaShapePenaltyINTEL(Op::OpSubgroupAvcMceGetDefaultIntraLumaShapePenaltyINTEL, c1,
          c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcMceGetDefaultInterMotionVectorCostTableINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceGetDefaultInterMotionVectorCostTableINTEL"))
          return false;
        load_context.onSubgroupAvcMceGetDefaultInterMotionVectorCostTableINTEL(
          Op::OpSubgroupAvcMceGetDefaultInterMotionVectorCostTableINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcMceGetDefaultHighPenaltyCostTableINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceGetDefaultHighPenaltyCostTableINTEL"))
          return false;
        load_context.onSubgroupAvcMceGetDefaultHighPenaltyCostTableINTEL(Op::OpSubgroupAvcMceGetDefaultHighPenaltyCostTableINTEL, c1,
          c0);
        break;
      }
      case Op::OpSubgroupAvcMceGetDefaultMediumPenaltyCostTableINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceGetDefaultMediumPenaltyCostTableINTEL"))
          return false;
        load_context.onSubgroupAvcMceGetDefaultMediumPenaltyCostTableINTEL(Op::OpSubgroupAvcMceGetDefaultMediumPenaltyCostTableINTEL,
          c1, c0);
        break;
      }
      case Op::OpSubgroupAvcMceGetDefaultLowPenaltyCostTableINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceGetDefaultLowPenaltyCostTableINTEL"))
          return false;
        load_context.onSubgroupAvcMceGetDefaultLowPenaltyCostTableINTEL(Op::OpSubgroupAvcMceGetDefaultLowPenaltyCostTableINTEL, c1,
          c0);
        break;
      }
      case Op::OpSubgroupAvcMceSetMotionVectorCostFunctionINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceSetMotionVectorCostFunctionINTEL"))
          return false;
        load_context.onSubgroupAvcMceSetMotionVectorCostFunctionINTEL(Op::OpSubgroupAvcMceSetMotionVectorCostFunctionINTEL, c1, c0, c2,
          c3, c4, c5);
        break;
      }
      case Op::OpSubgroupAvcMceGetDefaultIntraLumaModePenaltyINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceGetDefaultIntraLumaModePenaltyINTEL"))
          return false;
        load_context.onSubgroupAvcMceGetDefaultIntraLumaModePenaltyINTEL(Op::OpSubgroupAvcMceGetDefaultIntraLumaModePenaltyINTEL, c1,
          c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcMceGetDefaultNonDcLumaIntraPenaltyINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceGetDefaultNonDcLumaIntraPenaltyINTEL"))
          return false;
        load_context.onSubgroupAvcMceGetDefaultNonDcLumaIntraPenaltyINTEL(Op::OpSubgroupAvcMceGetDefaultNonDcLumaIntraPenaltyINTEL, c1,
          c0);
        break;
      }
      case Op::OpSubgroupAvcMceGetDefaultIntraChromaModeBasePenaltyINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceGetDefaultIntraChromaModeBasePenaltyINTEL"))
          return false;
        load_context.onSubgroupAvcMceGetDefaultIntraChromaModeBasePenaltyINTEL(
          Op::OpSubgroupAvcMceGetDefaultIntraChromaModeBasePenaltyINTEL, c1, c0);
        break;
      }
      case Op::OpSubgroupAvcMceSetAcOnlyHaarINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceSetAcOnlyHaarINTEL"))
          return false;
        load_context.onSubgroupAvcMceSetAcOnlyHaarINTEL(Op::OpSubgroupAvcMceSetAcOnlyHaarINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcMceSetSourceInterlacedFieldPolarityINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceSetSourceInterlacedFieldPolarityINTEL"))
          return false;
        load_context.onSubgroupAvcMceSetSourceInterlacedFieldPolarityINTEL(Op::OpSubgroupAvcMceSetSourceInterlacedFieldPolarityINTEL,
          c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcMceSetSingleReferenceInterlacedFieldPolarityINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceSetSingleReferenceInterlacedFieldPolarityINTEL"))
          return false;
        load_context.onSubgroupAvcMceSetSingleReferenceInterlacedFieldPolarityINTEL(
          Op::OpSubgroupAvcMceSetSingleReferenceInterlacedFieldPolarityINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcMceSetDualReferenceInterlacedFieldPolaritiesINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceSetDualReferenceInterlacedFieldPolaritiesINTEL"))
          return false;
        load_context.onSubgroupAvcMceSetDualReferenceInterlacedFieldPolaritiesINTEL(
          Op::OpSubgroupAvcMceSetDualReferenceInterlacedFieldPolaritiesINTEL, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpSubgroupAvcMceConvertToImePayloadINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceConvertToImePayloadINTEL"))
          return false;
        load_context.onSubgroupAvcMceConvertToImePayloadINTEL(Op::OpSubgroupAvcMceConvertToImePayloadINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcMceConvertToImeResultINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceConvertToImeResultINTEL"))
          return false;
        load_context.onSubgroupAvcMceConvertToImeResultINTEL(Op::OpSubgroupAvcMceConvertToImeResultINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcMceConvertToRefPayloadINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceConvertToRefPayloadINTEL"))
          return false;
        load_context.onSubgroupAvcMceConvertToRefPayloadINTEL(Op::OpSubgroupAvcMceConvertToRefPayloadINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcMceConvertToRefResultINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceConvertToRefResultINTEL"))
          return false;
        load_context.onSubgroupAvcMceConvertToRefResultINTEL(Op::OpSubgroupAvcMceConvertToRefResultINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcMceConvertToSicPayloadINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceConvertToSicPayloadINTEL"))
          return false;
        load_context.onSubgroupAvcMceConvertToSicPayloadINTEL(Op::OpSubgroupAvcMceConvertToSicPayloadINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcMceConvertToSicResultINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceConvertToSicResultINTEL"))
          return false;
        load_context.onSubgroupAvcMceConvertToSicResultINTEL(Op::OpSubgroupAvcMceConvertToSicResultINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcMceGetMotionVectorsINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceGetMotionVectorsINTEL"))
          return false;
        load_context.onSubgroupAvcMceGetMotionVectorsINTEL(Op::OpSubgroupAvcMceGetMotionVectorsINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcMceGetInterDistortionsINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceGetInterDistortionsINTEL"))
          return false;
        load_context.onSubgroupAvcMceGetInterDistortionsINTEL(Op::OpSubgroupAvcMceGetInterDistortionsINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcMceGetBestInterDistortionsINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceGetBestInterDistortionsINTEL"))
          return false;
        load_context.onSubgroupAvcMceGetBestInterDistortionsINTEL(Op::OpSubgroupAvcMceGetBestInterDistortionsINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcMceGetInterMajorShapeINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceGetInterMajorShapeINTEL"))
          return false;
        load_context.onSubgroupAvcMceGetInterMajorShapeINTEL(Op::OpSubgroupAvcMceGetInterMajorShapeINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcMceGetInterMinorShapeINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceGetInterMinorShapeINTEL"))
          return false;
        load_context.onSubgroupAvcMceGetInterMinorShapeINTEL(Op::OpSubgroupAvcMceGetInterMinorShapeINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcMceGetInterDirectionsINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceGetInterDirectionsINTEL"))
          return false;
        load_context.onSubgroupAvcMceGetInterDirectionsINTEL(Op::OpSubgroupAvcMceGetInterDirectionsINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcMceGetInterMotionVectorCountINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceGetInterMotionVectorCountINTEL"))
          return false;
        load_context.onSubgroupAvcMceGetInterMotionVectorCountINTEL(Op::OpSubgroupAvcMceGetInterMotionVectorCountINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcMceGetInterReferenceIdsINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceGetInterReferenceIdsINTEL"))
          return false;
        load_context.onSubgroupAvcMceGetInterReferenceIdsINTEL(Op::OpSubgroupAvcMceGetInterReferenceIdsINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcMceGetInterReferenceInterlacedFieldPolaritiesINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcMceGetInterReferenceInterlacedFieldPolaritiesINTEL"))
          return false;
        load_context.onSubgroupAvcMceGetInterReferenceInterlacedFieldPolaritiesINTEL(
          Op::OpSubgroupAvcMceGetInterReferenceInterlacedFieldPolaritiesINTEL, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpSubgroupAvcImeInitializeINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeInitializeINTEL"))
          return false;
        load_context.onSubgroupAvcImeInitializeINTEL(Op::OpSubgroupAvcImeInitializeINTEL, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpSubgroupAvcImeSetSingleReferenceINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeSetSingleReferenceINTEL"))
          return false;
        load_context.onSubgroupAvcImeSetSingleReferenceINTEL(Op::OpSubgroupAvcImeSetSingleReferenceINTEL, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpSubgroupAvcImeSetDualReferenceINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeSetDualReferenceINTEL"))
          return false;
        load_context.onSubgroupAvcImeSetDualReferenceINTEL(Op::OpSubgroupAvcImeSetDualReferenceINTEL, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpSubgroupAvcImeRefWindowSizeINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeRefWindowSizeINTEL"))
          return false;
        load_context.onSubgroupAvcImeRefWindowSizeINTEL(Op::OpSubgroupAvcImeRefWindowSizeINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcImeAdjustRefOffsetINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeAdjustRefOffsetINTEL"))
          return false;
        load_context.onSubgroupAvcImeAdjustRefOffsetINTEL(Op::OpSubgroupAvcImeAdjustRefOffsetINTEL, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpSubgroupAvcImeConvertToMcePayloadINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeConvertToMcePayloadINTEL"))
          return false;
        load_context.onSubgroupAvcImeConvertToMcePayloadINTEL(Op::OpSubgroupAvcImeConvertToMcePayloadINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcImeSetMaxMotionVectorCountINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeSetMaxMotionVectorCountINTEL"))
          return false;
        load_context.onSubgroupAvcImeSetMaxMotionVectorCountINTEL(Op::OpSubgroupAvcImeSetMaxMotionVectorCountINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcImeSetUnidirectionalMixDisableINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeSetUnidirectionalMixDisableINTEL"))
          return false;
        load_context.onSubgroupAvcImeSetUnidirectionalMixDisableINTEL(Op::OpSubgroupAvcImeSetUnidirectionalMixDisableINTEL, c1, c0,
          c2);
        break;
      }
      case Op::OpSubgroupAvcImeSetEarlySearchTerminationThresholdINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeSetEarlySearchTerminationThresholdINTEL"))
          return false;
        load_context.onSubgroupAvcImeSetEarlySearchTerminationThresholdINTEL(
          Op::OpSubgroupAvcImeSetEarlySearchTerminationThresholdINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcImeSetWeightedSadINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeSetWeightedSadINTEL"))
          return false;
        load_context.onSubgroupAvcImeSetWeightedSadINTEL(Op::OpSubgroupAvcImeSetWeightedSadINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcImeEvaluateWithSingleReferenceINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeEvaluateWithSingleReferenceINTEL"))
          return false;
        load_context.onSubgroupAvcImeEvaluateWithSingleReferenceINTEL(Op::OpSubgroupAvcImeEvaluateWithSingleReferenceINTEL, c1, c0, c2,
          c3, c4);
        break;
      }
      case Op::OpSubgroupAvcImeEvaluateWithDualReferenceINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeEvaluateWithDualReferenceINTEL"))
          return false;
        load_context.onSubgroupAvcImeEvaluateWithDualReferenceINTEL(Op::OpSubgroupAvcImeEvaluateWithDualReferenceINTEL, c1, c0, c2, c3,
          c4, c5);
        break;
      }
      case Op::OpSubgroupAvcImeEvaluateWithSingleReferenceStreaminINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeEvaluateWithSingleReferenceStreaminINTEL"))
          return false;
        load_context.onSubgroupAvcImeEvaluateWithSingleReferenceStreaminINTEL(
          Op::OpSubgroupAvcImeEvaluateWithSingleReferenceStreaminINTEL, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpSubgroupAvcImeEvaluateWithDualReferenceStreaminINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeEvaluateWithDualReferenceStreaminINTEL"))
          return false;
        load_context.onSubgroupAvcImeEvaluateWithDualReferenceStreaminINTEL(Op::OpSubgroupAvcImeEvaluateWithDualReferenceStreaminINTEL,
          c1, c0, c2, c3, c4, c5, c6);
        break;
      }
      case Op::OpSubgroupAvcImeEvaluateWithSingleReferenceStreamoutINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeEvaluateWithSingleReferenceStreamoutINTEL"))
          return false;
        load_context.onSubgroupAvcImeEvaluateWithSingleReferenceStreamoutINTEL(
          Op::OpSubgroupAvcImeEvaluateWithSingleReferenceStreamoutINTEL, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpSubgroupAvcImeEvaluateWithDualReferenceStreamoutINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeEvaluateWithDualReferenceStreamoutINTEL"))
          return false;
        load_context.onSubgroupAvcImeEvaluateWithDualReferenceStreamoutINTEL(
          Op::OpSubgroupAvcImeEvaluateWithDualReferenceStreamoutINTEL, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpSubgroupAvcImeEvaluateWithSingleReferenceStreaminoutINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeEvaluateWithSingleReferenceStreaminoutINTEL"))
          return false;
        load_context.onSubgroupAvcImeEvaluateWithSingleReferenceStreaminoutINTEL(
          Op::OpSubgroupAvcImeEvaluateWithSingleReferenceStreaminoutINTEL, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpSubgroupAvcImeEvaluateWithDualReferenceStreaminoutINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeEvaluateWithDualReferenceStreaminoutINTEL"))
          return false;
        load_context.onSubgroupAvcImeEvaluateWithDualReferenceStreaminoutINTEL(
          Op::OpSubgroupAvcImeEvaluateWithDualReferenceStreaminoutINTEL, c1, c0, c2, c3, c4, c5, c6);
        break;
      }
      case Op::OpSubgroupAvcImeConvertToMceResultINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeConvertToMceResultINTEL"))
          return false;
        load_context.onSubgroupAvcImeConvertToMceResultINTEL(Op::OpSubgroupAvcImeConvertToMceResultINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcImeGetSingleReferenceStreaminINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeGetSingleReferenceStreaminINTEL"))
          return false;
        load_context.onSubgroupAvcImeGetSingleReferenceStreaminINTEL(Op::OpSubgroupAvcImeGetSingleReferenceStreaminINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcImeGetDualReferenceStreaminINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeGetDualReferenceStreaminINTEL"))
          return false;
        load_context.onSubgroupAvcImeGetDualReferenceStreaminINTEL(Op::OpSubgroupAvcImeGetDualReferenceStreaminINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcImeStripSingleReferenceStreamoutINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeStripSingleReferenceStreamoutINTEL"))
          return false;
        load_context.onSubgroupAvcImeStripSingleReferenceStreamoutINTEL(Op::OpSubgroupAvcImeStripSingleReferenceStreamoutINTEL, c1, c0,
          c2);
        break;
      }
      case Op::OpSubgroupAvcImeStripDualReferenceStreamoutINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeStripDualReferenceStreamoutINTEL"))
          return false;
        load_context.onSubgroupAvcImeStripDualReferenceStreamoutINTEL(Op::OpSubgroupAvcImeStripDualReferenceStreamoutINTEL, c1, c0,
          c2);
        break;
      }
      case Op::OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeMotionVectorsINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeMotionVectorsINTEL"))
          return false;
        load_context.onSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeMotionVectorsINTEL(
          Op::OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeMotionVectorsINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeDistortionsINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeDistortionsINTEL"))
          return false;
        load_context.onSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeDistortionsINTEL(
          Op::OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeDistortionsINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeReferenceIdsINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeReferenceIdsINTEL"))
          return false;
        load_context.onSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeReferenceIdsINTEL(
          Op::OpSubgroupAvcImeGetStreamoutSingleReferenceMajorShapeReferenceIdsINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeMotionVectorsINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeMotionVectorsINTEL"))
          return false;
        load_context.onSubgroupAvcImeGetStreamoutDualReferenceMajorShapeMotionVectorsINTEL(
          Op::OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeMotionVectorsINTEL, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeDistortionsINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeDistortionsINTEL"))
          return false;
        load_context.onSubgroupAvcImeGetStreamoutDualReferenceMajorShapeDistortionsINTEL(
          Op::OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeDistortionsINTEL, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeReferenceIdsINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeReferenceIdsINTEL"))
          return false;
        load_context.onSubgroupAvcImeGetStreamoutDualReferenceMajorShapeReferenceIdsINTEL(
          Op::OpSubgroupAvcImeGetStreamoutDualReferenceMajorShapeReferenceIdsINTEL, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpSubgroupAvcImeGetBorderReachedINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeGetBorderReachedINTEL"))
          return false;
        load_context.onSubgroupAvcImeGetBorderReachedINTEL(Op::OpSubgroupAvcImeGetBorderReachedINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcImeGetTruncatedSearchIndicationINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeGetTruncatedSearchIndicationINTEL"))
          return false;
        load_context.onSubgroupAvcImeGetTruncatedSearchIndicationINTEL(Op::OpSubgroupAvcImeGetTruncatedSearchIndicationINTEL, c1, c0,
          c2);
        break;
      }
      case Op::OpSubgroupAvcImeGetUnidirectionalEarlySearchTerminationINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeGetUnidirectionalEarlySearchTerminationINTEL"))
          return false;
        load_context.onSubgroupAvcImeGetUnidirectionalEarlySearchTerminationINTEL(
          Op::OpSubgroupAvcImeGetUnidirectionalEarlySearchTerminationINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcImeGetWeightingPatternMinimumMotionVectorINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeGetWeightingPatternMinimumMotionVectorINTEL"))
          return false;
        load_context.onSubgroupAvcImeGetWeightingPatternMinimumMotionVectorINTEL(
          Op::OpSubgroupAvcImeGetWeightingPatternMinimumMotionVectorINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcImeGetWeightingPatternMinimumDistortionINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcImeGetWeightingPatternMinimumDistortionINTEL"))
          return false;
        load_context.onSubgroupAvcImeGetWeightingPatternMinimumDistortionINTEL(
          Op::OpSubgroupAvcImeGetWeightingPatternMinimumDistortionINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcFmeInitializeINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        auto c8 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcFmeInitializeINTEL"))
          return false;
        load_context.onSubgroupAvcFmeInitializeINTEL(Op::OpSubgroupAvcFmeInitializeINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8);
        break;
      }
      case Op::OpSubgroupAvcBmeInitializeINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        auto c8 = operands.read<IdRef>();
        auto c9 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcBmeInitializeINTEL"))
          return false;
        load_context.onSubgroupAvcBmeInitializeINTEL(Op::OpSubgroupAvcBmeInitializeINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8, c9);
        break;
      }
      case Op::OpSubgroupAvcRefConvertToMcePayloadINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcRefConvertToMcePayloadINTEL"))
          return false;
        load_context.onSubgroupAvcRefConvertToMcePayloadINTEL(Op::OpSubgroupAvcRefConvertToMcePayloadINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcRefSetBidirectionalMixDisableINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcRefSetBidirectionalMixDisableINTEL"))
          return false;
        load_context.onSubgroupAvcRefSetBidirectionalMixDisableINTEL(Op::OpSubgroupAvcRefSetBidirectionalMixDisableINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcRefSetBilinearFilterEnableINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcRefSetBilinearFilterEnableINTEL"))
          return false;
        load_context.onSubgroupAvcRefSetBilinearFilterEnableINTEL(Op::OpSubgroupAvcRefSetBilinearFilterEnableINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcRefEvaluateWithSingleReferenceINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcRefEvaluateWithSingleReferenceINTEL"))
          return false;
        load_context.onSubgroupAvcRefEvaluateWithSingleReferenceINTEL(Op::OpSubgroupAvcRefEvaluateWithSingleReferenceINTEL, c1, c0, c2,
          c3, c4);
        break;
      }
      case Op::OpSubgroupAvcRefEvaluateWithDualReferenceINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcRefEvaluateWithDualReferenceINTEL"))
          return false;
        load_context.onSubgroupAvcRefEvaluateWithDualReferenceINTEL(Op::OpSubgroupAvcRefEvaluateWithDualReferenceINTEL, c1, c0, c2, c3,
          c4, c5);
        break;
      }
      case Op::OpSubgroupAvcRefEvaluateWithMultiReferenceINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcRefEvaluateWithMultiReferenceINTEL"))
          return false;
        load_context.onSubgroupAvcRefEvaluateWithMultiReferenceINTEL(Op::OpSubgroupAvcRefEvaluateWithMultiReferenceINTEL, c1, c0, c2,
          c3, c4);
        break;
      }
      case Op::OpSubgroupAvcRefEvaluateWithMultiReferenceInterlacedINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcRefEvaluateWithMultiReferenceInterlacedINTEL"))
          return false;
        load_context.onSubgroupAvcRefEvaluateWithMultiReferenceInterlacedINTEL(
          Op::OpSubgroupAvcRefEvaluateWithMultiReferenceInterlacedINTEL, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpSubgroupAvcRefConvertToMceResultINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcRefConvertToMceResultINTEL"))
          return false;
        load_context.onSubgroupAvcRefConvertToMceResultINTEL(Op::OpSubgroupAvcRefConvertToMceResultINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcSicInitializeINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicInitializeINTEL"))
          return false;
        load_context.onSubgroupAvcSicInitializeINTEL(Op::OpSubgroupAvcSicInitializeINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcSicConfigureSkcINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicConfigureSkcINTEL"))
          return false;
        load_context.onSubgroupAvcSicConfigureSkcINTEL(Op::OpSubgroupAvcSicConfigureSkcINTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpSubgroupAvcSicConfigureIpeLumaINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        auto c8 = operands.read<IdRef>();
        auto c9 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicConfigureIpeLumaINTEL"))
          return false;
        load_context.onSubgroupAvcSicConfigureIpeLumaINTEL(Op::OpSubgroupAvcSicConfigureIpeLumaINTEL, c1, c0, c2, c3, c4, c5, c6, c7,
          c8, c9);
        break;
      }
      case Op::OpSubgroupAvcSicConfigureIpeLumaChromaINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        auto c6 = operands.read<IdRef>();
        auto c7 = operands.read<IdRef>();
        auto c8 = operands.read<IdRef>();
        auto c9 = operands.read<IdRef>();
        auto c10 = operands.read<IdRef>();
        auto c11 = operands.read<IdRef>();
        auto c12 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicConfigureIpeLumaChromaINTEL"))
          return false;
        load_context.onSubgroupAvcSicConfigureIpeLumaChromaINTEL(Op::OpSubgroupAvcSicConfigureIpeLumaChromaINTEL, c1, c0, c2, c3, c4,
          c5, c6, c7, c8, c9, c10, c11, c12);
        break;
      }
      case Op::OpSubgroupAvcSicGetMotionVectorMaskINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicGetMotionVectorMaskINTEL"))
          return false;
        load_context.onSubgroupAvcSicGetMotionVectorMaskINTEL(Op::OpSubgroupAvcSicGetMotionVectorMaskINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcSicConvertToMcePayloadINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicConvertToMcePayloadINTEL"))
          return false;
        load_context.onSubgroupAvcSicConvertToMcePayloadINTEL(Op::OpSubgroupAvcSicConvertToMcePayloadINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcSicSetIntraLumaShapePenaltyINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicSetIntraLumaShapePenaltyINTEL"))
          return false;
        load_context.onSubgroupAvcSicSetIntraLumaShapePenaltyINTEL(Op::OpSubgroupAvcSicSetIntraLumaShapePenaltyINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcSicSetIntraLumaModeCostFunctionINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicSetIntraLumaModeCostFunctionINTEL"))
          return false;
        load_context.onSubgroupAvcSicSetIntraLumaModeCostFunctionINTEL(Op::OpSubgroupAvcSicSetIntraLumaModeCostFunctionINTEL, c1, c0,
          c2, c3, c4, c5);
        break;
      }
      case Op::OpSubgroupAvcSicSetIntraChromaModeCostFunctionINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicSetIntraChromaModeCostFunctionINTEL"))
          return false;
        load_context.onSubgroupAvcSicSetIntraChromaModeCostFunctionINTEL(Op::OpSubgroupAvcSicSetIntraChromaModeCostFunctionINTEL, c1,
          c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcSicSetBilinearFilterEnableINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicSetBilinearFilterEnableINTEL"))
          return false;
        load_context.onSubgroupAvcSicSetBilinearFilterEnableINTEL(Op::OpSubgroupAvcSicSetBilinearFilterEnableINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcSicSetSkcForwardTransformEnableINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicSetSkcForwardTransformEnableINTEL"))
          return false;
        load_context.onSubgroupAvcSicSetSkcForwardTransformEnableINTEL(Op::OpSubgroupAvcSicSetSkcForwardTransformEnableINTEL, c1, c0,
          c2, c3);
        break;
      }
      case Op::OpSubgroupAvcSicSetBlockBasedRawSkipSadINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicSetBlockBasedRawSkipSadINTEL"))
          return false;
        load_context.onSubgroupAvcSicSetBlockBasedRawSkipSadINTEL(Op::OpSubgroupAvcSicSetBlockBasedRawSkipSadINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcSicEvaluateIpeINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicEvaluateIpeINTEL"))
          return false;
        load_context.onSubgroupAvcSicEvaluateIpeINTEL(Op::OpSubgroupAvcSicEvaluateIpeINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpSubgroupAvcSicEvaluateWithSingleReferenceINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicEvaluateWithSingleReferenceINTEL"))
          return false;
        load_context.onSubgroupAvcSicEvaluateWithSingleReferenceINTEL(Op::OpSubgroupAvcSicEvaluateWithSingleReferenceINTEL, c1, c0, c2,
          c3, c4);
        break;
      }
      case Op::OpSubgroupAvcSicEvaluateWithDualReferenceINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicEvaluateWithDualReferenceINTEL"))
          return false;
        load_context.onSubgroupAvcSicEvaluateWithDualReferenceINTEL(Op::OpSubgroupAvcSicEvaluateWithDualReferenceINTEL, c1, c0, c2, c3,
          c4, c5);
        break;
      }
      case Op::OpSubgroupAvcSicEvaluateWithMultiReferenceINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicEvaluateWithMultiReferenceINTEL"))
          return false;
        load_context.onSubgroupAvcSicEvaluateWithMultiReferenceINTEL(Op::OpSubgroupAvcSicEvaluateWithMultiReferenceINTEL, c1, c0, c2,
          c3, c4);
        break;
      }
      case Op::OpSubgroupAvcSicEvaluateWithMultiReferenceInterlacedINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicEvaluateWithMultiReferenceInterlacedINTEL"))
          return false;
        load_context.onSubgroupAvcSicEvaluateWithMultiReferenceInterlacedINTEL(
          Op::OpSubgroupAvcSicEvaluateWithMultiReferenceInterlacedINTEL, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpSubgroupAvcSicConvertToMceResultINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicConvertToMceResultINTEL"))
          return false;
        load_context.onSubgroupAvcSicConvertToMceResultINTEL(Op::OpSubgroupAvcSicConvertToMceResultINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcSicGetIpeLumaShapeINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicGetIpeLumaShapeINTEL"))
          return false;
        load_context.onSubgroupAvcSicGetIpeLumaShapeINTEL(Op::OpSubgroupAvcSicGetIpeLumaShapeINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcSicGetBestIpeLumaDistortionINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicGetBestIpeLumaDistortionINTEL"))
          return false;
        load_context.onSubgroupAvcSicGetBestIpeLumaDistortionINTEL(Op::OpSubgroupAvcSicGetBestIpeLumaDistortionINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcSicGetBestIpeChromaDistortionINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicGetBestIpeChromaDistortionINTEL"))
          return false;
        load_context.onSubgroupAvcSicGetBestIpeChromaDistortionINTEL(Op::OpSubgroupAvcSicGetBestIpeChromaDistortionINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcSicGetPackedIpeLumaModesINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicGetPackedIpeLumaModesINTEL"))
          return false;
        load_context.onSubgroupAvcSicGetPackedIpeLumaModesINTEL(Op::OpSubgroupAvcSicGetPackedIpeLumaModesINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcSicGetIpeChromaModeINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicGetIpeChromaModeINTEL"))
          return false;
        load_context.onSubgroupAvcSicGetIpeChromaModeINTEL(Op::OpSubgroupAvcSicGetIpeChromaModeINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSubgroupAvcSicGetPackedSkcLumaCountThresholdINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicGetPackedSkcLumaCountThresholdINTEL"))
          return false;
        load_context.onSubgroupAvcSicGetPackedSkcLumaCountThresholdINTEL(Op::OpSubgroupAvcSicGetPackedSkcLumaCountThresholdINTEL, c1,
          c0, c2);
        break;
      }
      case Op::OpSubgroupAvcSicGetPackedSkcLumaSumThresholdINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicGetPackedSkcLumaSumThresholdINTEL"))
          return false;
        load_context.onSubgroupAvcSicGetPackedSkcLumaSumThresholdINTEL(Op::OpSubgroupAvcSicGetPackedSkcLumaSumThresholdINTEL, c1, c0,
          c2);
        break;
      }
      case Op::OpSubgroupAvcSicGetInterRawSadsINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpSubgroupAvcSicGetInterRawSadsINTEL"))
          return false;
        load_context.onSubgroupAvcSicGetInterRawSadsINTEL(Op::OpSubgroupAvcSicGetInterRawSadsINTEL, c1, c0, c2);
        break;
      }
      case Op::OpVariableLengthArrayINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpVariableLengthArrayINTEL"))
          return false;
        load_context.onVariableLengthArrayINTEL(Op::OpVariableLengthArrayINTEL, c1, c0, c2);
        break;
      }
      case Op::OpSaveMemoryINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        if (!operands.finishOperands(on_error, "OpSaveMemoryINTEL"))
          return false;
        load_context.onSaveMemoryINTEL(Op::OpSaveMemoryINTEL, c1, c0);
        break;
      }
      case Op::OpRestoreMemoryINTEL:
      {
        auto c0 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRestoreMemoryINTEL"))
          return false;
        load_context.onRestoreMemoryINTEL(Op::OpRestoreMemoryINTEL, c0);
        break;
      }
      case Op::OpArbitraryFloatSinCosPiINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        auto c8 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatSinCosPiINTEL"))
          return false;
        load_context.onArbitraryFloatSinCosPiINTEL(Op::OpArbitraryFloatSinCosPiINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8);
        break;
      }
      case Op::OpArbitraryFloatCastINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatCastINTEL"))
          return false;
        load_context.onArbitraryFloatCastINTEL(Op::OpArbitraryFloatCastINTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatCastFromIntINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatCastFromIntINTEL"))
          return false;
        load_context.onArbitraryFloatCastFromIntINTEL(Op::OpArbitraryFloatCastFromIntINTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatCastToIntINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatCastToIntINTEL"))
          return false;
        load_context.onArbitraryFloatCastToIntINTEL(Op::OpArbitraryFloatCastToIntINTEL, c1, c0, c2, c3, c4, c5, c6);
        break;
      }
      case Op::OpArbitraryFloatAddINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        auto c8 = operands.read<LiteralInteger>();
        auto c9 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatAddINTEL"))
          return false;
        load_context.onArbitraryFloatAddINTEL(Op::OpArbitraryFloatAddINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8, c9);
        break;
      }
      case Op::OpArbitraryFloatSubINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        auto c8 = operands.read<LiteralInteger>();
        auto c9 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatSubINTEL"))
          return false;
        load_context.onArbitraryFloatSubINTEL(Op::OpArbitraryFloatSubINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8, c9);
        break;
      }
      case Op::OpArbitraryFloatMulINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        auto c8 = operands.read<LiteralInteger>();
        auto c9 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatMulINTEL"))
          return false;
        load_context.onArbitraryFloatMulINTEL(Op::OpArbitraryFloatMulINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8, c9);
        break;
      }
      case Op::OpArbitraryFloatDivINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        auto c8 = operands.read<LiteralInteger>();
        auto c9 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatDivINTEL"))
          return false;
        load_context.onArbitraryFloatDivINTEL(Op::OpArbitraryFloatDivINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8, c9);
        break;
      }
      case Op::OpArbitraryFloatGTINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatGTINTEL"))
          return false;
        load_context.onArbitraryFloatGTINTEL(Op::OpArbitraryFloatGTINTEL, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpArbitraryFloatGEINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatGEINTEL"))
          return false;
        load_context.onArbitraryFloatGEINTEL(Op::OpArbitraryFloatGEINTEL, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpArbitraryFloatLTINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatLTINTEL"))
          return false;
        load_context.onArbitraryFloatLTINTEL(Op::OpArbitraryFloatLTINTEL, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpArbitraryFloatLEINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatLEINTEL"))
          return false;
        load_context.onArbitraryFloatLEINTEL(Op::OpArbitraryFloatLEINTEL, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpArbitraryFloatEQINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatEQINTEL"))
          return false;
        load_context.onArbitraryFloatEQINTEL(Op::OpArbitraryFloatEQINTEL, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpArbitraryFloatRecipINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatRecipINTEL"))
          return false;
        load_context.onArbitraryFloatRecipINTEL(Op::OpArbitraryFloatRecipINTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatRSqrtINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatRSqrtINTEL"))
          return false;
        load_context.onArbitraryFloatRSqrtINTEL(Op::OpArbitraryFloatRSqrtINTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatCbrtINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatCbrtINTEL"))
          return false;
        load_context.onArbitraryFloatCbrtINTEL(Op::OpArbitraryFloatCbrtINTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatHypotINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        auto c8 = operands.read<LiteralInteger>();
        auto c9 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatHypotINTEL"))
          return false;
        load_context.onArbitraryFloatHypotINTEL(Op::OpArbitraryFloatHypotINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8, c9);
        break;
      }
      case Op::OpArbitraryFloatSqrtINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatSqrtINTEL"))
          return false;
        load_context.onArbitraryFloatSqrtINTEL(Op::OpArbitraryFloatSqrtINTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatLogINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatLogINTEL"))
          return false;
        load_context.onArbitraryFloatLogINTEL(Op::OpArbitraryFloatLogINTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatLog2INTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatLog2INTEL"))
          return false;
        load_context.onArbitraryFloatLog2INTEL(Op::OpArbitraryFloatLog2INTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatLog10INTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatLog10INTEL"))
          return false;
        load_context.onArbitraryFloatLog10INTEL(Op::OpArbitraryFloatLog10INTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatLog1pINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatLog1pINTEL"))
          return false;
        load_context.onArbitraryFloatLog1pINTEL(Op::OpArbitraryFloatLog1pINTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatExpINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatExpINTEL"))
          return false;
        load_context.onArbitraryFloatExpINTEL(Op::OpArbitraryFloatExpINTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatExp2INTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatExp2INTEL"))
          return false;
        load_context.onArbitraryFloatExp2INTEL(Op::OpArbitraryFloatExp2INTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatExp10INTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatExp10INTEL"))
          return false;
        load_context.onArbitraryFloatExp10INTEL(Op::OpArbitraryFloatExp10INTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatExpm1INTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatExpm1INTEL"))
          return false;
        load_context.onArbitraryFloatExpm1INTEL(Op::OpArbitraryFloatExpm1INTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatSinINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatSinINTEL"))
          return false;
        load_context.onArbitraryFloatSinINTEL(Op::OpArbitraryFloatSinINTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatCosINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatCosINTEL"))
          return false;
        load_context.onArbitraryFloatCosINTEL(Op::OpArbitraryFloatCosINTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatSinCosINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatSinCosINTEL"))
          return false;
        load_context.onArbitraryFloatSinCosINTEL(Op::OpArbitraryFloatSinCosINTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatSinPiINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatSinPiINTEL"))
          return false;
        load_context.onArbitraryFloatSinPiINTEL(Op::OpArbitraryFloatSinPiINTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatCosPiINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatCosPiINTEL"))
          return false;
        load_context.onArbitraryFloatCosPiINTEL(Op::OpArbitraryFloatCosPiINTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatASinINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatASinINTEL"))
          return false;
        load_context.onArbitraryFloatASinINTEL(Op::OpArbitraryFloatASinINTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatASinPiINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatASinPiINTEL"))
          return false;
        load_context.onArbitraryFloatASinPiINTEL(Op::OpArbitraryFloatASinPiINTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatACosINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatACosINTEL"))
          return false;
        load_context.onArbitraryFloatACosINTEL(Op::OpArbitraryFloatACosINTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatACosPiINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatACosPiINTEL"))
          return false;
        load_context.onArbitraryFloatACosPiINTEL(Op::OpArbitraryFloatACosPiINTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatATanINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatATanINTEL"))
          return false;
        load_context.onArbitraryFloatATanINTEL(Op::OpArbitraryFloatATanINTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatATanPiINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatATanPiINTEL"))
          return false;
        load_context.onArbitraryFloatATanPiINTEL(Op::OpArbitraryFloatATanPiINTEL, c1, c0, c2, c3, c4, c5, c6, c7);
        break;
      }
      case Op::OpArbitraryFloatATan2INTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        auto c8 = operands.read<LiteralInteger>();
        auto c9 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatATan2INTEL"))
          return false;
        load_context.onArbitraryFloatATan2INTEL(Op::OpArbitraryFloatATan2INTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8, c9);
        break;
      }
      case Op::OpArbitraryFloatPowINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        auto c8 = operands.read<LiteralInteger>();
        auto c9 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatPowINTEL"))
          return false;
        load_context.onArbitraryFloatPowINTEL(Op::OpArbitraryFloatPowINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8, c9);
        break;
      }
      case Op::OpArbitraryFloatPowRINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        auto c8 = operands.read<LiteralInteger>();
        auto c9 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatPowRINTEL"))
          return false;
        load_context.onArbitraryFloatPowRINTEL(Op::OpArbitraryFloatPowRINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8, c9);
        break;
      }
      case Op::OpArbitraryFloatPowNINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        auto c8 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpArbitraryFloatPowNINTEL"))
          return false;
        load_context.onArbitraryFloatPowNINTEL(Op::OpArbitraryFloatPowNINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8);
        break;
      }
      case Op::OpLoopControlINTEL:
      {
        auto c0 = operands.read<Multiple<LiteralInteger>>();
        if (!operands.finishOperands(on_error, "OpLoopControlINTEL"))
          return false;
        load_context.onLoopControlINTEL(Op::OpLoopControlINTEL, c0);
        break;
      }
      case Op::OpAliasDomainDeclINTEL:
      {
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpAliasDomainDeclINTEL"))
          return false;
        load_context.onAliasDomainDeclINTEL(Op::OpAliasDomainDeclINTEL, c0, c1);
        break;
      }
      case Op::OpAliasScopeDeclINTEL:
      {
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<Optional<IdRef>>();
        if (!operands.finishOperands(on_error, "OpAliasScopeDeclINTEL"))
          return false;
        load_context.onAliasScopeDeclINTEL(Op::OpAliasScopeDeclINTEL, c0, c1, c2);
        break;
      }
      case Op::OpAliasScopeListDeclINTEL:
      {
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<Multiple<IdRef>>();
        if (!operands.finishOperands(on_error, "OpAliasScopeListDeclINTEL"))
          return false;
        load_context.onAliasScopeListDeclINTEL(Op::OpAliasScopeListDeclINTEL, c0, c1);
        break;
      }
      case Op::OpFixedSqrtINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        auto c8 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpFixedSqrtINTEL"))
          return false;
        load_context.onFixedSqrtINTEL(Op::OpFixedSqrtINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8);
        break;
      }
      case Op::OpFixedRecipINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        auto c8 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpFixedRecipINTEL"))
          return false;
        load_context.onFixedRecipINTEL(Op::OpFixedRecipINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8);
        break;
      }
      case Op::OpFixedRsqrtINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        auto c8 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpFixedRsqrtINTEL"))
          return false;
        load_context.onFixedRsqrtINTEL(Op::OpFixedRsqrtINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8);
        break;
      }
      case Op::OpFixedSinINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        auto c8 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpFixedSinINTEL"))
          return false;
        load_context.onFixedSinINTEL(Op::OpFixedSinINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8);
        break;
      }
      case Op::OpFixedCosINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        auto c8 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpFixedCosINTEL"))
          return false;
        load_context.onFixedCosINTEL(Op::OpFixedCosINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8);
        break;
      }
      case Op::OpFixedSinCosINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        auto c8 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpFixedSinCosINTEL"))
          return false;
        load_context.onFixedSinCosINTEL(Op::OpFixedSinCosINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8);
        break;
      }
      case Op::OpFixedSinPiINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        auto c8 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpFixedSinPiINTEL"))
          return false;
        load_context.onFixedSinPiINTEL(Op::OpFixedSinPiINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8);
        break;
      }
      case Op::OpFixedCosPiINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        auto c8 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpFixedCosPiINTEL"))
          return false;
        load_context.onFixedCosPiINTEL(Op::OpFixedCosPiINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8);
        break;
      }
      case Op::OpFixedSinCosPiINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        auto c8 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpFixedSinCosPiINTEL"))
          return false;
        load_context.onFixedSinCosPiINTEL(Op::OpFixedSinCosPiINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8);
        break;
      }
      case Op::OpFixedLogINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        auto c8 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpFixedLogINTEL"))
          return false;
        load_context.onFixedLogINTEL(Op::OpFixedLogINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8);
        break;
      }
      case Op::OpFixedExpINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        auto c4 = operands.read<LiteralInteger>();
        auto c5 = operands.read<LiteralInteger>();
        auto c6 = operands.read<LiteralInteger>();
        auto c7 = operands.read<LiteralInteger>();
        auto c8 = operands.read<LiteralInteger>();
        if (!operands.finishOperands(on_error, "OpFixedExpINTEL"))
          return false;
        load_context.onFixedExpINTEL(Op::OpFixedExpINTEL, c1, c0, c2, c3, c4, c5, c6, c7, c8);
        break;
      }
      case Op::OpPtrCastToCrossWorkgroupINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpPtrCastToCrossWorkgroupINTEL"))
          return false;
        load_context.onPtrCastToCrossWorkgroupINTEL(Op::OpPtrCastToCrossWorkgroupINTEL, c1, c0, c2);
        break;
      }
      case Op::OpCrossWorkgroupCastToPtrINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpCrossWorkgroupCastToPtrINTEL"))
          return false;
        load_context.onCrossWorkgroupCastToPtrINTEL(Op::OpCrossWorkgroupCastToPtrINTEL, c1, c0, c2);
        break;
      }
      case Op::OpReadPipeBlockingINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpReadPipeBlockingINTEL"))
          return false;
        load_context.onReadPipeBlockingINTEL(Op::OpReadPipeBlockingINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpWritePipeBlockingINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpWritePipeBlockingINTEL"))
          return false;
        load_context.onWritePipeBlockingINTEL(Op::OpWritePipeBlockingINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpFPGARegINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpFPGARegINTEL"))
          return false;
        load_context.onFPGARegINTEL(Op::OpFPGARegINTEL, c1, c0, c2, c3);
        break;
      }
      case Op::OpRayQueryGetRayTMinKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryGetRayTMinKHR"))
          return false;
        load_context.onRayQueryGetRayTMinKHR(Op::OpRayQueryGetRayTMinKHR, c1, c0, c2);
        break;
      }
      case Op::OpRayQueryGetRayFlagsKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryGetRayFlagsKHR"))
          return false;
        load_context.onRayQueryGetRayFlagsKHR(Op::OpRayQueryGetRayFlagsKHR, c1, c0, c2);
        break;
      }
      case Op::OpRayQueryGetIntersectionTKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryGetIntersectionTKHR"))
          return false;
        load_context.onRayQueryGetIntersectionTKHR(Op::OpRayQueryGetIntersectionTKHR, c1, c0, c2, c3);
        break;
      }
      case Op::OpRayQueryGetIntersectionInstanceCustomIndexKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryGetIntersectionInstanceCustomIndexKHR"))
          return false;
        load_context.onRayQueryGetIntersectionInstanceCustomIndexKHR(Op::OpRayQueryGetIntersectionInstanceCustomIndexKHR, c1, c0, c2,
          c3);
        break;
      }
      case Op::OpRayQueryGetIntersectionInstanceIdKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryGetIntersectionInstanceIdKHR"))
          return false;
        load_context.onRayQueryGetIntersectionInstanceIdKHR(Op::OpRayQueryGetIntersectionInstanceIdKHR, c1, c0, c2, c3);
        break;
      }
      case Op::OpRayQueryGetIntersectionInstanceShaderBindingTableRecordOffsetKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryGetIntersectionInstanceShaderBindingTableRecordOffsetKHR"))
          return false;
        load_context.onRayQueryGetIntersectionInstanceShaderBindingTableRecordOffsetKHR(
          Op::OpRayQueryGetIntersectionInstanceShaderBindingTableRecordOffsetKHR, c1, c0, c2, c3);
        break;
      }
      case Op::OpRayQueryGetIntersectionGeometryIndexKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryGetIntersectionGeometryIndexKHR"))
          return false;
        load_context.onRayQueryGetIntersectionGeometryIndexKHR(Op::OpRayQueryGetIntersectionGeometryIndexKHR, c1, c0, c2, c3);
        break;
      }
      case Op::OpRayQueryGetIntersectionPrimitiveIndexKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryGetIntersectionPrimitiveIndexKHR"))
          return false;
        load_context.onRayQueryGetIntersectionPrimitiveIndexKHR(Op::OpRayQueryGetIntersectionPrimitiveIndexKHR, c1, c0, c2, c3);
        break;
      }
      case Op::OpRayQueryGetIntersectionBarycentricsKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryGetIntersectionBarycentricsKHR"))
          return false;
        load_context.onRayQueryGetIntersectionBarycentricsKHR(Op::OpRayQueryGetIntersectionBarycentricsKHR, c1, c0, c2, c3);
        break;
      }
      case Op::OpRayQueryGetIntersectionFrontFaceKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryGetIntersectionFrontFaceKHR"))
          return false;
        load_context.onRayQueryGetIntersectionFrontFaceKHR(Op::OpRayQueryGetIntersectionFrontFaceKHR, c1, c0, c2, c3);
        break;
      }
      case Op::OpRayQueryGetIntersectionCandidateAABBOpaqueKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryGetIntersectionCandidateAABBOpaqueKHR"))
          return false;
        load_context.onRayQueryGetIntersectionCandidateAABBOpaqueKHR(Op::OpRayQueryGetIntersectionCandidateAABBOpaqueKHR, c1, c0, c2);
        break;
      }
      case Op::OpRayQueryGetIntersectionObjectRayDirectionKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryGetIntersectionObjectRayDirectionKHR"))
          return false;
        load_context.onRayQueryGetIntersectionObjectRayDirectionKHR(Op::OpRayQueryGetIntersectionObjectRayDirectionKHR, c1, c0, c2,
          c3);
        break;
      }
      case Op::OpRayQueryGetIntersectionObjectRayOriginKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryGetIntersectionObjectRayOriginKHR"))
          return false;
        load_context.onRayQueryGetIntersectionObjectRayOriginKHR(Op::OpRayQueryGetIntersectionObjectRayOriginKHR, c1, c0, c2, c3);
        break;
      }
      case Op::OpRayQueryGetWorldRayDirectionKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryGetWorldRayDirectionKHR"))
          return false;
        load_context.onRayQueryGetWorldRayDirectionKHR(Op::OpRayQueryGetWorldRayDirectionKHR, c1, c0, c2);
        break;
      }
      case Op::OpRayQueryGetWorldRayOriginKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryGetWorldRayOriginKHR"))
          return false;
        load_context.onRayQueryGetWorldRayOriginKHR(Op::OpRayQueryGetWorldRayOriginKHR, c1, c0, c2);
        break;
      }
      case Op::OpRayQueryGetIntersectionObjectToWorldKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryGetIntersectionObjectToWorldKHR"))
          return false;
        load_context.onRayQueryGetIntersectionObjectToWorldKHR(Op::OpRayQueryGetIntersectionObjectToWorldKHR, c1, c0, c2, c3);
        break;
      }
      case Op::OpRayQueryGetIntersectionWorldToObjectKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpRayQueryGetIntersectionWorldToObjectKHR"))
          return false;
        load_context.onRayQueryGetIntersectionWorldToObjectKHR(Op::OpRayQueryGetIntersectionWorldToObjectKHR, c1, c0, c2, c3);
        break;
      }
      case Op::OpAtomicFAddEXT:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<IdScope>();
        auto c4 = operands.read<IdMemorySemantics>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpAtomicFAddEXT"))
          return false;
        load_context.onAtomicFAddEXT(Op::OpAtomicFAddEXT, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpTypeBufferSurfaceINTEL:
      {
        auto c0 = operands.read<IdResult>();
        auto c1 = operands.read<AccessQualifier>();
        if (!operands.finishOperands(on_error, "OpTypeBufferSurfaceINTEL"))
          return false;
        load_context.onTypeBufferSurfaceINTEL(Op::OpTypeBufferSurfaceINTEL, c0, c1);
        break;
      }
      case Op::OpTypeStructContinuedINTEL:
      {
        auto c0 = operands.read<Multiple<IdRef>>();
        if (!operands.finishOperands(on_error, "OpTypeStructContinuedINTEL"))
          return false;
        load_context.onTypeStructContinuedINTEL(Op::OpTypeStructContinuedINTEL, c0);
        break;
      }
      case Op::OpConstantCompositeContinuedINTEL:
      {
        auto c0 = operands.read<Multiple<IdRef>>();
        if (!operands.finishOperands(on_error, "OpConstantCompositeContinuedINTEL"))
          return false;
        load_context.onConstantCompositeContinuedINTEL(Op::OpConstantCompositeContinuedINTEL, c0);
        break;
      }
      case Op::OpSpecConstantCompositeContinuedINTEL:
      {
        auto c0 = operands.read<Multiple<IdRef>>();
        if (!operands.finishOperands(on_error, "OpSpecConstantCompositeContinuedINTEL"))
          return false;
        load_context.onSpecConstantCompositeContinuedINTEL(Op::OpSpecConstantCompositeContinuedINTEL, c0);
        break;
      }
      case Op::OpCompositeConstructContinuedINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<Multiple<IdRef>>();
        if (!operands.finishOperands(on_error, "OpCompositeConstructContinuedINTEL"))
          return false;
        load_context.onCompositeConstructContinuedINTEL(Op::OpCompositeConstructContinuedINTEL, c1, c0, c2);
        break;
      }
      case Op::OpConvertFToBF16INTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpConvertFToBF16INTEL"))
          return false;
        load_context.onConvertFToBF16INTEL(Op::OpConvertFToBF16INTEL, c1, c0, c2);
        break;
      }
      case Op::OpConvertBF16ToFINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpConvertBF16ToFINTEL"))
          return false;
        load_context.onConvertBF16ToFINTEL(Op::OpConvertBF16ToFINTEL, c1, c0, c2);
        break;
      }
      case Op::OpControlBarrierArriveINTEL:
      {
        auto c0 = operands.read<IdScope>();
        auto c1 = operands.read<IdScope>();
        auto c2 = operands.read<IdMemorySemantics>();
        if (!operands.finishOperands(on_error, "OpControlBarrierArriveINTEL"))
          return false;
        load_context.onControlBarrierArriveINTEL(Op::OpControlBarrierArriveINTEL, c0, c1, c2);
        break;
      }
      case Op::OpControlBarrierWaitINTEL:
      {
        auto c0 = operands.read<IdScope>();
        auto c1 = operands.read<IdScope>();
        auto c2 = operands.read<IdMemorySemantics>();
        if (!operands.finishOperands(on_error, "OpControlBarrierWaitINTEL"))
          return false;
        load_context.onControlBarrierWaitINTEL(Op::OpControlBarrierWaitINTEL, c0, c1, c2);
        break;
      }
      case Op::OpGroupIMulKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupIMulKHR"))
          return false;
        load_context.onGroupIMulKHR(Op::OpGroupIMulKHR, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupFMulKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupFMulKHR"))
          return false;
        load_context.onGroupFMulKHR(Op::OpGroupFMulKHR, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupBitwiseAndKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupBitwiseAndKHR"))
          return false;
        load_context.onGroupBitwiseAndKHR(Op::OpGroupBitwiseAndKHR, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupBitwiseOrKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupBitwiseOrKHR"))
          return false;
        load_context.onGroupBitwiseOrKHR(Op::OpGroupBitwiseOrKHR, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupBitwiseXorKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupBitwiseXorKHR"))
          return false;
        load_context.onGroupBitwiseXorKHR(Op::OpGroupBitwiseXorKHR, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupLogicalAndKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupLogicalAndKHR"))
          return false;
        load_context.onGroupLogicalAndKHR(Op::OpGroupLogicalAndKHR, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupLogicalOrKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupLogicalOrKHR"))
          return false;
        load_context.onGroupLogicalOrKHR(Op::OpGroupLogicalOrKHR, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpGroupLogicalXorKHR:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdScope>();
        auto c3 = operands.read<GroupOperation>();
        auto c4 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpGroupLogicalXorKHR"))
          return false;
        load_context.onGroupLogicalXorKHR(Op::OpGroupLogicalXorKHR, c1, c0, c2, c3, c4);
        break;
      }
      case Op::OpMaskedGatherINTEL:
      {
        auto c0 = operands.read<IdResultType>();
        auto c1 = operands.read<IdResult>();
        auto c2 = operands.read<IdRef>();
        auto c3 = operands.read<LiteralInteger>();
        auto c4 = operands.read<IdRef>();
        auto c5 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpMaskedGatherINTEL"))
          return false;
        load_context.onMaskedGatherINTEL(Op::OpMaskedGatherINTEL, c1, c0, c2, c3, c4, c5);
        break;
      }
      case Op::OpMaskedScatterINTEL:
      {
        auto c0 = operands.read<IdRef>();
        auto c1 = operands.read<IdRef>();
        auto c2 = operands.read<LiteralInteger>();
        auto c3 = operands.read<IdRef>();
        if (!operands.finishOperands(on_error, "OpMaskedScatterINTEL"))
          return false;
        load_context.onMaskedScatterINTEL(Op::OpMaskedScatterINTEL, c0, c1, c2, c3);
        break;
      }
    }
    at += len;
  }
  return true;
}
} // namespace spirv
