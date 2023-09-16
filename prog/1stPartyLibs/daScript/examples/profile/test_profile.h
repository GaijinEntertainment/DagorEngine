#pragma once

#include "daScript/simulate/simulate.h"
#include "daScript/simulate/sim_policy.h"
#include "daScript/simulate/aot.h"

struct Object {
    das::float3   pos;
    das::float3   vel;

    __forceinline bool operator == (const Object & that) const {
        return pos == that.pos && vel == that.vel;
    }
    __forceinline bool operator != (const Object & that) const {
        return pos != that.pos || vel != that.vel;
    }

};

typedef das::vector<Object> ObjectArray;

namespace das {
    template<>
    struct SimPolicy<Object>
    {
        static __forceinline const Object &to ( vec4f a ) { return *(const Object *)v_extract_ptr(v_cast_vec4i(a)); }
        static __forceinline bool Equ     ( vec4f a, vec4f b, Context &, LineInfo * ) { return to(a) == to(b); }
        static __forceinline bool NotEqu  ( vec4f a, vec4f b, Context &, LineInfo * ) { return to(a) != to(b); }
    };
}

int AddOne(int a);

int testDict(das::Array & arr);
float testExpLoop(int count);
int testFibR(int n);
int testFibI(int n);
void testNBodiesInit();
void testNBodies(int32_t N);
void testNBodiesS(int32_t N);
void testParticles(int count);
void testParticlesI(int count);
int testPrimes(int n);
void testTryCatch(das::Context * context, das::LineInfoArg * at);
int testTree();
uint32_t testMaxFrom1s(uint32_t x);

void testManagedInt(const das::TBlock<void, const das::vector<int32_t>> & blk, das::Context * context, das::LineInfoArg * at);

void updateObject(Object & obj);
void updateTest(ObjectArray & objects);
void update10000(ObjectArray & objects, das::Context * context, das::LineInfoArg * at);
void update10000ks(ObjectArray & objects, das::Context * context, das::LineInfoArg * at);

// ES

struct EsAttribute {
    EsAttribute() = default;
    EsAttribute ( const das::string & n, uint32_t sz, bool rf, vec4f d, const char * ds = "" )
        : def(d), name(n), size(sz), ref(rf), def_s(ds) {}
    ~EsAttribute() { if(def_s) free((void *)def_s); }
    vec4f       def = v_zero();
    das::string name;
    uint32_t    size = 0;
    bool        ref;
    const char *def_s = nullptr;
};

struct EsAttributeTable {
    das::vector<EsAttribute> attributes;
};

struct EsPassAttributeTable  : EsAttributeTable {
    das::string pass;
    uint64_t    mangledNameHash = 0;
    int32_t     functionIndex = 0;
};

struct EsComponent {
    das::string      name;
    uint32_t    size = 0;
    uint32_t    stride = 0;
    bool        boxed = false;

    EsComponent() = default;
    EsComponent(const das::string & n, size_t sz, size_t st, bool bx) :
    name(n), size(uint32_t(sz)), stride(uint32_t(st)), boxed(bx) {}
};

constexpr int g_total = 100000;
DAS_THREAD_LOCAL extern das::vector<das::float3>   g_pos;
DAS_THREAD_LOCAL extern das::vector<das::float3>   g_vel;
DAS_THREAD_LOCAL extern das::vector<das::float3 *> g_velBoxed;
DAS_THREAD_LOCAL extern das::vector<EsComponent>   g_components;

void initEsComponents();
void initEsComponentsTable ();
void releaseEsComponents();
void verifyEsComponents(das::Context * ctx, das::LineInfoArg * at);
void testEsUpdate(char * pass, das::Context * ctx, das::LineInfoArg * at);
uint32_t queryEs(const das::Block & block, das::Context * context, das::LineInfoArg * at);


