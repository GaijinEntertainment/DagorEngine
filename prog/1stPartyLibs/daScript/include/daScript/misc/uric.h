#pragma once

#ifndef URI_STATIC_BUILD
#define URI_STATIC_BUILD
#endif
#include "uriparser/Uri.h"

namespace das {

    class Uri {
    public:
        Uri();
        Uri(UriUriA && uriA);
        Uri(const char *);
        Uri(const string &);
        Uri(const Uri &);
        Uri(Uri &&);
        Uri & operator = ( const Uri & );
        Uri & operator = ( Uri && );
        Uri & operator = ( const char * );
        Uri & operator = ( const string & );
        ~Uri();
        Uri addBaseUri ( const Uri & base ) const;
        Uri removeBaseUri ( const Uri & base ) const;
        int status() const { return lastOp; }
        const char * getErrorPos() const { return errorPos; }
        bool parse ( const char * );
        void reset();
        string str() const;
        int size() const;
        bool empty() const { return isEmpty; }
        bool normalize();
        string toUnixFileName() const;
        string toWindowsFileName() const;
        string toFileName() const;
        bool fromUnixFileName ( const string & );
        bool fromWindowsFileName ( const string & );
        bool fromFileName ( const string & );
        bool fromUnixFileNameStr ( const char *, int len = -1 );
        bool fromWindowsFileNameStr ( const char *, int len = -1 );
        bool fromFileNameStr ( const char *, int len = -1 );
        vector<pair<string,string>> query() const;
        Uri strip(bool query = true, bool fragment = true) const;
    protected:
        void clone ( const Uri & uri );
    protected:
        const char* errorPos = nullptr;
        mutable int lastOp = 0;
        bool    isEmpty = true;
    public: // note - this is public so that we can bind it easier
        mutable UriUriA uri;
    };

}