#pragma once

namespace das
{
    template <typename EE>
    class Enum {
    public:
        struct InitEnum {
            EE          value;
            string      name;
        };
        Enum ( std::initializer_list<InitEnum> ie ) {
            name2enum.reserve(ie.size());
            enum2name.reserve(ie.size());
            for ( const auto & e : ie ) {
                name2enum[e.name] = e.value;
                enum2name[e.value] = e.name;
            }
        }
        EE find ( const string & name, EE fail ) const {
            auto it = name2enum.find(name);
            return it==name2enum.end() ? fail : it->second;
        }
        string find ( EE e ) const {
            auto it = enum2name.find(e);
            return it==enum2name.end() ? "" : it->second;
        }
        EE parse ( string::const_iterator & it, EE fail ) const {
            for ( auto & op : name2enum ) {
                auto & text = op.first;
                if ( equal(text.begin(), text.end(), it) ) {
                    it += text.length();
                    return op.second;
                }
            }
            return fail;
        }
    private:
        das_hash_map<string, EE>     name2enum;
        das_hash_map<EE, string>     enum2name;
    };
}
