#include "daScript/misc/platform.h"

#include "daScript/simulate/simulate.h"
#include "daScript/ast/ast.h"

namespace das {

    struct SimPrint : SimVisitor {
        Context * context = nullptr;
        bool displayHash = true;
        SimPrint ( TextWriter & wr, Context * ctx ) : context(ctx), ss(wr) {
        }
        // 64 bit FNV1a
        const uint64_t fnv_prime = 1099511628211ul;
        uint64_t offset_basis = 14695981039346656037ul;
        __forceinline void write ( const void * pb, uint32_t size ) {
            const uint8_t * block = (const uint8_t *) pb;
            while ( size-- ) {
                offset_basis = ( offset_basis ^ *block++ ) * fnv_prime;
            }
            if ( displayHash ) {
                ss << " " << HEX << offset_basis << DEC << " ";
            }
        }
        __forceinline void write ( const void * pb ) {
            const uint8_t * block = (const uint8_t *) pb;
            for (; *block; block++) {
                offset_basis = ( offset_basis ^ *block ) * fnv_prime;
            }
            if ( displayHash ) {
                ss << " " <<  HEX << offset_basis << DEC << " ";
            }
        }
        __forceinline uint64_t getHash() const  {
            return (offset_basis <= 1) ? fnv_prime : offset_basis;
        }
        // regular print
        void crlf() {
            if ( CR ) {
                ss << "\n" << string(tab,'\t');
            } else {
                ss << " ";
            }
        }
        virtual void preVisit ( SimNode * node ) override {
            xcr.push_back(CR);
            CR = false;
            tab ++;
            SimVisitor::preVisit(node);
            ss << "(";
        }
        virtual SimNode * visit ( SimNode * node ) override {
            ss << ")";
            tab --;
            CR = xcr.back();
            xcr.pop_back();
            return SimVisitor::visit(node);
        }
        virtual void cr () override {
            CR = true;
        }
        virtual void op ( const char * name, uint32_t sz, const string & TT ) override {
            SimVisitor::op(name);
            // hash
            write(name);
            if ( sz ) write(&sz, sizeof(sz));
            if ( !TT.empty() ) write(TT.c_str());
            // regular print
            ss << name;
            if ( !TT.empty() ) {
                ss << "_TT<" << TT << ">";
            } else if ( sz ) {
                ss << "_TT<(" << sz << ")>";
            }
        }
        virtual void sp ( uint32_t stackTop, const char * op ) override {
            SimVisitor::sp(stackTop,op);
            // hash
            write(&stackTop, sizeof(stackTop));
            write(op);
            // regular print
            crlf();
            ss << "#" << stackTop;
        }
        virtual void arg ( Func fun,  const char * mangledName, const char * argN ) override {
            SimVisitor::arg(fun, mangledName, argN);
            // hash
            write(mangledName);
            write(argN);
            // regular print
            crlf();
            ss << "@@" << mangledName;
        }
        virtual void arg ( Func fun,  uint32_t mangledNameHash, const char * argN ) override {
            SimVisitor::arg(fun, mangledNameHash, argN);
            // hash
            write(&mangledNameHash, sizeof(mangledNameHash));
            write(argN);
            // regular print
            crlf();
            if ( context ) {
                SimFunction * fn = nullptr;
                if ( strcmp(argN,"mnh")==0 ) {
                    fn = context->fnByMangledName(mangledNameHash);
                } else {
                    fn = context->getFunction(mangledNameHash);
                }
                if ( fn ) {
                    ss << "@@" << fn->name;
                } else {
                    ss << "@@null";
                }
            }
            ss << " (" << argN << "=" << mangledNameHash << ")";
        }
        virtual void arg ( int32_t argV,  const char * argN ) override {
            SimVisitor::arg(argV,argN);
            // hash
            write(&argV,sizeof(argV));
            write(argN);
            // regular print
            crlf();
            ss << argV;
        }
        virtual void arg ( uint32_t argV,  const char * argN ) override {
            SimVisitor::arg(argV,argN);
            // hash
            write(&argV,sizeof(argV));
            write(argN);
            // regular print
            crlf();
            ss << "0x" << HEX << argV << DEC;
        }
        virtual void arg ( const char * argV,  const char * argN ) override {
            SimVisitor::arg(argV,argN);
            // hash
            if ( argV ) write(argV);
            write(argN);
            // regular print
            crlf();
            if ( argV ) {
                ss << "\"" << escapeString(argV) << "\"";
            } else {
                ss << "null";
            }
        }
        virtual void arg ( vec4f argV,  const char * argN ) override {
            SimVisitor::arg(argV,argN);
            // hash
            write(&argV,sizeof(argV));
            write(argN);
            // regular print
            union {
                uint32_t    ui[4];
                vec4f       v;
            } X; X.v = argV;
            crlf();
            ss << "{" << X.ui[0] << "," << X.ui[1] << "," << X.ui[2] << "," << X.ui[3] << "}";
        }
        virtual void arg ( int64_t argV,  const char * argN ) override {
            SimVisitor::arg(argV,argN);
            crlf();
            ss << argV;
        }
        virtual void arg ( uint64_t argV,  const char * argN ) override {
            SimVisitor::arg(argV,argN);
            // hash
            write(&argV,sizeof(argV));
            write(argN);
            // regular print
            crlf();
            ss << "0x" << HEX << argV << DEC;
        }
        virtual void arg ( bool argV,  const char * argN ) override {
            SimVisitor::arg(argV,argN);
            // hash
            write(&argV,sizeof(argV));
            write(argN);
            // regular print
            crlf();
            ss << (argV ? "true" : "false");
        }
        virtual void arg ( float argV,  const char * argN ) override {
            SimVisitor::arg(argV,argN);
            // hash
            write(&argV,sizeof(argV));
            write(argN);
            // regular print
            crlf();
            ss << argV;
        }
        virtual void arg ( double argV,  const char * argN ) override {
            SimVisitor::arg(argV,argN);
            // hash
            write(&argV,sizeof(argV));
            write(argN);
            // regular print
            crlf();
            ss << argV;
        }
        virtual void sub ( SimNode ** nodes, uint32_t count, const char * argN ) override {
            // mixed hash and regular print
            write(&count, sizeof(count));
            write(argN);
            crlf();
            for ( uint32_t t = 0; t!=count; ++t ) {
                if ( t ) crlf();
                nodes[t] = nodes[t]->visit(*this);
            }
        }
        virtual SimNode * sub ( SimNode * node, const char * opN ) override {
            // hash
            write(opN);
            // regular print
            crlf();
            return SimVisitor::sub(node, opN);
        }
        TextWriter & ss;
        int tab = 0;
        bool CR = false;
        vector<bool> xcr;
    };

    void printSimFunction ( TextWriter & ss, Context * context, Function * fun, SimNode * node, bool debugHash ) {
        SimPrint prv(ss,context);
        prv.displayHash = debugHash;
        // append return type and result type
        if ( debugHash ) {
            ss << "// " << fun->name << " " << fun->getMangledName() << "\n";
        }
        string resT = fun->result->describe();
        if ( debugHash ) {
            ss << "(result " << resT << " ";
        }
        prv.write(resT.c_str(), uint32_t(resT.length()));
        if ( debugHash ) {
            ss << ")\n";
        }
        for ( auto & arg : fun->arguments ) {
            string argT = arg->type->describe();
            if ( debugHash ) {
                ss << "(argument " << argT << " ";
            }
            prv.write(argT.c_str(), uint32_t(argT.length()));
            if ( debugHash ) {
                ss << ")\n";
            }
        }
        node->visit(prv);
    }

    void printSimNode ( TextWriter & ss, Context * context, SimNode * node, bool debugHash ) {
        SimPrint prv(ss,context);
        prv.displayHash = debugHash;
        node->visit(prv);
    }
}
