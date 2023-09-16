/** \file   
    Convenient interfaces and wrappers  
    Rev 0.7
*/

#ifndef _BASES_H_
#define _BASES_H_

/** @name Bases */
//@{

#include <cstddef>
using std::size_t;
#include <iostream>
using std::ostream;


/** Reference counted class. I don't know what set of methods is the best,
    so there are two methods to add or remove a reference */
struct ref_counted {
    size_t _ref_count;

public:

    ref_counted(size_t ref_count = 0) : _ref_count(ref_count) {}

    /* Consider 
        class C : ref_counted { .. }; 
        C *a = ...;
        C *b = new C(*a); // reference count must be zero for *b
    */
    ref_counted(const ref_counted& another) : _ref_count(0) {}
    /* C *a = ..., *b = ...
        ....
        *a = *b; // reference count for *a should not change
    */
    ref_counted& operator=(const ref_counted& another) {
        return *this;
    }


protected:
    virtual ~ref_counted () {};

public:
    size_t ref_count() const { return _ref_count; }

    /* 1st interface */
    ///
    void add_ref() {
        ++_ref_count;
    }
    ///
    void release() {
        if (--_ref_count == 0)
            delete this;
    }

    /* 2nd interface */
    friend void add_reference(ref_counted *obj)     { obj->add_ref();  }
    friend void remove_reference(ref_counted *obj)  { obj->release();  }
    friend void add_possible_reference(ref_counted *obj) {
        if (obj) obj->add_ref();
    }
    friend void remove_possible_reference(ref_counted *obj) {
        if (obj) obj->release();
    }
};


template<class RefCounted>
struct ptr_to_ref_counted
{
protected:
    RefCounted *under;
public:
    ptr_to_ref_counted(RefCounted * under) : under(under) {
        add_possible_reference(under);
    }
    ptr_to_ref_counted(const ptr_to_ref_counted &another) {
        add_possible_reference(another.under);
        under = another.under;
    }
    ptr_to_ref_counted& operator=(const ptr_to_ref_counted &another) {
        add_possible_reference(another.under);
        remove_possible_reference(under);
        under = another.under;
        return *this;
    }
    ~ptr_to_ref_counted() {
        remove_possible_reference(under);
    }   

//  operator RefCounted&()  const   { return *under; }

    RefCounted &operator*()     const { return *under; }
    RefCounted *operator->()    const { return under;  }
    RefCounted *get()           const { return under;  }
};

template<class X>
struct ref_counted_ptr : ref_counted
{
    X *obj;
    explicit ref_counted_ptr(X * obj, size_t ref_count = 0)
        : obj(obj), ref_counted(ref_count) {}
private:
    ref_counted_ptr(const ref_counted_ptr &);
    ref_counted_ptr& operator=(const ref_counted_ptr &);
    ~ref_counted_ptr()  { delete obj; }     
};

template<class X>
struct counted_ptr : ptr_to_ref_counted< ref_counted_ptr<X> >
{
    typedef X element_type;

    typedef ptr_to_ref_counted< ref_counted_ptr<X> > base;
    typedef const ptr_to_ref_counted< ref_counted_ptr<X> >& cbase;
    typedef ref_counted_ptr<X> ptr;

    counted_ptr() : base(new ptr(0)) {}
    
//  explicit
    counted_ptr(X * obj) : base(new ptr(obj)) {}
    ~counted_ptr() {}

    X& operator*()  const { return *cbase(*this)->obj; }
    X *operator->() const { return cbase(*this)->obj;  }
    X *get()        const { return cbase(*this)->obj;  }

    operator bool() const { return get() != 0; }
};


struct rtti_on 
{
    virtual ~rtti_on() {;}
};


/** Base that provides convenient dynamic type cheching */
struct auto_cast : rtti_on
{
    ///
    template<class T>
    T &as() {
        return dynamic_cast<T &>(*this);
    }
    ///
    template<class T>
    const T &as() const {
        return dynamic_cast<const T &>(*this);
    }
};


/** Base that provides convenient dynamic cast */
struct auto_typeid : rtti_on
{
    ///
    template<class T>
    bool is() const {
        return (dynamic_cast<const T *>(this) != NULL);
    }
};

/** Combines auto_case and auto_typeid. Should be used instead of those two. */
struct auto_rtti : auto_typeid, auto_cast
{
};

/** Base that enforces virtual destructor */
struct virtual_destructor
{
    virtual ~virtual_destructor() {}
};

struct printable
{
    virtual void output(ostream &os) const = 0;
    friend ostream& operator<<(ostream& os, const printable& p)
    {
        p.output(os);
        return os;
    }
};

struct equalable
{
    virtual bool equal(const equalable &e1, const equalable &e2) const = 0;
    friend bool operator==(const equalable &e1, const equalable &e2)
    {
        if (typeid(e1) != typeid(e2)) return false; // What???
        return e1.equal(e1, e2);    
    }
};

//@}
#endif
