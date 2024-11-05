#pragma once

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/simulate/bind_enum.h"
#include "daScript/simulate/aot.h"
#include "daScript/simulate/aot_builtin_rtti.h"

DAS_BASE_BIND_ENUM(das::Type, Type,
    none,    autoinfer,    alias,    option, typeDecl, typeMacro, fakeContext,    fakeLineInfo,
    anyArgument,    tVoid,    tBool,    tInt8,    tUInt8,    tInt16,    tUInt16,
    tInt64,    tUInt64,    tInt,    tInt2,    tInt3,    tInt4,    tUInt,
    tUInt2,    tUInt3,    tUInt4,    tFloat,    tFloat2,    tFloat3,    tFloat4,
    tDouble,    tRange,    tURange,    tRange64,    tURange64,    tString,    tStructure,
    tHandle,    tEnumeration,    tEnumeration8,    tEnumeration16,    tEnumeration64,    tBitfield,    tPointer,    tFunction,
    tLambda,    tIterator,    tArray,    tTable,    tBlock,    tTuple,    tVariant
)

DAS_BASE_BIND_ENUM(das::RefMatters,   RefMatters,   no, yes)
DAS_BASE_BIND_ENUM(das::ConstMatters, ConstMatters, no, yes)
DAS_BASE_BIND_ENUM(das::TemporaryMatters, TemporaryMatters, no, yes)

DAS_BIND_ENUM_CAST(das::CompilationError)

MAKE_EXTERNAL_TYPE_FACTORY(FileInfo, das::FileInfo)
MAKE_EXTERNAL_TYPE_FACTORY(LineInfo, das::LineInfo)
MAKE_EXTERNAL_TYPE_FACTORY(Annotation, das::Annotation)
MAKE_EXTERNAL_TYPE_FACTORY(TypeAnnotation, das::TypeAnnotation)
MAKE_EXTERNAL_TYPE_FACTORY(BasicStructureAnnotation, das::BasicStructureAnnotation)
MAKE_EXTERNAL_TYPE_FACTORY(StructInfo, das::StructInfo)
MAKE_EXTERNAL_TYPE_FACTORY(EnumInfo, das::EnumInfo)
MAKE_EXTERNAL_TYPE_FACTORY(EnumValueInfo, das::EnumValueInfo)
MAKE_EXTERNAL_TYPE_FACTORY(TypeInfo, das::TypeInfo)
MAKE_EXTERNAL_TYPE_FACTORY(VarInfo, das::VarInfo)
MAKE_EXTERNAL_TYPE_FACTORY(LocalVariableInfo, das::LocalVariableInfo)
MAKE_EXTERNAL_TYPE_FACTORY(FuncInfo, das::FuncInfo)
MAKE_EXTERNAL_TYPE_FACTORY(AnnotationArgument, das::AnnotationArgument)
MAKE_EXTERNAL_TYPE_FACTORY(AnnotationArguments, das::AnnotationArguments)
MAKE_EXTERNAL_TYPE_FACTORY(AnnotationArgumentList, das::AnnotationArgumentList)
MAKE_EXTERNAL_TYPE_FACTORY(AnnotationDeclaration, das::AnnotationDeclaration)
MAKE_EXTERNAL_TYPE_FACTORY(AnnotationList, das::AnnotationList)
MAKE_EXTERNAL_TYPE_FACTORY(Program, das::Program)
MAKE_EXTERNAL_TYPE_FACTORY(Module, das::Module)
MAKE_EXTERNAL_TYPE_FACTORY(Error,Error)
MAKE_EXTERNAL_TYPE_FACTORY(FileAccess,FileAccess)
MAKE_EXTERNAL_TYPE_FACTORY(Context,Context)
MAKE_EXTERNAL_TYPE_FACTORY(CodeOfPolicies,CodeOfPolicies)
MAKE_EXTERNAL_TYPE_FACTORY(SimFunction,SimFunction)
MAKE_EXTERNAL_TYPE_FACTORY(recursive_mutex,das::recursive_mutex)

