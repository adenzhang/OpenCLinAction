#include "clutil.h"
#include <assert.h>

#include <stdarg.h>

using namespace std;

void LOGMSG__(const char *format, ...) {
    va_list args;
    va_start (args, format);
    vprintf (format, args);
    va_end (args);
}
#ifdef WIN32
#define LOGMSG(msg) LOGMSG__##msg;
#else
#define LOGMSG(msg) LOGMSG__(msg);
#endif


cl_int oclCompileProgram(cl_context contxt, cl_device_id devid, const char* src, cl_program *program)
{
  cl_int err;
  size_t len = strlen(src) + 1;
  *program = clCreateProgramWithSource(contxt, 1, (const char **)&src, &len, &err);
  if(err != CL_SUCCESS) {
      LOGMSG(("Failed to clCreateProgramWithSource\n"))
      return err;
  }
  err = clBuildProgram(*program, 0, NULL, "", NULL, NULL);
  if(err != CL_SUCCESS) {
      LOGMSG(("Failed to clBuildProgram\n"))
      return err;
  }

  return 0;
}

//static std::vector<ClPlatform> ClPlatform::_platforms;
//static std::vector<cl_platform_id> ClPlatform::_ids;
std::vector<ClPlatform>& ClPlatform::Instances(cl_int* er)
{
    cl_uint numPlatforms, n;
    cl_int errcode = CL_SUCCESS ;
    static std::vector<ClPlatform> _platforms;
    static std::vector<cl_platform_id> _ids;
    static int __inited = 0;
    if( __inited ) {
        if(er) *er = errcode;
        return _platforms;
    }
    __inited = 1;

    errcode = clGetPlatformIDs(0, NULL, &numPlatforms);
    CHECK_ERROR_RET(_platforms, errcode, CL_SUCCESS, "Failed to get clGetPlatformIDs 0");
    _platforms.resize(numPlatforms);
    _ids.resize(numPlatforms);

    errcode = clGetPlatformIDs(numPlatforms, &_ids[0], &n );
    CHECK_ERROR_RET(_platforms, errcode, CL_SUCCESS, "Failed to get clGetPlatformIDs n");
    assert(n == numPlatforms);

    const int BUFSIZE = 128;
    char buf[BUFSIZE];
    for(cl_uint i=0;i<numPlatforms; ++i) {
        errcode = clGetPlatformInfo(_ids[i], CL_PLATFORM_NAME, BUFSIZE, buf, NULL);
        CHECK_ERROR_RET(_platforms, errcode, CL_SUCCESS, "Failed to get platforminfo");
        _platforms[i].id = _ids[i];
        _platforms[i].name = buf;
        errcode = clGetPlatformInfo(_ids[i], CL_PLATFORM_VERSION, BUFSIZE, buf, NULL);
        CHECK_ERROR_RET(_platforms, errcode, CL_SUCCESS, "Failed to get CL_PLATFORM_VERSION");
        _platforms[i].version = buf;

        // get device info
        errcode = clGetDeviceIDs(_ids[i], CL_DEVICE_TYPE_ALL, 0, NULL, &n);
        CHECK_ERROR_RET(_platforms, errcode, CL_SUCCESS, "Failed to get CL_DEVICE_TYPE_ALL");
        _platforms[i].devices.resize(n);
        _platforms[i].deviceIDs.resize(n);
        errcode = clGetDeviceIDs(_ids[i], CL_DEVICE_TYPE_ALL, n, &_platforms[i].deviceIDs[0], &n);
        CHECK_ERROR_RET(_platforms, errcode, CL_SUCCESS, "Failed to get CL_DEVICE_TYPE_ALL");
        cl_context contxt= clCreateContext(NULL, _platforms[i].devices.size(), &_platforms[i].deviceIDs[0],NULL, NULL,&errcode);
        if( errcode ) {
            LOGMSG(("Failed to clCreateContext\n"))
        }else{
            const char *kernel_src = "__kernel void blank(__global float *a, __global float *b){}";
            for(cl_uint j=0; j<n; ++j) {
                cl_device_id  devid = _platforms[i].deviceIDs[j];
                _platforms[i].devices[j].id = devid;
                _platforms[i].devices[j].pPlatform = &_platforms[i];
                _platforms[i].devices[j].setDeviceInfo(devid);

                // get kernel workgroup info
                cl_program program;
                if( 0 == oclCompileProgram(contxt, devid, kernel_src, &program) ) {
                    cl_kernel kernel = clCreateKernel(program, "blank", &errcode);
                    if( errcode ) {
                        LOGMSG(("Failed to clCreateKernel\n"))
                    }else{
                        _platforms[i].devices[j].setKernelWorkGroupInfo(kernel, devid);
                        clReleaseKernel(kernel);
                    }
                    clReleaseProgram(program);
                }

            }
            // todo get context Image Info


            clReleaseContext(contxt);
        }
    }
    if(er) *er = errcode;
    return _platforms;
}

std::vector<ClDevice*> ClPlatform::getDevices(cl_device_type devtype) {
    std::vector<ClDevice*> v;
    for(int i=0; i<devices.size();++i) if(devices[i].dType & devtype) v.push_back(&devices[i]);
    return v;
}
std::vector<ClDevice*> ClPlatform::getDevices(const std::vector<cl_device_id>& devids )
{
    std::vector<ClDevice*>  v;
    for(int k = 0;k <devids.size(); ++k)
        for(int i=0; i<devices.size();++i)
            if(devices[i].id == devids[k]) v.push_back(&devices[i]);
    return v;
}

std::vector<cl_device_id> ClPlatform::getDeviceIDs(cl_device_type devtype) {
    std::vector<cl_device_id> v;
    for(int i=0; i<devices.size();++i) if(devices[i].dType & devtype) v.push_back(devices[i].id);
    return v;
}
std::vector<cl_device_id> ClPlatform::getDeviceIDs(const std::vector<ClDevice*>& devs)
{
    std::vector<cl_device_id> ids;
    for(int i=0;i<devs.size(); ++i) ids.push_back(devs[i]->id);
    return ids;
}

std::vector<ClKernelPtr> ClPlatform::createSimpleKernel(std::ostream& os, int platformIdx, const char *src, const char *kernelname)
{
    ClKernelPtr krn;
    std::vector<ClKernelPtr> ret;
    bool blank = false;
    const char *srcblank = "__kernel void blank(__global float *a, __global float *b) {}";
    vector<char> srcbuf;

    vector<ClPlatform>& _platforms = ClPlatform::Instances();
    ClContextPtr contxt;
    ClProgramPtr program;

    if( _platforms.size() == 0 ) {
        LOGMSG(("Failed to find any platforms\n"))
        return ret;
    }
    if( !src || src[0] == '\0') {
        blank = true;
        src = srcblank;
        kernelname="blank";
    }else{
        if(NULL == strstr(src, "__kernel")) { // treat it as a file name
            FILE *f = fopen(src, "r");
            if(!f) {
                LOGMSG(("Failed to open src file %s", src))
            }
            fseek(f, 0, SEEK_END);
            int n = ftell(f), m;
            rewind(f);
            srcbuf.resize(n+1);
            if(n != (m=fread(&srcbuf[0], sizeof(char), n, f) )) {
                LOGMSG__("Error! read file length %d != %d", m, n);
            }
            fclose(f);
            srcbuf[n] = '\0';
            src = &srcbuf[0];
        }
    }
    for(int i=0;i<_platforms.size();++i) {
        if( i!=platformIdx && platformIdx!= -1) continue;
        os << "---------------- Platform :" << i << " --------------" << endl;
        contxt = boost::make_shared<ClContext>(&_platforms[i], 0, (ClDevice*) NULL);
        if( contxt->errcode ) {
            LOGMSG(("Failed to create context\n"))
            continue;
        }
        if( program = contxt->createProgram(src, false, NULL, os) ) {
            krn = program->createKernel(kernelname);
            if( krn ) {
                ret.push_back(krn);
                for(int k=0; k < contxt->devices.size(); ++k) {
                    ClDevice *device = contxt->devices[k];
                    device->setKernelWorkGroupInfo(krn->_kernel, device->id);
                    os << "--- Device " << k << " ---" << endl;
                    os << "  - Device Infos: name:" << device->name << " vendorName:" << device->vendorName << endl
                       << "    maxComputeUnits       : " << device->maxComputeUnits << endl
                       << "    maxWorkItemDims       : " << device->maxWorkItemDims << endl
                       << "    maxWorkItemSizes      : " << device->maxWorkItemSizes[0]
                               << "," <<device->maxWorkItemSizes[1] <<"," << device->maxWorkItemSizes[2]<< endl
                       << "    maxWorkGroupSize      : " << device->maxWorkGroupSize << endl
                       << "    maxMemAllocSize       : " << device->maxMemAllocSize << endl
                       << "    localMemSize          : " << device->localMemSize << endl;
                    os << "  - Kernel Info" << endl
                       << "    kernelWorkGroupSize   : " << device->kernelWorkGroupSize << endl
                       << "    kernelGlobalWorkSize  : " << device->kernelGlobalWorkSize[0]
                       << "," << device->kernelGlobalWorkSize[1] << "," << device->kernelGlobalWorkSize[2] << endl
                       << "    localMemoryUsed       : " << device->localMemoryUsed << endl
                       << "    kernelLocalMemSize  : " << device->kernelLocalMemSize << endl
                       << "    kernelPrivateMemSize  : " << device->kernelPrivateMemSize << endl
                       << "    preferredWorkGroupSizeMultiple  : " << device->preferredWorkGroupSizeMultiple << endl;
                }
            }
        }
        os << "+++++++++++++++ end Platform :" << i << " +++++++++++++++" << endl;
    }
    return ret;
}

//=============================================================================
//// Dynamic Info
ClContext::ClContext(ClPlatform* p, size_t ndev, ClDevice *devs )
    :platform(p)
{
    std::vector<cl_device_id> devids;
    if(ndev) {
        for(int i=0;i<ndev; ++i) devids.push_back(devs[i].id);
    }else{
        devids = p->getDeviceIDs();
    }
    _context = clCreateContext(NULL, devids.size(), &devids[0], NULL, NULL, &errcode);
    if( !errcode ) {
        devices = p->getDevices(devids);
    }
}

ClProgramPtr ClContext::createProgram(const char *src, bool fromFile, const char *cflag, std::ostream& os)
{
   ClProgramPtr ret = boost::make_shared<ClProgram>(this->shared_from_this(), src, fromFile, cflag);
   if( ret->errcode ){
       for(int i=0; i<ret->buildLog.size();++i) {
           os << "Build Info " << i << "[" << endl << ret->buildLog[i] << "]" << endl;
       }
       ret.reset();
   }else
       addProgram(ret);;

   return ret;
}
ClProgramPtr ClContext::createProgram(std::vector<string>& vsrc, std::vector<ClDevice*>& vdev, bool fromFile, const char *cflags, std::ostream& os)
{
    ClProgramPtr ret = boost::make_shared<ClProgram>(this->shared_from_this(), vsrc, vdev, fromFile, cflags);
    if( ret->errcode ){
        for(int i=0; i<ret->buildLog.size();++i) {
            os << "Build Info " << i << "[" << endl << ret->buildLog[i] << "]" << endl;
        }
        ret.reset();
    }else
        addProgram(ret);;

    return ret;
}

ClCmdQPtr ClContext::createCmdQ(ClDevice* dev)
{
   ClCmdQPtr ret = boost::make_shared<ClCmdQ>(this->shared_from_this(), dev);
   if( ret->errcode )
       ret.reset();
   else
       addCmdQ(ret);
   return ret;
}

ClMemPtr ClContext::createMem(cl_mem_flags flags, size_t bufsize, void* buf)
{
   ClMemPtr ret = boost::make_shared<ClMem>(this->shared_from_this(), flags, bufsize, buf);
   if( ret->errcode )
       ret.reset();
   else
       addMem(ret);
   return ret;
}

ClProgram::ClProgram(ClContextPtr ctxt, const char *src, bool fromFile, const char *cflags)
    : pContext(ctxt)
{
    vector<vector<char> > vsrc(1);
    vector<char*>         psrc(1);
    if( fromFile ) {
        for(int i=0; i < 1; ++i ) {
            FILE *f = fopen(src, "r");
            if(!f) {
                LOGMSG(("Failed to open src file %s to compile\n", src))
                errcode = -1;
                return;
            }
            fseek(f, 0, SEEK_END);
            size_t n = ftell(f);
            vsrc[i].resize(n+1);
            fread(&vsrc[i][0], sizeof(char), n, f);
            fclose(f);
            vsrc[i][n] = '\0';
            psrc[i] = &vsrc[i][0];
        }
    }else{
        for(int i=0; i < 1; ++i ) {
            size_t n = strlen(src);
            vsrc[i].resize(n +1 );
            strcpy(&vsrc[i][0], src);
            vsrc[i][n] = '\0';
            psrc[i] = &vsrc[i][0];
        }
    }
    compile(psrc.size(), (const char**) &psrc[0], 0, (ClDevice **) NULL, cflags);

}
ClProgram::ClProgram(ClContextPtr ctxt, const std::vector<string>& fsrc, const std::vector<ClDevice*>& vdev, bool fromFile, const char *cflags)
    : pContext(ctxt)
{
    vector<vector<char> > vsrc(fsrc.size());
    vector<char*>         psrc(fsrc.size());
    if( fromFile ) {
        for(int i=0; i < fsrc.size(); ++i ) {
            FILE *f = fopen(fsrc[i].c_str(), "r");
            if(!f) {
                LOGMSG(("Failed to open src file %s to compile\n", fsrc[i].c_str()))
                errcode = -1;
                return;
            }
            fseek(f, 0, SEEK_END);
            size_t n = ftell(f);
            vsrc[i].resize(n+1);
            fread(&vsrc[i][0], sizeof(char), n, f);
            fclose(f);
            vsrc[i][n] = '\0';
            psrc[i] = &vsrc[i][0];
        }
    }else{
        for(int i=0; i < fsrc.size(); ++i ) {
            size_t n = fsrc[i].size();
            vsrc[i].resize(n +1 );
            std::copy(fsrc[i].begin(), fsrc[i].end(), vsrc[i].begin());
            vsrc[i][n] = '\0';
            psrc[i] = &vsrc[i][0];
        }
    }
    compile(psrc.size(), (const char**) &psrc[0], vdev.size(), (ClDevice **) &vdev[0], cflags);

}

cl_int ClProgram::compile(size_t nsrc, const char **src, size_t ndev, ClDevice **vdev, const char *cflags)
{
   vector<size_t> Lens(nsrc);
   for(int i=0; i< nsrc; ++i) {
       Lens[i] = strlen(src[i]) + 1;
   }
   _program = clCreateProgramWithSource(pContext->_context, nsrc, src, &Lens[0], &errcode);
   if( errcode ) {
       LOGMSG(("Failed to create program with source!\n"))
       return errcode;
   }
   if( ndev == 0 ){
       devices = pContext->devices;
   }else{
       devices = vector<ClDevice*>(vdev, vdev+ndev);
   }
   std::vector<cl_device_id> ids = ClPlatform::getDeviceIDs(devices);
   if( ids.size() == 0) {
       LOGMSG(("Failed to find any devices to compile\n"))
       return errcode = -1;
   }

   errcode = clBuildProgram(_program, ids.size(), &ids[0], cflags, NULL, NULL);
   buildLog.resize(devices.size());
   if( errcode ) {
       for(int i=0;i<devices.size();++i) {
           size_t log_size;
           clGetProgramBuildInfo(_program, devices[i]->id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
           vector<char> log(log_size+1);
           clGetProgramBuildInfo(_program, devices[i]->id, CL_PROGRAM_BUILD_LOG, log_size, &log[0], NULL);
           log[log_size] = '\0';
           buildLog[i] = &log[0];
       }
   }
   return errcode;
}

ClKernelPtr ClProgram::createKernel(const char *name)
{
   ClKernelPtr ret = boost::make_shared<ClKernel>(this->shared_from_this(), name);
   if( ret->errcode )
       ret.reset();
   else
       addKernel(ret);
   return ret;
}

ClKernel::ClKernel(ClProgramPtr program, const char* name)
{
   _kernel = clCreateKernel(program->_program, name, &errcode);
   if( errcode ) {
       LOGMSG(("Failed to create kernel %s\n", name))
       return;
   }
   pProgram = program;

   workGroupInfos.resize(pProgram->devices.size());
   for(int i=0;i < pProgram->devices.size(); ++i) {
       workGroupInfos[i].setKernelWorkGroupInfo(_kernel, pProgram->devices[i]->id);
   }
}
cl_int ClKernel::setArg(size_t idx, const ClMemPtr& m)
{
    return clSetKernelArg(_kernel, idx, sizeof(cl_mem), &m->_mem);
}
cl_int ClKernel::setArg(size_t idx, cl_int v)
{
    return clSetKernelArg(_kernel, idx, sizeof(cl_int), &v);
}
cl_int ClKernel::setArg(size_t idx, cl_uint v)
{
    return clSetKernelArg(_kernel, idx, sizeof(cl_uint), &v);
}

cl_int ClKernel::setArgLocal(size_t idx, size_t v) // local mem
{
    return clSetKernelArg(_kernel, idx, v, NULL);
}

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
cl_int ClKernel::validSizes(int deviceId, size_t dim, size_t *globalsize, size_t *localsize, size_t localmem)
{
    cl_int ret = CL_SUCCESS;
    cl_ulong availableLocalMemory = pProgram->devices[deviceId]->localMemSize - workGroupInfos[deviceId].localMemoryUsed;
    if( availableLocalMemory < localmem) return ++ret;
    if( !localsize ) return ret;
    size_t prod = 1;
    for(int i=0; i<dim; ++i){
        if(localsize[i] > pProgram->devices[deviceId]->maxWorkItemSizes[i]) return ++ret;
        if(localsize[i]>0){
            if( globalsize[i]%localsize[i] != 0 ) return ++ret;
            prod *= localsize[i];
        }else{
            return ++ret;
        }
    }
    if(prod > workGroupInfos[deviceId].kernelWorkGroupSize) return ++ret;
    return ret;
}

ClCmdQ::ClCmdQ(ClContextPtr contxt, ClDevice *dev)
{
   _cmdQ = clCreateCommandQueue(contxt->_context, dev->id, NULL, &errcode);
   if( errcode ) {
       LOGMSG(("Failed to create CommandQ\n"))
       return;
   }
   pDevice= dev;
   pContext = contxt;
}
cl_int ClCmdQ::flush()
{
    return clFlush(_cmdQ);
}

cl_int ClCmdQ::enqueue(ClKernelPtr kernel, size_t dim, const size_t *global_work_offset, const size_t *global_work_size, const size_t *local_work_size,
               cl_uint num_events_in_wait_list, const cl_event *event_wait_list,cl_event *event)
{
    return clEnqueueNDRangeKernel(_cmdQ, kernel->_kernel, dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, event);
}

cl_int ClCmdQ::enqueueWrite(ClMemPtr& buffer, cl_bool blocking, size_t offset,size_t cb, const void *ptr,
                     cl_uint num_events_in_wait_list, const cl_event *event_wait_list, cl_event *event)
{
    return clEnqueueWriteBuffer(_cmdQ, buffer->_mem, blocking, offset, cb, ptr, num_events_in_wait_list, event_wait_list, event);
}

cl_int ClCmdQ::enqueueRead(ClMemPtr& buffer, cl_bool blocking, size_t offset,size_t cb, void *ptr,
                   cl_uint num_events_in_wait_list, const cl_event *event_wait_list, cl_event *event)
{
    return clEnqueueReadBuffer(_cmdQ, buffer->_mem, blocking, offset, cb, ptr, num_events_in_wait_list, event_wait_list, event);
}

cl_int ClCmdQ::enqueueCopy(ClMemPtr& src, ClMemPtr& dest, size_t src_offset, size_t dest_offset, size_t cb,
                   cl_uint num_events_in_wait_list, const cl_event *event_wait_list, cl_event *event)
{
    return clEnqueueCopyBuffer(_cmdQ, src->_mem, dest->_mem, src_offset, dest_offset, cb, num_events_in_wait_list, event_wait_list, event);
}
cl_int ClCmdQ::enqueueBarrier()
{
    return clEnqueueBarrier(_cmdQ);
}
cl_int ClCmdQ::enqueueMap(ClMemPtr& buffer, cl_bool blocking, cl_map_flags flags, size_t offset, size_t cb, void** ptr,
                 cl_uint num_events_in_wait_list, const cl_event *event_wait_list, cl_event *event)
{
    cl_int status;
    *ptr = clEnqueueMapBuffer(_cmdQ, buffer->_mem, blocking, flags, offset, cb, num_events_in_wait_list, event_wait_list, event, &status);
    return status;
}
cl_int ClCmdQ::enqueueUnmap(ClMemPtr& buffer, void *ptr,
                    cl_uint num_events_in_wait_list, const cl_event *event_wait_list, cl_event *event)
{
    return clEnqueueUnmapMemObject(_cmdQ, buffer->_mem, ptr, num_events_in_wait_list, event_wait_list, event);
}

ClMem::ClMem(ClContextPtr contxt, cl_mem_flags flags, size_t bufsize, void* buf )
   :buf(buf), bufsize(bufsize), flags(flags)
{
   _mem = clCreateBuffer(contxt->_context, flags, bufsize, buf, &errcode);
   if( errcode ) {
       LOGMSG(("Failed to create Buffer\n"))
       return;
   }
   pContext = contxt;
}


cl_int WaitForEvent(cl_event evt, cl_int evt_state, bool release)
{
    cl_int eventStatus = CL_QUEUED;
    cl_int status = CL_SUCCESS;
    while(eventStatus != evt_state)
    {
        status = clGetEventInfo(
                    evt,
                    CL_EVENT_COMMAND_EXECUTION_STATUS,
                    sizeof(cl_int),
                    &eventStatus,
                    NULL);
        CHECK_OPENCL_ERROR(status, "clGetEventInfo failed.");
    }
    if(release)
        return clReleaseEvent(evt);
    return status;
}
