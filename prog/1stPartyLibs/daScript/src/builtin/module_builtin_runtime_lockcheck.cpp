#include "daScript/misc/platform.h"

#include "module_builtin.h"

#include "daScript/ast/ast_interop.h"
#include "daScript/simulate/aot_builtin.h"
#include "daScript/simulate/sim_policy.h"

namespace das
{
    struct LockDataWalker : DataWalker {
        bool locked = false;
        virtual bool canVisitArray ( Array * ar, TypeInfo * ) override {
            return !ar->forego_lock_check;
        }
        virtual bool canVisitArrayData ( TypeInfo * ti, uint32_t ) override {
            return (ti->flags & TypeInfo::flag_lockCheck) == TypeInfo::flag_lockCheck;
        }
        virtual bool canVisitTable ( char * pa, TypeInfo * ) override {
            return !((Table *)pa)->forego_lock_check;
        }
        virtual bool canVisitTableData ( TypeInfo * ti ) override {
            return ti->secondType && ((ti->secondType->flags & TypeInfo::flag_lockCheck) == TypeInfo::flag_lockCheck);
        }
        virtual bool canVisitHandle ( char *, TypeInfo * ) override {
            return false;
        }
        virtual bool canVisitPointer ( TypeInfo * ) override {
            return false;
        }
        virtual bool canVisitLambda ( TypeInfo * ) override {
            return false;
        }
        virtual bool canVisitIterator ( TypeInfo * ) override {
            return false;
        }
        virtual bool canVisitStructure ( char *, StructInfo * si ) override {
            return (si->flags & StructInfo::flag_lockCheck) == StructInfo::flag_lockCheck;
        }
        virtual void beforeArray ( Array * pa, TypeInfo * ) override {
            if ( pa->lock ) {
                locked = true;
                _cancel = true;
            }
        }
        virtual void beforeTable ( Table * pa, TypeInfo * ) override {
            if ( pa->lock ) {
                locked = true;
                _cancel = true;
            }
        }

        // fast versions of data walker functions

        virtual void walk_table ( Table * tab, TypeInfo * info ) override {
            if ( !info->secondType || ((info->secondType->flags & TypeInfo::flag_lockCheck) != TypeInfo::flag_lockCheck) ) return;
            int keySize = info->firstType->size;
            int valueSize = info->secondType->size;
            uint32_t count = 0;
            for ( uint32_t i=0, is=tab->capacity; i!=is; ++i ) {
                if ( tab->hashes[i] > HASH_KILLED64 ) {
                    char * value = tab->data + i*valueSize;
                    walk ( value, info->secondType );
                    if ( _cancel ) return;
                    count ++;
                }
            }
        }

        virtual void walk_struct ( char * ps, StructInfo * si ) override {
            if ( (si->flags & StructInfo::flag_class) ) {
                auto ti = *(TypeInfo **) ps;
                if ( ti!=nullptr ) si = ti->structType;
                else invalidData(); // we are walking uninitialized class here
            }
            if ( canVisitStructure_(ps, si) ) {
                beforeStructure_(ps, si);
                if ( _cancel ) {
                    afterStructureCancel_(ps, si);
                    return;
                }
                for ( uint32_t i=0, is=si->count; i!=is; ++i ) {
                    VarInfo * vi = si->fields[i];
                    char * pf = ps + vi->offset;
                    if ((vi->flags & TypeInfo::flag_lockCheck) == TypeInfo::flag_lockCheck) {
                        walk(pf, vi);
                        if ( _cancel ) {
                            afterStructureCancel_(ps, si);
                            return;
                        }
                    }
                }
            }
        }

        virtual void walk_array ( char * pa, uint32_t stride, uint32_t count, TypeInfo * ti ) override {
            if ( (ti->flags & TypeInfo::flag_lockCheck) != TypeInfo::flag_lockCheck ) return;
            char * pe = pa;
            for ( uint32_t i=0; i!=count; ++i ) {
                walk(pe, ti);
                if ( _cancel ) return;
                pe += stride;
            }
        }

        using DataWalker::walk;

        virtual void walk ( char * pa, TypeInfo * info ) override {
            if ( pa == nullptr ) {
                Null(info);
            } else if ( info->flags & TypeInfo::flag_ref ) {
                TypeInfo ti = *info;
                ti.flags &= ~TypeInfo::flag_ref;
                walk(*(char **)pa, &ti);
                ti.flags |= TypeInfo::flag_ref;
                if ( _cancel ) return;
            } else if ( info->dimSize ) {
                walk_dim(pa, info);
            } else if ( info->type==Type::tArray ) {
                auto arr = (Array *) pa;
                if ( !arr->forego_lock_check ) {
                    if ( arr->lock ) {
                        locked = true;
                        _cancel = true;
                        return;
                    }
                    walk_array(arr->data, info->firstType->size, arr->size, info->firstType);
                    if ( _cancel ) return;
                }
            } else if ( info->type==Type::tTable ) {
                auto tab = (Table *) pa;
                if ( !((Table *)pa)->forego_lock_check ) {
                    if ( tab->lock ) {
                        locked = true;
                        _cancel = true;
                        return;
                    }
                    walk_table(tab, info);
                    if ( _cancel ) return;
                }
            } else {
                switch ( info->type ) {
                    case Type::tStructure:  walk_struct(pa, info->structType); break;
                    case Type::tTuple:      walk_tuple(pa, info); break;
                    case Type::tVariant:    walk_variant(pa, info); break;
                    case Type::tBlock:      WalkBlock((Block *)pa); break;
                    case Type::tFunction:   WalkFunction((Func *)pa); break;
                    default:                break;
                }
            }
        }
    };

    struct LockErrorReporter : DataWalker {

        enum class PathType {
            none
        ,   field
        ,   tuple
        ,   array_element
        ,   table_element
        ,   variant
        };

        struct PathChunk {
            PathType type = PathType::none;
            const char * name = nullptr;
            TypeInfo * ti = nullptr;
            TypeInfo * vi = nullptr;
            int32_t index = 0;
            PathChunk ( const char * name ) {
                this->type = PathType::field;
                this->name = name;
            }
            PathChunk ( TypeInfo * TI, TypeInfo * VI ) {
                this->type = PathType::tuple;
                this->ti = TI;
                this->vi = VI;
            }
            PathChunk ( int32_t index ) {
                this->type = PathType::array_element;
                this->index = index;
            }
            PathChunk ( TypeInfo * TI, int32_t index ) {
                this->type = PathType::table_element;
                this->ti = TI;
                this->index = index;
            }
            PathChunk ( int32_t index, TypeInfo * TI ) {
                this->type = PathType::variant;
                this->index = index;
                this->ti = TI;
            }
        };
        vector<PathChunk> path;
        string pathStr;
        bool locked = false;
        uint32_t totalLockCount = 0;
        virtual bool canVisitArray ( Array * ar, TypeInfo * ) override {
            return !ar->forego_lock_check;
        }
        virtual bool canVisitArrayData ( TypeInfo * ti, uint32_t ) override {
            return (ti->flags & TypeInfo::flag_lockCheck) == TypeInfo::flag_lockCheck;
        }
        virtual bool canVisitTable ( char * pa, TypeInfo * ) override {
            return !((Table *)pa)->forego_lock_check;
        }
        virtual bool canVisitTableData ( TypeInfo * ti ) override {
            if ( ti->secondType ) {
                return (ti->secondType->flags & TypeInfo::flag_lockCheck) == TypeInfo::flag_lockCheck;
            } else {
                return false;
            }
        }
        virtual bool canVisitHandle ( char *, TypeInfo * ) override {
            return false;
        }
        virtual bool canVisitPointer ( TypeInfo * ) override {
            return false;
        }
        virtual bool canVisitLambda ( TypeInfo * ) override {
            return false;
        }
        virtual bool canVisitIterator ( TypeInfo * ) override {
            return false;
        }
        virtual bool canVisitStructure ( char *, StructInfo * si ) override {
            return (si->flags & StructInfo::flag_lockCheck) == StructInfo::flag_lockCheck;
        }
        void collectPath ( void ) {
            pathStr.clear();
            for ( auto & pc : path ) {
                switch ( pc.type ) {
                    case PathType::field:
                        pathStr += ".";
                        pathStr += pc.name ? pc.name : "???";
                        break;
                    case PathType::tuple:
                    {
                        int32_t idx = -1u;
                        for ( int32_t i=0, is=pc.ti->argCount; i!=is; ++i ) {
                            if ( pc.ti->argTypes[i]==pc.vi ) {
                                idx = i;
                                break;
                            }
                        }
                        pathStr += ".";
                        if ( pc.ti->argNames && pc.ti->argNames[idx] ) {
                            pathStr += pc.ti->argNames[idx];
                        } else {
                            pathStr += to_string(idx);
                        }
                        break;
                    }
                    case PathType::array_element:
                    case PathType::table_element:
                        pathStr += "[";
                        pathStr += to_string(pc.index);
                        pathStr += "]";
                        break;
                    case PathType::variant:
                    {
                        pathStr += " as ";
                        if ( pc.ti->argNames && pc.ti->argNames[pc.index] ) {
                            pathStr += pc.ti->argNames[pc.index];
                        } else {
                            pathStr += to_string(pc.index);
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }
        virtual void beforeArray ( Array * pa, TypeInfo * ) override {
            if ( pa->lock ) {
                locked = true;
                _cancel = true;
                totalLockCount = pa->lock;
            }
        }
        virtual void beforeTable ( Table * pa, TypeInfo * ) override {
            if ( pa->lock ) {
                locked = true;
                _cancel = true;
                totalLockCount = pa->lock;
            }
        }
        virtual void beforeStructureField ( char *, StructInfo *, char *, VarInfo * vi, bool ) override {
            path.emplace_back(PathChunk(vi->name));
        }
        virtual void afterStructureField ( char *, StructInfo *, char *, VarInfo *, bool ) override {
            path.pop_back();
        }
        virtual void beforeTupleEntry ( char *, TypeInfo * ti, char *, int idx, bool ) override {
            path.emplace_back(PathChunk(ti,ti->argTypes[idx]));
        }
        virtual void afterTupleEntry ( char *, TypeInfo *, char *, int, bool ) override {
            path.pop_back();
        }
        virtual void beforeArrayElement ( char *, TypeInfo *, char *, uint32_t index, bool ) override {
            path.emplace_back(PathChunk(index));
        }
        virtual void afterArrayElement ( char *, TypeInfo *, char *, uint32_t, bool ) override {
            path.pop_back();
        }
        virtual void beforeTableValue ( Table *, TypeInfo *, char *, TypeInfo * kv, uint32_t index, bool ) override {
            path.emplace_back(PathChunk(kv,index));
        }
        virtual void afterTableValue ( Table *, TypeInfo *, char *, TypeInfo *, uint32_t, bool ) override {
            path.pop_back();
        }
        virtual void beforeVariant ( char * ps, TypeInfo * ti ) override {
            int32_t fidx = *((int32_t *)ps);
            path.emplace_back(PathChunk(fidx,ti));
        }
        virtual void afterVariant ( char *, TypeInfo * ) override {
            path.pop_back();
        }
    };

    LineInfo rtti_get_line_info ( int depth, Context * context, LineInfoArg * at );

    vec4f builtin_verify_locks ( Context & context, SimNode_CallBase * node, vec4f * args ) {
        if ( context.skipLockChecks ) return v_zero();
        auto typeInfo = node->types[0];
        auto value = args[0];
        bool failed = false;
        {
            LockDataWalker walker;
            walker.walk(value,typeInfo);
            failed = walker.locked;
        }
        if ( failed ) {
            LineInfo atProblem = rtti_get_line_info(1,&context,(LineInfoArg *) &node->debugInfo);
            string errorPath;
            uint32_t totalLockCount = 0;
            {
                LockErrorReporter reporter;
                reporter.walk(value,typeInfo);
                reporter.collectPath();
                totalLockCount = reporter.totalLockCount;
                errorPath = reporter.pathStr;
            }
            context.throw_error_at(&atProblem, "object type<%s>%s contains locked elements (count=%u) and can't be modified (resized, pushed to, erased from, cleared, deleted, moved, or returned via move)",
                debug_type(typeInfo).c_str(), errorPath.c_str(), totalLockCount);
        }
        return v_zero();
    }

    bool builtin_set_verify_context ( bool check, Context * context ) {
        bool result = !context->skipLockChecks;
        context->skipLockChecks = !check;
        return result;
    }

    bool builtin_set_verify_array_locks ( Array & arr, bool value ) {
        auto result = !arr.forego_lock_check;
        arr.forego_lock_check = !value;
        return result;
    }

    bool builtin_set_verify_table_locks ( Table & tab, bool value ) {
        auto result = !tab.forego_lock_check;
        tab.forego_lock_check = !value;
        return result;
    }
}
