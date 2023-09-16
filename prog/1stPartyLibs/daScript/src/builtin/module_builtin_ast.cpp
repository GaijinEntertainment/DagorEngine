#include "daScript/misc/platform.h"

#include "module_builtin_rtti.h"

#include "daScript/simulate/simulate_visit_op.h"
#include "daScript/ast/ast_policy_types.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_generate.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/simulate/aot_builtin_ast.h"
#include "daScript/simulate/aot_builtin_string.h"
#include "daScript/misc/performance_time.h"

#include "module_builtin_ast.h"
namespace das {

    int adapt_field_offset ( const char * fName, const StructInfo * info ) {
        for ( uint32_t i=0, is=info->count; i!=is; ++i ) {
            if ( strcmp(info->fields[i]->name,fName)==0 ) {
                return info->fields[i]->offset;
            }
        }
        DAS_VERIFYF(0,"mapping %s not found. not fully implemented dervied class %s", fName, info->name);
        return 0;
    }

    char * adapt_field ( const char * fName, char * pClass, const StructInfo * info ) {
        return pClass + adapt_field_offset(fName,info);
    }

    Func adapt ( const char * funcName, char * pClass, const StructInfo * info ) {
        char * field = adapt_field(funcName, pClass, info);
        return field ? *(Func*)field : Func(0);
    }

    bool addModuleFunction ( Module * module, FunctionPtr & _func, Context * context, LineInfoArg * lineInfo ) {
        if ( !module ) context->throw_error_at(lineInfo, "expecting module, not null");
        FunctionPtr func = das::move(_func);
        return module->addFunction(func, true);
    }

    bool addModuleKeyword ( Module * module, char * kwd, bool needOxfordComma, Context * context, LineInfoArg * lineInfo ) {
        if ( !module ) context->throw_error_at(lineInfo, "expecting module, not null");
        if ( !kwd || kwd[0]==0 ) context->throw_error_at(lineInfo, "expecting keyword, not empty string");
        return module->addKeyword(kwd, needOxfordComma, true);
    }

    void forEachFunction ( Module * module, const char * name, const TBlock<void,FunctionPtr> & block, Context * context, LineInfoArg * lineInfo ) {
        if ( !module ) context->throw_error_at(lineInfo, "expecting module, not null");
        vec4f args[1];
        if ( builtin_empty(name) ) {
            auto & fnbn = module->functions;
            context->invokeEx(block, args, nullptr, [&](SimNode * code){
                fnbn.foreach([&](auto fnv){
                    args[0] = cast<FunctionPtr>::from(fnv);
                    code->eval(*context);
                });
            },lineInfo);
        } else {
            auto & fnbn = module->functionsByName[hash64z(name)];
            context->invokeEx(block, args, nullptr, [&](SimNode * code){
                for ( auto & nv : fnbn ) {
                    args[0] = cast<FunctionPtr>::from(nv);
                    code->eval(*context);
                }
            },lineInfo);
        }
    }

    void forEachGenericFunction ( Module * module, const char * name, const TBlock<void,FunctionPtr> & block, Context * context, LineInfoArg * lineInfo ) {
        if ( !module ) context->throw_error_at(lineInfo, "expecting module, not null");
        vec4f args[1];
        if ( builtin_empty(name) ) {
            auto & fnbn = module->generics;
            context->invokeEx(block, args, nullptr, [&](SimNode * code){
                fnbn.foreach([&](auto nvfn){
                    args[0] = cast<FunctionPtr>::from(nvfn);
                    code->eval(*context);
                });
            },lineInfo);
        } else {
            auto & fnbn = module->genericsByName[hash64z(name)];
            context->invokeEx(block, args, nullptr, [&](SimNode * code){
                for ( auto & nv : fnbn ) {
                    args[0] = cast<FunctionPtr>::from(nv);
                    code->eval(*context);
                }
            },lineInfo);
        }
    }

    bool addModuleGeneric ( Module * module, FunctionPtr & _func, Context * context, LineInfoArg * lineInfo ) {
        if ( !module ) context->throw_error_at(lineInfo, "expecting module, not null");
        FunctionPtr func = das::move(_func);
        return module->addGeneric(func, true);
    }

    bool addModuleVariable ( Module * module, VariablePtr & _var, Context * context, LineInfoArg * lineInfo ) {
        if ( !module ) context->throw_error_at(lineInfo, "expecting module, not null");
        VariablePtr var = das::move(_var);
        return module->addVariable(das::move(var), true);
    }

    VariablePtr findModuleVariable ( Module * module, const char * name ) {
        return name ? module->findVariable(name) : nullptr;
    }

    bool addModuleStructure ( Module * module, StructurePtr & _struct ) {
        StructurePtr stru = das::move(_struct);
        return module->addStructure(stru, true);
    }

    bool removeModuleStructure ( Module * module, StructurePtr & _struct ) {
        StructurePtr stru = das::move(_struct);
        return module->removeStructure(stru);
    }

    bool addModuleAlias ( Module * module, TypeDeclPtr & _ptr ) {
        TypeDeclPtr ptr = das::move(_ptr);
        return module->addAlias(ptr, true);
    }

    Module * thisModule ( Context * context, LineInfoArg * lineinfo ) {
        if ( !context->thisProgram ) {
            context->throw_error_at(lineinfo, "can't get this module past compilation");
        }
        return context->thisProgram->thisModule.get();
    }

    Module * findRttiModule ( smart_ptr<Program> THAT_PROGRAM, const char * name, Context *, LineInfoArg *) {
        name = name ? name : "";
        auto found = THAT_PROGRAM->library.findModule(name);
        return found ? found : Module::require(name);
    }

    smart_ptr<Function> findRttiFunction ( Module * mod, Func func, Context * context, LineInfoArg * line_info ) {
        if ( !func.PTR ) context->throw_error_at(line_info, "function not found");
        if ( !mod ) context->throw_error_at(line_info, "module not found");
        return mod->findFunction(func.PTR->mangledName);
    }

    smart_ptr_raw<Program> thisProgram ( Context * context ) {
        return context->thisProgram;
    }

    Module * compileModule ( Context * context, LineInfoArg * at ) {
        auto program = daScriptEnvironment::bound->g_Program;
        if ( !program ) context->throw_error_at(at, "compileModule only available during compilation");
        return program->thisModule.get();
    }

    smart_ptr_raw<Program> compileProgram ( Context * context, LineInfoArg * at ) {
        auto program = daScriptEnvironment::bound->g_Program;
        if ( !program ) context->throw_error_at(at, "compileProgram only available during compilation");
        return program;
    }

    char * ast_describe_typedecl ( smart_ptr_raw<TypeDecl> t, bool d_extra, bool d_contracts, bool d_module, Context * context, LineInfoArg * at ) {
        if ( !t ) context->throw_error_at(at, "expecting type, not null");
        return context->stringHeap->allocateString(t->describe(
            d_extra ? TypeDecl::DescribeExtra::yes : TypeDecl::DescribeExtra::no,
            d_contracts ? TypeDecl::DescribeContracts::yes : TypeDecl::DescribeContracts::no,
            d_module ? TypeDecl::DescribeModule::yes : TypeDecl::DescribeModule::no));
    }

    char * ast_describe_typedecl_cpp ( smart_ptr_raw<TypeDecl> t, bool d_substitureRef, bool d_skipRef, bool d_skipConst, bool d_redundantConst, Context * context, LineInfoArg * at ) {
        if ( !t ) context->throw_error_at(at, "expecting type, not null");
        return context->stringHeap->allocateString(describeCppType(t,
            d_substitureRef ? CpptSubstitureRef::yes : CpptSubstitureRef::no,
            d_skipRef ? CpptSkipRef::yes : CpptSkipRef::no,
            d_skipConst ? CpptSkipConst::yes : CpptSkipConst::no,
            d_redundantConst ? CpptRedundantConst::yes : CpptRedundantConst::no));
    }

    char * ast_describe_expression ( smart_ptr_raw<Expression> t, Context * context, LineInfoArg * at ) {
        if ( !t ) context->throw_error_at(at, "expecting expression, not null");
        TextWriter ss;
        ss << *t;
        return context->stringHeap->allocateString(ss.str());
    }

    char * ast_describe_function ( smart_ptr_raw<Function> t, Context * context, LineInfoArg * at ) {
        if ( !t ) context->throw_error_at(at, "expecting function, not null");
        TextWriter ss;
        ss << *t;
        return context->stringHeap->allocateString(ss.str());
    }

    char * ast_das_to_string ( Type bt, Context * context ) {
        return context->stringHeap->allocateString(das_to_string(bt));
    }

    char * ast_find_bitfield_name ( smart_ptr_raw<TypeDecl> bft, Bitfield value, Context * context, LineInfoArg * at ) {
        if ( !bft ) context->throw_error_at(at, "expecting bitfield type, not null");
        return context->stringHeap->allocateString(bft->findBitfieldName(value));
    }

    int64_t ast_find_enum_value ( EnumerationPtr enu, const char * value ) {
        if ( !value ) return 0ul;
        return enu->find(value, 0ul);
    }

    void ast_error ( ProgramPtr prog, const LineInfo & at, const char * message, Context * context, LineInfoArg * lineInfo ) {
        if ( !prog ) context->throw_error_at(lineInfo,"program can't be null (expecting compiling_program())");
        prog->error(message ? message : "macro error","","",at,CompilationError::macro_failed);
    }

    int32_t get_variant_field_offset ( smart_ptr_raw<TypeDecl> td, int32_t index, Context * context, LineInfoArg * at ) {
        if ( !td ) context->throw_error_at(at,"expecting variant type");
        if ( td->baseType!=Type::tVariant ) context->throw_error_at(at,"expecting variant type, not %s", td->describe().c_str());
        return td->getVariantFieldOffset(index);
    }

    int32_t get_tuple_field_offset ( smart_ptr_raw<TypeDecl> td, int32_t index, Context * context, LineInfoArg * at ) {
        if ( !td ) context->throw_error_at(at,"expecting tuple type");
        if ( td->baseType!=Type::tTuple ) context->throw_error_at(at,"expecting tuple type, not %s", td->describe().c_str());
        return td->getTupleFieldOffset(index);
    }

    void any_table_foreach ( void * _tab, int keyStride, int valueStride, const TBlock<void,void *,void *> & blk, Context * context, LineInfoArg * at ) {
        auto tab = (Table *) _tab;
        if ( !tab->data ) return;
        char * values = tab->data;
        char * keys = tab->keys;
        for ( uint32_t index=0, indexs=tab->capacity; index!=indexs; index++, keys+=keyStride, values+=valueStride ) {
            if ( tab->hashes[index] > HASH_KILLED64 ) {
                das_invoke<void>::invoke<void *,void *>(context,at,blk,(void*)keys,(void*)values);
            }
        }
    }

    void any_array_foreach ( void * _arr, int stride, const TBlock<void,void *> & blk, Context * context, LineInfoArg * at ) {
        auto arr = (Array *) _arr;
        if ( !arr->data ) return;
        char * values = arr->data;
        for ( uint32_t index=0, indexs=arr->size; index!=indexs; index++, values+=stride ) {
            das_invoke<void>::invoke<void *>(context,at,blk,(void*)values);
        }
    }

    int32_t any_array_size ( void * _arr ) {
        return int32_t(((Array *) _arr)->size);
    }

    int32_t any_table_size ( void * _tab ) {
        return int32_t(((Table *) _tab)->size);
    }

    void for_each_module ( Program * prog, const TBlock<void,Module *> & block, Context * context, LineInfoArg * at ) {
        prog->library.foreach_in_order([&](auto mod){
            das_invoke<void>::invoke<Module *>(context,at,block,mod);
            return true;
        },prog->thisModule.get());
    }

    void for_each_typedef ( Module * mod, const TBlock<void,TTemporary<char *>,TypeDeclPtr> & block, Context * context, LineInfoArg * at ) {
        mod->aliasTypes.foreach([&](auto aliasType){
            das_invoke<void>::invoke<const char *,TypeDeclPtr>(context,at,block,aliasType->alias.c_str(),aliasType);
        });
    }

    void for_each_enumeration ( Module * mod, const TBlock<void,EnumerationPtr> & block, Context * context, LineInfoArg * at ) {
        mod->enumerations.foreach([&](auto penum){
            das_invoke<void>::invoke<EnumerationPtr>(context,at,block,penum);
        });
    }

    void for_each_structure ( Module * mod, const TBlock<void,StructurePtr> & block, Context * context, LineInfoArg * at ) {
        mod->structures.foreach([&](auto pst){
            das_invoke<void>::invoke<StructurePtr>(context,at,block,pst);
        });
    }

    void for_each_generic ( Module * mod, const TBlock<void,FunctionPtr> & block, Context * context, LineInfoArg * at ) {
        mod->generics.foreach([&](auto fn){
            das_invoke<void>::invoke<FunctionPtr>(context,at,block,fn);
        });
    }

    void for_each_global ( Module * mod, const TBlock<void,VariablePtr> & block, Context * context, LineInfoArg * at ) {
        mod->globals.foreach([&](auto var){
            das_invoke<void>::invoke<VariablePtr>(context,at,block,var);
        });
    }

    void for_each_call_macro ( Module * mod, const TBlock<void,TTemporary<char *>> & block, Context * context, LineInfoArg * at ) {
        for ( auto & td : mod->callThis ) {
            das_invoke<void>::invoke<const char *>(context,at,block,td.first.c_str());
        }
    }

    void for_each_reader_macro ( Module * mod, const TBlock<void,TTemporary<char *>> & block, Context * context, LineInfoArg * at ) {
        for ( auto & td : mod->readMacros ) {
            das_invoke<void>::invoke<const char *>(context,at,block,td.first.c_str());
        }
    }

    void for_each_variant_macro ( Module * mod, const TBlock<void,VariantMacroPtr> & block, Context * context, LineInfoArg * at ) {
        for ( auto & td : mod->variantMacros ) {
            das_invoke<void>::invoke<VariantMacroPtr>(context,at,block,td);
        }
    }

    void for_each_for_loop_macro ( Module * mod, const TBlock<void,ForLoopMacroPtr> & block, Context * context, LineInfoArg * at ) {
        for ( auto & td : mod->forLoopMacros ) {
            das_invoke<void>::invoke<ForLoopMacroPtr>(context,at,block,td);
        }
    }

    void for_each_typeinfo_macro ( Module * mod, const TBlock<void,TypeInfoMacroPtr> & block, Context * context, LineInfoArg * at ) {
        for ( auto & td : mod->typeInfoMacros ) {
            das_invoke<void>::invoke<TypeInfoMacroPtr>(context,at,block,td.second);
        }
    }

    bool isSameAstType ( TypeDeclPtr THIS,
                     TypeDeclPtr decl,
                     RefMatters refMatters,
                     ConstMatters constMatters,
                     TemporaryMatters temporaryMatters,
                     Context * context, LineInfoArg * at ) {
        if ( !THIS ) context->throw_error_at(at, "expecting left type");
        if ( !decl ) context->throw_error_at(at, "expecting right type");
        return THIS->isSameType(*decl,refMatters,constMatters,temporaryMatters);
    }

    void builtin_structure_for_each_field ( const BasicStructureAnnotation & ann,
        const TBlock<void,char *,char*,TypeDeclPtr,uint32_t> & block, Context * context, LineInfoArg * at ) {
        for ( auto & it : ann.fields ) {
            const auto & fld = it.second;
            das_invoke<void>::invoke<const char *,const char *,TypeDeclPtr,uint32_t>(context,at,block,
                it.first.c_str(), fld.cppName.c_str(),fld.decl,fld.offset);
        }
    }

    char * get_mangled_name ( smart_ptr_raw<Function> func, Context * context, LineInfoArg * at ) {
        if ( !func ) context->throw_error_at(at,"expecting function");
        return context->stringHeap->allocateString(func->getMangledName());
    }

    char * get_mangled_name_t ( smart_ptr_raw<TypeDecl> typ, Context * context, LineInfoArg * at ) {
        if ( !typ ) context->throw_error_at(at,"expecting function");
        return context->stringHeap->allocateString(typ->getMangledName());
    }

    char * get_mangled_name_v ( smart_ptr_raw<Variable> var, Context * context, LineInfoArg * at ) {
        if ( !var ) context->throw_error_at(at,"expecting function");
        return context->stringHeap->allocateString(var->getMangledName());
    }

    char * get_mangled_name_b ( smart_ptr_raw<ExprBlock> expr, Context * context, LineInfoArg * at ) {
        if ( !expr ) context->throw_error_at(at,"expecting block");
        return context->stringHeap->allocateString(expr->getMangledName());
    }

    void get_use_global_variables ( smart_ptr_raw<Function> func, const TBlock<void,VariablePtr> & block, Context * context, LineInfoArg * at ) {
        if ( !func ) context->throw_error_at(at,"expecting function");
        for ( auto & var : func->useGlobalVariables ) {
            das_invoke<void>::invoke<VariablePtr>(context,at,block,var);
        }
    }

    void get_use_functions ( smart_ptr_raw<Function> func, const TBlock<void,FunctionPtr> & block, Context * context, LineInfoArg * at ) {
        if ( !func ) context->throw_error_at(at,"expecting function");
        for ( auto & fn : func->useFunctions ) {
            das_invoke<void>::invoke<FunctionPtr>(context,at,block,fn);
        }
    }

    class MangledNameParserCtx : public MangledNameParser {
        virtual void error ( const string & err, const char * ch ) override {
            context->throw_error_at(at, "%s, near %s", err.c_str(), ch );
        }
    public:
        Context *   context = nullptr;
        LineInfo *  at = nullptr;
    };

    TypeDeclPtr parseMangledNameFn ( const char * txt, ModuleGroup & lib, Module * thisModule, Context * context, LineInfoArg * at ) {
        if ( !txt ) context->throw_error_at(at, "can't parse empty mangled name");
        MangledNameParserCtx parser;
        parser.context = context;
        parser.at = at;
        return parser.parseTypeFromMangledName(txt, lib, thisModule);
    }

    void forceAtRaw ( const smart_ptr_raw<Expression> & expr, const LineInfo & at ) {
        forceAt(expr, at);
    }

    void forceGeneratedRaw ( const smart_ptr_raw<Expression> & expr, bool setGenerated ) {
        forceGenerated(expr, setGenerated);
    }

    bool isExprLikeCall ( const ExpressionPtr & expr ) {
        return expr->rtti_isCallLikeExpr();
    }

    bool isExprConst ( const ExpressionPtr & expr ) {
        return expr->rtti_isConstant();
    }

    bool isTempType ( TypeDeclPtr ptr, bool refMatters ) {
        return ptr->isTempType(refMatters);
    }

    ExpressionPtr makeCall ( const LineInfo & at, const char * name ) {
        name = name ? name : "";
        auto program = daScriptEnvironment::bound->g_Program;
        return program->makeCall(at, name);
    }

    float4 evalSingleExpression ( const ExpressionPtr & expr, bool & ok ) {
        ok = true;
        das::Context ctx;
        auto node = expr->simulate(ctx);
        ctx.restart();
        vec4f result = ctx.evalWithCatch(node);
        if ( ctx.getException() ) ok = false;
        return result;
    }

    bool builtin_isVisibleDirectly ( Module * from, Module * too ) {
        return from->isVisibleDirectly(too);
    }

    bool builtin_hasField ( TypeDeclPtr ptr, const char * field, bool constant ) {
        if ( !field || !ptr ) return false;
        if ( ptr->baseType==Type::tStructure ) {
            return ptr->structType->findField(field);
        } else if ( ptr->baseType==Type::tTuple ) {
            return ptr->tupleFieldIndex(field) != -1;
        } else if ( ptr->baseType==Type::tVariant ) {
            return ptr->variantFieldIndex(field) != -1;
        } else if ( ptr->baseType==Type::tHandle ) {
            return ptr->annotation->makeFieldType(field, constant);
        } else if ( ptr->baseType==Type::tPointer && ptr->firstType ) {
            return builtin_hasField(ptr->firstType, field, constant);
        }
        return false;
    }

    TypeDeclPtr builtin_fieldType ( TypeDeclPtr ptr, const char * field, bool constant ) {
        if ( !field || !ptr ) return nullptr;
        if ( ptr->baseType==Type::tStructure ) {
            return ptr->structType->findField(field)->type;
        } else if ( ptr->baseType==Type::tTuple ) {
            auto idx = ptr->tupleFieldIndex(field);
            return idx!=-1 ? ptr->argTypes[idx] : nullptr;
        } else if ( ptr->baseType==Type::tVariant ) {
            auto idx = ptr->variantFieldIndex(field);
            return idx!=-1 ? ptr->argTypes[idx] : nullptr;
        } else if ( ptr->baseType==Type::tHandle ) {
            return ptr->annotation->makeFieldType(field, constant);
        } else if ( ptr->baseType==Type::tPointer && ptr->firstType ) {
            return builtin_fieldType(ptr->firstType, field, constant);
        }
        return nullptr;
    }

    Structure::FieldDeclaration * ast_findStructureField ( Structure * structType, const char * field, Context * context, LineInfoArg * at ) {
        if ( !structType ) context->throw_error_at(at,"expecting structure");
        if ( !field ) return nullptr;
        return (Structure::FieldDeclaration *) structType->findField(field);
    }

    Structure * find_unique_structure ( smart_ptr_raw<Program> prog, const char * name, Context * context, LineInfoArg * at ) {
        if ( !name ) return nullptr;
        if ( !prog ) context->throw_error_at(at, "expecting program");
        auto st = prog->findStructure(name);
        if ( st.size()!=1 ) return nullptr;
        return st.back().get();
    }

    void * das_get_builtin_function_address ( Function * fn, Context * context, LineInfoArg * at ) {
        if ( !fn ) context->throw_error_at(at, "expecting function");
        if ( !fn->builtIn ) context->throw_error_at(at, "expecting built-in interop function");
        return ((BuiltInFunction *)fn)->getBuiltinAddress();
    }

    void * das_make_interop_node ( Context & ctx, ExprCallFunc * call, Context * context, LineInfoArg * at ) {
        if ( !call ) context->throw_error_at(at, "expecting function call");
        auto fn = call->func;
        if ( !fn ) context->throw_error_at(at, "expecting function");
        if ( !fn->builtIn || !fn->interopFn ) context->throw_error_at(at, "expecting built-in interop function");
        if ( !ctx.thisHelper ) context->throw_error_at(at, "missing debug info helper. get_aot_interop_node can only be called in the SimulateMacro");
        auto node = ctx.code->makeNode<SimNode_AotInteropBase>();
        node->nArguments = (int) call->arguments.size();
        node->argumentValues = nullptr;
        if ( node->nArguments ) {
            node->types = (TypeInfo **) ctx.code->allocate(node->nArguments * sizeof(TypeInfo*));
            for ( int i=0, is=node->nArguments; i!=is; ++i ) {
                node->types[i] = ctx.thisHelper->makeTypeInfo(nullptr, call->arguments[i]->type);
            }
        } else {
            node->types = nullptr;
        }
        return node;
    }

    TypeInfo * das_make_type_info_structure ( Context & ctx, TypeDeclPtr ptr, Context * context, LineInfoArg * at ) {
        if ( !ptr ) context->throw_error_at(at, "expecting type");
        if ( !ctx.thisHelper ) context->throw_error_at(at, "missing type-info helper. context allready fully compiled");
        return ctx.thisHelper->makeTypeInfo(nullptr, ptr);
    }

    void * das_sb_make_interop_node ( Context & ctx, ExprStringBuilder * call, Context * context, LineInfoArg * at ) {
        if ( !call ) context->throw_error_at(at, "expecting string builder");
        if ( !ctx.thisHelper ) context->throw_error_at(at, "missing debug info helper. get_aot_interop_node can only be called in the SimulateMacro");
        auto node = ctx.code->makeNode<SimNode_AotInteropBase>();
        node->nArguments = (int) call->elements.size();
        node->argumentValues = nullptr;
        if ( node->nArguments ) {
            node->types = (TypeInfo **) ctx.code->allocate(node->nArguments * sizeof(TypeInfo*));
            for ( int i=0, is=node->nArguments; i!=is; ++i ) {
                node->types[i] = ctx.thisHelper->makeTypeInfo(nullptr, call->elements[i]->type);
            }
        } else {
            node->types = nullptr;
        }
        return node;
    }

    void das_comp_log ( const char * text, Context * context, LineInfoArg * at ) {
        if ( !text ) return;
        if ( !daScriptEnvironment::bound || !daScriptEnvironment::bound->g_compilerLog ) {
             context->throw_error_at(at, "compiler log is not set. its only available for the macros during compilation");
        }
        (*daScriptEnvironment::bound->g_compilerLog) << text;
    }

    Annotation * get_expression_annotation ( Expression * expr, Context * context, LineInfoArg * at ) {
        if ( !expr ) return nullptr;
        if ( !daScriptEnvironment::bound ) context->throw_error_at(at, "expecting bound environment");
        auto mod = Module::require("ast");
        return mod->findAnnotation(expr->__rtti).get();
    }

    void addModuleOption ( Module * mod, char * option, Type type, Context * context, LineInfoArg * at ) {
        if ( !mod ) context->throw_error_at(at, "expecting module");
        if ( !option ) context->throw_error_at(at, "expecting option name");
        if ( mod->getOptionType(option) != Type::none ) context->throw_error_at(at, "option %s already exists", option);
        mod->options[option] = type;
    }

    TypeDeclPtr getUnderlyingValueType ( smart_ptr_raw<TypeDecl> type, Context * context, LineInfoArg * at ) {
        if ( !type ) context->throw_error_at(at, "expecting type");
        if ( type->baseType!=Type::tHandle ) context->throw_error_at(at, "expecting handle type");
        return type->annotation->makeValueType();
    }

    TypeInfo * getHandledTypeFieldType ( smart_ptr_raw<TypeAnnotation> annotation, char * name, Context * context, LineInfoArg * at ) {
        if ( !name ) context->throw_error_at(at, "expecting field name");
        if ( !annotation ) context->throw_error_at(at, "expecting type annotation");
        return annotation->getFieldType(name);
    }

    uint32_t getHandledTypeFieldOffset ( smart_ptr_raw<TypeAnnotation> annotation, char * name, Context * context, LineInfoArg * at ) {
        if ( !name ) context->throw_error_at(at, "expecting field name");
        if ( !annotation ) context->throw_error_at(at, "expecting type annotation");
        return annotation->getFieldOffset(name);
    }

    void addModuleRequire ( Module * module, Module * reqModule, bool publ ) {
        module->requireModule[reqModule] |= publ;
    }

    #include "ast.das.inc"

    Module_Ast::Module_Ast() : Module("ast") {
        DAS_PROFILE_SECTION("Module_Ast");
        ModuleLibrary lib(this);
        lib.addBuiltInModule();
        addBuiltinDependency(lib, Module::require("rtti"));
        registerAnnotations(lib);
        registerAnnotations1(lib);
        registerAnnotations2(lib);
        registerAnnotations3(lib);
        registerAdapterAnnotations(lib);
        registerMacroExpressions(lib);
        registerFunctions(lib);
        // add builtin module
        compileBuiltinModule("ast.das",ast_das,sizeof(ast_das));
        // lets make sure its all aot ready
        // verifyAotReady();
    }

    void Module_Ast::registerFunctions(ModuleLibrary & lib){
        addExtern<DAS_BIND_FUN(thisProgram)>(*this, lib,  "this_program",
            SideEffects::accessExternal, "thisProgram")
                ->arg("context");
        addExtern<DAS_BIND_FUN(thisModule)>(*this, lib,  "this_module",
            SideEffects::accessExternal, "thisModule")
                ->args({"context","line"});
        addExtern<DAS_BIND_FUN(findRttiModule)>(*this, lib,  "find_module_via_rtti",
            SideEffects::accessExternal, "findRttiModule")
                ->args({"program","name","context","lineinfo"});
        addExtern<DAS_BIND_FUN(findRttiFunction)>(*this, lib,  "find_module_function_via_rtti",
            SideEffects::accessExternal, "findRttiFunction")
                ->args({"module","function","context","lineinfo"});
        addExtern<DAS_BIND_FUN(compileProgram)>(*this, lib,  "compiling_program",
            SideEffects::accessExternal, "compileProgram")
                ->args({"context","at"});
        addExtern<DAS_BIND_FUN(compileModule)>(*this, lib,  "compiling_module",
            SideEffects::accessExternal, "compileModule")
                ->args({"context","at"});
        addExtern<DAS_BIND_FUN(for_each_module)>(*this, lib,  "for_each_module",
            SideEffects::accessExternal, "for_each_module")
                ->args({"program","block","context","line"});
        addExtern<DAS_BIND_FUN(forEachFunction)>(*this, lib,  "for_each_function",
            SideEffects::accessExternal, "forEachFunction")
                ->args({"module","name","block","context","line"});
        addExtern<DAS_BIND_FUN(forEachGenericFunction)>(*this, lib,  "for_each_generic",
            SideEffects::accessExternal, "forEachGenericFunction")
                ->args({"module","name","block","context","line"});
        addExtern<DAS_BIND_FUN(forceAtRaw)>(*this, lib,  "force_at",
            SideEffects::accessExternal, "forceAtRaw")
                ->args({"expression","at"});
        addExtern<DAS_BIND_FUN(forceGeneratedRaw)>(*this, lib,  "force_generated",
            SideEffects::accessExternal, "forceGeneratedRaw")
                ->args({"expression","value"});
        addExtern<DAS_BIND_FUN(parseMangledNameFn)>(*this, lib,  "parse_mangled_name",
            SideEffects::none, "parseMangledNameFn")
                ->args({"txt","lib","thisModule","context","line"});
        addExtern<DAS_BIND_FUN(collectDependencies)>(*this, lib,  "collect_dependencies",
            SideEffects::invoke, "collectDependencies")
                ->args({"function","block","context","line"});;
        addExtern<DAS_BIND_FUN(addModuleFunction)>(*this, lib, "add_function",
            SideEffects::modifyExternal, "addModuleFunction")
                ->args({"module","function","context","line"});
        addExtern<DAS_BIND_FUN(addModuleGeneric)>(*this, lib, "add_generic",
            SideEffects::modifyExternal, "addModuleGeneric")
                ->args({"module","function","context","line"});
        addExtern<DAS_BIND_FUN(addModuleVariable)>(*this, lib, "add_variable",
            SideEffects::modifyExternal, "addModuleVariable")
                ->args({"module","variable","context","line"});
        addExtern<DAS_BIND_FUN(addModuleKeyword)>(*this, lib, "add_keyword",
            SideEffects::modifyExternal, "addModuleKeyword")
                ->args({"module","keyword","needOxfordComma","context","line"});
        addExtern<DAS_BIND_FUN(findModuleVariable)>(*this, lib, "find_variable",
            SideEffects::modifyExternal, "findModuleVariable")
                ->args({"module","variable"});
        addExtern<DAS_BIND_FUN(addModuleStructure)>(*this, lib, "add_structure",
            SideEffects::modifyExternal, "addModuleStructure")
                ->args({"module","structure"});
        addExtern<DAS_BIND_FUN(removeModuleStructure)>(*this, lib, "remove_structure",
            SideEffects::modifyExternal, "removeModuleStructure")
                ->args({"module","structure"});
        addExtern<DAS_BIND_FUN(clone_structure)>(*this, lib,  "clone_structure",
            SideEffects::none, "clone_structure")
                ->arg("structure");
        addExtern<DAS_BIND_FUN(addModuleAlias)>(*this, lib, "add_alias",
            SideEffects::modifyExternal, "addModuleAlias")
                ->args({"module","structure"});
        addExtern<DAS_BIND_FUN(ast_describe_typedecl)>(*this, lib,  "describe_typedecl",
            SideEffects::none, "ast_describe_typedecl")
                ->args({"type","extra","contracts","module","context","lineinfo"});
        addExtern<DAS_BIND_FUN(ast_describe_typedecl_cpp)>(*this, lib,  "describe_typedecl_cpp",
            SideEffects::none, "ast_describe_typedecl_cpp")
                ->args({"type","substitueRef","skipRef","skipConst","redundantConst","context","lineinfo"});
        addExtern<DAS_BIND_FUN(ast_describe_expression)>(*this, lib,  "describe_expression",
            SideEffects::none, "ast_describe_expression")
                ->args({"expression","context","lineinfo"});
        addExtern<DAS_BIND_FUN(ast_describe_function)>(*this, lib,  "describe_function",
            SideEffects::none, "describe_function")
                ->args({"function","context","lineinfo"});
        addExtern<DAS_BIND_FUN(ast_find_bitfield_name)>(*this, lib,  "find_bitfield_name",
            SideEffects::none, "find_bitfield_name")
                ->args({"bit","value","context","lineinfo"});
        addExtern<DAS_BIND_FUN(ast_find_enum_value)>(*this, lib,  "find_enum_value",
            SideEffects::none, "ast_find_enum_value")
                ->args({"enum","value"});
        addExtern<DAS_BIND_FUN(ast_findStructureField)>(*this, lib,  "find_structure_field",
            SideEffects::none, "ast_findStructureField")
                ->args({"structPtr","field","context","lineinfo"});
        addExtern<DAS_BIND_FUN(get_mangled_name)>(*this, lib,  "get_mangled_name",
            SideEffects::none, "get_mangled_name")
                ->args({"function","context","line"});
        addExtern<DAS_BIND_FUN(get_mangled_name_t)>(*this, lib,  "get_mangled_name",
            SideEffects::none, "get_mangled_name_t")
                ->args({"type","context","line"});
        addExtern<DAS_BIND_FUN(get_mangled_name_v)>(*this, lib,  "get_mangled_name",
            SideEffects::none, "get_mangled_name_v")
                ->args({"variable","context","line"});
        addExtern<DAS_BIND_FUN(get_mangled_name_b)>(*this, lib,  "get_mangled_name",
            SideEffects::none, "get_mangled_name_b")
                ->args({"variable","context","line"});
        addExtern<DAS_BIND_FUN(get_expression_annotation)>(*this, lib,  "get_expression_annotation",
            SideEffects::none, "get_expression_annotation")
                ->args({"expr","context","line"});
        // type conversion functions
        addExtern<DAS_BIND_FUN(ast_das_to_string)>(*this, lib,  "das_to_string",
            SideEffects::none, "das_to_string")
                ->args({"type","context"});
        // clone
        addExtern<DAS_BIND_FUN(clone_expression)>(*this, lib,  "clone_expression",
            SideEffects::none, "clone_expression")
                ->arg("expression");
        addExtern<DAS_BIND_FUN(clone_function)>(*this, lib,  "clone_function",
            SideEffects::none, "clone_function")
                ->arg("function");
        addExtern<DAS_BIND_FUN(clone_variable)>(*this, lib,  "clone_variable",
            SideEffects::none, "clone_variable")
                ->arg("variable");
        // type
        addExtern<DAS_BIND_FUN(isTempType)>(*this, lib,  "is_temp_type",
            SideEffects::none, "isTempType")
                ->args({"type","refMatters"});
        addExtern<DAS_BIND_FUN(isSameAstType)>(*this, lib,  "is_same_type",
            SideEffects::none, "isSameAstType")
                ->args({"leftType","rightType","refMatters","constMatters","tempMatters","context","at"});
        addExtern<DAS_BIND_FUN(clone_type)>(*this, lib,  "clone_type",
            SideEffects::none, "clone_type")
                ->arg("type");
        addExtern<DAS_BIND_FUN(get_variant_field_offset)>(*this, lib,  "get_variant_field_offset",
            SideEffects::none, "get_variant_field_offset")
                ->args({"variant","index","context","at"});
        addExtern<DAS_BIND_FUN(get_tuple_field_offset)>(*this, lib,  "get_tuple_field_offset",
            SideEffects::none, "get_tuple_field_offset")
                ->args({"typle","index","context","at"});
        addExtern<DAS_BIND_FUN(any_table_foreach)>(*this, lib,  "any_table_foreach",
            SideEffects::modifyArgumentAndExternal, "any_table_foreach")
                ->args({"table","keyStride","valueStride","block","context","line"});
        addExtern<DAS_BIND_FUN(any_array_foreach)>(*this, lib,  "any_array_foreach",
            SideEffects::modifyArgumentAndExternal, "any_array_foreach")
                ->args({"array","stride","block","context","line"});
        addExtern<DAS_BIND_FUN(any_array_size)>(*this, lib,  "any_array_size",
            SideEffects::none, "any_array_size")
                ->arg("array");
        addExtern<DAS_BIND_FUN(any_table_size)>(*this, lib,  "any_table_size",
            SideEffects::none, "any_table_size")
                ->arg("table");
        addExtern<DAS_BIND_FUN(getUnderlyingValueType)>(*this, lib,  "get_underlying_value_type",
            SideEffects::none, "getUnderlyingValueType")
                ->args({"type","context","line"});
        addExtern<DAS_BIND_FUN(getHandledTypeFieldOffset)>(*this, lib,  "get_handled_type_field_offset",
            SideEffects::none, "getHandledTypeFieldOffset")
                ->args({"type","field","context","line"});
        addExtern<DAS_BIND_FUN(getHandledTypeFieldType)>(*this, lib,  "get_handled_type_field_type",
            SideEffects::none, "getHandledTypeFieldType")
                ->args({"type","field","context","line"});
        // module
        addExtern<DAS_BIND_FUN(for_each_typedef)>(*this, lib,  "for_each_typedef",
            SideEffects::modifyExternal, "for_each_typedef")
                ->args({"module","block","context","line"});
        addExtern<DAS_BIND_FUN(for_each_enumeration)>(*this, lib,  "for_each_enumeration",
            SideEffects::modifyExternal, "for_each_enumeration")
                ->args({"module","block","context","line"});
        addExtern<DAS_BIND_FUN(for_each_structure)>(*this, lib,  "for_each_structure",
            SideEffects::modifyExternal, "for_each_structure")
                ->args({"module","block","context","line"});
        addExtern<DAS_BIND_FUN(for_each_generic)>(*this, lib,  "for_each_generic",
            SideEffects::modifyExternal, "for_each_generic")
                ->args({"module","block","context","line"});
        addExtern<DAS_BIND_FUN(for_each_global)>(*this, lib,  "for_each_global",
            SideEffects::modifyExternal, "for_each_global")
                ->args({"module","block","context","line"});
        addExtern<DAS_BIND_FUN(for_each_call_macro)>(*this, lib,  "for_each_call_macro",
            SideEffects::modifyExternal, "for_each_call_macro")
                ->args({"module","block","context","line"});
        addExtern<DAS_BIND_FUN(for_each_reader_macro)>(*this, lib,  "for_each_reader_macro",
            SideEffects::modifyExternal, "for_each_reader_macro")
                ->args({"module","block","context","line"});
        addExtern<DAS_BIND_FUN(for_each_variant_macro)>(*this, lib,  "for_each_variant_macro",
            SideEffects::modifyExternal, "for_each_variant_macro")
                ->args({"module","block","context","line"});
        addExtern<DAS_BIND_FUN(for_each_for_loop_macro)>(*this, lib,  "for_each_for_loop_macro",
            SideEffects::modifyExternal, "for_each_for_loop_macro")
                ->args({"module","block","context","line"});
        addExtern<DAS_BIND_FUN(for_each_typeinfo_macro)>(*this, lib,  "for_each_typeinfo_macro",
            SideEffects::modifyExternal, "for_each_typeinfo_macro")
                ->args({"module","block","context","line"});
        addExtern<DAS_BIND_FUN(builtin_structure_for_each_field)>(*this, lib,  "for_each_field",
            SideEffects::modifyExternal, "builtin_structure_for_each_field")
                ->args({"annotation","block","context","line"});
        addExtern<DAS_BIND_FUN(builtin_hasField)>(*this, lib, "has_field",
            SideEffects::modifyExternal, "builtin_hasField")
                ->args({"type","fieldName","constant"});
        addExtern<DAS_BIND_FUN(builtin_fieldType)>(*this, lib, "get_field_type",
            SideEffects::modifyExternal, "builtin_fieldType")
                ->args({"type","fieldName","constant"});
        addExtern<DAS_BIND_FUN(addModuleRequire)>(*this, lib, "add_module_require",
                SideEffects::modifyExternal, "addModuleRequire")
                    ->args({"module","publicModule","pub"});
        // type
        addExtern<DAS_BIND_FUN(builtin_isVisibleDirectly)>(*this, lib, "is_visible_directly",
            SideEffects::modifyExternal, "builtin_isVisibleDirectly")
                ->args({"from_module","which_module"});
        // context
        addExtern<DAS_BIND_FUN(getAstContext)>(*this, lib,  "get_ast_context",
            SideEffects::modifyExternal, "get_ast_context")
                ->args({"program","expression","block","context","line"});
        // code generation
        addExtern<DAS_BIND_FUN(makeClone)>(*this, lib,  "make_clone_structure",
            SideEffects::none, "makeClone")
                ->arg("structure");
        // expression generation
        addExtern<DAS_BIND_FUN(isExprLikeCall)>(*this, lib,  "is_expr_like_call",
            SideEffects::none, "isExprLikeCall")
                ->arg("expression");
        addExtern<DAS_BIND_FUN(isExprConst)>(*this, lib,  "is_expr_const",
            SideEffects::none, "isExprConst")
                ->arg("expression");
        addExtern<DAS_BIND_FUN(makeCall)>(*this, lib,  "make_call",
            SideEffects::none, "makeCall")
                ->args({"at","name"});
        addExtern<DAS_BIND_FUN(evalSingleExpression)>(*this, lib, "eval_single_expression",
            SideEffects::modifyArgument, "evalSingleExpression")
                ->args({"expr","ok"})->unsafeOperation = true;
        // errors
        addExtern<DAS_BIND_FUN(ast_error)>(*this, lib,  "macro_error",
            SideEffects::modifyArgumentAndExternal, "ast_error")
                ->args({"porogram","at","message","context","line"});
        // class
        addExtern<DAS_BIND_FUN(makeClassRtti)>(*this, lib,  "builtin_ast_make_class_rtti",
            SideEffects::modifyArgumentAndExternal, "makeClassRtti")
                ->args({"class"})->unsafeOperation = true;
        addExtern<DAS_BIND_FUN(makeClassFinalize)>(*this, lib,  "builtin_ast_make_class_finalize",
            SideEffects::modifyArgumentAndExternal, "makeClassFinalize")
                ->args({"class"})->unsafeOperation = true;
        addExtern<DAS_BIND_FUN(makeClassConstructor)>(*this, lib,  "builtin_ast_make_class_constructor",
            SideEffects::modifyArgumentAndExternal, "makeClassConstructor")
                ->args({"baseClass","method"})->unsafeOperation = true;
        addExtern<DAS_BIND_FUN(modifyToClassMember)>(*this, lib,  "builtin_ast_modify_to_class_member",
            SideEffects::modifyArgumentAndExternal, "modifyToClassMember")
                ->args({"func","baseClass","isExplicit","isConstant"})->unsafeOperation = true;
        addExtern<DAS_BIND_FUN(find_unique_structure)>(*this, lib,  "find_unique_structure",
            SideEffects::accessExternal, "find_unique_structure")
                ->args({"program","name","context","at"});
        // used variables and functions
        addExtern<DAS_BIND_FUN(get_use_global_variables)>(*this, lib,  "get_use_global_variables",
            SideEffects::invoke, "get_use_global_variables")
                ->args({"func","block","context","at"});
        addExtern<DAS_BIND_FUN(get_use_functions)>(*this, lib,  "get_use_functions",
            SideEffects::invoke, "get_use_functions")
                ->args({"func","block","context","at"});
        // typeinfo
        addExtern<DAS_BIND_FUN(das_make_type_info_structure)>(*this, lib,  "make_type_info_structure",
            SideEffects::none, "das_make_type_info_structure")
                ->args({"ctx","type","context","at"});
        // compilation log
        addExtern<DAS_BIND_FUN(das_comp_log)>(*this, lib,  "to_compilation_log",
            SideEffects::modifyExternal, "das_comp_log")
                ->args({"text","context","at"});
        //options
        addExtern<DAS_BIND_FUN(addModuleOption)>(*this, lib,  "add_module_option",
            SideEffects::modifyExternal, "addModuleOption")
                ->args({"module","option","type","context","at"});
    }

    ModuleAotType Module_Ast::aotRequire ( TextWriter & tw ) const {
        tw << "#include \"daScript/ast/ast.h\"\n";
        tw << "#include \"daScript/simulate/aot_builtin_ast.h\"\n";
        return ModuleAotType::cpp;
    }
}

REGISTER_MODULE_IN_NAMESPACE(Module_Ast,das);
