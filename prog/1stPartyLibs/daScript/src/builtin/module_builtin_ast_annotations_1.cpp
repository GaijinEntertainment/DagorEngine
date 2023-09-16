#include "daScript/misc/platform.h"

#include "module_builtin_rtti.h"

#include "daScript/simulate/simulate_visit_op.h"
#include "daScript/ast/ast_policy_types.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_generate.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/simulate/aot_builtin_ast.h"
#include "daScript/simulate/aot_builtin_string.h"
#include "daScript/misc/performance_time.h"

#include "module_builtin_ast.h"

using namespace das;

namespace das {
    struct AstModuleLibraryAnnotation : ManagedStructureAnnotation<ModuleLibrary,false> {
        AstModuleLibraryAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("ModuleLibrary", ml) {
        }
    };

    struct AstTypeDeclAnnnotation : ManagedStructureAnnotation <TypeDecl> {
        AstTypeDeclAnnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("TypeDecl", ml) {
        }
        void init () {
            addField<DAS_BIND_MANAGED_FIELD(baseType)>("baseType");
            addField<DAS_BIND_MANAGED_FIELD(structType)>("structType");
            addField<DAS_BIND_MANAGED_FIELD(enumType)>("enumType");
            addField<DAS_BIND_MANAGED_FIELD(annotation)>("annotation");
            addField<DAS_BIND_MANAGED_FIELD(firstType)>("firstType");
            addField<DAS_BIND_MANAGED_FIELD(secondType)>("secondType");
            addField<DAS_BIND_MANAGED_FIELD(argTypes)>("argTypes");
            addField<DAS_BIND_MANAGED_FIELD(argNames)>("argNames");
            addField<DAS_BIND_MANAGED_FIELD(dim)>("dim");
            addField<DAS_BIND_MANAGED_FIELD(dimExpr)>("dimExpr");
            addFieldEx ( "flags", "flags", offsetof(TypeDecl, flags), makeTypeDeclFlags() );
            addField<DAS_BIND_MANAGED_FIELD(alias)>("alias");
            addField<DAS_BIND_MANAGED_FIELD(at)>("at");
            addField<DAS_BIND_MANAGED_FIELD(module)>("_module", "module");
            // properties
            addProperty<bool (TypeDecl::*)() const, &TypeDecl::canAot>("canAot","canAot");
            addProperty<DAS_BIND_MANAGED_PROP(isExprType)>("isExprType","isExprType");
            addProperty<bool (TypeDecl::*)() const, &ManagedType::isSimpleType>("isSimpleType","isSimpleType");
            addProperty<DAS_BIND_MANAGED_PROP(isArray)>("isArray","isArray");
            addProperty<DAS_BIND_MANAGED_PROP(isGoodIteratorType)>("isGoodIteratorType","isGoodIteratorType");
            addProperty<DAS_BIND_MANAGED_PROP(isGoodArrayType)>("isGoodArrayType","isGoodArrayType");
            addProperty<DAS_BIND_MANAGED_PROP(isGoodTableType)>("isGoodTableType","isGoodTableType");
            addProperty<DAS_BIND_MANAGED_PROP(isGoodBlockType)>("isGoodBlockType","isGoodBlockType");
            addProperty<DAS_BIND_MANAGED_PROP(isGoodFunctionType)>("isGoodFunctionType","isGoodFunctionType");
            addProperty<DAS_BIND_MANAGED_PROP(isGoodLambdaType)>("isGoodLambdaType","isGoodLambdaType");
            addProperty<DAS_BIND_MANAGED_PROP(isGoodTupleType)>("isGoodTupleType","isGoodTupleType");
            addProperty<DAS_BIND_MANAGED_PROP(isGoodVariantType)>("isGoodVariantType","isGoodVariantType");
            addProperty<DAS_BIND_MANAGED_PROP(isVoid)>("isVoid","isVoid");
            addProperty<DAS_BIND_MANAGED_PROP(isRef)>("isRef","isRef");
            addProperty<DAS_BIND_MANAGED_PROP(isRefType)>("isRefType","isRefType");
            addProperty<DAS_BIND_MANAGED_PROP(canWrite)>("canWrite","canWrite");
            addProperty<DAS_BIND_MANAGED_PROP(isAotAlias)>("isAotAlias","isAotAlias");
            addProperty<bool (TypeDecl::*)() const, &ManagedType::isShareable>("isShareable", "isShareable");
            addProperty<DAS_BIND_MANAGED_PROP(isIndex)>("isIndex","isIndex");
            addProperty<DAS_BIND_MANAGED_PROP(isBool)>("isBool","isBool");
            addProperty<DAS_BIND_MANAGED_PROP(isInteger)>("isInteger","isInteger");
            addProperty<DAS_BIND_MANAGED_PROP(isSignedInteger)>("isSignedInteger","isSignedInteger");
            addProperty<DAS_BIND_MANAGED_PROP(isUnsignedInteger)>("isUnsignedInteger","isUnsignedInteger");
            addProperty<DAS_BIND_MANAGED_PROP(isSignedIntegerOrIntVec)>("isSignedIntegerOrIntVec","isSignedIntegerOrIntVec");
            addProperty<DAS_BIND_MANAGED_PROP(isUnsignedIntegerOrIntVec)>("isUnsignedIntegerOrIntVec","isUnsignedIntegerOrIntVec");
            addProperty<DAS_BIND_MANAGED_PROP(isFloatOrDouble)>("isFloatOrDouble","isFloatOrDouble");
            addProperty<DAS_BIND_MANAGED_PROP(isNumeric)>("isNumeric","isNumeric");
            addProperty<DAS_BIND_MANAGED_PROP(isNumericComparable)>("isNumericComparable","isNumericComparable");
            addProperty<DAS_BIND_MANAGED_PROP(isPointer)>("isPointer","isPointer");
            addProperty<DAS_BIND_MANAGED_PROP(isVoidPointer)>("isVoidPointer","isVoidPointer");
            addProperty<DAS_BIND_MANAGED_PROP(isIterator)>("isIterator","isIterator");
            addProperty<DAS_BIND_MANAGED_PROP(isEnum)>("isEnum","isEnum");
            addProperty<DAS_BIND_MANAGED_PROP(isEnumT)>("isEnumT","isEnumT");
            addProperty<DAS_BIND_MANAGED_PROP(isHandle)>("isHandle","isHandle");
            addProperty<DAS_BIND_MANAGED_PROP(isStructure)>("isStructure","isStructure");
            addProperty<DAS_BIND_MANAGED_PROP(isClass)>("isClass","isClass");
            addProperty<DAS_BIND_MANAGED_PROP(isFunction)>("isFunction","isFunction");
            addProperty<DAS_BIND_MANAGED_PROP(isTuple)>("isTuple","isTuple");
            addProperty<DAS_BIND_MANAGED_PROP(isVariant)>("isVariant","isVariant");
            addProperty<DAS_BIND_MANAGED_PROP(getSizeOf)>("sizeOf","getSizeOf");
            addProperty<DAS_BIND_MANAGED_PROP(getCountOf)>("countOf","getCountOf");
            addProperty<DAS_BIND_MANAGED_PROP(getAlignOf)>("alignOf","getAlignOf");
            addProperty<DAS_BIND_MANAGED_PROP(getBaseSizeOf)>("baseSizeOf","getBaseSizeOf");
            addProperty<DAS_BIND_MANAGED_PROP(getStride)>("stride","getStride");
            addProperty<DAS_BIND_MANAGED_PROP(getTupleSize)>("tupleSize","getTupleSize");
            addProperty<DAS_BIND_MANAGED_PROP(getTupleAlign)>("tupleAlign","getTupleAlign");
            addProperty<DAS_BIND_MANAGED_PROP(getVariantSize)>("variantSize","getVariantSize");
            addProperty<DAS_BIND_MANAGED_PROP(getVariantAlign)>("variantAlign","getVariantAlign");
            addProperty<bool (TypeDecl::*)() const, &ManagedType::canCopy>("canCopy","canCopy");
            addProperty<DAS_BIND_MANAGED_PROP(canMove)>("canMove","canMove");
            addProperty<DAS_BIND_MANAGED_PROP(canClone)>("canClone","canClone");
            addProperty<DAS_BIND_MANAGED_PROP(canNew)>("canNew","canNew");
            addProperty<DAS_BIND_MANAGED_PROP(canDeletePtr)>("canDeletePtr","canDeletePtr");
            addProperty<DAS_BIND_MANAGED_PROP(canDelete)>("canDelete","canDelete");
            addProperty<DAS_BIND_MANAGED_PROP(needDelete)>("needDelete","needDelete");
            addProperty<DAS_BIND_MANAGED_PROP(isPod)>("isPod","isPod");
            addProperty<DAS_BIND_MANAGED_PROP(isRawPod)>("isRawPod","isRawPod");
            addProperty<DAS_BIND_MANAGED_PROP(isNoHeapType)>("isNoHeapType","isNoHeapType");
            addProperty<DAS_BIND_MANAGED_PROP(isWorkhorseType)>("isWorkhorseType","isWorkhorseType");
            addProperty<DAS_BIND_MANAGED_PROP(isPolicyType)>("isPolicyType","isPolicyType");
            addProperty<DAS_BIND_MANAGED_PROP(isVecPolicyType)>("isVecPolicyType","isVecPolicyType");
            addProperty<DAS_BIND_MANAGED_PROP(isReturnType)>("isReturnType","isReturnType");
            addProperty<DAS_BIND_MANAGED_PROP(isCtorType)>("isCtorType","isCtorType");
            addProperty<DAS_BIND_MANAGED_PROP(isRange)>("isRange","isRange");
            addProperty<DAS_BIND_MANAGED_PROP(isString)>("isString","isString");
            addProperty<DAS_BIND_MANAGED_PROP(isConst)>("isConst","isConst");
            addProperty<DAS_BIND_MANAGED_PROP(isFoldable)>("isFoldable","isFoldable");
            addProperty<DAS_BIND_MANAGED_PROP(isAlias)>("isAlias","isAlias");
            addProperty<DAS_BIND_MANAGED_PROP(isAutoArrayResolved)>("isAutoArrayResolved","isAutoArrayResolved");
            addProperty<DAS_BIND_MANAGED_PROP(isAuto)>("isAuto","isAuto");
            addProperty<DAS_BIND_MANAGED_PROP(isAutoOrAlias)>("isAutoOrAlias","isAutoOrAlias");
            addProperty<DAS_BIND_MANAGED_PROP(isVectorType)>("isVectorType","isVectorType");
            addProperty<DAS_BIND_MANAGED_PROP(isBitfield)>("isBitfield","isBitfield");
            addProperty<bool (TypeDecl::*)() const, &ManagedType::isLocal>("isLocal", "isLocal");
            addProperty<bool (TypeDecl::*)() const, &ManagedType::hasClasses>("hasClasses", "hasClasses");
            addProperty<bool (TypeDecl::*)() const, &ManagedType::hasNonTrivialCtor>("hasNonTrivialCtor", "hasNonTrivialCtor");
            addProperty<bool (TypeDecl::*)() const, &ManagedType::hasNonTrivialDtor>("hasNonTrivialDtor", "hasNonTrivialDtor");
            addProperty<bool (TypeDecl::*)() const, &ManagedType::hasNonTrivialCopy>("hasNonTrivialCopy", "hasNonTrivialCopy");
            addProperty<bool (TypeDecl::*)() const, &ManagedType::canBePlacedInContainer>("canBePlacedInContainer", "canBePlacedInContainer");
            addProperty<DAS_BIND_MANAGED_PROP(getVectorBaseType)>("vectorBaseType","getVectorBaseType");
            addProperty<DAS_BIND_MANAGED_PROP(getVectorDim)>("vectorDim","getVectorDim");
            addProperty<DAS_BIND_MANAGED_PROP(canInitWithZero)>("canInitWithZero","canInitWithZero");
            addProperty<DAS_BIND_MANAGED_PROP(getRangeBaseType)>("rangeBaseType","getRangeBaseType");
        }
    };

    struct AstStructureAnnotation : ManagedStructureAnnotation<Structure> {
        AstStructureAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("Structure", ml) {
        }
        void init () {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(fields)>("fields");
            addField<DAS_BIND_MANAGED_FIELD(at)>("at");
            addField<DAS_BIND_MANAGED_FIELD(module)>("_module","module");
            addField<DAS_BIND_MANAGED_FIELD(parent)>("parent");
            addField<DAS_BIND_MANAGED_FIELD(annotations)>("annotations");
            addFieldEx ( "flags", "flags", offsetof(Structure, flags), makeStructureFlags() );
        }
    };

    struct AstFieldDeclarationAnnotation : ManagedStructureAnnotation<Structure::FieldDeclaration> {
        AstFieldDeclarationAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("FieldDeclaration", ml, "Structure::FieldDeclaration") {
        }
        void init () {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(type)>("_type","type");
            addField<DAS_BIND_MANAGED_FIELD(init)>("init");
            addField<DAS_BIND_MANAGED_FIELD(annotation)>("annotation");
            addField<DAS_BIND_MANAGED_FIELD(at)>("at");
            addField<DAS_BIND_MANAGED_FIELD(offset)>("offset");
            addFieldEx ( "flags", "flags", offsetof(Structure::FieldDeclaration, flags), makeFieldDeclarationFlags() );
        }
    };

    struct AstEnumEntryAnnotation : ManagedStructureAnnotation <Enumeration::EnumEntry> {
        AstEnumEntryAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("EnumEntry", ml, "Enumeration::EnumEntry") {
        }
        void init () {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(value)>("value");
            addField<DAS_BIND_MANAGED_FIELD(at)>("at");
        }
    };

    struct AstEnumerationAnnotation : ManagedStructureAnnotation <Enumeration> {
        AstEnumerationAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("Enumeration", ml) {
        }
        void init () {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(cppName)>("cppName");
            addField<DAS_BIND_MANAGED_FIELD(at)>("at");
            addField<DAS_BIND_MANAGED_FIELD(list)>("list");
            addField<DAS_BIND_MANAGED_FIELD(module)>("_module","module");
            addField<DAS_BIND_MANAGED_FIELD(external)>("external");
            addField<DAS_BIND_MANAGED_FIELD(baseType)>("baseType");
            addField<DAS_BIND_MANAGED_FIELD(annotations)>("annotations");
            addField<DAS_BIND_MANAGED_FIELD(isPrivate)>("isPrivate");
        }
    };

    struct AstFunctionAnnotation : ManagedStructureAnnotation<Function> {
        AstFunctionAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("Function", ml) {
        }
        void init () {
            addField<DAS_BIND_MANAGED_FIELD(annotations)>("annotations");
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(arguments)>("arguments");
            addField<DAS_BIND_MANAGED_FIELD(result)>("result");
            addField<DAS_BIND_MANAGED_FIELD(body)>("body");
            addField<DAS_BIND_MANAGED_FIELD(index)>("index");
            addField<DAS_BIND_MANAGED_FIELD(totalStackSize)>("totalStackSize");
            addField<DAS_BIND_MANAGED_FIELD(totalGenLabel)>("totalGenLabel");
            addField<DAS_BIND_MANAGED_FIELD(at)>("at");
            addField<DAS_BIND_MANAGED_FIELD(atDecl)>("atDecl");
            addField<DAS_BIND_MANAGED_FIELD(module)>("_module", "module");
            // addField<DAS_BIND_MANAGED_FIELD(useFunctions)>("useFunctions");
            // addField<DAS_BIND_MANAGED_FIELD(useGlobalVariables)>("useGlobalVariables");
            addField<DAS_BIND_MANAGED_FIELD(classParent)>("classParent", "classParent");
            // use global v
            addFieldEx ( "flags", "flags",
                offsetof(Function, flags), makeFunctionFlags() );
            addFieldEx ( "moreFlags", "moreFlags",
                offsetof(Function, moreFlags), makeMoreFunctionFlags() );
            addFieldEx ( "sideEffectFlags", "sideEffectFlags",
                offsetof(Function, sideEffectFlags), makeFunctionSideEffectFlags() );
            addField<DAS_BIND_MANAGED_FIELD(inferStack)>("inferStack");
            addField<DAS_BIND_MANAGED_FIELD(fromGeneric)>("fromGeneric");
            addField<DAS_BIND_MANAGED_FIELD(hash)>("hash");
            addField<DAS_BIND_MANAGED_FIELD(aotHash)>("aotHash");
            // properties
            addProperty<DAS_BIND_MANAGED_PROP(getOrigin)>("origin","getOrigin");
            addProperty<DAS_BIND_MANAGED_PROP(isGeneric)>("isGeneric","isGeneric");
        }
    };

    struct AstInferHistoryAnnotation : ManagedStructureAnnotation<InferHistory> {
        AstInferHistoryAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("InferHistory", ml) {
        }
        void init () {
            addField<DAS_BIND_MANAGED_FIELD(at)>("at");
            addField<DAS_BIND_MANAGED_FIELD(func)>("func");
        }
    };


    struct AstVariableAnnotation : ManagedStructureAnnotation<Variable> {
        AstVariableAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("Variable", ml) {
        }
        void init () {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(aka)>("_aka","aka");
            addField<DAS_BIND_MANAGED_FIELD(type)>("_type","type");
            addField<DAS_BIND_MANAGED_FIELD(init)>("init");
            addField<DAS_BIND_MANAGED_FIELD(source)>("source");
            addField<DAS_BIND_MANAGED_FIELD(at)>("at");
            addField<DAS_BIND_MANAGED_FIELD(index)>("index");
            addField<DAS_BIND_MANAGED_FIELD(stackTop)>("stackTop");
            addField<DAS_BIND_MANAGED_FIELD(module)>("_module", "module");
            // addField<DAS_BIND_MANAGED_FIELD(useFunctions)>("useFunctions");
            // addField<DAS_BIND_MANAGED_FIELD(useFunctions)>("useGlobalVariables");
            addField<DAS_BIND_MANAGED_FIELD(initStackSize)>("initStackSize");
            addFieldEx ( "flags", "flags", offsetof(Variable, flags), makeVariableFlags() );
            addFieldEx ( "access_flags", "access_flags", offsetof(Variable, access_flags), makeVariableAccessFlags() );
            addField<DAS_BIND_MANAGED_FIELD(annotation)>("annotation");
            // properties
            addProperty<DAS_BIND_MANAGED_PROP(isAccessUnused)>("isAccessUnused","isAccessUnused");
        }
    };

    struct AstContextAnnotation : ManagedStructureAnnotation<AstContext,false> {
        AstContextAnnotation(ModuleLibrary & ml)
            :  ManagedStructureAnnotation ("AstContext", ml) {
            addField<DAS_BIND_MANAGED_FIELD(func)>("func");
            addField<DAS_BIND_MANAGED_FIELD(loop)>("_loop","loop");
            addField<DAS_BIND_MANAGED_FIELD(blocks)>("blocks");
            addField<DAS_BIND_MANAGED_FIELD(scopes)>("scopes");
            addField<DAS_BIND_MANAGED_FIELD(with)>("_with","with");
        }
    };

    struct AstExprBlockAnnotation : AstExpressionAnnotation<ExprBlock> {
        AstExprBlockAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprBlock> ("ExprBlock", ml) {
            addField<DAS_BIND_MANAGED_FIELD(list)>("list");
            addField<DAS_BIND_MANAGED_FIELD(finalList)>("finalList");
            addField<DAS_BIND_MANAGED_FIELD(returnType)>("returnType");
            addField<DAS_BIND_MANAGED_FIELD(arguments)>("arguments");
            addField<DAS_BIND_MANAGED_FIELD(stackTop)>("stackTop");
            addField<DAS_BIND_MANAGED_FIELD(stackVarTop)>("stackVarTop");
            addField<DAS_BIND_MANAGED_FIELD(stackVarBottom)>("stackVarBottom");
            addField<DAS_BIND_MANAGED_FIELD(stackCleanVars)>("stackCleanVars");
            addField<DAS_BIND_MANAGED_FIELD(maxLabelIndex)>("maxLabelIndex");
            addField<DAS_BIND_MANAGED_FIELD(annotations)>("annotations");
            addField<DAS_BIND_MANAGED_FIELD(annotationData)>("annotationData");
            addField<DAS_BIND_MANAGED_FIELD(annotationDataSid)>("annotationDataSid");
            addFieldEx ( "blockFlags", "blockFlags", offsetof(ExprBlock, blockFlags), makeExprBlockFlags() );
        }
    };

    struct AstExprLetAnnotation : AstExpressionAnnotation<ExprLet> {
        AstExprLetAnnotation(ModuleLibrary & ml)
            :  AstExpressionAnnotation<ExprLet> ("ExprLet", ml) {
            addField<DAS_BIND_MANAGED_FIELD(variables)>("variables");
            addField<DAS_BIND_MANAGED_FIELD(atInit)>("atInit");
            addFieldEx("letFlags", "letFlags", offsetof(ExprLet, letFlags), makeExprLetFlagsFlags());
        }
    };

    void Module_Ast::registerAnnotations1(ModuleLibrary & lib) {
        // QUOTE
        addCall<ExprQuote>("quote");
        // flags
        registerFlags(lib);
        // modules
        addAnnotation(make_smart<AstModuleLibraryAnnotation>(lib));
        // AST TYPES (due to a lot of xrefs we declare everyone as recursive type)
        auto exa = make_smart<AstExprAnnotation<Expression>>("Expression",lib);
        addAnnotation(exa);
        auto tda = make_smart<AstTypeDeclAnnnotation>(lib);
        addAnnotation(tda);
        auto sta = make_smart<AstStructureAnnotation>(lib);
        addAnnotation(sta);
        auto fta = make_smart<AstFieldDeclarationAnnotation>(lib);
        addAnnotation(fta);
        auto ene = make_smart<AstEnumEntryAnnotation>(lib);
        addAnnotation(ene);
        auto ena = make_smart<AstEnumerationAnnotation>(lib);
        addAnnotation(ena);
        auto fna = make_smart<AstFunctionAnnotation>(lib);
        addAnnotation(fna);
        auto iha = make_smart<AstInferHistoryAnnotation>(lib);
        addAnnotation(iha);
        auto vaa = make_smart<AstVariableAnnotation>(lib);
        addAnnotation(vaa);
        initRecAnnotation(tda, lib);
        initRecAnnotation(sta, lib);
        initRecAnnotation(fta, lib);
        initRecAnnotation(ene, lib);
        initRecAnnotation(ena, lib);
        initRecAnnotation(exa, lib);
        initRecAnnotation(fna, lib);
        initRecAnnotation(iha, lib);
        initRecAnnotation(vaa, lib);
        // misc
        addAnnotation(make_smart<AstContextAnnotation>(lib));
        // expressions (in order of inheritance)
        addExpressionAnnotation(make_smart<AstExprBlockAnnotation>(lib))->from("Expression");
        addExpressionAnnotation(make_smart<AstExprLetAnnotation>(lib))->from("Expression");
    }
}
