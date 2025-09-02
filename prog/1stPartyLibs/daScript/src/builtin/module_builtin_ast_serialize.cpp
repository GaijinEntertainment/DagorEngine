#include "daScript/misc/platform.h"
#include "daScript/misc/performance_time.h"

#include "daScript/ast/ast_serialize_macro.h"
#include "daScript/ast/ast_serializer.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast.h"
#include <cstdarg>
#include <stdexcept>

namespace das {

    AstSerializer::AstSerializer ( SerializationStorage * storage, bool isWriting ) {
        astModule = Module::require("ast");
        astModule->handleTypes.foreach([&](const AnnotationPtr & annotation) {
            if ( starts_with(annotation->name,"Expr") ) {
                uint32_t hash = hash_tag(annotation->name.c_str());
                rttiHash2Annotation[hash] = annotation.get();
            }
        });
        writing = isWriting;
        buffer = storage;
    }

    void AstSerializer::collectFileInfo ( vector<FileInfoPtr> & orphanedFileInfos ) {
        for ( auto fileInfo : deleteUponFinish ) {
            if ( doNotDelete.count(fileInfo) == 0 ) {
                orphanedFileInfos.emplace_back(fileInfo);
            }
        }
        deleteUponFinish.clear();
        doNotDelete.clear();
    }

    void AstSerializer::getCompiledModules ( ) {
        Module::foreach([this](Module * m) {
            writingReadyModules.emplace(m);
            return true;
        });
    }

    AstSerializer::~AstSerializer () {
        for ( auto fileInfo : deleteUponFinish ) {
            if ( doNotDelete.count(fileInfo) == 0 ) {
                delete fileInfo;
            }
        }
        if ( writing || !moduleLibrary ) {
            return;
        }
    // gather modules to delete
        vector<Module*> modules_to_delete;
        moduleLibrary->foreach([&]( Module * m ) {
            if (!m->builtIn || m->promoted)
                modules_to_delete.push_back(m);
            return true;
        }, "*");
    // delete modules
        for ( auto m : modules_to_delete ) {
            m->builtIn = false; // created manually, don't care about flags
            delete m;
        }
    }

    template <typename TT>
    void patchRefs ( vector<pair<TT**,uint64_t>> & refs, const das_hash_map<uint64_t, smart_ptr<TT>> & objects) {
        for ( auto & p : refs ) {
            auto it = objects.find(p.second);
            if ( it == objects.end() ) {
                throw std::runtime_error{"ast serializer function ref not found"};
            } else {
                *p.first = it->second.get();
            }
        }
        refs.clear();
    }

    void throw_formatted_error ( const char * fmt, ... ) {
        va_list args;
        va_start(args, fmt);

        char err[256];
        vsnprintf(err, 256, fmt, args);

        va_end(args);

        throw std::runtime_error{err};
    }

    #define SERIALIZER_VERIFYF(cond, ...) {                 \
        if ( !(cond) ) {                                    \
            throw_formatted_error(__VA_ARGS__);             \
        }                                                   \
    }

    void AstSerializer::patch () {
        patchRefs(functionRefs, smartFunctionMap);
        patchRefs(variableRefs, smartVariableMap);
        patchRefs(structureRefs, smartStructureMap);
        patchRefs(enumerationRefs, smartEnumerationMap);
    // finally, patch block refs (differenct container)
        for ( auto & p : blockRefs ) {
            auto it = exprBlockMap.find(p.second);
            if ( it == exprBlockMap.end() ) {
                throw_formatted_error("ast serializer block ref not found");
            } else {
                *p.first = it->second;
            }
        }
        blockRefs.clear();

        for ( auto & [field, mod, name, fieldname] : fieldRefs ) {
            auto struct_ = moduleLibrary->findStructure(name, mod);
            if ( struct_.size() == 0 ) {
                throw_formatted_error("expected to find structure '%s'", name.c_str());
            } else if ( struct_.size() > 1 ) {
                throw_formatted_error("too many candidates for structure '%s'", name.c_str());
            }
        // set the missing field field
            *field = struct_.front()->findField(fieldname);
        }
        fieldRefs.clear();
    }

    void AstSerializer::write ( const void * data, size_t size ) {
        buffer->write(data, size);
    }

    void AstSerializer::read ( void * data, size_t size ) {
        if ( !buffer->read(data, size) ) {
            throw_formatted_error("ast serializer read overflow");
        }
    }

    void AstSerializer::serialize ( void * data, size_t size ) {
        if ( writing ) {
            write(data,size);
        } else {
            read(data,size);
        }
    }

    ___noinline bool AstSerializer::trySerialize ( const callable<void(AstSerializer&)> &cb ) noexcept {
        try {
            cb(*this);
            return true;
        } catch ( const std::runtime_error & ) {
            failed = true;
            return false;
        }
    }

    #define HASH_TAG(tag)   tag,hash_tag(tag)

    void AstSerializer::tag ( const char * name, uint32_t hash ) {
        if ( writing ) {
            *this << hash;
        } else  {
            uint32_t hash2 = 0;
            *this << hash2;
            if ( hash != hash2 ) {
                throw_formatted_error("ast serializer tag '%s' mismatch", name);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////

    // Encode numbers by their size:
    //      0...254 (just value) => 1 byte
    //      254 (tag) + 2 bytes value => 3 bytes
    //      255 (tag) + 4 bytes value => 5 bytes
    void AstSerializer::serializeAdaptiveSize32 ( uint32_t & size ) {
        if ( writing ) {
            if ( size < 254 ) {
                uint8_t sz = static_cast<uint8_t>(size);
                *this << sz;
            } else if ( size <= 65535 ) {
                uint8_t tag = 254;
                uint16_t sz = static_cast<uint16_t>(size);
                *this << tag << sz;
            } else {
                uint8_t tag = 255;
                uint32_t sz = static_cast<uint32_t>(size);
                *this << tag << sz;
            }
        } else {
            uint8_t tag = 0; *this << tag;
            if ( tag < 254 ) {
                size = tag;
            } else if ( tag == 254 ) {
                uint16_t sz = 0; *this << sz;
                size = sz;
            } else {
                uint32_t sz = 0; *this << sz;
                size = sz;
            }
        }
    }

    void AstSerializer::serializeAdaptiveSize64 ( uint64_t & size ) {
        SERIALIZER_VERIFYF(size < (uint64_t(1) << 32), "number too large");
        if ( writing ) {
            uint32_t sz = static_cast<uint32_t>(size);
            serializeAdaptiveSize32(sz);
        } else {
            uint32_t sz; serializeAdaptiveSize32(sz);
            size = sz;
        }
    }

    AstSerializer & AstSerializer::operator << ( string & str ) {
        dtag(HASH_TAG("string"));
        if ( writing ) {
            uint64_t size = str.size();
            serializeAdaptiveSize64(size);
            write((void *)str.data(), size);
        } else {
            uint64_t size = 0;
            serializeAdaptiveSize64(size);
            str.resize(size);
            read(&str[0], size);
        }
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( const char * & value ) {
        dtag(HASH_TAG("const char *"));
        bool is_null = value == nullptr;
        *this << is_null;
        if ( is_null ) {
            if ( !writing ) value = nullptr;
            return *this;
        }
        if ( writing ) {
            uint64_t len = strlen(value);
            serializeAdaptiveSize64(len);
            write(static_cast<const void*>(value), len);
        } else {
            uint64_t len = 0;
            serializeAdaptiveSize64(len);
            auto data = new char [len + 1]();
            read(static_cast<void*>(data), len);
            data[len] = '\0';
            value = data;
        }
        return *this;
    }

    template <typename V>
    AstSerializer & AstSerializer::operator << ( safebox<V> & box ) {
        dtag(HASH_TAG("Safebox"));
        if ( writing ) {
            uint64_t size = box.unlocked_size(); *this << size;
            box.foreach_with_hash ([&](smart_ptr<V> obj, uint64_t hash) {
                *this << hash << obj;
            });
            return *this;
        }
        uint64_t size = 0; *this << size;
        safebox<V> deser;
        for ( uint64_t i = 0; i < size; i++ ) {
            smart_ptr<V> obj; uint64_t hash = 0;
            *this << hash << obj;
            deser.insert(hash, obj);
        }
        box = das::move(deser);
        return *this;
    }

    template <typename K, typename V, typename H, typename E>
    void AstSerializer::serialize_hash_map ( das_hash_map<K, V, H, E> & value ) {
        dtag(HASH_TAG("DasHashmap"));
        if ( writing ) {
            uint64_t size = value.size(); *this << size;
            for ( auto & item : value ) {
                *this << item.first << item.second;
            }
            return;
        }
        uint64_t size = 0; *this << size;
        das_hash_map<K, V, H, E> deser;
        deser.reserve(size);
        for ( uint64_t i = 0; i < size; i++ ) {
            K k; V v; *this << k << v;
            deser.emplace(das::move(k),das::move(v));
        }
        value = das::move(deser);
    }

    template <typename K, typename V>
    AstSerializer & AstSerializer::operator << ( das_hash_map<K, V> & value ) {
        serialize_hash_map<K, V, hash<K>, equal_to<K>>(value);
        return *this;
    }

    template <typename V>
    AstSerializer & AstSerializer::operator << ( safebox_map<V> & box ) {
        serialize_hash_map<uint64_t, V, skip_hash, das::equal_to<uint64_t>>(box);
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( Type & baseType ) {
        dtag(HASH_TAG("Type"));
        serialize_small_enum(baseType);
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( ExpressionPtr & expr ) {
        dtag(HASH_TAG("ExpressionPtr"));
        bool is_null = expr == nullptr;
        *this << is_null;
        if ( is_null ) {
            if ( !writing ) expr = nullptr;
            return *this;
        }
        if ( writing ) {
            uint32_t rtti = hash_tag(expr->__rtti);
            DAS_ASSERT(rtti);
            *this << rtti;
            expr->serialize(*this);
        } else {
            uint32_t rtti = 0; *this << rtti;
            auto itA = rttiHash2Annotation.find(rtti);
            SERIALIZER_VERIFYF(itA != rttiHash2Annotation.end(), "annotation '%u' is not found", rtti);
            auto annotation = itA->second;
            expr.reset((Expression *) static_cast<TypeAnnotation*>(annotation)->factory());
            expr->serialize(*this);
        }
        dtag(HASH_TAG("/ExpressionPtr"));
        return *this;
    }

    bool AstSerializer::isInThisModule ( Function * & ptr ) {
        return ptr->module == thisModule;
    }

    bool AstSerializer::isInThisModule ( Enumeration * & ptr ) {
        return ptr->module == thisModule;
    }

    bool AstSerializer::isInThisModule ( Structure * & ptr ) {
        return ptr->module == thisModule;
    }

    bool AstSerializer::isInThisModule ( Variable * & ptr ) {
        return ptr->module == thisModule || ptr->module == nullptr;;
    }

    bool AstSerializer::isInThisModule ( TypeInfoMacro * & ptr ) {
        return ptr->module == thisModule;
    }

    void AstSerializer::writeIdentifications ( Function * & func ) {
        string mangeldName = func->getMangledName();
        uint64_t moduleName = func->module->nameHash;
        *this << moduleName << mangeldName;
    }

    void AstSerializer::writeIdentifications ( Enumeration * & ptr ) {
        *this << ptr->module->nameHash;
        uint64_t nameHash = hash64z(ptr->name.c_str());
        *this << nameHash;
    }

    void AstSerializer::writeIdentifications ( Structure * & ptr ) {
        *this << ptr->module->nameHash;
        uint64_t nameHash = hash64z(ptr->name.c_str());
        *this << nameHash;
    }

    void AstSerializer::writeIdentifications ( Variable * & ptr ) {
        *this << ptr->module->nameHash << ptr->name;
    }

    void AstSerializer::writeIdentifications ( TypeInfoMacro * & ptr ) {
        *this << ptr->module->nameHash << ptr->name;
    }

    void AstSerializer::fillOrPatchLater ( Function * & func, uint64_t id ) {
        auto it = smartFunctionMap.find(id);
        if ( it == smartFunctionMap.end() ) {
            func = ( Function * ) 1;
            functionRefs.emplace_back(&func, id);
        } else {
            func = it->second.get();
        }
    }

    void AstSerializer::fillOrPatchLater ( Enumeration * & ptr, uint64_t id ) {
        auto it = smartEnumerationMap.find(id);
        if ( it == smartEnumerationMap.end() ) {
            ptr = ( Enumeration * ) 1;
            enumerationRefs.emplace_back(&ptr, id);
        } else {
            ptr = it->second.get();
        }
    }

    void AstSerializer::fillOrPatchLater ( Structure * & ptr, uint64_t id ) {
        auto it = smartStructureMap.find(id);
        if ( it == smartStructureMap.end() ) {
            ptr = ( Structure * ) 1;
            structureRefs.emplace_back(&ptr, id);
        } else {
            ptr = it->second.get();
        }
    }

    void AstSerializer::fillOrPatchLater ( Variable * & ptr, uint64_t id ) {
        auto it = smartVariableMap.find(id);
        if ( it == smartVariableMap.end() ) {
            ptr = ( Variable * ) 1;
            variableRefs.emplace_back(&ptr, id);
        } else {
            ptr = it->second.get();
        }
    }

    auto AstSerializer::readModuleAndNameHash () -> pair<Module *, uint64_t> {
        uint64_t moduleNameHash = 0;
        uint64_t mangledNameHash = 0;
        *this << moduleNameHash << mangledNameHash;
        auto funcModule = moduleLibrary->findModuleByMangledNameHash(moduleNameHash);
        SERIALIZER_VERIFYF(ignoreEmptyExternal || funcModule, "module '%llu' is not found", moduleNameHash);
        return {funcModule, mangledNameHash};
    }

    auto AstSerializer::readModuleAndName () -> pair<Module *, string> {
        uint64_t moduleNameHash = 0;
        string mangledName;
        *this << moduleNameHash << mangledName;
        auto funcModule = moduleLibrary->findModuleByMangledNameHash(moduleNameHash);
        SERIALIZER_VERIFYF(ignoreEmptyExternal || funcModule, "module '%llu' is not found", moduleNameHash);
        return {funcModule, mangledName};
    }

    void AstSerializer::findExternal ( Function * & func ) {
        auto [funcModule, mangledName] = readModuleAndName();
        if ( !funcModule ) {
          func = nullptr;
          return;
        }
        auto f = funcModule->findFunction(mangledName);
        if ( f ) {
            func = f.get();
        } else if ( auto genericFn = funcModule->findGeneric(mangledName) ) {
            func = genericFn.get();
        }
        if ( func == nullptr && funcModule->wasParsedNameless ) {
            string modname, funcname;
            splitTypeName(mangledName, modname, funcname);
            func = funcModule->findFunction(funcname).get();
        }
        if ( func == nullptr ) {
            failed = true;
            SERIALIZER_VERIFYF(false, "function '%s' not found", mangledName.c_str());
        }
    }

    void AstSerializer::findExternal ( Enumeration * & ptr ) {
        auto [mod, mangledNameHash] = readModuleAndNameHash();
        ptr = mod->findEnumByMangledNameHash(mangledNameHash).get();
        SERIALIZER_VERIFYF(ptr!=nullptr, "enumeration '%llu' is not found", mangledNameHash);
    }

    void AstSerializer::findExternal ( Structure * & ptr ) {
        auto [mod, mangledNameHash] = readModuleAndNameHash();
        ptr = mod->findStructureByMangledNameHash(mangledNameHash).get();
        SERIALIZER_VERIFYF(ptr!=nullptr, "structure '%llu' is not found", mangledNameHash);
    }

    void AstSerializer::findExternal ( Variable * & ptr ) {
        auto [mod, mangledName] = readModuleAndName();
        ptr = mod->findVariable(mangledName).get();
        SERIALIZER_VERIFYF(ptr!=nullptr, "variable '%s' is not found", mangledName.c_str());
    }

    void AstSerializer::findExternal ( TypeInfoMacro * & ptr ) {
        auto [mod, mangledName] = readModuleAndName();
        ptr = mod->findTypeInfoMacro(mangledName).get();
        SERIALIZER_VERIFYF(ptr!=nullptr, "variable '%s' is not found", mangledName.c_str());
    }

    template<typename TT>
    AstSerializer & AstSerializer::serializePointer ( TT * & ptr ) {
        uint64_t fid = uintptr_t(ptr);
        *this << fid;
        if ( !fid ) {
            if ( !writing ) ptr = nullptr;
            return *this;
        }
        if ( writing ) {
            bool inThisModule = isInThisModule(ptr);
            *this << inThisModule;
            if ( !inThisModule )
                writeIdentifications(ptr);
        } else {
            bool inThisModule = false;
            *this << inThisModule;
            if ( inThisModule )
                fillOrPatchLater(ptr, fid);
            else
                findExternal(ptr);
        }
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( Function * & ptr ) {
        dtag(HASH_TAG("Function pointer"));
        return serializePointer(ptr);
    }

    AstSerializer & AstSerializer::operator << ( Structure * & ptr ) {
        dtag(HASH_TAG("Structure pointer"));
        return serializePointer(ptr);
    }

    AstSerializer & AstSerializer::operator << ( Enumeration * & ptr ) {
        dtag(HASH_TAG("Enumeration pointer"));
        return serializePointer(ptr);
    }

    AstSerializer & AstSerializer::operator << ( Variable * & ptr ) {
        dtag(HASH_TAG("Variable pointer"));
        return serializePointer(ptr);
    }

    AstSerializer & AstSerializer::operator << ( FunctionPtr & func ) {
        dtag(HASH_TAG("FunctionPtr"));
        if ( writing && func ) {
            SERIALIZER_VERIFYF(!func->builtIn, "cannot serialize built-in function");
        }
        serializeSmartPtr(func, smartFunctionMap);
        if ( func ) {
            if ( writing ) {
                string name = func->name;
                *this << name;
            } else {
                string name; *this << name;
                string expect = func->name;
                SERIALIZER_VERIFYF(name == expect, "expected different function");
            }
        }
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( TypeInfoMacro * & ptr ) {
        dtag(HASH_TAG("TypeInfoMacroPtr"));
        uint64_t id = uintptr_t(ptr);
        *this << id;
        if ( !id ) {
            if ( !writing ) ptr = nullptr;
            return *this;
        }
        if ( writing ) {
            bool inThisModule = isInThisModule(ptr);
            *this << inThisModule;
            SERIALIZER_VERIFYF(!inThisModule, "Unexpected typeinfo macro from the current module");
            writeIdentifications(ptr);
        } else {
            bool inThisModule = false;
            *this << inThisModule;
            SERIALIZER_VERIFYF(!inThisModule, "Unexpected typeinfo macro from the current module");
            findExternal(ptr);
        }
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( TypeDeclPtr & type ) {
        dtag(HASH_TAG("TypeDeclPtr"));
        bool is_null = type == nullptr;
        *this << is_null;
        if ( is_null ) {
            if ( !writing ) type = nullptr;
            return *this;
        }
        uint64_t id = intptr_t(type.get());
        *this << id;
        if ( writing ) {
            if ( smartTypeDeclMap[id] == nullptr ) {
                smartTypeDeclMap[id] = type;
                type->serialize(*this);
            }
        } else {
            if ( smartTypeDeclMap[id] == nullptr ) {
                type = make_smart<TypeDecl>();
                smartTypeDeclMap[id] = type;
                type->serialize(*this);
            } else {
                type = smartTypeDeclMap[id];
            }
        }
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( AnnotationArgument & arg ) {
        dtag(HASH_TAG("AnnotationArgument"));
        arg.serialize(*this);
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( AnnotationDeclarationPtr & annotation_decl ) {
        dtag(HASH_TAG("AnnotationDeclarationPtr"));
        if ( !writing ) annotation_decl = make_smart<AnnotationDeclaration>();
        annotation_decl->serialize(*this);
        return *this;
    }

    bool isLogicAnnotation ( string & name ) {
        return name == "||" || name == "&&" || name == "!" || name == "^^";
    }

    LogicAnnotationOp makeOpFromName ( string & name ) {
        switch ( name[0] ) {
        case '|':    return LogicAnnotationOp::Or;
        case '&':    return LogicAnnotationOp::And;
        case '!':    return LogicAnnotationOp::Not;
        case '^':    return LogicAnnotationOp::Xor;
        default: SERIALIZER_VERIFYF(false, "expected to be called on logic annotation name");
        }
        abort(); // warning: function does not return on all control paths
    }

    void serializeAnnotationPointer ( AstSerializer & ser, AnnotationPtr & anno ) {
        bool is_null = anno == nullptr;
        ser << is_null;
        if ( is_null ) {
            if ( !ser.writing ) anno = nullptr;
            return;
        }
        if ( ser.writing ) {
            bool inThisModule = anno->module == ser.thisModule;
            ser << inThisModule;
            if ( !inThisModule ) {
                ser << anno->name;
                if ( isLogicAnnotation(anno->name) ) {
                    LogicAnnotationOp op = makeOpFromName(anno->name);
                    ser.serialize_enum(op);
                    anno->serialize(ser);
                } else {
                    ser << anno->module->nameHash;
                }
            } else {
                // If the macro is from current module, do nothing
                // it will probably take care of itself during compilation
                SERIALIZER_VERIFYF( anno->module->macroContext,
                    "expected to see macro module '%s'", anno->module->name.c_str()
                );
            }
        } else {
            bool inThisModule = false;
            ser << inThisModule;
            if ( !inThisModule ) {
                string name;
                ser << name;
                if ( isLogicAnnotation(name) ) {
                    LogicAnnotationOp op; ser.serialize_enum(op);
                    anno = newLogicAnnotation(op);
                    anno->serialize(ser);
                } else {
                    uint64_t moduleNameHash = 0;
                    ser << moduleNameHash;
                    auto mod = ser.moduleLibrary->findModuleByMangledNameHash(moduleNameHash);
                    SERIALIZER_VERIFYF(mod!=nullptr, "module '%llu' is not found", moduleNameHash);
                    anno = mod->findAnnotation(name).get();
                    SERIALIZER_VERIFYF(anno!=nullptr, "annotation '%s' is not found", name.c_str());
                }
            }
        }
    }

    AstSerializer & AstSerializer::operator << ( AnnotationPtr & anno ) {
        dtag(HASH_TAG("AnnotationPtr"));
        serializeAnnotationPointer(*this, anno);
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( Structure::FieldDeclaration & field_declaration ) {
        field_declaration.serialize(*this);
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( LineInfo & at ) {
        dtag(HASH_TAG("LineInfo"));
        *this << at.fileInfo;

        serializeAdaptiveSize32(at.line);

        if ( writing ) {
            uint32_t diff = at.last_line - at.line;
            serializeAdaptiveSize32(diff);
        } else {
            uint32_t diff; serializeAdaptiveSize32(diff);
            at.last_line = at.line + diff;
        }

        // if ( writing ) {
        //     DAS_ASSERTF(at.column <= 255 && at.last_column <= 255, "unexpected long line");
        //     uint8_t column = at.column, last_column = at.last_column;
        //     *this << column << last_column;
        // } else {
        //     uint8_t column, last_column;
        //     *this << column << last_column;
        //     at.column = column; at.last_column = last_column;
        // }

        return *this;
    }

    AstSerializer & AstSerializer::operator << ( FileInfo * & info ) {
        dtag(HASH_TAG("FileInfo *"));
        bool is_null = info == nullptr;
        *this << is_null;
        if ( is_null ) {
            if ( !writing ) { info = nullptr; }
            return *this;
        }
        if ( writing ) {
            if ( writingFileInfoMap[info] == 0 ) {
                uint64_t curOffset = buffer->writingSize() + sizeof(curOffset);
                *this << curOffset;
                writingFileInfoMap[info] = curOffset;
                info->serialize(*this);
            } else {
                *this << writingFileInfoMap[info];
            }
        } else {
            uint64_t curOffset = 0; *this << curOffset;
            if ( readingFileInfoMap[curOffset] == nullptr ) {
                uint64_t savedOffset = readOffset;
                readOffset = curOffset;
                uint8_t tag = 0; *this << tag;
                switch ( tag ) {
                    case 0: info = new FileInfo; break;
                    case 1: info = new TextFileInfo; break;
                    default: SERIALIZER_VERIFYF(false, "Unreachable");
                }
                info->serialize(*this);
                readingFileInfoMap[curOffset] = info;
                if ( curOffset != savedOffset )
                    readOffset = savedOffset;
            } else {
                info = readingFileInfoMap[curOffset];
            }
        }
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( FileInfoPtr & info ) {
        dtag(HASH_TAG("FileInfoPtr"));
        if ( writing ) {
            FileInfo * info_ptr = info.get();
            *this << info_ptr;
        } else {
            FileInfo * info_ptr = nullptr; *this << info_ptr;
            info.reset(info_ptr);
            doNotDelete.insert(info_ptr);
        }
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( StructurePtr & struct_ ) {
        serializeSmartPtr(struct_, smartStructureMap);
        return *this;
    }

    void FileAccess::serialize ( AstSerializer & ser ) {
        if ( ser.writing ) {
            uint8_t tag = 0;
            ser << tag;
        }
        ser << files;
    }

    void ModuleFileAccess::serialize ( AstSerializer & ser ) {
        if ( ser.writing ) {
            uint8_t tag = 1;
            ser << tag;
        }
        ser << files;
    }

    AstSerializer & AstSerializer::operator << ( FileAccessPtr & ptr ) {
        dtag(HASH_TAG("FileAccessPtr"));
        bool is_null = ptr == nullptr;
        *this << is_null;
        if ( is_null ) {
            if ( !writing ) ptr = nullptr;
            return *this;
        }
        if ( writing ) {
            uint64_t p = (uint64_t) ptr.get();
            *this << p;
            if ( fileAccessMap[p] == nullptr ) {
                fileAccessMap[p] = ptr.get();
                ptr->serialize(*this);
            }
        } else {
            uint64_t p = 0; *this << p;
            if ( fileAccessMap[p] == nullptr ) {
                uint8_t tag = 0; *this << tag;
                switch ( tag ) {
                    case 0: ptr = make_smart<FileAccess>(); break;
                    case 1: ptr = make_smart<ModuleFileAccess>(); break;
                    default: SERIALIZER_VERIFYF(false, "Unreachable");
                }
                fileAccessMap[p] = ptr.get();
                ptr->serialize(*this);
            } else {
                ptr.orphan();
                FileAccessPtr t = fileAccessMap[p];
                ptr = t;
            }
        }
        return *this;
    }

    // This method creates concrete (i.e. non-polymorphic types without duplications)
    template<typename T>
    void AstSerializer::serializeSmartPtr( smart_ptr<T> & obj, das_hash_map<uint64_t, smart_ptr<T>> & objMap) {
        uint64_t id = uint64_t(uintptr_t(obj.get()));
        *this << id;
        if ( id == 0 ) {
            if ( !writing ) obj = nullptr;
            return;
        }
        if ( writing ) {
            if ( objMap.find(id) == objMap.end() ) {
                objMap[id] = obj;
                obj->serialize(*this);
            }
        } else {
            auto it = objMap.find(id);
            if ( it == objMap.end() ) {
                obj = make_smart<T>();
                objMap[id] = obj;
                obj->serialize(*this);
            } else {
                obj = it->second;
            }
        }
    }

    AstSerializer & AstSerializer::operator << ( EnumerationPtr & enum_type ) {
        if ( writing ) {
            bool builtin = enum_type->module->builtIn && !enum_type->module->promoted;
            *this << builtin;
            if ( builtin ) {
                uint64_t module = enum_type->module->nameHash;
                string name = enum_type->name;
                *this << module << name;
            } else {
                serializeSmartPtr(enum_type, smartEnumerationMap);
            }
        } else {
            bool builtin = false;
            *this << builtin;
            if ( builtin ) {
                uint64_t module = 0;
                string name;
                *this << module << name;
                auto pModule = this->moduleLibrary->findModuleByMangledNameHash(module);
                SERIALIZER_VERIFYF(pModule, "expected to find module '%llu'", module);
                enum_type = pModule->findEnum(name);
                SERIALIZER_VERIFYF(enum_type, "expected to find enumeration '%llu'::'%s'", module, name.c_str());
            } else {
                serializeSmartPtr(enum_type, smartEnumerationMap);
                SERIALIZER_VERIFYF(enum_type, "expected to find enumeration");
            }
        }

        return *this;
    }

    AstSerializer & AstSerializer::operator << ( Enumeration::EnumEntry & entry ) {
        entry.serialize(*this);
        return *this;
    }

    // Note for review: this is the usual downward serialization, no need to backpatch
    AstSerializer & AstSerializer::operator << ( TypeAnnotationPtr & type_anno ) {
        AnnotationPtr a = static_pointer_cast<Annotation>(type_anno);
        *this << a;
        type_anno = static_pointer_cast<TypeAnnotation>(a);
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( TypeAnnotation * & type_anno ) {
        TypeAnnotationPtr t = type_anno;
        *this << t;
        type_anno = t.get();
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( VariablePtr & var ) {
        serializeSmartPtr(var, smartVariableMap);
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( Module * & module ) {
        bool is_null = module == nullptr;
        *this << is_null;
        if ( writing ) {
            if ( !is_null ) {
                *this << module->nameHash;
            }
        } else {
            if ( !is_null ) {
                uint64_t nameHash; *this << nameHash;
                module = moduleLibrary->findModuleByMangledNameHash(nameHash);
                SERIALIZER_VERIFYF(module, "expected to fetch module %llu from library", nameHash);
            } else {
                module = nullptr;
            }
        }
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( Function::AliasInfo & alias_info ) {
        alias_info.serialize(*this);
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( ReaderMacroPtr & ptr ) {
        dtag(HASH_TAG("ReaderMacroPtr"));
        if ( writing ) {
            SERIALIZER_VERIFYF(ptr, "did not expext to see null ReaderMacroPtr");
            SERIALIZER_VERIFYF(!(ptr->module == thisModule), "did not expect to find macro from the current module");
            *this << ptr->module->nameHash;
            *this << ptr->name;
        } else {
            uint64_t moduleNameHash = 0;
            string name;
            *this << moduleNameHash << name;
            auto mod = moduleLibrary->findModuleByMangledNameHash(moduleNameHash);
            SERIALIZER_VERIFYF(mod!=nullptr, "module '%llu' not found", moduleNameHash);
            ptr = mod->findReaderMacro(name);
            SERIALIZER_VERIFYF(ptr, "Reader macro '%s' not found in the module '%s'",
                name.c_str(), mod->name.c_str()
            );
        }
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( ExprBlock * & block ) {
        dtag(HASH_TAG("ExprBlock*"));
        void * addr = block;
        *this << addr;
        if ( !writing && addr ) {
            block = ( ExprBlock * ) 1;
            blockRefs.emplace_back(&block, (uint64_t) addr);
        }
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( InferHistory & history ) {
        dtag(HASH_TAG("InferHistory"));
        history.serialize(*this);
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( CaptureEntry & entry ) {
        *this << entry.name;
        serialize_enum<CaptureMode>(entry.mode);
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( MakeFieldDeclPtr & ptr ) {
        serializeSmartPtr(ptr, smartMakeFieldDeclMap);
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( MakeStructPtr & ptr ) {
        serializeSmartPtr(ptr, smartMakeStructMap);
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( Module & module ) {
        thisModule = &module;
        module.serialize(*this, /*already_exists*/false);
        return *this;
    }

    AstSerializer & AstSerializer::serializeModule ( Module & module, bool already_exists ) {
        thisModule = &module;
        module.serialize(*this, already_exists);
        return *this;
    }

// typedecl

    #define DAS_VERIFYF_MULTI(...) do {                     \
        int arr[] = {__VA_ARGS__};                          \
        for(size_t i = 0; i < sizeof(arr)/sizeof(int); ++i) {  \
            DAS_VERIFYF(arr[i], "not expected to see");     \
        }                                                   \
    } while(0)

    void TypeDecl::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("TypeDecl"));
        ser << baseType;
        switch ( baseType ) {
            case Type::typeMacro:
            case Type::typeDecl:
                ser << alias << dimExpr;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !firstType, !secondType,
                                argTypes.empty(), argNames.empty());
                break;
            case Type::alias:
                ser << alias << firstType << dim << dimExpr;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !secondType,
                                !alias.empty(), argTypes.empty(), argNames.empty());
                break;
            case option:
                ser << argTypes << dim << dimExpr;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !firstType, !secondType,
                                alias.empty(), !argTypes.empty(), argNames.empty());
                break;
            case autoinfer:
                ser << dim << dimExpr << alias;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !firstType, !secondType,
                                argTypes.empty(), argNames.empty());
                break;
            case fakeContext:
            case fakeLineInfo:
            case none:
            case anyArgument:
            case tVoid:
            case tBool:
            case tInt8:
            case tUInt8:
            case tInt16:
            case tUInt16:
            case tInt64:
            case tUInt64:
            case tInt:
            case tInt2:
            case tInt3:
            case tInt4:
            case tUInt:
            case tUInt2:
            case tUInt3:
            case tUInt4:
            case tFloat:
            case tFloat2:
            case tFloat3:
            case tFloat4:
            case tDouble:
            case tString:
                ser << alias << dim << dimExpr;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !firstType, !secondType,
                                argTypes.empty(), argNames.empty());
                break;
            case tRange:
            case tURange:
            case tRange64:
            case tURange64: // blow up!
                ser << alias << dim << dimExpr;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !firstType, !secondType,
                                argTypes.empty(), argNames.empty());
                break;
            case tStructure:
                ser << alias << structType << dim << dimExpr;
                DAS_VERIFYF_MULTI(!annotation, !!structType, !enumType, !firstType, !secondType,
                                argTypes.empty(), argNames.empty());
                break;
            case tHandle:
                ser << alias << annotation << dim << dimExpr;
                DAS_VERIFYF_MULTI(!!annotation, !structType, !enumType, !firstType, !secondType,
                                argTypes.empty(), argNames.empty());
                break;
            case tEnumeration:
            case tEnumeration8:
            case tEnumeration16:
            case tEnumeration64:
                ser << alias << enumType << dim << dimExpr;
                DAS_VERIFYF_MULTI(!annotation, !structType, !!enumType, !firstType, !secondType,
                                argTypes.empty(), argNames.empty());
                break;
            case tBitfield:  // blow up!
                ser << alias << argNames << dim << dimExpr;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !firstType, !secondType,
                                argTypes.empty());
                break;
            case tIterator:
            case tPointer:
            case tArray: // blow up!
                ser << alias << firstType << dim << dimExpr;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !secondType,
                                argTypes.empty(), argNames.empty());
                break;
            case tFunction:
            case tLambda:
            case tBlock:
                ser << alias << firstType << argTypes << argNames << dim << dimExpr;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !secondType);
                break;
            case tTable:
                ser << alias << firstType << secondType << dim << dimExpr;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !!firstType,
                                argTypes.empty(), argNames.empty());
                break;
            case tTuple:
            case tVariant:
                ser << alias << argTypes << argNames << dim << dimExpr;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !firstType, !secondType,
                                !argTypes.empty());
                break;
            default:
                SERIALIZER_VERIFYF(false,  "not expected to be here");
                break;
        }

        ser << flags << at << module;
    }

    void AnnotationArgument::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("AnnotationArgument"));
        ser << type << name << sValue << iValue << at;
    }

    void AnnotationArgumentList::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("AnnotationArgumentList"));
        ser << * static_cast <AnnotationArguments *> (this);
    }

    void AnnotationDeclaration::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("AnnotationDeclaration"));
        ser << annotation << arguments << at << flags;
        ptr_ref_count::serialize(ser);
    }

    void ptr_ref_count::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("ptr_ref_count"));
        // Do nothing
    }

    void Structure::FieldDeclaration::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("FieldDeclaration"));
        ser << name << at;
        ser << type;
        ser.ignoreEmptyExternal = true;
        ser << init;
        ser.ignoreEmptyExternal = false;
        ser << annotation << offset << flags;
    }

    void Enumeration::EnumEntry::serialize( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("EnumEntry"));
        ser << name << cppName << at << value;
    }

    void serializeAnnotationList ( AstSerializer & ser, AnnotationList & list ) {
        if ( ser.writing ) {
            uint64_t size = 0;
        // count the real size without generated annotations
            for ( auto & it : list ) {
                bool inThisModule = it->annotation->module == ser.thisModule;
                if ( !inThisModule ) { size += 1; }
            }
            ser << size;
            for ( auto & it : list ) {
                bool inThisModule = it->annotation->module == ser.thisModule;
                if ( !inThisModule ) { ser << it; }
            }
        } else {
            uint64_t size = 0; ser << size;
            AnnotationList result; result.resize(size);
            for ( uint64_t i = 0; i < size; i++ ) {
                ser << result[i];
            }
            list = das::move(result);
        }
    }

    void Enumeration::serialize ( AstSerializer & ser ) {
        ser.tag(HASH_TAG("Enumeration"));
        ser << name     << cppName  << at << list << module
            << external << baseType << isPrivate;
        serializeAnnotationList(ser, annotations);
        ptr_ref_count::serialize(ser);
    }

    void Structure::serialize ( AstSerializer & ser ) {
        ser.tag(HASH_TAG("Structure"));
        ser << name;
        ser << at     << module;
        ser << fields << fieldLookup;
        ser << parent // parent could be in the current module or in some other
                      // module
            << flags
            << ownSemanticHash;
        serializeAnnotationList(ser, annotations);
        ptr_ref_count::serialize(ser);
    }

    void Variable::serialize ( AstSerializer & ser ) {
        ser.tag(HASH_TAG("Variable"));
        ser << name << aka << type << init << source << at << index << stackTop
            << extraLocalOffset << module
            << initStackSize << flags << access_flags << annotation;
        ptr_ref_count::serialize(ser);
    }

    void Function::AliasInfo::serialize ( AstSerializer & ser ) {
        ser.tag(HASH_TAG("AliasInfo"));
        ser << var << func << viaPointer;
    }

    void InferHistory::serialize ( AstSerializer & ser ) {
        ser.tag(HASH_TAG("InferHistory"));
        ser << at << func;
    }

// function

    void Function::serialize ( AstSerializer & ser ) {
        ser.tag(HASH_TAG("Function"));
        ser << name;
        // Note: important fields are placed separately for easier debugging
        serializeAnnotationList(ser, annotations);
        ser.ignoreEmptyExternal = true;
        ser << arguments;
        ser.ignoreEmptyExternal = false;
        ser << result;
        ser << body;
        ser << classParent;
        //ser << fromGeneric;
        ser << index         << totalStackSize  << totalGenLabel;
        ser << at            << atDecl          << module;
        ser << hash          << aotHash;  // do not serialize inferStack
        ser << resultAliases << argumentAliases << resultAliasesGlobals;
        ser << flags         << moreFlags       << sideEffectFlags;
    }

// Expressions

    void ExprReader::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("ExprReader"));
        Expression::serialize(ser);
        ser << macro << sequence;
    }

    void ExprLabel::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("ExprLabel"));
        Expression::serialize(ser);
        ser << label << comment;
    }

    void ExprGoto::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("ExprGoto"));
        Expression::serialize(ser);
        ser << label << subexpr;
    }

    void ExprRef2Value::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("ExprRef2Value"));
        Expression::serialize(ser);
        ser << subexpr;
    }

    void ExprRef2Ptr::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("ExprRef2Ptr"));
        Expression::serialize(ser);
        ser << subexpr;
    }

    void ExprPtr2Ref::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("ExprPtr2Ref"));
        Expression::serialize(ser);
        ser << subexpr << unsafeDeref << assumeNoAlias;
    }

    void ExprAddr::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("ExprAddr"));
        Expression::serialize(ser);
        ser << target << funcType << func;
    }

    void ExprNullCoalescing::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("ExprNullCoalescing"));
        ExprPtr2Ref::serialize(ser);
        ser << defaultValue;
    }

    void ExprDelete::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("ExprDelete"));
        Expression::serialize(ser);
        ser << subexpr << sizeexpr << native;
    }

    void ExprAt::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("ExprAt"));
        Expression::serialize(ser);
        ser << subexpr << index;
        ser << atFlags;
    }

    void ExprSafeAt::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("ExprSafeAt"));
        ExprAt::serialize(ser);
    }

    void ExprBlock::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("ExprBlock"));
        Expression::serialize(ser);

        if ( ser.writing ) {
            void * thisBlock = this;
            ser << thisBlock;
        } else {
            void * thisBlock = nullptr; ser << thisBlock;
            ser.exprBlockMap.emplace((uint64_t) thisBlock, this);
        }

        ser << list << finalList << returnType << arguments << stackTop
            << stackVarTop << stackVarBottom << stackCleanVars << maxLabelIndex
            << annotationData << annotationDataSid << blockFlags
            << inFunction;

        serializeAnnotationList(ser, annotations);
    }

    void ExprVar::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("ExprVar"));
        Expression::serialize(ser);

        ser << name << argumentIndex << varFlags;
        ser << pBlock;

        // The variable is smart_ptr but we actually need
        // non-owning semantics
        if ( ser.writing ) {
            bool inThisModule =  variable == nullptr // this happens with [generic] functions, for example
                      || variable->module == nullptr
                      || variable->module == ser.thisModule;
            ser << inThisModule;
            if ( inThisModule ) {
                ser << variable; // serialize as smart pointer
            } else {
                ser << variable->name;
                ser << variable->module->nameHash;
            }
        } else {

            bool inThisModule = false; ser << inThisModule;
            if ( inThisModule ) {
                ser << variable;
            } else {
                string varname;
                uint64_t modname = 0;
                ser << varname << modname;
                auto mod = ser.moduleLibrary->findModuleByMangledNameHash(modname);
                SERIALIZER_VERIFYF(mod, "expected to find module '%llu'", modname);
                variable = mod->findVariable(varname);
            }

        }
    }

    void ExprTag::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("ExprTag"));
        Expression::serialize(ser);
        ser << subexpr << value << name;
    }

    void ExprField::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("ExprField"));
        Expression::serialize(ser);
        ser << value      << name       << atField
            << fieldIndex << annotation << derefFlags
            << fieldFlags;

        if ( ser.writing ) {
            bool has_field = value->type && (
                value->type->isStructure() || ( value->type->isPointer() && value->type->firstType->isStructure() )
            );
            ser << has_field;
            if ( !has_field ) return;
            string mangledName;
            if ( value->type->isPointer() ) {
                SERIALIZER_VERIFYF(value->type->firstType->isStructure(), "expected to see structure field access via pointer");
                mangledName = value->type->firstType->structType->getMangledName();
                ser << value->type->firstType->structType->module;
            } else {
                SERIALIZER_VERIFYF(value->type->isStructure(), "expected to see structure field access");
                mangledName = value->type->structType->getMangledName();
                ser << value->type->structType->module;
            }
            ser << mangledName;
            if ( annotation != nullptr && annotation->getFieldOffset(name) == static_cast<uint32_t>(-1) ) {
                LOG(LogLevel::warning) << "das: serialize: Field '" << name << "' not found in '" << annotation->name << "'";
            }
        } else {
            if ( annotation != nullptr && annotation->getFieldOffset(name) == static_cast<uint32_t>(-1) ) {
                SERIALIZER_VERIFYF("Field '%s' not found in '%s'", name.c_str(), annotation->name.c_str());
            }
            bool has_field = false; ser << has_field;
            if ( !has_field ) return;
            Module * module = nullptr; ser << module;
            string mangledName; ser << mangledName;
            field = ( Structure::FieldDeclaration * ) 1;
            ser.fieldRefs.emplace_back(&field, module, das::move(mangledName), name);
        }
    }

    void ExprSafeAsVariant::serialize ( AstSerializer & ser ) {
        ExprField::serialize(ser);
        ser << skipQQ;
    }

    void ExprSwizzle::serialize ( AstSerializer & ser ) {
        Expression::serialize(ser);
        ser << value << mask << fields << fieldFlags;
    }

    void ExprSafeField::serialize ( AstSerializer & ser ) {
        ExprField::serialize(ser);
        ser << skipQQ;
    }

    void ExprLooksLikeCall::serialize ( AstSerializer & ser ) {
        Expression::serialize(ser);
        ser << name                   << arguments;
        ser << argumentsFailedToInfer << aliasSubstitution << atEnclosure;
    }

    void ExprCallMacro::serialize ( AstSerializer & ser ) {
        ExprLooksLikeCall::serialize(ser);
        ser << macro << inFunction;
    }

    void ExprCallFunc::serialize ( AstSerializer & ser ) {
        ExprLooksLikeCall::serialize(ser);
        ser << func << stackTop;
    }

    void ExprOp::serialize ( AstSerializer & ser ) {
        ExprCallFunc::serialize(ser);
        ser << op;
    }

    void ExprOp1::serialize ( AstSerializer & ser ) {
        ExprOp::serialize(ser);
        ser << subexpr;
    }

    void ExprOp2::serialize ( AstSerializer & ser ) {
        ExprOp::serialize(ser);
        ser << left;
        ser << right;
    }

    void ExprCopy::serialize ( AstSerializer & ser ) {
        ExprOp2::serialize(ser);
        ser << copyFlags;
    }

    void ExprMove::serialize ( AstSerializer & ser ) {
        ExprOp2::serialize(ser);
        ser << moveFlags;
    }

    void ExprClone::serialize ( AstSerializer & ser ) {
        ExprOp2::serialize(ser);
    }

    void ExprOp3::serialize ( AstSerializer & ser ) {
        ExprOp::serialize(ser);
        ser << subexpr << left << right;
    }

    void ExprTryCatch::serialize ( AstSerializer & ser ) {
        Expression::serialize(ser);
        ser << try_block << catch_block;
    }

    void ExprReturn::serialize ( AstSerializer & ser ) {
        Expression::serialize(ser);
        ser << subexpr    << returnFlags << stackTop << refStackTop
            << returnFunc << block       << returnType;
    }

    void ExprConst::serialize ( AstSerializer & ser ) {
        Expression::serialize(ser);
        ser << baseType << value << foldedNonConst;
    }

    void ExprConstPtr::serialize( AstSerializer & ser ) {
        ExprConstT<void *,ExprConstPtr>::serialize(ser);
        ser << isSmartPtr << ptrType;
    }

     void ExprConstEnumeration::serialize( AstSerializer & ser ) {
        ExprConst::serialize(ser);
        ser << enumType << text;
     }

    void ExprConstString::serialize(AstSerializer& ser) {
        ExprConst::serialize(ser);
        ser << text;
    }

    void ExprStringBuilder::serialize(AstSerializer& ser) {
        Expression::serialize(ser);
        ser << elements << stringBuilderFlags;
    }

    void ExprLet::serialize(AstSerializer& ser) {
        Expression::serialize(ser);
        ser << variables << visibility << atInit << letFlags;
    }

    void ExprFor::serialize(AstSerializer& ser) {
        Expression::serialize(ser);
        ser << iterators << iteratorsAka << iteratorsAt << iteratorsTupleExpansion << iteratorsTags
            << iteratorVariables << sources << body << visibility
            << allowIteratorOptimization << canShadow;
    }

    void ExprUnsafe::serialize(AstSerializer& ser) {
        Expression::serialize(ser);
        ser << body;
    }

    void ExprWhile::serialize(AstSerializer& ser) {
        Expression::serialize(ser);
        ser << cond << body;
    }

    void ExprWith::serialize(AstSerializer& ser) {
        Expression::serialize(ser);
        ser << with << body;
    }

    void ExprAssume::serialize(AstSerializer& ser) {
        Expression::serialize(ser);
        ser << alias << subexpr;
    }

    void ExprMakeBlock::serialize(AstSerializer & ser) {
        Expression::serialize(ser);
        ser << capture << block << stackTop << mmFlags;
    }

    void ExprMakeGenerator::serialize(AstSerializer & ser) {
        ExprLooksLikeCall::serialize(ser);
        ser << iterType << capture;
    }

    void ExprYield::serialize(AstSerializer & ser) {
        Expression::serialize(ser);
        ser << subexpr << returnFlags;
    }

    void ExprInvoke::serialize(AstSerializer & ser) {
        ExprLikeCall<ExprInvoke>::serialize(ser);
        ser << stackTop << doesNotNeedSp << isInvokeMethod << cmresAlias;
    }

    void ExprAssert::serialize(AstSerializer & ser) {
        ExprLikeCall<ExprAssert>::serialize(ser);
        ser << isVerify;
    }

    void ExprQuote::serialize(AstSerializer & ser) {
        ExprLikeCall<ExprQuote>::serialize(ser);
    }

    template <typename It, typename SimNodeT, bool first>
    void ExprTableKeysOrValues<It,SimNodeT,first>::serialize(AstSerializer & ser) {
        ExprLooksLikeCall::serialize(ser);
    }

    template <typename It, typename SimNodeT>
    void ExprArrayCallWithSizeOrIndex<It,SimNodeT>::serialize(AstSerializer & ser) {
        ExprLooksLikeCall::serialize(ser);
    }

    void ExprTypeInfo::serialize(AstSerializer & ser) {
        Expression::serialize(ser);
        ser << trait << subexpr << typeexpr << subtrait << extratrait << macro;
    }

    void ExprIs::serialize(AstSerializer & ser) {
        Expression::serialize(ser);
        ser << subexpr << typeexpr;
    }

    void ExprAscend::serialize(AstSerializer & ser) {
        Expression::serialize(ser);
        ser << subexpr << ascType << stackTop << ascendFlags;
    }

    void ExprCast::serialize(AstSerializer & ser) {
        Expression::serialize(ser);
        ser << subexpr << castType << castFlags;
    }

    void ExprNew::serialize(AstSerializer & ser) {
        ExprCallFunc::serialize(ser);
        ser << typeexpr << initializer;
    }

    void ExprCall::serialize(AstSerializer & ser) {
        ExprCallFunc::serialize(ser);
        ser << doesNotNeedSp << cmresAlias;
    }

    void ExprIfThenElse::serialize ( AstSerializer & ser ) {
        Expression::serialize(ser);
        ser << cond << if_true << if_false << ifFlags;
    }

    void MakeFieldDecl::serialize ( AstSerializer & ser ) {
        ser << at << name << value << tag << flags;
        ptr_ref_count::serialize(ser);
    }

    void MakeStruct::serialize( AstSerializer & ser ) {
        ser << static_cast <vector<MakeFieldDeclPtr> &> ( *this );
        ptr_ref_count::serialize(ser);
    }

    void ExprNamedCall::serialize ( AstSerializer & ser ) {
        Expression::serialize(ser);
        ser << name << nonNamedArguments << arguments << argumentsFailedToInfer;
    }

    void ExprMakeLocal::serialize ( AstSerializer & ser ) {
        Expression::serialize(ser);
        ser << makeType << stackTop << extraOffset << makeFlags;
    }

    void ExprMakeStruct::serialize ( AstSerializer & ser ) {
        ExprMakeLocal::serialize(ser);
        ser << structs << block << makeStructFlags << constructor;
    }

    void ExprMakeVariant::serialize ( AstSerializer & ser ) {
        ExprMakeLocal::serialize(ser);
        ser << variants;
    }

    void ExprMakeArray::serialize ( AstSerializer & ser ) {
        ExprMakeLocal::serialize(ser);
        ser << recordType << values << gen2;
    }

    void ExprMakeTuple::serialize ( AstSerializer & ser ) {
        ExprMakeArray::serialize(ser);
        ser << isKeyValue << recordNames;
    }

    void ExprArrayComprehension::serialize ( AstSerializer & ser ) {
        Expression::serialize(ser);
        ser << exprFor << exprWhere << subexpr << generatorSyntax << tableSyntax;
    }

    void ExprTypeDecl::serialize ( AstSerializer & ser ) {
        Expression::serialize(ser);
        ser << typeexpr;
    }

    void Expression::serialize ( AstSerializer & ser ) {
        ser << at
            << type
            << genFlags
            << flags
            << printFlags;
        ptr_ref_count::serialize(ser);
    }

    void FileInfo::serialize ( AstSerializer & ser ) {
        uint8_t tag = 0;
        if ( ser.writing ) {
            ser << tag;
        }
        ser << name << tabSize;
        if ( !ser.writing ) {
            ser.deleteUponFinish.push_back(this);
        }
        // Note: we do not serialize profileData
    }

    void TextFileInfo::serialize ( AstSerializer & ser ) {
        uint8_t tag = 1; // Signify the text file info
        if ( ser.writing ) {
            ser << tag;
        }
        ser << name << tabSize;
        ser.serializeAdaptiveSize32(sourceLength);
        if ( !ser.writing ) {
            ser.deleteUponFinish.push_back(this);
        }
        // ser << owner;
        // if ( ser.writing ) {
        //     ser.write((const void *) source, sourceLength);
        // } else {
        //     source = (char *) das_aligned_alloc16(sourceLength + 1);
        //     ser.read((void *) source, sourceLength);
        // }
    }

    AstSerializer & AstSerializer::operator << ( CallMacro * & ptr ) {
        dtag(HASH_TAG("CallMacro *"));
        if ( writing ) {
            SERIALIZER_VERIFYF ( ptr, "did not expect to see a nullptr CallMacro *" );
            SERIALIZER_VERIFYF ( !(ptr->module == thisModule), "did not expect to find macro from the current module" );
            *this << ptr->module->nameHash;
            *this << ptr->name;
        } else {
            uint64_t moduleName = 0;
            string name;
            *this << moduleName << name;
            auto mod = moduleLibrary->findModuleByMangledNameHash(moduleName);
            SERIALIZER_VERIFYF(mod!=nullptr, "module '%llu' not found", moduleName);
        // perform a litte dance to access the internal macro;
        // for details see: src/builtin/module_builtin_ast_adapters.cpp
        // 1564: void addModuleCallMacro ( .... CallMacroPtr & .... )
            auto callFactory = mod->findCall(name);
            SERIALIZER_VERIFYF(
                callFactory, "could not find CallMacro '%s' in the module '%s'",
                name.c_str(), mod->name.c_str()
            );
            auto exprLooksLikeCall = (*callFactory)({});
            SERIALIZER_VERIFYF(
                strncmp("ExprCallMacro", exprLooksLikeCall->__rtti, 14) == 0,
                "excepted to see an ExprCallMacro"
            );
            ptr = static_cast<ExprCallMacro *>(exprLooksLikeCall)->macro;
            delete exprLooksLikeCall;
        }
        return *this;
    }

    // Restores the internal state of macro module
    Module * reinstantiateMacroModuleState ( AstSerializer & /*ser*/, ProgramPtr program ) {
        TextWriter ignore_logs;
    // set the current module
    // create the module macro state
        program->isCompiling = false;
        program->markMacroSymbolUse();
        // program->deriveAliases(ignore_logs); // this info should already be there
        program->allocateStack(ignore_logs,true,false);
        program->makeMacroModule(ignore_logs);
    // unbind the module from the program
        return program->thisModule.release();
    }

    // Restores the internal state of macro module
    void finalizeModule ( AstSerializer & ser, ModuleLibrary & lib, Module * this_mod, bool already_exists ) {
        ProgramPtr program;

        if ( ser.failed ) return;
    // simulate macros
        if ( ser.writing ) {
            bool is_macro_module = this_mod->macroContext; // it's a macro module if it has macroContext
            ser << is_macro_module;
        } else {
            bool is_macro_module = false;
            ser << is_macro_module;
            if ( !already_exists ) {
                TextWriter ignore_logs;
                ReuseCacheGuard rcg;
            // initialize program
                program = make_smart<Program>();
                program->promoteToBuiltin = this_mod->promoted;;
                program->isDependency = true;
                program->thisModuleGroup = ser.thisModuleGroup;
                program->thisModuleName.clear();
                program->library.reset();
                program->policies.stack = 64 * 1024;
                program->thisModule.release();
                program->thisModule.reset(this_mod);
                lib.foreach([&] ( Module * pm ) {
                    program->library.addModule(pm);
                    return true;
                },"*");
            // always finalize annotations
                (*daScriptEnvironment::bound)->g_Program = program;
                program->finalizeAnnotations();

                if ( is_macro_module ) {
                    auto time0 = ref_time_ticks();
                    reinstantiateMacroModuleState(ser, program);
                    ser.totMacroTime += get_time_usec(time0);
                }
            // unbind the module from the program
                program->thisModule.release();
            }
        }
    }

    void serializeUseFunctions ( AstSerializer & ser, const FunctionPtr & f ) {
        ser.tag(HASH_TAG("serializeUseFunctions"));
        if ( ser.writing ) {
            string fname = f->name; ser << fname;
            uint64_t sz = f->useFunctions.size();
            ser << sz;
            for ( auto & usedFun : f->useFunctions ) {
                bool builtin = usedFun->module->builtIn;
                ser << builtin;
                if ( builtin ) {
                    uint64_t module = usedFun->module->nameHash;
                    uint64_t mnh = usedFun->getMangledNameHash();
                    ser << module << mnh;
                } else {
                    void * addr = usedFun;
                    ser << addr;
                }
            }
        } else {
            string fname; ser << fname;
            SERIALIZER_VERIFYF(fname == f->name, "expected to serialize in the same order");
            uint64_t size = 0; ser << size;
            f->useFunctions.reserve(size);
            for ( uint64_t i = 0; i < size; i++ ) {
                bool builtin = false;
                ser << builtin;
                if ( builtin ) {
                    uint64_t module = 0;
                    uint64_t mnh = 0;
                    ser << module << mnh;
                    auto pModule = ser.moduleLibrary->findModuleByMangledNameHash(module);
                    SERIALIZER_VERIFYF(pModule, "expected to find module '%llu'", module);
                    auto fun = pModule->findFunctionByMangledNameHash(mnh);
                    SERIALIZER_VERIFYF(fun, "expected to find function");
                    f->useFunctions.emplace(fun.get());
                } else {
                    void * addr = nullptr; ser << addr;
                    auto fun = ser.smartFunctionMap[(uint64_t)(uintptr_t) addr];
                    SERIALIZER_VERIFYF(fun, "expected to find function");
                    f->useFunctions.emplace(fun.get());
                }
            }
        }
    }

    void serializeUseFunctions ( AstSerializer & ser, const VariablePtr & f ) {
        ser.tag(HASH_TAG("serializeUseFunctions"));
        if ( ser.writing ) {
            string name = f->name; ser << name;
            uint64_t sz = f->useFunctions.size();
            ser << sz;
            for ( auto & usedFun : f->useFunctions ) {
                bool builtin = usedFun->module->builtIn;
                ser << builtin;
                if ( builtin ) {
                    uint64_t module = usedFun->module->nameHash;
                    uint64_t mnh = usedFun->getMangledNameHash();
                    ser << module << mnh;
                } else {
                    // we serialize the address of the function (not the function itself
                    void * addr = usedFun;
                    ser << addr;
                }
            }
        } else {
            string name; ser << name;
            SERIALIZER_VERIFYF(name == f->name, "expected to serialize in the same order");
            uint64_t size = 0; ser << size;
            f->useFunctions.reserve(size);
            for ( uint64_t i = 0; i < size; i++ ) {
                bool builtin = false;
                ser << builtin;
                if ( builtin ) {
                    uint64_t module = 0;
                    uint64_t mnh = 0;
                    ser << module << mnh;
                    auto pModule = ser.moduleLibrary->findModuleByMangledNameHash(module);
                    SERIALIZER_VERIFYF(pModule, "expected to find module '%llu'", module);
                    auto fun = pModule->findFunctionByMangledNameHash(mnh);
                    SERIALIZER_VERIFYF(fun, "expected to find function");
                    f->useFunctions.emplace(fun.get());
                } else {
                    void * addr = nullptr; ser << addr;
                    auto fun = ser.smartFunctionMap[(uint64_t)(uintptr_t) addr];
                    SERIALIZER_VERIFYF(fun, "expected to find function");
                    f->useFunctions.emplace(fun.get());
                }
            }
        }
    }

    void serializeUseVariables ( AstSerializer & ser, const FunctionPtr & f ) {
        ser.tag(HASH_TAG("serializeUseVariables"));
        if ( ser.writing ) {
            string name = f->name; ser << name;
            uint64_t sz = f->useGlobalVariables.size();
            ser << sz;
            for ( auto & use : f->useGlobalVariables ) {
                bool builtin = use->module->builtIn;
                ser << builtin;
                if ( builtin ) {
                    uint64_t module = use->module->nameHash;
                    string varname = use->name;
                    ser << module << varname;
                } else {
                    void * addr = use;
                    ser << addr;
                }
            }
        } else {
            string name; ser << name;
            SERIALIZER_VERIFYF(name == f->name, "expected to serialize in the same order");
            uint64_t size = 0; ser << size;
            f->useGlobalVariables.reserve(size);
            for ( uint64_t i = 0; i < size; i++ ) {
                bool builtin = false;
                ser << builtin;
                if ( builtin ) {
                    uint64_t module = 0;
                    string varname;
                    ser << module << varname;
                    auto pModule = ser.moduleLibrary->findModuleByMangledNameHash(module);
                    SERIALIZER_VERIFYF(pModule, "expected to find module '%llu'", module);
                    auto var = pModule->findVariable(varname);
                    SERIALIZER_VERIFYF(var, "expected to find variable '%s::%s'", pModule->name.c_str(), varname.c_str());
                    f->useGlobalVariables.emplace(var.get());
                } else {
                    void * addr = nullptr; ser << addr;
                    auto var = ser.smartVariableMap[(uint64_t)(uintptr_t) addr];
                    SERIALIZER_VERIFYF(var, "expected to find variable");
                    f->useGlobalVariables.emplace(var.get());
                }
            }
        }
    }

    void serializeUseVariables ( AstSerializer & ser, const VariablePtr & f ) {
        ser.tag(HASH_TAG("serializeUseVariables"));
        if ( ser.writing ) {
            string name = f->name; ser << name;
            uint64_t sz = f->useGlobalVariables.size();
            ser << sz;
            for ( auto & use : f->useGlobalVariables ) {
                bool builtin = use->module->builtIn;
                ser << builtin;
                if ( builtin ) {
                    uint64_t module = use->module->nameHash;
                    string varname = use->name;
                    ser << module << varname;
                } else {
                    void * addr = use;
                    ser << addr;
                }
            }
        } else {
            string name; ser << name;
            SERIALIZER_VERIFYF(name == f->name, "expected to serialize in the same order");
            uint64_t size = 0; ser << size;
            f->useGlobalVariables.reserve(size);
            for ( uint64_t i = 0; i < size; i++ ) {
                bool builtin = false;
                ser << builtin;
                if ( builtin ) {
                    uint64_t module = 0;
                    string varname;
                    ser << module << varname;
                    auto pModule = ser.moduleLibrary->findModuleByMangledNameHash(module);
                    SERIALIZER_VERIFYF(pModule, "expected to find module '%llu'", module);
                    auto var = pModule->findVariable(varname);
                    SERIALIZER_VERIFYF(var, "expected to find variable '%s::%s'", pModule->name.c_str(), varname.c_str());
                    f->useGlobalVariables.emplace(var.get());
                } else {
                    void * addr = nullptr; ser << addr;
                    auto var = ser.smartVariableMap[(uint64_t)(uintptr_t) addr];
                    SERIALIZER_VERIFYF(var, "expected to find variable");
                    f->useGlobalVariables.emplace(var.get());
                }
            }
        }
    }

    void serializeGlobals ( AstSerializer & ser, safebox<Variable> & globals ) {
        if ( ser.writing ) {
            uint64_t size = globals.unlocked_size(); ser << size;
            globals.foreach ( [&] ( VariablePtr g ) {
                ser << g;
            });
        } else {
            safebox<Variable> result;
            uint64_t size = 0; ser << size;
            for ( uint64_t i = 0; i < size; i++ ) {
                VariablePtr g; ser << g;
                SERIALIZER_VERIFYF(g!=nullptr, "expected to find variable");
                result.insert(g->name, g);
            }
            globals = das::move(result);
        }
    }

    void serializeStructures ( AstSerializer & ser, safebox<Structure> & structures ) {
        if ( ser.writing ) {
            uint64_t size = structures.unlocked_size(); ser << size;
            structures.foreach ( [&] ( StructurePtr g ) {
                ser << g;
            });
        } else {
            uint64_t size = 0; ser << size;
            for ( uint64_t i = 0; i < size; i++ ) {
                StructurePtr g; ser << g;
                SERIALIZER_VERIFYF(g!=nullptr, "expected to find structure");
                structures.insert(g->name, g);
            }
        }
    }

    void serializeFunctions ( AstSerializer & ser, safebox<Function> & functions ) {
        if ( ser.writing ) {
            uint64_t size = functions.unlocked_size(); ser << size;
            functions.foreach ( [&] ( FunctionPtr g ) {
                string name = g->getMangledName();
                ser << name << g;
            });
        } else {
            uint64_t size = 0; ser << size;
            for ( uint64_t i = 0; i < size; i++ ) {
                string name; ser << name;
                FunctionPtr g; ser << g;
                SERIALIZER_VERIFYF(g!=nullptr, "expected to find function");
                functions.insert(name, g);
            }
        }
    }

    void serializeFunctionsByName ( AstSerializer & ser, fragile_hash<vector<Function *>> & functionsByName ) {
        if ( ser.writing ) {
            uint32_t capacity = functionsByName.capacity();
            uint32_t size = functionsByName.size();
            ser << capacity << size;
            uint32_t count = 0;
            functionsByName.foreach ([&] ( uint64_t nameHash, vector<Function *> & functions ) {
                ser << nameHash;
                ser << functions;
                count ++;
            });
            DAS_VERIFYF(count == size, "expected to serialize all functions");
        } else {
            uint32_t capacity = 0; ser << capacity;
            uint32_t size = 0; ser << size;
            functionsByName.reserve(capacity);
            for ( uint32_t i = 0; i < size; i++ ) {
                uint64_t nameHash = 0; ser << nameHash;
                vector<Function *> functions; ser << functions;
                functionsByName[nameHash] = das::move(functions);
            }
        }
    }

    void Module::serialize ( AstSerializer & ser, bool already_exists ) {
        ser.tag(HASH_TAG("Module"));
        ser << name << nameHash << moduleFlags;
        ser << annotationData << requireModule;
        ser << aliasTypes     << enumerations;
        ser << keywords;
        ser << typeFunctions;
        serializeGlobals(ser, globals); // globals require insertion in the same order
        serializeStructures(ser, structures);
        serializeFunctions(ser, functions);
        if ( ser.failed ) return;
        serializeFunctions(ser, generics);
        if ( ser.failed ) return;
        serializeFunctionsByName(ser, functionsByName);
        serializeFunctionsByName(ser, genericsByName);
        ser << ownFileInfo;     //<< promotedAccess;

        functions.foreach ([&] ( smart_ptr<Function> f ) {
            if ( ser.writing ) {
                ser << f->name;
            } else {
                string fname; ser << fname;
                SERIALIZER_VERIFYF(fname == f->name, "expected to walk in the same order");
            }
            serializeUseVariables(ser, f);
            serializeUseFunctions(ser, f);
        });

        generics.foreach ([&] ( smart_ptr<Function> f ) {
            if ( ser.writing ) {
                ser << f->name;
            } else {
                string fname; ser << fname;
                SERIALIZER_VERIFYF(fname == f->name, "expected to walk in the same order");
            }
            serializeUseVariables(ser, f);
            serializeUseFunctions(ser, f);
        });

        globals.foreach_with_hash ([&](smart_ptr<Variable> g, uint64_t hash) {
            if ( ser.writing ) {
                ser << hash;
            } else {
                uint64_t h = 0; ser << h;
                SERIALIZER_VERIFYF(h == hash, "expected to walk in the same order");
            }
            serializeUseVariables(ser, g);
            serializeUseFunctions(ser, g);
        });

        ser.patch();

        // Now we need to restore the internal state in case this has been a macro module

        finalizeModule(ser, *ser.moduleLibrary, this, already_exists);
    }

    class TopSort {
    public:
        TopSort(const vector<Module*> & inputModules) : input(inputModules) {
            for (auto mod : input) {
                visited[mod] = NOT_SEEN;
            }
        }

        vector<Module*> getDependecyOrdered(Module * m) {
            visit(m);
            return das::move(sorted);
        }

        vector<Module*> getDependecyOrdered() {
            for ( auto mod : input ) {
                visit(mod);
            }
            return das::move(sorted);
        }

        void visit( Module * mod ) {
            if ( visited[mod] != NOT_SEEN ) return;
            visited[mod] = IN_PROGRESS;
            for ( auto [module, required] : mod->requireModule ) {
                if ( module != mod ) {
                    visit(module);
                }
            }
            visited[mod] = FINISHED;
            sorted.push_back(mod);
        }

    private:
        enum WalkStatus { NOT_SEEN, IN_PROGRESS, FINISHED };
        vector<Module*> sorted;
        const vector<Module*> & input;
        das_hash_map<Module*, WalkStatus> visited;
    };


    AstSerializer & AstSerializer::operator << ( CodeOfPolicies & value ) {
        // This is like so because of several fields with strings
        *this << value.aot
              << value.aot_module
              << value.completion
              << value.export_all
              << value.keep_alive
              << value.very_safe_context
              << value.always_report_candidates_threshold
              << value.max_infer_passes
              << value.stack
              << value.intern_strings
              << value.persistent_heap
              << value.multiple_contexts
              << value.heap_size_hint
              << value.string_heap_size_hint
              << value.solid_context
              << value.macro_context_persistent_heap
              << value.macro_context_collect
              << value.rtti
              << value.unsafe_table_lookup
              << value.relaxed_pointer_const
              << value.version_2_syntax
              << value.gen2_make_syntax
              << value.relaxed_assign
              << value.no_unsafe
              << value.local_ref_is_unsafe
              << value.no_global_variables
              << value.no_global_variables_at_all
              << value.no_global_heap
              << value.only_fast_aot
              << value.aot_order_side_effects
              << value.no_unused_function_arguments
              << value.no_unused_block_arguments
              << value.allow_block_variable_shadowing
              << value.allow_local_variable_shadowing
              << value.allow_shared_lambda
              << value.ignore_shared_modules
              << value.default_module_public
              << value.no_deprecated
              << value.no_aliasing
              << value.strict_smart_pointers
              << value.no_init
              << value.strict_unsafe_delete
              << value.no_members_functions_in_struct
              << value.no_local_class_members
              << value.no_unsafe_uninitialized_structures
              << value.strict_properties
              << value.no_optimizations
              << value.fail_on_no_aot
              << value.fail_on_lack_of_aot_export
              << value.no_fast_call
              << value.debugger
              << value.debug_module
              << value.profiler
              << value.profile_module
              << value.jit
              << value.threadlock_context;
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( tuple<Module *, string, string, bool, LineInfo> & value ) {
        *this << get<0>(value) << get<1>(value) << get<2>(value) << get<3>(value) << get<4>(value);
        return *this;
    }

    // Used in eden
    void AstSerializer::serializeProgram ( ProgramPtr program, ModuleGroup & libGroup ) noexcept {
        auto & ser = *this;

        ser << program->thisNamespace << program->thisModuleName;

        ser << program->totalFunctions      << program->totalVariables << program->newLambdaIndex;
        ser << program->globalInitStackSize << program->globalStringHeapSize;
        ser << program->flags;

        ser << program->options << program->policies;

        if ( writing ) {
            TopSort ts(program->library.getModules());
            auto modules = ts.getDependecyOrdered(program->thisModule.get());

            uint64_t size = modules.size(); *this << size;

            for ( auto & m : modules ) {
                bool builtin = m->builtIn, promoted = m->promoted;
                *this << builtin << promoted;
                *this << m->name;

                if ( m->builtIn && !m->promoted ) {
                    *this << m->cumulativeHash;
                    continue;
                }

                if ( writingReadyModules.count(m) == 0 ) {
                    writingReadyModules.insert(m);
                    *this << *m;
                }
            }
        } else {
            uint64_t size = 0; ser << size;

            program->library.reset();
            program->thisModule.release();
            moduleLibrary = &program->library;

            for ( uint64_t i = 0; i < size; i++ ) {
                bool builtin = false, promoted = false;
                ser << builtin << promoted;
                string name; ser << name;

                if ( builtin && !promoted ) {
                    auto m = Module::require(name);
                    uint64_t savedHash = 0, moduleHash = m->cumulativeHash;
                    *this << savedHash;

                    if ( moduleHash != savedHash ) {
                        LOG(LogLevel::warning) << "das: serialize: cumulative hash for module '" << m->name
                                               << "' differs" << " (" << moduleHash << " vs " << savedHash << ") ";
                        program->failToCompile = true;
                        return;
                    }

                    program->library.addModule(m);
                    continue;
                }

                if ( auto m = libGroup.findModule(name) ) {
                    program->library.addModule(m);
                    continue;
                }

                Module* deser = nullptr;
                try {
                    deser = new Module();
                    deser->setModuleName(name);
                    program->library.addModule(deser);
                    ser << *deser;
                } catch ( const std::runtime_error & r ) {
                    delete deser;
                    LOG(LogLevel::warning) << "das: serialize: " << r.what();
                    program->failToCompile = true;
                    return;
                }
            }

            program->thisModule.reset(program->library.getModules().back());
        }
    }

    uint32_t AstSerializer::getVersion () {
        static constexpr uint32_t currentVersion = 138;
        return currentVersion;
    }

    // Serializes the whole script as opposed to just one module
    bool AstSerializer::serializeScript ( ProgramPtr program ) noexcept {
        try {
            program->serialize(*this);
            return true;
        } catch ( const std::runtime_error & r ) {
            program->failToCompile = true;
            LOG(LogLevel::warning) << "das: serialize: " << r.what();
            return false;
        }
    }

    // Used in daNetGame currently
    void Program::serialize ( AstSerializer & ser ) {
        ser << thisNamespace << thisModuleName;

        ser << totalFunctions      << totalVariables << newLambdaIndex;
        ser << globalInitStackSize << globalStringHeapSize;
        ser << flags;

        ser << options << policies;

    // serialize library
        if ( ser.writing ) {
            ser.moduleLibrary = &library;
            TopSort ts(library.modules);
            auto modules = ts.getDependecyOrdered();

            vector<Module*> builtinModules;
            for ( auto m : modules ) {
                if ( m->builtIn && !m->promoted ) {
                    builtinModules.push_back(m);
                }
            }

            uint64_t size_builtin = builtinModules.size();
            ser << size_builtin;

            for ( auto m : builtinModules ) {
                ser << m->name;
            }

            uint64_t size = modules.size();
            ser << size;

            for ( auto & m : modules ) {
                bool builtin = m->builtIn, promoted = m->promoted;
                ser << builtin << promoted;
                ser << m->name << m->fileName;

                if ( m->builtIn && m->promoted ) {
                    bool isNew = ser.writingReadyModules.count(m) == 0;
                    ser << isNew;
                    if ( isNew ) {
                        ser.writingReadyModules.insert(m);
                        ser << *m;
                    }
                } else if ( m->builtIn ) {
                    continue;
                } else {
                    ser << *m;
                }
            }

            ser << allRequireDecl;
            return;
        }

        library.reset();
        thisModule.release();
        ser.moduleLibrary = &library;

        uint64_t size_builtin = 0; ser << size_builtin;
        for ( uint64_t i = 0; i < size_builtin; i++ ) {
            string name; ser << name;
            Module * m = Module::require(name);
            library.addModule(m);
        }

        uint64_t size = 0; ser << size;
        for ( uint64_t i = 0; i < size; i++ ) {
            bool builtin = false, promoted = false;
            string name, fileName;
            ser << builtin << promoted << name << fileName;
            if ( builtin && !promoted ) {
                // pass
            } else if ( builtin && promoted ) {
                bool isNew = false; ser << isNew;
                if ( isNew ) {
                    Module *prev = Module::require(name);
                    auto mod = new Module;
                    mod->setModuleName(name);
                    mod->fileName = fileName;
                    if ( prev ) {
                        library.addModule(prev);
                        ser.serializeModule(*mod, /*already_exists*/true);
                        mod->builtIn = false; // suppress assert
                        delete mod;
                    } else {
                        library.addModule(mod);
                        ser.serializeModule(*mod, /*already_exists*/false);
                        mod->builtIn = false; // suppress assert
                        mod->promoteToBuiltin(nullptr);
                    }
                } else {
                    Module * m = Module::require(name);
                    library.addModule(m);
                }
            } else {
                auto mod = new Module;
                mod->setModuleName(name);
                mod->fileName = fileName;
                library.addModule(mod);
                ser << *mod;
            }
        }

        thisModule.reset(library.modules.back());

        ser << allRequireDecl;

    // for the last module, mark symbols manually
        markExecutableSymbolUse();
        removeUnusedSymbols();
        TextWriter logs;
        allocateStack(logs,true,false);
    }

}
