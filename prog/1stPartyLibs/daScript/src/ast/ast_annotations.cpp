#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"

namespace das
{
    class FinAnnotationVisitor : public Visitor {
    public:
        FinAnnotationVisitor( const ProgramPtr & prog ) {
            program = prog;
        }
    protected:
        ProgramPtr              program;
    protected:
        virtual bool canVisitStructureFieldInit ( Structure * ) override { return false; }
        virtual bool canVisitArgumentInit ( Function * , const VariablePtr &, Expression * ) override { return false; }
        virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }

        virtual void preVisit ( Structure * var ) override {
            Visitor::preVisit(var);
            for ( const auto & pA : var->annotations ) {
                if (pA->annotation->rtti_isStructureAnnotation()) {
                    auto sa = static_pointer_cast<StructureAnnotation>(pA->annotation);
                    string err = "";
                    if (!sa->look(var, *program->thisModuleGroup, pA->arguments, err)) {
                        program->error("can't finalize structure annotation [" + sa->name + "]",err,"",
                            var->at, CompilationError::invalid_annotation);
                    }
                }
            }
        }
        virtual void preVisit ( Function * fn ) override {
            Visitor::preVisit(fn);
            for ( const auto & an : fn->annotations ) {
                auto fna = static_pointer_cast<FunctionAnnotation>(an->annotation);
                string err = "";
                if ( !fna->finalize(fn, *program->thisModuleGroup, an->arguments, program->options, err) ) {
                    program->error("can't finalize annotation [" + fna->name + "]",err,"",
                        fn->at, CompilationError::invalid_annotation);
                }
            }
        }
        virtual void preVisit ( ExprBlock * block ) override {
            Visitor::preVisit(block);
            if ( block->annotations.size() ) {
                for ( const auto & an : block->annotations ) {
                    auto fna = static_pointer_cast<FunctionAnnotation>(an->annotation);
                    string err = "";
                    if ( !fna->finalize(block, *program->thisModuleGroup, an->arguments, program->options, err) ) {
                        program->error("can't finalize annotation [" + fna->name + "]",err,"",
                            block->at, CompilationError::invalid_annotation);
                    }
                }
                if ( block->annotationDataSid || block->annotationData ) {
                    if ( block->annotationDataSid && block->annotationData ) {
                        program->thisModule->annotationData[block->annotationDataSid] = block->annotationData;
                    } else {
                        program->error("internal error. both annotationData and Sid must be provided","","",
                                       block->at, CompilationError::invalid_annotation);
                    }
                }
            }
        }

        void skipMakeBlock ( const ExpressionPtr & expr ) {
            if ( expr->rtti_isMakeBlock() ) {
                auto mkb = static_pointer_cast<ExprMakeBlock>(expr);
                auto blk = static_pointer_cast<ExprBlock>(mkb->block);
                blk->aotSkipMakeBlock = true;
            }
        }
        virtual void preVisit ( ExprCall * call ) override {
            Visitor::preVisit(call);
            if ( call->func->aotTemplate ) {
                for ( auto & arg : call->arguments ) {
                    skipMakeBlock(arg);
                }
            }
        }
        virtual void preVisit ( ExprOp1 * op1 ) override {
            Visitor::preVisit(op1);
            skipMakeBlock(op1->subexpr);
        }
        virtual void preVisit ( ExprOp2 * op2 ) override {
            Visitor::preVisit(op2);
            skipMakeBlock(op2->left);
            skipMakeBlock(op2->right);
        }
        virtual void preVisit ( ExprOp3 * op3 ) override {
            Visitor::preVisit(op3);
            skipMakeBlock(op3->left);
            skipMakeBlock(op3->right);
        }
    };

    void Program::finalizeAnnotations() {
        FinAnnotationVisitor fin(this);
        visit(fin);
    }

    bool Program::patchAnnotations() {
        bool astChanged = false;
        thisModule->functions.foreach([&](auto fn){
            for ( auto & ann : fn->annotations ) {
                if ( ann->annotation->rtti_isFunctionAnnotation() ) {
                    auto fann = static_pointer_cast<FunctionAnnotation>(ann->annotation);
                    string err;
                    if ( !fann->patch(fn, *thisModuleGroup, ann->arguments, options, err, astChanged) ) {
                        error("function annotation patch failed\n", err, "", fn->at, CompilationError::annotation_failed );
                    }
                    if ( astChanged ) return true;
                }
            }
            return false;
        });
        if ( astChanged ) goto done;
        thisModule->structures.find_first([&](auto st){
            for ( auto & ann : st->annotations ) {
                if ( ann->annotation->rtti_isStructureAnnotation() ) {
                    auto sann = static_pointer_cast<StructureAnnotation>(ann->annotation);
                    string err;
                    if ( !sann->patch(st, *thisModuleGroup, ann->arguments, err, astChanged) ) {
                        error("structure annotation patch failed\n", err, "", st->at, CompilationError::annotation_failed );
                    }
                    if ( astChanged ) return true;
                }
            }
            return false;
        });
    done:;
        return astChanged;
    }

    struct NormalizeOptionTypes : Visitor {
        static void run ( ProgramPtr program ) {
            NormalizeOptionTypes vis;
            program->visit(vis,/*visitGenerics =*/true);
        }

        virtual void preVisit ( TypeDecl * td ) {
            if ( !td ) return;
            if ( td->baseType != option ) return;

            for ( size_t i=0, is=td->argTypes.size(); i!=is; ++i ) {
                auto & TT = td->argTypes[i];
                TT->ref             = TT->ref            || td->ref;
                TT->constant        = TT->constant       || td->constant;
                TT->temporary       = TT->temporary      || td->temporary;
                TT->removeConstant  = TT->removeConstant || td->removeConstant;
                TT->removeDim       = TT->removeDim      || td->removeDim;
                TT->removeRef       = TT->removeRef      || td->removeRef;
                TT->explicitConst   = TT->explicitConst  || td->explicitConst;
                TT->explicitRef     = TT->explicitRef    || td->explicitRef;
                TT->implicit        = TT->implicit       || td->implicit;
            }
        }
    };

    void Program::normalizeOptionTypes () {
        NormalizeOptionTypes::run(this);
    }

    void Program::fixupAnnotations() {
        thisModule->functions.foreach([&](auto fn){
            for ( auto & ann : fn->annotations ) {
                if ( ann->annotation->rtti_isFunctionAnnotation() ) {
                    auto fann = static_pointer_cast<FunctionAnnotation>(ann->annotation);
                    string err;
                    if ( !fann->fixup(fn, *thisModuleGroup, ann->arguments, options, err) ) {
                        error("function annotation patch failed\n", err, "", fn->at, CompilationError::annotation_failed );
                    }
                }
            }
        });
    }

}
