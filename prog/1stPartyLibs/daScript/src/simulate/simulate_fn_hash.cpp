#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/simulate/hash.h"

namespace das {

#if 1
#define debug_hash(...)
#else
#define debug_hash  printf
#endif

#if 1
#define debug_aot_hash(...)
#else
#define debug_aot_hash  printf
#endif

    struct SimFnHashVisitor :  SimVisitor {
        Context * context = nullptr;
        SimFnHashVisitor ( Context * ctx )
            : context(ctx) {
        }
        // 64 bit FNV1a
        const uint64_t fnv_prime = 1099511628211ul;
        uint64_t offset_basis = 14695981039346656037ul;
        __forceinline void write ( const void * pb, uint32_t size ) {
            const uint8_t * block = (const uint8_t *) pb;
            while ( size-- ) {
                offset_basis = ( offset_basis ^ *block++ ) * fnv_prime;
            }
            debug_hash("%llx ", offset_basis);
        }
        __forceinline void write ( const void * pb ) {
            const uint8_t * block = (const uint8_t *) pb;
            for (; *block; block++) {
                offset_basis = ( offset_basis ^ *block ) * fnv_prime;
            }
            debug_hash("[%s] %llx ", (const char *) pb, offset_basis);
        }
        __forceinline uint64_t getHash() const  {
            return (offset_basis <= 1) ? fnv_prime : offset_basis;
        }
        // semantic walker
        virtual void op ( const char * name, uint32_t sz, const string & TT ) override {
            write(name);
            if ( sz ) write(&sz, sizeof(sz));
            if ( !TT.empty() ) write(TT.c_str());
        }
        virtual void sp ( uint32_t stackTop,  const char * op ) override {
            write(&stackTop, sizeof(stackTop));
            write(op);
        }
        virtual void arg ( Func, const char * mangledName, const char * argN ) override {
            write(mangledName);
            write(argN);
        }
        virtual void arg ( Func, uint32_t mangledNameHash, const char * argN ) override {
            write(&mangledNameHash, sizeof(mangledNameHash));
            write(argN);
        }
        virtual void arg ( int32_t argV,  const char * argN ) override {
            write(&argV,sizeof(argV));
            write(argN);
        }
        virtual void arg ( uint32_t argV,  const char * argN ) override {
            write(&argV,sizeof(argV));
            write(argN);
        }
        virtual void arg ( const char * argV,  const char * argN  ) override {
            if ( argV ) write(argV);
            write(argN);
        }
        virtual void arg ( vec4f argV,  const char * argN ) override {
            write(&argV,sizeof(argV));
            write(argN);
        }
        virtual void arg ( uint64_t argV,  const char * argN ) override {
            write(&argV,sizeof(argV));
            write(argN);
        }
        virtual void arg ( bool argV,  const char * argN ) override {
            write(&argV,sizeof(argV));
            write(argN);
        }
        virtual void arg ( float argV,  const char * argN ) override {
            write(&argV,sizeof(argV));
            write(argN);
        }
        virtual void arg ( double argV,  const char * argN ) override {
            write(&argV,sizeof(argV));
            write(argN);
        }
        virtual void sub ( SimNode ** nodes, uint32_t count, const char * argN ) override {
            write(&count, sizeof(count));
            write(argN);
            for ( uint32_t t=0; t!=count; ++t ) {
                nodes[t] = nodes[t]->visit(*this);
            }
        }
        virtual SimNode * sub ( SimNode * node, const char * opN ) override {
            write(opN);
            return SimVisitor::sub(node, opN);
        }
    };

    uint64_t getSemanticHash ( SimNode * node, Context * context ) {
        debug_hash("\n");
        SimFnHashVisitor hashV(context);
        node->visit(hashV);
        debug_hash("\n");
        return hashV.getHash();
    }

    uint64_t getFunctionHash ( Function * fun, SimNode * node, Context * context ) {
        debug_hash("\n%s\n", fun->name.c_str());
        SimFnHashVisitor hashV(context);
        // append return type and result type
        uint64_t resT = fun->result->getSemanticHash();
        hashV.write(&resT, sizeof(uint64_t));
        debug_hash("\nresult = %llx\n", resT);
        for ( auto & arg : fun->arguments ) {
            uint64_t argT = arg->type->getSemanticHash();
            hashV.write(&argT, sizeof(argT));
            debug_hash("arg %s = %llx\n", arg->name.c_str(), argT);
        }
        // append code
        node->visit(hashV);
        debug_hash("\n");
        if ( fun->aotHashDeppendsOnArguments ) {
            for ( auto & arg : fun->arguments ) {
                hashV.write(arg->name.c_str());
                if ( arg->init ) {
                    TextWriter tw;
                    tw << *(arg->init);
                    hashV.write(tw.str().c_str());
                }
            }
        }
        return hashV.getHash();
    }

    struct DependencyCollector {
        void collect ( const Function * fun ) {
            if ( functions.find(fun) != functions.end() ) return;
            functions.insert(fun);
            for ( const auto & depF : fun->useFunctions ) {
                collect(depF);
            }
            for ( const auto & depV : fun->useGlobalVariables ) {
                collect(depV);
            }
        }
        void collect ( const Variable * var ) {
            if ( variables.find(var) != variables.end() ) return;
            variables.insert(var);
            for ( const auto & depF : var->useFunctions ) {
                collect(depF);
            }
            for ( const auto & depV : var->useGlobalVariables ) {
                collect(depV);
            }
        }
        using FunAndName = pair<const Function *,string>;
        vector<FunAndName> getStableDependencies() {
            vector<FunAndName> vec;
            vec.reserve(functions.size());
            for ( const auto & it : functions ) {
                vec.emplace_back(make_pair(it,it->getMangledName()));
            }
            stable_sort(vec.begin(), vec.end(), [&](const FunAndName & a, const FunAndName & b){
                return a.second < b.second;
            });
            return vec;
        }
        using VarAndName = pair<const Variable *,string>;
        vector<VarAndName> getStableVariableDependencies() {
            vector<VarAndName> vec;
            vec.reserve(variables.size());
            for ( const auto & it : variables ) {
                vec.emplace_back(make_pair(it,it->getMangledName()));
            }
            stable_sort(vec.begin(), vec.end(), [&](const VarAndName & a, const VarAndName & b){
                return a.second < b.second;
            });
            return vec;
        }
        das_hash_set<const Function *> functions;
        das_hash_set<const Variable *> variables;
    };

    void collectDependencies ( FunctionPtr fun, const TBlock<void,TArray<Function *>,TArray<Variable *>> & block, Context * context, LineInfoArg * line ) {
        auto program = daScriptEnvironment::bound->g_Program;
        if ( !program ) context->throw_error_at(line, "Can't collect dependencies outside of compilation.");
        program->markExecutableSymbolUse();
        DependencyCollector collector;
        collector.collect(fun.get());
        auto vecFunc = collector.getStableDependencies();
        auto vecVars = collector.getStableVariableDependencies();
        vector<const Function *> tfun;
        tfun.reserve(vecFunc.size());
        for ( auto & fn : vecFunc ) {
            tfun.push_back(fn.first);
        }
        vector<const Variable *> tvar;
        tvar.reserve(vecVars.size());
        for ( auto & fn : vecVars ) {
            tvar.push_back(fn.first);
        }
        Array afun, avar;
        if ( tfun.size() ) {
            afun.data = (char *) tfun.data();
            afun.capacity = afun.size = uint32_t(tfun.size());
            afun.lock = 1;
            afun.flags = 0;
        }
        if ( tvar.size() ) {
            avar.data = (char *) tvar.data();
            avar.capacity = avar.size = uint32_t(tvar.size());
            avar.lock = 1;
            avar.flags = 0;
        }
        vec4f args[2];
        args[0] = cast<Array &>::from(afun);
        args[1] = cast<Array &>::from(avar);
        context->invoke(block, args, nullptr, line);
    }

    uint64_t getFunctionAotHash ( const Function * fun ) {
        DependencyCollector collector;
        collector.collect(fun);
        auto vec = collector.getStableDependencies();
        vector<uint64_t> uvec;
        uvec.reserve(vec.size() + 1);
        debug_aot_hash("HASH %s %llx\n", fun->getMangledName().c_str(), fun->hash);
        uvec.push_back(fun->hash);
        for ( const auto & fn : vec ) {
            if ( !fn.first->noAot &&!fn.first->builtIn ) {
                DAS_ASSERTF(fn.first->hash,"%s has dependency on %s, which hash hash of 0",
                    fun->getMangledName().c_str(), fn.first->getMangledName().c_str());
                uvec.push_back(fn.first->hash);
                debug_aot_hash("\t%s %llx\n", fn.second.c_str(), fn.first->hash);
            }
        }
        uint64_t res = hash_block64((const uint8_t *)uvec.data(), uint32_t(uvec.size()*sizeof(uint64_t)));
        debug_aot_hash("AOT HASH %llx\n", res);
        return res;
    }

    uint64_t getVariableListAotHash ( const vector<const Variable *> & globs, uint64_t initHash ) {
        DependencyCollector collector;
        for ( const auto & var : globs ) {
            collector.collect(var);
        }
        auto vec = collector.getStableDependencies();
        vector<uint64_t> uvec;
        uvec.reserve(vec.size() + 1);
        uvec.push_back(initHash);
        for ( const auto & fn : vec ) {
            if ( !fn.first->noAot ) {
                uvec.push_back(fn.first->hash);
            }
        }
        return hash_block64((const uint8_t *)uvec.data(), uint32_t(uvec.size()*sizeof(uint64_t)));
    }
}
