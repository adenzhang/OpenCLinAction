#ifndef CMAT_H
#define CMAT_H

#ifdef __cplusplus

#include <assert.h>


#define CMAT_BOOST_SMART_PTR
//======== matrix
#ifdef CMAT_BOOST_SMART_PTR
#include <boost/smart_ptr.hpp>
template<typename T>
struct CMat{
    struct NullDeleter {void operator ()(T*){}};
    typedef boost::shared_array<T> DPtr;
    const bool  rowmajor;
    const int    idx0;
    unsigned long  nrow, ncol;
    long  rowend, colend;
    DPtr    data;

    explicit CMat(T* a, unsigned long M, unsigned long N=1, int start_idx=0, bool row_major=true)
        :data(a, NullDeleter()), nrow(M), ncol(N), idx0(start_idx), rowmajor(N==1?false:row_major), rowend(idx0+nrow), colend(idx0+ncol){}

    explicit CMat(DPtr a, unsigned long M, unsigned long N=1, int start_idx=0, bool row_major=true)
        :data(a), nrow(M), ncol(N), idx0(start_idx),  rowmajor(N==1?false:row_major), rowend(idx0+nrow), colend(idx0+ncol){}

    explicit CMat(unsigned long M, unsigned long N=1, unsigned long start_idx=0, bool row_major=true)
        :data(new T[M*N]), nrow(M), ncol(N), idx0(start_idx), rowmajor(N==1?false:row_major), rowend(idx0+nrow), colend(idx0+ncol){}

    CMat(CMat& m)
        :rowmajor(m.rowmajor), nrow(m.nrow), ncol(m.ncol), idx0(m.idx0), data(m.data), rowend(idx0+nrow), colend(idx0+ncol){}

    CMat(bool row_major=true, int start_idx=0)
        :rowmajor(row_major), idx0(start_idx), nrow(0), ncol(0), rowend(idx0+nrow), colend(idx0+ncol){}

    void allocate(unsigned long M, unsigned long N) {
        nrow = M;
        ncol = N;
        rowend = idx0 + nrow;
        colend = idx0 + ncol;
        data.reset(new T[M*N]);
    }
    T* ptr(){return data.get();}

    inline T& operator()(long i, long j) {
        assert(i >= idx0 && i < idx0+nrow);
        assert(j >= idx0 && j < idx0+ncol);

        return rowmajor? data[ncol*(i-idx0) + j-idx0] : data[nrow*(j-idx0) + i-idx0];
    }
    inline T operator()(long i, long j) const {
        assert(i >= idx0 && i < idx0+nrow);
        assert(j >= idx0 && j < idx0+ncol);

        return rowmajor? data[ncol*(i-idx0) + j-idx0] : data[nrow*(j-idx0) + i-idx0];
    }
    inline T& operator()(long i) {
        assert(i >= idx0 && i < idx0+nrow);
        // work as vector
        return data[i-idx0];
    }
    inline T operator()(long i) const{
        assert(i >= idx0 && i < idx0+nrow);
        // work as vector
        return data[i-idx0];
    }
    inline T& operator[](long i) {
        assert(i >= idx0 && i < idx0+nrow);
        // work as vector
        return data[i-idx0];
    }
    inline T operator[](long i) const {
        assert(i >= idx0 && i < idx0+nrow);
        // work as vector
        return data[i-idx0];
    }
    CMat<T>& assign(T  t) {
        CMat<T>& c(*this);
        for(int i=c.idx0; i<c.rowend; ++i) {
            for(int j=c.idx0; j<c.colend; ++j)
                c(i,j) = t;
        }
        return c;
    }
    CMat<T>& add(T  t) {
        CMat<T>& c(*this);
        for(int i=c.idx0; i<c.rowend; ++i) {
            for(int j=c.idx0; j<c.colend; ++j)
                c(i,j) += t;
        }
        return c;
    }
    CMat<T>& multiplty(T  t) {
        CMat<T>& c(*this);
        for(int i=c.idx0; i<c.rowend; ++i) {
            for(int j=c.idx0; j<c.colend; ++j)
                c(i,j) *= t;
        }
        return c;
    }
    CMat<T>& add(const CMat<T>& m) {
        CMat<T>& c(*this);
        assert(c.nrow == m.nrow && c.ncol == m.ncol);
        for(int i=c.idx0; i<c.rowend; ++i) {
            for(int j=c.idx0; j<c.colend; ++j)
                c(i,j) += m(i,j);
        }
        return c;
    }

    CMat<T>& operator=(const CMat<T>& m){
        CMat<T>& a(*this);
        assert(a.nrow == m.nrow && a.ncol == m.ncol);
        if( a.rowmajor )
            for(int i=a.idx0; i<a.rowend; ++i)
                for(int j=a.idx0; j<a.colend; ++j)
                    a(i,j) = m(i,j);
        else
            for(int i=a.idx0; i<colend; ++i)
                for(int j=a.idx0; j<rowend; ++j)
                    a(j,i) = m(j,i);
        return a;
    }

private:
};
#else
template<typename T>
struct CMat{
    const bool  rowmajor;
    const int idx0;
    bool  _owndata;
    unsigned long  nrow, ncol;
    long  rowend, colend;
    T    *data;
    CMat(T *a, unsigned long M, unsigned long N=1, int start_idx=0, bool row_major=true)
        :data(a), nrow(M), ncol(N), idx0(start_idx), rowmajor(N==1?false:row_major), _owndata(false), rowend(idx0+nrow), colend(idx0+ncol){}

    CMat(unsigned long M, unsigned long N=1, int start_idx=0, bool row_major=true)
        :data(new T[M*N]), nrow(M), ncol(N), idx0(start_idx), rowmajor(N==1?false:row_major),_owndata(true), rowend(idx0+nrow), colend(idx0+ncol){}

    CMat(bool row_major=true, int start_idx=0)
        :data(0), nrow(0), ncol(0), idx0(start_idx), rowmajor(row_major),_owndata(false), rowend(idx0+nrow), colend(idx0+ncol) {}

    CMat(CMat& m)
        :data(m.data), nrow(m.nrow), ncol(m.ncol), idx0(m.start_idx), rowmajor(m.row_major),_owndata(false), rowend(idx0+nrow), colend(idx0+ncol) {}

    void deallocate(){ if(_owndata && data){ delete[] data;data=0;}}
    void allocate(unsigned long M, unsigned long N){
        deallocate();
        data = new T[M*N];
        nrow = M;
        ncol = N;
        rowend = idx0 + nrow;
        colend = idx0 + ncol;
        _owndata = true;
    }
    T* ptr(){return data;}

    ~CMat(){ deallocate();}

    inline T& operator()(long i, long j) {
        assert(i >= idx0 && i < idx0+nrow);
        assert(j >= idx0 && j < idx0+ncol);

        return rowmajor? *(data + ncol*(i-idx0) + j-idx0) : *(data+ nrow*(j-idx0) + i-idx0);
    }
    inline T operator()(long i, long j)const {
        assert(i >= idx0 && i < idx0+nrow);
        assert(j >= idx0 && j < idx0+ncol);

        return rowmajor? *(data + ncol*(i-idx0) + j-idx0) : *(data+ nrow*(j-idx0) + i-idx0);
    }
    inline T& operator()(long i) {
        assert(i >= idx0 && i < idx0+nrow);
        // work as vector
        return *(data + i-idx0);
    }
    inline T operator()(long i) const {
        assert(i >= idx0 && i < idx0+nrow);
        // work as vector
        return *(data + i-idx0);
    }
    inline T& operator[](long i) {
        assert(i >= idx0 && i < idx0+nrow);
        // work as vector
        return *(data + i-idx0);
    }
    inline T operator[](long i) const {
        assert(i >= idx0 && i < idx0+nrow);
        // work as vector
        return *(data + i-idx0);
    }
    CMat<T>& assign(T  t) {
        CMat<T>& c(*this);
        for(long i=c.idx0; i<c.rowend; ++i) {
            for(long j=c.idx0; j<c.colend; ++j)
                c(i,j) = t;
        }
        return c;
    }
    CMat<T>& add(T  t) {
        CMat<T>& c(*this);
        for(long i=c.idx0; i<c.rowend; ++i) {
            for(long j=c.idx0; j<c.colend; ++j)
                c(i,j) += t;
        }
        return c;
    }
    CMat<T>& multiplty(T  t) {
        CMat<T>& c(*this);
        for(long i=c.idx0; i<c.rowend; ++i) {
            for(long j=c.idx0; j<c.colend; ++j)
                c(i,j) *= t;
        }
        return c;
    }
    CMat<T>& add(const CMat<T>& m) {
        CMat<T>& c(*this);
        assert(c.nrow == m.nrow && c.ncol == m.ncol);
        for(int i=c.idx0; i<c.rowend; ++i) {
            for(int j=c.idx0; j<c.colend; ++j)
                c(i,j) += m(i,j);
        }
        return c;
    }
    CMat<T>& operator=(const CMat<T>& m){
        CMat<T>& a(*this);
        assert(a.nrow == m.nrow && a.ncol == m.ncol);
        if( a.rowmajor )
            for(long i=a.idx0; i<a.nrow; ++i)
                for(long j=a.idx0; j<a.ncol; ++j)
                    a(i,j) = m(i,j);
        else
            for(long i=a.idx0; i<ncol; ++i)
                for(long j=a.idx0; j<nrow; ++j)
                    a(j,i) = m(j,i);
        return a;
    }
};
#endif
typedef CMat<double> CMatd;
typedef CMat<float> CMatf;
typedef CMat<int> CMati;


template< typename T>
CMat<T> operator +(const CMat<T>& a, T t)
{
    CMat<T> c(a.nrow, a.ncol, a.idx0, a.rowmajor);
    for(long i=c.idx0; i<c.rowend; ++i) {
        for(long j=c.idx0; j<c.colend; ++j)
            c(i,j) = a(i,j)+t;
    }
    return c;
}
template< typename T>
CMat<T> operator *(const CMat<T>& a,T t)
{
    CMat<T> c(a.nrow, a.ncol, a.idx0, a.rowmajor);
    for(long i=c.idx0; i<c.rowend; ++i) {
        for(long j=c.idx0; j<c.colend; ++j)
            c(i,j) = a(i,j)*t;
    }
    return c;
}

template< typename T>
CMat<T> operator +(const CMat<T>& a, const CMat<T>& b)
{
    assert(a.idx0 == b.idx0 && a.nrow == b.nrow && a.ncol == b.ncol);
    CMat<T> c(a.nrow, a.ncol, a.idx0, a.rowmajor);
    for(long i=c.idx0; i<c.rowend; ++i) {
        for(long j=c.idx0; j<c.colend; ++j)
            c(i,j) = a(i,j) + b(i,j);
    }
    return c;
}

template< typename T>
CMat<T> operator *(const CMat<T>& a, const CMat<T>& b)
{
    assert(a.idx0 == b.idx0 && a.ncol == b.nrow);
    CMat<T> c(a.nrow, b.ncol, a.idx0, a.rowmajor);
    for(long i=a.idx0; i<a.rowend; ++i) {
        for(long j=b.idx0; j<b.colend; ++j) {
            c(i,j) = 0;
            for(long k=a.idx0; k < a.colend; ++k)
                c(i,j) += a(i,k) * b(k,j);
        }
    }
    return c;
}

#endif // __cplusplus

#define MATNAME(a) _a##__mat
#define MATDEF(a, TYPE, M, N, IDX_START, ROW_MAJOR) \
    TYPE *MATNAME(a) = a; \
    int   MATNAME(a)##_M = M, MATNAME(a)##_N = N; \
    int   MATNAME(a)##_idx0 = IDX_START, MATNAME(a)##_rowmaj = ROW_MAJOR;

#define MATASSERT(a, i, j) \
    assert(i >= MATNAME(a)##_idx0 && i < MATNAME(a)##_idx0+MATNAME(a)##_M); \
    assert(j >= MATNAME(a)##_idx0 && j < MATNAME(a)##_idx0+MATNAME(a)##_N)

#define CMATDEF(a, TYPE, M, N) MATDEF(a, TYPE, M, N, 0, 1)
#define FMATDEF(a, TYPE, M, N) MATDEF(a, TYPE, M, N, 1, 0)

// c style row major storage
#define CMATAT(a,M,N) MATNAME(a)[(MATNAME(a)##_N-MATNAME(a)##_idx0) *(M) + (N-MATNAME(a)##_idx0)]
// fortran style column major storage
#define FMATAT(a,M,N) MATNAME(a)[(MATNAME(a)##_M-MATNAME(a)##_idx0) *(N) + (M-MATNAME(a)##_idx0)]

#define MATAT(a, M, N)  \
    (MATNAME(a)##_rowmaj ? CMATAT(a,M,N) : FMATAT(a,M,N))

#define VECAT(a, M)  MATNAME(a)[(M-MATNAME(a)##_idx0)]



#endif // CMAT_H
