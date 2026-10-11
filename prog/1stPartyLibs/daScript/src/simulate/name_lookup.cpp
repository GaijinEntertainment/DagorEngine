#include "daScript/misc/platform.h"

#include "daScript/simulate/name_lookup.h"
#include "daScript/misc/anyhash.h"

namespace das {

    static const uint32_t g_emptyDisp[1] = { 0 };
    static const NameLookup::Entry g_emptyEntries[1] = { { 0, NameLookup::NOT_FOUND, NameLookup::NOT_FOUND, -1, 0 } };
    static const NameLookup::NameSlot g_emptyNames[1] = { { 0, -1, 0 } };

    NameLookup::NameLookup () {
        pointAtEmpty();
    }

    NameLookup::~NameLookup () {
        if ( blob ) das_aligned_free16(blob);
    }

    void NameLookup::pointAtEmpty () {
        byMnh = PerfectHash();
        byMnh.disp = g_emptyDisp;
        byName = PerfectHash();
        byName.disp = g_emptyDisp;
        entries = g_emptyEntries;
        names = g_emptyNames;
        count = 0;
    }

    uint64_t NameLookup::hashName ( const char * name ) {
        return hash_blockz64((const uint8_t *) (name ? name : ""));
    }

    void NameLookup::insert ( uint64_t mnh, const char * name, uint32_t index, uint32_t value ) {
        if ( sealed ) DAS_FATAL_ERROR("NameLookup::insert after seal: '%s'\n", name ? name : "");
        if ( !name ) name = "";
        auto nameOffset = uint32_t(nameBytes.size());
        nameBytes.insert(nameBytes.end(), name, name + strlen(name) + 1);
        staged.push_back(Staged{mnh, hashName(name), nameOffset, index, value});
    }

    void NameLookup::adopt ( const StaticTable & table ) {
        if ( sealed ) DAS_FATAL_ERROR("NameLookup::adopt after seal\n");
        byMnh.nbuckets = table.mnhBuckets;
        byMnh.nslots = table.mnhSlots;
        byMnh.disp = table.mnhDisp;
        byName.nbuckets = table.nameBuckets;
        byName.nslots = table.nameSlots;
        byName.disp = table.nameDisp;
        entries = table.entries;
        names = table.names;
        count = table.count;
        sealed = true;
    }

    bool NameLookup::PerfectHash::build ( const vector<uint64_t> & keys, vector<uint32_t> & dispOut, uint32_t slack ) {
        uint32_t n = uint32_t(keys.size());
        nbuckets = das::max(uint32_t(1), (n + 4) / 5);
        nslots = n + das::max(uint32_t(1), n / 20) * slack;
        vector<uint64_t> scrambled(n);
        vector<uint32_t> bucketOf(n);
        vector<uint32_t> bucketSize(nbuckets, 0);
        for ( uint32_t i=0; i!=n; ++i ) {
            scrambled[i] = scramble(keys[i]);
            bucketOf[i] = fastRange(uint32_t(scrambled[i] >> 32), nbuckets);
            bucketSize[bucketOf[i]] ++;
        }
        vector<uint32_t> bucketStart(nbuckets + 1, 0);
        for ( uint32_t b=0; b!=nbuckets; ++b ) bucketStart[b + 1] = bucketStart[b] + bucketSize[b];
        vector<uint32_t> members(n);
        {
            vector<uint32_t> fill(bucketStart.begin(), bucketStart.end() - 1);
            for ( uint32_t i=0; i!=n; ++i ) members[fill[bucketOf[i]]++] = i;
        }
        vector<uint32_t> order(nbuckets);
        for ( uint32_t b=0; b!=nbuckets; ++b ) order[b] = b;
        das::stable_sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) {
            return bucketSize[a] > bucketSize[b];
        });
        dispOut.assign(nbuckets, 0);
        vector<uint8_t> occupied(nslots, 0);
        vector<uint32_t> slots;
        const uint32_t dispLimit = 1u << 24;
        for ( uint32_t b : order ) {
            uint32_t first = bucketStart[b], last = bucketStart[b + 1];
            if ( first==last ) break;
            for ( uint32_t d=0; ; ++d ) {
                slots.clear();
                bool fits = true;
                for ( uint32_t m=first; m!=last && fits; ++m ) {
                    uint32_t s = fastRange(mix(scrambled[members[m]], d), nslots);
                    if ( occupied[s] ) { fits = false; break; }
                    for ( uint32_t t : slots ) if ( t==s ) { fits = false; break; }
                    slots.push_back(s);
                }
                if ( fits ) {
                    for ( uint32_t s : slots ) occupied[s] = 1;
                    dispOut[b] = d;
                    break;
                }
                if ( d==dispLimit ) return false;
            }
        }
        disp = dispOut.data();
        return true;
    }

    bool NameLookup::seal ( string * failure ) {
        if ( sealed ) {
            if ( failure ) *failure = "sealed twice";
            return false;
        }
        das::sort(staged.begin(), staged.end(), [](const Staged & a, const Staged & b) {
            return a.mnh < b.mnh;
        });
        for ( size_t i=1; i<staged.size(); ++i ) {
            if ( staged[i - 1].mnh==staged[i].mnh ) {
                if ( failure ) *failure = string("mangled name hash collision '") + stagedName(staged[i - 1]) + "' and '" + stagedName(staged[i]) + "'";
                pointAtEmpty();
                return false;
            }
        }
        vector<uint64_t> keys;
        keys.reserve(staged.size());
        for ( auto & s : staged ) keys.push_back(s.mnh);
        vector<uint32_t> mnhDisp;
        bool built = false;
        for ( uint32_t slack=1; slack<=4 && !built; ++slack ) built = byMnh.build(keys, mnhDisp, slack);
        if ( !built ) {
            pointAtEmpty();
            if ( failure ) *failure = "mangled name perfect hash did not converge";
            return false;
        }
        vector<Entry> builtEntries(byMnh.nslots, Entry{0, NOT_FOUND, NOT_FOUND, -1, 0});
        for ( auto & s : staged ) {
            builtEntries[byMnh.slotOf(s.mnh)] = Entry{s.mnh, s.value, s.index, -1, 0};
        }
        das::sort(staged.begin(), staged.end(), [](const Staged & a, const Staged & b) {
            return a.nameHash!=b.nameHash ? a.nameHash < b.nameHash : a.index < b.index;
        });
        keys.clear();
        for ( size_t i=0; i<staged.size(); ) {
            size_t j = i + 1;
            while ( j<staged.size() && staged[j].nameHash==staged[i].nameHash ) {
                if ( strcmp(stagedName(staged[j]), stagedName(staged[i]))!=0 ) {
                    if ( failure ) *failure = string("name hash collision '") + stagedName(staged[i]) + "' and '" + stagedName(staged[j]) + "'";
                    pointAtEmpty();
                    return false;
                }
                builtEntries[byMnh.slotOf(staged[j - 1].mnh)].next = int32_t(byMnh.slotOf(staged[j].mnh));
                ++j;
            }
            keys.push_back(staged[i].nameHash);
            i = j;
        }
        vector<uint32_t> nameDisp;
        built = false;
        for ( uint32_t slack=1; slack<=4 && !built; ++slack ) built = byName.build(keys, nameDisp, slack);
        if ( !built ) {
            pointAtEmpty();
            if ( failure ) *failure = "name perfect hash did not converge";
            return false;
        }
        vector<NameSlot> builtNames(byName.nslots, NameSlot{0, -1, 0});
        for ( size_t i=0; i<staged.size(); ) {
            size_t j = i + 1;
            while ( j<staged.size() && staged[j].nameHash==staged[i].nameHash ) ++j;
            builtNames[byName.slotOf(staged[i].nameHash)] = NameSlot{staged[i].nameHash, int32_t(byMnh.slotOf(staged[i].mnh)), 0};
            i = j;
        }
        // one owned blob: entries, names, then the two displacement arrays - every section 8-aligned
        size_t entriesBytes = builtEntries.size() * sizeof(Entry);
        size_t namesBytes = builtNames.size() * sizeof(NameSlot);
        size_t mnhDispBytes = (mnhDisp.size() * sizeof(uint32_t) + 7) & ~size_t(7);
        size_t nameDispBytes = (nameDisp.size() * sizeof(uint32_t) + 7) & ~size_t(7);
        auto bytes = (uint8_t *) das_aligned_alloc16(entriesBytes + namesBytes + mnhDispBytes + nameDispBytes);
        auto blobEntries = (Entry *) bytes;
        auto blobNames = (NameSlot *) (bytes + entriesBytes);
        auto blobMnhDisp = (uint32_t *) (bytes + entriesBytes + namesBytes);
        auto blobNameDisp = (uint32_t *) (bytes + entriesBytes + namesBytes + mnhDispBytes);
        memcpy(blobEntries, builtEntries.data(), entriesBytes);
        memcpy(blobNames, builtNames.data(), namesBytes);
        memcpy(blobMnhDisp, mnhDisp.data(), mnhDisp.size() * sizeof(uint32_t));
        memcpy(blobNameDisp, nameDisp.data(), nameDisp.size() * sizeof(uint32_t));
        blob = bytes;
        entries = blobEntries;
        names = blobNames;
        byMnh.disp = blobMnhDisp;
        byName.disp = blobNameDisp;
        count = uint32_t(staged.size());
        vector<Staged>().swap(staged);
        vector<char>().swap(nameBytes);
        sealed = true;
        return true;
    }

}
