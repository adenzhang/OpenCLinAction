#ifndef CMAT_H
#define CMAT_H

#ifdef __cplusplus

#include <assert.h>

#include <boost/mpl/if.hpp>

#define CMAT_BOOST_SMART_PTR
//======== matrix
#ifdef CMAT_BOOST_SMART_PTR
#include <boost/smart_ptr.hpp>
#endif
template<typename T, bool RowMajor>
struct CMatBase{
   struct NullDeleter {void operator ()(T*){}};
#ifdef CMAT_BOOST_SMART_PTR
   typedef boost::shared_array<T> DPtr;
#else
   typedef T* DPtr;
   int        bOwnData;
#endif

   const bool  _rowmajor;
   const int    idx0;
   unsigned long  nrow, ncol, nsize;
   long  rowend, colend;
   DPtr    data;

   explicit CMatBase(T* a, unsigned long M, unsigned long N=1, int startidx=RowMajor?0:1)
#ifdef CMAT_BOOST_SMART_PTR
       :data(a, NullDeleter())
#else
       :data(a)
#endif
       , nrow(M), ncol(N), idx0(startidx), rowend(idx0+nrow), colend(idx0+ncol), nsize(nrow*ncol), _rowmajor(RowMajor){}

#ifdef CMAT_BOOST_SMART_PTR
   explicit CMatBase(DPtr a, unsigned long M, unsigned long N=1, int startidx=RowMajor?0:1)
       :data(a), nrow(M), ncol(N), idx0(startidx), rowend(idx0+nrow), colend(idx0+ncol), nsize(nrow*ncol), _rowmajor(RowMajor){}
#endif
   explicit CMatBase(unsigned long M, unsigned long N=1, int startidx=RowMajor?0:1)
       :nrow(M), ncol(N), idx0(startidx), rowend(idx0+nrow), colend(idx0+ncol), nsize(nrow*ncol), _rowmajor(RowMajor){allocate(nrow, ncol);}

   CMatBase(const CMatBase& m)
       :data(), nrow(m.nrow), ncol(m.ncol), idx0(m.idx0), rowend(idx0+nrow), colend(idx0+ncol), nsize(nrow*ncol), _rowmajor(RowMajor){allocate(nrow, ncol);assign(m);}

   CMatBase()
       :data(), nrow(0), ncol(0), idx0(RowMajor?0:1), rowend(idx0+nrow), colend(idx0+ncol), nsize(nrow*ncol), _rowmajor(RowMajor){}

   void allocate(unsigned long M, unsigned long N) {
       nrow = M;
       ncol = N;
       rowend = idx0 + M;
       colend = idx0 + N;
       nsize = nrow*ncol;
#ifdef CMAT_BOOST_SMART_PTR
       if(nsize>0)
           data.reset(new T[M*N]);
       else
           data.reset();;
#else
       if(data && bOwnData) { delete[] data; data=NULL;}
       if(nsize>0) data = new T[nsize];
       bOwnData = 1;
#endif
   }

#ifdef CMAT_BOOST_SMART_PTR
   T* ptr() {return data.get();}
   const T* ptr() const {return data.get();}
   virtual ~CMatBase() {}
#else
   T* ptr() {return data;}
   const T* ptr() const {return data;}
   virtual ~CMatBase() {
       if(data && bOwnData) { delete[] data; data=NULL;}
   }
#endif

   unsigned long size(){
       return nsize;
   }

   void resize(unsigned long M, unsigned long N) {
       if(size() == M*N) return;
       allocate(M, N);
   }


   inline T& operator()(long i) {
       assert(i >= idx0 && i < idx0+nrow);
       // work as vector
       return data[i-idx0];
   }
   inline const T& operator()(long i) const{
       assert(i >= idx0 && i < idx0+nrow);
       // work as vector
       return data[i-idx0];
   }
   inline T& operator[](long i) {
       assert(i >= idx0 && i < idx0+nrow);
       // work as vector
       return data[i-idx0];
   }
   inline const T& operator[](long i) const {
       assert(i >= idx0 && i < idx0+nrow);
       // work as vector
       return data[i-idx0];
   }
   // row major, 0 indexed
   inline T& at0(long i, long j) {
       assert(i >= 0 && i < nrow);
       assert(j >= 0 && j < ncol);

       return data[ncol*i + j];
   }
   inline const T& at0(long i, long j) const {
       assert(i >= 0 && i < nrow);
       assert(j >= 0 && j < ncol);

       return data[ncol*i + j];
   }
   // col major, 1 indexed
   inline T& at1(long i, long j) {
       return data[ncol*(j-1) + (i-1)];
   }
   inline const T& at1(long i, long j) const {
       return data[ncol*(j-1) + (i-1)];
   }

   template <bool RowMj>
   inline T& At(long i, long j){}
   template<>
   inline T& At<true>(long i, long j){
       return data[(i-idx0)*ncol+(j-idx0)];
   }
   template<>
   inline T& At<false>(long i, long j){
       return data[(j-idx0)*row+(i-idx0)];
   }
   template <bool RowMj>
   inline const T& At(long i, long j) const {}
   template<>
   inline const T& At<true>(long i, long j)const{
       return data[(i-idx0)*ncol+(j-idx0)];
   }
   template<>
   inline const T& At<false>(long i, long j)const {
       return data[(j-idx0)*row+(i-idx0)];
   }

   // default operator by row
   T& operator()(long i, long j){
       return At<RowMajor>(i,j);
   }

   const T& operator()(long i, long j) const{
       return At<RowMajor>(i,j);
   }

   // deep copy
   CMatBase& assign(const CMatBase& m){
       resize(m.nrow, m.ncol);
       T *p = ptr();
       const T *q = m.ptr();
       for(int i=0; i<m.nsize; ++i, ++p, ++q)  *p = *q;
       return *this;
   }
   CMatBase& operator=(const CMatBase& m){
       return assign(m);
   }
   template<bool RowMj>
   CMatBase& copyRect_(const CMatBase& m, int rm0=0, int cm0=0, int r0=0, int c0=0, unsigned int nr=0, unsigned int nc=0) {}
   template<>
   CMatBase& copyRect_<true>(const CMatBase& m, int rm0, int cm0, int r0, int c0, unsigned int nr, unsigned int nc) {
       long NR = (nrow-nr0+idx0)<(m.nrow-rm0+m.idx0)?(nrow-nr0+idx0):(m.nrow-rm0+m.idx0);
       long NC = (ncol-nc0+idx0)<(m.ncol-cm0+m.idx0)?(ncol-nc0+idx0):(m.ncol-cm0+m.idx0);
       if(nr>0 && nr<NR) NR = nr;
       if(nc>0 && nc<NC) NC = nc;
       for(int i=0; i<NR; ++i) {
           T *p = &At<true>(i+r0,c0);
           const T *q = &m.At<true>(i+rm0,cm0);
           for(int j=0; j<NC; ++j, ++p, ++q) {
               *p = *q;
           }
       }
       return (*this);
   }
   template<>
   CMatBase& copyRect_<false>(const CMatBase& m, int rm0, int cm0, int r0, int c0, unsigned int nr, unsigned int nc) {
       long NR = (nrow-nr0+idx0)<(m.nrow-rm0+m.idx0)?(nrow-nr0+idx0):(m.nrow-rm0+m.idx0);
       long NC = (ncol-nc0+idx0)<(m.ncol-cm0+m.idx0)?(ncol-nc0+idx0):(m.ncol-cm0+m.idx0);
       if(nr>0 && nr<NR) NR = nr;
       if(nc>0 && nc<NC) NC = nc;
       for(int j=0; j<NC; ++j) {
           T *p = &At<false>(r0,j+c0);
           const T *q = &m.At<false>(rm0,i+cm0);
           for(int i=0; i<NR; ++i, ++p, ++q) {
               *p = *q;
           }
       }
       return (*this);
   }
   CMatBase& copyRect(const CMatBase& m, int rm0=0, int cm0=0, int r0=0, int c0=0, unsigned int nr=0, unsigned int nc=0) {
       return copyRect_<RowMajor>(m, rm0, cm0, r0,c0, nr, nc);
   }

   CMatBase& assign(T  t) {
       T *p = &at0(0,0);
       for(int i=0; i<nsize; ++i)
           *(p++) = t;
       return *this;
   }
   CMatBase& operator = (T  t) {
       return assign(t);
   }
   CMatBase& operator +(T  t) {
       T *p = &at0(0,0);
       for(int i=0; i<nsize; ++i)  *(p++) += t;
       return *this;
   }
   CMatBase& operator *(T  t) {
       T *p = &at0(0,0);
       for(int i=0; i<c.nsize; ++i)  *(p++) *= t;
       return *this;
   }
   CMatBase& operator +(const CMatBase& m) {
       assert(nrow == m.nrow && ncol == m.ncol);
       T *p = &at0(0,0);
       const T *q = &m.at0(0,0);
       for(int i=0; i<nsize; ++i)  *(p++) += *(q++);
       return *this;
   }
   // element multi
   CMatBase& multiply_e(const CMatBase& m) {
       assert(nrow == m.nrow && ncol == m.ncol);
       if( RowMajor ){
           for(int i=0; i<a.nrow; ++i){
               T *p = &at0(i,0);
               const T *q=&m.at0(i,0);
               for(int j=0; j<a.ncol; ++j, ++p, ++q)
                   *p *= *q;
           }
       }else{
           for(int i=0; i<ncol; ++i) {
               T *p = &at0(0,i);
               const T *q=&m.at0(0,i);
               for(int j=0; j<nrow; ++j, ++p, ++q)
                   *p *= *q;
           }
       }
       return *this;
   }
   // element multi
   CMatBase& divide_e(const CMatBase& m) {
       assert(nrow == m.nrow && ncol == m.ncol);
       if( RowMajor ){
           for(int i=0; i<nrow; ++i){
               T *p = &at0(i,0);
               const T *q=&m.at0(i,0);
               for(int j=0; j<ncol; ++j, ++p, ++q)
                   *p /= *q;
           }
       }else{
           for(int i=0; i<ncol; ++i) {
               T *p = &at0(0,i);
               const *q=&m.at0(0,i);
               for(int j=0; j<nrow; ++j, ++p, ++q)
                   *p /= *q;
           }
       }
       return *this;
   }
   CMatBase& trans(const CMatBase& m) {
       resize(m.ncol, m.nrow);
       if( RowMajor ) {
           for(int i=0; i<nrow; i++) {
               const T* p = &m.at0(i,0);
               for(int j=0; j<ncol;++j, ++p)
                   at0(j,i) = *p;
           }
       }else{
           for(int i=0; i<ncol; i++) {
               const T* p = &m.at0(0,i);
               for(int j=0; j<nrow;++j, ++p)
                   at0(i,j) = *p;
           }
       }
       return *this;
   }
   CMatBase& trans() {
       if( ncol == nrow ) {
           if( RowMajor ) {
               for(int i=0; i<nrow; i++) {
                   T* p = &cat0(i,i+1);
                   for(int j=i+1; j<ncol;++j, ++p)
                       swap(at0(j,i), *p);
               }
           }else{
               for(int i=0; i<ncol; i++) {
                   T* p = &cat0(i+1,i);
                   for(int j=i+1; j<nrow;++j, ++p)
                       swap(at0(i,j),*p);
               }
           }
       }else{
           CMatBase m(*this);
           trans(m);
       }
       return *this;
   }

   CMatBase& multiply(const CMatBase& a, const CMatBase& b) {
       resize(a.nrow, b.ncol);
       if( RowMajor ) {
           T *p = &at0(0,0);
           for(long i=0; i<a.nrow; ++i) {
               for(long j=0; j<b.ncol; ++j) {
                   const T *a0= &a.at0(i,0);
                   *p = 0;
                   for(long k=0; k < a.ncol; ++k, ++a0)
                       *p += *a0 * b.at0(k,j);
                   ++p;
               }
           }
       }else{
           T *p = &at0(0,0);
           for(long j=0; j<b.ncol; ++j) {
               for(long i=0; i<a.nrow; ++i) {
                   const T *b0= &b.at0(i,0);
                   *p = 0;
                   for(long k=0; k < a.ncol; ++k, ++b0)
                       *p += *b0 * a.at0(k,j);
                   ++p;
               }
           }
       }
       return *this;
   }
   const CMatBase operator *(const CMatBase& m) const
   {
       return CMatBase().multiply(*this, m);
   }

   CMatBase& operator +(const CMatBase& m) const
   {
       return CMatBase(*this).add(m);
   }

   void swap(T& a, T&b){
       T t = a;
       a=b;
       b=t;
   }

private:
};


template<typename T>
class CMat
       : public CMatBase<T, true>
{
public:
   explicit CMat(T* a, unsigned long M, unsigned long N=1, int startidx=0) : CMatBase(a, M, N, startidx){}
   explicit CMat(DPtr a, unsigned long M, unsigned long N=1, int startidx=0) :CMatBase(a, M, N, startidx) {}
   explicit CMat(unsigned long M, unsigned long N=1, int startidx=0) : CMatBase(M, N, startidx) {}
   CMat(const CMat& m) : CMatBase(m) {}
   CMat():CMatBase(){}

   using CMatBase<T, true>::operator =;

};
template<typename T>
class FMat
       : public CMatBase<T, false>
{
public:
   explicit FMat(T* a, unsigned long M, unsigned long N=1, int startidx=1) : CMatBase(a, M, N, startidx){}
   explicit FMat(DPtr a, unsigned long M, unsigned long N=1, int startidx=1) :CMatBase(a, M, N, startidx) {}
   explicit FMat(unsigned long M, unsigned long N=1, int startidx=1) : CMatBase(M, N, startidx) {}
   FMat(const FMat& m) : CMatBase(m) {}
   FMat():CMatBase(){}

   using CMatBase<T, true>::operator =;
};

//typedef template<typename T> class CMatBase<T, 0, true> CMat;
typedef CMat<double> CMatd;
typedef CMat<float> CMatf;
typedef CMat<int> CMati;

typedef FMat<double> FMatd;
typedef FMat<float> FMatf;
typedef FMat<int> FMati;

template<typename T>
T prod(const T* v, size_t n) {
    T s = 1;
    for(size_t i=0;i<n; ++i) s *= *(v++);
    return s;
}
template<typename T>
T sum(const T* v, size_t n) {
    T s = 0;
    for(size_t i=0;i<n; ++i) s += *(v++);
    return s;
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



