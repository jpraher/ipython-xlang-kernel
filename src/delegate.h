
#ifndef __delegate__h__
#define __delegate__h__

#include "tthread/tinythread.h"
#include <cassert>
#include <iostream>


/*
  the clever trick about boost::function is that
  it can be either a functor or a function pointer
 */

// struct {
// }

template <typename R, typename A1>
struct fun_pointer1 {
    typedef R (*type)(A1);
};

template <typename R, typename A1, typename A2>
struct fun_pointer2 {
    typedef R (*type)(A1, A2);
};

template <typename T, typename R, typename A1>
struct mem_fun_pointer1
{
    typedef R (T::*type)(A1);
};



template <typename T>
struct delegate_t {

    typedef void (T::*pmf_t)();
    delegate_t(T* that_, pmf_t mf_)
        : that(that_), mf(mf_)
    {
    }

    T* that;
    pmf_t mf;

    static void dispatch(void * data) {
        assert(data != NULL);
        delegate_t<T>* p = reinterpret_cast<delegate_t<T>*>(data);
        (p->that->*(p->mf))();
    }
};


template <typename T, typename R, typename A1>
struct delegate1_t {

    typedef R (T::*pmf_t)(A1 a1);
    typedef delegate1_t<T,R,A1> SelfType;

    delegate1_t(T* that_, pmf_t mf_)
        : that(that_), mf(mf_)
    {
    }

    R operator()(A1 a1) {
        return (that->*mf)(a1);
    }

    T* that;
    pmf_t mf;

    static R dispatch(void * data, A1 a1) {
        assert(data != NULL);
        delegate1_t<T, R, A1>* p = reinterpret_cast<delegate1_t<T, R, A1>*>(data);
        return (*p)(a1);
    }

    static R dispatch(void * data, void * mf, A1 a1) {
        pmf_t p = (pmf_t)mf;
    }

    static void free(void* p) {
        SelfType * that = reinterpret_cast<SelfType*>(p);
        delete that;
    }

    static void * clone(void *p) {
        SelfType * that = reinterpret_cast<SelfType*>(p);
        return new SelfType(that->that, that->mf);
    }
};


template<typename T, typename R, typename A1>
R _thunk(void* p, A1 a1) {
    T* t = (T*)(p);
    return (*t)(a1);
}


// struct binding

template <typename R, typename A1>
struct function1 {

    typedef fun_pointer1<void, void*>::type free_type;
    typedef fun_pointer1<void*, void*>::type clone_type;

    function1(typename fun_pointer1<R, A1>::type f)
    :   _fp((void*)f),
        _arg(NULL),
        _free(NULL),
        _clone(NULL)
    {
    }

    template<typename T>
    function1(delegate1_t<T,R,A1> del)
    {        // this is
        typedef R (T::*MPTR) (A1);
        typedef R (*FPTR)(void*, A1);

        MPTR ptr = del.mf;
        // del.dispatch(del.that, ptr, )
        // void * p = reinterpret_cast<void*>((del.that)->*ptr);
        // FPTR fptr = &_thunk<T, R, A1>;

        typedef delegate1_t<T,R,A1> D;
        _free  = D::free;
        _clone = D::clone;
        _arg = new D(del.that, del.mf);
        FPTR fptr = _thunk<D, R, A1>;
        _fp = (void*)fptr;
        std::cerr << "_fp " << _fp << std::endl;
        std::cerr << "_arg " << _arg << std::endl;
    }

    /*
    template<typename T>
    function1(T &t)
    : _free(NULL), _clone(NULL)
    {        // this is
        typedef R (T::*MPTR) (A1);
        typedef R (*FPTR)(void*, A1);

        MPTR ptr = &T::operator();
        FPTR fptr = &_thunk<T, R, A1>;
        _arg = (void*) &t;
        _fp  = (void*) fptr;
    }
    */

    function1(const function1<R, A1> & f)
    {
        if (f._clone != NULL && f._arg != NULL) _arg = f._clone(f._arg);
        _clone = f._clone;
        _free  = f._free;
        _fp = f._fp;
        std::cerr << "clone _fp " << _fp << std::endl;
        std::cerr << "clone _arg " << _arg << std::endl;
    }

    ~function1() {
        if (_free && _arg) _free(_arg);
    }

    R operator ()(A1 a1) {
        if (_arg == NULL) {
            typedef typename fun_pointer1<R,A1>::type ty;
            ty *p = reinterpret_cast<ty*>(_fp);
            return p(a1);
        }
        else {
            typedef typename fun_pointer2<R,void*,A1>::type ty;
            ty p = reinterpret_cast<ty>(_fp);
            return p(_arg, a1);
        }
    }

    /* typename fun_pointer<R, A1>::type _fp; */
    void * _fp;
    void * _arg;
    free_type _free;
    clone_type _clone;

    // Tag _tag;
};



template <typename T>
struct active_method
{
    active_method(const delegate_t<T> & obj);
    ~active_method();

    bool start();
    void join();

private:
    tthread::thread * _thread;
    delegate_t<T> _delegate;
};

template<typename T>
inline
active_method<T>::active_method(const delegate_t<T> & obj)
  : _thread(NULL),
    _delegate(obj)
{
}

template<typename T>
inline
active_method<T>::~active_method()
{
    if (_thread) {
        _thread->join();
        delete _thread;
    }
}


template<typename T>
inline
bool active_method<T>::start() {
    // lock-free?
    if (_thread) {
        return false;
    }
    _thread = new tthread::thread(delegate_t<T>::dispatch, &_delegate);
    // should begin already.
    return true;
}

template<typename T>
inline
void active_method<T>::join() {
    // lock-free?
    if (_thread) {
        return;
    }
    _thread->join();
}


#endif
