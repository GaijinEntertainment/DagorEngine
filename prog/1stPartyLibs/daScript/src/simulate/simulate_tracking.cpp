#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_expressions.h"

namespace das
{
#if DAS_ENABLE_SMART_PTR_TRACKING
    vector<smart_ptr<ptr_ref_count>> Context::sptrAllocations;
#endif

    string around ( const LineInfo & info ) {
        TextWriter tw;
        tw << ":" << info.line << ":" << info.column;
        return tw.str();
    }

#if DAS_ENABLE_SMART_PTR_TRACKING
    void safeDumpType ( TextWriter & tp, const TypeDeclPtr & td ) {
        if ( td->baseType==Type::tStructure ) {
            tp << "structure";
        } else if ( td->baseType==Type::tHandle ) {
            tp << "handle";
        } else if ( td->baseType==Type::tPointer ) {
            if ( td->firstType ) {
                safeDumpType(tp,td->firstType);
                tp << "?";
            } else {
                tp << "void?";
            }
        } else if ( td->baseType==Type::tTuple || td->baseType==Type::tVariant ) {
            tp << das_to_string(td->baseType) << "<";
            for ( auto & st : td->argTypes ) {
                safeDumpType(tp, st);
                if ( st != td->argTypes.back() ) tp << ";";
            }
            tp << ">";
        } else {
            tp << das_to_string(td->baseType);
        }
        for ( auto i : td->dim ) {
            tp << "[" << i << "]";
        }
        if ( td->constant ) tp << " const";
        if ( td->ref ) tp << " &";
        if ( td->temporary ) tp << " #";
    }
#endif

    void dumpTrackingLeaks ( ) {
        LOG tp(LogLevel::debug);
#if DAS_ENABLE_SMART_PTR_TRACKING
        bool any = true;
        tp << "cleaning up tracking...\n";
        while ( any ) {
            any = false;
            for ( auto it = Context::sptrAllocations.begin(); it!=Context::sptrAllocations.end(); ) {
                auto & ptr = *it;
                if ( ptr->use_count()==1 ) {
                    it = Context::sptrAllocations.erase(it);
                    any = true;
                } else {
                    ++it;
                }
            }
        }
        int total = 0;
        for ( auto & ptr : Context::sptrAllocations ) {
            tp << "0x" << HEX << intptr_t(ptr.get()) << DEC << " (" << ptr->use_count() << ")";
#if DAS_SMART_PTR_ID
            tp << " id=" << HEX << ptr->ref_count_id << DEC;
#endif
            if ( auto td = dynamic_cast<TypeDecl *>(ptr.get()) ) {
                tp << " TypeDecl at " << around(td->at) << " ";
                safeDumpType(tp,td);
            } else if ( auto ex = dynamic_cast<Expression *>(ptr.get()) ) {
                tp << " " <<  ex->__rtti << " at " << around(ex->at);
                if ( strcmp(ex->__rtti,"ExprConstString")==0 ) {
                    auto excs = static_cast<ExprConstString *>(ex);
                    tp << " \"" << excs->text << "\"";
                } else if ( strcmp(ex->__rtti,"ExprField")==0 ) {
                    auto exf = static_cast<ExprField *>(ex);
                    tp << " ." << exf->field;
                } else if ( strcmp(ex->__rtti,"ExprCall")==0 ) {
                    auto exc = static_cast<ExprCall *>(ex);
                    tp << " " << exc->name << "(with " << exc->arguments.size() << " arguments)";
                }
            } else if ( auto vp = dynamic_cast<Variable *>(ptr.get()) ) {
                tp << " Variable " << vp->name << " at " << around(vp->at);
            } else if ( auto fp = dynamic_cast<Function *>(ptr.get()) ) {
                tp << " Function " << fp->name << " at " << around(fp->at);
            }
            tp << "\n";
            total ++;
        }
        tp << "total " << total << " leaks\n";
#else
        tp << "smart_ptr tracking is disabled\n";
#endif
    }
}

