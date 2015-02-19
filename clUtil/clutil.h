#ifndef CLUTIL_H
#define CLUTIL_H
#include <vector>
#include <list>

#include <boost/smart_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>

#include "SDKApplication.hpp"

//// Dynamic Info
#ifdef DECLARE_PTR
#undef DECLARE_PTR
#endif
#define DECLARE_PTR(CLASS) typedef boost::shared_ptr<CLASS> CLASS##Ptr;

//===============================================
//// Static Info
struct PlatformInfo
{
    cl_platform_id id;
    std::string name;
    std::string version;
};
class ClProgram;
DECLARE_PTR(ClProgram)
class ClContext;
DECLARE_PTR(ClContext)
class ClKernel;
DECLARE_PTR(ClKernel)
class ClCmdQ;
DECLARE_PTR(ClCmdQ)
class ClMem;
DECLARE_PTR(ClMem)
class ClDevice;
class ClPlatform : public PlatformInfo
{
public:
    std::vector<ClDevice>    devices;
    std::vector<cl_device_id> deviceIDs;
protected:
    //  static std::vector<ClPlatform> _platforms;
    //  static std::vector<cl_platform_id> _ids;

public:
    //  static cl_int errcode;

    static std::vector<ClPlatform>& Instances(cl_int* err=NULL);

    static ClPlatform* get(cl_platform_id id) {
        std::vector<ClPlatform>& _platforms = Instances();
        for(int i = 0;i < _platforms.size(); ++i)
            if( _platforms[i].id == id) return &_platforms[i];
        return NULL;
    }
    // if kernelstr is NULL, just create a blank kernel to retrieve system info. return NULL pointer, and delete every thing when function return.
    static std::vector<ClKernelPtr> createSimpleKernel(std::ostream& os, int platformIdx=-1, const char *kernelsrc=NULL, const char *kernelname=NULL);

    std::vector<ClDevice*> getDevices(cl_device_type devtype=CL_DEVICE_TYPE_ALL);
    std::vector<ClDevice*> getDevices(const std::vector<cl_device_id>& devids );

    std::vector<cl_device_id> getDeviceIDs(cl_device_type devtype=CL_DEVICE_TYPE_ALL);
    static std::vector<cl_device_id> getDeviceIDs(const std::vector<ClDevice*>& devs);
};

class ClContextInfo
{
public:
    std::vector<cl_image_format> image2DFormats, image3DFormats;

};

class ClDevice : public streamsdk::SDKDeviceInfo, public streamsdk::KernelWorkGroupInfo
{
public:
    cl_device_id              id;
    ClPlatform               *pPlatform;

    void destroy() {
        clReleaseDevice(id);
    }
};

//==============================================

#define DECLARE_MAKE_SHARED0(T) \
    boost::shared_ptr<T> boost::make_shared<T>();

#define DECLARE_MAKE_SHARED1(T, A0) \
    boost::shared_ptr<T> boost::make_shared<T, typename A0>(A0 const&)

#define DECLARE_MAKE_SHARED2(T, A0, A1) \
    boost::shared_ptr<T> boost::make_shared<T, typename A0, typename A1>(A0 const&, A1 const&)

#define DECLARE_MAKE_SHARED3(T, A0, A1, A2) \
    boost::shared_ptr<T> boost::make_shared<T, typename A0, typename A1>(A0 const&, A1 const&, A2 const &)

#define DECLARE_MAKE_SHARED4(T, A0, A1, A2, A3) \
    boost::shared_ptr<T> boost::make_shared<T, typename A0, typename A1>(A0 const&, A1 const&, A2 const &, A3 const &)

#define DECLARE_MAKE_SHARED5(T, A0, A1, A2, A3, A4) \
    boost::shared_ptr<T> boost::make_shared<T, typename A0, typename A1>(A0 const&, A1 const&, A2 const &, A3 const &, A4 const &)

#define DECLARE_MAKE_SHARED6(T, A0, A1, A2, A3, A4, A5) \
    boost::shared_ptr<T> boost::make_shared<T, typename A0, typename A1>(A0 const&, A1 const&, A2 const &, A3 const &, A4 const &, A5 const &)



class ClContext
        : public boost::enable_shared_from_this<ClContext>
{
public:
    cl_context       _context;
    ClPlatform*      platform;

    cl_int          errcode;
    std::list<ClProgramPtr> programs;
    std::list<ClCmdQPtr> cmdQs;
    std::list<ClMemPtr> mems;

    std::vector<ClDevice *> devices;
public:

    ClProgramPtr createProgram(const char *src, bool fromFile=true, const char *cflags=NULL, std::ostream& os=std::cout);
    ClProgramPtr createProgram(std::vector<std::string>& vsrc, std::vector<ClDevice*>& vdev, bool fromFile=true, const char *cflags=NULL, std::ostream& os=std::cout);

    ClCmdQPtr createCmdQ(ClDevice*);
    ClMemPtr createMem(cl_mem_flags flags, size_t bufsize, void* buf);

    void release(ClProgramPtr p){
        programs.remove(p);
    }
    void release(ClCmdQPtr c) {
        cmdQs.remove(c);
    }
    void release(ClMemPtr m) {
        mems.remove(m);
    }

    ~ClContext(){destroy();}
protected:
    void destroy(){
        programs.clear();
        mems.clear();
        cmdQs.clear();
        clReleaseContext(_context);
    }
    void addProgram(ClProgramPtr program){
        programs.push_back(program);
    }
    void addCmdQ(ClCmdQPtr cmdQ){
        cmdQs.push_back(cmdQ);
    }
    void addMem(ClMemPtr mem){
        mems.push_back(mem);
    }

public:
    ClContext(ClPlatform* p, size_t ndev, ClDevice *devs );
    //  friend DECLARE_MAKE_SHARED2(ClContext, ClPlatform*, cl_device_type);

    //   friend DECLARE_MAKE_SHARED3(ClContext, ClPlatform, size_t, ClDevice*);
    //   ClContext(ClPlatform& p, size_t n, ClDevice*):platform(p){}

    //   friend DECLARE_MAKE_SHARED3(ClContext, ClPlatform, size_t, const cl_device_id*);
    //   ClContext(ClPlatform& p, size_t n, const cl_device_id*):platform(p){}

};

class ClProgram
        : public boost::enable_shared_from_this<ClProgram>
{
public:
    cl_program    _program;
    std::vector<unsigned char> binary;
    ClContextPtr pContext;

    std::list<ClKernelPtr> kernels;
    std::vector<ClDevice*> devices;

    cl_int          errcode;
    std::vector<std::string>    buildLog; // same size as devices
public:

    ClKernelPtr createKernel(const char *name);

    //   int fromBinary(ClContextPtr contxt, size_t size, unsigned char *bin) {}


    //   int saveBinary(const char* fname){}
    //   int loadBinary(const char* fname){}

    void release(ClKernelPtr k) {
        kernels.remove(k);
    }

    ~ClProgram(){destroy();}
protected:
    void destroy(){
        kernels.clear();
        clReleaseProgram(_program);
    }
    void addKernel(ClKernelPtr kernel){
        kernels.push_back(kernel);
    }
private:
    cl_int compile(size_t nsrc, const char **src, size_t ndev, ClDevice **vdev, const char *cflags);

public:
    //  friend DECLARE_MAKE_SHARED4(ClProgram, ClContextPtr, size_t, const char **, bool);
    ClProgram(ClContextPtr, const char *src, bool fromFile, const char *cflags=NULL);
    ClProgram(ClContextPtr, const std::vector<std::string>& vsrc, const std::vector<ClDevice*>& vdev, bool fromFile=true, const char *cflags=NULL);


};

class ClKernel
      :  public boost::enable_shared_from_this<ClKernel>
{
public:
    cl_kernel     _kernel;
    ClProgramPtr    pProgram;
    cl_int          errcode;
    std::vector<streamsdk::KernelWorkGroupInfo> workGroupInfos;
public:
    ~ClKernel(){destroy();}

    cl_int setArg(size_t idx, const ClMemPtr& m);
    cl_int setArg(size_t idx, cl_int v);
    cl_int setArg(size_t idx, cl_uint v);
    cl_int setArgLocal(size_t idx, size_t v); // local mem

    template<typename T1, typename T2>
    cl_int setArgs(size_t idx, const T1& a1, const T2& a2) {
        cl_int ret = CL_SUCCESS;
        if( CL_SUCCESS != (ret=setArg(idx++, a1)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a2)) ) return ret;
        return ret;
    }
    template<typename T1, typename T2, typename T3>
    cl_int setArgs(size_t idx, const T1& a1, const T2& a2, const T3& a3) {
        cl_int ret = CL_SUCCESS;
        if( CL_SUCCESS != (ret=setArg(idx++, a1)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a2)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a3)) ) return ret;
        return ret;
    }
    template<typename T1, typename T2, typename T3, typename T4>
    cl_int setArgs(size_t idx, const T1& a1, const T2& a2, const T3& a3, const T4& a4) {
        cl_int ret = CL_SUCCESS;
        if( CL_SUCCESS != (ret=setArg(idx++, a1)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a2)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a3)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a4)) ) return ret;
        return ret;
    }
    template<typename T1, typename T2, typename T3, typename T4, typename T5>
    cl_int setArgs(size_t idx, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5) {
        cl_int ret = CL_SUCCESS;
        if( CL_SUCCESS != (ret=setArg(idx++, a1)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a2)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a3)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a4)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a5)) ) return ret;
        return ret;
    }
    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
    cl_int setArgs(size_t idx, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5, const T6& a6) {
        cl_int ret = CL_SUCCESS;
        if( CL_SUCCESS != (ret=setArg(idx++, a1)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a2)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a3)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a4)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a5)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a6)) ) return ret;
        return ret;
    }
    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
    cl_int setArgs(size_t idx, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5, const T6& a6, const T7& a7) {
        cl_int ret = CL_SUCCESS;
        if( CL_SUCCESS != (ret=setArg(idx++, a1)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a2)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a3)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a4)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a5)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a6)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a7)) ) return ret;
        return ret;
    }
    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
    cl_int setArgs(size_t idx, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5, const T6& a6, const T7& a7, const T8& a8) {
        cl_int ret = CL_SUCCESS;
        if( CL_SUCCESS != (ret=setArg(idx++, a1)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a2)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a3)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a4)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a5)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a6)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a7)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a8)) ) return ret;
        return ret;
    }
    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
    cl_int setArgs(size_t idx, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5, const T6& a6, const T7& a7, const T8& a8, const T9& a9) {
        cl_int ret = CL_SUCCESS;
        if( CL_SUCCESS != (ret=setArg(idx++, a1)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a2)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a3)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a4)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a5)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a6)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a7)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a8)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a9)) ) return ret;
        return ret;
    }
    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
    cl_int setArgs(size_t idx, const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5, const T6& a6, const T7& a7, const T8& a8, const T9& a9, const T10& a10) {
        cl_int ret = CL_SUCCESS;
        if( CL_SUCCESS != (ret=setArg(idx++, a1)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a2)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a3)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a4)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a5)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a6)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a7)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a8)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a9)) ) return ret;
        if( CL_SUCCESS != (ret=setArg(idx++, a10)) ) return ret;
        return ret;
    }

private:
    void destroy(){
        clReleaseKernel(_kernel);
    }
public:
    //  friend DECLARE_MAKE_SHARED2(ClKernel, ClProgramPtr, const char*);
    ClKernel(ClProgramPtr, const char* name);

    cl_int validSizes(int deviceId, size_t dim, size_t *globalsize, size_t *localsize, size_t localmem=0);
};

class ClCmdQ
        : public boost::enable_shared_from_this<ClCmdQ>
{
public:
    cl_command_queue _cmdQ;

    cl_int errcode;

    ClDevice    *pDevice;
    ClContextPtr pContext;
public:
    ~ClCmdQ(){destroy();}
    cl_int flush();

    cl_int enqueue(ClKernelPtr kernel, size_t dim, const size_t *global_work_offset, const size_t *global_work_size, const size_t *local_work_size=NULL,
                   cl_uint num_events_in_wait_list=0, const cl_event *event_wait_list=NULL,cl_event *event=NULL);

    cl_int enqueueWrite(ClMemPtr& buffer, cl_bool blocking, size_t offset, size_t cb, const void *ptr,
                            cl_uint num_events_in_wait_list=0, const cl_event *event_wait_list=NULL, cl_event *even=NULL);
    cl_int enqueueRead(ClMemPtr& buffer, cl_bool blocking, size_t offset,size_t cb, void *ptr,
                       cl_uint num_events_in_wait_list=0, const cl_event *event_wait_list=NULL, cl_event *event=NULL);

    cl_int enqueueCopy(ClMemPtr& src, ClMemPtr& dest, size_t src_offset, size_t dest_offset, size_t cb,
                       cl_uint num_events_in_wait_list=0, const cl_event *event_wait_list=NULL, cl_event *event=NULL);

    cl_int enqueueMap(ClMemPtr& buffer, cl_bool blocking, cl_map_flags flags, size_t offset, size_t cb, void **ptr,
                     cl_uint num_events_in_wait_list=0, const cl_event *event_wait_list=NULL, cl_event *event=NULL);

    cl_int enqueueUnmap(ClMemPtr& buffer, void *ptr,
                        cl_uint num_events_in_wait_list=0, const cl_event *event_wait_list=NULL, cl_event *event=NULL);

    cl_int enqueueBarrier();

private:
    void destroy(){
        clReleaseCommandQueue(_cmdQ);
    }
public:
    //  friend DECLARE_MAKE_SHARED2(ClCmdQ, ClContextPtr, ClDevice*);
    ClCmdQ(ClContextPtr, ClDevice*);
};

class ClMem
        : public boost::enable_shared_from_this<ClMem>
{
public:
    cl_mem _mem;

    cl_int errcode;

    ClContextPtr pContext;
    void *buf;
    size_t bufsize;
    cl_mem_flags flags;
public:
    ~ClMem(){destroy();}
private:
    void destroy(){
        clReleaseMemObject(_mem);
    }
    //  friend DECLARE_MAKE_SHARED4(ClMem, ClContextPtr, cl_mem_flags, size_t, char*);
public:
    ClMem(ClContextPtr contxt, cl_mem_flags flags, size_t bufsize, void* buf =NULL);
};
//===================================================
//===        Execution

struct Exec
{
    ClKernelPtr  pKernel;
    ClCmdQPtr    pCmdQ;
    cl_event     evt;
    Exec(ClKernelPtr pKrn, ClCmdQPtr pCmd): pKernel(pKrn), pCmdQ(pCmd) {}
    virtual cl_int operator()() {return CL_SUCCESS;}
    virtual ~Exec(){}
};
typedef boost::shared_ptr<Exec> ExecPtr;

struct ReadWriteMem : public Exec
{
    ClMemPtr pMem;
    void *ptr;
    bool read;
    ReadWriteMem(ClKernelPtr pKrn, ClCmdQPtr pCmd, ClMemPtr mem, void *ptr=NULL, bool read=false)
        :Exec(pKrn, pCmd), pMem(mem),ptr(ptr), read(read) {}
    cl_int operator << (void *aptr) {
        return pCmdQ->enqueueWrite(pMem, CL_TRUE, 0, pMem->bufsize, ptr=aptr);
    }
    cl_int operator >> (void *aptr) {
        return pCmdQ->enqueueRead(pMem, CL_TRUE, 0, pMem->bufsize, ptr=aptr);
    }
    cl_int operator()() {
        return read? *this >> ptr : *this << ptr;
    }
};
struct MapMem : public Exec
{
    void *ptr;
    ClMemPtr pMem;
    size_t bufsize;

    MapMem(ClKernelPtr pKrn, ClCmdQPtr pCmd, ClMemPtr buf=ClMemPtr()):Exec(pKrn, pCmd), pMem(buf), ptr(NULL),bufsize(buf?buf->bufsize:0) {}

    cl_int unmap() {
        cl_int status = CL_SUCCESS;
        if(ptr) {
            if(CL_SUCCESS == (status = pCmdQ->enqueueUnmap(pMem, ptr)) ) {
                pCmdQ->flush();
                pCmdQ->enqueueBarrier();
            }
            ptr = NULL;
        }
        return status;
    }
    cl_int map(bool read=false, ClMemPtr buf=ClMemPtr()) {
        cl_int status = CL_SUCCESS;
        unmap();
        if(buf) {
            pMem = buf;
            bufsize = pMem->bufsize;
        }
        if(CL_SUCCESS == (status = pCmdQ->enqueueMap(pMem, CL_TRUE, read?CL_MAP_READ:CL_MAP_WRITE, 0, pMem->bufsize, &ptr) )) {
            pCmdQ->flush();
            pCmdQ->enqueueBarrier();
        }
        return status;
    }
    virtual ~MapMem(){
        unmap();
    }
};
struct ExecKernel : public Exec
{
    size_t dim;
    size_t *globalsize;
    size_t *localsize;
    ExecKernel(ClKernelPtr pKrn, ClCmdQPtr pCmd, size_t dim=1, size_t *globalsize=NULL, size_t *localsize=NULL)
        :Exec(pKrn, pCmd),dim(dim),globalsize(globalsize), localsize(localsize) {}
    cl_int operator() () {
        cl_int status = CL_SUCCESS;
        if(CL_SUCCESS == (status = pCmdQ->enqueue(pKernel, dim, NULL, globalsize,localsize) )) {
            pCmdQ->flush();
            pCmdQ->enqueueBarrier();
        }
        return status;
    }
    cl_int operator() (size_t pdim, size_t *pglobalsize, size_t *plocalsize) {
        cl_int status = CL_SUCCESS;
        if(CL_SUCCESS == (status = pCmdQ->enqueue(pKernel, dim=pdim, NULL, globalsize=pglobalsize,localsize=plocalsize) )) {
            pCmdQ->flush();
            pCmdQ->enqueueBarrier();
        }
        return status;
    }
};

cl_int WaitForEvent(cl_event, cl_int evt_state = CL_COMPLETE, bool release = true);

template<unsigned int B>
unsigned long clCeiling(unsigned long n) {
    return n%B == 0? n: (n-n%B+B);
}



#endif // CLUTIL_H





