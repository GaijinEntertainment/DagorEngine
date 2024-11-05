#include "daScript/misc/platform.h"

#include "daScript/simulate/simulate.h"
#include "daScript/simulate/simulate_nodes.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4611)
#endif

/*
    NOTE: this should be the only file where daScript needs exceptions enabled.
*/

// Workaround for https://github.com/google/sanitizers/issues/749
#if DAS_ENABLE_EXCEPTIONS && defined(__clang__) && defined(_WIN32) && defined(__has_feature)
#if __has_feature(address_sanitizer)
#define WIN_EH_NO_ASAN __attribute__((no_sanitize_address))
#endif
#endif
#ifndef WIN_EH_NO_ASAN
#define WIN_EH_NO_ASAN
#endif

namespace das {
    static void stackWalkToErr(Context & ctx, const LineInfo & at, bool showArguments, bool showLocalVariables) {
        const das::string stack = ctx.getStackWalk(&at, showArguments, showLocalVariables);
        ctx.to_err(&at, stack.c_str());
    }

    void Context::throw_fatal_error ( const char * message, const LineInfo & at ) {
        constexpr bool SHOW_ARGUMENTS = false;
        constexpr bool SHOW_LOCAL_VARIABLES = false;
        exceptionMessage = message ? message : "";
        exceptionMessage += "\n";
        exception = exceptionMessage.c_str();
        exceptionAt = at;
#if DAS_ENABLE_EXCEPTIONS
        if ( alwaysErrorOnException ) {
            to_err(&at, exception);
        }
        if ( alwaysStackWalkOnException ) {
            stackWalkToErr(*this, at, SHOW_ARGUMENTS, SHOW_LOCAL_VARIABLES);
        }
        if ( breakOnException ) breakPoint(at, "exception", exception);
        throw dasException(exception, at);
#else
        if ( throwBuf ) {
            if ( alwaysErrorOnException ) {
                to_err(&at, exception);
            }
            if ( alwaysStackWalkOnException ) {
                stackWalkToErr(*this, at, SHOW_ARGUMENTS, SHOW_LOCAL_VARIABLES);
            }
            if ( breakOnException ) breakPoint(at, "exception", exception);
#if defined(WIN64) || defined(_WIN64)
            //  "An invalid or unaligned stack was encountered during an unwind operation." exception is issued via longjmp
            //  this is a known issue with longjmp on x64, and this workaround disables stack unwinding
            ((_JUMP_BUFFER *)throwBuf)->Frame = 0;
#endif
            longjmp(*throwBuf,1);
        } else {
            to_err(&at, "\nunhandled exception\n");
            string msg = exceptionAt.describe() + ": " + exception;
            to_err(&at, msg.c_str());
            stackWalkToErr(*this, at, SHOW_ARGUMENTS, SHOW_LOCAL_VARIABLES);
            breakPoint(at, "exception", exception);
        }
#endif
#if !defined(_MSC_VER) || (_MSC_VER>1900)
        exit(0);
#endif
    }

    void Context::rethrow () {
#if DAS_ENABLE_EXCEPTIONS
        throw dasException(exception ? exception : "", exceptionAt);
#else
        if ( throwBuf ) {
#if defined(WIN64) || defined(_WIN64)
            //  "An invalid or unaligned stack was encountered during an unwind operation." exception is issued via longjmp
            //  this is a known issue with longjmp on x64, and this workaround disables stack unwinding
            ((_JUMP_BUFFER *)throwBuf)->Frame = 0;
#endif

            longjmp(*throwBuf,1);
        } else {
            to_err(nullptr, "\nunhandled exception\n");
            if ( exception ) {
                string msg = exceptionAt.describe() + ": " + exception;
                to_err(nullptr, msg.c_str());
                to_err(nullptr, "\n");
            }
            stackWalk(nullptr, false, false);
            os_debug_break();
        }
#endif
#if !defined(_MSC_VER) || (_MSC_VER>1900)
        exit(0);
#endif
    }

    vec4f WIN_EH_NO_ASAN Context::evalWithCatch ( SimNode * node ) {
        auto aa = abiArg;
        auto acm = abiCMRES;
        auto atba = abiThisBlockArg;
        char * EP, * SP;
        stack.watermark(EP,SP);
        vec4f vres = v_zero();
#if DAS_ENABLE_EXCEPTIONS
        try {
            vres = node->eval(*this);
        } catch ( const dasException & ex ) {
            abiArg = aa;
            abiCMRES = acm;
            abiThisBlockArg = atba;
            stack.pop(EP,SP);
            exceptionMessage = ex.what();
            exception = exceptionMessage.c_str();
            exceptionAt = ex.exceptionAt;
        }
#else
        jmp_buf ev;
        jmp_buf * JB = throwBuf;
        throwBuf = &ev;
        if ( !setjmp(ev) ) {
            vres = node->eval(*this);
        } else {
            abiArg = aa;
            abiCMRES = acm;
            abiThisBlockArg = atba;
            stack.pop(EP,SP);
        }
        throwBuf = JB;
#endif
        return vres;
    }

    bool WIN_EH_NO_ASAN Context::runWithCatch ( const callable<void()> & subexpr ) {
        auto aa = abiArg;
        auto acm = abiCMRES;
        auto atba = abiThisBlockArg;
        char * EP, * SP;
        stack.watermark(EP,SP);
        bool bres = false;
#if DAS_ENABLE_EXCEPTIONS
        try {
            subexpr();
            bres = true;
        } catch ( const dasException & ex ) {
            abiArg = aa;
            abiCMRES = acm;
            abiThisBlockArg = atba;
            stack.pop(EP,SP);
            exceptionMessage = ex.what();
            exception = exceptionMessage.c_str();
            exceptionAt = ex.exceptionAt;
        }
#else
        jmp_buf ev;
        jmp_buf * JB = throwBuf;
        throwBuf = &ev;
        if ( !setjmp(ev) ) {
            subexpr();
            bres = true;
        } else {
            abiArg = aa;
            abiCMRES = acm;
            abiThisBlockArg = atba;
            stack.pop(EP,SP);
        }
        throwBuf = JB;
#endif
        return bres;
    }

    vec4f WIN_EH_NO_ASAN Context::evalWithCatch ( SimFunction * fnPtr, vec4f * args, void * res ) {
        auto aa = abiArg;
        auto acm = abiCMRES;
        auto atba = abiThisBlockArg;
        char * EP, * SP;
        stack.watermark(EP,SP);
        vec4f vres = v_zero();
#if DAS_ENABLE_EXCEPTIONS
        try {
            vres = callWithCopyOnReturn(fnPtr, args, res, 0);
        } catch ( const dasException & ex ) {
            abiArg = aa;
            abiCMRES = acm;
            abiThisBlockArg = atba;
            stack.pop(EP,SP);
            exceptionMessage = ex.what();
            exception = exceptionMessage.c_str();
            exceptionAt = ex.exceptionAt;
        }
#else
        jmp_buf ev;
        jmp_buf * JB = throwBuf;
        throwBuf = &ev;
        if ( !setjmp(ev) ) {
            vres = callWithCopyOnReturn(fnPtr, args, res, 0);
        } else {
            abiArg = aa;
            abiCMRES = acm;
            abiThisBlockArg = atba;
            stack.pop(EP,SP);
        }
        throwBuf = JB;
#endif
        return vres;
    }

    // SimNode_TryCatch

    vec4f WIN_EH_NO_ASAN SimNode_TryCatch::eval ( Context & context ) {
        DAS_PROFILE_NODE
        auto aa = context.abiArg; auto acm = context.abiCMRES;
        char * EP, * SP;
        context.stack.watermark(EP,SP);
        #if DAS_ENABLE_EXCEPTIONS
            try {
                try_block->eval(context);
            } catch ( const dasException & ) {
                context.abiArg = aa;
                context.abiCMRES = acm;
                context.stack.pop(EP,SP);
                context.stopFlags = 0;
                context.last_exception = context.exception;
                context.exception = nullptr;
                catch_block->eval(context);
            }
        #else
            jmp_buf ev;
            jmp_buf * JB = context.throwBuf;
            context.throwBuf = &ev;
            if ( !setjmp(ev) ) {
                try_block->eval(context);
            } else {
                context.throwBuf = JB;
                context.abiArg = aa;
                context.abiCMRES = acm;
                context.stack.pop(EP,SP);
                context.stopFlags = 0;
                context.last_exception = context.exception;
                context.exception = nullptr;
                catch_block->eval(context);
            }
            context.throwBuf = JB;
        #endif
        return v_zero();
    }

#if DAS_DEBUGGER
    vec4f WIN_EH_NO_ASAN SimNodeDebug_TryCatch::eval ( Context & context ) {
        DAS_PROFILE_NODE
        auto aa = context.abiArg; auto acm = context.abiCMRES;
        char * EP, * SP;
        context.stack.watermark(EP,SP);
        #if DAS_ENABLE_EXCEPTIONS
            try {
                DAS_SINGLE_STEP(context,try_block->debugInfo,false);
                try_block->eval(context);
            } catch ( const dasException & ) {
                context.abiArg = aa;
                context.abiCMRES = acm;
                context.stack.pop(EP,SP);
                context.stopFlags = 0;
                context.last_exception = context.exception;
                context.exception = nullptr;
                DAS_SINGLE_STEP(context,catch_block->debugInfo,false);
                catch_block->eval(context);
            }
        #else
            jmp_buf ev;
            jmp_buf * JB = context.throwBuf;
            context.throwBuf = &ev;
            if ( !setjmp(ev) ) {
                DAS_SINGLE_STEP(context,try_block->debugInfo,false);
                try_block->eval(context);
            } else {
                context.throwBuf = JB;
                context.abiArg = aa;
                context.abiCMRES = acm;
                context.stack.pop(EP,SP);
                context.stopFlags = 0;
                context.last_exception = context.exception;
                context.exception = nullptr;
                DAS_SINGLE_STEP(context,catch_block->debugInfo,false);
                catch_block->eval(context);
            }
            context.throwBuf = JB;
        #endif
        return v_zero();
    }
#endif

    void WIN_EH_NO_ASAN das_try_recover ( Context * __context__, const callable<void()> & try_block, const callable<void()> & catch_block ) {
        auto aa = __context__->abiArg; auto acm = __context__->abiCMRES;
        char * EP, * SP;
        __context__->stack.watermark(EP,SP);
#if DAS_ENABLE_EXCEPTIONS
        try {
            try_block();
        } catch ( const dasException & ) {
            catch_block();
            __context__->abiArg = aa;
            __context__->abiCMRES = acm;
            __context__->stack.pop(EP,SP);
            __context__->stopFlags = 0;
            __context__->last_exception = __context__->exception;
            __context__->exception = nullptr;
        }
#else
        jmp_buf ev;
        jmp_buf * JB = __context__->throwBuf;
        __context__->throwBuf = &ev;
        if ( !setjmp(ev) ) {
            try_block();
        } else {
            catch_block();
            __context__->throwBuf = JB;
            __context__->abiArg = aa;
            __context__->abiCMRES = acm;
            __context__->stack.pop(EP,SP);
            __context__->stopFlags = 0;
            __context__->last_exception = __context__->exception;
            __context__->exception = nullptr;
        }
        __context__->throwBuf = JB;
#endif
    }

    void WIN_EH_NO_ASAN builtin_try_recover ( const Block & try_block, const Block & catch_block, Context * context, LineInfoArg * at ) {
        auto aa = context->abiArg; auto acm = context->abiCMRES;
        char * EP, * SP;
        context->stack.watermark(EP,SP);
        #if DAS_ENABLE_EXCEPTIONS
            try {
                context->invoke(try_block, nullptr, nullptr, at);
            } catch ( const dasException & ) {
                context->abiArg = aa;
                context->abiCMRES = acm;
                context->stack.pop(EP,SP);
                context->stopFlags = 0;
                context->last_exception = context->exception;
                context->exception = nullptr;
                context->invoke(catch_block,nullptr,nullptr, at);
            }
        #else
            jmp_buf ev;
            jmp_buf * JB = context->throwBuf;
            context->throwBuf = &ev;
            if ( !setjmp(ev) ) {
                context->invoke(try_block,nullptr,nullptr, at);
            } else {
                context->throwBuf = JB;
                context->abiArg = aa;
                context->abiCMRES = acm;
                context->stack.pop(EP,SP);
                context->stopFlags = 0;
                context->last_exception = context->exception;
                context->exception = nullptr;
                context->invoke(catch_block,nullptr,nullptr, at);
            }
            context->throwBuf = JB;
        #endif
    }
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
