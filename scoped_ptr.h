/* -----------------------------------------------------------------------------
 * Copyright (C) 2012 Jakob Praher
 *
 * Distributed under the terms of the BSD License. The full license is in
 * the file COPYING, distributed as part of this software.
 * -----------------------------------------------------------------------------
 */

#ifndef __scoped_ptr__
#define __scoped_ptr__


template <typename T>
void delete_op(T * p) {
    delete p;
}

template<typename T>
struct delete_trait {
    void operator()(T*& t);
};

template<typename T>
inline
void delete_trait<T>::operator()(T*& t) {
    delete_op(t);
    t = NULL;
}

template<typename T>
T* alloc_init() throw () /*nothrow*/  {
    T* p = (T*)malloc(sizeof(T));
    if (!p) return p;
    memset(p, 0, sizeof(T));
    return p;
}

template<typename T, typename D = delete_trait<T> >
struct scoped_ptr {
    explicit scoped_ptr(T* p= 0);
    ~scoped_ptr();

    void _delete();

    T& operator* () const;
    T* operator-> () const;
    T* get() const ;

    bool operator == (T* p) const;
    bool operator != (T* p) const;

    void swap(scoped_ptr& p2);
    void reset(T* p = NULL);
    T * release() ;

private:
    template<typename T2, typename D2>
    scoped_ptr(const scoped_ptr<T2,D2> & other);

    template <typename T2, typename D2> bool operator == (scoped_ptr<T2,D2> const & p2 ) const;
    template <typename T2, typename D2> bool operator != (scoped_ptr<T2,D2> const & p2 ) const;


    T* _ptr;
    D _del;
};



template<typename T, typename D>
inline
scoped_ptr<T, D>::scoped_ptr(T * p)
    : _ptr(p)
{
}

template<typename T, typename D>
inline
scoped_ptr<T, D>::~scoped_ptr()
{
    _delete();
}

template<typename T, typename D>
inline
void scoped_ptr<T, D>::_delete()
{
    _del(_ptr);
}

template<typename T, typename D>
inline
void scoped_ptr<T, D>::swap(scoped_ptr<T, D>& p)
{
    T * ptr = _ptr;
    _ptr = p._ptr;
    p._ptr = ptr;
}

template<typename T, typename D>
inline
void scoped_ptr<T, D>::reset(T* p)
{
    _delete();
    _ptr = p;
}

template<typename T, typename D>
inline
T*  scoped_ptr<T, D>::release()
{
    T* ptr = _ptr;
    _ptr = NULL;
    return ptr;
}

template<typename T, typename D>
inline
T& scoped_ptr<T, D>::operator* () const
{
    assert(_ptr != NULL);
    return *_ptr;
}

template<typename T, typename D>
inline
T* scoped_ptr<T, D>::operator-> () const
{
    assert(_ptr != NULL);
    return _ptr;
}

template<typename T, typename D>
inline
T* scoped_ptr<T, D>::get() const
{
    return _ptr;
}




// template<class T> void swap(scoped_ptr<T> & a, scoped_ptr<T> & b); // never throws


#endif
