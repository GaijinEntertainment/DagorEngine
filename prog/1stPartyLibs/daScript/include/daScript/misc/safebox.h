#pragma once

#include "daScript/misc/anyhash.h"

namespace das {

    struct fragile_bit_set {    // key==hash, value = top bit of hash
    public:
        static constexpr uint64_t top_bit = 0x8000000000000000ull;
        static constexpr uint64_t top_bit_mask = 0x7fffffffffffffffull;
        static __forceinline uint64_t key ( uint64_t hash ) { return hash & top_bit_mask; } // up to user to call 'key'
        static __forceinline uint64_t set_true ( uint64_t hash ) { return hash | top_bit; }
        static __forceinline uint64_t set_false ( uint64_t hash ) { return hash & top_bit_mask; }
        static __forceinline bool is_true ( uint64_t hash ) { return hash & top_bit; }
    public:
        fragile_bit_set() {
            mask = 63;
        }
        ~fragile_bit_set() {
            if ( objects ) delete [] objects;
        }
        void clear () {
            if ( objects ) {
                memset(objects,0,(mask+1)*sizeof(uint64_t));
                occupancy = 0;
            }
        }
        __forceinline uint64_t *  find_and_reserve ( uint64_t key ) { // its up to user to write both hash and value
            if ( !objects ) allocate();
            uint64_t index = key & mask;
            for ( ;; ) {
                auto hash = objects[index];
                if ( hash==0 ) {
                    if ( occupancy*2 > mask ) {
                        rehash(mask*2 + 1);
                        index = key & mask;
                        continue;
                    }
                    occupancy ++;
                    return objects + index;
                } else if ( (hash & top_bit_mask)==key ) {
                    return objects + index;
                }
                index = (index + 1) & mask;
            }
            return nullptr;
        }
    protected:
        void allocate() {
            objects = new uint64_t[mask+1];
            memset(objects,0,(mask+1)*sizeof(uint64_t));
        }
        void rehash ( uint32_t newMask ) {
            auto old_objects = objects;
            auto old_mask = mask;
            mask = newMask;
            allocate();
            if ( old_objects ) {
                for ( uint32_t i=0; i<=old_mask; ++i ) {
                    if ( auto hash = old_objects[i] ) {
                        uint32_t index = hash & mask;
                        for ( ;; ) {
                            auto & new_kv = objects[index];
                            if ( new_kv == 0 ) {
                                new_kv = hash;
                                break;
                            }
                            index = (index + 1) & mask;
                        }
                    }
                }
                delete [] old_objects;
            }
        }
    protected:
        uint64_t *  objects = nullptr;
        uint32_t    mask = 0;
        uint32_t    occupancy = 0;
    };


    template <typename ValueType>
    struct fragile_hash {    // key==hash, no delete, lookup only
    public:
        struct KV {
            uint64_t    hash = 0;
            ValueType   second;
            __forceinline bool found() const { return hash!=0; }
        };
    public:
        fragile_hash() {
            mask = 63;
        }
        ~fragile_hash() {
            if ( objects ) delete [] objects;
        }
        void clear () {
            if ( objects ) {
                for ( uint32_t i=0; i<=mask; ++i ) {
                    if ( objects[i].hash != 0 ) {
                        objects[i].hash = 0;
                        objects[i].second = ValueType();
                    }
                }
                occupancy = 0;
            }
        }
        __forceinline KV * find ( uint64_t key ) const {
            if ( !objects ) return nullptr;
            uint64_t index = key & mask;
            for ( ;; ) {
                auto hash = objects[index].hash;
                if ( hash==0 ) {
                    return nullptr;
                } else if ( hash==key ) {
                    return objects + index;
                }
                index = (index + 1) & mask;
            }
            return nullptr;
        }
        __forceinline ValueType & operator [] ( uint64_t key ) {
            if ( !objects ) allocate();
            uint64_t index = key & mask;
            for ( ;; ) {
                auto hash = objects[index].hash;
                if ( hash==0 ) {
                    if ( occupancy*2 > mask ) {
                        rehash(mask*2 + 1);
                        index = key & mask;
                        continue;
                    }
                    occupancy ++;
                    objects[index].hash = key;
                    return objects[index].second;
                } else if ( hash==key ) {
                    return objects[index].second;
                }
                index = (index + 1) & mask;
            }
            return objects[0].second;   // never happens
        }
        template <typename BT>
        __forceinline void foreach ( BT && block ) {
            if ( objects ) {
                for ( uint32_t i=0; i<=mask; ++i ) {
                    auto & kv = objects[i];
                    if ( kv.hash ) {
                        block(kv.hash, kv.second);
                    }
                }
            }
        }
        __forceinline void reserve ( uint32_t size ) {
            DAS_VERIFYF(!(size & (size-1)), "only power of 2 sizes are supported, and not %i", int(size));
            if ( size > (mask+1) ) {
                rehash(size-1);
            }
        }
        __forceinline uint32_t size() const {
            return occupancy;
        }
        __forceinline uint32_t capacity() const {
            return mask + 1;
        }
    protected:
        void allocate() {
            objects = new KV[mask+1];
        }
        void rehash ( uint32_t newMask ) {
            auto old_objects = objects;
            auto old_mask = mask;
            mask = newMask;
            allocate();
            if ( old_objects ) {
                for ( uint32_t i=0; i<=old_mask; ++i ) {
                    auto & kv = old_objects[i];
                    if ( kv.hash ) {
                        uint32_t index = kv.hash & mask;
                        for ( ;; ) {
                            auto & new_kv = objects[index];
                            if ( new_kv.hash==0 ) {
                                new_kv = das::move(kv);
                                break;
                            }
                            index = (index + 1) & mask;
                        }
                    }
                }
                delete [] old_objects;
            }
        }
        void verify() {
            uint32_t count = 0;
            this->foreach([&](uint64_t,const ValueType &){
                count++;
            });
            DAS_VERIFYF(count==occupancy, "count mismatch");
        }
    protected:
        KV *        objects = nullptr;
        uint32_t    mask = 0;
        uint32_t    occupancy = 0;
    };

    struct skip_hash {
        uint64_t operator() ( uint64_t key ) const {
            return key;
        };
    };

    __forceinline uint64_t hash64z ( const char * str ) {
        return hash_blockz64((const uint8_t *) (str ? str : ""));
    }

    template <typename V>
    using safebox_map = das_hash_map<uint64_t,V,skip_hash,das::equal_to<uint64_t>>;

    template <typename DataType>
    struct safebox {
    public:
        typedef smart_ptr<DataType> ValueType;
        typedef safebox<ValueType> this_type;
    public:
        safebox() {}
        safebox( this_type && sb ) {
            objects = das::move(sb.objects);
            objectsInOrder = das::move(sb.objectsInOrder);
            postponed = das::move(sb.postponed);
        }
        safebox ( const this_type & sb ) {
            objects = sb.objects;
            objectsInOrder = sb.objectsInOrder;
            postponed = sb.postponed;
        }
        this_type & operator = ( this_type && sb ) {
            objects = das::move(sb.objects);
            objectsInOrder = das::move(sb.objectsInOrder);
            postponed = das::move(sb.postponed);
            return * this;
        }
        this_type & operator = ( const this_type & sb ) {
            objects = sb.objects;
            objectsInOrder = sb.objectsInOrder;
            postponed = sb.postponed;
            return * this;
        }
        void clear() {
            DAS_ASSERT(!locked);
            objects.clear();
            objectsInOrder.clear();
            postponed.clear();
        }
        void tryResolve() {
            if ( !locked ) {
                // note - the reason for the 'neworder', as oppose to regular push_back into objectsInOrder
                //  is that there is an order dependency for compiling things like templates, where otherwise
                //  it would require us to actually check, if the template aliasing has been addressed
                //  before switching to template`
                vector<ValueType> neworder;
                neworder.reserve(postponed.size() + objectsInOrder.size());
                for ( auto & p : postponed ) {
                    neworder.push_back(p.second);
                    objects[p.first] = p.second;
                }
                for ( auto & p : objectsInOrder ) {
                    neworder.push_back(p);
                }
                postponed.clear();
                swap(objectsInOrder, neworder);
            }
        }
        template <typename TT>
        __forceinline void foreach ( TT && closure ) {
            auto saveLock = locked;
            locked = true;
            for ( auto & obj : objectsInOrder ) {
                closure(obj);
            }
            locked = saveLock;
            tryResolve();
        }
        template <typename TT>
        __forceinline void foreach_with_hash ( TT && closure ) {
            auto saveLock = locked;
            locked = true;
            for ( auto & obj : objects ) {
                closure(obj.second, obj.first);
            }
            locked = saveLock;
            tryResolve();
        }
        template <typename TT>
        __forceinline void find_first ( TT && closure ) {
            auto saveLock = locked;
            locked = true;
            for ( auto & obj : objectsInOrder ) {
                if ( closure(obj) ) break;
            }
            locked = saveLock;
            tryResolve();
        }
        bool insert ( const string & key, const ValueType & value ) {
            return insert(hash64z(key.c_str()),value);
        }
        bool insert ( uint64_t key, const ValueType & value ) {
            if ( locked ) {
                if ( objects.find(key)!=objects.end() ) {
                    return false;
                }
                for ( auto & pp : postponed ) {
                    if ( pp.first==key ) {
                        return false;
                    }
                }
                postponed.push_back(make_pair(key,value));
                return true;
            } else {
                if ( objects.insert(make_pair(key,value)).second ) {
                    objectsInOrder.push_back(value);
                    return true;
                } else {
                    return false;
                }
            }
        }
        void replace ( const string & key, const ValueType & value ) {
            return replace(hash64z(key.c_str()),value);
        }
        void replace ( uint64_t key, const ValueType & value ) {
            auto it = objects.find(key);
            DAS_ASSERT(it!=objects.end());
            it->second = value;
        }
        ValueType find ( const string & key ) const {
            return find(hash64z(key.c_str()));
        }
        ValueType find ( uint64_t key ) const {
            auto it = objects.find(key);
            if ( it!=objects.end() ) {
                return it->second;
             } else {
                for ( auto & kv : postponed ) {
                    if ( kv.first==key ) {
                        return kv.second;
                    }
                }
                return ValueType();
             }
        }
        bool remove ( const string & key ) {
            return remove(hash64z(key.c_str()));
        }
        bool remove ( uint64_t key ) {
            DAS_ASSERT(!locked);
            auto it = objects.find(key);
            if ( it != objects.end() ) {
                auto pObj = it->second;
                objects.erase(it);
                auto ito = das::find ( objectsInOrder.begin(), objectsInOrder.end(), pObj );
                DAS_ASSERT(ito != objectsInOrder.end() );
                objectsInOrder.erase(ito);
                return true;
            } else {
                return false;
            }
        }
        __forceinline const vector<ValueType> & each () const { return objectsInOrder; }
        size_t unlocked_size () const {
            DAS_VERIFYF(locked == false, "expected to see an unlocked box");
            return objectsInOrder.size();
        }
        bool refresh_key ( uint64_t oldKey, uint64_t newKey ) {
            DAS_ASSERT(!locked);
            auto it = objects.find(oldKey);
            if ( it != objects.end() ) {
                auto pObj = it->second;
                objects.erase(it);
                objects[newKey] = pObj;
                return true;
            } else {
                return false;
            }
        }
    protected:
        safebox_map<ValueType>           objects;
        vector<ValueType>                objectsInOrder;
        vector<pair<uint64_t,ValueType>> postponed;
        bool                             locked = false;
    };
}
