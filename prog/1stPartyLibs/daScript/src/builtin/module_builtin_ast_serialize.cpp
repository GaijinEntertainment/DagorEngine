#include "daScript/misc/das_common.h"
#include "daScript/misc/platform.h"
#include "daScript/misc/performance_time.h"

#include "daScript/ast/ast_serialize_macro.h"
#include "daScript/ast/ast_serializer.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"
#include <cstdarg>
#include <stdexcept>
#include <type_traits>

namespace das {

    AstSerializer::AstSerializer ( SerializationStorage * storage, bool isWriting ) {
        astModule = Module::require("ast_core");
        for ( auto & [key, annotation] : astModule->handleTypes ) {
            if ( starts_with(annotation->name,"Expr") ) {
                uint32_t hash = hash_tag(annotation->name.c_str());
                rttiHash2Annotation[hash] = annotation;
            }
        }
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
    void patchRefs ( vector<pair<TT**,SerializeNodeId>> & refs, const das_hash_map<SerializeNodeId, smart_ptr<TT>> & objects) {
        for ( auto & p : refs ) {
            auto it = objects.find(p.second);
            if ( it == objects.end() ) {
                throw dasException{"ast serializer function ref not found", LineInfo()};
            } else {
                *p.first = it->second.get();
            }
        }
        refs.clear();
    }

    template <typename TT>
    void patchRefs ( vector<pair<TT**,SerializeNodeId>> & refs, const das_hash_map<SerializeNodeId, TT*> & objects) {
        for ( auto & p : refs ) {
            auto it = objects.find(p.second);
            if ( it == objects.end() ) {
                throw dasException{"ast serializer function ref not found", LineInfo()};
            } else {
                *p.first = it->second;
            }
        }
        refs.clear();
    }

    void throw_formatted_error ( const char * fmt, ... ) {
        va_list args;
        va_start(args, fmt);

        char err[256];
        int len = vsnprintf(err, 256, fmt, args);
        err[len] = '\0';

        va_end(args);

        throw dasException{err, LineInfo()};
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
            *field = struct_.front()->findFieldRef(fieldname);
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
        } catch ( const dasException & ) {
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

    AstSerializer & AstSerializer::operator << ( SerializeNodeId & value ) {
        dtag(HASH_TAG("SerializeNodeId"));
        // 64-bit user-space pointers fit in 48 bits (canonical form on x86-64
        // and AArch64 Linux/macOS — top 16 bits are sign-extended copies of
        // bit 47, and we only ever see user-space addresses here, so they are
        // zero — but we mask them on write regardless, since ptr is an identity
        // key only). Pack a small epoch into those unused top 16 bits so the
        // common case is a single 8-byte word. Reserve sentinel 0xFFFF for
        // "epoch overflow" — fall back to a separate adaptive-size write.
        // 32-bit hosts have no headroom; always emit ptr + adaptive epoch.
        if constexpr ( sizeof(void *) == 8 ) {
            constexpr uint64_t kPtrMask  = (uint64_t(1) << 48) - 1;
            constexpr uint64_t kEpochOverflowTag = 0xFFFFull << 48;
            if ( writing ) {
                uintptr_t pbits = reinterpret_cast<uintptr_t>(value.ptr) & kPtrMask;
                if ( value.epoch < 0xFFFFull ) {
                    uint64_t packed = (uint64_t(value.epoch) << 48) | uint64_t(pbits);
                    *this << packed;
                } else {
                    uint64_t packed = kEpochOverflowTag | uint64_t(pbits);
                    *this << packed;
                    uint32_t epoch32 = uint32_t(value.epoch);
                    serializeAdaptiveSize32(epoch32);
                }
            } else {
                uint64_t packed = 0;
                *this << packed;
                uint64_t topBits = packed & ~kPtrMask;
                value.ptr = reinterpret_cast<void *>(uintptr_t(packed & kPtrMask));
                if ( topBits == kEpochOverflowTag ) {
                    uint32_t epoch32 = 0;
                    serializeAdaptiveSize32(epoch32);
                    value.epoch = size_t(epoch32);
                } else {
                    value.epoch = size_t(topBits >> 48);
                }
            }
        } else {
            // 32-bit: pointer fills the word; epoch goes in its own adaptive int.
            if ( writing ) {
                uint32_t pbits = uint32_t(reinterpret_cast<uintptr_t>(value.ptr));
                uint32_t epoch32 = uint32_t(value.epoch);
                *this << pbits;
                serializeAdaptiveSize32(epoch32);
            } else {
                uint32_t pbits = 0;
                *this << pbits;
                uint32_t epoch32 = 0;
                serializeAdaptiveSize32(epoch32);
                value.ptr = reinterpret_cast<void *>(uintptr_t(pbits));
                value.epoch = size_t(epoch32);
            }
        }
        return *this;
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

    template <typename V, typename VT>
    AstSerializer & AstSerializer::operator << ( safebox<V,VT> & box ) {
        dtag(HASH_TAG("Safebox"));
        if ( writing ) {
            uint64_t size = box.unlocked_size(); *this << size;
            box.foreach_with_hash ([&](VT obj, uint64_t hash) {
                *this << hash << obj;
            });
            return *this;
        }
        uint64_t size = 0; *this << size;
        safebox<V,VT> deser;
        for ( uint64_t i = 0; i < size; i++ ) {
            VT obj {}; uint64_t hash = 0;
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

    template <typename K, typename V, typename H, typename E>
    AstSerializer & AstSerializer::operator << ( das_hash_map<K, V, H, E> & value ) {
        serialize_hash_map<K, V, H, E>(value);
        return *this;
    }

    // Guarded: see ast_serializer.h note. Under DAS_CUSTOM_HASH=0 these would
    // duplicate the das_hash_map definitions above (identical aliases).
#if DAS_CUSTOM_HASH
    template <typename K, typename V, typename H, typename E>
    void AstSerializer::serialize_hash_map ( das_insert_only_hash_map<K, V, H, E> & value ) {
        dtag(HASH_TAG("DasHashmap"));
        if ( writing ) {
            uint64_t size = value.size(); *this << size;
            for ( auto & item : value ) {
                *this << item.first << item.second;
            }
            return;
        }
        uint64_t size = 0; *this << size;
        das_insert_only_hash_map<K, V, H, E> deser;
        deser.reserve(size);
        for ( uint64_t i = 0; i < size; i++ ) {
            K k; V v; *this << k << v;
            deser.emplace(das::move(k),das::move(v));
        }
        value = das::move(deser);
    }

    template <typename K, typename V, typename H, typename E>
    AstSerializer & AstSerializer::operator << ( das_insert_only_hash_map<K, V, H, E> & value ) {
        serialize_hash_map<K, V, H, E>(value);
        return *this;
    }
#endif

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

    void AstSerializer::fillOrPatchLater ( Function * & func, SerializeNodeId id ) {
        auto it = smartFunctionMap.find(id);
        if ( it == smartFunctionMap.end() ) {
            func = ( Function * ) 1;
            functionRefs.emplace_back(&func, id);
        } else {
            func = it->second;
        }
    }

    void AstSerializer::fillOrPatchLater ( Enumeration * & ptr, SerializeNodeId id ) {
        auto it = smartEnumerationMap.find(id);
        if ( it == smartEnumerationMap.end() ) {
            ptr = ( Enumeration * ) 1;
            enumerationRefs.emplace_back(&ptr, id);
        } else {
            ptr = it->second;
        }
    }

    void AstSerializer::fillOrPatchLater ( Structure * & ptr, SerializeNodeId id ) {
        auto it = smartStructureMap.find(id);
        if ( it == smartStructureMap.end() ) {
            ptr = ( Structure * ) 1;
            structureRefs.emplace_back(&ptr, id);
        } else {
            ptr = it->second;
        }
    }

    void AstSerializer::fillOrPatchLater ( Variable * & ptr, SerializeNodeId id ) {
        auto it = smartVariableMap.find(id);
        if ( it == smartVariableMap.end() ) {
            ptr = ( Variable * ) 1;
            variableRefs.emplace_back(&ptr, id);
        } else {
            ptr = it->second;
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
            func = f;
        } else if ( auto genericFn = funcModule->findGeneric(mangledName) ) {
            func = genericFn;
        }
        if ( func == nullptr && funcModule->wasParsedNameless ) {
            string modname, funcname;
            splitTypeName(mangledName, modname, funcname);
            func = funcModule->findFunction(funcname);
        }
        if ( func == nullptr ) {
            failed = true;
            SERIALIZER_VERIFYF(false, "function '%s' not found", mangledName.c_str());
        }
    }

    void AstSerializer::findExternal ( Enumeration * & ptr ) {
        auto [mod, mangledNameHash] = readModuleAndNameHash();
        ptr = mod->findEnumByMangledNameHash(mangledNameHash);
        SERIALIZER_VERIFYF(ptr!=nullptr, "enumeration '%llu' is not found", mangledNameHash);
    }

    void AstSerializer::findExternal ( Structure * & ptr ) {
        auto [mod, mangledNameHash] = readModuleAndNameHash();
        ptr = mod->findStructureByMangledNameHash(mangledNameHash);
        SERIALIZER_VERIFYF(ptr!=nullptr, "structure '%llu' is not found", mangledNameHash);
    }

    void AstSerializer::findExternal ( Variable * & ptr ) {
        auto [mod, mangledName] = readModuleAndName();
        ptr = mod->findVariable(mangledName);
        SERIALIZER_VERIFYF(ptr!=nullptr, "variable '%s' is not found", mangledName.c_str());
    }

    void AstSerializer::findExternal ( TypeInfoMacro * & ptr ) {
        auto [mod, mangledName] = readModuleAndName();
        ptr = mod->findTypeInfoMacro(mangledName);
        SERIALIZER_VERIFYF(ptr!=nullptr, "variable '%s' is not found", mangledName.c_str());
    }

    template<typename TT>
    AstSerializer & AstSerializer::serializePointer ( TT * & ptr ) {
        auto fid = getSerializeId(ptr);
        *this << fid;
        if ( !fid.ptr ) {
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


    AstSerializer & AstSerializer::operator << ( FunctionPtr & func ) {
        dtag(HASH_TAG("FunctionPtr"));
        if ( writing && func ) {
            SERIALIZER_VERIFYF(!func->builtIn, "cannot serialize built-in function");
        }
        auto id = getSerializeId(func);
        *this << id;
        if ( id.ptr == 0 ) {
            if ( !writing ) func = nullptr;
            return *this;
        }
        if ( writing ) {
            if ( smartFunctionMap.find(id) == smartFunctionMap.end() ) {
                smartFunctionMap[id] = func;
                func->serialize(*this);
            }
        } else {
            auto it = smartFunctionMap.find(id);
            if ( it == smartFunctionMap.end() ) {
                func = new Function();
                smartFunctionMap[id] = func;
                func->serialize(*this);
            } else {
                func = it->second;
            }
        }
        if ( func ) {
            if ( writing ) {
                string name = func->name;
                *this << name;
            } else {
                string name; *this << name;
                string expect = func->name;
                SERIALIZER_VERIFYF(name == expect, "expected different function %s %s", name.c_str(), expect.c_str());
            }
        }
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( TypeInfoMacro * & ptr ) {
        dtag(HASH_TAG("TypeInfoMacroPtr"));
        // TypeInfoMacro is not gc_node and is always external (lives in another
        // module). It is identified by name+module hash, so the wire form only
        // needs a presence bit — no pointer/id leaks into the stream.
        bool is_null = ptr == nullptr;
        *this << is_null;
        if ( is_null ) {
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
        auto id = getSerializeId(type);
        *this << id;
        if ( writing ) {
            if ( smartTypeDeclMap[id] == nullptr ) {
                smartTypeDeclMap[id] = type;
                type->serialize(*this);
            }
        } else {
            if ( smartTypeDeclMap[id] == nullptr ) {
                type = new TypeDecl();
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
        if ( !writing ) annotation_decl = new AnnotationDeclaration();
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
                    anno = mod->findAnnotation(name);
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
        auto id = getSerializeId(struct_);
        *this << id;
        if ( id.ptr == 0 ) {
            if ( !writing ) struct_ = nullptr;
            return *this;
        }
        if ( writing ) {
            if ( smartStructureMap.find(id) == smartStructureMap.end() ) {
                smartStructureMap[id] = struct_;
                struct_->serialize(*this);
            }
        } else {
            auto it = smartStructureMap.find(id);
            if ( it == smartStructureMap.end() ) {
                struct_ = new Structure();
                smartStructureMap[id] = struct_;
                struct_->serialize(*this);
            } else {
                struct_ = it->second;
            }
        }
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
            auto p = getSerializeId(ptr.get());
            *this << p;
            if ( fileAccessMap[p] == nullptr ) {
                fileAccessMap[p] = ptr.get();
                ptr->serialize(*this);
            }
        } else {
            SerializeNodeId p; *this << p;
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

    AstSerializer & AstSerializer::operator << ( EnumerationPtr & enum_type ) {
        if ( writing ) {
            bool builtin = enum_type->module->builtIn && !enum_type->module->promoted;
            *this << builtin;
            if ( builtin ) {
                uint64_t module = enum_type->module->nameHash;
                string name = enum_type->name;
                *this << module << name;
            } else {
                auto id = getSerializeId(enum_type);
                *this << id;
                if ( smartEnumerationMap.find(id) == smartEnumerationMap.end() ) {
                    smartEnumerationMap[id] = enum_type;
                    enum_type->serialize(*this);
                }
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
                SerializeNodeId id;
                *this << id;
                SERIALIZER_VERIFYF(id.ptr != 0, "expected non-null enumeration id");
                auto it = smartEnumerationMap.find(id);
                if ( it == smartEnumerationMap.end() ) {
                    enum_type = new Enumeration();
                    smartEnumerationMap[id] = enum_type;
                    enum_type->serialize(*this);
                } else {
                    enum_type = it->second;
                }
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
        AnnotationPtr a = static_cast<Annotation*>(type_anno);
        *this << a;
        type_anno = static_cast<TypeAnnotation*>(a);
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( VariablePtr & var ) {
        auto id = getSerializeId(var);
        *this << id;
        if ( id.ptr == 0 ) {
            if ( !writing ) var = nullptr;
            return *this;
        }
        if ( writing ) {
            if ( smartVariableMap.find(id) == smartVariableMap.end() ) {
                smartVariableMap[id] = var;
                var->serialize(*this);
            }
        } else {
            auto it = smartVariableMap.find(id);
            if ( it == smartVariableMap.end() ) {
                var = new Variable();
                smartVariableMap[id] = var;
                var->serialize(*this);
            } else {
                var = it->second;
            }
        }
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
        auto id = getSerializeId(block);
        *this << id;
        if ( !writing && id.ptr ) {
            block = ( ExprBlock * ) 1;
            blockRefs.emplace_back(&block, id);
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
        dtag(HASH_TAG("MakeFieldDeclPtr"));
        bool is_null = ptr == nullptr;
        *this << is_null;
        if ( is_null ) {
            if ( !writing ) ptr = nullptr;
            return *this;
        }
        if ( writing ) {
            ptr->serialize(*this);
        } else {
            ptr = new MakeFieldDecl();
            ptr->serialize(*this);
        }
        dtag(HASH_TAG("/MakeFieldDeclPtr"));
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( MakeStructPtr & ptr ) {
        dtag(HASH_TAG("MakeStructPtr"));
        bool is_null = ptr == nullptr;
        *this << is_null;
        if ( is_null ) {
            if ( !writing ) ptr = nullptr;
            return *this;
        }
        if ( writing ) {
            ptr->serialize(*this);
        } else {
            ptr = new MakeStruct();
            ptr->serialize(*this);
        }
        dtag(HASH_TAG("/MakeStructPtr"));
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
                ser << alias;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !firstType, !secondType,
                                argTypes.empty(), argNames.empty());
                break;
            case Type::tFixedArray:
                ser << alias << firstType << fixedDim << fixedDimExpr;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !secondType,
                                argTypes.empty(), argNames.empty());
                break;
            case Type::alias:
                ser << alias << firstType;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !secondType,
                                !alias.empty(), argTypes.empty(), argNames.empty());
                break;
            case option:
                ser << argTypes;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !firstType, !secondType,
                                alias.empty(), !argTypes.empty(), argNames.empty());
                break;
            case autoinfer:
                ser << alias;
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
                ser << alias;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !firstType, !secondType,
                                argTypes.empty(), argNames.empty());
                break;
            case tRange:
            case tURange:
            case tRange64:
            case tURange64: // blow up!
                ser << alias;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !firstType, !secondType,
                                argTypes.empty(), argNames.empty());
                break;
            case tStructure:
                ser << alias << structType;
                DAS_VERIFYF_MULTI(!annotation, !!structType, !enumType, !firstType, !secondType,
                                argTypes.empty(), argNames.empty());
                break;
            case tHandle:
                ser << alias << annotation;
                DAS_VERIFYF_MULTI(!!annotation, !structType, !enumType, !firstType, !secondType,
                                argTypes.empty(), argNames.empty());
                break;
            case tEnumeration:
            case tEnumeration8:
            case tEnumeration16:
            case tEnumeration64:
                ser << alias << enumType;
                DAS_VERIFYF_MULTI(!annotation, !structType, !!enumType, !firstType, !secondType,
                                argTypes.empty(), argNames.empty());
                break;
            case tBitfield:
            case tBitfield8:
            case tBitfield16:
            case tBitfield64:
                ser << alias << argNames;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !firstType, !secondType,
                                argTypes.empty());
                break;
            case tIterator:
            case tPointer:
            case tArray: // blow up!
                ser << alias << firstType;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !secondType,
                                argTypes.empty(), argNames.empty());
                break;
            case tFunction:
            case tLambda:
            case tBlock:
                ser << alias << firstType << argTypes << argNames;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !secondType);
                break;
            case tTable:
                ser << alias << firstType << secondType;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !!firstType,
                                argTypes.empty(), argNames.empty());
                break;
            case tTuple:
            case tVariant:
                ser << alias << argTypes << argNames;
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !firstType, !secondType,
                                !argTypes.empty());
                break;
            default:
                SERIALIZER_VERIFYF(false,  "not expected to be here");
                break;
        }

        // unconditional: typeMacro/typeDecl payload, and the tag payload riding on an
        // autoinfer firstType (FIXED_ARRAY_REWORK.md, 1b)
        ser << typeMacroExpr;

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
    }

    void Structure::serialize ( AstSerializer & ser ) {
        ser.tag(HASH_TAG("Structure"));
        ser << name;
        ser << at     << module;
        ser << fields << fieldLookup;
        ser << aliases;
        ser << parent // parent could be in the current module or in some other
                      // module
            << flags
            << ownSemanticHash;
        serializeAnnotationList(ser, annotations);
    }

    void Variable::serialize ( AstSerializer & ser ) {
        ser.tag(HASH_TAG("Variable"));
        ser << name << aka << type << init << source << at << index << stackTop
            << extraLocalOffset << module
            << initStackSize << flags << access_flags << annotation;
    }

    void Function::AliasInfo::serialize ( AstSerializer & ser ) {
        ser.tag(HASH_TAG("AliasInfo"));
        ser << var;
        ser.serializePointer(func);
        ser << viaPointer;
    }

    void InferHistory::serialize ( AstSerializer & ser ) {
        ser.tag(HASH_TAG("InferHistory"));
        ser << at;
        ser.serializePointer(func);
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
//
// Per-node serialization lives in SerializeVisitor. AstSerializer::operator<<(ExpressionPtr&)
// calls expr->dispatch(v), which selects the matching preVisit(ExprXxx*) override below.
// Helper methods serializeXxx(...) replicate the old virtual inheritance chain
// (ExprOp1 -> ExprOp -> ExprCallFunc -> ExprLooksLikeCall -> Expression) without
// relying on virtual dispatch inside this visitor.

    class SerializeVisitor : public Visitor {
        AstSerializer & ser;

        void serializeBase          ( Expression * expr );
        void serializeLooksLikeCall ( ExprLooksLikeCall * expr );
        void serializeCallFunc      ( ExprCallFunc * expr );
        void serializeOp            ( ExprOp * expr );
        void serializeOp2           ( ExprOp2 * expr );
        void serializeConst         ( ExprConst * expr );
        void serializeMakeLocal     ( ExprMakeLocal * expr );
        void serializeAt            ( ExprAt * expr );
        void serializePtr2Ref       ( ExprPtr2Ref * expr );
        void serializeField         ( ExprField * expr );
        void serializeMakeArray     ( ExprMakeArray * expr );

    public:
        explicit SerializeVisitor ( AstSerializer & s ) : ser(s) {}
        using Visitor::preVisit;

        void preVisitExpression ( Expression            * expr ) override;
        void preVisit ( ExprReader             * expr ) override;
        void preVisit ( ExprLabel              * expr ) override;
        void preVisit ( ExprGoto               * expr ) override;
        void preVisit ( ExprRef2Value          * expr ) override;
        void preVisit ( ExprRef2Ptr            * expr ) override;
        void preVisit ( ExprPtr2Ref            * expr ) override;
        void preVisit ( ExprAddr               * expr ) override;
        void preVisit ( ExprNullCoalescing     * expr ) override;
        void preVisit ( ExprDelete             * expr ) override;
        void preVisit ( ExprAt                 * expr ) override;
        void preVisit ( ExprSafeAt             * expr ) override;
        void preVisit ( ExprBlock              * expr ) override;
        void preVisit ( ExprVar                * expr ) override;
        void preVisit ( ExprTag                * expr ) override;
        void preVisit ( ExprField              * expr ) override;
        void preVisit ( ExprSafeAsVariant      * expr ) override;
        void preVisit ( ExprSwizzle            * expr ) override;
        void preVisit ( ExprSafeField          * expr ) override;
        void preVisit ( ExprLooksLikeCall      * expr ) override;
        void preVisit ( ExprCallMacro          * expr ) override;
        void preVisit ( ExprOp1                * expr ) override;
        void preVisit ( ExprOp2                * expr ) override;
        void preVisit ( ExprCopy               * expr ) override;
        void preVisit ( ExprMove               * expr ) override;
        void preVisit ( ExprClone              * expr ) override;
        void preVisit ( ExprOp3                * expr ) override;
        void preVisit ( ExprTryCatch           * expr ) override;
        void preVisit ( ExprReturn             * expr ) override;
        void preVisit ( ExprConst              * expr ) override;
        void preVisit ( ExprConstPtr           * expr ) override;
        void preVisit ( ExprConstEnumeration   * expr ) override;
        void preVisit ( ExprConstBitfield      * expr ) override;
        void preVisit ( ExprConstString        * expr ) override;
        void preVisit ( ExprStringBuilder      * expr ) override;
        void preVisit ( ExprLet                * expr ) override;
        void preVisit ( ExprFor                * expr ) override;
        void preVisit ( ExprUnsafe             * expr ) override;
        void preVisit ( ExprWhile              * expr ) override;
        void preVisit ( ExprWith               * expr ) override;
        void preVisit ( ExprAssume             * expr ) override;
        void preVisit ( ExprMakeBlock          * expr ) override;
        void preVisit ( ExprMakeGenerator      * expr ) override;
        void preVisit ( ExprYield              * expr ) override;
        void preVisit ( ExprInvoke             * expr ) override;
        void preVisit ( ExprAssert             * expr ) override;
        void preVisit ( ExprQuote              * expr ) override;
        void preVisit ( ExprTypeInfo           * expr ) override;
        void preVisit ( ExprIs                 * expr ) override;
        void preVisit ( ExprAscend             * expr ) override;
        void preVisit ( ExprCast               * expr ) override;
        void preVisit ( ExprNew                * expr ) override;
        void preVisit ( ExprCall               * expr ) override;
        void preVisit ( ExprIfThenElse         * expr ) override;
        void preVisit ( ExprNamedCall          * expr ) override;
        void preVisit ( ExprMakeStruct         * expr ) override;
        void preVisit ( ExprMakeVariant        * expr ) override;
        void preVisit ( ExprMakeArray          * expr ) override;
        void preVisit ( ExprMakeTuple          * expr ) override;
        void preVisit ( ExprArrayComprehension * expr ) override;
        void preVisit ( ExprTypeDecl           * expr ) override;
    };

    void SerializeVisitor::serializeBase ( Expression * expr ) {
        ser << expr->at
            << expr->type
            << expr->genFlags
            << expr->flags
            << expr->printFlags;
        ser.dtag(HASH_TAG("ptr_ref_count"));
    }

    void SerializeVisitor::serializeLooksLikeCall ( ExprLooksLikeCall * expr ) {
        serializeBase(expr);
        ser << expr->name                   << expr->arguments;
        ser << expr->argumentsFailedToInfer << expr->aliasSubstitution << expr->atEnclosure;
        ser << expr->pipedCallArgument;
    }

    void SerializeVisitor::serializeCallFunc ( ExprCallFunc * expr ) {
        serializeLooksLikeCall(expr);
        ser.serializePointer(expr->func);
        ser << expr->stackTop;
    }

    void SerializeVisitor::serializeOp ( ExprOp * expr ) {
        serializeCallFunc(expr);
        ser << expr->op;
    }

    void SerializeVisitor::serializeOp2 ( ExprOp2 * expr ) {
        serializeOp(expr);
        ser << expr->left;
        ser << expr->right;
    }

    void SerializeVisitor::serializeConst ( ExprConst * expr ) {
        serializeBase(expr);
        ser << expr->baseType << expr->value << expr->foldedNonConst << expr->promotedFromInt << expr->inexactFloatPromotion;
    }

    void SerializeVisitor::serializeMakeLocal ( ExprMakeLocal * expr ) {
        serializeBase(expr);
        ser << expr->makeType << expr->stackTop << expr->extraOffset << expr->makeFlags;
    }

    void SerializeVisitor::serializeAt ( ExprAt * expr ) {
        ser.dtag(HASH_TAG("ExprAt"));
        serializeBase(expr);
        ser << expr->subexpr << expr->index;
        ser << expr->atFlags;
    }

    void SerializeVisitor::serializePtr2Ref ( ExprPtr2Ref * expr ) {
        serializeBase(expr);
        ser << expr->subexpr << expr->unsafeDeref << expr->assumeNoAlias;
    }

    void SerializeVisitor::serializeField ( ExprField * expr ) {
        ser.dtag(HASH_TAG("ExprField"));
        serializeBase(expr);
        ser << expr->value      << expr->name       << expr->atField
            << expr->fieldIndex << expr->annotation << expr->derefFlags
            << expr->fieldFlags;

        if ( ser.writing ) {
            bool has_field = expr->value->type && (
                expr->value->type->isStructure() || ( expr->value->type->isPointer() && expr->value->type->firstType->isStructure() )
            );
            ser << has_field;
            if ( !has_field ) return;
            string mangledName;
            if ( expr->value->type->isPointer() ) {
                SERIALIZER_VERIFYF(expr->value->type->firstType->isStructure(), "expected to see structure field access via pointer");
                mangledName = expr->value->type->firstType->structType->getMangledName();
                ser << expr->value->type->firstType->structType->module;
            } else {
                SERIALIZER_VERIFYF(expr->value->type->isStructure(), "expected to see structure field access");
                mangledName = expr->value->type->structType->getMangledName();
                ser << expr->value->type->structType->module;
            }
            ser << mangledName;
            if ( expr->annotation != nullptr && expr->annotation->getFieldOffset(expr->name) == static_cast<uint32_t>(-1) ) {
                LOG(LogLevel::warning) << "das: serialize: Field '" << expr->name << "' not found in '" << expr->annotation->name << "'";
            }
        } else {
            if ( expr->annotation != nullptr && expr->annotation->getFieldOffset(expr->name) == static_cast<uint32_t>(-1) ) {
                SERIALIZER_VERIFYF(false, "Field '%s' not found in '%s'", expr->name.c_str(), expr->annotation->name.c_str());
            }
            bool has_field = false; ser << has_field;
            if ( !has_field ) return;
            Module * module = nullptr; ser << module;
            string mangledName; ser << mangledName;
            ser.fieldRefs.emplace_back(&expr->fieldRef, module, das::move(mangledName), expr->name);
        }
    }

    void SerializeVisitor::serializeMakeArray ( ExprMakeArray * expr ) {
        serializeMakeLocal(expr);
        ser << expr->recordType << expr->values << expr->gen2;
    }

    void SerializeVisitor::preVisitExpression ( Expression * expr ) {
        // ExprMakeLocal::dispatch routes here (no dedicated preVisit overload).
        // ExprMakeLocal carries makeType/stackTop/extraOffset/makeFlags that
        // serializeBase alone would drop, so detect it and use the proper helper.
        if ( expr->rtti_isMakeLocal() ) {
            serializeMakeLocal(static_cast<ExprMakeLocal*>(expr));
            return;
        }
        serializeBase(expr);
    }

    void SerializeVisitor::preVisit ( ExprReader * expr ) {
        ser.dtag(HASH_TAG("ExprReader"));
        serializeBase(expr);
        ser << expr->macro << expr->sequence;
    }

    void SerializeVisitor::preVisit ( ExprLabel * expr ) {
        ser.dtag(HASH_TAG("ExprLabel"));
        serializeBase(expr);
        ser << expr->label << expr->comment;
    }

    void SerializeVisitor::preVisit ( ExprGoto * expr ) {
        ser.dtag(HASH_TAG("ExprGoto"));
        serializeBase(expr);
        ser << expr->label << expr->subexpr;
    }

    void SerializeVisitor::preVisit ( ExprRef2Value * expr ) {
        ser.dtag(HASH_TAG("ExprRef2Value"));
        serializeBase(expr);
        ser << expr->subexpr;
    }

    void SerializeVisitor::preVisit ( ExprRef2Ptr * expr ) {
        ser.dtag(HASH_TAG("ExprRef2Ptr"));
        serializeBase(expr);
        ser << expr->subexpr;
    }

    void SerializeVisitor::preVisit ( ExprPtr2Ref * expr ) {
        ser.dtag(HASH_TAG("ExprPtr2Ref"));
        serializePtr2Ref(expr);
    }

    void SerializeVisitor::preVisit ( ExprAddr * expr ) {
        ser.dtag(HASH_TAG("ExprAddr"));
        serializeBase(expr);
        ser << expr->target << expr->funcType;
        ser.serializePointer(expr->func);
    }

    void SerializeVisitor::preVisit ( ExprNullCoalescing * expr ) {
        ser.dtag(HASH_TAG("ExprNullCoalescing"));
        serializePtr2Ref(expr);
        ser << expr->defaultValue;
    }

    void SerializeVisitor::preVisit ( ExprDelete * expr ) {
        ser.dtag(HASH_TAG("ExprDelete"));
        serializeBase(expr);
        ser << expr->subexpr << expr->sizeexpr << expr->native;
    }

    void SerializeVisitor::preVisit ( ExprAt * expr ) {
        serializeAt(expr);
    }

    void SerializeVisitor::preVisit ( ExprSafeAt * expr ) {
        ser.dtag(HASH_TAG("ExprSafeAt"));
        serializeAt(expr);
    }

    void SerializeVisitor::preVisit ( ExprBlock * expr ) {
        ser.dtag(HASH_TAG("ExprBlock"));
        serializeBase(expr);

        if ( ser.writing ) {
            auto thisBlockId = ser.getSerializeId(expr);
            ser << thisBlockId;
        } else {
            SerializeNodeId thisBlockId; ser << thisBlockId;
            ser.exprBlockMap.emplace(thisBlockId, expr);
        }

        ser << expr->list << expr->finalList << expr->returnType << expr->arguments << expr->stackTop
            << expr->stackVarTop << expr->stackVarBottom << expr->stackCleanVars << expr->maxLabelIndex
            << expr->annotationData << expr->annotationDataSid << expr->blockFlags;
        ser.serializePointer(expr->inFunction);

        serializeAnnotationList(ser, expr->annotations);
    }

    void SerializeVisitor::preVisit ( ExprVar * expr ) {
        ser.dtag(HASH_TAG("ExprVar"));
        serializeBase(expr);

        ser << expr->name << expr->argumentIndex << expr->varFlags;
        ser << expr->pBlock;

        // The variable is smart_ptr but we actually need
        // non-owning semantics
        if ( ser.writing ) {
            bool inThisModule =  expr->variable == nullptr // this happens with [generic] functions, for example
                      || expr->variable->module == nullptr
                      || expr->variable->module == ser.thisModule;
            ser << inThisModule;
            if ( inThisModule ) {
                ser << expr->variable; // serialize as smart pointer
            } else {
                ser << expr->variable->name;
                ser << expr->variable->module->nameHash;
            }
        } else {

            bool inThisModule = false; ser << inThisModule;
            if ( inThisModule ) {
                ser << expr->variable;
            } else {
                string varname;
                uint64_t modname = 0;
                ser << varname << modname;
                auto mod = ser.moduleLibrary->findModuleByMangledNameHash(modname);
                SERIALIZER_VERIFYF(mod, "expected to find module '%llu'", modname);
                expr->variable = mod->findVariable(varname);
            }

        }
    }

    void SerializeVisitor::preVisit ( ExprTag * expr ) {
        ser.dtag(HASH_TAG("ExprTag"));
        serializeBase(expr);
        ser << expr->subexpr << expr->value << expr->name;
    }

    void SerializeVisitor::preVisit ( ExprField * expr ) {
        serializeField(expr);
    }

    void SerializeVisitor::preVisit ( ExprSafeAsVariant * expr ) {
        serializeField(expr);
        ser << expr->skipQQ;
    }

    void SerializeVisitor::preVisit ( ExprSwizzle * expr ) {
        serializeBase(expr);
        ser << expr->value << expr->mask << expr->fields << expr->fieldFlags;
    }

    void SerializeVisitor::preVisit ( ExprSafeField * expr ) {
        serializeField(expr);
        ser << expr->skipQQ;
    }

    void SerializeVisitor::preVisit ( ExprLooksLikeCall * expr ) {
        // ExprCallFunc::dispatch and ExprOp::dispatch route here (no dedicated
        // preVisit overload). They carry func/stackTop/callFlags (and op for
        // ExprOp) that serializeLooksLikeCall alone would drop, so detect them
        // via __rtti and use the proper helper.
        if ( expr->rtti_isCallFunc() ) {
            if ( expr->__rtti && strcmp(expr->__rtti, "ExprOp") == 0 ) {
                serializeOp(static_cast<ExprOp*>(expr));
                return;
            }
            serializeCallFunc(static_cast<ExprCallFunc*>(expr));
            return;
        }
        serializeLooksLikeCall(expr);
    }

    void SerializeVisitor::preVisit ( ExprCallMacro * expr ) {
        serializeLooksLikeCall(expr);
        ser << expr->macro;
        ser.serializePointer(expr->inFunction);
    }

    void SerializeVisitor::preVisit ( ExprOp1 * expr ) {
        serializeOp(expr);
        ser << expr->subexpr;
    }

    void SerializeVisitor::preVisit ( ExprOp2 * expr ) {
        serializeOp2(expr);
    }

    void SerializeVisitor::preVisit ( ExprCopy * expr ) {
        serializeOp2(expr);
        ser << expr->copyFlags;
    }

    void SerializeVisitor::preVisit ( ExprMove * expr ) {
        serializeOp2(expr);
        ser << expr->moveFlags;
    }

    void SerializeVisitor::preVisit ( ExprClone * expr ) {
        serializeOp2(expr);
    }

    void SerializeVisitor::preVisit ( ExprOp3 * expr ) {
        serializeOp(expr);
        ser << expr->subexpr << expr->left << expr->right;
    }

    void SerializeVisitor::preVisit ( ExprTryCatch * expr ) {
        serializeBase(expr);
        ser << expr->try_block << expr->catch_block;
    }

    void SerializeVisitor::preVisit ( ExprReturn * expr ) {
        serializeBase(expr);
        ser << expr->subexpr    << expr->returnFlags << expr->stackTop << expr->refStackTop
            << expr->returnFunc << expr->block       << expr->returnType;
    }

    void SerializeVisitor::preVisit ( ExprConst * expr ) {
        serializeConst(expr);
    }

    void SerializeVisitor::preVisit ( ExprConstPtr * expr ) {
        serializeConst(expr);
        ser << expr->isSmartPtr << expr->ptrType;
    }

    void SerializeVisitor::preVisit ( ExprConstEnumeration * expr ) {
        serializeConst(expr);
        ser << expr->enumType << expr->text;
    }

    void SerializeVisitor::preVisit ( ExprConstBitfield * expr ) {
        serializeConst(expr);
        ser << expr->bitfieldType;
    }

    void SerializeVisitor::preVisit ( ExprConstString * expr ) {
        serializeConst(expr);
        ser << expr->text;
    }

    void SerializeVisitor::preVisit ( ExprStringBuilder * expr ) {
        serializeBase(expr);
        ser << expr->elements << expr->stringBuilderFlags;
    }

    void SerializeVisitor::preVisit ( ExprLet * expr ) {
        serializeBase(expr);
        ser << expr->variables << expr->visibility << expr->atInit << expr->letFlags;
    }

    void SerializeVisitor::preVisit ( ExprFor * expr ) {
        serializeBase(expr);
        ser << expr->iterators << expr->iteratorsAka << expr->iteratorsAt << expr->iteratorsTupleExpansion << expr->iteratorsTags
            << expr->iteratorVariables << expr->sources << expr->body << expr->visibility
            << expr->allowIteratorOptimization << expr->canShadow << expr->annotations;
    }

    void SerializeVisitor::preVisit ( ExprUnsafe * expr ) {
        serializeBase(expr);
        ser << expr->body;
    }

    void SerializeVisitor::preVisit ( ExprWhile * expr ) {
        serializeBase(expr);
        ser << expr->cond << expr->body << expr->annotations;
    }

    void SerializeVisitor::preVisit ( ExprWith * expr ) {
        serializeBase(expr);
        ser << expr->with << expr->body;
    }

    void SerializeVisitor::preVisit ( ExprAssume * expr ) {
        serializeBase(expr);
        ser << expr->alias << expr->subexpr << expr->assumeType;
    }

    void SerializeVisitor::preVisit ( ExprMakeBlock * expr ) {
        serializeBase(expr);
        ser << expr->capture << expr->captureAt << expr->block << expr->stackTop << expr->mmFlags;
    }

    void SerializeVisitor::preVisit ( ExprMakeGenerator * expr ) {
        serializeLooksLikeCall(expr);
        ser << expr->iterType << expr->capture << expr->captureAt;
    }

    void SerializeVisitor::preVisit ( ExprYield * expr ) {
        serializeBase(expr);
        ser << expr->subexpr << expr->returnFlags;
    }

    void SerializeVisitor::preVisit ( ExprInvoke * expr ) {
        serializeLooksLikeCall(expr);
        ser << expr->stackTop << expr->doesNotNeedSp << expr->isInvokeMethod << expr->cmresAlias;
    }

    void SerializeVisitor::preVisit ( ExprAssert * expr ) {
        serializeLooksLikeCall(expr);
        ser << expr->isVerify;
    }

    void SerializeVisitor::preVisit ( ExprQuote * expr ) {
        serializeLooksLikeCall(expr);
    }

    void SerializeVisitor::preVisit ( ExprTypeInfo * expr ) {
        serializeBase(expr);
        ser << expr->trait << expr->subexpr << expr->typeexpr << expr->subtrait << expr->extratrait << expr->macro;
    }

    void SerializeVisitor::preVisit ( ExprIs * expr ) {
        serializeBase(expr);
        ser << expr->subexpr << expr->typeexpr;
    }

    void SerializeVisitor::preVisit ( ExprAscend * expr ) {
        serializeBase(expr);
        ser << expr->subexpr << expr->ascType << expr->stackTop << expr->ascendFlags;
    }

    void SerializeVisitor::preVisit ( ExprCast * expr ) {
        serializeBase(expr);
        ser << expr->subexpr << expr->castType << expr->castFlags;
    }

    void SerializeVisitor::preVisit ( ExprNew * expr ) {
        serializeCallFunc(expr);
        ser << expr->typeexpr << expr->initializer << expr->allocate_on_stack;
    }

    void SerializeVisitor::preVisit ( ExprCall * expr ) {
        serializeCallFunc(expr);
        ser << expr->doesNotNeedSp << expr->cmresAlias;
    }

    void SerializeVisitor::preVisit ( ExprIfThenElse * expr ) {
        serializeBase(expr);
        ser << expr->cond << expr->if_true << expr->if_false << expr->ifFlags;
    }

    void SerializeVisitor::preVisit ( ExprNamedCall * expr ) {
        serializeBase(expr);
        ser << expr->name << expr->nonNamedArguments << expr->arguments << expr->argumentsFailedToInfer;
    }

    void SerializeVisitor::preVisit ( ExprMakeStruct * expr ) {
        serializeMakeLocal(expr);
        ser << expr->structs << expr->block << expr->makeStructFlags;
        ser.serializePointer(expr->constructor);
    }

    void SerializeVisitor::preVisit ( ExprMakeVariant * expr ) {
        serializeMakeLocal(expr);
        ser << expr->variants;
    }

    void SerializeVisitor::preVisit ( ExprMakeArray * expr ) {
        serializeMakeArray(expr);
    }

    void SerializeVisitor::preVisit ( ExprMakeTuple * expr ) {
        serializeMakeArray(expr);
        ser << expr->isKeyValue << expr->recordNames << expr->shorthandRecordNames;
    }

    void SerializeVisitor::preVisit ( ExprArrayComprehension * expr ) {
        serializeBase(expr);
        ser << expr->exprFor << expr->exprWhere << expr->subexpr << expr->generatorSyntax << expr->tableSyntax;
    }

    void SerializeVisitor::preVisit ( ExprTypeDecl * expr ) {
        serializeBase(expr);
        ser << expr->typeexpr;
    }

    AstSerializer & AstSerializer::operator << ( ExpressionPtr & expr ) {
        dtag(HASH_TAG("ExpressionPtr"));
        bool is_null = expr == nullptr;
        *this << is_null;
        if ( is_null ) {
            if ( !writing ) expr = nullptr;
            return *this;
        }
        SerializeVisitor sv(*this);
        if ( writing ) {
            uint32_t rtti = hash_tag(expr->__rtti);
            DAS_ASSERT(rtti);
            *this << rtti;
            expr->dispatch(sv);
        } else {
            uint32_t rtti = 0; *this << rtti;
            auto itA = rttiHash2Annotation.find(rtti);
            SERIALIZER_VERIFYF(itA != rttiHash2Annotation.end(), "annotation '%u' is not found", rtti);
            auto annotation = itA->second;
            expr = (Expression *) static_cast<TypeAnnotation*>(annotation)->factory();
            expr->dispatch(sv);
        }
        dtag(HASH_TAG("/ExpressionPtr"));
        return *this;
    }

    void MakeFieldDecl::serialize ( AstSerializer & ser ) {
        ser << at << name << value << tag << flags;
    }

    void MakeStruct::serialize( AstSerializer & ser ) {
        ser << static_cast <vector<MakeFieldDeclPtr> &> ( *this );
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
            gc_local<Expression> exprLooksLikeCall = (*callFactory)({});
            SERIALIZER_VERIFYF(
                strncmp("ExprCallMacro", exprLooksLikeCall->__rtti, 14) == 0,
                "excepted to see an ExprCallMacro"
            );
            ptr = static_cast<ExprCallMacro *>(exprLooksLikeCall.ptr)->macro;
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
                daScriptEnvironment::getBound()->g_Program = program;
                program->finalizeAnnotations();

                if ( is_macro_module ) {
                    auto time0 = ref_time_ticks();
                    reinstantiateMacroModuleState(ser, program);
                    ser.totMacroTime += get_time_usec(time0);
                }

            // collect TypeDecl nodes onto module root
                this_mod->gc_collect(gc_root::gc_get_active_root());

            // unbind the module from the program
                program->thisModule.release();
            } else {
                // we DO NOT collect something which is "already exists"
                // we leave it hanging, and we keep links to types from other modules and let them claim
                // this_mod->gc_collect(gc_root::gc_get_active_root());
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
                    auto fid = ser.getSerializeId(usedFun);
                    ser << fid;
                }
            }
        } else {
            string fname; ser << fname;
            SERIALIZER_VERIFYF(fname == f->name, "expected to serialize in the same order: %s != %s", fname.c_str(), f->name.c_str());
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
                    f->useFunctions.emplace(fun);
                } else {
                    SerializeNodeId fid; ser << fid;
                    auto fun = ser.smartFunctionMap[fid];
                    SERIALIZER_VERIFYF(fun, "expected to find function");
                    f->useFunctions.emplace(fun);
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
                    auto fid = ser.getSerializeId(usedFun);
                    ser << fid;
                }
            }
        } else {
            string name; ser << name;
            SERIALIZER_VERIFYF(name == f->name, "expected to serialize in the same order: %s != %s", name.c_str(), f->name.c_str());
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
                    f->useFunctions.emplace(fun);
                } else {
                    SerializeNodeId fid; ser << fid;
                    auto fun = ser.smartFunctionMap[fid];
                    SERIALIZER_VERIFYF(fun, "expected to find function");
                    f->useFunctions.emplace(fun);
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
                    auto vid = ser.getSerializeId(use);
                    ser << vid;
                }
            }
        } else {
            string name; ser << name;
            SERIALIZER_VERIFYF(name == f->name, "expected to serialize in the same order: %s %s", name.c_str(), f->name.c_str());
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
                    f->useGlobalVariables.emplace(var);
                } else {
                    SerializeNodeId vid; ser << vid;
                    auto var = ser.smartVariableMap[vid];
                    SERIALIZER_VERIFYF(var, "expected to find variable");
                    f->useGlobalVariables.emplace(var);
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
                    auto vid = ser.getSerializeId(use);
                    ser << vid;
                }
            }
        } else {
            string name; ser << name;
            SERIALIZER_VERIFYF(name == f->name, "expected to serialize in the same order: %s != %s", name.c_str(), f->name.c_str());
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
                    f->useGlobalVariables.emplace(var);
                } else {
                    SerializeNodeId vid; ser << vid;
                    auto var = ser.smartVariableMap[vid];
                    SERIALIZER_VERIFYF(var, "expected to find variable");
                    f->useGlobalVariables.emplace(var);
                }
            }
        }
    }

    void serializeGlobals ( AstSerializer & ser, safebox<Variable, VariablePtr> & globals ) {
        if ( ser.writing ) {
            uint64_t size = globals.unlocked_size(); ser << size;
            globals.foreach ( [&] ( VariablePtr g ) {
                ser << g;
            });
        } else {
            safebox<Variable, VariablePtr> result;
            uint64_t size = 0; ser << size;
            for ( uint64_t i = 0; i < size; i++ ) {
                VariablePtr g = nullptr; ser << g;
                SERIALIZER_VERIFYF(g!=nullptr, "expected to find variable");
                result.insert(g->name, g);
            }
            globals = das::move(result);
        }
    }

    void serializeStructures ( AstSerializer & ser, safebox<Structure, StructurePtr> & structures ) {
        if ( ser.writing ) {
            uint64_t size = structures.unlocked_size(); ser << size;
            structures.foreach ( [&] ( StructurePtr g ) {
                ser << g;
            });
        } else {
            uint64_t size = 0; ser << size;
            for ( uint64_t i = 0; i < size; i++ ) {
                StructurePtr g = nullptr; ser << g;
                SERIALIZER_VERIFYF(g!=nullptr, "expected to find structure");
                structures.insert(g->name, g);
            }
        }
    }

    void serializeFunctions ( AstSerializer & ser, safebox<Function, FunctionPtr> & functions ) {
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
                FunctionPtr g = nullptr; ser << g;
                SERIALIZER_VERIFYF(g!=nullptr, "expected to find function");
                functions.insert(name, g);
            }
        }
    }

    void serializeFunctionPointerVector ( AstSerializer & ser, vector<Function *> & functions ) {
        ser.dtag("Vector",hash_tag("Vector"));
        if ( ser.writing ) {
            uint64_t size = functions.size();
            ser.serializeAdaptiveSize64(size);
            for ( auto & f : functions ) ser.serializePointer(f);
        } else {
            uint64_t size = 0;
            ser.serializeAdaptiveSize64(size);
            functions.resize(size);
            for ( auto & f : functions ) ser.serializePointer(f);
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
                serializeFunctionPointerVector(ser, functions);
                count ++;
            });
            DAS_VERIFYF(count == size, "expected to serialize all functions");
        } else {
            uint32_t capacity = 0; ser << capacity;
            uint32_t size = 0; ser << size;
            functionsByName.reserve(capacity);
            for ( uint32_t i = 0; i < size; i++ ) {
                uint64_t nameHash = 0; ser << nameHash;
                vector<Function *> functions;
                serializeFunctionPointerVector(ser, functions);
                functionsByName[nameHash] = das::move(functions);
            }
        }
    }

    void Module::serialize ( AstSerializer & ser, bool already_exists ) {
        ser.tag(HASH_TAG("Module"));
        ser << name << nameHash << moduleFlags;
        ser << annotationData << requireModule;
        ser << aliasTypes     << enumerations;
        /*
        // serialize handleTypes (annotation lookup table)
        if ( ser.writing ) {
            uint64_t htSize = handleTypes.size();
            ser << htSize;
            for ( auto & [key, ann] : handleTypes ) {
                ser << key;
                AnnotationPtr a = ann;
                ser << a;
            }
        } else {
            uint64_t htSize = 0;
            ser << htSize;
            for ( uint64_t i = 0; i < htSize; i++ ) {
                uint64_t key = 0;
                ser << key;
                AnnotationPtr a = nullptr;
                ser << a;
                if ( a ) {
                    handleTypes[key] = a;
                }
            }
        }
        */
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

        functions.foreach ([&] ( FunctionPtr f ) {
            if ( ser.writing ) {
                ser << f->name;
            } else {
                string fname; ser << fname;
                SERIALIZER_VERIFYF(fname == f->name, "expected to walk in the same order: %s != %s", fname.c_str(), f->name.c_str());
            }
            serializeUseVariables(ser, f);
            serializeUseFunctions(ser, f);
        });

        generics.foreach ([&] ( FunctionPtr f ) {
            if ( ser.writing ) {
                ser << f->name;
            } else {
                string fname; ser << fname;
                SERIALIZER_VERIFYF(fname == f->name, "expected to walk in the same order: %s != %s", fname.c_str(), f->name.c_str());
            }
            serializeUseVariables(ser, f);
            serializeUseFunctions(ser, f);
        });

        globals.foreach ([&]( VariablePtr g ) {
            uint64_t hash = hash64z(g->name.c_str());
            if ( ser.writing ) {
                ser << hash;
            } else {
                uint64_t h = 0; ser << h;
                SERIALIZER_VERIFYF(h == hash, "expected to walk in the same order: %llu != %llu",
                    (unsigned long long) h, (unsigned long long) hash);
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
            // visibleEverywhere modules (!inscope)
            // are implicit dependencies of every other module
            if ( !mod->visibleEverywhere ) {
                for ( const auto dep : input ) {
                    if ( dep != mod && dep->visibleEverywhere ) {
                        visit(dep);
                    }
                }
            }
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
              << value.aot_macros
              << value.completion
              << value.export_all
              << value.serialize_main_module
              << value.keep_alive
              << value.very_safe_context
              << value.max_infer_passes
              << value.max_call_depth
              << value.verify_infer_types
              << value.stack
              << value.intern_strings
              << value.persistent_heap
              << value.multiple_contexts
              << value.heap_size_hint
              << value.string_heap_size_hint
              << value.solid_context
              << value.macro_context_persistent_heap
              << value.macro_context_collect
              << value.max_static_variables_size
              << value.max_heap_allocated
              << value.max_string_heap_allocated
              << value.track_allocations
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
              << value.no_writing_to_nameless
              << value.no_optimizations
              << value.no_infer_time_folding
              << value.fail_on_no_aot
              << value.fail_on_lack_of_aot_export
              << value.no_fast_call
              << value.scoped_stack_allocator
              << value.force_inscope_pod
              << value.log_inscope_pod
              << value.debugger
              << value.profiler
              << value.jit_enabled
              << value.jit_jit_all_functions
              << value.jit_debug_info
              << value.jit_opt_level
              << value.jit_size_level
              << value.jit_dll_mode
              << value.jit_output_path
              << value.jit_path_to_shared_lib
              << value.jit_path_to_linker
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
        // Bump epoch so reused pointer addresses across program boundaries
        // get distinct SerializeNodeIds on this persistent serializer.
        ser.epoch++;

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

                bool isNew = writingReadyModules.count(m) == 0;
                *this << isNew;
                if ( isNew ) {
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

                bool isNew = false;
                *this << isNew;
                Module * existing = libGroup.findModule(name);
                if ( !isNew ) {
                    if ( existing ) {
                        program->library.addModule(existing);
                        continue;
                    }
                    LOG(LogLevel::warning) << "das: serialize: module '" << name << "' not found";
                    program->failToCompile = true;
                    return;
                }

                Module* deser = nullptr;
                try {
                    deser = new Module();
                    deser->setModuleName(name);
                    if ( existing ) {
                        program->library.addModule(existing);
                        ser.serializeModule(*deser, /*already_exists*/true);
                        deser->builtIn = false; // suppress dtor unlink assert
                        delete deser;
                        continue;
                    }
                    program->library.addModule(deser);
                    ser << *deser;
                } catch ( const dasException & r ) {
                    delete deser;
                    LOG(LogLevel::warning) << "das: serialize: " << r.what();
                    program->failToCompile = true;
                    return;
                }
            }

            program->thisModule.reset(program->library.getModules().back());
        }

        // drop ref_counts
        smartEnumerationMap.clear();
        smartStructureMap.clear();
        smartVariableMap.clear();
        smartFunctionMap.clear();
        smartMakeStructMap.clear();
        smartTypeDeclMap.clear();
        exprBlockMap.clear();
    }

    uint32_t AstSerializer::getVersion () {
        static constexpr uint32_t currentVersion = 191;
        return currentVersion;
    }

    // Serializes the whole script as opposed to just one module
    bool WIN_EH_NO_ASAN AstSerializer::serializeScript ( ProgramPtr program ) noexcept {
        try {
            program->serialize(*this);
            return true;
        } catch ( const dasException & r ) {
            program->failToCompile = true;
            LOG(LogLevel::warning) << "das: serialize:" << r.what();
            return false;
        }
    }

    // Used in daNetGame currently
    void Program::serialize ( AstSerializer & ser ) {
        // Bump epoch so reused pointer addresses across program boundaries
        // get distinct SerializeNodeIds on this persistent serializer.
        ser.epoch++;

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

    AstSerializerState * rtti_create_ast_serializer () {
        auto state = new AstSerializerState();
        state->storage = make_unique<SerializationStorageVector>();
        state->serializer = make_unique<AstSerializer>(state->storage.get(), true);
        return state;
    }

    AstSerializerState * rtti_create_ast_deserializer ( const TArray<uint8_t> & data ) {
        auto state = new AstSerializerState();
        state->storage = make_unique<SerializationStorageVector>();
        state->storage->buffer.assign(data.data, data.data + data.size);
        state->serializer = make_unique<AstSerializer>(state->storage.get(), false);
        return state;
    }

    void rtti_delete_ast_serializer ( AstSerializerState * state ) {
        if ( state ) {
            state->serializer->moduleLibrary = nullptr;
            delete state;
        }
    }

    bool rtti_ast_serializer_serialize_program (
            AstSerializerState * state,
            const smart_ptr<Program> & program ) {
        auto & prog = const_cast<smart_ptr<Program> &>(program);
        prog->serialize(*state->serializer);
        return !prog->failToCompile;
    }

    void rtti_ast_serializer_deserialize_program (
            AstSerializerState * state,
            const TBlock<void,bool,smart_ptr<Program>,const string> & block,
            Context * context, LineInfoArg * at ) {
        auto prog = make_smart<Program>();
        {
            gc_guard deserialize_gc_scope;
            prog->serialize(*state->serializer);
            /*
            // THIS ONES ARE FROM THE "already exist" MODULES
            auto leftover = deserialize_gc_scope.guard_root.gc_count;
            if ( leftover ) {
                LOG(LogLevel::warning) << "das: deserialize: " << leftover << " gc_node(s) left after deserialization\n";
                deserialize_gc_scope.guard_root.gc_dump_to_thread_root();
            }
            */
        }
        // Module::serialize leaves g_Program pointing to a temporary program with a
        // released thisModule. Restore it so compiling_module() sees the correct module
        // during simulate() and while tests run inside the block.
        auto & bound = *daScriptEnvironment::getBound();
        auto savedProg = bound.g_Program;
        bound.g_Program = prog.get();
        if ( prog->failToCompile ) {
            string err = "deserialization failed";
            das_invoke<void>::invoke<bool,smart_ptr<Program>,const string &>(
                context, at, block, false, ProgramPtr(), err);
            (void)prog->thisModule.release();
            prog->library.reset();
            bound.g_Program = savedProg;
            return;
        }
        string okStr;
        das_invoke<void>::invoke<bool,smart_ptr<Program>,const string &>(
            context, at, block, true, prog, okStr);
        (void)prog->thisModule.release();
        prog->library.reset();
        bound.g_Program = savedProg;
    }

    void rtti_ast_serializer_get_data (
            AstSerializerState * state,
            const TBlock<void,TTemporary<TArray<uint8_t> const>> & block,
            Context * context, LineInfoArg * at ) {
        Array arr;
        array_mark_locked(arr, state->storage->buffer.data(),
            uint64_t(state->storage->buffer.size()), uint64_t(state->storage->buffer.size()));
        vec4f args[1] = { cast<Array *>::from(&arr) };
        context->invoke(block, args, nullptr, at);
    }

}
