/*  see copyright notice in squirrel.h */
#pragma once

struct SQBlob : public SQStream
{
    SQBlob(SQAllocContext alloc_ctx, SQInteger size)
        : _alloc_ctx(alloc_ctx)
    {
        _size = size;
        _allocated = size;
        _buf = (unsigned char *)sq_malloc(_alloc_ctx, size);
        memset(_buf, 0, _size);
        _ptr = 0;
    }

    virtual ~SQBlob() {
        sq_free(_alloc_ctx, _buf, _allocated);
    }

    SQInteger Write(const void *buffer, SQInteger size) override {
        if(!CanAdvance(size)) {
            GrowBufOf(_ptr + size - _size);
        }
        memcpy(&_buf[_ptr], buffer, size);
        _ptr += size;
        return size;
    }

    SQInteger Read(void *buffer,SQInteger size) override {
        SQInteger n = size;
        if(!CanAdvance(size)) {
            if((_size - _ptr) > 0)
                n = _size - _ptr;
            else return 0;
        }
        memcpy(buffer, &_buf[_ptr], n);
        _ptr += n;
        return n;
    }

    bool Resize(SQInteger n) {
        if (n < 0)
            return false;

        if(n != _allocated) {
            unsigned char *newbuf = (unsigned char *)sq_malloc(_alloc_ctx, n);
            memset(newbuf,0,n);
            if(_size > n)
                memcpy(newbuf,_buf,n);
            else
                memcpy(newbuf,_buf,_size);
            sq_free(_alloc_ctx, _buf,_allocated);
            _buf=newbuf;
            _allocated = n;
            if(_size > _allocated)
                _size = _allocated;
            if(_ptr > _allocated)
                _ptr = _allocated;
        }
        return true;
    }

    void GrowBufOf(SQInteger n)
    {
        if(_size + n > _allocated) {
            if(_size + n > _size * 2)
                Resize(_size + n);
            else
                Resize(_size * 2);
        }
        _size = _size + n;
    }

    bool CanAdvance(SQInteger n) {
        if(_ptr+n>_size)return false;
        return true;
    }

    SQInteger Seek(SQInteger offset, SQInteger origin) override {
        switch(origin) {
            case SQ_SEEK_SET:
                if(offset > _size || offset < 0) return -1;
                _ptr = offset;
                break;
            case SQ_SEEK_CUR:
                if(_ptr + offset > _size || _ptr + offset < 0) return -1;
                _ptr += offset;
                break;
            case SQ_SEEK_END:
                if(_size + offset > _size || _size + offset < 0) return -1;
                _ptr = _size + offset;
                break;
            default: return -1;
        }
        return 0;
    }

    bool IsValid() override {
        return _size == 0 || _buf?true:false;
    }

    bool EOS() override {
        return _ptr == _size;
    }

    SQInteger Flush() override { return 0; }
    SQInteger Tell() override { return _ptr; }
    SQInteger Len() override { return _size; }
    SQUserPointer GetBuf(){ return _buf; }

public:
    SQAllocContext _alloc_ctx;

private:
    SQInteger _size;
    SQInteger _allocated;
    SQInteger _ptr;
    unsigned char *_buf;
};
