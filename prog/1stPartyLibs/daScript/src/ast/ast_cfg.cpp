#include "daScript/misc/platform.h"

#include "daScript/ast/ast_cfg.h"
#include "daScript/ast/ast_expressions.h"

#include "daScript/misc/string_writer.h"

namespace das {

    Cfg::~Cfg () { for ( auto b : blocks ) delete b; }

    CfgBlock * Cfg::newBlock () {
        auto b = new CfgBlock();
        b->id = uint32_t(blocks.size());
        blocks.push_back(b);
        return b;
    }

    void Cfg::addEdge ( CfgBlock * from, CfgBlock * to ) {
        if ( !from || !to ) return;
        from->succ.push_back(to);
        to->pred.push_back(from);
    }

    string Cfg::dump () const {
        TextWriter ss;
        ss << "cfg '" << (func ? func->name : "?") << "': " << uint32_t(blocks.size()) << " blocks\n";
        for ( auto b : blocks ) {
            ss << "  B" << b->id;
            if ( b==entry ) ss << " [entry]";
            if ( b==exit ) ss << " [exit]";
            ss << " ->";
            for ( auto s : b->succ ) ss << " B" << s->id;
            ss << "  (" << uint32_t(b->stmts.size()) << " stmt)\n";
        }
        return ss.str();
    }

    namespace {
        // recursive-descent CFG builder. `cur` is the block control is currently flowing through;
        // each step returns the block control continues in, or nullptr if the path terminated
        // (return / break / continue).
        struct CfgBuilder {
            Cfg & cfg;
            vector<pair<CfgBlock *, CfgBlock *>> loops;   // (continue-target = header, break-target = exit)
            explicit CfgBuilder ( Cfg & c ) : cfg(c) {}

            CfgBlock * seq ( CfgBlock * cur, ExprBlock * owner, const vector<Expression *> & list ) {
                for ( auto & e : list ) {
                    if ( !cur ) break;                    // statements after a terminator are unreachable
                    // a block that did not open a lexical scope and is now receiving its first statement
                    // is a continuation (e.g. the merge after an if) - anchor it so a free can be inserted
                    // as a statement right before `e`, on the fall-through path only
                    if ( !cur->astHead && !cur->contOwner ) { cur->contOwner = owner; cur->contBefore = e; }
                    cur = stmt(cur, e);
                }
                return cur;
            }

            // a lexical block: its statements, then its `finally` on the normal exit path
            CfgBlock * block ( CfgBlock * cur, ExprBlock * blk ) {
                // record the AST anchor when this CFG block begins exactly at the AST block's first
                // statement (fresh block) - lets an analysis insert "at this block's entry"
                if ( cur && cur->stmts.empty() && !cur->astHead ) cur->astHead = blk;
                cur = seq(cur, blk, blk->list);
                cur = seq(cur, blk, blk->finalList);
                return cur;
            }

            CfgBlock * loop ( CfgBlock * cur, Expression * cond, Expression * body, ExprFor * forSource = nullptr ) {
                auto header = cfg.newBlock();
                auto bodyB  = cfg.newBlock();
                auto exitB  = cfg.newBlock();
                bodyB->loopSource = forSource;            // for-loop induction anchor (null for while)
                cfg.addEdge(cur, header);
                if ( cond ) header->stmts.push_back(cond);
                cfg.addEdge(header, bodyB);               // enter the body
                cfg.addEdge(header, exitB);               // 0 iterations / loop finished
                loops.push_back({header, exitB});
                auto bEnd = stmt(bodyB, body);
                if ( bEnd ) cfg.addEdge(bEnd, header);    // back-edge
                loops.pop_back();
                return exitB;
            }

            CfgBlock * stmt ( CfgBlock * cur, Expression * e ) {
                if ( !e ) return cur;
                if ( e->rtti_isBlock() ) {
                    return block(cur, (ExprBlock *) e);
                } else if ( e->rtti_isIfThenElse() ) {
                    auto iff = (ExprIfThenElse *) e;
                    cur->stmts.push_back(iff->cond);      // condition evaluated in the current block
                    auto merge = cfg.newBlock();
                    auto thenB = cfg.newBlock();
                    cfg.addEdge(cur, thenB);
                    auto tEnd = stmt(thenB, iff->if_true);
                    if ( tEnd ) cfg.addEdge(tEnd, merge);
                    if ( iff->if_false ) {
                        auto elseB = cfg.newBlock();
                        cfg.addEdge(cur, elseB);
                        auto eEnd = stmt(elseB, iff->if_false);
                        if ( eEnd ) cfg.addEdge(eEnd, merge);
                    } else {
                        cfg.addEdge(cur, merge);          // no else: condition-false falls to merge
                    }
                    // if both arms terminated (e.g. both return), nothing reaches the merge -> the
                    // whole if terminates the path; leave the merge block inert and report no continuation
                    return merge->pred.empty() ? nullptr : merge;
                } else if ( e->rtti_isWhile() ) {
                    auto wh = (ExprWhile *) e;
                    return loop(cur, wh->cond, wh->body);
                } else if ( e->rtti_isFor() ) {
                    auto fr = (ExprFor *) e;
                    for ( auto & src : fr->sources ) cur->stmts.push_back(src);
                    return loop(cur, nullptr, fr->body, fr);  // the iteration check is implicit; model like while
                } else if ( e->rtti_isWith() ) {
                    auto wi = (ExprWith *) e;
                    if ( wi->with ) cur->stmts.push_back(wi->with);   // the subject, then the body runs unconditionally (null for module flavor)
                    return stmt(cur, wi->body);
                } else if ( e->rtti_isReturn() ) {
                    cur->stmts.push_back(e);
                    cfg.addEdge(cur, cfg.exit);
                    return nullptr;
                } else if ( e->rtti_isBreak() ) {
                    if ( !loops.empty() ) cfg.addEdge(cur, loops.back().second);
                    return nullptr;
                } else if ( e->rtti_isContinue() ) {
                    if ( !loops.empty() ) cfg.addEdge(cur, loops.back().first);
                    return nullptr;
                } else {
                    cur->stmts.push_back(e);              // ordinary straight-line statement
                    return cur;
                }
            }
        };
    }

    Cfg buildCfg ( Function * fn ) {
        Cfg cfg;
        cfg.func = fn;
        cfg.entry = cfg.newBlock();
        cfg.exit = cfg.newBlock();
        cfg.exit->isExit = true;
        if ( fn && fn->body && fn->body->rtti_isBlock() ) {
            CfgBuilder b(cfg);
            auto end = b.block(cfg.entry, (ExprBlock *) fn->body);
            if ( end ) cfg.addEdge(end, cfg.exit);        // fallthrough / implicit return
        } else {
            cfg.addEdge(cfg.entry, cfg.exit);
        }
        return cfg;
    }

    ProgramCfg buildProgramCfg ( Program * program ) {
        ProgramCfg out;
        if ( !program || !program->thisModule ) return out;
        program->thisModule->functions.foreach([&](auto & fn){
            if ( fn->body && fn->body->rtti_isBlock() ) {
                out.perFunction[fn] = buildCfg(fn);
            }
        });
        return out;
    }

}
