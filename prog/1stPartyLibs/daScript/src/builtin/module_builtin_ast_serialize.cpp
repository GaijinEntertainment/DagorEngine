#include "daScript/misc/das_common.h"
#include "daScript/misc/platform.h"

#include "daScript/misc/dep_recorder.h"
#include "daScript/misc/performance_time.h"

#include "daScript/ast/ast_serialize_macro.h"
#include "daScript/ast/ast_serializer.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/misc/anyhash.h"
#include "daScript/misc/sysos.h"
#include "daScript/misc/env_cfg.h"
#include "daScript/simulate/aot_builtin.h"
#include <cstdarg>
#include <cstdio>
#include <sys/stat.h>
#include <algorithm>
#if !DAS_NO_FILEIO
#ifdef _WIN32
#include <io.h>
#include <process.h>
#include <sys/utime.h>
#else
#include <dirent.h>
#include <unistd.h>
#include <utime.h>
#endif
#endif
#ifdef _WIN32
#include <direct.h>
#include <stdlib.h>
#endif
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
        bufferAsVector = storage->asVector();
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

    void AstSerializer::clearNodeIds () {
        writeIds.clear();
        writtenIds.clear();
        readNodes.clear();
        pendingRefs.clear();
        writeMangledNames.clear();
        writeStrings.clear();
        emptyStringNumber = -1;
        readStrings.clear();
        stringArena.clear();
        writeFileInfos.clear();
        readFileInfos.clear();
        lastWriteFileInfo = nullptr;
        lineBaseFile = nullptr;
        lineBaseLine = 0;
        writeTypes.clear();
        writeFreshTypeCount = 0;
        readTypes.clear();
        writeExprClasses.clear();
        readExprClasses.clear();
        writeModules.clear();
        readModules.clear();
    }

    void AstSerializer::patch () {
        for ( auto & p : pendingRefs ) {
            auto node = readNodes[p.second.index];
            if ( !node ) throw_formatted_error("ast serializer ref #%u not found", p.second.index);
            memcpy(p.first, &node, sizeof(node));   // the slot is some TT *, not a void *; a typed store through void ** would alias it

        }
        pendingRefs.clear();

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

    void AstSerializer::read ( void * data, size_t size ) {
        if ( !buffer->read(data, size) ) {
            throw_formatted_error("ast serializer read overflow");
        }
    }

    void AstSerializer::onReadFailure () {
        throw dasException{"ast serializer read overflow", LineInfo()};
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
        } catch ( ... ) {
            failed = true;
            return false;
        }
    }

    #define HASH_TAG(tag)   tag,hash_tag(tag)

    void AstSerializer::tag ( const char * name, uint32_t hash ) {
        DAS_SER_PROFILE(*this, "tag");
        if ( writing ) {
            serialize(hash);
        } else  {
            uint32_t hash2 = 0;
            serialize(hash2);
            if ( hash != hash2 ) {
                throw_formatted_error("ast serializer tag '%s' mismatch", name);
            }
        }
    }

#if DAS_SERIALIZE_PROFILE
    void AstSerializer::profBegin ( const char * name ) {
        if ( profNodes.empty() ) {
            profNodes.push_back({});
            profStartTicks = ref_time_ticks();
        }
        uint32_t nameId = ~0u;
        for ( auto & [ptr, id] : profNameIds ) {
            if ( ptr == name ) { nameId = id; break; }
        }
        if ( nameId == ~0u ) {
            nameId = uint32_t(profNames.size());
            profNames.push_back(name);
            profAgg.push_back({});
            profNameIds.push_back({name, nameId});
        }
        uint32_t parent = profStack.empty() ? 0 : profStack.back().node;
        uint32_t node = ~0u;
        for ( auto child : profNodes[parent].children ) {
            if ( profNodes[child].nameId == nameId ) { node = child; break; }
        }
        if ( node == ~0u ) {
            node = uint32_t(profNodes.size());
            profNodes.push_back({nameId, parent, 0, 0, 0, {}});
            profNodes[parent].children.push_back(node);
        }
        profStack.push_back({nameId, node, profPosition(), 0, ref_time_ticks()});
    }

    void AstSerializer::profEnd () {
        auto frame = profStack.back();
        profStack.pop_back();
        uint64_t bytes = profPosition() - frame.startBytes;
        int64_t ticks = get_time_nsec(frame.startTicks);
        auto & node = profNodes[frame.node];
        node.inclBytes += bytes;
        node.count += 1;
        node.inclTicks += ticks;
        auto & agg = profAgg[frame.nameId];
        agg.count += 1;
        agg.selfBytes += bytes - frame.childBytes;
        bool outermost = true;
        for ( auto & f : profStack ) {
            if ( f.nameId == frame.nameId ) { outermost = false; break; }
        }
        if ( outermost ) {
            agg.outermostInclBytes += bytes;
            agg.outermostInclTicks += ticks;
        }
        if ( !profStack.empty() ) profStack.back().childBytes += bytes;
    }

    void AstSerializer::profString ( const string & str ) {
        profStrings[str] += 1;
    }

    void AstSerializer::profType ( const TypeDecl * type, uint64_t bytes ) {
        profTypes[type->getMangledName(true)] += 1;
        profTypeBytes += bytes;
    }

    static string profFmtBytes ( uint64_t b ) {
        char buf[64];
        if ( b >= 10*1024*1024 ) snprintf(buf, sizeof(buf), "%.1f MB", double(b) / (1024.0*1024.0));
        else if ( b >= 10*1024 ) snprintf(buf, sizeof(buf), "%.1f KB", double(b) / 1024.0);
        else snprintf(buf, sizeof(buf), "%llu B", (unsigned long long) b);
        return buf;
    }

    // src/builtin/ARCHITECTURE.md#serialization-profile-rail
    void AstSerializer::profReport ( TextWriter & tw ) const {
        if ( profNodes.empty() ) return;
        uint64_t total = 0;
        int64_t totalTicks = 0;
        for ( auto & n : profNodes ) {
            if ( n.parent == 0 && &n != &profNodes[0] ) { total += n.inclBytes; totalTicks += n.inclTicks; }
        }
        if ( total == 0 ) total = 1;
        auto pct = [&](uint64_t b) { return 100.0 * double(b) / double(total); };
        tw << "=== serialization profile (" << (writing ? "writing" : "reading") << "): "
           << profFmtBytes(total) << " (" << total << " bytes), " << (totalTicks / 1000000.0) << " ms inside frames ===\n";
        das_hash_map<string, ProfAgg> byText;
        for ( size_t i = 0; i != profNames.size(); ++i ) {
            auto & a = byText[profNames[i]];
            a.selfBytes += profAgg[i].selfBytes;
            a.outermostInclBytes += profAgg[i].outermostInclBytes;
            a.count += profAgg[i].count;
            a.outermostInclTicks += profAgg[i].outermostInclTicks;
        }
        vector<pair<string, ProfAgg>> rows(byText.begin(), byText.end());
        auto printRows = [&](const char * title, uint64_t ProfAgg::* field, bool showTicks) {
            sort(rows.begin(), rows.end(), [&](auto & a, auto & b) { return a.second.*field > b.second.*field; });
            tw << "-- " << title << " --\n";
            char line[256];
            int shown = 0;
            for ( auto & r : rows ) {
                uint64_t v = r.second.*field;
                if ( v == 0 || shown++ >= 48 ) break;
                if ( showTicks ) {
                    snprintf(line, sizeof(line), "%-28s %12llu %6.2f%% %10llu x %8.1f  %8.2f ms\n", r.first.c_str(),
                        (unsigned long long) v, pct(v), (unsigned long long) r.second.count,
                        double(v) / double(r.second.count ? r.second.count : 1), r.second.outermostInclTicks / 1000000.0);
                } else {
                    snprintf(line, sizeof(line), "%-28s %12llu %6.2f%% %10llu x %8.1f\n", r.first.c_str(),
                        (unsigned long long) v, pct(v), (unsigned long long) r.second.count,
                        double(v) / double(r.second.count ? r.second.count : 1));
                }
                tw << line;
            }
        };
        printRows("self bytes by frame (name, bytes, %, count, avg)", &ProfAgg::selfBytes, false);
        printRows("inclusive bytes by frame, outermost only (name, bytes, %, count, avg, ms)", &ProfAgg::outermostInclBytes, true);
        tw << "-- tree (depth<=6, >=0.5%) --\n";
        vector<vector<uint32_t>> children(profNodes.size());
        for ( uint32_t i = 1; i < uint32_t(profNodes.size()); ++i ) children[profNodes[i].parent].push_back(i);
        for ( auto & c : children ) sort(c.begin(), c.end(), [&](uint32_t a, uint32_t b) { return profNodes[a].inclBytes > profNodes[b].inclBytes; });
        vector<pair<uint32_t,int>> walk;
        for ( auto it = children[0].rbegin(); it != children[0].rend(); ++it ) walk.push_back({*it, 0});
        char line[512];
        while ( !walk.empty() ) {
            auto [ni, depth] = walk.back(); walk.pop_back();
            auto & n = profNodes[ni];
            if ( pct(n.inclBytes) < 0.5 ) continue;
            snprintf(line, sizeof(line), "%*s%-*s %12llu %6.2f%% %9llu x %8.1f  %8.2f ms\n", depth*2, "", 30 - depth*2,
                profNames[n.nameId].c_str(), (unsigned long long) n.inclBytes, pct(n.inclBytes), (unsigned long long) n.count,
                double(n.inclBytes) / double(n.count ? n.count : 1), n.inclTicks / 1000000.0);
            tw << line;
            if ( depth < 6 ) {
                for ( auto it = children[ni].rbegin(); it != children[ni].rend(); ++it ) walk.push_back({*it, depth + 1});
            }
        }
        {
            uint64_t occ = 0, bytes = 0, distinct = profStrings.size(), distinctBytes = 0;
            for ( auto & [s, c] : profStrings ) { occ += c; bytes += uint64_t(s.size()) * c; distinctBytes += s.size(); }
            tw << "-- strings: " << occ << " occurrences, " << profFmtBytes(bytes) << " payload, "
               << distinct << " distinct (" << profFmtBytes(distinctBytes) << "); a per-record string table would take ~"
               << profFmtBytes(distinctBytes + distinct + occ * 2) << "\n";
            vector<pair<string,uint32_t>> top(profStrings.begin(), profStrings.end());
            sort(top.begin(), top.end(), [](auto & a, auto & b) { return uint64_t(a.first.size()) * a.second > uint64_t(b.first.size()) * b.second; });
            for ( size_t i = 0; i < top.size() && i < 24; ++i ) {
                snprintf(line, sizeof(line), "   %8u x %5u B  %.60s\n", top[i].second, unsigned(top[i].first.size()), top[i].first.c_str());
                tw << line;
            }
        }
        {
            uint64_t occ = 0;
            for ( auto & [s, c] : profTypes ) occ += c;
            tw << "-- TypeDecl payloads: " << occ << " written, " << profFmtBytes(profTypeBytes) << ", "
               << profTypes.size() << " distinct mangled types; a per-record type table would take ~"
               << profFmtBytes(profTypes.size() ? profTypeBytes * profTypes.size() / (occ ? occ : 1) + occ * 2 : 0) << "\n";
            vector<pair<string,uint32_t>> top(profTypes.begin(), profTypes.end());
            sort(top.begin(), top.end(), [](auto & a, auto & b) { return a.second > b.second; });
            for ( size_t i = 0; i < top.size() && i < 16; ++i ) {
                snprintf(line, sizeof(line), "   %8u x  %.80s\n", top[i].second, top[i].first.c_str());
                tw << line;
            }
        }
        if ( !profRecords.empty() ) {
            auto recs = profRecords;
            sort(recs.begin(), recs.end(), [](auto & a, auto & b) { return a.payloadBytes > b.payloadBytes; });
            uint64_t sum = 0;
            for ( auto & r : recs ) sum += r.lengthWordBytes + r.payloadBytes;
            tw << "-- records: " << recs.size() << ", " << profFmtBytes(sum) << " (payload, length word, ms, file) --\n";
            for ( auto & r : recs ) {
                snprintf(line, sizeof(line), "%12llu %6llu %8.2f  %s\n", (unsigned long long) r.payloadBytes,
                    (unsigned long long) r.lengthWordBytes, r.usec / 1000.0, r.file.c_str());
                tw << line;
            }
        }
    }
#endif

    ////////////////////////////////////////////////////////////////////////////

    // src/builtin/ARCHITECTURE.md#module-cache-record-bytes
    void AstSerializer::serializeAdaptiveSize32 ( uint32_t & size ) {
        if ( writing ) {
            uint8_t enc[5];
            write(enc, encodeAdaptiveSize32(enc, size));
        } else {
            if ( bufferAsVector ) {
                auto & pos = bufferAsVector->bufferPos;
                auto & buf = bufferAsVector->buffer;
                if ( buf.size() - pos >= 5 ) {
                    const uint8_t * p = buf.data() + pos;
                    uint32_t b = p[0];
                    if ( b < 0x80 ) {
                        size = b;
                        pos += 1;
                        return;
                    }
                    uint32_t value = b & 0x7f;
                    size_t n = 1;
                    for ( ;; ) {
                        b = p[n];
                        value |= (b & 0x7f) << (7 * n);
                        n ++;
                        if ( b < 0x80 ) break;
                        SERIALIZER_VERIFYF(n < 5, "corrupt stream: adaptive size runs past five bytes");
                    }
                    size = value;
                    pos += n;
                    return;
                }
            }
            uint32_t value = 0;
            for ( size_t n = 0; ; ++n ) {
                SERIALIZER_VERIFYF(n < 5, "corrupt stream: adaptive size runs past five bytes");
                uint8_t b = 0; *this << b;
                value |= uint32_t(b & 0x7f) << (7 * n);
                if ( b < 0x80 ) break;
            }
            size = value;
        }
    }

    // a stream-backed length can never exceed the bytes left (every element costs at
    // least one byte) - a corrupted length must fail HERE, as a recoverable throw,
    // not as a layout-dependent wild allocation later (SIGSEGV under one allocator,
    // clean bad_alloc under another - the module-cache corruption CI red). Called at
    // the sites that ALLOCATE from a deserialized count - not inside the varint codec,
    // which also carries plain values (line diffs, sourceLength of a file that is not
    // in the stream) where the invariant does not hold
    void AstSerializer::verifyLength ( uint64_t size ) {
        SERIALIZER_VERIFYF(size <= buffer->readRemaining(),
            "corrupt stream: size %llu exceeds remaining bytes", (unsigned long long)size);
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
        DAS_SER_PROFILE(*this, "NodeId");
        serializeAdaptiveSize32(value.index);
        if ( !writing ) {
            // numbers arrive in first-mention order, so a first sight is exactly the next
            // slot; anything past it is a corrupt stream, not a resize
            if ( readNodes.empty() ) readNodes.push_back(nullptr);
            if ( value.index == readNodes.size() ) {
                readNodes.push_back(nullptr);
            } else {
                SERIALIZER_VERIFYF(value.index < readNodes.size(), "corrupt stream: node #%u past the %u numbered so far",
                    value.index, unsigned(readNodes.size()));
            }
        }
        return *this;
    }

    // src/builtin/ARCHITECTURE.md#module-cache-record-bytes
    AstSerializer & AstSerializer::serializeString ( string & str, bool temp ) {
        dtag(HASH_TAG("string"));
        DAS_SER_PROFILE(*this, "string");
        if ( writing ) {
#if DAS_SERIALIZE_PROFILE
            profString(str);
#endif
            SERIALIZER_VERIFYF(uint64_t(str.size()) < (uint64_t(1) << 32), "string too long");
            uint32_t length = uint32_t(str.size());
            if ( length == 0 && emptyStringNumber >= 0 ) {
                uint32_t known = uint32_t(emptyStringNumber);
                serializeAdaptiveSize32(known);
                return *this;
            }
            StringView view { str.data(), length };
            if ( auto it = writeStrings.find(view); it != writeStrings.end() ) {
                serializeAdaptiveSize32(it->second);
                return *this;
            }
            if ( temp ) {
                stringArena.push_back(make_unique<string>(str));
                view.data = stringArena.back()->data();
            }
            uint32_t index = uint32_t(writeStrings.size());
            writeStrings.emplace(view, index);
            if ( length == 0 ) emptyStringNumber = int32_t(index);
            serializeAdaptiveSize32(index);
            serializeAdaptiveSize32(length);
            write(str.data(), length);
        } else {
            uint32_t index = 0;
            serializeAdaptiveSize32(index);
            if ( index < readStrings.size() ) {
                auto & view = readStrings[index];
                str.assign(view.data, view.length);
                return *this;
            }
            SERIALIZER_VERIFYF(index == readStrings.size(), "corrupt stream: string #%u past the %u numbered so far",
                index, unsigned(readStrings.size()));
            uint32_t length = 0;
            serializeAdaptiveSize32(length);
            verifyLength(length);
            str.resize(length);
            if ( length ) read(&str[0], length);
            const char * data = str.data();
            if ( temp ) {
                stringArena.push_back(make_unique<string>(str));
                data = stringArena.back()->data();
            }
            readStrings.push_back({data, length});
        }
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( const char * & value ) {
        dtag(HASH_TAG("const char *"));
        DAS_SER_PROFILE(*this, "cstring");
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
            verifyLength(len);
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
        DAS_SER_PROFILE(*this, "Safebox");
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
        DAS_SER_PROFILE(*this, "HashMap");
        if ( writing ) {
            uint64_t size = value.size(); *this << size;
            for ( auto & item : value ) {
                serializeTemp(item.first); serializeTemp(item.second);
            }
            return;
        }
        uint64_t size = 0; *this << size;
        verifyLength(size);
        das_hash_map<K, V, H, E> deser;
        deser.reserve(size);
        for ( uint64_t i = 0; i < size; i++ ) {
            K k; V v; serializeTemp(k); serializeTemp(v);
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
        DAS_SER_PROFILE(*this, "HashMap");
        if ( writing ) {
            uint64_t size = value.size(); *this << size;
            for ( auto & item : value ) {
                serializeTemp(item.first); serializeTemp(item.second);
            }
            return;
        }
        uint64_t size = 0; *this << size;
        verifyLength(size);
        das_insert_only_hash_map<K, V, H, E> deser;
        deser.reserve(size);
        for ( uint64_t i = 0; i < size; i++ ) {
            K k; V v; serializeTemp(k); serializeTemp(v);
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

    // A module-less struct/enum (e.g. a local/anonymous type reached through a
    // Variable's type or init) has no identification to write -- treat it as
    // in-this-module so it goes through the id/patch path instead of
    // writeIdentifications, which would dereference the null module. Mirrors the
    // isInThisModule(Variable*&) overload below. Defence-in-depth for the
    // serializePointer(parent/classParent) sites; the TypeDecl struct/enum cases
    // route module-less types through the inline path before reaching here.
    bool AstSerializer::isInThisModule ( Enumeration * & ptr ) {
        return ptr->module == thisModule || ptr->module == nullptr;
    }

    bool AstSerializer::isInThisModule ( Structure * & ptr ) {
        return ptr->module == thisModule || ptr->module == nullptr;
    }

    bool AstSerializer::isInThisModule ( Variable * & ptr ) {
        return ptr->module == thisModule || ptr->module == nullptr;;
    }

    bool AstSerializer::isInThisModule ( TypeInfoMacro * & ptr ) {
        return ptr->module == thisModule;
    }

    // src/builtin/ARCHITECTURE.md#module-cache-record-bytes
    void AstSerializer::writeIdentifications ( Function * & func ) {
        auto & name = writeMangledNames[func];
        if ( name.empty() ) name = func->getMangledName();
        Module * module = func->module;
        *this << module;
        serializeTemp(name);
    }

    void AstSerializer::writeIdentifications ( Enumeration * & ptr ) {
        Module * module = ptr->module;
        *this << module;
        uint64_t nameHash = hash64z(ptr->name.c_str());
        *this << nameHash;
    }

    void AstSerializer::writeIdentifications ( Structure * & ptr ) {
        Module * module = ptr->module;
        *this << module;
        uint64_t nameHash = hash64z(ptr->name.c_str());
        *this << nameHash;
    }

    void AstSerializer::writeIdentifications ( Variable * & ptr ) {
        Module * module = ptr->module;
        *this << module << ptr->name;
    }

    void AstSerializer::writeIdentifications ( TypeInfoMacro * & ptr ) {
        Module * module = ptr->module;
        *this << module << ptr->name;
    }

    auto AstSerializer::readModuleAndNameHash () -> pair<Module *, uint64_t> {
        Module * funcModule = nullptr;
        uint64_t mangledNameHash = 0;
        *this << funcModule << mangledNameHash;
        return {funcModule, mangledNameHash};
    }

    auto AstSerializer::readModuleAndName () -> pair<Module *, string> {
        Module * funcModule = nullptr;
        string mangledName;
        *this << funcModule;
        serializeTemp(mangledName);
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
        // under ignoreEmptyExternal (function arguments / field inits) a missing external
        // module reaches here as mod==nullptr. Unlike a function (which infer re-resolves
        // by name when the default expr is cloned at a call site), a null type pointer is
        // never re-resolved and crashes later -- so fail the read; the boundary catches
        // the exception and the caller falls back to text parsing
        SERIALIZER_VERIFYF(mod, "module for enumeration '%llu' is not found", mangledNameHash);
        ptr = mod->findEnumByMangledNameHash(mangledNameHash);
        SERIALIZER_VERIFYF(ptr!=nullptr, "enumeration '%llu' is not found", mangledNameHash);
    }

    void AstSerializer::findExternal ( Structure * & ptr ) {
        auto [mod, mangledNameHash] = readModuleAndNameHash();
        // missing external module under ignoreEmptyExternal - see findExternal(Enumeration*&)
        SERIALIZER_VERIFYF(mod, "module for structure '%llu' is not found", mangledNameHash);
        ptr = mod->findStructureByMangledNameHash(mangledNameHash);
        SERIALIZER_VERIFYF(ptr!=nullptr, "structure '%llu' is not found", mangledNameHash);
    }

    void AstSerializer::findExternal ( Variable * & ptr ) {
        auto [mod, mangledName] = readModuleAndName();
        // missing external module under ignoreEmptyExternal - see findExternal(Enumeration*&)
        SERIALIZER_VERIFYF(mod, "module for variable '%s' is not found", mangledName.c_str());
        ptr = mod->findVariable(mangledName);
        SERIALIZER_VERIFYF(ptr!=nullptr, "variable '%s' is not found", mangledName.c_str());
    }

    void AstSerializer::findExternal ( TypeInfoMacro * & ptr ) {
        auto [mod, mangledName] = readModuleAndName();
        // missing external module under ignoreEmptyExternal - see findExternal(Enumeration*&)
        SERIALIZER_VERIFYF(mod, "module for type info macro '%s' is not found", mangledName.c_str());
        ptr = mod->findTypeInfoMacro(mangledName);
        SERIALIZER_VERIFYF(ptr!=nullptr, "type info macro '%s' is not found", mangledName.c_str());
    }

#if DAS_SERIALIZE_PROFILE
    static const char * serializePointerName ( Function * ) { return "Pointer<Function>"; }
    static const char * serializePointerName ( Enumeration * ) { return "Pointer<Enumeration>"; }
    static const char * serializePointerName ( Structure * ) { return "Pointer<Structure>"; }
    static const char * serializePointerName ( Variable * ) { return "Pointer<Variable>"; }
#endif

    template<typename TT>
    AstSerializer & AstSerializer::serializePointer ( TT * & ptr ) {
        DAS_SER_PROFILE(*this, serializePointerName(ptr));
        auto fid = getSerializeId(ptr);
        *this << fid;
        if ( !fid.index ) {
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
        DAS_SER_PROFILE(*this, "FunctionPtr");
        if ( writing && func ) {
            SERIALIZER_VERIFYF(!func->builtIn, "cannot serialize built-in function");
        }
        auto id = getSerializeId(func);
        *this << id;
        if ( id.index == 0 ) {
            if ( !writing ) func = nullptr;
            return *this;
        }
        if ( writing ) {
            if ( !isWritten(id) ) {
                markWritten(id);
                func->serialize(*this);
            }
        } else {
            if ( auto node = readNode<Function>(id) ) {
                func = node;
            } else {
                func = new Function();
                setReadNode(id, func);
                func->serialize(*this);
            }
        }
        if ( func ) {
            DAS_SER_PROFILE(*this, "FunctionPtr.nameCheck");
            if ( writing ) {
                *this << func->name;
            } else {
                string name; serializeTemp(name);
                string expect = func->name;
                SERIALIZER_VERIFYF(name == expect, "expected different function %s %s", name.c_str(), expect.c_str());
            }
        }
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( TypeInfoMacro * & ptr ) {
        dtag(HASH_TAG("TypeInfoMacroPtr"));
        DAS_SER_PROFILE(*this, "TypeInfoMacro*");
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

    static uint32_t aliasCacheFlagsMask () {
        static const uint32_t mask = [] {
            TypeDecl probe;
            probe.flags = 0;
            probe.aliasCacheValid = true;
            probe.aliasCacheHasAlias = true;
            return probe.flags;
        }();
        return mask;
    }

    static bool typeTableable ( const TypeDecl * t ) {
        if ( t->fixedDimExpr || !t->typeMacroExpr.empty() ) return false;
        if ( t->firstType && !typeTableable(t->firstType) ) return false;
        if ( t->secondType && !typeTableable(t->secondType) ) return false;
        for ( auto & a : t->argTypes ) {
            if ( a && !typeTableable(a) ) return false;
        }
        return true;
    }

    static uint64_t hashTypeContent ( const TypeDecl * t ) {
        // FNV-1a over the words: hash_combine64 runs wyhash per word and costs 4 ms of a 16 ms write
        uint64_t h = 14695981039346656037ull;
        auto mix = [&h]( uint64_t v ) { h = (h ^ v) * 1099511628211ull; };
        mix(uint32_t(t->baseType));
        mix(t->flags & ~aliasCacheFlagsMask());
        mix(uint32_t(t->fixedDim));
        mix(uintptr_t(t->structType) >> 4);
        mix(uintptr_t(t->enumType) >> 4);
        mix(uintptr_t(t->annotation) >> 4);
        mix(uintptr_t(t->module) >> 4);
        if ( !t->alias.empty() ) mix(hash_block64((const uint8_t *) t->alias.data(), t->alias.size()));
        mix(t->firstType ? hashTypeContent(t->firstType) : 7);
        mix(t->secondType ? hashTypeContent(t->secondType) : 11);
        mix(t->argTypes.size());
        for ( auto & a : t->argTypes ) mix(a ? hashTypeContent(a) : 13);
        mix(t->argNames.size());
        for ( auto & n : t->argNames ) mix(hash_block64((const uint8_t *) n.data(), n.size()));
        return h;
    }

    static bool sameTypeContent ( const TypeDecl * a, const TypeDecl * b ) {
        if ( a == b ) return true;
        if ( a->baseType != b->baseType ) return false;
        if ( ((a->flags ^ b->flags) & ~aliasCacheFlagsMask()) != 0 ) return false;
        if ( a->fixedDim != b->fixedDim ) return false;
        if ( a->structType != b->structType || a->enumType != b->enumType || a->annotation != b->annotation || a->module != b->module ) return false;
        if ( a->alias != b->alias ) return false;
        if ( (a->firstType == nullptr) != (b->firstType == nullptr) ) return false;
        if ( a->firstType && !sameTypeContent(a->firstType, b->firstType) ) return false;
        if ( (a->secondType == nullptr) != (b->secondType == nullptr) ) return false;
        if ( a->secondType && !sameTypeContent(a->secondType, b->secondType) ) return false;
        if ( a->argTypes.size() != b->argTypes.size() ) return false;
        for ( size_t i = 0; i != a->argTypes.size(); ++i ) {
            if ( (a->argTypes[i] == nullptr) != (b->argTypes[i] == nullptr) ) return false;
            if ( a->argTypes[i] && !sameTypeContent(a->argTypes[i], b->argTypes[i]) ) return false;
        }
        if ( a->argNames != b->argNames ) return false;
        return true;
    }

    size_t AstSerializer::TypeHash::operator () ( const TypeDecl * t ) const noexcept {
        return size_t(hashTypeContent(t));
    }

    bool AstSerializer::TypeEqual::operator () ( const TypeDecl * a, const TypeDecl * b ) const noexcept {
        return sameTypeContent(a, b);
    }

    enum TypeRef : uint32_t { TypeRefNull = 0, TypeRefInline = 1, TypeRefFresh = 2, TypeRefFirstEntry = 3 };

    // src/builtin/ARCHITECTURE.md#module-cache-record-bytes
    AstSerializer & AstSerializer::operator << ( TypeDeclPtr & type ) {
        dtag(HASH_TAG("TypeDeclPtr"));
        DAS_SER_PROFILE(*this, "TypeDeclPtr");
        if ( writing ) {
            if ( !type ) {
                uint32_t ref = TypeRefNull;
                serializeAdaptiveSize32(ref);
                return *this;
            }
            if ( !typeTableable(type) ) {
                uint32_t ref = TypeRefInline;
                serializeAdaptiveSize32(ref);
                type->serialize(*this);
                return *this;
            }
            if ( auto it = writeTypes.find(type); it != writeTypes.end() ) {
                uint32_t known = it->second;
                serializeAdaptiveSize32(known);
                *this << type->at;
                return *this;
            }
            uint32_t ref = TypeRefFresh;
            serializeAdaptiveSize32(ref);
            type->serialize(*this);
            writeFreshTypeCount ++;
            writeTypes.emplace(type, writeFreshTypeCount - 1 + TypeRefFirstEntry);
        } else {
            uint32_t number = 0;
            serializeAdaptiveSize32(number);
            if ( number == TypeRefNull ) {
                type = nullptr;
                return *this;
            }
            if ( number == TypeRefInline ) {
                type = new TypeDecl();
                type->serialize(*this);
                return *this;
            }
            if ( number == TypeRefFresh ) {
                type = new TypeDecl();
                type->serialize(*this);
                readTypes.push_back(type);
                return *this;
            }
            uint32_t index = number - TypeRefFirstEntry;
            SERIALIZER_VERIFYF(index < readTypes.size(), "corrupt stream: type #%u past the %u numbered so far",
                number, unsigned(readTypes.size()));
            type = new TypeDecl(*readTypes[index]);
            *this << type->at;
        }
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( AnnotationArgument & arg ) {
        dtag(HASH_TAG("AnnotationArgument"));
        DAS_SER_PROFILE(*this, "AnnotationArgument");
        arg.serialize(*this);
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( AnnotationDeclarationPtr & annotation_decl ) {
        dtag(HASH_TAG("AnnotationDeclarationPtr"));
        DAS_SER_PROFILE(*this, "AnnotationDecl");
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
                    Module * module = anno->module;
                    ser << module;
                }
            } else {
                // das-declared distinct-type entities round-trip with the module itself
                // (Module::serialize registers them before anything that references them
                // deserializes), so an own-module reference resolves by name
                bool isDistinct = anno->rtti_isDistinctTypeAnnotation();
                ser << isDistinct;
                if ( isDistinct ) {
                    ser << anno->name;
                } else {
                    // If the macro is from current module, do nothing
                    // it will probably take care of itself during compilation
                    SERIALIZER_VERIFYF( anno->module->macroContext,
                        "expected to see macro module '%s'", anno->module->name.c_str()
                    );
                }
            }
        } else {
            bool inThisModule = false;
            ser << inThisModule;
            if ( !inThisModule ) {
                string name;
                ser.serializeTemp(name);
                if ( isLogicAnnotation(name) ) {
                    LogicAnnotationOp op; ser.serialize_enum(op);
                    anno = newLogicAnnotation(op);
                    anno->serialize(ser);
                } else {
                    Module * mod = nullptr;
                    ser << mod;
                    SERIALIZER_VERIFYF(mod!=nullptr, "module of annotation '%s' is not found", name.c_str());
                    anno = mod->findAnnotation(name);
                    SERIALIZER_VERIFYF(anno!=nullptr, "annotation '%s' is not found", name.c_str());
                }
            } else {
                bool isDistinct = false;
                ser << isDistinct;
                if ( isDistinct ) {
                    string name;
                    ser.serializeTemp(name);
                    anno = ser.thisModule->findAnnotation(name);
                    SERIALIZER_VERIFYF(anno!=nullptr && anno->rtti_isDistinctTypeAnnotation(),
                        "distinct type '%s' is not found in module '%s'", name.c_str(), ser.thisModule->name.c_str());
                }
            }
        }
    }

    AstSerializer & AstSerializer::operator << ( AnnotationPtr & anno ) {
        dtag(HASH_TAG("AnnotationPtr"));
        DAS_SER_PROFILE(*this, "AnnotationPtr");
        serializeAnnotationPointer(*this, anno);
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( Structure::FieldDeclaration & field_declaration ) {
        field_declaration.serialize(*this);
        return *this;
    }


    enum LineShape : uint8_t { LineSameFile = 0x80, LineOneLine = 0x40, LineDeltaMask = 0x3f, LineDeltaBias = 31, LineDeltaEscape = 63 };

    // src/builtin/ARCHITECTURE.md#module-cache-record-bytes
    AstSerializer & AstSerializer::operator << ( LineInfo & at ) {
        dtag(HASH_TAG("LineInfo"));
        DAS_SER_PROFILE(*this, "LineInfo");
        if ( writing ) {
            int32_t lineDelta = int32_t(at.line) - int32_t(lineBaseLine);
            bool sameFile = at.fileInfo == lineBaseFile;
            bool oneLine = at.last_line == at.line;
            bool smallDelta = lineDelta >= -LineDeltaBias && lineDelta <= LineDeltaBias;
            uint8_t shape = uint8_t(smallDelta ? lineDelta + LineDeltaBias : LineDeltaEscape);
            if ( oneLine ) shape |= LineOneLine;
            if ( sameFile ) shape |= LineSameFile;
            write(&shape, 1);
            if ( !sameFile ) *this << at.fileInfo;
            uint8_t enc[20];
            size_t n = 0;
            if ( !smallDelta ) n += encodeAdaptiveSize32(enc + n, AstSerializer::zigzag32(lineDelta));
            if ( !oneLine ) n += encodeAdaptiveSize32(enc + n, at.last_line - at.line);
            n += encodeAdaptiveSize32(enc + n, at.column);
            n += encodeAdaptiveSize32(enc + n, AstSerializer::zigzag32(int32_t(at.last_column) - int32_t(at.column)));
            write(enc, n);
        } else {
            uint8_t shape = 0;
            read(&shape, 1);
            if ( shape & LineSameFile ) {
                at.fileInfo = lineBaseFile;
            } else {
                *this << at.fileInfo;
            }
            int32_t lineDelta;
            if ( (shape & LineDeltaMask) == LineDeltaEscape ) {
                uint32_t z; serializeAdaptiveSize32(z);
                lineDelta = AstSerializer::unzigzag32(z);
            } else {
                lineDelta = int32_t(shape & LineDeltaMask) - LineDeltaBias;
            }
            at.line = uint32_t(int32_t(lineBaseLine) + lineDelta);
            if ( shape & LineOneLine ) {
                at.last_line = at.line;
            } else {
                uint32_t diff; serializeAdaptiveSize32(diff);
                at.last_line = at.line + diff;
            }
            serializeAdaptiveSize32(at.column);
            uint32_t span; serializeAdaptiveSize32(span);
            at.last_column = uint32_t(int32_t(at.column) + AstSerializer::unzigzag32(span));
        }
        lineBaseFile = at.fileInfo;
        lineBaseLine = at.line;
        return *this;
    }

    // src/builtin/ARCHITECTURE.md#module-cache-record-bytes
    AstSerializer & AstSerializer::operator << ( FileInfo * & info ) {
        dtag(HASH_TAG("FileInfo *"));
        DAS_SER_PROFILE(*this, "FileInfo*");
        if ( writing ) {
            if ( !info ) {
                uint32_t null = 0;
                serializeAdaptiveSize32(null);
                return *this;
            }
            if ( info == lastWriteFileInfo ) {
                serializeAdaptiveSize32(lastWriteFileInfoNumber);
                return *this;
            }
            auto & number = writeFileInfos[info];
            if ( number == 0 ) {
                DAS_SER_PROFILE(*this, "FileInfo.payload");
                number = uint32_t(writeFileInfos.size());
                serializeAdaptiveSize32(number);
                info->serialize(*this);
            } else {
                serializeAdaptiveSize32(number);
            }
            lastWriteFileInfo = info;
            lastWriteFileInfoNumber = number;
        } else {
            uint32_t number = 0;
            serializeAdaptiveSize32(number);
            if ( number == 0 ) {
                info = nullptr;
                return *this;
            }
            if ( number <= readFileInfos.size() ) {
                info = readFileInfos[number - 1];
                return *this;
            }
            SERIALIZER_VERIFYF(number == readFileInfos.size() + 1, "corrupt stream: file info #%u past the %u numbered so far",
                number, unsigned(readFileInfos.size()));
            uint8_t tag = 0; *this << tag;
            switch ( tag ) {
                case 0: info = new FileInfo; break;
                case 1: info = new TextFileInfo; break;
                default: SERIALIZER_VERIFYF(false, "Unreachable");
            }
            info->serialize(*this);
            if ( fileAccess && !info->name.empty() && !isCppBindingAt(LineInfo(info, 0, 0, 0, 0)) ) {
                if ( FileInfo * live = fileAccess->getFileInfo(info->name) ) {
                    info = live;
                }
            }
            readFileInfos.push_back(info);
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
            if ( fileAccess && info_ptr ) {
                if ( auto owned = fileAccess->letGoOfFileInfo(info_ptr->name) ) {
                    info = das::move(owned);
                    return *this;
                }
            }
            info.reset(info_ptr);
            doNotDelete.insert(info_ptr);
        }
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( StructurePtr & struct_ ) {
        DAS_SER_PROFILE(*this, "StructurePtr");
        auto id = getSerializeId(struct_);
        *this << id;
        if ( id.index == 0 ) {
            if ( !writing ) struct_ = nullptr;
            return *this;
        }
        if ( writing ) {
            if ( !isWritten(id) ) {
                markWritten(id);
                struct_->serialize(*this);
            }
        } else {
            if ( auto node = readNode<Structure>(id) ) {
                struct_ = node;
            } else {
                struct_ = new Structure();
                setReadNode(id, struct_);
                struct_->serialize(*this);
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
            if ( !isWritten(p) ) {
                markWritten(p);
                ptr->serialize(*this);
            }
        } else {
            SerializeNodeId p; *this << p;
            if ( auto node = readNode<FileAccess>(p) ) {
                ptr.orphan();
                FileAccessPtr t = node;
                ptr = t;
            } else {
                uint8_t tag = 0; *this << tag;
                switch ( tag ) {
                    case 0: ptr = make_smart<FileAccess>(); break;
                    case 1: ptr = make_smart<ModuleFileAccess>(); break;
                    default: SERIALIZER_VERIFYF(false, "Unreachable");
                }
                setReadNode(p, ptr.get());
                ptr->serialize(*this);
            }
        }
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( EnumerationPtr & enum_type ) {
        DAS_SER_PROFILE(*this, "EnumerationPtr");
        if ( writing ) {
            bool builtin = enum_type->module->builtIn && !enum_type->module->promoted;
            *this << builtin;
            if ( builtin ) {
                Module * module = enum_type->module;
                *this << module << enum_type->name;
            } else {
                auto id = getSerializeId(enum_type);
                *this << id;
                if ( !isWritten(id) ) {
                    markWritten(id);
                    enum_type->serialize(*this);
                }
            }
        } else {
            bool builtin = false;
            *this << builtin;
            if ( builtin ) {
                Module * pModule = nullptr;
                string name;
                *this << pModule;
                serializeTemp(name);
                SERIALIZER_VERIFYF(pModule, "expected to find the module of enumeration '%s'", name.c_str());
                enum_type = pModule->findEnum(name);
                SERIALIZER_VERIFYF(enum_type, "expected to find enumeration '%s'::'%s'", pModule->name.c_str(), name.c_str());
            } else {
                SerializeNodeId id;
                *this << id;
                SERIALIZER_VERIFYF(id.index != 0, "expected non-null enumeration id");
                if ( auto node = readNode<Enumeration>(id) ) {
                    enum_type = node;
                } else {
                    enum_type = new Enumeration();
                    setReadNode(id, enum_type);
                    enum_type->serialize(*this);
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
        DAS_SER_PROFILE(*this, "VariablePtr");
        auto id = getSerializeId(var);
        *this << id;
        if ( id.index == 0 ) {
            if ( !writing ) var = nullptr;
            return *this;
        }
        if ( writing ) {
            if ( !isWritten(id) ) {
                markWritten(id);
                var->serialize(*this);
            }
        } else {
            if ( auto node = readNode<Variable>(id) ) {
                var = node;
            } else {
                var = new Variable();
                setReadNode(id, var);
                var->serialize(*this);
            }
        }
        return *this;
    }

    // a read no prerequisite walk precedes meets a deferred C++ module here (src/ast/ARCHITECTURE.md#module-scan-manifest)
    static Module * requireBuiltinModule ( const string & name ) {
        auto m = Module::require(name);
        if ( !m ) {
            if ( auto loader = getDeferredModuleLoader(); loader && loader(name) ) m = Module::require(name);
        }
        return m;
    }

    // src/builtin/ARCHITECTURE.md#module-cache-record-bytes
    AstSerializer & AstSerializer::operator << ( Module * & module ) {
        DAS_SER_PROFILE(*this, "Module*");
        if ( writing ) {
            if ( !module ) {
                uint32_t null = 0;
                serializeAdaptiveSize32(null);
                return *this;
            }
            auto & number = writeModules[module];
            if ( number == 0 ) {
                number = uint32_t(writeModules.size());
                serializeAdaptiveSize32(number);
                *this << module->nameHash;
            } else {
                serializeAdaptiveSize32(number);
            }
        } else {
            uint32_t number = 0;
            serializeAdaptiveSize32(number);
            if ( number == 0 ) {
                module = nullptr;
                return *this;
            }
            if ( number <= readModules.size() ) {
                module = readModules[number - 1].first;
                SERIALIZER_VERIFYF(module || ignoreEmptyExternal, "expected to fetch module %llu from library",
                    (unsigned long long) readModules[number - 1].second);
                return *this;
            }
            SERIALIZER_VERIFYF(number == readModules.size() + 1, "corrupt stream: module #%u past the %u numbered so far",
                number, unsigned(readModules.size()));
            uint64_t nameHash = 0; *this << nameHash;
            module = moduleLibrary->findModuleByMangledNameHash(nameHash);
            SERIALIZER_VERIFYF(module || ignoreEmptyExternal, "expected to fetch module %llu from library", nameHash);
            readModules.push_back({module, nameHash});
        }
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( Function::AliasInfo & alias_info ) {
        alias_info.serialize(*this);
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( ReaderMacroPtr & ptr ) {
        dtag(HASH_TAG("ReaderMacroPtr"));
        DAS_SER_PROFILE(*this, "ReaderMacroPtr");
        if ( writing ) {
            SERIALIZER_VERIFYF(ptr, "did not expect to see null ReaderMacroPtr");
            SERIALIZER_VERIFYF(!(ptr->module == thisModule), "did not expect to find macro from the current module");
            Module * module = ptr->module;
            *this << module;
            *this << ptr->name;
        } else {
            Module * mod = nullptr;
            string name;
            *this << mod;
            serializeTemp(name);
            SERIALIZER_VERIFYF(mod!=nullptr, "module of reader macro '%s' not found", name.c_str());
            ptr = mod->findReaderMacro(name);
            SERIALIZER_VERIFYF(ptr, "Reader macro '%s' not found in the module '%s'",
                name.c_str(), mod->name.c_str()
            );
        }
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( ExprBlock * & block ) {
        dtag(HASH_TAG("ExprBlock*"));
        DAS_SER_PROFILE(*this, "ExprBlock*");
        auto id = getSerializeId(block);
        *this << id;
        if ( !writing ) {
            if ( id.index ) fillOrPatchLater(block, id);
            else block = nullptr;
        }
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( InferHistory & history ) {
        dtag(HASH_TAG("InferHistory"));
        DAS_SER_PROFILE(*this, "InferHistory");
        history.serialize(*this);
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( CaptureEntry & entry ) {
        DAS_SER_PROFILE(*this, "CaptureEntry");
        *this << entry.name;
        serialize_enum<CaptureMode>(entry.mode);
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( MakeFieldDeclPtr & ptr ) {
        dtag(HASH_TAG("MakeFieldDeclPtr"));
        DAS_SER_PROFILE(*this, "MakeFieldDecl");
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
        DAS_SER_PROFILE(*this, "MakeStruct");
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
        return serializeModule(module, /*already_exists*/false);
    }

    AstSerializer & AstSerializer::serializeModule ( Module & module, bool already_exists ) {
        thisModule = &module;
        if ( writing ) {
            module.serialize(*this, already_exists);
            return *this;
        }
        // reading: Module::serialize restores moduleFlags - builtIn among them - before the body
        // is read, so a record that throws partway would leave a module flagged builtIn that was
        // never linked into the environment's module list, where ~Module's unlink walk finds
        // nothing and asserts. Put the flag back before the exception reaches a caller that
        // disposes of the module; the callers that keep it re-establish builtIn via promoteToBuiltin
        const bool wasBuiltIn = module.builtIn;
        try {
            module.serialize(*this, already_exists);
        } catch ( ... ) {
            module.builtIn = wasBuiltIn;
            throw;
        }
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
        DAS_SER_PROFILE(ser, "TypeDecl");
#if DAS_SERIALIZE_PROFILE
        uint64_t profStart = ser.profPosition();
        struct ProfTypeEnd {
            AstSerializer & ser; const TypeDecl * type; uint64_t start;
            ~ProfTypeEnd () { if ( ser.writing ) ser.profType(type, ser.profPosition() - start); }
        } profTypeEnd { ser, this, profStart };
#endif
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
            case tFloat16:
            case tHalf2:
            case tHalf3:
            case tHalf4:
            case tHalf8:
            case tShort2:
            case tShort3:
            case tShort4:
            case tShort8:
            case tUShort2:
            case tUShort3:
            case tUShort4:
            case tUShort8:
            case tByte2:
            case tByte3:
            case tByte4:
            case tByte8:
            case tByte16:
            case tUByte2:
            case tUByte3:
            case tUByte4:
            case tUByte8:
            case tUByte16:
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
            case tStructure: {
                ser << alias;
                // A genuinely cross-module struct is referenced by name (serializePointer):
                // the plain smart-map operator would materialize a per-stream COPY on read,
                // breaking type identity when a parsed module infers against cached
                // signatures (mixed compiles after a partial cache invalidation) -- see
                // Structure::serialize (parent). A same-module OR module-less struct (e.g. a
                // local/anonymous type reached through a Variable's type) has no external
                // identity to write, so keep the inline smart-map path, which serializes its
                // content once and preserves identity within this stream. The chosen path is
                // stamped into the stream so read takes the matching branch.
                bool crossModule = ser.writing && structType && structType->module && structType->module != ser.thisModule;
                ser << crossModule;
                if ( crossModule ) ser.serializePointer(structType);
                else               ser << structType;
                DAS_VERIFYF_MULTI(!annotation, !!structType, !enumType, !firstType, !secondType,
                                argTypes.empty(), argNames.empty());
                break;
            }
            case tHandle:
                ser << alias << annotation;
                DAS_VERIFYF_MULTI(!!annotation, !structType, !enumType, !firstType, !secondType,
                                argTypes.empty(), argNames.empty());
                break;
            case tDistinct:
                ser << alias << annotation << firstType;
                DAS_VERIFYF_MULTI(!!annotation, !structType, !enumType, !secondType,
                                argTypes.empty(), argNames.empty());
                break;
            case tEnumeration:
            case tEnumeration8:
            case tEnumeration16:
            case tEnumeration64: {
                ser << alias;
                // cross-module by name, else inline smart-map — see tStructure above
                bool crossModule = ser.writing && enumType && enumType->module && enumType->module != ser.thisModule;
                ser << crossModule;
                if ( crossModule ) ser.serializePointer(enumType);
                else               ser << enumType;
                DAS_VERIFYF_MULTI(!annotation, !structType, !!enumType, !firstType, !secondType,
                                argTypes.empty(), argNames.empty());
                break;
            }
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
                DAS_VERIFYF_MULTI(!annotation, !structType, !enumType, !firstType, !secondType);
                break;
            default:
                SERIALIZER_VERIFYF(false,  "not expected to be here");
                break;
        }

        // unconditional: typeMacro/typeDecl payload, and the tag payload riding on an
        // autoinfer firstType (FIXED_ARRAY_REWORK.md, 1b)
        ser << typeMacroExpr;

        if ( ser.writing ) {
            uint32_t streamed = flags & ~aliasCacheFlagsMask();
            ser << streamed;
        } else {
            ser << flags;
        }
        ser << at << module;
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
        ser << annotation;
        {
            DAS_SER_PROFILE(ser, "AnnotationDecl.arguments");
            ser << arguments;
        }
        ser << at << flags;
    }

    void ptr_ref_count::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("ptr_ref_count"));
        // Do nothing
    }

    void Structure::FieldDeclaration::serialize ( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("FieldDeclaration"));
        DAS_SER_PROFILE(ser, "FieldDecl");
        ser << name << at;
        ser << type;
        ser.ignoreEmptyExternal = true;
        ser << init;
        ser.ignoreEmptyExternal = false;
        ser << annotation << offset << flags;
    }

    void Enumeration::EnumEntry::serialize( AstSerializer & ser ) {
        ser.dtag(HASH_TAG("EnumEntry"));
        DAS_SER_PROFILE(ser, "EnumEntry");
        ser << name << cppName << at << value;
    }

    void serializeAnnotationList ( AstSerializer & ser, AnnotationList & list ) {
        DAS_SER_PROFILE(ser, "AnnotationList");
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
        DAS_SER_PROFILE(ser, "Enumeration");
        ser.tag(HASH_TAG("Enumeration"));
        ser << name     << cppName  << at << list << module
            << external << baseType << isPrivate;
        serializeAnnotationList(ser, annotations);
    }

    void Structure::serialize ( AstSerializer & ser ) {
        DAS_SER_PROFILE(ser, "Structure");
        ser.tag(HASH_TAG("Structure"));
        ser << name;
        ser << at     << module;
        {
            DAS_SER_PROFILE(ser, "Structure.fields");
            ser << fields;
        }
        {
            DAS_SER_PROFILE(ser, "Structure.fieldLookup");
            ser << fieldLookup;
        }
        ser << aliases;
        // A cross-module parent may live in another module whose content is NOT in this
        // stream; serializePointer binds it by name to the real library structure, avoiding
        // a private COPY on read that would break pointer-identity checks (is_subclass_of
        // walks parent pointers). A same-module or module-less parent has no external
        // identity, so keep the inline smart-map path -- see the tStructure case in
        // TypeDecl::serialize.
        bool parentCross = ser.writing && parent && parent->module && parent->module != ser.thisModule;
        ser << parentCross;
        if ( parentCross ) ser.serializePointer(parent);
        else               ser << parent;
        ser << flags
            << ownSemanticHash;
        serializeAnnotationList(ser, annotations);
    }

    void Variable::serialize ( AstSerializer & ser ) {
        DAS_SER_PROFILE(ser, "Variable");
        ser.tag(HASH_TAG("Variable"));
        ser << name << aka << type << init << source << at << stackTop
            << extraLocalOffset << module
            << initStackSize << flags << access_flags << access_info << annotation;
    }

    void Function::AliasInfo::serialize ( AstSerializer & ser ) {
        DAS_SER_PROFILE(ser, "AliasInfo");
        ser.tag(HASH_TAG("AliasInfo"));
        // var is often a global owned by ANOTHER module (deriveAliasing pulls
        // useGlobalVariables transitively across modules). The owning module's content is
        // NOT in this stream, so the inline operator<<(VariablePtr&) would serialize a
        // foreign or half-constructed var->type and read garbage (0xCD-filled type pointer
        // -> AV at TypeDecl::serialize). Cross-module vars resolve by module+name, exactly
        // like serializeUseVariables and Structure::serialize (parent); same-module or
        // module-less vars stay inline (smart-map).
        bool crossModule = ser.writing && var && var->module && var->module != ser.thisModule;
        ser << crossModule;
        if ( crossModule ) ser.serializePointer(var);
        else               ser << var;
        ser.serializePointer(func);
        ser << viaPointer;
    }

    void InferHistory::serialize ( AstSerializer & ser ) {
        ser.tag(HASH_TAG("InferHistory"));
        ser << at;
        ser.serializePointer(func);
    }

// function

    // a function has a restorable identity only while its module still lists it
    static bool moduleListsFunction ( Function * fn ) {
        if ( !fn || !fn->module ) return false;
        auto hName = hash64z(fn->name.c_str());
        if ( auto it = fn->module->functionsByName.find(hName) ) {
            for ( auto & f : it->second ) if ( f==fn ) return true;
        }
        if ( auto it = fn->module->genericsByName.find(hName) ) {
            for ( auto & f : it->second ) if ( f==fn ) return true;
        }
        return false;
    }

    void Function::serialize ( AstSerializer & ser ) {
        DAS_SER_PROFILE(ser, "Function");
        ser.tag(HASH_TAG("Function"));
        ser << name;
        // Note: important fields are placed separately for easier debugging
        serializeAnnotationList(ser, annotations);
        {
            DAS_SER_PROFILE(ser, "Function.arguments");
            ser.ignoreEmptyExternal = true;
            ser << arguments;
            ser.ignoreEmptyExternal = false;
        }
        {
            DAS_SER_PROFILE(ser, "Function.result");
            ser << result;
        }
        {
            DAS_SER_PROFILE(ser, "Function.body");
            ser << body;
        }
        DAS_SER_PROFILE(ser, "Function.tail");
        // cross-module by name, else inline smart-map — see Structure::serialize (parent)
        bool classParentCross = ser.writing && classParent && classParent->module && classParent->module != ser.thisModule;
        ser << classParentCross;
        if ( classParentCross ) ser.serializePointer(classParent);
        else                    ser << classParent;
        // usually another module's generic, so it binds by name - see Structure::serialize
        // (parent). an origin its module no longer lists has no identity to write
        if ( ser.writing ) {
            Function * origin = moduleListsFunction(fromGeneric) ? fromGeneric : nullptr;
            ser.serializePointer(origin);
        } else {
            ser.serializePointer(fromGeneric);
        }
        ser << totalStackSize  << totalGenLabel;
        ser << at            << atDecl          << module;
        ser << hash          << aotHash;  // do not serialize inferStack
        {
            DAS_SER_PROFILE(ser, "Function.aliases");
            ser << resultAliases << argumentAliases << resultAliasesGlobals;
        }
        ser << flags         << moreFlags       << moreFlags2      << sideEffectFlags;
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
        ser << expr->baseType << expr->value << expr->foldedNonConst << expr->promotedFromInt << expr->inexactFloatPromotion << expr->isConstLiteral;
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
            ser.serializeTemp(mangledName);
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
            string mangledName; ser.serializeTemp(mangledName);
            ser.fieldRefs.emplace_back(&expr->fieldRef, module, das::move(mangledName), expr->name);
        }
    }

    void SerializeVisitor::serializeMakeArray ( ExprMakeArray * expr ) {
        serializeMakeLocal(expr);
        ser << expr->recordType << expr->values << expr->gen2 << expr->makeArrayOnHeap << expr->constValues;
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
        ser << expr->defaultValue << expr->no_promotion;
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
            ser.setReadNode(thisBlockId, expr);
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
                Module * module = expr->variable->module;
                ser << module;
            }
        } else {

            bool inThisModule = false; ser << inThisModule;
            if ( inThisModule ) {
                ser << expr->variable;
            } else {
                string varname;
                Module * mod = nullptr;
                ser.serializeTemp(varname);
                ser << mod;
                SERIALIZER_VERIFYF(mod, "expected to find the module of variable '%s'", varname.c_str());
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
        ser << expr->cloneFlags;
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
        // cross-module enum by name (serializePointer avoids a per-stream COPY that breaks
        // identity and leaves dangling pointers in deserialized const expressions); a
        // same-module or module-less enum keeps the inline smart-map path -- see the
        // tStructure/tEnumeration cases in TypeDecl::serialize.
        bool crossModule = ser.writing && expr->enumType && expr->enumType->module && expr->enumType->module != ser.thisModule;
        ser << crossModule;
        if ( crossModule ) ser.serializePointer(expr->enumType);
        else               ser << expr->enumType;
        ser << expr->text;
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
        ser << expr->with << expr->body << expr->moduleName << expr->moduleUnsafeByProject;
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
        ser << expr->subexpr << expr->typeexpr << expr->no_promotion;
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

    // src/builtin/ARCHITECTURE.md#module-cache-record-bytes
    AstSerializer & AstSerializer::operator << ( ExpressionPtr & expr ) {
        dtag(HASH_TAG("ExpressionPtr"));
        DAS_SER_PROFILE(*this, "ExpressionPtr");
        SerializeVisitor sv(*this);
        if ( writing ) {
            if ( !expr ) {
                uint32_t null = 0;
                serializeAdaptiveSize32(null);
                return *this;
            }
            uint32_t rtti = hash_tag(expr->__rtti);
            DAS_ASSERT(rtti);
            auto & number = writeExprClasses[rtti];
            if ( number == 0 ) {
                number = uint32_t(writeExprClasses.size());
                serializeAdaptiveSize32(number);
                serialize(rtti);
            } else {
                serializeAdaptiveSize32(number);
            }
            DAS_SER_PROFILE(*this, expr->__rtti);
            expr->dispatch(sv);
        } else {
            uint32_t number = 0;
            serializeAdaptiveSize32(number);
            if ( number == 0 ) {
                expr = nullptr;
                return *this;
            }
            Annotation * annotation;
            if ( number <= readExprClasses.size() ) {
                annotation = readExprClasses[number - 1];
            } else {
                SERIALIZER_VERIFYF(number == readExprClasses.size() + 1, "corrupt stream: expression class #%u past the %u numbered so far",
                    number, unsigned(readExprClasses.size()));
                uint32_t rtti = 0; serialize(rtti);
                auto itA = rttiHash2Annotation.find(rtti);
                SERIALIZER_VERIFYF(itA != rttiHash2Annotation.end(), "annotation '%u' is not found", rtti);
                annotation = itA->second;
                readExprClasses.push_back(annotation);
            }
            expr = (Expression *) static_cast<TypeAnnotation*>(annotation)->factory();
            DAS_SER_PROFILE(*this, annotation->name.c_str());
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
        DAS_SER_PROFILE(*this, "CallMacro*");
        if ( writing ) {
            SERIALIZER_VERIFYF ( ptr, "did not expect to see a nullptr CallMacro *" );
            SERIALIZER_VERIFYF ( !(ptr->module == thisModule), "did not expect to find macro from the current module" );
            Module * module = ptr->module;
            *this << module;
            *this << ptr->name;
        } else {
            Module * mod = nullptr;
            string name;
            *this << mod;
            serializeTemp(name);
            SERIALIZER_VERIFYF(mod!=nullptr, "module of call macro '%s' not found", name.c_str());
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
                program->policies.jit_enabled = ser.readJitEnabled;     // the two things the cold path's program also carries
                program->options = ser.readOptions;
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
        DAS_SER_PROFILE(ser, "UseFunctions");
        ser.tag(HASH_TAG("serializeUseFunctions"));
        if ( ser.writing ) {
            ser << f->name;
            uint64_t sz = f->useFunctions.size();
            ser << sz;
            for ( auto & usedFun : f->useFunctions ) {
                // cross-module refs by module+mangled-name-hash — see serializeUseVariables(FunctionPtr)
                bool builtin = usedFun->module->builtIn || usedFun->module != f->module;
                ser << builtin;
                if ( builtin ) {
                    Module * module = usedFun->module;
                    uint64_t mnh = usedFun->getMangledNameHash();
                    ser << module << mnh;
                } else {
                    auto fid = ser.getSerializeId(usedFun);
                    if ( !ser.isWritten(fid) )
                        LOG(LogLevel::warning) << "das: serialize: [write] unregistered id for function '" << usedFun->name
                            << "' of module '" << usedFun->module->name << "' in use-set of fn '" << f->name << "' - will be unresolvable on read\n";
                    ser << fid;
                }
            }
        } else {
            string fname; ser.serializeTemp(fname);
            SERIALIZER_VERIFYF(fname == f->name, "expected to serialize in the same order: %s != %s", fname.c_str(), f->name.c_str());
            uint64_t size = 0; ser << size;
            f->useFunctions.reserve(size);
            for ( uint64_t i = 0; i < size; i++ ) {
                bool builtin = false;
                ser << builtin;
                if ( builtin ) {
                    Module * pModule = nullptr;
                    uint64_t mnh = 0;
                    ser << pModule << mnh;
                    SERIALIZER_VERIFYF(pModule, "expected to find the module (useFunctions[%llu/%llu] of function '%s')",
                        (unsigned long long) i, (unsigned long long) size, f->name.c_str());
                    auto fun = pModule->findFunctionByMangledNameHash(mnh);
                    SERIALIZER_VERIFYF(fun, "expected to find function (mnh %llu in module '%s', useFunctions[%llu/%llu] of function '%s')",
                        (unsigned long long) mnh, pModule->name.c_str(), (unsigned long long) i, (unsigned long long) size, f->name.c_str());
                    f->useFunctions.emplace(fun);
                } else {
                    SerializeNodeId fid; ser << fid;
                    auto fun = ser.readNode<Function>(fid);
                    SERIALIZER_VERIFYF(fun, "expected to find function (id #%u, useFunctions[%llu/%llu] of function '%s')",
                        fid.index, (unsigned long long) i, (unsigned long long) size, f->name.c_str());
                    f->useFunctions.emplace(fun);
                }
            }
        }
    }

    void serializeUseFunctions ( AstSerializer & ser, const VariablePtr & f ) {
        DAS_SER_PROFILE(ser, "UseFunctions");
        ser.tag(HASH_TAG("serializeUseFunctions"));
        if ( ser.writing ) {
            ser << f->name;
            uint64_t sz = f->useFunctions.size();
            ser << sz;
            for ( auto & usedFun : f->useFunctions ) {
                // cross-module refs by module+mangled-name-hash — see serializeUseVariables(FunctionPtr)
                bool builtin = usedFun->module->builtIn || usedFun->module != f->module;
                ser << builtin;
                if ( builtin ) {
                    Module * module = usedFun->module;
                    uint64_t mnh = usedFun->getMangledNameHash();
                    ser << module << mnh;
                } else {
                    auto fid = ser.getSerializeId(usedFun);
                    if ( !ser.isWritten(fid) )
                        LOG(LogLevel::warning) << "das: serialize: [write] unregistered id for function '" << usedFun->name
                            << "' of module '" << usedFun->module->name << "' in use-set of global '" << f->name << "' - will be unresolvable on read\n";
                    ser << fid;
                }
            }
        } else {
            string name; ser.serializeTemp(name);
            SERIALIZER_VERIFYF(name == f->name, "expected to serialize in the same order: %s != %s", name.c_str(), f->name.c_str());
            uint64_t size = 0; ser << size;
            f->useFunctions.reserve(size);
            for ( uint64_t i = 0; i < size; i++ ) {
                bool builtin = false;
                ser << builtin;
                if ( builtin ) {
                    Module * pModule = nullptr;
                    uint64_t mnh = 0;
                    ser << pModule << mnh;
                    SERIALIZER_VERIFYF(pModule, "expected to find the module (useFunctions[%llu/%llu] of global '%s')",
                        (unsigned long long) i, (unsigned long long) size, f->name.c_str());
                    auto fun = pModule->findFunctionByMangledNameHash(mnh);
                    SERIALIZER_VERIFYF(fun, "expected to find function (mnh %llu in module '%s', useFunctions[%llu/%llu] of global '%s')",
                        (unsigned long long) mnh, pModule->name.c_str(), (unsigned long long) i, (unsigned long long) size, f->name.c_str());
                    f->useFunctions.emplace(fun);
                } else {
                    SerializeNodeId fid; ser << fid;
                    auto fun = ser.readNode<Function>(fid);
                    SERIALIZER_VERIFYF(fun, "expected to find function (id #%u, useFunctions[%llu/%llu] of global '%s')",
                        fid.index, (unsigned long long) i, (unsigned long long) size, f->name.c_str());
                    f->useFunctions.emplace(fun);
                }
            }
        }
    }

    void serializeUseVariables ( AstSerializer & ser, const FunctionPtr & f ) {
        DAS_SER_PROFILE(ser, "UseVariables");
        ser.tag(HASH_TAG("serializeUseVariables"));
        if ( ser.writing ) {
            ser << f->name;
            uint64_t sz = f->useGlobalVariables.size();
            ser << sz;
            for ( auto & use : f->useGlobalVariables ) {
                // cross-module refs must resolve by module+name: the owning module's content
                // is NOT re-serialized in this program's stream (isNew=false skips it), so a
                // SerializeNodeId written here would be unresolvable on read. The id branch is
                // only valid for the function's own module, whose content is in this stream.
                bool builtin = use->module->builtIn || use->module != f->module;
                ser << builtin;
                if ( builtin ) {
                    Module * module = use->module;
                    ser << module << use->name;
                } else {
                    auto vid = ser.getSerializeId(use);
                    if ( !ser.isWritten(vid) )
                        LOG(LogLevel::warning) << "das: serialize: [write] unregistered id for variable '" << use->name
                            << "' of module '" << use->module->name << "' in use-set of fn '" << f->name << "' - will be unresolvable on read\n";
                    ser << vid;
                }
            }
        } else {
            string name; ser.serializeTemp(name);
            SERIALIZER_VERIFYF(name == f->name, "expected to serialize in the same order: %s %s", name.c_str(), f->name.c_str());
            uint64_t size = 0; ser << size;
            f->useGlobalVariables.reserve(size);
            for ( uint64_t i = 0; i < size; i++ ) {
                bool builtin = false;
                ser << builtin;
                if ( builtin ) {
                    Module * pModule = nullptr;
                    string varname;
                    ser << pModule;
                    ser.serializeTemp(varname);
                    SERIALIZER_VERIFYF(pModule, "expected to find the module (useGlobalVariables[%llu/%llu] of function '%s')",
                        (unsigned long long) i, (unsigned long long) size, f->name.c_str());
                    auto var = pModule->findVariable(varname);
                    SERIALIZER_VERIFYF(var, "expected to find variable '%s::%s' (useGlobalVariables[%llu/%llu] of function '%s')",
                        pModule->name.c_str(), varname.c_str(), (unsigned long long) i, (unsigned long long) size, f->name.c_str());
                    f->useGlobalVariables.emplace(var);
                } else {
                    SerializeNodeId vid; ser << vid;
                    auto var = ser.readNode<Variable>(vid);
                    SERIALIZER_VERIFYF(var, "expected to find variable (id #%u, useGlobalVariables[%llu/%llu] of function '%s')",
                        vid.index, (unsigned long long) i, (unsigned long long) size, f->name.c_str());
                    f->useGlobalVariables.emplace(var);
                }
            }
        }
    }

    void serializeUseVariables ( AstSerializer & ser, const VariablePtr & f ) {
        DAS_SER_PROFILE(ser, "UseVariables");
        ser.tag(HASH_TAG("serializeUseVariables"));
        if ( ser.writing ) {
            ser << f->name;
            uint64_t sz = f->useGlobalVariables.size();
            ser << sz;
            for ( auto & use : f->useGlobalVariables ) {
                // cross-module refs by module+name — see serializeUseVariables(FunctionPtr)
                bool builtin = use->module->builtIn || use->module != f->module;
                ser << builtin;
                if ( builtin ) {
                    Module * module = use->module;
                    ser << module << use->name;
                } else {
                    auto vid = ser.getSerializeId(use);
                    if ( !ser.isWritten(vid) )
                        LOG(LogLevel::warning) << "das: serialize: [write] unregistered id for variable '" << use->name
                            << "' of module '" << use->module->name << "' in use-set of global '" << f->name << "' - will be unresolvable on read\n";
                    ser << vid;
                }
            }
        } else {
            string name; ser.serializeTemp(name);
            SERIALIZER_VERIFYF(name == f->name, "expected to serialize in the same order: %s != %s", name.c_str(), f->name.c_str());
            uint64_t size = 0; ser << size;
            f->useGlobalVariables.reserve(size);
            for ( uint64_t i = 0; i < size; i++ ) {
                bool builtin = false;
                ser << builtin;
                if ( builtin ) {
                    Module * pModule = nullptr;
                    string varname;
                    ser << pModule;
                    ser.serializeTemp(varname);
                    SERIALIZER_VERIFYF(pModule, "expected to find the module (useGlobalVariables[%llu/%llu] of global '%s')",
                        (unsigned long long) i, (unsigned long long) size, f->name.c_str());
                    auto var = pModule->findVariable(varname);
                    SERIALIZER_VERIFYF(var, "expected to find variable '%s::%s' (useGlobalVariables[%llu/%llu] of global '%s')",
                        pModule->name.c_str(), varname.c_str(), (unsigned long long) i, (unsigned long long) size, f->name.c_str());
                    f->useGlobalVariables.emplace(var);
                } else {
                    SerializeNodeId vid; ser << vid;
                    auto var = ser.readNode<Variable>(vid);
                    SERIALIZER_VERIFYF(var, "expected to find variable (id #%u, useGlobalVariables[%llu/%llu] of global '%s')",
                        vid.index, (unsigned long long) i, (unsigned long long) size, f->name.c_str());
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
                {
                    DAS_SER_PROFILE(ser, "Functions.mangledName");
                    ser.serializeTemp(name);
                }
                ser << g;
            });
        } else {
            uint64_t size = 0; ser << size;
            for ( uint64_t i = 0; i < size; i++ ) {
                string name; ser.serializeTemp(name);
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
            ser.verifyLength(size);
            functions.resize(size);
            for ( auto & f : functions ) ser.serializePointer(f);
        }
    }

    void serializeFunctionsByName ( AstSerializer & ser, fragile_hash<vector<Function *>> & functionsByName ) {
        DAS_SER_PROFILE(ser, "FunctionsByName");
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
            // capacity is deliberately NOT length-checked: an over-reserved table's bucket
            // count can legitimately exceed the bytes left; size is a true element count
            ser.verifyLength(size);
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
        DAS_SER_PROFILE(ser, "Module");
        ser.tag(HASH_TAG("Module"));
        // builtIn / promoted say whether this module is linked into daScriptEnvironment::modules,
        // which is a fact about the running process, not about the stream. Restoring them hands
        // back a module that claims to be a promoted builtin without ever having been linked, and
        // the next promoteToBuiltin trips its own assert on it
        const bool selfBuiltIn = builtIn;
        const bool selfPromoted = promoted;
        if ( ser.writing ) {
            builtIn = false;
            promoted = false;
        }
        ser << name << nameHash << moduleFlags << inlineTempIndex;
        builtIn = selfBuiltIn;
        promoted = selfPromoted;
        ser << annotationData << requireModule;
        // das-declared distinct types round-trip with the module (C++-module annotations
        // re-register on load, parser-created ones don't). they stream BEFORE aliasTypes and
        // enumerations - any TypeDecl deserialized below resolves tDistinct annotation
        // pointers via findAnnotation, so the entities must exist first
        if ( ser.writing ) {
            uint64_t dtSize = 0;
            for ( auto & [key, ann] : handleTypes ) {
                (void) key;
                if ( ann->rtti_isDistinctTypeAnnotation() ) dtSize ++;
            }
            ser << dtSize;
            for ( auto & [key, ann] : handleTypes ) {
                (void) key;
                if ( !ann->rtti_isDistinctTypeAnnotation() ) continue;
                auto dann = static_cast<DistinctTypeAnnotation *>(ann);
                ser << dann->name << dann->cppName << dann->at << dann->isPrivate << dann->underlyingType;
            }
        } else {
            uint64_t dtSize = 0;
            ser << dtSize;
            for ( uint64_t i=0; i<dtSize; ++i ) {
                string dname, dcppName;
                LineInfo dat;
                bool dpriv = false;
                TypeDeclPtr dunder = nullptr;
                ser.serializeTemp(dname);
                ser.serializeTemp(dcppName);
                ser << dat << dpriv << dunder;
                auto dann = new DistinctTypeAnnotation(dname, dunder, dcppName);
                dann->at = dat;
                dann->isPrivate = dpriv;
                addAnnotation(dann, true);
            }
        }
        {
            DAS_SER_PROFILE(ser, "Module.aliasTypes");
            ser << aliasTypes;
        }
        {
            DAS_SER_PROFILE(ser, "Module.enumerations");
            ser << enumerations;
        }
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
        {
            DAS_SER_PROFILE(ser, "Module.keywords");
            ser << keywords;
        }
        {
            DAS_SER_PROFILE(ser, "Module.typeFunctions");
            ser << typeFunctions;
        }
        {
            DAS_SER_PROFILE(ser, "Module.globals");
            serializeGlobals(ser, globals); // globals require insertion in the same order
        }
        {
            DAS_SER_PROFILE(ser, "Module.structures");
            serializeStructures(ser, structures);
        }
        {
            DAS_SER_PROFILE(ser, "Module.functions");
            serializeFunctions(ser, functions);
        }
        if ( ser.failed ) return;
        {
            DAS_SER_PROFILE(ser, "Module.generics");
            serializeFunctions(ser, generics);
        }
        if ( ser.failed ) return;
        {
            DAS_SER_PROFILE(ser, "Module.functionsByName");
            serializeFunctionsByName(ser, functionsByName);
        }
        {
            DAS_SER_PROFILE(ser, "Module.genericsByName");
            serializeFunctionsByName(ser, genericsByName);
        }
        ser << ownFileInfo;     //<< promotedAccess;

        DAS_SER_PROFILE(ser, "Module.useSets");
        functions.foreach ([&] ( FunctionPtr f ) {
            if ( ser.writing ) {
                ser << f->name;
            } else {
                string fname; ser.serializeTemp(fname);
                SERIALIZER_VERIFYF(fname == f->name, "expected to walk in the same order: %s != %s", fname.c_str(), f->name.c_str());
            }
            serializeUseVariables(ser, f);
            serializeUseFunctions(ser, f);
        });

        generics.foreach ([&] ( FunctionPtr f ) {
            if ( ser.writing ) {
                ser << f->name;
            } else {
                string fname; ser.serializeTemp(fname);
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

        auto finalize0 = ref_time_ticks();
        finalizeModule(ser, *ser.moduleLibrary, this, already_exists);
        if ( !ser.writing ) ser.totFinalizeTime += get_time_usec(finalize0);
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

        vector<Module*> getDependecyOrderedEndingWith(Module * last) {
            for ( auto mod : input ) {
                if ( mod != last ) visit(mod);
            }
            visit(last);
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


    #define DAS_MODULE_CACHE_POLICY_FIELDS(X) \
        X(aot) X(aot_module) X(aot_macros) X(tune_frozen) X(completion) X(building_documentation) \
        X(export_all) X(serialize_main_module) X(keep_alive) X(very_safe_context) X(max_infer_passes) \
        X(max_call_depth) X(verify_infer_types) X(stack) X(intern_strings) X(persistent_heap) \
        X(multiple_contexts) X(heap_size_hint) X(string_heap_size_hint) X(solid_context) \
        X(macro_context_persistent_heap) X(macro_context_collect) X(max_static_variables_size) \
        X(max_heap_allocated) X(max_string_heap_allocated) X(max_unreserved_size) X(track_allocations) \
        X(rtti) X(unsafe_table_lookup) X(relaxed_pointer_const) X(version_2_syntax) X(gen2_make_syntax) \
        X(relaxed_assign) X(no_unsafe) X(local_ref_is_unsafe) X(with_module_is_unsafe) \
        X(no_global_variables) X(no_global_variables_at_all) X(no_global_heap) X(only_fast_aot) \
        X(aot_order_side_effects) X(no_unused_function_arguments) X(no_unused_block_arguments) \
        X(allow_block_variable_shadowing) X(allow_local_variable_shadowing) X(allow_shared_lambda) \
        X(ignore_shared_modules) X(default_module_public) X(no_deprecated) X(no_aliasing) \
        X(strict_smart_pointers) X(no_init) X(strict_unsafe_delete) X(no_members_functions_in_struct) \
        X(no_local_class_members) X(no_unsafe_uninitialized_structures) X(default_init_containers) \
        X(strict_properties) X(no_writing_to_nameless) X(no_optimizations) X(no_fold_unsafe) X(fast_math) X(disable_dse) \
        X(disable_cse) X(disable_temp_string_reclaim) X(disable_inline) X(disable_auto_inline) \
        X(auto_inline_functions) X(auto_inline_cost) X(disable_run) X(no_infer_time_folding) \
        X(fail_on_no_aot) X(fail_on_lack_of_aot_export) X(no_fast_call) X(max_fast_call_depth) X(fusion) X(scoped_stack_allocator) \
        X(force_inscope_pod) X(log_inscope_pod) X(debugger) X(profiler) X(jit_enabled) \
        X(jit_jit_all_functions) X(jit_debug_info) X(jit_opt_level) X(jit_size_level) X(jit_dll_mode) \
        X(jit_output_path) X(jit_path_to_shared_lib) X(jit_path_to_linker) X(threadlock_context) \
        X(lint_check) X(no_lint) X(abi_stamp) X(paranoid_validation) X(cross_platform) X(aot_result) \
        X(no_init_check) X(always_report_candidates_threshold) X(jit_exe_mode) X(jit_emit_object) \
        X(jit_emit_prologue) X(dll_search_paths) X(aot_lib) X(standalone_context) X(gc_infer_collect) \
        X(gc_infer_collect_nodes) X(gc_infer_collect_pct) X(report_invisible_functions) \
        X(report_private_functions) X(log_compile_time) X(log_total_compile_time) \
        X(log_module_compile_time) X(log_optimization) X(log_optimization_passes) X(force_escape_free) \
        X(force_allocate_on_stack) X(force_partial_escape_free) X(log_escape_analysis) X(log_gc_time) \
        X(debug_infer_flag) X(temp_table_lint_warning) X(module_cache)

    static bool cachedPoliciesMatch ( const CodeOfPolicies & a, const CodeOfPolicies & b ) {
    #define DAS_POLICY_FIELD_SAME(f) if ( !(a.f == b.f) ) return false;
        DAS_MODULE_CACHE_POLICY_FIELDS(DAS_POLICY_FIELD_SAME)
    #undef DAS_POLICY_FIELD_SAME
        return true;
    }

    AstSerializer & AstSerializer::operator << ( CodeOfPolicies & value ) {
    #define DAS_POLICY_FIELD_STREAM(f) serializeTemp(value.f);
        DAS_MODULE_CACHE_POLICY_FIELDS(DAS_POLICY_FIELD_STREAM)
    #undef DAS_POLICY_FIELD_STREAM
        return *this;
    }

    AstSerializer & AstSerializer::operator << ( tuple<Module *, string, string, bool, LineInfo> & value ) {
        *this << get<0>(value) << get<1>(value) << get<2>(value) << get<3>(value) << get<4>(value);
        return *this;
    }

    // Used in eden
    // if an early return (bad hash, missing module, throw) leaves the thread-active
    // root pointing at a scope-local gc_root, repoint it to the permanent thread root
    // before the local dies
    struct ActiveRootGuard {
        gc_root * scopeRoot;
        ~ActiveRootGuard() {
            auto & active = gc_root::gc_get_active_root();
            if ( active == scopeRoot ) active = &gc_root::gc_get_thread_root();
        }
    };

    // embedders are built without exception handling, so nothing may escape this
    // rail: contain dasException here (truncated/corrupt cache streams throw from
    // the stream readers) and report failure through the failed/failToCompile flags
    // src/builtin/ARCHITECTURE.md#module-cache-record-bytes
    void AstSerializer::serializeProgram ( ProgramPtr program, ModuleGroup & libGroup ) noexcept {
        try {
            serializeProgramImpl(program, libGroup);
        } catch ( const dasException & r ) {
            if ( !quietCache ) LOG(LogLevel::warning) << "das: serialize: program " << (writing ? "write" : "read") << " failed: " << r.what() << "\n";
            failed = true;
            seenNewModule = true;
            if ( program ) program->failToCompile = true;
            // scope guards in the impl repointed the active gc root during unwind;
            // the caller replaces the (now gutted) program via replaceProgramKeepGcRootValid
        } catch ( ... ) {
            // this method is noexcept for embedders built without EH; a non-das
            // exception (e.g. std::bad_alloc deep in deserialization) escaping here
            // would std::terminate. Contain it the same way and report failure.
            if ( !quietCache ) LOG(LogLevel::warning) << "das: serialize: program " << (writing ? "write" : "read") << " failed: unknown exception\n";
            failed = true;
            seenNewModule = true;
            if ( program ) program->failToCompile = true;
        }
        // per-record scratch must never survive into the next record: a failed or
        // early-returned record leaves refs whose targets die with it (throwaway root,
        // `delete deser`), and its node table would make the next record's patch()
        // resolve those refs INTO FREED MEMORY. The success path cleared the table
        // already and patch() cleared the refs - re-clearing is free.
        fieldRefs.clear();
        clearNodeIds();
        if ( writing ) buffer->flush();
    }

    struct SerializerRecordScope {
        AstSerializer & ser;
        SerializerRecordScope ( AstSerializer & s ) : ser(s) { ser.recordDepth ++; }
        ~SerializerRecordScope () { ser.recordDepth --; }
    };

    void AstSerializer::serializeProgramImpl ( ProgramPtr program, ModuleGroup & libGroup ) {
        auto & ser = *this;
        SerializerRecordScope record(ser);
        // version gate — the module-cache path (trySerializeProgramModule) checks only the
        // file name and content stamp before this; a cache written by a different serializer version must
        // fail cleanly here (the caller falls back to a full parse on ser.failed), not misparse
        // every field after the first layout difference
        uint32_t version = getVersion();
        ser << version;
        if ( !ser.writing && version != getVersion() ) {
            if ( !ser.quietCache ) LOG(LogLevel::warning) << "das: deserialize: module cache version " << version
                << " does not match serializer version " << getVersion() << "\n";
            ser.failed = true;
            return;
        }
        ser.builtinHashDrift = false;   // per-record flavor bit, read by the resume path
        ser.clearNodeIds();             // numbering restarts with every program, on both sides

        DAS_SER_PROFILE(ser, "Program");
#if DAS_SERIALIZE_PROFILE
        ser.profBegin("Program.header");
#endif
        ser << program->thisNamespace << program->thisModuleName;

        ser << program->totalFunctions      << program->totalVariables << program->newLambdaIndex;
        ser << program->globalInitStackSize << program->globalStringHeapSize;
        ser << program->flags;

        if ( writing ) {
            ser << program->options << program->policies;
        } else {
            ser << program->options;
            CodeOfPolicies stored = program->policies;
            ser << stored;
            if ( !cachedPoliciesMatch(stored, program->policies) ) {
                ser.policyMismatch = true;
                ser.failed = true;
#if DAS_SERIALIZE_PROFILE
                ser.profEnd();
#endif
                return;
            }
            ser.readJitEnabled = program->policies.jit_enabled;
            ser.readOptions = program->options;
        }
#if DAS_SERIALIZE_PROFILE
        ser.profEnd();
#endif

        if ( writing ) {
            moduleLibrary = &program->library;  // Module::serialize binds *moduleLibrary (finalizeModule)
            vector<Module *> modules;
            uint64_t thisAt = 0;    // program module's position in the LIVE library
            if ( !program->isDependency ) {
                // MAIN record: this list becomes the deserialized program's library
                // insertion order, which drives foreach_in_order - module [init] order at
                // runtime and the jit's partition/chain order. Write the LIVE library order
                // (thisModule moved last: forward refs from its body resolve only after its
                // dependencies' records are read), NOT a re-derived TopSort - any other
                // valid topological order still reorders [init]s and re-keys every jit obj
                // partition. The live order also already carries the ambient extra modules
                // (-jit's just_in_time chain, the profiler) that a reachability walk drops -
                // their simulate macros (the jit hook) ride the library walk at simulate.
                // thisAt records where the program module REALLY sits, so the reader can
                // put it back: raw library order feeds func->index and with it the
                // cross-module [init] sequence, which must not differ cached vs fresh.
                auto & lib = program->library.getModules();
                for ( size_t i = 0; i != lib.size(); ++i ) {
                    if ( lib[i] == program->thisModule.get() ) thisAt = uint64_t(i);
                    else modules.push_back(lib[i]);
                }
                modules.push_back(program->thisModule.get());
            } else {
                // dependency record: the reachable closure only. The full library here would
                // make the FIRST record mentioning a later module embed its whole payload
                // early, dragging not-yet-valid state (lazy builtin hashes) into the record.
                // Dependency programs never run a context, so their record position (last)
                // doubles as the restore position - no reorder on read.
                TopSort ts(program->library.getModules());
                modules = ts.getDependecyOrdered(program->thisModule.get());
                thisAt = modules.empty() ? 0 : uint64_t(modules.size() - 1);
            }

            uint64_t size = modules.size(); *this << size;
            *this << thisAt;

            for ( auto & m : modules ) {
                DAS_SER_PROFILE(ser, "Program.moduleRef");
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
            uint64_t thisAt = 0; ser << thisAt;

            // parseDaScript runs with the placeholder thisModule's gc root as the
            // thread-active root; library.reset() below deletes that module (and its
            // root) — repoint the active root as we go, or every node deserialized
            // after this line gc_links through a dangling pointer into freed memory
            auto & activeRoot = gc_root::gc_get_active_root();
            const bool activeWasThisModule = program->thisModule && activeRoot == program->thisModule->module_gc_root.get();
            // throwaway already-exists reads park nodes here — they may be referenced
            // through the patch maps until all modules are read, then sweep with scope
            gc_root throwaway_root;
            ActiveRootGuard throwaway_guard { &throwaway_root };
            if ( activeWasThisModule ) activeRoot = &throwaway_root;

            program->library.reset();
            program->thisModule.release();
            moduleLibrary = &program->library;

            for ( uint64_t i = 0; i < size; i++ ) {
                DAS_SER_PROFILE(ser, "Program.moduleRef");
                bool builtin = false, promoted = false;
                ser << builtin << promoted;
                string name; ser.serializeTemp(name);

                if ( builtin && !promoted ) {
                    auto m = requireBuiltinModule(name);
                    // a corrupted record can hand this arm a garbage name - require()
                    // answers null, and the deref was a SIGSEGV (recoverable throw now;
                    // the resume reparses the record in place)
                    SERIALIZER_VERIFYF(m != nullptr, "builtin module '%s' not found", name.c_str());
                    uint64_t savedHash = 0, moduleHash = m->cumulativeHash;
                    *this << savedHash;

                    if ( moduleHash != savedHash ) {
                        if ( !ser.quietCache ) {
                            LOG(LogLevel::warning) << "das: serialize: cumulative hash for module '" << m->name
                                                   << "' differs" << " (" << moduleHash << " vs " << savedHash << ") ";
                        }
                        ser.builtinHashDrift = true;    // per-process-deterministic drift, not damage
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
                    if ( !ser.quietCache ) LOG(LogLevel::warning) << "das: serialize: module '" << name << "' not found";
                    program->failToCompile = true;
                    return;
                }

                Module* deser = nullptr;
                try {
                    deser = new ModuleDas();     // the parser's class: aotRequire answers cpp, a plain Module answers no_aot
                    deser->setModuleName(name);
                    if ( existing ) {
                        program->library.addModule(existing);
                        // throwaway read into a temp module — keep nodes off its root
                        // (they may be referenced through the patch maps past `delete deser`)
                        ser.serializeModule(*deser, /*already_exists*/true);
                        deser->builtIn = false; // suppress dtor unlink assert
                        delete deser;
                        continue;
                    }
                    program->library.addModule(deser);
                    if ( activeWasThisModule ) activeRoot = deser->module_gc_root.get();
                    ser << *deser;
                    if ( activeWasThisModule ) activeRoot = &throwaway_root;
                } catch ( const dasException & r ) {
                    if ( activeWasThisModule ) activeRoot = &gc_root::gc_get_thread_root();
                    delete deser;
                    if ( !ser.quietCache ) LOG(LogLevel::warning) << "das: serialize: reading module '" << name << "' (" << i << "/" << size
                                           << (existing ? ", already exists" : ", new") << "): " << r.what() << "\n";
                    program->failToCompile = true;
                    return;
                } catch ( ... ) {
                    // a non-das exception (e.g. std::bad_alloc) escaping ser << *deser would
                    // otherwise leave the thread active root pointing at deser's root, which
                    // dies with the replaced program (the ActiveRootGuard only tracks
                    // throwaway_root). Restore the thread root and rethrow to the noexcept
                    // serializeProgram boundary, which reports failure.
                    if ( activeWasThisModule ) activeRoot = &gc_root::gc_get_thread_root();
                    delete deser;
                    throw;
                }
            }

            if ( program->library.getModules().empty() ) {
                if ( !ser.quietCache ) LOG(LogLevel::warning) << "das: serialize: program '" << program->thisModuleName << "' stream has no modules\n";
                program->failToCompile = true;
                return;
            }

            program->thisModule.reset(program->library.getModules().back());
            // the deserialized module is the program's module now — new nodes and the
            // ModuleGcFinalize collect belong on its root
            if ( activeWasThisModule ) activeRoot = program->thisModule->module_gc_root.get();
            // the record serialized the program module LAST (its refs resolve only after
            // its dependencies), but the LIVE library had it at thisAt - put it back, or
            // func->index and with it the cross-module [init] order differ cached vs fresh
            auto & libModules = program->library.getModules();
            if ( thisAt + 1 < uint64_t(libModules.size()) ) {
                libModules.pop_back();
                libModules.insert(libModules.begin() + size_t(thisAt), program->thisModule.get());
            } else if ( thisAt + 1 > uint64_t(libModules.size()) ) {
                LOG(LogLevel::warning) << "das: serialize: program module index " << thisAt
                    << " out of range (" << libModules.size() << " modules)\n";
                program->failToCompile = true;
                return;
            }
        }

        // the node table is per program
        clearNodeIds();
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
        } catch ( ... ) {
            // noexcept for embedders built without EH: a non-das exception (e.g.
            // std::bad_alloc deep in serialize) would std::terminate here. Contain it.
            program->failToCompile = true;
            LOG(LogLevel::warning) << "das: serialize: unknown exception\n";
            return false;
        }
    }

    // Used in daNetGame currently
    void Program::serialize ( AstSerializer & ser ) {
        SerializerRecordScope record(ser);
        // version gate: any layout change shifts every subsequent field, so a stale stream must
        // fail cleanly here — not misparse into a patch() throw thousands of fields later
        uint32_t version = AstSerializer::getVersion();
        ser << version;
        if ( !ser.writing && version != AstSerializer::getVersion() ) {
            LOG(LogLevel::warning) << "das: deserialize: stream version " << version
                << " does not match serializer version " << AstSerializer::getVersion() << "\n";
            failToCompile = true;
            return;
        }

        ser.clearNodeIds();             // numbering restarts with every program, on both sides
        ser << thisNamespace << thisModuleName;

        ser << totalFunctions      << totalVariables << newLambdaIndex;
        ser << globalInitStackSize << globalStringHeapSize;
        ser << flags;

        ser << options << policies;
        ser.readJitEnabled = policies.jit_enabled;  // what finalizeModule hands the macro program a served module reinstantiates
        ser.readOptions = options;

    // serialize library
        if ( ser.writing ) {
            ser.moduleLibrary = &library;
            TopSort ts(library.modules);
            auto modules = ts.getDependecyOrderedEndingWith(thisModule.get());

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
                ser << m->name << m->fileName << m->promotedRequire;

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
            ser.buffer->flush();
            return;
        }

        // parseDaScript runs with the placeholder thisModule's gc root as the
        // thread-active root; library.reset() below deletes that module (and its
        // root) - without repointing, every node deserialized after this line would
        // gc_link through a dangling root pointer into freed memory
        auto & activeRoot = gc_root::gc_get_active_root();
        const bool activeWasThisModule = thisModule && activeRoot == thisModule->module_gc_root.get();
        // throwaway already-exists reads park nodes here - they may be referenced
        // through the patch maps until all modules are read, then sweep with scope
        gc_root throwaway_root;
        ActiveRootGuard throwaway_guard { &throwaway_root };
        if ( activeWasThisModule ) activeRoot = &throwaway_root;
        library.reset();
        thisModule.release();
        ser.moduleLibrary = &library;

        uint64_t size_builtin = 0; ser << size_builtin;
        for ( uint64_t i = 0; i < size_builtin; i++ ) {
            string name; ser.serializeTemp(name);
            Module * m = requireBuiltinModule(name);
            library.addModule(m);
        }

        uint64_t size = 0; ser << size;
        for ( uint64_t i = 0; i < size; i++ ) {
            bool builtin = false, promoted = false;
            string name, fileName, promotedRequire;
            ser << builtin << promoted;
            ser.serializeTemp(name);
            ser.serializeTemp(fileName);
            ser.serializeTemp(promotedRequire);
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
                        // throwaway read into a temp module — keep nodes off its root
                        // (they may be referenced through the patch maps past `delete mod`)
                        ser.serializeModule(*mod, /*already_exists*/true);
                        mod->builtIn = false; // suppress assert
                        delete mod;
                    } else {
                        library.addModule(mod);
                        if ( activeWasThisModule ) activeRoot = mod->module_gc_root.get();
                        // the active root points at mod's root while it is read; if the read
                        // throws, mod dies with the replaced program, so drop the active root
                        // back to the permanent thread root before the exception propagates
                        // (the ActiveRootGuard only tracks throwaway_root, not this one)
                        try {
                            ser.serializeModule(*mod, /*already_exists*/false);
                        } catch ( ... ) {
                            if ( activeWasThisModule ) activeRoot = &gc_root::gc_get_thread_root();
                            throw;
                        }
                        if ( activeWasThisModule ) activeRoot = &throwaway_root;
                        mod->promoteToBuiltin(nullptr, promotedRequire);
                    }
                } else {
                    Module * m = Module::require(name);
                    library.addModule(m);
                }
            } else {
                auto mod = new ModuleDas;
                mod->setModuleName(name);
                mod->fileName = fileName;
                library.addModule(mod);
                if ( activeWasThisModule ) activeRoot = mod->module_gc_root.get();
                // see the promoted-new branch above: restore the thread root if the read
                // throws, so a dangling mod->module_gc_root never stays the active root
                try {
                    ser << *mod;
                } catch ( ... ) {
                    if ( activeWasThisModule ) activeRoot = &gc_root::gc_get_thread_root();
                    throw;
                }
                if ( activeWasThisModule ) activeRoot = &throwaway_root;
            }
        }

        thisModule.reset(library.modules.back());
        // the deserialized module is the program's module now — new nodes (allocateStack
        // init script, etc.) and the ModuleGcFinalize collect belong on its root
        if ( activeWasThisModule ) activeRoot = thisModule->module_gc_root.get();

        ser << allRequireDecl;

    // for the last module, mark symbols manually
        auto setup0 = ref_time_ticks();
        markExecutableSymbolUse();
        removeUnusedSymbols();
        TextWriter logs;
        allocateStack(logs,true,false);
        ser.totSetupTime += get_time_usec(setup0);
    }

#if !DAS_NO_FILEIO
    static void ensureParentDirectories ( const string & path ) {
        for ( size_t i = 1; i < path.size(); ++i ) {
            if ( path[i] == '/' || path[i] == '\\' ) {
                string dir = path.substr(0, i);
#ifdef _WIN32
                _mkdir(dir.c_str());
#else
                mkdir(dir.c_str(), 0755);
#endif
            }
        }
    }
#endif

#if !DAS_NO_FILEIO
    static const char * MODULE_CACHE_DEFAULT_DIR = ".jitted_scripts/module_cache/";

    // DAS_MODULE_CACHE_LIMIT, megabytes: 4096 unless set, 0 = no eviction; garbage keeps the default
    static uint64_t moduleCacheLimitBytes () {
        uint64_t mb = 4096;
        if ( const char * env = get_dasenv_module_cache_limit() ) {
            if ( *env ) {
                char * end = nullptr;
                unsigned long long v = strtoull(env, &end, 10);
                if ( end && *end == 0 ) mb = uint64_t(v);
            }
        }
        return mb * 1024ull * 1024ull;
    }

    struct CacheRecordInfo {
        string      path;
        uint64_t    size;
        int64_t     mtime;
    };

    static void listCacheRecords ( const string & dir, vector<CacheRecordInfo> & out ) {
#ifdef _WIN32
        struct _finddata_t c_file;
        string findPath = dir + "*.dascache";
        intptr_t hFile = _findfirst(findPath.c_str(), &c_file);
        if ( hFile != -1L ) {
            do {
                out.push_back({dir + c_file.name, uint64_t(c_file.size), int64_t(c_file.time_write)});
            } while ( _findnext(hFile, &c_file) == 0 );
            _findclose(hFile);
        }
#else
        if ( DIR * d = opendir(dir.c_str()) ) {
            while ( dirent * e = readdir(d) ) {
                string name = e->d_name;
                if ( name.size() < 9 || name.compare(name.size() - 9, 9, ".dascache") != 0 ) continue;
                string p = dir + name;
                struct stat st;
                if ( stat(p.c_str(), &st) == 0 ) out.push_back({p, uint64_t(st.st_size), int64_t(st.st_mtime)});
            }
            closedir(d);
        }
#endif
    }

    static long long das_process_id () {
#ifdef _WIN32
        return (long long) _getpid();
#else
        return (long long) getpid();
#endif
    }

    static void touchFile ( const string & path ) {
#ifdef _WIN32
        (void) _utime(path.c_str(), nullptr);
#else
        (void) utime(path.c_str(), nullptr);
#endif
    }

    // LRU eviction of the DEFAULT cache directory down to the limit after a writeback; the record
    // just written stays, and a record a run read was touched, so it is never the oldest. An
    // explicit -module-cache path is the user's directory and is never pruned.
    static void evictModuleCache ( const string & justWrote ) {
        size_t dirLen = strlen(MODULE_CACHE_DEFAULT_DIR);
        if ( justWrote.compare(0, dirLen, MODULE_CACHE_DEFAULT_DIR) != 0 ) return;
        uint64_t limit = moduleCacheLimitBytes();
        if ( limit == 0 ) return;
        vector<CacheRecordInfo> recs;
        listCacheRecords(MODULE_CACHE_DEFAULT_DIR, recs);
        uint64_t total = 0;
        for ( auto & r : recs ) total += r.size;
        if ( total <= limit ) return;
        sort(recs.begin(), recs.end(), [](const CacheRecordInfo & a, const CacheRecordInfo & b) {
            return a.mtime != b.mtime ? a.mtime < b.mtime : a.path < b.path;
        });
        for ( auto & r : recs ) {
            if ( total <= limit ) break;
            if ( r.path == justWrote ) continue;
            if ( remove(r.path.c_str()) == 0 ) total -= r.size;
        }
    }
#endif

    string ModuleFileCache::defaultPath ( const string & scriptPath, const string & hostBinary, const string & hostOptions ) {
        string norm = normalizeFileName(scriptPath.c_str());
        size_t slash = norm.find_last_of("/\\");
        string stem = slash == string::npos ? norm : norm.substr(slash + 1);
        size_t dot = stem.rfind('.');
        if ( dot != string::npos && dot != 0 ) stem = stem.substr(0, dot);
        struct stat bst;
        char host[64];
        if ( !hostBinary.empty() && stat(hostBinary.c_str(), &bst) == 0 ) {
            snprintf(host, sizeof(host), "\n%lld:%lld", (long long) bst.st_mtime, (long long) bst.st_size);
        } else {
            host[0] = 0;
        }
        vector<string> envs;
        for ( char * const * e = das_environ(); e && *e; ++e ) {
            if ( strncmp(*e, "DAS", 3) == 0 && strncmp(*e, "DAS_MODULE_CACHE_LIMIT=", 23) != 0
                && !das_dep_is_env_entry(*e) ) envs.push_back(*e);
        }
        sort(envs.begin(), envs.end());
        string key = norm + host + "\n" + hostOptions;
        for ( auto & e : envs ) {
            key += "\n";
            key += e;
        }
        char hex[17];
        snprintf(hex, sizeof(hex), "%016llx", (unsigned long long) hash_blockz64((const uint8_t *) key.c_str()));
        return string(".jitted_scripts/module_cache/") + stem + "-" + string(hex, 8) + ".dascache";
    }

    string ModuleFileCache::embeddedHostOptions ( const CodeOfPolicies & policies ) {
        string key;
        Array args;
        getCommandLineArguments(args);
        auto argv = (char **) args.data;
        for ( uint32_t i=1; i<args.size; ++i ) {
            const char * a = argv[i] ? argv[i] : "";
            if ( strcmp(a, "--") == 0 ) break;
            if ( int skip = das_dep_flag_argc(a) ) { i += uint32_t(skip) - 1; continue; }
            key += a;
            key += '\n';
        }
        SerializationStorageVector storage;
        AstSerializer ser(&storage, true);
        CodeOfPolicies streamed = policies;
        ser << streamed;
        storage.flush();
        string jitTarget = commandLineArgumentOccurrences("--jit-target");   // NUL-separated; the key hashes as a C string
        for ( auto & ch : jitTarget ) {
            if ( ch == 0 ) ch = '\n';
        }
        key += jitTarget;
        char hex[17];
        snprintf(hex, sizeof(hex), "%016llx", (unsigned long long) hash_block64(storage.buffer.data(), storage.buffer.size()));
        key += "policies:";
        key += hex;
        return key;
    }

    void ModuleFileCache::install ( const string & readFrom, const string & writeTo, bool quiet ) {
        requested = !readFrom.empty() || !writeTo.empty();
#if !DAS_NO_FILEIO
        writePath = writeTo;
        auto & env = *daScriptEnvironment::getBound();
        if ( !readFrom.empty() ) {
            if ( FILE * f = fopen(readFrom.c_str(), "rb") ) {
                // 64-bit tell: plain ftell returns long (32-bit on Windows), where an
                // oversized cache would read back negative and silently count as absent
#ifdef _MSC_VER
                int64_t size = (fseek(f, 0, SEEK_END) == 0) ? _ftelli64(f) : -1;
#else
                int64_t size = (fseek(f, 0, SEEK_END) == 0) ? int64_t(ftello(f)) : -1;
#endif
                if ( size > 0 && fseek(f, 0, SEEK_SET) == 0 ) {
                    readStorage.buffer.resize(size_t(size));
                    if ( fread(readStorage.buffer.data(), 1, size_t(size), f) != size_t(size) ) {
                        readStorage.buffer.clear();     // short read - treat as no cache
                    }
                }
                fclose(f);
            }
            if ( !readStorage.buffer.empty() ) {
                reader = make_unique<AstSerializer>(&readStorage, false);
                reader->quietCache = quiet;
                env.serializer_read = reader.get();
                touchFile(readFrom);    // a record in use is the newest for the eviction's LRU
            }
        }
        if ( !writePath.empty() ) {
            writer = make_unique<AstSerializer>(&writeStorage, true);
            writer->quietCache = quiet;
            env.serializer_write = writer.get();
        }
#else
        (void) quiet;
#endif
    }

    ModuleFileCache::Result ModuleFileCache::finish () {
        Result res;
#if DAS_NO_FILEIO
        // no filesystem on this target: install() bound nothing and the compile parsed
        // from source - answer 'unavailable' rather than a silent 'none'
        if ( requested ) res.verdict = ReadVerdict::unavailable;
#else
        auto & env = *daScriptEnvironment::getBound();
        // the debugger path (disableSerializationOnDebugger) may have cleared these
        // mid-compile; clearing again is harmless, leaving them dangling is not
        env.serializer_read = nullptr;
        env.serializer_write = nullptr;
        if ( reader ) {
#if DAS_SERIALIZE_PROFILE
            if ( !reader->quietCache ) { LOG tw(LogLevel::info); reader->profReport(tw); }
#endif
            if ( !reader->checkedStreamHeader ) {
                res.verdict = ReadVerdict::none;    // nothing was ever read (rail disabled mid-compile)
            } else if ( reader->failed || reader->seenNewModule ) {
                res.verdict = ReadVerdict::fallback;
            } else if ( reader->resumedModules != 0 ) {
                res.verdict = ReadVerdict::partial;
            } else {
                res.verdict = ReadVerdict::clean;
            }
            res.resumed = reader->resumedModules;
            res.served = reader->servedModules;
            res.cutoffFile = reader->cutoffFile;
            res.cutoffReason = reader->cutoffReason;
            reader->moduleLibrary = nullptr;    // not ours to free
            // claim the FileInfos the reader created that no module took ownership of -
            // ~AstSerializer would DELETE them, and the deserialized AST's LineInfos still
            // point at them (the crash is deferred to the first LineInfo::describe)
            reader->collectFileInfo(orphanedFileInfos);
            reader.reset();
        }
        if ( writer ) {
#if DAS_SERIALIZE_PROFILE
            if ( !writer->quietCache ) { LOG tw(LogLevel::info); writer->profReport(tw); }
#endif
            writer->moduleLibrary = nullptr;
            writer.reset();     // releases the parsedModules program refs before the program runs
            writeStorage.flush();
            if ( !writeStorage.buffer.empty() ) {
                // write-to-temp + rename: a concurrent reader on the same path sees a
                // complete old or a complete new stream, never a torn one. The CRT rename
                // cannot replace on Windows, so there is a missing-file blink there - a
                // reader in that window just takes the cold path
                static atomic<uint64_t> writebackSeq { 0 };
                string tmpPath = writePath + "." + to_string(das_process_id()) + "." + to_string(writebackSeq.fetch_add(1)) + ".tmp";
                ensureParentDirectories(writePath);
                if ( FILE * f = fopen(tmpPath.c_str(), "wb") ) {
                    res.wroteBytes = uint64_t(fwrite(writeStorage.buffer.data(), 1, writeStorage.buffer.size(), f));
                    fclose(f);
                    res.wrote = res.wroteBytes == uint64_t(writeStorage.buffer.size());
                    if ( res.wrote ) {
#ifdef _WIN32
                        remove(writePath.c_str());
#endif
                        res.wrote = rename(tmpPath.c_str(), writePath.c_str()) == 0;
                    }
                    if ( !res.wrote ) remove(tmpPath.c_str());   // never leave a corpse
                    res.saveFailed = !res.wrote;
                    if ( res.wrote ) evictModuleCache(writePath);
                } else {
                    res.saveFailed = true;
                }
            }
        }
#endif
        return res;
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

    void rtti_ast_serializer_deserialize_program_ex (
            AstSerializerState * state, smart_ptr<FileAccess> access, ModuleGroup * libGroup,
            const TBlock<void,bool,smart_ptr<Program>,const string> & block,
            Context * context, LineInfoArg * at ) {
        auto prog = make_smart<Program>();
        prog->access = access;      // the reader's: a stream carries none (src/ast/ARCHITECTURE.md#require-after-walk)
        state->serializer->thisModuleGroup = libGroup;
        {
            gc_guard deserialize_gc_scope;
            // same-version streams can still be truncated/corrupt: the stream readers throw
            // dasException — turn it into the clean failToCompile path (mirrors serializeScript)
            try {
                prog->serialize(*state->serializer);
            } catch ( const dasException & r ) {
                prog->failToCompile = true;
                LOG(LogLevel::warning) << "das: deserialize: " << r.what() << "\n";
            }
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

    void rtti_ast_serializer_deserialize_program (
            AstSerializerState * state, ModuleGroup * libGroup,
            const TBlock<void,bool,smart_ptr<Program>,const string> & block,
            Context * context, LineInfoArg * at ) {
        rtti_ast_serializer_deserialize_program_ex(state, nullptr, libGroup, block, context, at);
    }

    int64_t rtti_ast_serializer_finalize_usec ( AstSerializerState * state ) {
        return state ? int64_t(state->serializer->totFinalizeTime) : 0;
    }

    int64_t rtti_ast_serializer_setup_usec ( AstSerializerState * state ) {
        return state ? int64_t(state->serializer->totSetupTime) : 0;
    }

    void rtti_ast_serializer_get_data (
            AstSerializerState * state,
            const TBlock<void,TTemporary<TArray<uint8_t> const>> & block,
            Context * context, LineInfoArg * at ) {
        state->storage->flush();
        Array arr;
        array_mark_locked(arr, state->storage->buffer.data(),
            uint64_t(state->storage->buffer.size()), uint64_t(state->storage->buffer.size()));
        vec4f args[1] = { cast<Array *>::from(&arr) };
        context->invoke(block, args, nullptr, at);
    }

}
