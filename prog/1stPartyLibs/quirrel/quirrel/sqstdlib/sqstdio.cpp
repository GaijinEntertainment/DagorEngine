/* see copyright notice in squirrel.h */
#include <new>
#include <stdio.h>
#include <squirrel.h>
#include <sqstdio.h>
#include <assert.h>
#include "sqstdstream.h"

#define SQSTD_FILE_TYPE_TAG ((SQUnsignedInteger)(SQSTD_STREAM_TYPE_TAG | 0x00000001))
//basic API
SQFILE sqstd_fopen(const char *filename ,const char *mode)
{
    return (SQFILE)fopen(filename,mode);
}

SQInteger sqstd_fread(void* buffer, SQInteger size, SQInteger count, SQFILE file)
{
    SQInteger ret = (SQInteger)fread(buffer,size,count,(FILE *)file);
    return ret;
}

SQInteger sqstd_fwrite(const SQUserPointer buffer, SQInteger size, SQInteger count, SQFILE file)
{
    return (SQInteger)fwrite(buffer,size,count,(FILE *)file);
}

SQInteger sqstd_fseek(SQFILE file, SQInteger offset, SQInteger origin)
{
    SQInteger realorigin;
    switch(origin) {
        case SQ_SEEK_CUR: realorigin = SEEK_CUR; break;
        case SQ_SEEK_END: realorigin = SEEK_END; break;
        case SQ_SEEK_SET: realorigin = SEEK_SET; break;
        default: return -1; //failed
    }
    return fseek((FILE *)file,(long)offset,(int)realorigin);
}

SQInteger sqstd_ftell(SQFILE file)
{
    return ftell((FILE *)file);
}

SQInteger sqstd_fflush(SQFILE file)
{
    return fflush((FILE *)file);
}

SQInteger sqstd_fclose(SQFILE file)
{
    return fclose((FILE *)file);
}

SQInteger sqstd_feof(SQFILE file)
{
    return feof((FILE *)file);
}

//File
struct SQFile : public SQStream {
    SQFile(SQAllocContext alloc_ctx)
        : _alloc_ctx(alloc_ctx)
        , _handle(nullptr)
        , _owns(false)
    {}

    SQFile(SQAllocContext alloc_ctx, SQFILE file, bool owns)
        : _alloc_ctx(alloc_ctx)
        , _handle(file)
        , _owns(owns)
    {}

    virtual ~SQFile() { Close(); }

    bool Open(const char *filename ,const char *mode) {
        Close();
        if( (_handle = sqstd_fopen(filename,mode)) ) {
            _owns = true;
            return true;
        }
        return false;
    }

    void Close() {
        if(_handle && _owns) {
            sqstd_fclose(_handle);
            _handle = NULL;
            _owns = false;
        }
    }

    SQInteger Read(void *buffer,SQInteger size) override {
        return sqstd_fread(buffer,1,size,_handle);
    }

    SQInteger Write(const void *buffer,SQInteger size) override {
        return sqstd_fwrite(const_cast<void*>(buffer), 1, size, _handle);
    }

    SQInteger Flush() override {
        return sqstd_fflush(_handle);
    }

    SQInteger Tell() override {
        return sqstd_ftell(_handle);
    }

    SQInteger Len() override {
        SQInteger prevpos=Tell();
        Seek(0,SQ_SEEK_END);
        SQInteger size=Tell();
        Seek(prevpos,SQ_SEEK_SET);
        return size;
    }

    SQInteger Seek(SQInteger offset, SQInteger origin) override {
        return sqstd_fseek(_handle,offset,origin);
    }

    bool IsValid() override { return _handle?true:false; }
    bool EOS() override { return Tell()==Len()?true:false;}
    SQFILE GetHandle() {return _handle;}

public:
    SQAllocContext _alloc_ctx;

private:
    SQFILE _handle;
    bool _owns;
};

static SQInteger _file__typeof(HSQUIRRELVM v)
{
    sq_pushstring(v,"file",-1);
    return 1;
}

static SQInteger _file_releasehook(HSQUIRRELVM SQ_UNUSED_ARG(vm), SQUserPointer p, SQInteger SQ_UNUSED_ARG(size))
{
    SQFile *self = (SQFile*)p;
    SQAllocContext alloc_ctx = self->_alloc_ctx;
    self->~SQFile();
    sq_free(alloc_ctx, self,sizeof(SQFile));
    return 1;
}

static bool _file_is_valid_mode(const char *mode)
{
    static const char *ok[] = {
        "r", "w", "a", "r+", "w+", "a+",
        "rb", "wb", "ab", "rt", "wt", "at",
        "r+b", "w+b", "a+b", "rb+", "wb+", "ab+",
        "r+t", "w+t", "a+t", "rt+", "wt+", "at+", nullptr };
    if (!mode)
        return false;
    for (int i = 0; ok[i]; ++i)
        if (!strcmp(mode, ok[i]))
            return true;
    return false;
}

static SQInteger _file_constructor(HSQUIRRELVM v)
{
    const char *filename,*mode;
    bool owns = true;
    SQFile *f;
    SQFILE newf;
    if(sq_gettype(v,2) == OT_STRING && sq_gettype(v,3) == OT_STRING) {
        sq_getstring(v, 2, &filename);
        sq_getstring(v, 3, &mode);
        if(!_file_is_valid_mode(mode))
            return sq_throwerror(v, "invalid file mode");
        newf = sqstd_fopen(filename, mode);
        if(!newf) return sq_throwerror(v, "cannot open file");
    } else if(sq_gettype(v,2) == OT_USERPOINTER) {
        owns = !(sq_gettype(v,3) == OT_NULL);
        sq_getuserpointer(v,2,&newf);
    } else {
        return sq_throwerror(v,"wrong parameter");
    }

    SQAllocContext alloc_ctx = sq_getallocctx(v);

    f = new (sq_malloc(alloc_ctx, sizeof(SQFile)))SQFile(alloc_ctx,newf,owns);
    if(SQ_FAILED(sq_setinstanceup(v,1,f))) {
        f->~SQFile();
        sq_free(alloc_ctx,f,sizeof(SQFile));
        return sq_throwerror(v, "cannot create blob with negative size");
    }
    sq_setreleasehook(v,1,_file_releasehook);
    return 0;
}

static SQInteger _file_close(HSQUIRRELVM v)
{
    SQFile *self = NULL;
    if(SQ_SUCCEEDED(sq_getinstanceup(v,1,(SQUserPointer*)&self,(SQUserPointer)SQSTD_FILE_TYPE_TAG))
        && self != NULL)
    {
        self->Close();
    }
    return 0;
}

static const SQRegFunctionFromStr _file_methods[] = {
    { _file_constructor, "constructor(path: string|userpointer, mode: string|int|null): instance",
      "Two forms: (path: string, mode: string) opens the file via fopen with the given mode; "
      "(handle: userpointer, own) wraps an existing FILE* and takes ownership (closes on destruction) "
      "when `own` is non-null, or shares it without closing when `own` is null" },

    { _file__typeof, "instance._typeof(): string", "Returns 'file'" },
    { _file_close,   "instance.close()", "Closes the file if it is still open" },
    { NULL, NULL, NULL }
};



SQRESULT sqstd_createfile(HSQUIRRELVM v, SQFILE file,SQBool own)
{
    SQInteger top = sq_gettop(v);
    sq_pushregistrytable(v);
    sq_pushstring(v,"std_file",-1);
    if(SQ_SUCCEEDED(sq_get(v,-2))) {
        sq_remove(v,-2); //removes the registry
        sq_pushroottable(v); // push the this
        sq_pushuserpointer(v,file); //file
        if(own){
            sq_pushinteger(v,1); //true
        }
        else{
            sq_pushnull(v); //false
        }
        if(SQ_SUCCEEDED( sq_call(v,3,SQTrue,SQFalse) )) {
            sq_remove(v,-2);
            return SQ_OK;
        }
    }
    else
        assert(0);
    sq_settop(v,top);
    return SQ_ERROR;
}

SQRESULT sqstd_getfile(HSQUIRRELVM v, SQInteger idx, SQFILE *file)
{
    SQFile *fileobj = NULL;
    if(SQ_SUCCEEDED(sq_getinstanceup(v,idx,(SQUserPointer*)&fileobj,(SQUserPointer)SQSTD_FILE_TYPE_TAG))) {
        *file = fileobj->GetHandle();
        return SQ_OK;
    }
    return sq_throwerror(v,"not a file");
}



#define IO_BUFFER_SIZE 2048
struct IOBuffer {
    unsigned char buffer[IO_BUFFER_SIZE];
    SQInteger size;
    SQInteger ptr;
    SQFILE file;
};

SQInteger _read_byte(IOBuffer *iobuffer)
{
    if(iobuffer->ptr < iobuffer->size) {

        SQInteger ret = iobuffer->buffer[iobuffer->ptr];
        iobuffer->ptr++;
        return ret;
    }
    else {
        if( (iobuffer->size = sqstd_fread(iobuffer->buffer,1,IO_BUFFER_SIZE,iobuffer->file )) > 0 )
        {
            SQInteger ret = iobuffer->buffer[0];
            iobuffer->ptr = 1;
            return ret;
        }
    }

    return 0;
}

SQInteger _read_two_bytes(IOBuffer *iobuffer)
{
    if(iobuffer->ptr < iobuffer->size) {
        if(iobuffer->size < 2) return 0;
        SQInteger ret = *((const wchar_t*)&iobuffer->buffer[iobuffer->ptr]);
        iobuffer->ptr += 2;
        return ret;
    }
    else {
        if( (iobuffer->size = sqstd_fread(iobuffer->buffer,1,IO_BUFFER_SIZE,iobuffer->file )) > 0 )
        {
            if(iobuffer->size < 2) return 0;
            SQInteger ret = *((const wchar_t*)&iobuffer->buffer[0]);
            iobuffer->ptr = 2;
            return ret;
        }
    }

    return 0;
}


SQInteger file_read(SQUserPointer file,SQUserPointer buf,SQInteger size)
{
    SQInteger ret;
    if( ( ret = sqstd_fread(buf,1,size,(SQFILE)file ))!=0 )return ret;
    return -1;
}

SQInteger file_write(SQUserPointer file,SQUserPointer p,SQInteger size)
{
    return sqstd_fwrite(p,1,size,(SQFILE)file);
}

static char *readFileData(HSQUIRRELVM v, SQFILE f, SQInteger &s) {
    sqstd_fseek(f, 0, SQ_SEEK_SET);
    SQInteger size = sqstd_fseek(f, 0, SQ_SEEK_END);
    char *buffer = (char *)sq_malloc(sq_getallocctx(v), size * sizeof(char));

    sqstd_fseek(f, 0, SQ_SEEK_SET);
    s = sqstd_fread(buffer, sizeof(char), size, f);

    return buffer;
}

SQRESULT sqstd_loadfile(HSQUIRRELVM v,const char *filename,SQBool printerror)
{
    SQFILE file = sqstd_fopen(filename,"rb");

    if(file){
        SQInteger size = 0;
        char *buffer = readFileData(v, file, size);
        SQRESULT r = SQ_ERROR;

        if(SQ_SUCCEEDED(sq_compile(v,buffer,size,filename,printerror))){
            r = SQ_OK;
        }
        sq_free(sq_getallocctx(v), buffer, size);
        sqstd_fclose(file);
        return r;
    }
    return sq_throwerror(v,"cannot open the file");
}

SQRESULT sqstd_dofile(HSQUIRRELVM v,const char *filename,SQBool retval,SQBool printerror)
{
    //at least one entry must exist in order for us to push it as the environment
    if(sq_gettop(v) == 0)
        return sq_throwerror(v,"environment table expected");

    if(SQ_SUCCEEDED(sqstd_loadfile(v,filename,printerror))) {
        sq_push(v,-2);
        if(SQ_SUCCEEDED(sq_call(v,1,retval,SQTrue))) {
            sq_remove(v,retval?-2:-1); //removes the closure
            return 1;
        }
        sq_pop(v,1); //removes the closure
    }
    return SQ_ERROR;
}

SQRESULT sqstd_writeclosuretofile(HSQUIRRELVM v,const char *filename)
{
    SQFILE file = sqstd_fopen(filename,"wb+");
    if(!file) return sq_throwerror(v,"cannot open the file");
    if(SQ_SUCCEEDED(sq_writeclosure(v,file_write,file))) {
        sqstd_fclose(file);
        return SQ_OK;
    }
    sqstd_fclose(file);
    return SQ_ERROR; //forward the error
}

static const SQRegFunctionFromStr iolib_funcs[] = {
    { NULL, NULL, NULL }
};

SQRESULT sqstd_register_iolib(HSQUIRRELVM v)
{
    SQInteger top = sq_gettop(v);
    //create delegate
    declare_stream(v,"file",(SQUserPointer)SQSTD_FILE_TYPE_TAG,"std_file",_file_methods,iolib_funcs);
    sq_pushstring(v,"stdout",-1);
    sqstd_createfile(v,stdout,SQFalse);
    sq_newslot(v,-3,SQFalse);
    sq_pushstring(v,"stdin",-1);
    sqstd_createfile(v,stdin,SQFalse);
    sq_newslot(v,-3,SQFalse);
    sq_pushstring(v,"stderr",-1);
    sqstd_createfile(v,stderr,SQFalse);
    sq_newslot(v,-3,SQFalse);
    sq_settop(v,top);
    return SQ_OK;
}
