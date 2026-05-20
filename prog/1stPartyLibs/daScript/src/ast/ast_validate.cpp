#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    class ValidateAstVisitor : public Visitor {
        Program *   program;
        Module *    currentModule = nullptr;
        Function *  currentFunction = nullptr;
        Structure * currentStructure = nullptr;
        Variable *  currentGlobal = nullptr;
        Expression * currentExpression = nullptr;
        const char * currentField = nullptr;
        struct SeenInfo {
            string      context;
            LineInfo    at;
        };
        das_hash_map<void *, SeenInfo>  seen;
    public:
        ValidateAstVisitor ( Program * prog ) : program(prog) {}
        const das_hash_map<void *, SeenInfo> & getSeen () const { return seen; }
        void trackAnnotationTypeDecl ( TypeDecl * td, const string & annName ) {
            currentField = "annotation";
            trackTypeDeclTree(td, annName.c_str());
            currentField = nullptr;
        }
    protected:
        string currentContext () const {
            string ctx;
            if ( currentModule ) {
                ctx += "module '" + currentModule->name + "'";
            }
            if ( currentFunction ) {
                ctx += ", function '" + currentFunction->name + "'";
            }
            if ( currentStructure ) {
                ctx += ", structure '" + currentStructure->name + "'";
            }
            if ( currentGlobal ) {
                ctx += ", global '" + currentGlobal->name + "'";
            }
            if ( currentExpression ) {
                ctx += ", expr ";
                if ( currentExpression->__rtti ) ctx += currentExpression->__rtti;
                if ( currentExpression->at.fileInfo ) {
                    ctx += " at " + currentExpression->at.fileInfo->name
                        + ":" + to_string(currentExpression->at.line);
                }
            }
            if ( currentField ) {
                ctx += string(", field '") + currentField + "'";
            }
            return ctx;
        }
        bool trackNode ( void * node, const LineInfo & at ) {
            auto it = seen.find(node);
            if ( it != seen.end() ) return false; // duplicate
            seen[node] = SeenInfo { currentContext(), at };
            return true;
        }
        void reportDuplicateTypeDecl ( TypeDecl * td ) {
            auto it = seen.find(td);
            string firstCtx = it != seen.end() ? it->second.context : "?";
            string err = "validate_ast: duplicate TypeDecl (gc_id=" + to_string(td->gc_id) + ") " + td->describe();
            string extra = currentContext() + "\n  first seen in " + firstCtx;
            if ( it != seen.end() && it->second.at.fileInfo ) {
                extra += " at " + it->second.at.fileInfo->name + ":" + to_string(it->second.at.line);
            }
            program->error(err, extra, "", td->at, CompilationError::unspecified);
        }
        void reportDuplicateExpression ( Expression * expr ) {
            auto it = seen.find(expr);
            string firstCtx = it != seen.end() ? it->second.context : "?";
            string err = "validate_ast: duplicate Expression (gc_id=" + to_string(expr->gc_id) + ") ";
            if ( expr->__rtti ) err += expr->__rtti;
            string extra = currentContext() + "\n  first seen in " + firstCtx;
            if ( it != seen.end() && it->second.at.fileInfo ) {
                extra += " at " + it->second.at.fileInfo->name + ":" + to_string(it->second.at.line);
            }
            program->error(err, extra, "", expr->at, CompilationError::unspecified);
        }
        void trackTypeDeclTree ( TypeDecl * td, const char * field ) {
            if ( !td ) return;
            auto savedField = currentField;
            currentField = field;
            if ( !trackNode(td, td->at) ) {
                reportDuplicateTypeDecl(td);
                currentField = savedField;
                return;
            }
            trackTypeDeclTree(td->firstType, "firstType");
            trackTypeDeclTree(td->secondType, "secondType");
            for ( auto & argType : td->argTypes ) {
                trackTypeDeclTree(argType, "argTypes[]");
            }
            for ( auto de : td->dimExpr ) {
                trackExpression(de);
            }
            currentField = savedField;
        }
        void trackExpression ( Expression * expr ) {
            if ( !expr ) return;
            currentExpression = expr;
            if ( !trackNode(expr, expr->at) ) {
                reportDuplicateExpression(expr);
                return;
            }
            if ( expr->type ) {
                trackTypeDeclTree(expr->type, "type");
            }
        }
    // module
        virtual void preVisitModule ( Module * mod ) override {
            Visitor::preVisitModule(mod);
            currentModule = mod;
        }
        virtual void visitModule ( Module * mod ) override {
            Visitor::visitModule(mod);
            currentModule = nullptr;
        }
    // function
        virtual void preVisit ( Function * fn ) override {
            Visitor::preVisit(fn);
            currentFunction = fn;
        }
        virtual FunctionPtr visit ( Function * fn ) override {
            currentFunction = nullptr;
            return Visitor::visit(fn);
        }
    // structure
        virtual void preVisit ( Structure * st ) override {
            Visitor::preVisit(st);
            currentStructure = st;
        }
        virtual StructurePtr visit ( Structure * st ) override {
            currentStructure = nullptr;
            return Visitor::visit(st);
        }
    // global variable
        virtual void preVisitGlobalLet ( const VariablePtr & var ) override {
            Visitor::preVisitGlobalLet(var);
            currentGlobal = var;
        }
        virtual VariablePtr visitGlobalLet ( const VariablePtr & var ) override {
            currentGlobal = nullptr;
            return Visitor::visitGlobalLet(var);
        }
    // TypeDecl — standard visitor path (visited by the visitor framework itself)
        virtual void preVisit ( TypeDecl * td ) override {
            Visitor::preVisit(td);
            if ( !trackNode(td, td->at) ) {
                reportDuplicateTypeDecl(td);
            }
            // dimExpr: standard visitor only visits dimExpr when dim==dimConst or baseType is typeDecl/typeMacro.
            // After inference resolves dimensions, resolved dimExpr stays on gc_root but is skipped.
            if ( td->baseType != Type::typeDecl && td->baseType != Type::typeMacro ) {
                for ( size_t i=0, is=td->dimExpr.size(); i!=is; ++i ) {
                    if ( td->dimExpr[i] && (i >= td->dim.size() || td->dim[i] != TypeDecl::dimConst) ) {
                        trackExpression(td->dimExpr[i]);
                    }
                }
            }
        }
    // Expression — standard visitor path
        virtual void preVisitExpression ( Expression * expr ) override {
            Visitor::preVisitExpression(expr);
            trackExpression(expr);
        }
    // visit quote and assume sub-expressions (skipped by default in other visitors)
        virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return true; }
        virtual bool canVisitWithAliasSubexpression ( ExprAssume * ) override { return true; }
    // ExprConst — suppress preVisitExpression from ExprConst* dispatch.
    //   ExprConstT::visit calls both preVisit(ExprConst*) and preVisit(ExprConstInt*),
    //   both of which default to preVisitExpression. Override the base to avoid double-tracking.
        virtual void preVisit ( ExprConst * ) override {}
        virtual ExpressionPtr visit ( ExprConst * expr ) override { return expr; }
    // ExprLooksLikeCall — same pattern: ExprLikeCall<T>::visit chains to ExprLooksLikeCall::visit
    //   suppress preVisitExpression but still track aliasSubstitution
        virtual void preVisit ( ExprLooksLikeCall * expr ) override {
            trackTypeDeclTree(expr->aliasSubstitution, "aliasSubstitution");
        }
        virtual ExpressionPtr visit ( ExprLooksLikeCall * expr ) override { return expr; }
    // ExprCallMacro: canVisitArguments may skip arguments, but gc_collect walks them all.
    //   Visit skipped arguments so the validator sees them.
        virtual void preVisit ( ExprCallMacro * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->macro ) {
                int index = 0;
                for ( auto & arg : expr->arguments ) {
                    if ( !expr->macro->canVisitArguments(expr, index) && arg ) {
                        arg->visit(*this);
                    }
                    index++;
                }
            }
        }
    // --- additional overrides for TypeDecl fields NOT visited by standard visitor ---
        virtual void preVisit ( ExprConstPtr * expr ) override {
            Visitor::preVisit(expr);
            trackTypeDeclTree(expr->ptrType, "ptrType");
        }
        virtual void preVisit ( ExprConstBitfield * expr ) override {
            Visitor::preVisit(expr);
            trackTypeDeclTree(expr->bitfieldType, "bitfieldType");
        }
        virtual void preVisit ( ExprAscend * expr ) override {
            Visitor::preVisit(expr);
            trackTypeDeclTree(expr->ascType, "ascType");
        }
        virtual void preVisit ( ExprAssume * expr ) override {
            Visitor::preVisit(expr);
            // ExprAssume::visit calls assumeType->visit(vis) but NOT vis.preVisit(assumeType)
            // so children get visited by standard path — only track the top-level node here
            if ( expr->assumeType ) {
                auto saved = currentField;
                currentField = "assumeType";
                if ( !trackNode(expr->assumeType, expr->assumeType->at) ) {
                    reportDuplicateTypeDecl(expr->assumeType);
                }
                currentField = saved;
            }
        }
        virtual void preVisit ( ExprMakeArray * expr ) override {
            Visitor::preVisit(expr);
            trackTypeDeclTree(expr->recordType, "recordType");
        }
        virtual void preVisit ( ExprMakeTuple * expr ) override {
            Visitor::preVisit(expr);
            trackTypeDeclTree(expr->recordType, "recordType");
        }
        virtual void preVisit ( ExprReturn * expr ) override {
            Visitor::preVisit(expr);
            trackTypeDeclTree(expr->returnType, "returnType");
        }
        // ExprAddr::funcType — visited by standard visitor path in ExprAddr::visit
        // ExprMakeGenerator::iterType — visited by standard visitor path in ExprMakeGenerator::visit
    // ExprBlock: visitor only visits returnType and arguments when isClosure.
    //   gc_collect walks them unconditionally — track non-closure blocks here.
        virtual void preVisit ( ExprBlock * blk ) override {
            Visitor::preVisit(blk);
            if ( !blk->isClosure ) {
                trackTypeDeclTree(blk->returnType, "returnType");
                for ( auto & arg : blk->arguments ) {
                    if ( arg ) {
                        trackTypeDeclTree(arg->type, "blockArg.type");
                        trackExpression(arg->init);
                    }
                }
            }
        }
    // ExprFor: iteratorsTags are gc_collected but not visited by ExprFor::visit.
        virtual void preVisit ( ExprFor * expr ) override {
            Visitor::preVisit(expr);
            for ( auto & tag : expr->iteratorsTags ) {
                if ( tag ) tag->visit(*this);
            }
        }
        virtual void preVisitFor ( ExprFor * expr, const VariablePtr & var, bool last ) override {
            Visitor::preVisitFor(expr, var, last);
            trackTypeDeclTree(var->type, "iteratorVariable.type");
        }
    };

    void Program::validateAst () {
        if ( !policies.validate_ast ) return;
        // Phase 1: visitor traversal
        ValidateAstVisitor vis(this);
        visitBuiltinFunctions = true;
        visitModules(vis, true);
        visitBuiltinFunctions = false;
        // Phase 2: visit annotation TypeDecls (not covered by standard visitor)
        auto & modules = library.getModules();
        for ( auto mod : modules ) {
            for ( auto & [key, ann] : mod->handleTypes ) {
                if ( ann ) {
                    const auto &ann_name = ann->name;
                    ann->visitTypeDecls([&]( TypeDecl * td ) {
                        vis.trackAnnotationTypeDecl(td, ann_name);
                    });
                }
            }
        }
        // Phase 3: gc_root cross-check
        for ( auto mod : modules ) {
            for ( auto node = mod->module_gc_root.gc_first; node; node = node->gc_next ) {
                auto tag = node->gc_type_tag();
                if ( tag == GC_TAG_TYPEDECL ) {
                    if ( vis.getSeen().find((void *)node) == vis.getSeen().end() ) {
                        auto td = static_cast<TypeDecl *>(node);
                        string err = "validate_ast: TypeDecl (gc_id=" + to_string(td->gc_id) + ") not reached by visitor: " + td->describe();
                        string extra = "module '" + mod->name + "'";
                        error(err, extra, "", td->at, CompilationError::unspecified);
                    }
                } else if ( tag == GC_TAG_EXPRESSION ) {
                    if ( vis.getSeen().find((void *)node) == vis.getSeen().end() ) {
                        auto expr = static_cast<Expression *>(node);
                        string err = "validate_ast: Expression (gc_id=" + to_string(expr->gc_id) + ") not reached by visitor: ";
                        if ( expr->__rtti ) err += expr->__rtti;
                        string extra = "module '" + mod->name + "'";
                        error(err, extra, "", expr->at, CompilationError::unspecified);
                    }
                }
            }
        }
    }

}
