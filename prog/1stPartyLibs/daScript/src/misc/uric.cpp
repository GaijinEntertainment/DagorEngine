#include "daScript/misc/platform.h"

#include "daScript/misc/uric.h"


namespace das {

    Uri::Uri(UriUriA && uriA) {
        uri = uriA;
        memset(&uriA, 0, sizeof(UriUriA));
        isEmpty = false;
        lastOp = 0;
    }

    Uri::Uri() {
        memset(&uri, 0, sizeof(UriUriA));
        isEmpty = true;
    }

    Uri::Uri(const char * uriString) {
        memset(&uri, 0, sizeof(UriUriA));
        parse(uriString);
    }
    Uri::Uri(const string & uriS) {
        memset(&uri, 0, sizeof(UriUriA));
        parse(uriS.c_str());
    }

    Uri::Uri(const Uri & uriA) {
        memset(&uri, 0, sizeof(UriUriA));
        clone(uriA);
    }

    Uri::Uri(Uri && uriA) {
        memset(&uri, 0, sizeof(UriUriA));
        if ( !uriA.isEmpty ) {
            memcpy(&uri,&uriA.uri,sizeof(UriUriA));
            uriA.isEmpty = true;
            lastOp = uriA.lastOp;
            errorPos = uriA.errorPos;
            isEmpty = false;
        }
    }

    Uri & Uri::operator = ( const Uri & uriA ) {
        reset();
        clone(uriA);
        return *this;
    }

    Uri & Uri::operator = ( Uri && uriA ) {
        reset();
        if ( !uriA.isEmpty ) {
            memcpy(&uri,&uriA.uri,sizeof(UriUriA));
            uriA.isEmpty = true;
            lastOp = uriA.lastOp;
            errorPos = uriA.errorPos;
            isEmpty = false;
        }
        return * this;
    }

    Uri & Uri::operator = ( const char * uriString ) {
        reset();
        parse(uriString);
        return *this;
    }

    Uri & Uri::operator = ( const string & uriS ) {
        reset();
        parse(uriS.c_str());
        return *this;
    }

    Uri::~Uri() {
        reset();
    }

    Uri Uri::addBaseUri ( const Uri & base ) const {
        UriUriA absoluteDest;
        auto lOp = uriAddBaseUriA(&absoluteDest, &uri, &base.uri);
        if ( lOp != URI_SUCCESS ) {
            uriFreeUriMembersA(&absoluteDest);
            Uri failed;
            failed.lastOp = lOp;
            return failed;
        }
        return Uri(das::move(absoluteDest));
    }

    Uri Uri::strip(bool query, bool fragment) const {
        UriUriA saveUri = uri;
        if ( query ) {
            uri.query.first = nullptr;
            uri.query.afterLast = nullptr;
        }
        if ( fragment ) {
            uri.fragment.first = nullptr;
            uri.fragment.afterLast = nullptr;
        }
        auto text = str();
        uri = saveUri;
        return Uri(text);
    }

    Uri Uri::removeBaseUri ( const Uri & base ) const {
        UriUriA dest;
        auto lOp = uriRemoveBaseUriA(&dest, &uri, &base.uri, URI_FALSE);
        if ( lOp != URI_SUCCESS ) {
            uriFreeUriMembersA(&dest);
            Uri failed;
            failed.lastOp = lOp;
            return failed;
        }
        return Uri(das::move(dest));
    }

    bool Uri::normalize() {
        const unsigned int dirtyParts = uriNormalizeSyntaxMaskRequiredA(&uri);
        lastOp = uriNormalizeSyntaxExA(&uri, dirtyParts);
        return lastOp == URI_SUCCESS;
    }

    void Uri::reset() {
        if ( !isEmpty ) {
            uriFreeUriMembersA(&uri);
            isEmpty = true;
        }
        errorPos = 0;
        lastOp = URI_SUCCESS;
    }

    bool Uri::parse ( const char * uriString ) {
        if ( uriString==nullptr ) uriString = "";
        reset();
        lastOp = uriParseSingleUriA(&uri, uriString, &errorPos);
        if ( lastOp != URI_SUCCESS ) return false;
        uriMakeOwnerA(&uri);
        isEmpty = false;
        lastOp = URI_SUCCESS;
        return true;
    }

    void Uri::clone ( const Uri & uriS ) {
        reset();
        if ( !uriS.isEmpty ) parse(uriS.str().c_str());
    }

    int Uri::size() const {
        if ( isEmpty ) return 0;
        int charsRequired;
        lastOp = uriToStringCharsRequiredA(&uri, &charsRequired);
        return (lastOp != URI_SUCCESS) ? 0 : charsRequired;
    }

    string Uri::str() const {
        int charsRequired = size();
        if (lastOp != URI_SUCCESS) return "";
        charsRequired++;
        vector<char> result;
        result.resize(charsRequired);
        lastOp = uriToStringA(result.data(), &uri, charsRequired, nullptr);
        return result.data();
    }

    string Uri::toUnixFileName() const {
        auto uriS = str();
        vector<char> result;
        result.resize(uriS.length()+1);
        lastOp = uriUriStringToUnixFilenameA(uriS.c_str(), result.data());
        return result.data();
    }

    string Uri::toWindowsFileName() const {
        auto uriS = str();
        vector<char> result;
        result.resize(uriS.length()+1);
        lastOp = uriUriStringToWindowsFilenameA(uriS.c_str(), result.data());
        return result.data();
    }

    string Uri::toFileName() const {
        #ifdef _WIN32
            return toWindowsFileName();
        #else
            return toUnixFileName();
        #endif
    }

    bool Uri::fromUnixFileNameStr ( const char * fileName, int len ) {
        vector<char> result;
        if ( len==-1 ) len = int(strlen(fileName));
        result.resize(3*len + 1);
        lastOp = uriUnixFilenameToUriStringA(fileName, result.data());
        if ( lastOp != URI_SUCCESS) return false;
        return parse(result.data());
    }

    bool Uri::fromWindowsFileNameStr ( const char * fileName, int len ) {
        vector<char> result;
        if ( len==-1 ) len = int(strlen(fileName));
        result.resize(8 + 3*len + 1);
        lastOp = uriWindowsFilenameToUriStringA(fileName, result.data());
        if ( lastOp != URI_SUCCESS) return false;
        return parse(result.data());
    }

    bool Uri::fromFileNameStr ( const char * fileName, int len ) {
        #ifdef _WIN32
            return fromWindowsFileNameStr(fileName,len);
        #else
            return fromUnixFileNameStr(fileName,len);
        #endif
    }

    bool Uri::fromUnixFileName ( const string & fileName ) {
        return fromUnixFileNameStr(fileName.c_str(),int(fileName.length()));
    }

    bool Uri::fromWindowsFileName ( const string & fileName ) {
        return fromWindowsFileNameStr(fileName.c_str(),int(fileName.length()));
    }

    bool Uri::fromFileName ( const string & fileName ) {
        return fromFileNameStr(fileName.c_str(),int(fileName.length()));
    }

    vector<pair<string,string>> Uri::query() const {
        vector<pair<string,string>> res;
        if ( !isEmpty ) {
            UriQueryListA * queryList;
            int itemCount;
            lastOp = uriDissectQueryMallocA(&queryList, &itemCount, uri.query.first,uri.query.afterLast);
            if ( lastOp == URI_SUCCESS ) {
                for ( auto q=queryList; q!=nullptr; q=q->next ) {
                    res.push_back(make_pair<string,string>(q->key ? q->key : "",q->value ? q->value : ""));
                }
            }
            uriFreeQueryListA(queryList);
        }
        return res;
    }
}
