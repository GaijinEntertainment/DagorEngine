#include "daScript/misc/platform.h"

#include "daScript/simulate/simulate.h"
#include "daScript/simulate/heap.h"
#include "daScript/misc/debug_break.h"

namespace das {

#if DAS_TRACK_ALLOCATIONS
    uint64_t    g_tracker_string = 0;
    uint64_t    g_breakpoint_string = -1ul;

    void das_track_string_breakpoint ( uint64_t id ) {
        g_breakpoint_string = id;
    }
#endif

    char * AnyHeapAllocator::allocateName ( const string & name ) {
        if (!name.empty()) {
            auto length = uint32_t(name.length());
            if (auto str = (char *)allocate(length + 1)) {
                memcpy(str, name.c_str(), length);
                str[length] = 0;
                return str;
            }
        }
        return nullptr;
    }

    char * StringHeapAllocator::allocateString ( const string & str ) {
        return allocateString ( str.c_str(), uint32_t(str.length()) );
    }

    bool PersistentHeapAllocator::mark() {
        model.shoe.beforeGC();
        return true;
    }

    bool PersistentHeapAllocator::mark ( char * ptr, uint32_t len ) {
        auto size = (len + 15) & ~15; // model.alignMask
#if !DAS_TRACK_ALLOCATIONS
        if ( size <= DAS_MAX_SHOE_ALLOCATION ) {
            return model.shoe.mark(ptr,size);
        } else
#endif
        {
            auto it = model.bigStuff.find(ptr);
            if ( it != model.bigStuff.end() ) {
                bool marked = it->second & DAS_PAGE_GC_MASK;
                it->second |= DAS_PAGE_GC_MASK;
                return !marked;
            }
        }
        return false;
    }

    void PersistentHeapAllocator::report() {
        LOG tout(LogLevel::debug);
        for ( uint32_t si=0; si!=DAS_MAX_SHOE_CUNKS; ++si ) {
            if ( model.shoe.chunks[si] ) tout << "decks of size " << int((si+1)<<4) << "\n";
            for ( auto ch=model.shoe.chunks[si]; ch; ch=ch->next ) {
                tout << HEX << "\t" << "[" << uint64_t(ch->data) << ".." << (uint64_t(ch->data)+(ch->size*ch->total)) << ")\n" << DEC;
                tout << "\t" << ch->allocated << " of " << ch->total << ", " << (ch->allocated*ch->size) << " of " << ch->totalBytes << " bytes\n";
            }
        }
        if ( !model.bigStuff.empty() ) {
            tout << "big stuff:\n";
#if DAS_TRACK_ALLOCATIONS
            tout << "\tsize\tpointer\t\tid\n";
#else
            tout << "\tsize\tpointer\n";
#endif
            for ( const auto & it : model.bigStuff ) {
#if DAS_TRACK_ALLOCATIONS
                auto itId = model.bigStuffId.find(it.first);
                uint64_t eeid = itId==model.bigStuffId.end() ? 0 : itId->second;
                tout << "\t" << it.second << "\t0x" << HEX << intptr_t(it.first) << DEC
                    << "\t" << eeid;
                auto itComment = model.bigStuffComment.find(it.first);
                if ( itComment != model.bigStuffComment.end() ) {
                    tout << "\t" << itComment->second;
                }
                auto itLoc = model.bigStuffAt.find(it.first);
                if ( itLoc != model.bigStuffAt.end() ) {
                    tout << "\t" << itLoc->second->describe();
                }
                tout << "\n";
#else
                tout << "\t" << it.second << "\t0x" << HEX << intptr_t(it.first) << DEC
                    << "\n";
#endif
            }
#if DAS_TRACK_ALLOCATIONS
            das_safe_map<LineInfo,int64_t> bytesPerLocation;
            for ( const auto & ppl : model.bigStuffAt) {
                auto ptr = ppl.first;
                auto bytes = model.bigStuff[ptr];
                bytesPerLocation[*(ppl.second)] += bytes;
            }
            if ( !bytesPerLocation.empty() ) {
                vector<pair<int64_t,LineInfo>> bplv;
                bplv.reserve(bytesPerLocation.size());
                for ( const auto & bpl : bytesPerLocation ) {
                    bplv.emplace_back(make_pair(bpl.second,bpl.first));
                }
                sort(bplv.begin(), bplv.end(), [&]( auto a, auto b){
                    return a.first > b.first;
                });
                tout << "bytes per location:\n";
                for ( const auto & bpl : bplv ) {
                    tout << bpl.first << "\t" << bpl.second.describe() << "\n";
                }
            }
#endif
        }
    }

    void PersistentStringAllocator::forEachString ( const callable<void (const char *)> & fn ) {
        for ( uint32_t si=0; si!=DAS_MAX_SHOE_CUNKS; ++si ) {
            for ( auto ch=model.shoe.chunks[si]; ch; ch=ch->next ) {
                uint32_t utotal = ch->total / 32;
                for ( uint32_t i=0; i!=utotal; ++i ) {
                    uint32_t b = ch->bits[i];
                    for ( uint32_t j=0; j!=32; ++j ) {    // todo: simpler bit loop
                        if ( b & (1<<j) ) {
                            fn ( ch->data + (i*32 + j)*ch->size );
                        }
                    }
                }
            }
        }
        if ( !model.bigStuff.empty() ) {
            for ( auto it : model.bigStuff ) {
                fn ( (char*) it.first );
            }
        }
    }

    void LinearHeapAllocator::report() {
        LOG tout(LogLevel::debug);
        for ( auto ch=model.chunk; ch; ch=ch->next ) {
            tout << HEX << intptr_t(ch->data) << DEC << "\t"
                << ch->offset << " of " << ch->size << "\n";
        }
    }

    void StringHeapAllocator::setIntern(bool on) {
        needIntern = on;
        if ( !needIntern ) {
            das_string_set empty;
            swap ( internMap, empty );
        }
    }

    void StringHeapAllocator::reset() {
        das_string_set empty;
        swap ( internMap, empty );
    }

    char * StringHeapAllocator::intern(const char * str, uint32_t length) const {
        if ( needIntern ) {
            auto it = internMap.find(StrHashEntry(str,length));
            return it != internMap.end() ? (char *)it->ptr : nullptr;
        } else {
            return nullptr;
        }
    }

    void StringHeapAllocator::recognize ( char * str ) {
        if ( !str ) return;
        uint32_t length = uint32_t(strlen(str));
        uint32_t size = length + 1;
        size = (size + 15) & ~15;
        if ( needIntern && isOwnPtr(str, size) ) {
            internMap.insert(StrHashEntry(str,length));
        }
    }

    char * ConstStringAllocator::intern(const char * str, uint32_t length) const {
        auto it = internMap.find(StrHashEntry(str,length));
        return it != internMap.end() ? (char*)it->ptr : nullptr;
    }

    void ConstStringAllocator::reset () {
        LinearChunkAllocator::reset();
        das_string_set dummy;
        swap(internMap, dummy);
    }

    char * ConstStringAllocator::allocateString ( const char * text, uint32_t length ) {
        if ( length ) {
            if ( text ) {
                auto it = internMap.find(StrHashEntry(text,length));
                if ( it != internMap.end() ) {
                    return (char *) it->ptr;
                }
            }
            if ( auto str = (char *)allocate(length + 1) ) {
                if ( text ) memcpy(str, text, length);
                str[length] = 0;
                internMap.insert(StrHashEntry(str,length));
                return str;
            }
        }
        return nullptr;
    }

    char * StringHeapAllocator::allocateString ( const char * text, uint32_t length ) {
        if ( length ) {
            if ( needIntern && text ) {
                auto it = internMap.find(StrHashEntry(text,length));
                if ( it != internMap.end() ) {
                    return (char *) it->ptr;
                }
            }
            if ( auto str = (char *)allocate(length + 1) ) {
#if DAS_TRACK_ALLOCATIONS
                if ( g_tracker_string==g_breakpoint_string ) os_debug_break();
#endif
                if ( text ) memmove(str, text, length);
                str[length] = 0;
                if ( needIntern && text ) internMap.insert(StrHashEntry(str,length));
                return str;
            }
        }
        return nullptr;
    }

    void StringHeapAllocator::freeString ( char * text, uint32_t length ) {
        if ( needIntern ) internMap.erase(StrHashEntry(text,length));
        free ( text, length + 1 );
    }

    char * presentStr ( char * buf, char * ch, int size ) {
        auto str = escapeString(ch);
        strncpy(buf,str.c_str(),size);
        buf[size-4] = '.';
        buf[size-3] = '.';
        buf[size-2] = '.';
        buf[size-1] = 0;
        return buf;
    }

    bool PersistentStringAllocator::mark ( char * ptr, uint32_t len ) {
        auto size = (len + 15) & ~15; // model.alignMask
#if !DAS_TRACK_ALLOCATIONS
        if (size <= DAS_MAX_SHOE_ALLOCATION) {
            return model.shoe.mark(ptr,len);
        } else
#endif
        {
            auto it = model.bigStuff.find(ptr);
            if ( it != model.bigStuff.end() ) {
                bool marked = it->second & DAS_PAGE_GC_MASK;
                it->second |= DAS_PAGE_GC_MASK;
                return !marked;
            }
        }
        return false;
    }

    bool PersistentStringAllocator::mark() {
        model.shoe.beforeGC();
        return true;
    }

    void PersistentStringAllocator::sweep() {
        model.sweep();
        if ( needIntern ) {
            das_string_set empty;
            swap ( internMap, empty );
            forEachString([&](const char * str){
                uint32_t length = uint32_t(strlen(str));
                internMap.insert(StrHashEntry(str,length));
            });
        }
    }

    void PersistentStringAllocator::report() {
        LOG tout(LogLevel::debug);
        char buf[33];
        for ( uint32_t si=0; si!=DAS_MAX_SHOE_CUNKS; ++si ) {
            if ( model.shoe.chunks[si] ) tout << "string decks of size " << int((si+1)<<4) << "\n";
            uint64_t bytesInDeck = 0;
            uint32_t totalChunks = 0;
            for ( auto ch=model.shoe.chunks[si]; ch; ch=ch->next ) {
                bytesInDeck += ch->totalBytes;
                totalChunks ++;
                tout << "\t" << HEX << "[" << uint64_t(ch->data) << ".." << (uint64_t(ch->data)+(ch->size*ch->total)) << ")\n" << DEC;
                tout << "\t" << ch->allocated << " of " << ch->total << ", " << (ch->allocated*ch->size) << " of " << ch->totalBytes << " bytes\n";
                uint32_t utotal = ch->total / 32;
                for ( uint32_t i=0; i!=utotal; ++i ) {
                    uint32_t b = ch->bits[i];
                    for ( uint32_t j=0; j!=32; ++j ) {
                        if ( b & (1<<j) ) {
                            char * str = ( ch->data + (i*32 + j)*ch->size );
                            tout << "\t\t" << presentStr(buf,str,32) << "\n";
                        }
                    }
                }
            }
            if (totalChunks != 0)
                tout << "decks reserves " << (bytesInDeck + 1023) / 1024 << " kb in " << totalChunks << " chunks\n";
        }
        uint64_t totalBigStuff = 0;
        if ( !model.bigStuff.empty() ) {
            tout << "big stuff:\n";
            for ( auto it : model.bigStuff ) {
                char * ch = (char *)it.first;
                tout << "\t" << presentStr(buf,ch,32) << " size " << it.second << " bytes, at 0x" << uint64_t(ch) << "\n";
                totalBigStuff += it.second;
            }
            tout << " big stuff total size:" << (totalBigStuff + 1023) / 1024 << " kb\n";
        }
    }

    void LinearStringAllocator::report() {
        LOG tout(LogLevel::debug);
        char buf[33];
        for ( auto ch=model.chunk; ch; ch=ch->next ) {
            tout << HEX << intptr_t(ch->data) << DEC << "\t"
                << ch->offset << " of " << ch->size << "\n";
            char * tail = ch->data + ch->offset;
            for ( char * txt = ch->data; txt!=tail; ) {
                strncpy(buf,txt,32);
                buf[32] = 0;
                tout << "\t" << presentStr(buf,txt,32) << "\n";
                auto sz = uint32_t(strlen(txt)) + 1;
                sz = ( sz + model.alignMask ) & ~model.alignMask;
                txt += sz;
            }
        }
    }

    void LinearStringAllocator::forEachString ( const callable<void (const char *)> & fn ) {
        for ( auto ch=model.chunk; ch; ch=ch->next ) {
            char * tail = ch->data + ch->offset;
            for ( char * txt = ch->data; txt!=tail; ) {
                fn(txt);
                auto sz = uint32_t(strlen(txt)) + 1;
                sz = ( sz + model.alignMask ) & ~model.alignMask;
                txt += sz;
            }
        }
    }

    char * DebugInfoAllocator::allocateCachedName ( const string & name ) {
        auto it = stringLookup.find(name);
        if ( it!=stringLookup.end() )  return it->second;
        auto nname = NodeAllocator::allocateName(name);
        stringLookup[name] = nname;
        uint32_t bytes = uint32_t(name.size()+1);
        bytes = (bytes+15) & ~15;
        stringBytes += bytes;
        return nname;
    }
}
