#include "daScript/misc/platform.h"

#include "unitTest.h"
#include "module_unitTest.h"

#include "daScript/simulate/simulate_visit_op.h"
#include "daScript/ast/ast_policy_types.h"

 int32_t TestObjectSmart::total = 0;

//sample of your-engine-float3-type to be aliased as float3 in daScript.
template<> struct das::cast <Point3>  : cast_fVec<Point3> {};

namespace das {
    template <>
    struct typeFactory<Point3> {
        static TypeDeclPtr make(const ModuleLibrary &) {
            auto t = make_smart<TypeDecl>(Type::tFloat3);
            t->alias = "Point3";
            t->aotAlias = true;
            return t;
        }
    };

    template <> struct typeName<Point3>   { constexpr static const char * name() { return "Point3"; } };
}

DAS_BASE_BIND_ENUM_98(SomeEnum_16, SomeEnum_16, SomeEnum_16_zero, SomeEnum_16_one, SomeEnum_16_two)

//sample of your engine annotated struct
MAKE_TYPE_FACTORY(TestObjectSmart,TestObjectSmart)
MAKE_TYPE_FACTORY(TestObjectFoo,TestObjectFoo)
MAKE_TYPE_FACTORY(TestObjectBar, TestObjectBar)
MAKE_TYPE_FACTORY(TestObjectNotLocal, TestObjectNotLocal)
MAKE_TYPE_FACTORY(TestObjectNotNullPtr, TestObjectNotNullPtr)
MAKE_TYPE_FACTORY(FancyClass, FancyClass)

MAKE_TYPE_FACTORY(SomeDummyType, SomeDummyType)
MAKE_TYPE_FACTORY(Point3Array, Point3Array)

namespace das {
  template <>
  struct typeFactory<SampleVariant> {
      static TypeDeclPtr make(const ModuleLibrary & library ) {
          auto vtype = make_smart<TypeDecl>(Type::tVariant);
          vtype->alias = "SampleVariant";
          vtype->aotAlias = true;
          vtype->addVariant("i_value", typeFactory<decltype(SampleVariant::i_value)>::make(library));
          vtype->addVariant("f_value", typeFactory<decltype(SampleVariant::f_value)>::make(library));
          vtype->addVariant("s_value", typeFactory<decltype(SampleVariant::s_value)>::make(library));
          // optional validation
          DAS_ASSERT(sizeof(SampleVariant) == vtype->getSizeOf());
          DAS_ASSERT(alignof(SampleVariant) == vtype->getAlignOf());
          DAS_ASSERT(offsetof(SampleVariant, i_value) == vtype->getVariantFieldOffset(0));
          DAS_ASSERT(offsetof(SampleVariant, f_value) == vtype->getVariantFieldOffset(1));
          DAS_ASSERT(offsetof(SampleVariant, s_value) == vtype->getVariantFieldOffset(2));
          return vtype;
      }
  };
};

struct TestObjectNotLocalAnnotation : ManagedStructureAnnotation <TestObjectNotLocal> {
    TestObjectNotLocalAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("TestObjectNotLocal", ml) {
        addField<DAS_BIND_MANAGED_FIELD(fooData)>("fooData");
    }
    virtual bool isLocal() const { return false; }                  // this is here so that the class is not local
    virtual bool canBePlacedInContainer() const { return false; }   // this can't be in the container either
};

struct TestObjectNotNullPtrAnnotation : ManagedStructureAnnotation <TestObjectNotNullPtr> {
    TestObjectNotNullPtrAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("TestObjectNotNullPtr", ml) {
        addField<DAS_BIND_MANAGED_FIELD(fooData)>("fooData");
    }
    virtual bool avoidNullPtr() const override { return true; }         // obvious null-ptr-ing this object is an error
};

struct TestObjectSmartAnnotation : ManagedStructureAnnotation <TestObjectSmart> {
    TestObjectSmartAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("TestObjectSmart", ml) {
        addField<DAS_BIND_MANAGED_FIELD(fooData)>("fooData");
    }
    void init() {
        // this needs to be separate
        // reason: recursive type
        addField<DAS_BIND_MANAGED_FIELD(first)>("first");
    }
};

struct TestObjectFooAnnotation : ManagedStructureAnnotation <TestObjectFoo> {
    TestObjectFooAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("TestObjectFoo", ml) {
        addField<DAS_BIND_MANAGED_FIELD(hit)>("hit");
        addField<DAS_BIND_MANAGED_FIELD(lookAt)>("lookAt");
        addField<DAS_BIND_MANAGED_FIELD(fooData)>("fooData");
        addField<DAS_BIND_MANAGED_FIELD(e16)>("e16");
        addProperty<DAS_BIND_MANAGED_PROP(propAdd13)>("propAdd13");
        addProperty<DAS_BIND_MANAGED_PROP(hitPos)>("hitPos");
        addProperty<DAS_BIND_MANAGED_PROP(hitPosRef)>("hitPosRef");
        addPropertyExtConst<
            bool (TestObjectFoo::*)(), &TestObjectFoo::isReadOnly,
            bool (TestObjectFoo::*)() const, &TestObjectFoo::isReadOnly
        >("isReadOnly","isReadOnly");
        addField<DAS_BIND_MANAGED_FIELD(fooArray)>("fooArray");
        addProperty<DAS_BIND_MANAGED_PROP(getWordFoo)>("wordFoo","getWordFoo");
    }
    void init() {
        addField<DAS_BIND_MANAGED_FIELD(foo_loop)>("foo_loop");
        addPropertyExtConst<
            TestObjectFoo * (TestObjectFoo::*)(), &TestObjectFoo::getLoop,
            const TestObjectFoo * (TestObjectFoo::*)() const, &TestObjectFoo::getLoop
        >("getLoop","getLoop");
    }
};

struct TestObjectBarAnnotation : ManagedStructureAnnotation <TestObjectBar> {
    TestObjectBarAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("TestObjectBar", ml) {
        addField<DAS_BIND_MANAGED_FIELD(fooPtr)>("fooPtr");
        addField<DAS_BIND_MANAGED_FIELD(barData)>("barData");
        addProperty<DAS_BIND_MANAGED_PROP(getFoo)>("getFoo");
        addProperty<DAS_BIND_MANAGED_PROP(getFooPtr)>("getFooPtr");
    }
};

void testFoo ( TestObjectFoo & foo ) {
    foo.fooData = 1234;
}

void testAdd ( int & a, int b ) {
    a += b;
}

struct CheckRange : StructureAnnotation {
    CheckRange() : StructureAnnotation("checkRange") {}
    virtual bool touch(const StructurePtr & st, ModuleGroup &,
        const AnnotationArgumentList & args, string & ) override {
        // this is here for the 'example' purposes
        // lets add a sample 'dummy' field
        if (args.getBoolOption("dummy",false) && !st->findField("dummy")) {
            st->fields.emplace_back("dummy", make_smart<TypeDecl>(Type::tInt),
                nullptr /*init*/, AnnotationArgumentList(), false /*move_to_init*/, LineInfo());
            st->fieldLookup.clear();
        }
        return true;
    }
    virtual bool look ( const StructurePtr & st, ModuleGroup &,
                        const AnnotationArgumentList & args, string & err ) override {
        bool ok = true;
        if (!args.getBoolOption("disable", false)) {
            for (auto & fd : st->fields) {
                if (fd.type->isSimpleType(Type::tInt) && fd.annotation.size()) {
                    int32_t val = 0;
                    int32_t minVal = INT32_MIN;
                    int32_t maxVal = INT32_MAX;
                    if (fd.init && fd.init->rtti_isConstant()) {
                        val = static_pointer_cast<ExprConstInt>(fd.init)->getValue();
                    }
                    if (auto minA = fd.annotation.find("min", Type::tInt)) {
                        minVal = minA->iValue;
                    }
                    if (auto maxA = fd.annotation.find("max", Type::tInt)) {
                        maxVal = maxA->iValue;
                    }
                    if (val<minVal || val>maxVal) {
                        err += fd.name + " out of annotated range [" + to_string(minVal) + ".." + to_string(maxVal) + "]\n";
                        ok = false;
                    }
                }
            }
        }
        return ok;
    }
};

void test_das_string(const Block & block, Context * context, LineInfoArg * at) {
    string str = "test_das_string";
    string str2;
    vec4f args[2];
    args[0] = cast<void *>::from(&str);
    args[1] = cast<void *>::from(&str2);
    context->invoke(block, args, nullptr, at);
    if (str != "out_of_it") context->throw_error_at(at, "test string mismatch");
    if (str2 != "test_das_string") context->throw_error_at(at, "test string clone mismatch");
}

vec4f new_and_init ( Context & context, SimNode_CallBase * call, vec4f * ) {
    TypeInfo * typeInfo = call->types[0];
    if ( typeInfo->dim || typeInfo->type!=Type::tStructure ) {
        context.throw_error_at(call->debugInfo, "invalid type");
        return v_zero();
    }
    auto size = getTypeSize(typeInfo);
    auto data = context.allocate(size, &call->debugInfo);
    if ( typeInfo->structType && typeInfo->structType->init_mnh ) {
        auto fn = context.fnByMangledName(typeInfo->structType->init_mnh);
        context.callWithCopyOnReturn(fn, nullptr, data, 0);
    } else {
        memset(data, 0, size);
    }
    return cast<char *>::from(data);
}
int g_st = 0;
int *getPtr() {return &g_st;}

uint2 get_screen_dimensions() {return uint2{1280, 720};}

uint64_t CheckEid ( TestObjectFoo & foo, char * const name, Context * context, LineInfoArg * at ) {
    if (!name) context->throw_error_at(at, "invalid id");
    return hash_function(*context, name) + foo.fooData;
}

uint64_t CheckEidHint ( TestObjectFoo & foo, char * const name, uint64_t hashHint, Context * context, LineInfoArg * at ) {
    if (!name) context->throw_error_at(at, "invalid id");
    uint64_t hv = hash_function(*context, name) + foo.fooData;
    if ( hv != hashHint ) context->throw_error_at(at, "invalid hash value");
    return hashHint;
}

struct CheckEidFunctionAnnotation : TransformFunctionAnnotation {
    CheckEidFunctionAnnotation() : TransformFunctionAnnotation("check_eid") { }
    virtual ExpressionPtr transformCall ( ExprCallFunc * call, string & err ) override {
        auto arg = call->arguments[1];
        if ( arg->type && arg->type->isString() && arg->type->isConst() && arg->rtti_isConstant() ) {
            auto starg = static_pointer_cast<ExprConstString>(arg);
            if (!starg->getValue().empty()) {
                auto hv = hash_blockz64((uint8_t *)starg->text.c_str());
                auto hconst = make_smart<ExprConstUInt64>(arg->at, hv);
                hconst->type = make_smart<TypeDecl>(Type::tUInt64);
                hconst->type->constant = true;
                auto newCall = static_pointer_cast<ExprCallFunc>(call->clone());
                newCall->arguments.insert(newCall->arguments.begin() + 2, hconst);
                return newCall;
            } else {
                err = "EID can't be an empty string";
            }
        }
        return nullptr;
    }
    // note:
    //  the hints will appear only on the non-transformed function!!!
    string aotArgumentPrefix(ExprCallFunc * /*call*/, int argIndex) override {
        return "/*arg-" + to_string(argIndex) + "-prefix*/ ";
    }
    string aotArgumentSuffix(ExprCallFunc * /*call*/, int argIndex) override {
        return " /*arg-" + to_string(argIndex) + "-suffix*/";
    }
};

struct TestFunctionAnnotation : FunctionAnnotation {
    TestFunctionAnnotation() : FunctionAnnotation("test_function") { }
    virtual bool apply ( const FunctionPtr & fn, ModuleGroup &, const AnnotationArgumentList &, string & ) override {
        TextPrinter tp;
        tp << "test function: apply " << fn->describe() << "\n";
        return true;
    }
    virtual bool finalize ( const FunctionPtr & fn, ModuleGroup &, const AnnotationArgumentList &, const AnnotationArgumentList &, string & ) override {
        TextPrinter tp;
        tp << "test function: finalize " << fn->describe() << "\n";
        return true;
    }
    virtual bool apply ( ExprBlock * blk, ModuleGroup &, const AnnotationArgumentList &, string & ) override {
        TextPrinter tp;
        tp << "test function: apply to block at " << blk->at.describe() << "\n";
        return true;
    }
    virtual bool finalize ( ExprBlock * blk, ModuleGroup &, const AnnotationArgumentList &, const AnnotationArgumentList &, string & ) override {
        TextPrinter tp;
        tp << "test function: finalize block at " << blk->at.describe() << "\n";
        return true;
    }
};

struct EventRegistrator : StructureAnnotation {
    EventRegistrator() : StructureAnnotation("event") {}
    bool touch ( const StructurePtr & st, ModuleGroup & /*libGroup*/,
        const AnnotationArgumentList & /*args*/, string & /*err*/ ) override {
        st->fields.emplace(st->fields.begin(), "eventFlags", make_smart<TypeDecl>(Type::tUInt16),
            ExpressionPtr(), AnnotationArgumentList(), false, st->at);
        st->fields.emplace(st->fields.begin(), "eventSize", make_smart<TypeDecl>(Type::tUInt16),
            ExpressionPtr(), AnnotationArgumentList(), false, st->at);
        st->fields.emplace(st->fields.begin(), "eventType", make_smart<TypeDecl>(Type::tUInt64),
            ExpressionPtr(), AnnotationArgumentList(), false, st->at);
        return true;
    }
    bool look (const StructurePtr & /*st*/, ModuleGroup & /*libGroup*/,
        const AnnotationArgumentList & /*args*/, string & /* err */ ) override {
        return true;
    }
};

#if defined(__APPLE__)

void builtin_printw(char * utf8string) {
    fprintf(stdout, "%s", utf8string);
    fflush(stdout);
}

#else

#include <iostream>
#include <codecvt>
#include <locale>

#if defined(_MSC_VER)
#include <fcntl.h>
#include <io.h>
#endif

void builtin_printw(char * utf8string) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
    auto outs = utf8_conv.from_bytes(utf8string);
#if defined(_MSC_VER)
    _setmode(_fileno(stdout), _O_U8TEXT);
#else
    std::wcout.sync_with_stdio(false);
    std::wcout.imbue(std::locale("en_US.utf8"));
#endif
    std::wcout << outs;
#if defined(_MSC_VER)
    _setmode(_fileno(stdout), _O_TEXT);
#endif
}

#endif

bool tempArrayExample( const TArray<char *> & arr,
    const TBlock<void, TTemporary<const TArray<char *>>> & blk, Context * context, LineInfoArg * at ) {
    vec4f args[1];
    args[0] = cast<void *>::from(&arr);
    context->invoke(blk, args, nullptr, at);
    return (arr.size == 1) && (strcmp(arr[0], "one") == 0);
}

void testPoint3Array(const TBlock<void,const Point3Array> & blk, Context * context, LineInfoArg * at) {
    Point3Array arr;
    for (int32_t x = 0; x != 10; ++x) {
        Point3 p;
        p.x = 1.0f;
        p.y = 2.0f;
        p.z = 3.0f;
        arr.push_back(p);
    }
    vec4f args[1];
    args[0] = cast<Point3Array *>::from(&arr);
    context->invoke(blk, args, nullptr, at);
}

TDim<int32_t,10> testCMRES ( Context * __context__ )
{
	TDim<int32_t,10> __a_rename_at_7; das_zero(__a_rename_at_7);
	{
		bool __need_loop_8 = true;
		// j : int& -const
		das_iterator<TDim<int32_t,10>> __j_iterator(__a_rename_at_7);
		int32_t * __j_rename_at_8;
		__need_loop_8 = __j_iterator.first(__context__,(__j_rename_at_8)) && __need_loop_8;
		for ( ; __need_loop_8 ; __need_loop_8 = __j_iterator.next(__context__,(__j_rename_at_8)) )
		{
			das_copy((*__j_rename_at_8),13);
		}
		__j_iterator.close(__context__,(__j_rename_at_8));
	};
	return das_auto_cast_ref<TDim<int32_t,10>>::cast(__a_rename_at_7);
}

void testFooArray(const TBlock<void,FooArray> & blk, Context * context, LineInfoArg * at) {
    FooArray arr;
    arr.reserve(16);
    for (int32_t x = 0; x != 10; ++x) {
        TestObjectFoo p;
        p.fooData = x;
        p.foo_loop = nullptr;
        p.lookAt = nullptr;
        arr.push_back(p);
    }
    vec4f args[1];
    args[0] = cast<FooArray *>::from(&arr);
    context->invoke(blk, args, nullptr, at);
}

void tableMojo ( TTable<char *,int> & in, const TBlock<void,TTable<char *,int>> & block, Context * context, LineInfoArg * at ) {
    vec4f args[1];
    args[0] = cast<Table *>::from(&in);
    context->invoke(block, args, nullptr, at);
}

class Point3ArrayAnnotation : public ManagedVectorAnnotation<Point3Array> {
public:
    Point3ArrayAnnotation(ModuleLibrary & lib)
        : ManagedVectorAnnotation<Point3Array>("Point3Array",lib) {
    }
    virtual bool isIndexMutable ( const TypeDeclPtr & ) const override { return true; }
};

MAKE_TYPE_FACTORY(EntityId,EntityId);

struct EntityIdAnnotation final: das::ManagedValueAnnotation <EntityId> {
    EntityIdAnnotation(ModuleLibrary & mlib) : ManagedValueAnnotation  (mlib,"EntityId","EntityId") {}
    virtual void walk ( das::DataWalker & walker, void * data ) override {
        if ( !walker.reading ) {
            const EntityId * t = (EntityId *) data;
            int32_t eidV = t->value;
            walker.Int(eidV);
        }
    }
    virtual bool isLocal() const override { return true; }
    virtual bool hasNonTrivialCtor() const override { return false; }
    virtual bool canBePlacedInContainer() const override { return true;}
};

void tempArrayAliasExample(const das::TArray<Point3> & arr,
    const das::TBlock<void, das::TTemporary<const das::TArray<Point3>>> & blk,
    das::Context * context, LineInfoArg * at) {
    vec4f args[1];
    args[0] = cast<void *>::from(&arr);
    context->invoke(blk, args, nullptr, at);
}

struct FancyClassAnnotation : ManagedStructureAnnotation <FancyClass,true,true> {
    FancyClassAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("FancyClass", ml) {
        addField<DAS_BIND_MANAGED_FIELD(value)>("value");
    }
    virtual bool isLocal() const override { return true; }                  // this is here so that we can make local variable and init with special c-tor
    virtual bool canBePlacedInContainer() const override { return true; }   // this is here so that we can make array<FancyClass>
};

void test_abi_lambda_and_function ( Lambda lambda, Func fn, int32_t lambdaSize, Context * context, LineInfoArg * lineinfo ) {
    das_invoke_function<void>::invoke(context, lineinfo, fn, lambdaSize);
    das_invoke_lambda<void>::invoke(context, lineinfo, lambda, lambdaSize);
}

MAKE_TYPE_FACTORY(SceneNodeId,SceneNodeId);


struct SceneNodeIdAnnotation final: das::ManagedValueAnnotation <SceneNodeId> {
    SceneNodeIdAnnotation(ModuleLibrary & mlib) : ManagedValueAnnotation  (mlib,"SceneNodeId","SceneNodeId") {}
    virtual void walk ( das::DataWalker & walker, void * data ) override {
        if ( !walker.reading ) {
            const SceneNodeId * t = (SceneNodeId *) data;
            uint32_t eidV = t->id;
            walker.UInt(eidV);
        }
    }
    virtual bool isLocal() const override { return true; }
    virtual bool hasNonTrivialCtor() const override { return false; }
    virtual bool canBePlacedInContainer() const override { return true;}
};

SceneNodeId __create_scene_node() {
    static uint32_t id = 1;
    return SceneNodeId{id++};
}

Module_UnitTest::Module_UnitTest() : Module("UnitTest") {
    ModuleLibrary lib(this);
    lib.addBuiltInModule();
    addBuiltinDependency(lib, Module::require("math"));
    addEnumTest(lib);
    // options
    options["unit_test"] = Type::tFloat;
    // constant
    addConstant(*this, "UNIT_TEST_CONSTANT", 0x12345678);
    // structure annotations
    addAnnotation(make_smart<CheckRange>());
    // dummy type example
    addAnnotation(make_smart<DummyTypeAnnotation>("SomeDummyType", "SomeDummyType", sizeof(SomeDummyType), alignof(SomeDummyType)));
    // register types
    addAnnotation(make_smart<TestObjectNotNullPtrAnnotation>(lib));
    addAnnotation(make_smart<TestObjectNotLocalAnnotation>(lib));
    auto fooann = make_smart<TestObjectFooAnnotation>(lib);
    addAnnotation(fooann);
    initRecAnnotation(fooann,lib);
    addAnnotation(make_smart<TestObjectBarAnnotation>(lib));
    // smart object recursive type
    auto tosa = make_smart<TestObjectSmartAnnotation>(lib);
    addAnnotation(tosa);
    initRecAnnotation(tosa, lib);
    // events
    addAnnotation(make_smart<EventRegistrator>());
    // test
    addAnnotation(make_smart<TestFunctionAnnotation>());
    // point3 array
    addAlias(typeFactory<Point3>::make(lib));
    addVectorAnnotation<Point3Array>(this,lib,make_smart<Point3ArrayAnnotation>(lib));
    addCtorAndUsing<Point3Array>(*this, lib, "Point3Array", "Point3Array");
    addExtern<DAS_BIND_FUN(testPoint3Array)>(*this, lib, "testPoint3Array",
        SideEffects::modifyExternal, "testPoint3Array");
    addExtern<DAS_BIND_FUN(testCMRES),SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "testCMRES",
        SideEffects::modifyExternal, "testCMRES");
    // foo array
    addExtern<DAS_BIND_FUN(testFooArray)>(*this, lib, "testFooArray",
        SideEffects::modifyExternal, "testFooArray");
    addExtern<DAS_BIND_FUN(set_foo_data)>(*this, lib, "set_foo_data",
        das::SideEffects::modifyArgument, "set_foo_data");
    // utf8 print
    addExtern<DAS_BIND_FUN(builtin_printw)>(*this, lib, "printw",
        SideEffects::modifyExternal, "builtin_printw");
    // register function
    addEquNeq<TestObjectFoo>(*this, lib);
    addExtern<DAS_BIND_FUN(complex_bind)>(*this, lib, "complex_bind",
        SideEffects::modifyExternal, "complex_bind");
    addInterop<new_and_init,void *,vec4f>(*this, lib, "new_and_init",
        SideEffects::none, "new_and_init");
    addExtern<DAS_BIND_FUN(get_screen_dimensions)>(*this, lib, "get_screen_dimensions",
        SideEffects::none, "get_screen_dimensions");
    addExtern<DAS_BIND_FUN(test_das_string)>(*this, lib, "test_das_string",
        SideEffects::modifyExternal, "test_das_string");
    addExtern<DAS_BIND_FUN(testFoo)>(*this, lib, "testFoo",
        SideEffects::modifyArgument, "testFoo");
    addExtern<DAS_BIND_FUN(testAdd)>(*this, lib, "testAdd",
        SideEffects::modifyArgument, "testAdd");
    addExtern<DAS_BIND_FUN(getSamplePoint3)>(*this, lib, "getSamplePoint3",
        SideEffects::modifyExternal, "getSamplePoint3");
    addExtern<DAS_BIND_FUN(doubleSamplePoint3)>(*this, lib, "doubleSamplePoint3",
        SideEffects::none, "doubleSamplePoint3");
    addExtern<DAS_BIND_FUN(project_to_nearest_navmesh_point)>(*this, lib, "project_to_nearest_navmesh_point",
        SideEffects::modifyArgument, "project_to_nearest_navmesh_point");
    addExtern<DAS_BIND_FUN(getPtr)>(*this, lib, "getPtr",
        SideEffects::modifyExternal, "getPtr");
    /*
     addExtern<DAS_BIND_FUN(makeDummy)>(*this, lib, "makeDummy", SideEffects::none, "makeDummy");
     */
    addExtern<DAS_BIND_FUN(makeDummy), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "makeDummy",
        SideEffects::none, "makeDummy");
    addExtern<DAS_BIND_FUN(makeDummy), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "makeTempDummy",
        SideEffects::none, "makeDummy")->setTempResult();
    addExtern<DAS_BIND_FUN(takeDummy)>(*this, lib, "takeDummy",
        SideEffects::none, "takeDummy");
    // register Cpp alignment functions
    addExtern<DAS_BIND_FUN(CppS1Size)>(*this, lib, "CppS1Size",
        SideEffects::modifyExternal, "CppS1Size");
    addExtern<DAS_BIND_FUN(CppS2Size)>(*this, lib, "CppS2Size",
        SideEffects::modifyExternal, "CppS2Size");
    addExtern<DAS_BIND_FUN(CppS2DOffset)>(*this, lib, "CppS2DOffset",
        SideEffects::modifyExternal, "CppS2DOffset");
    // register CheckEid functions
    addExtern<DAS_BIND_FUN(CheckEidHint)>(*this, lib, "CheckEid",
        SideEffects::none, "CheckEidHint");
    auto ceid = addExtern<DAS_BIND_FUN(CheckEid)>(*this, lib,
        "CheckEid", SideEffects::none, "CheckEid");
    auto ceid_decl = make_smart<AnnotationDeclaration>();
    ceid_decl->annotation = make_smart<CheckEidFunctionAnnotation>();
    ceid->annotations.push_back(ceid_decl);
    // register CheckEid2 functoins and macro
    addExtern<DAS_BIND_FUN(CheckEidHint)>(*this, lib, "CheckEid2",
        SideEffects::modifyExternal, "CheckEidHint");
    addExtern<DAS_BIND_FUN(CheckEid)>(*this, lib, "CheckEid2",
        SideEffects::modifyExternal, "CheckEid");
    addExtern<DAS_BIND_FUN(CheckEid)>(*this, lib, "CheckEid3",
        SideEffects::modifyExternal, "CheckEid");
    // extra tests
    addExtern<DAS_BIND_FUN(start_effect)>(*this, lib, "start_effect",
        SideEffects::modifyExternal, "start_effect");
    addExtern<DAS_BIND_FUN(tempArrayExample)>(*this, lib, "temp_array_example",
        SideEffects::modifyExternal, "tempArrayExample");
    addExtern<DAS_BIND_FUN(tempArrayAliasExample)>(*this, lib, "temp_array_alias_example",
        SideEffects::invoke, "tempArrayAliasExample");
    // ptr2ref
    addExtern<DAS_BIND_FUN(fooPtr2Ref),SimNode_ExtFuncCallRef>(*this, lib, "fooPtr2Ref",
        SideEffects::none, "fooPtr2Ref");
    // compiled functions
    appendCompiledFunctions();
    // add sample variant type
    addAlias(typeFactory<SampleVariant>::make(lib));
    addExtern<DAS_BIND_FUN(makeSampleI), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "makeSampleI",
        SideEffects::none, "makeSampleI");
    addExtern<DAS_BIND_FUN(makeSampleF), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "makeSampleF",
        SideEffects::none, "makeSampleF");
    addExtern<DAS_BIND_FUN(makeSampleS), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "makeSampleS",
        SideEffects::none, "makeSampleS");
    // smart ptr
    addExtern<DAS_BIND_FUN(getTotalTestObjectSmart)>(*this, lib, "getTotalTestObjectSmart",
        SideEffects::accessExternal, "getTotalTestObjectSmart");
    addExtern<DAS_BIND_FUN(makeTestObjectSmart)>(*this, lib, "makeTestObjectSmart",
        SideEffects::modifyExternal, "makeTestObjectSmart");
    addExtern<DAS_BIND_FUN(countTestObjectSmart)>(*this, lib, "countTestObjectSmart",
        SideEffects::none, "countTestObjectSmart");
    // div
    addExtern<DAS_BIND_FUN(testGetDiv)>(*this, lib, "testGetDiv",
        SideEffects::none, "testGetDiv");
    addExtern<DAS_BIND_FUN(testGetNan)>(*this, lib, "testGetNan",
        SideEffects::none, "testGetNan");
    // call line
    addExtern<DAS_BIND_FUN(testCallLine)>(*this, lib, "testCallLine",
        SideEffects::modifyExternal, "testCallLine");
    // table mojo
    addExtern<DAS_BIND_FUN(tableMojo)>(*this, lib, "tableMojo",
        SideEffects::modifyExternal, "tableMojo");
    // EntityId
    addAnnotation(make_smart<EntityIdAnnotation>(lib));
    addExtern<DAS_BIND_FUN(make_invalid_id)>(*this, lib, "make_invalid_id",
        SideEffects::none, "make_invalid_id");
    addExtern<DAS_BIND_FUN(eidToInt)>(*this, lib, "int",
        SideEffects::none, "eidToInt");
    addExtern<DAS_BIND_FUN(intToEid)>(*this, lib, "EntityId",
        SideEffects::none, "intToEid");
    // FancyClass
    addAnnotation(make_smart<FancyClassAnnotation>(lib));
    addCtorAndUsing<FancyClass>(*this,lib,"FancyClass","FancyClass");
    addCtorAndUsing<FancyClass,int32_t,int32_t>(*this,lib,"FancyClass","FancyClass");
    addExtern<DAS_BIND_FUN(deleteFancyClass)>(*this, lib, "deleteFancyClass",
        SideEffects::modifyArgumentAndExternal, "deleteFancyClassDummy");
    // member function
    using method_hitMe = DAS_CALL_MEMBER(TestObjectFoo::hitMe);
    addExtern< DAS_CALL_METHOD(method_hitMe) >(*this, lib, "hit_me", SideEffects::modifyArgument,
       DAS_CALL_MEMBER_CPP(TestObjectFoo::hitMe));
    // abit
    addExtern<DAS_BIND_FUN(test_abi_lambda_and_function)>(*this, lib, "test_abi_lambda_and_function",
        SideEffects::invokeAndAccessExternal, "test_abi_lambda_and_function");
    // SceneNodeId
    addAnnotation(make_smart<SceneNodeIdAnnotation>(lib));
    addExtern<DAS_BIND_FUN(__create_scene_node)>(*this, lib, "__create_scene_node",
        SideEffects::none, "__create_scene_node");
    // and verify
    verifyAotReady();
}

ModuleAotType Module_UnitTest::aotRequire ( TextWriter & tw ) const {
    tw << "#include \"unitTest.h\"\n";
    return ModuleAotType::cpp;
}

#include "unit_test.das.inc"
bool Module_UnitTest::appendCompiledFunctions() {
    return compileBuiltinModule("unit_test.das",unit_test_das, sizeof(unit_test_das));
}

REGISTER_MODULE(Module_UnitTest);
