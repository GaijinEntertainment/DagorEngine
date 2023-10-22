#include "daScript/misc/platform.h"

#include "test_profile.h"

#include "daScript/daScript.h"
#include "daScript/ast/ast_policy_types.h"

#if defined(_MSC_VER) && defined(__clang__)
#include <stdexcept>
#endif

#define FAST_PATH_ANNOTATION    1
#define FUNC_TO_QUERY           1

using namespace das;

void testManagedInt(const TBlock<void,const vector<int32_t>> & blk, Context * context, LineInfoArg * at) {
    vector<int32_t> arr;
    for (int32_t x = 0; x != 10; ++x) {
        arr.push_back(x);
    }
    vec4f args[1];
    args[0] = cast<vector<int32_t> *>::from(&arr);
    context->invoke(blk, args, nullptr, at);
}

___noinline void updateObject ( Object & obj ) {
    obj.pos.x += obj.vel.x;
    obj.pos.y += obj.vel.y;
    obj.pos.z += obj.vel.z;
}

void updateTest ( ObjectArray & objects ) {
    for ( auto & obj : objects ) {
        updateObject(obj);
    }
}

void update10000 ( ObjectArray & objects, Context * context, LineInfoArg * at ) {
    auto updateFn = context->findFunction("update");
    if (!updateFn) {
        context->throw_error_at(at, "update not exported");
        return;
    }
    for ( auto & obj : objects ) {
        vec4f args[1] = { cast<Object &>::from(obj) };
        context->eval(updateFn,  args);
    }
}

void update10000ks ( ObjectArray & objects, Context * context, LineInfoArg * at ) {
    auto ksUpdateFn = context->findFunction("ks_update");
    if (!ksUpdateFn) {
        context->throw_error_at(at, "ks_update not exported");
        return;
    }
    for ( auto & obj : objects ) {
        vec4f args[2] = { cast<float3 &>::from(obj.pos), cast<float3>::from(obj.vel) };
        context->eval(ksUpdateFn,  args);
    }
}

struct ObjectStructureTypeAnnotation : ManagedStructureAnnotation <Object> {
    ObjectStructureTypeAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("Object",ml) {
        addField<DAS_BIND_MANAGED_FIELD(pos)>("position","pos");
        addField<DAS_BIND_MANAGED_FIELD(vel)>("velocity","vel");
    }
};

MAKE_TYPE_FACTORY(Object, Object)

namespace das {
    template <>
    struct typeName<ObjectArray> {
        constexpr static const char * name() {
            return "ObjectArray";
        }
    };
};

namespace das {

	IMPLEMENT_OP2_EVAL_BOOL_POLICY(Equ, Object);
	IMPLEMENT_OP2_EVAL_BOOL_POLICY(NotEqu, Object);

}


___noinline int AddOne(int a) {
    return a+1;
}

// ES


struct EsGroupData : ModuleGroupUserData {
    EsGroupData() : ModuleGroupUserData("es") {
        THAT = this;
    }
    virtual ~EsGroupData() {
        THAT = nullptr;
    }
    vector<unique_ptr<EsPassAttributeTable>>    g_esPassTable;
    vector<unique_ptr<EsAttributeTable>>        g_esBlockTable;
    static EsGroupData * THAT;
};
EsGroupData * EsGroupData::THAT = nullptr;

struct EsFunctionAnnotation : FunctionAnnotation {
    EsFunctionAnnotation() : FunctionAnnotation("es") { }
    EsGroupData * getGroupData( ModuleGroup & group ) const {
        if ( auto data = group.getUserData("es") ) {
            return (EsGroupData *) data;
        }
        auto esData = new EsGroupData();
        group.setUserData(esData);
        return esData;
    }
    void buildAttributeTable ( EsAttributeTable & tab, const vector<VariablePtr> & arguments, string & err  ) {
        for ( const auto & arg : arguments ) {
            vec4f def = v_zero();
            const char * def_s = nullptr;
            if ( arg->init ) {
                if ( arg->init->rtti_isConstant() && !arg->init->rtti_isStringConstant() ) {
                    auto pConst = static_pointer_cast<ExprConst>(arg->init);
                    def = pConst->value;
                } else if ( arg->init->rtti_isConstant() && arg->init->rtti_isStringConstant() ) {
                    auto pConst = static_pointer_cast<ExprConstString>(arg->init);
                    def_s = strdup(pConst->text.c_str());
                    def = cast<char *>::from(def_s);
                } else {
                    err += "default for " + arg->name + " is not a constant\n";
                }
            }
            tab.attributes.emplace_back(arg->name, arg->type->getSizeOf(), arg->type->isRef(), def, def_s);
        }
    }
    virtual bool apply ( ExprBlock * block, ModuleGroup &, const AnnotationArgumentList &, string & err ) override {
        if ( block->arguments.empty() ) {
            err = "block needs arguments";
            return false;
        }
        return true;
    }
    virtual bool finalize ( ExprBlock * block, ModuleGroup & group, const AnnotationArgumentList &,
                           const AnnotationArgumentList &, string & err ) override {
        auto esData = getGroupData(group);
        auto tab = make_unique<EsAttributeTable>();
#if FAST_PATH_ANNOTATION
        block->aotSkipMakeBlock = true;
        block->aotDoNotSkipAnnotationData = true;
#endif
        block->annotationData = uint64_t(tab.get());
        auto mangledName = block->getMangledName(true,true);
        for (auto & ann : block->annotations) {
            if (ann->annotation.get() == this) {
                mangledName += " " + ann->getMangledName();
            }
        }
        block->annotationDataSid = hash_blockz64((uint8_t *)mangledName.c_str());
        buildAttributeTable(*tab, block->arguments, err);
        esData->g_esBlockTable.emplace_back(das::move(tab));
        return err.empty();
    }
    virtual bool apply ( const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList & args, string & err ) override {
        if ( func->arguments.empty() ) {
            err = "function needs arguments";
            return false;
        }
        func->exports = true;
#if FUNC_TO_QUERY
        // we are transforming function into query-call here
        // def funName // note, no more arguments
        //    testProfile::queryEs() <| $( clone of function arguments )
        //        clone of functoin body
        auto cle = make_smart<ExprCall>(func->at, "testProfile::queryEs");
        auto blk = make_smart<ExprBlock>();
        blk->at = func->at;
        blk->isClosure = true;
        for ( auto & arg : func->arguments ) {
            auto carg = arg->clone();
            blk->arguments.push_back(carg);
        }
        blk->returnType = make_smart<TypeDecl>(Type::tVoid);
        auto ann = make_smart<AnnotationDeclaration>();
        ann->annotation = this;
        ann->arguments = args;
        blk->annotations.push_back(ann);
        blk->list.push_back(func->body->clone());
        auto mkb = make_smart<ExprMakeBlock>(func->at, blk);
        cle->arguments.push_back(mkb);
        auto cleb = make_smart<ExprBlock>();
        cleb->at = func->at;
        cleb->list.push_back(cle);
        func->body = cleb;
        func->arguments.clear();
#endif
        return true;
    };
    virtual bool finalize ( const FunctionPtr & func, ModuleGroup & group, const AnnotationArgumentList & args,
                           const AnnotationArgumentList &, string & err ) override {
        if ( func->module->name.empty() ) {
            err = "es function needs to have explicit module name, i.e. 'module YOU_MODULE_NAME' on top of the file.";
            return false;
        }
        auto esData = getGroupData(group);
        auto tab = make_unique<EsPassAttributeTable>();
        if ( auto pp = args.find("es_pass", Type::tString) ) {
            tab->pass = pp->sValue;
        } else {
            err = "pass is not specified";
            return false;
        }
        tab->mangledNameHash = func->getMangledNameHash();
        buildAttributeTable(*tab, func->arguments, err);
        esData->g_esPassTable.emplace_back(das::move(tab));

        return err.empty();
    }
};

void * getComponentData ( const string & name ) {
    if (name == "pos") return g_pos.data();
    else if (name == "vel") return g_vel.data();
    else if (name == "velBoxed") return g_velBoxed.data();
    else return nullptr;
}

#if FUNC_TO_QUERY
bool EsRunPass ( Context & context, EsPassAttributeTable & table, const vector<EsComponent> &, uint32_t ) {
    auto functionPtr = context.fnByMangledName(table.mangledNameHash);
    context.call(functionPtr, nullptr, 0);
    return true;
}
#else
bool EsRunPass ( Context & context, EsPassAttributeTable & table, const vector<EsComponent> & components, uint32_t totalComponents ) {
    auto functionPtr = context.fnByMangledName(table.mangledNameHash);;
    vec4f * _args = (vec4f *)(alloca(table.attributes.size() * sizeof(vec4f)));
    context.callEx(functionPtr, _args, nullptr, 0, [&](SimNode * code){
        uint32_t nAttr = (uint32_t) table.attributes.size();
        vec4f * args = _args;
        char **        data    = (char **) alloca(nAttr * sizeof(char *));
        uint32_t *    stride    = (uint32_t *) alloca(nAttr * sizeof(uint32_t));
        uint32_t *  size    = (uint32_t *) alloca(nAttr * sizeof(uint32_t));
        bool *        boxed    = (bool *) alloca(nAttr * sizeof(bool));
        bool *      ref     = (bool *) alloca(nAttr * sizeof(bool));
        for ( uint32_t a=0; a!=nAttr; ++a ) {
            auto it = find_if ( components.begin(), components.end(), [&](const EsComponent & esc){
                return esc.name == table.attributes[a].name;
            });
            if ( it != components.end() ) {
                data[a]   = (char *) getComponentData(it->name);
                stride[a] = it->stride;
                boxed[a]  = it->boxed;
            } else {
                data[a] = nullptr;
                args[a] = table.attributes[a].def;
            }
            size[a] = table.attributes[a].size;
            ref[a] = table.attributes[a].ref;
        }
        for ( uint32_t i=0; i != totalComponents; ++i ) {
            for ( uint32_t a=0; a!=nAttr; ++a ) {
                if ( data[a] ) {
                    char * src =  boxed[a] ? *((char **)data[a]) : data[a];
                    if ( !ref[a] ) {
                        args[a] = v_ldu((float *)src);
                    } else {
                        *((void **)&args[a]) = src;
                    }
                    data[a] += stride[a];
                }
            }
            code->eval(context);
            context.stopFlags = 0;
            if ( context.getException() ) {
                // TODO: report exception here??
                return;
            }
        }
    });
    return true;
}
#endif

uint32_t EsRunBlock ( Context & context, LineInfo * at, const Block & block, const vector<EsComponent> & components, uint32_t totalComponents ) {
    auto * closure = (SimNode_ClosureBlock *) block.body;
    EsAttributeTable * table = (EsAttributeTable *) closure->annotationData;
    if ( !table ) {
        context.throw_error_at(at, "EsRunBlock - query missing annotation data");
    }
    uint32_t nAttr = (uint32_t) table->attributes.size();
    DAS_VERIFYF(nAttr, "EsRunBlock - query has no attributes");
    vec4f * _args = (vec4f *)(alloca(table->attributes.size() * sizeof(vec4f)));
    context.invokeEx(block, _args, nullptr, [&](SimNode * code){
        vec4f * args = _args;
        char **        data    = (char **) alloca(nAttr * sizeof(char *));
        uint32_t *    stride    = (uint32_t *) alloca(nAttr * sizeof(uint32_t));
        uint32_t *  size    = (uint32_t *) alloca(nAttr * sizeof(uint32_t));
        bool *        boxed    = (bool *) alloca(nAttr * sizeof(bool));
        bool *      ref     = (bool *) alloca(nAttr * sizeof(bool));
        for ( uint32_t a=0; a!=nAttr; ++a ) {
            auto it = find_if ( components.begin(), components.end(), [&](const EsComponent & esc){
                return esc.name == table->attributes[a].name;
            });
            if ( it != components.end() ) {
                data[a] = (char *) getComponentData(it->name);
                stride[a] = it->stride;
                boxed[a]  = it->boxed;
            } else {
                data[a] = nullptr;
                args[a] = table->attributes[a].def;
            }
            size[a] = table->attributes[a].size;
            ref[a] = table->attributes[a].ref;
        }
        for ( uint32_t i=0; i != totalComponents; ++i ) {
            for ( uint32_t a=0; a!=nAttr; ++a ) {
                if ( data[a] ) {
                    char * src =  boxed[a] ? *((char **)data[a]) : data[a];
                    if ( !ref[a] ) {
                        args[a] = v_ldu((float *)src);
                    } else {
                        *((void **)&args[a]) = src;
                    }
                    data[a] += stride[a];
                }
            }
            code->eval(context);
            context.stopFlags = 0;
            if ( context.getException() ) {
                // TODO: report exception here??
                return;
            }
        }
    }, nullptr); // TODO: line?
    return totalComponents;
}

string aotEsRunBlockName ( EsAttributeTable * table, const vector<EsComponent> & components ) {
    TextWriter nw;
    nw << "__query_es";
    uint32_t nAttr = (uint32_t) table->attributes.size();
    DAS_VERIFYF(nAttr, "EsRunBlock - query has no attributes");
    for ( uint32_t a=0; a!=nAttr; ++a ) {
        auto it = find_if ( components.begin(), components.end(), [&](const EsComponent & esc){
            return esc.name == table->attributes[a].name;
        });
        nw << "_" << table->attributes[a].name;
        if ( it != components.end() ) {
            nw << it->stride;
            if ( it->boxed ) {
                nw << "_b";
            }
        } else {
            nw << "_null";
        }
        nw << table->attributes[a].size;
        if ( table->attributes[a].ref ) {
            nw << "_r";
        }
    }
    return nw.str();
}

void aotEsRunBlock ( TextWriter & ss, EsAttributeTable * table, const vector<EsComponent> & components ) {
    auto fnName = aotEsRunBlockName(table, components);
    ss << "template<typename BT>\ninline uint32_t " << fnName << " ( uint64_t /*annotationData*/, const BT & block, Context * __context, LineInfoArg * __at )\n{\n";
    ss << "\tfor ( uint32_t i=0; i != g_total; ++i ) {\n";
    ss << "\t\tblock(";
    uint32_t nAttr = (uint32_t) table->attributes.size();
    DAS_VERIFYF(nAttr, "EsRunBlock - query has no attributes");
    for ( uint32_t a=0; a!=nAttr; ++a ) {
        auto it = find_if ( components.begin(), components.end(), [&](const EsComponent & esc){
            return esc.name == table->attributes[a].name;
        });
        if ( a ) ss << ",";
        if ( it != components.end() ) {
            ss << "g_" << table->attributes[a].name << "[i]";
        } else {
            vec4f def = table->attributes[a].def;
            const char * def_s = table->attributes[a].def_s;
            if (def_s) {
                ss << "\"" << def_s << "\"";
            } else if ( table->attributes[a].size==4 ) {
                ss << to_string_ex(v_extract_x(def)) << "f";
            } else {
                ss << "v_make_vec4f("
                    << to_string_ex(v_extract_x(def)) << "f," << to_string_ex(v_extract_y(def)) << "f,"
                    << to_string_ex(v_extract_z(def)) << "f," << to_string_ex(v_extract_w(def)) << "f)";
            }
        }
    }
    ss << ");\n";
    ss << "\t}\n";
    ss << "\treturn g_total;\n";
    ss << "}\n\n";
}

struct QueryEsFunctionAnnotation : FunctionAnnotation {
    QueryEsFunctionAnnotation() : FunctionAnnotation("query_es") { }

    virtual bool apply ( ExprBlock *, ModuleGroup &, const AnnotationArgumentList &, string & err ) override {
        err = "not a block annotation";
        return false;
    }
    virtual bool finalize ( ExprBlock *, ModuleGroup &, const AnnotationArgumentList &,
                           const AnnotationArgumentList &, string & err ) override {
        err = "not a block annotation";
        return false;
    }
    virtual bool apply ( const FunctionPtr &, ModuleGroup &, const AnnotationArgumentList &, string & ) override {
        return true;
    };
    virtual bool finalize ( const FunctionPtr &, ModuleGroup &, const AnnotationArgumentList &,
                           const AnnotationArgumentList &, string & ) override {
        return true;
    }
    virtual string aotName ( ExprCallFunc * call ) override {
        auto mb = static_pointer_cast<ExprMakeBlock>(call->arguments[0]);
        auto closure = static_pointer_cast<ExprBlock>(mb->block);
        EsAttributeTable * table = (EsAttributeTable *) closure->annotationData;
        return aotEsRunBlockName(table, g_components);
    }
    virtual void aotPrefix ( TextWriter & ss, ExprCallFunc * call ) override {
        auto mb = static_pointer_cast<ExprMakeBlock>(call->arguments[0]);
        auto closure = static_pointer_cast<ExprBlock>(mb->block);
        EsAttributeTable * table = (EsAttributeTable *) closure->annotationData;
        aotEsRunBlock(ss, table, g_components);
    }
    virtual bool verifyCall ( ExprCallFunc * call, const AnnotationArgumentList &, const AnnotationArgumentList &, string & err ) override {
        auto brt = call->arguments[0]->type->firstType;
        if ( !brt->isVoid() ) {
            err = "block can't return values";
            return false;
        }
        return true;
    }
    virtual ExpressionPtr transformCall ( ExprCallFunc * call, string & err ) override {
        if ( call->arguments.size()<2 || !call->arguments[0]->rtti_isMakeBlock() ) {
            err = "expecting make block, i.e <| $, got " + string(call->arguments[0]->__rtti) + " with " + to_string(call->arguments.size()) + " arguments";
            return nullptr;
        }
        auto mkb = static_pointer_cast<ExprMakeBlock>(call->arguments[0]);
        auto blk = static_pointer_cast<ExprBlock>(mkb->block);
        bool any = false;
        for ( auto & arg : blk->arguments ) {
            if ( !arg->can_shadow ) {
                arg->can_shadow = true;
                any = true;
            }
        }
        if ( any ) {
            return call->clone();
        } else {
            return nullptr;
        }
    }
};

DAS_THREAD_LOCAL vector<float3>          g_pos;
DAS_THREAD_LOCAL vector<float3>          g_vel;
DAS_THREAD_LOCAL vector<float3 *>        g_velBoxed;
DAS_THREAD_LOCAL vector<EsComponent>     g_components;

template <typename TT>
void releaseVec(vector<TT>& vec) {
    vector<TT> temp;
    vec.swap(temp);
}

void releaseEsComponents() {
    releaseVec(g_pos);
    releaseVec(g_vel);
    releaseVec(g_velBoxed);
}

void initEsComponents() {
    // build components
    g_pos.resize(g_total);
    g_vel.resize(g_total);
    g_velBoxed.resize(g_total);
    float f = 1.0f;
    for (int i = 0; i != g_total; ++i, ++f) {
        g_pos[i] = { f, f + 1, f + 2 };
        g_vel[i] = { 1.0f, 2.0f, 3.0f };
        g_velBoxed[i] = &g_vel[i];
    }
}

void initEsComponentsTable() {
    g_components.clear();
    g_components.emplace_back("pos",      sizeof(float3), sizeof(float3),   false);
    g_components.emplace_back("vel",      sizeof(float3), sizeof(float3),   false);
    g_components.emplace_back("velBoxed", sizeof(float3), sizeof(float3 *), true );
}

void verifyEsComponents(Context * context, LineInfoArg * at) {
    float f = 1.0f;
    float t = 1.0f;
    for ( int i = 0; i != g_total; ++i, ++f ) {
        float3 apos = { f, f + 1, f + 2 };
        float3 avel = { 1.0f, 2.0f, 3.0f };
        float3 npos;
        npos.x = apos.x + avel.x * t;
        npos.y = apos.y + avel.y * t;
        npos.z = apos.z + avel.z * t;
        if ( g_pos[i].x!=npos.x || g_pos[i].y!=npos.y || g_pos[i].z!=npos.z ) {
            TextWriter twrt;
            twrt << "g_pos[" << i << "] (" << g_pos[i] << ") != npos (" << npos << ")\n";
            context->throw_error_at(at, "verifyEsComponents failed, %s\n", twrt.str().c_str());
        }
    }
}


void testEsUpdate ( char * pass, Context * ctx, LineInfoArg * at ) {
    das_stack_prologue guard(ctx, sizeof(Prologue), "testEsUpdate " DAS_FILE_LINE);
    if ( !EsGroupData::THAT ) {
        ctx->throw_error_at(at, "missing pass data for the pass %s", pass);
        return;
    }
    for ( auto & tab : EsGroupData::THAT->g_esPassTable ) {
        if ( tab->pass==pass ) {
            EsRunPass(*ctx, *tab, g_components, g_total);
        }
    }
}

uint32_t queryEs (const Block & block, Context * context, LineInfoArg * at) {
    das_stack_prologue guard(context,sizeof(Prologue), "queryEs " DAS_FILE_LINE);
    return EsRunBlock(*context, at, block, g_components, g_total);
}

#if DAS_USE_EASTL
#include <EASTL/unordered_map.h>
#else
#include <unordered_map>
#endif

struct dictKeyEqual {
    __forceinline bool operator()( const char * lhs, const char * rhs ) const {
        return ( lhs==rhs ) ? true : strcmp(lhs,rhs)==0;
    }
};

struct dictKeyHash {
    __forceinline uint64_t operator () ( const char * str ) const {
        return str ? hash_blockz64((uint8_t *)str) : 1099511628211ul;
    }
};

typedef unordered_map<char *, int32_t, dictKeyHash, dictKeyEqual> dict_hash_map;

 int testDict(Array & arr) {
    dict_hash_map tab;
    char ** data = (char **) arr.data;
    int maxOcc = 0;
    for ( uint32_t t = 0; t !=arr.size; ++t ) {
        maxOcc = das::max(++tab[data[t]], maxOcc);
    }
    return maxOcc;
}

___noinline bool isprime(int n) {
    for (int i = 2; i != n; ++i) {
        if (n % i == 0) {
            return false;
        }
    }
    return true;
}

___noinline int testPrimes(int n) {
    int count = 0;
    for (int i = 2; i != n + 1; ++i) {
        if (isprime(i)) {
            ++count;
        }
    }
    return count;
}

___noinline int testFibR(int n) {
    if (n < 2) {
        return n;
    }
    else {
        return testFibR(n - 1) + testFibR(n - 2);
    }
}

___noinline int testFibI(int n) {
    int last = 0;
    int cur = 1;
    for (int i = 0; i != n - 1; ++i) {
        int tmp = cur;
        cur += last;
        last = tmp;
    }
    return cur;
}

___noinline void particles(ObjectArray & objects, int count) {
    for (int i = 0; i != count; ++i) {
        for (auto & obj : objects) {
            updateObject(obj);
        }
    }
}

___noinline void particlesI(ObjectArray & objects, int count) {
    for (int i = 0; i != count; ++i) {
        for (auto & obj : objects) {
            obj.pos.x += obj.vel.x;
            obj.pos.y += obj.vel.y;
            obj.pos.z += obj.vel.z;
        }
    }
}

___noinline void testParticles(int count) {
    ObjectArray objects;
    objects.resize(50000);
    particles(objects, count);
}

___noinline void testParticlesI(int count) {
    ObjectArray objects;
    objects.resize(50000);
    particlesI(objects, count);
}

___noinline void testTryCatch(Context * context, LineInfoArg * at) {
    #if _CPPUNWIND || __cpp_exceptions
    int arr[1000]; memset(arr, 0, sizeof(arr));
    int cnt = 0; cnt;
    for (int j = 0; j != 100; ++j) {
        int fail = 0;
        for (int i = 0; i != 2000; ++i) {
            try {
                if (i < 0 || i>=1000) throw "range check error";
                cnt += arr[i];
            }
            catch (...) {
                fail++;
            }
        }
        if (fail != 1000) {
            context->throw_error_at(at, "test optimized out");
            return;
        }
    }
    #endif
}

___noinline float testExpLoop(int count) {
    float ret = 0;
    for (int i = 0; i < count; ++i)
        ret += expf(1.f/(1.f+i));
    return ret;
}

#define pi 3.141592653589793f
#define solar_mass (4 * pi * pi)
#define days_per_year 365.24f

struct planet {
    float x, y, z; float pad;
    float vx, vy, vz; float mass;
};

void advance(int nbodies, struct planet * __restrict bodies, float dt)
{
    int i, j;

    for (i = 0; i < nbodies; i++) {
        struct planet * b = &(bodies[i]);
        for (j = i + 1; j < nbodies; j++) {
            struct planet * b2 = &(bodies[j]);
            float dx = b->x - b2->x;
            float dy = b->y - b2->y;
            float dz = b->z - b2->z;
            float distanced = dx * dx + dy * dy + dz * dz;
            float distance = sqrtf(distanced);
            float mag = dt / (distanced * distance);
            b->vx -= dx * b2->mass * mag;
            b->vy -= dy * b2->mass * mag;
            b->vz -= dz * b2->mass * mag;
            b2->vx += dx * b->mass * mag;
            b2->vy += dy * b->mass * mag;
            b2->vz += dz * b->mass * mag;
        }
    }
    for (i = 0; i < nbodies; i++) {
        struct planet * b = &(bodies[i]);
        b->x += dt * b->vx;
        b->y += dt * b->vy;
        b->z += dt * b->vz;
    }
}

void advancev(int nbodies, struct planet * __restrict bodies, float dt)
{
    vec4f vdt = v_splats(dt);
    struct planet *__restrict b = bodies, *be = bodies + nbodies;
    do {
        for (struct planet *__restrict b2 = b+1; b2 != be; ++b2) {
            vec4f dx = v_sub(v_ld(&b->x), v_ld(&b2->x));
            vec4f distanced = v_length3_sq_x(dx);
            vec4f distance = v_sqrt_x(distanced);
            vec4f mag = v_splat_x(v_div_x(vdt, v_mul_x(distanced, distance)));
            vec4f bv = v_ld(&b->vx), b2v = v_ld(&b2->vx);
            bv = v_sub(bv, v_mul(dx, v_mul(v_splat_w(b2v), mag)));//.w will be zero, since dx.w is zero
            b2v = v_add(b2v, v_mul(dx, v_mul(v_splat_w(bv), mag)));
            v_st(&b->vx, bv);
            v_st(&b2->vx, b2v);
        }
    } while(++b != be);

    for (struct planet *__restrict bB = bodies; bB != be; ++bB)
    {
        vec4f bx = v_ld(&bB->x), bv = v_ld(&b->vx);
        v_st(&bB->x, v_madd(vdt, bv, bx));
    }
}

float energy(int nbodies, struct planet * bodies)
{
    float e;
    int i, j;

    e = 0.0f;
    for (i = 0; i < nbodies; i++) {
        struct planet * b = &(bodies[i]);
        e += 0.5f * b->mass * (b->vx * b->vx + b->vy * b->vy + b->vz * b->vz);
        for (j = i + 1; j < nbodies; j++) {
            struct planet * b2 = &(bodies[j]);
            float dx = b->x - b2->x;
            float dy = b->y - b2->y;
            float dz = b->z - b2->z;
            float distance = sqrt(dx * dx + dy * dy + dz * dz);
            e -= (b->mass * b2->mass) / distance;
        }
    }
    return e;
}

void offset_momentum(int nbodies, struct planet * bodies)
{
    float px = 0.0f, py = 0.0f, pz = 0.0f;
    int i;
    for (i = 0; i < nbodies; i++) {
        px += bodies[i].vx * bodies[i].mass;
        py += bodies[i].vy * bodies[i].mass;
        pz += bodies[i].vz * bodies[i].mass;
    }
    bodies[0].vx = - px / solar_mass;
    bodies[0].vy = - py / solar_mass;
    bodies[0].vz = - pz / solar_mass;
}

#define NBODIES 5
struct planet bodies[NBODIES] = {
    {                               /* sun */
        0, 0, 0, 0, 0, 0, 0,solar_mass
    },
    {                               /* jupiter */
        4.84143144246472090e+00f,
        -1.16032004402742839e+00f,
        -1.03622044471123109e-01f,
        0.f,
        1.66007664274403694e-03f * days_per_year,
        7.69901118419740425e-03f * days_per_year,
        -6.90460016972063023e-05f * days_per_year,
        9.54791938424326609e-04f * solar_mass
    },
    {                               /* saturn */
        8.34336671824457987e+00f,
        4.12479856412430479e+00f,
        -4.03523417114321381e-01f,
        0.f,
        -2.76742510726862411e-03f * days_per_year,
        4.99852801234917238e-03f * days_per_year,
        2.30417297573763929e-05f * days_per_year,
        2.85885980666130812e-04f * solar_mass
    },
    {                               /* uranus */
        1.28943695621391310e+01f,
        -1.51111514016986312e+01f,
        -2.23307578892655734e-01f,
        0.f,
        2.96460137564761618e-03f * days_per_year,
        2.37847173959480950e-03f * days_per_year,
        -2.96589568540237556e-05f * days_per_year,
        4.36624404335156298e-05f * solar_mass
    },
    {                               /* neptune */
        1.53796971148509165e+01f,
        -2.59193146099879641e+01f,
        1.79258772950371181e-01f,
        0.f,
        2.68067772490389322e-03f * days_per_year,
        1.62824170038242295e-03f * days_per_year,
        -9.51592254519715870e-05f * days_per_year,
        5.15138902046611451e-05f * solar_mass
    }
};

void testNBodiesInit() {
    offset_momentum(NBODIES, bodies);
}

void testNBodies(int32_t N) {
    for (int32_t n = 0; n != N; ++n) {
        advancev(NBODIES, bodies, 0.01f);
    }
}

void testNBodiesS(int32_t N) {
    for (int32_t n = 0; n != N; ++n) {
        advance(NBODIES, bodies, 0.01f);
    }
}

// tree benchmark

class Tree {
public:
    Tree() = default;
    ~Tree() { delete mRoot; }
        bool hasValue(int x);
    void insert(int x);
    void erase(int x);
private:
    struct Node {
        Node(int x): x(x) {}
        Node() {}
        ~Node() {
            delete left;
            delete right;
        }
        int x = 0;
        int y = rand();
        Node* left = nullptr;
        Node* right = nullptr;
    };
    using NodePtr = Node*;
    static void merge(NodePtr lower, NodePtr greater, NodePtr& merged);
    static void merge(NodePtr lower, NodePtr equal, NodePtr greater, NodePtr& merged);
    static void split(NodePtr orig, NodePtr& lower, NodePtr& greaterOrEqual, int val);
    static void split(NodePtr orig, NodePtr& lower, NodePtr& equal, NodePtr& greater, int val);
    NodePtr mRoot = nullptr;
};

bool Tree::hasValue(int x) {
    NodePtr lower, equal, greater;
    split(mRoot, lower, equal, greater, x);
    bool res = equal != nullptr;
    merge(lower, equal, greater, mRoot);
    return res;
}

void Tree::insert(int x) {
    NodePtr lower, equal, greater;
    split(mRoot, lower, equal, greater, x);
    if( !equal ) equal = new Node(x);
    merge(lower, equal, greater, mRoot);
}

void Tree::erase(int x) {
    NodePtr lower, equal, greater;
    split(mRoot, lower, equal, greater, x);
    merge(lower, greater, mRoot);
    delete equal;
}

void Tree::merge(NodePtr lower, NodePtr greater, NodePtr& merged) {
    if ( !lower ) {
        merged = greater;
        return;
    }
    if ( !greater ) {
        merged = lower;
        return;
    }
    if ( lower->y < greater->y ) {
        merged = lower;
        merge(merged->right, greater, merged->right);
    } else {
        merged = greater;
        merge(lower, merged->left, merged->left);
    }
}

void Tree::merge(NodePtr lower, NodePtr equal, NodePtr greater, NodePtr& merged) {
    merge(lower, equal, merged);
    merge(merged, greater, merged);
}

void Tree::split(NodePtr orig, NodePtr& lower, NodePtr& greaterOrEqual, int val) {
    if ( !orig ) {
        lower = greaterOrEqual = nullptr;
        return;
    }
    if( orig->x < val ) {
        lower = orig;
        split(lower->right, lower->right, greaterOrEqual, val);
    } else {
        greaterOrEqual = orig;
        split(greaterOrEqual->left, lower, greaterOrEqual->left, val);
    }
}

void Tree::split(NodePtr orig, NodePtr& lower, NodePtr& equal, NodePtr& greater, int val) {
    NodePtr equalOrGreater;
    split(orig, lower, equalOrGreater, val);
    split(equalOrGreater, equal, greater, val + 1);
}

int testTree() {
    Tree tree;
    int cur = 5;
    int res = 0;
    for(int i = 1; i < 1000000; i++) {
        int mode = i % 3;
        cur = (cur * 57 + 43) % 10007;
        if ( mode == 0 ) {
            tree.insert(cur);
        } else if ( mode == 1 ) {
            tree.erase(cur);
        } else if ( mode == 2 ) {
            res += tree.hasValue(cur);
        }
    }
    return res;
}

uint32_t testMaxFrom1s(uint32_t x) {
    // from https://spiiin.github.io/blog/2385889062/
    uint32_t count1s = (uint32_t)das_popcount(x);
    uint32_t res = 0;
    for ( uint32_t i=0; i!=count1s; ++i) {
        res++;
        res<<=1;
    }
    for ( uint32_t i=0; i!=(31u - count1s); ++i) {
        res<<=1u;
    }
    return res;
}

class Module_TestProfile : public Module {
public:
    Module_TestProfile() : Module("testProfile") {
        ModuleLibrary lib(this);
        lib.addBuiltInModule();
        // function annotations
        addAnnotation(make_smart<EsFunctionAnnotation>());
        // register types
        addAnnotation(make_smart<ObjectStructureTypeAnnotation>(lib));
        addAnnotation(make_smart<ManagedVectorAnnotation<ObjectArray>>("ObjectArray",lib));
        addFunctionBasic<Object>(*this, lib);
        registerVectorFunctions<ObjectArray>::init(this, lib, true, true);
        // register functions
        addExtern<DAS_BIND_FUN(AddOne)>(*this,lib,"AddOne",SideEffects::none, "AddOne");
        addExtern<DAS_BIND_FUN(updateObject)>(*this,lib,"interopUpdate",SideEffects::modifyExternal,"updateObject");
        addExtern<DAS_BIND_FUN(updateTest)>(*this,lib,"interopUpdateTest",SideEffects::modifyExternal,"updateTest");
        addExtern<DAS_BIND_FUN(update10000)>(*this,lib,"update10000",SideEffects::modifyExternal,"update10000");
        addExtern<DAS_BIND_FUN(update10000ks)>(*this,lib,"update10000ks",SideEffects::modifyExternal,"update10000ks");
        addExtern<DAS_BIND_FUN(testManagedInt)>(*this,lib,"testManagedInt",SideEffects::modifyExternal,"testManagedInt");
        // es
        initEsComponentsTable();
        auto qes_annotation = make_smart<QueryEsFunctionAnnotation>();
        addAnnotation(qes_annotation);
        auto queryEsFn = addExtern<DAS_BIND_FUN(queryEs)>(*this, lib, "queryEs",SideEffects::modifyExternal,"queryEs");
#if FAST_PATH_ANNOTATION
        auto qes_decl = make_smart<AnnotationDeclaration>();
        qes_decl->annotation = qes_annotation;
        queryEsFn->annotations.push_back(qes_decl);
#endif
        addExtern<DAS_BIND_FUN(testEsUpdate)>(*this, lib, "testEsUpdate",SideEffects::modifyExternal,"testEsUpdate");
        addExtern<DAS_BIND_FUN(initEsComponents)>(*this, lib, "initEsComponents",SideEffects::modifyExternal,"initEsComponents");
        addExtern<DAS_BIND_FUN(releaseEsComponents)>(*this, lib, "releaseEsComponents", SideEffects::modifyExternal, "releaseEsComponents");
        addExtern<DAS_BIND_FUN(verifyEsComponents)>(*this, lib, "verifyEsComponents",SideEffects::modifyExternal,"verifyEsComponents");
        // C++ copy of all tests
        addExtern<DAS_BIND_FUN(testPrimes)>(*this, lib, "testPrimes",SideEffects::modifyExternal,"testPrimes");
        addExtern<DAS_BIND_FUN(testDict)>(*this, lib, "testDict",SideEffects::modifyExternal,"testDict");
        addExtern<DAS_BIND_FUN(testFibR)>(*this, lib, "testFibR",SideEffects::modifyExternal,"testFibR");
        addExtern<DAS_BIND_FUN(testFibI)>(*this, lib, "testFibI",SideEffects::modifyExternal,"testFibI");
        addExtern<DAS_BIND_FUN(testParticles)>(*this, lib, "testParticles",SideEffects::modifyExternal,"testParticles");
        addExtern<DAS_BIND_FUN(testParticlesI)>(*this, lib, "testParticlesI",SideEffects::modifyExternal,"testParticlesI");
        addExtern<DAS_BIND_FUN(testTryCatch)>(*this, lib, "testTryCatch",SideEffects::modifyExternal,"testTryCatch");
        addExtern<DAS_BIND_FUN(testExpLoop)>(*this, lib, "testExpLoop",SideEffects::modifyExternal,"testExpLoop");
        addExtern<DAS_BIND_FUN(testNBodiesInit)>(*this, lib, "testNBodiesInit",SideEffects::modifyExternal,"testNBodiesInit");
        addExtern<DAS_BIND_FUN(testNBodies)>(*this, lib, "testNBodies",SideEffects::modifyExternal,"testNBodies");
        addExtern<DAS_BIND_FUN(testNBodiesS)>(*this, lib, "testNBodiesS",SideEffects::modifyExternal,"testNBodiesS");
        addExtern<DAS_BIND_FUN(testTree)>(*this, lib, "testTree",SideEffects::modifyExternal,"testTree");
        addExtern<DAS_BIND_FUN(testMaxFrom1s)>(*this, lib, "testMaxFrom1s",SideEffects::modifyExternal,"testMaxFrom1s");
        // its AOT ready
        verifyAotReady();
    }
    virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
        tw << "#include \"test_profile.h\"\n";
        return ModuleAotType::cpp;
    }
};

REGISTER_MODULE(Module_TestProfile);


