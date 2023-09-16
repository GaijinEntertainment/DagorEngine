#pragma once

#include "daScript/misc/fnv.h"

namespace das {

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
    protected:
        safebox_map<ValueType>           objects;
        vector<ValueType>                objectsInOrder;
        vector<pair<uint64_t,ValueType>> postponed;
        bool                             locked = false;
    };
}
