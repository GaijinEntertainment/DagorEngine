#include "daScript/misc/platform.h"

#include "daScript/simulate/simulate.h"
#include "daScript/simulate/aot.h"
#include "daScript/simulate/runtime_iterator.h"
#include "daScript/simulate/data_walker.h"

// The runtime half of a few of daslang's core builtins - the ones AOT-generated
// code calls that need only a context. Their registration stays in
// module_builtin_runtime.cpp, where the compiler is; this file has no compiler
// in it, which is what lets the minimal runtime (nano/) compile it.

namespace das {

    void builtin_throw ( char * text, Context * context, LineInfoArg * at ) {
        context->throw_error_at(at, "%s", text);
    }

    void builtin_print ( char * text, Context * context, LineInfoArg * at ) {
        context->to_out(at, text);
    }

    void builtin_feint ( char *, Context *, LineInfoArg * ) {
        // this function intentionally does nothing. its a fair replacement for the print, where we don't want print
    }


    struct LambdaIterator : Iterator {
        using lambdaFunc = bool (*) (Context *,void*, char*);
        LambdaIterator ( Context & context, const Lambda & ll, int st, LineInfo * at ) : Iterator(at), lambda(ll), stride(st) {
            SimFunction ** fnMnh = (SimFunction **) lambda.capture;
            if (!fnMnh) context.throw_error("invoke null lambda");
            simFunc = *fnMnh;
            if (!simFunc) context.throw_error("invoke null function");
            aotFunc = (lambdaFunc) simFunc->aotFunction;
        }

        DAS_SUPPRESS_UB
        __forceinline bool InvokeLambda ( Context & context, char * ptr ) {
            if ( aotFunc ) {
                return (*aotFunc) ( &context, lambda.capture, ptr );
            } else {
                vec4f argValues[4] = {
                    cast<Lambda>::from(lambda),
                    cast<char *>::from(ptr)
                };
                auto res = context.call(simFunc, argValues, 0);
                return cast<bool>::to(res);
            }
        }
        virtual bool first ( Context & context, char * ptr ) override {
            memset(ptr, 0, stride);
            return InvokeLambda(context, ptr);
        }
        virtual bool next  ( Context & context, char * ptr ) override {
            return InvokeLambda(context, ptr);
        }
        virtual void close ( Context & context, char * ) override {
            SimFunction ** fnMnh = (SimFunction **) lambda.capture;
            SimFunction * finFunc = fnMnh[1];
            if (!finFunc) context.throw_error("generator finalizer is a null function");
            vec4f argValues[1] = {
                cast<void *>::from(lambda.capture)
            };
            auto flags = context.stopFlags; // need to save stop flags, we can be in the middle of some return or something
            context.call(finFunc, argValues, 0);
            context.freeIterator((char *)this, debugInfo);
            context.stopFlags = flags;
        }
        virtual void walk ( DataWalker & walker ) override {
            walker.beforeLambda(&lambda, lambda.getTypeInfo());
            walker.walk(lambda.capture, lambda.getTypeInfo());
            walker.afterLambda(&lambda, lambda.getTypeInfo());
        }
        Lambda          lambda;
        SimFunction *   simFunc = nullptr;
        lambdaFunc      aotFunc = nullptr;
        int             stride = 0;
    };

    void builtin_make_lambda_iterator ( Sequence & result, const Lambda lambda, int stride, Context * context, LineInfoArg * at ) {
        char * iter = context->allocateIterator(sizeof(LambdaIterator), "lambda iterator", at);
        new (iter) LambdaIterator(*context, lambda, stride, at);
        result = { (Iterator *) iter };
    }

}
