#pragma once

#include "daScript/simulate/simulate.h"
#include "daScript/simulate/bind_enum.h"
#include "daScript/simulate/aot.h"
#include "daScript/simulate/jit_abi.h"

enum class SomeEnum {
    zero
,   one
,   two
};

DAS_BIND_ENUM_CAST(SomeEnum);

namespace Goo {
    enum class GooEnum {
        regular
    ,   hazardous
    };

    enum GooEnum98 {
        soft
    ,   hard
    };
}

DAS_BIND_ENUM_CAST(::Goo::GooEnum);
DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(::Goo::GooEnum98,GooEnum98);

enum SomeEnum98 {
    SomeEnum98_zero = 0
,   SomeEnum98_one  = 1
,   SomeEnum98_two  = 2
};

DAS_BIND_ENUM_CAST_98(SomeEnum98);

enum SomeEnum_16 : int16_t {
    SomeEnum_16_zero = 0
,   SomeEnum_16_one  = 1
,   SomeEnum_16_two  = 2
};
DAS_BIND_ENUM_CAST_98(SomeEnum_16);

Goo::GooEnum efn_flip ( Goo::GooEnum goo );
SomeEnum efn_takeOne_giveTwo ( SomeEnum one );
SomeEnum98 efn_takeOne_giveTwo_98 ( SomeEnum98 one );
SomeEnum98_DasProxy efn_takeOne_giveTwo_98_DasProxy ( SomeEnum98_DasProxy two );

//sample of your-engine-float3-type to be aliased as float3 in daScript.
class Point3 { public: float x, y, z; };

template <> struct das::das_alias<Point3> : das::das_alias_vec<Point3,float3> {};

typedef das::vector<Point3> Point3Array;

void testPoint3Array(const das::TBlock<void, const Point3Array> & blk, das::Context * context, das::LineInfoArg * lineinfo);
das::TDim<int32_t,10> testCMRES ( das::Context * __context__ );

struct SomeDummyType {
    int32_t foo;
    float   bar;
};

__forceinline Point3 getSamplePoint3() {return Point3{0,1,2};}
__forceinline Point3 doubleSamplePoint3(const Point3 &a) { return Point3{ a.x + a.x, a.y + a.y, a.z + a.z }; }
__forceinline void project_to_nearest_navmesh_point(Point3 & a, float t) { a = Point3{ a.x + t, a.y + t, a.z + t }; }

struct TestObjectFoo {
    Point3 hit;
    const Point3 * lookAt;
    int32_t fooData;
    SomeEnum_16 e16;
    TestObjectFoo * foo_loop;
    int32_t fooArray[3];
    int propAdd13() {
        return fooData + 13;
    }
    __forceinline Point3 hitPos() const { return hit; }
    __forceinline const Point3 & hitPosRef() const { return hit; }
    __forceinline bool operator == ( const TestObjectFoo & obj ) const {
        return fooData == obj.fooData;
    }
    __forceinline bool operator != ( const TestObjectFoo & obj ) const {
        return fooData != obj.fooData;
    }
    __forceinline bool isReadOnly() { return false; }
    __forceinline bool isReadOnly() const { return true; }
    __forceinline TestObjectFoo * getLoop() { return foo_loop; }
    __forceinline const TestObjectFoo * getLoop() const { return foo_loop; }
    __forceinline const char * getWordFoo() const { return "foo"; }
    void hitMe ( int a, int b ) { fooData = a + b; }
};
static_assert ( std::is_trivially_constructible<TestObjectFoo>::value, "this one needs to remain trivially constructable" );

typedef das::vector<TestObjectFoo> FooArray;

void testFooArray(const das::TBlock<void, FooArray> & blk, das::Context * context, das::LineInfoArg * lineinfo);

__forceinline void set_foo_data (TestObjectFoo * obj, int32_t data ) { obj->fooData = data; }

struct TestObjectSmart : public das::ptr_ref_count {
     int fooData = 1234;
     das::smart_ptr<TestObjectSmart> first;
     TestObjectSmart() { total ++; }
     virtual ~TestObjectSmart() { total --; }
     static int32_t total;
};

__forceinline das::smart_ptr<TestObjectSmart> makeTestObjectSmart() { return das::make_smart<TestObjectSmart>(); }
__forceinline uint32_t countTestObjectSmart( const das::smart_ptr<TestObjectSmart> & p ) { return p->use_count(); }

__forceinline int32_t getTotalTestObjectSmart() { return TestObjectSmart::total; }

__forceinline TestObjectFoo makeDummy() { TestObjectFoo x; memset(&x, 0, sizeof(x)); x.fooData = 1;  return x; }
__forceinline int takeDummy ( const TestObjectFoo & x ) { return x.fooData; }

struct TestObjectBar {
    TestObjectFoo * fooPtr;
    float           barData;
    TestObjectFoo & getFoo() { return *fooPtr; }
    TestObjectFoo * getFooPtr() { return fooPtr; }
};

struct TestObjectNotLocal {
    int fooData;
};

struct TestObjectNotNullPtr {
    int fooData;
};


int *getPtr();

void testFields ( das::Context * ctx );
void test_das_string(const das::Block & block, das::Context * context, das::LineInfoArg * lineinfo);
vec4f new_and_init ( das::Context & context, das::SimNode_CallBase * call, vec4f * );

struct CppS1 {
    virtual ~CppS1() {}
    // int64_t * a;
    int64_t b;
    int32_t c;
};

struct CppS2 : CppS1 {
    int32_t d;
};

__forceinline float testGetDiv ( float a, float b ) { return a / b; }
__forceinline float testGetNan() { return nanf("test"); }

__forceinline int CppS1Size() { return int(sizeof(CppS1)); }
__forceinline int CppS2Size() { return int(sizeof(CppS2)); }
__forceinline int CppS2DOffset() { return int(offsetof(CppS2, d)); }


uint64_t CheckEid ( TestObjectFoo & foo, char * const name, das::Context * context, das::LineInfoArg * lineinfo );
uint64_t CheckEidHint ( TestObjectFoo & foo, char * const name, uint64_t hashHint, das::Context * context, das::LineInfoArg * lineinfo );

__forceinline void complex_bind (const TestObjectFoo &,
                          const das::TBlock<void, das::TArray<TestObjectFoo>> &,
                          das::Context *, das::LineInfoArg *) {
    // THIS DOES ABSOLUTELY NOTHING, ITS HERE TO TEST BIND OF ARRAY INSIDE BLOCK
}

__forceinline bool start_effect(const char *, const das::float3x4 &, float) {
    // THIS DOES ABSOLUTELY NOTHING, ITS HERE TO TEST DEFAULT ARGUMENTS
    return false;
}

void builtin_printw(char * utf8string);

bool tempArrayExample(const das::TArray<char *> & arr,
    const das::TBlock<void, das::TTemporary<const das::TArray<char *>>> & blk,
    das::Context * context, das::LineInfoArg * lineinfo);

void tempArrayAliasExample(const das::TArray<Point3> & arr,
    const das::TBlock<void, das::TTemporary<const das::TArray<Point3>>> & blk,
    das::Context * context, das::LineInfoArg * lineinfo);

__forceinline TestObjectFoo & fooPtr2Ref(TestObjectFoo * pMat) {
    return *pMat;
}

struct SampleVariant {
    int32_t _variant;
    union {
        int32_t     i_value;
        float       f_value;
        char *      s_value;
    };
};

template <> struct das::das_alias<SampleVariant>
    : das::das_alias_ref<SampleVariant,TVariant<sizeof(SampleVariant),alignof(SampleVariant),int32_t,float,char *>> {};

__forceinline SampleVariant makeSampleI() {
    SampleVariant v;
    v._variant = 0;
    v.i_value = 1;
    return v;
}

__forceinline SampleVariant makeSampleF() {
    SampleVariant v;
    v._variant = 1;
    v.f_value = 2.0f;
    return v;
}

__forceinline SampleVariant makeSampleS() {
    SampleVariant v;
    v._variant = 2;
    v.s_value = (char *)("3");
    return v;
}

__forceinline int32_t testCallLine ( das::LineInfoArg * arg ) { return arg ? arg->line : 0; }

void tableMojo ( das::TTable<char *,int> & in, const das::TBlock<void,das::TTable<char *,int>> & block, das::Context * context, das::LineInfoArg * lineinfo );

struct EntityId {
    int32_t value = 0;
    EntityId() : value(0) {}
    EntityId( const EntityId & t ) : value(t.value) {}
    EntityId & operator = ( const EntityId & t ) { value = t.value; return * this; }
    operator int32_t () const { return value; }
};

struct EntityId_WrapArg : EntityId {
    EntityId_WrapArg ( int32_t t ) { value = t; }
};

__forceinline EntityId make_EntityId(int32_t value) {
    EntityId t; t.value = value; return t;
}

namespace das {
    template <>
    struct cast<EntityId> {
        static __forceinline EntityId to ( vec4f x )            { return make_EntityId(v_extract_xi(v_cast_vec4i(x))); }
        static __forceinline vec4f from ( EntityId x )          { return v_cast_vec4f(v_seti_x(x.value)); }
    };
    template <> struct WrapType<EntityId> { enum { value = true }; typedef int32_t type; typedef int32_t rettype; };
    template <> struct WrapArgType<EntityId> { typedef EntityId_WrapArg type; };
}
__forceinline EntityId make_invalid_id() {
    return make_EntityId(-1);
}

__forceinline EntityId intToEid(int value) {
    return make_EntityId(value);
}

__forceinline int32_t eidToInt(EntityId id) {
    return id.value;
}

struct SceneNodeId
{
  typedef uint32_t handle_type_t;
  handle_type_t id;

  SceneNodeId() = default;
  SceneNodeId(handle_type_t i) : id(i) {}
  operator uint32_t() const { return id; }
};

SceneNodeId __create_scene_node();

namespace das {
    template <>
    struct cast<SceneNodeId> {
        static __forceinline SceneNodeId to ( vec4f x )            { return SceneNodeId{uint32_t(v_extract_xi(v_cast_vec4i(x)))}; }
        static __forceinline vec4f from ( SceneNodeId x )          { return v_cast_vec4f(v_seti_x(x.id)); }
    };
    template <> struct WrapType<SceneNodeId> { enum { value = true }; typedef uint32_t type; typedef uint32_t rettype; };
}

struct FancyClass {
    int32_t value;
    das::string hidden; // hidden property which makes it non-trivial
    FancyClass () : value(13) {}
    FancyClass ( int32_t a, int32_t b ) : value(a+b) {}
};

inline void deleteFancyClass(FancyClass& fclass) { fclass.~FancyClass(); }
inline void deleteFancyClassDummy(FancyClass& ) {  } // this one in AOT version, since local class will be deleted by dtor

void test_abi_lambda_and_function ( das::Lambda lambda, das::Func fn, int32_t lambdaSize, das::Context * context, das::LineInfoArg * lineinfo );

