
#include <SDKApplication.hpp>

#include <float.h>
#include <vector>
#include <map>

#include <boost/program_options.hpp>

#include <clutil.h>
#include <CMat.h>

using namespace std;

#define PROGRAM_FILE "matvec.cl"
#define KERNEL_FUNC "matvec_mult"

#define NROUND 64

bool Near(float t1, float t2, float div = 10000)
{
    return  fabs(t1-t2) < t1/div;
}

class MatVec: public SDKSample
{
public:
    ClContextPtr   pContext;
    ClKernelPtr pKernel, pKrnMult, pKrnSum, pKernelMatMat;
    ClCmdQPtr   pCmdQ;
    ClProgramPtr pProgram;
    ClDevice     *pDevice;
    const cl_uint M, N, K;
    cl_uint       RM, RN, RK;
    CMat<float> mat, vec, result, correct;
    ClMemPtr mat_mem, vec_mem, res_mem;
    int timer;
    cl_double gpuTime, cpuTime;
    MatVec(const char *name):
        SDKSample(name)
//            , M(8), N(10), K(10)
      ,M(1024), N(1024), K(1024)
      , RM(clCeiling<NROUND>(M)), RN(clCeiling<NROUND>(N)), RK(clCeiling<NROUND>(K))
      , mat((unsigned long)RM,(unsigned long)RN)
      , vec((unsigned long)RN, (unsigned long)RK)
      , result((unsigned long)RM, (unsigned long)RK)
      , correct((unsigned long)M, (unsigned long)K){}

    virtual int setup()
    {
        std::vector<ClKernelPtr> vecKrn;
        vecKrn = ClPlatform::createSimpleKernel(std::cout, platformId, PROGRAM_FILE, "matmat_mult");
        if(vecKrn.size() == 0) {
            printf("Failed to create kernel!\n");
            return SDK_FAILURE;
        }
        pKernel = vecKrn[0];
        pProgram = pKernel->pProgram;
        pContext =  pProgram->pContext;
        pDevice = pContext->devices[deviceId];
        pCmdQ = pContext->createCmdQ(pDevice);
        if( sizeof(float)*(M*N+N*K+M*K) >= pContext->devices[deviceId]->maxMemAllocSize ) {
            printf("Larger than device maxMemAllocSize %ld >= %ld\n", sizeof(float)*(M*N+N*K+M*K), pDevice->maxMemAllocSize);
            return SDK_FAILURE;
        }
        if( NULL == (pKernelMatMat = pProgram->createKernel("matmat_mult") ) ) {
            cout << "failed to created kernel matmat_mult" << endl;
            return SDK_FAILURE;
        }

        if( NULL == (pKrnMult = pProgram->createKernel("matvecmult_mult") ) ) {
            cout << "failed to created kernel matvecmult_mult" << endl;
            return SDK_FAILURE;
        }
        if( NULL == (pKrnSum = pProgram->createKernel("matvecmult_sum") ) ) {
            cout << "failed to created kernel matvecmult_sum" << endl;
            return SDK_FAILURE;
        }

        /* Initialize data to be processed by the kernel */
        mat = (float)0.0;
        vec = (float)0.0;
        for(int i=0; i<N; ++i) {
            for(int j=0; j<M; ++j)
                mat(j,i)  = (i+j) *2.0;
            for(int j=0; j<K; ++j)
                vec(i,j) = i*3.0;
        }
        timer = sampleCommon->createTimer();
        sampleCommon->resetTimer(timer);

        sampleCommon->startTimer(timer);
        correct = mat * vec;
        double t = mat(0,0) *vec(0,0);
//        for(int i=0;i<M; ++i)
//            for(int j=0; j<K; ++j) {
//                float *cor = &correct(i,j), *a = &mat(i,0);
//                cor[0] = 0;
//                for(int k=0; k<N; ++k, ++a)
//                    cor[0] += a[0] * vec.ptr()[k*N + j];
//            }
        sampleCommon->stopTimer(timer);
        cpuTime = sampleCommon->readTimer(timer);
        sampleCommon->resetTimer(timer);
        cout << "Matrix size: (" <<M << "," <<N <<"," <<K << ")    CPU time:" << cpuTime << endl;


        mat_mem = pContext->createMem(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(float)*RM*RN, &mat[0]);
        if( ! mat_mem ) {
            printf("Failed to create mat_mem!\n");
            return SDK_FAILURE;
        }
        vec_mem = pContext->createMem(CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float)*RN*RK, &vec[0]);
        res_mem = pContext->createMem(CL_MEM_WRITE_ONLY, sizeof(float)*RM*RK, NULL);
        if( ! res_mem ) {
            printf("Failed to create res_mem!\n");
            return SDK_FAILURE;
        }


        return SDK_SUCCESS;

    }

    virtual int initialize()
    {
        // Call base class Initialize to get default configuration
        if(this->SDKSample::initialize())
            return SDK_FAILURE;
        return SDK_SUCCESS;
    }
    int runMatVec()
    {
        size_t globalsize;
        sampleCommon->startTimer(timer);
        if( CL_SUCCESS  != pKernel->setArgs(0, mat_mem, vec_mem, res_mem, M, N) ) {
            printf("Failed to set arg!\n");
            return SDK_FAILURE;
        }
        globalsize = M;
        cl_event evts[3];
        pCmdQ->enqueue(pKernel, 1, NULL, &globalsize, NULL, 0, NULL, &evts[0] );
        pCmdQ->enqueueRead(res_mem, CL_FALSE, 0, sizeof(float)*M, &result[0], 1, &evts[0], &evts[1]);
        clWaitForEvents(1, &evts[1]);
        for(int i=0; i < 2; ++i)
            clReleaseEvent(evts[i]);

        sampleCommon->stopTimer(timer);
        cl_double gpuTime = sampleCommon->readTimer(timer);
        cout << " Device Timing:" << gpuTime << endl;
        return SDK_SUCCESS;
    }
    int runMatMat()
    {
        cl_int status=CL_SUCCESS;
        bool USE_MAP = true;

        const size_t NDIM = 2;
        size_t globalsize[NDIM] = {RM/4, RK/4};
        size_t localsize[NDIM] = {16, 16};

        ExecKernel exKrn(pKernelMatMat, pCmdQ, NDIM, globalsize, localsize);
        MapMem     exMap(pKernelMatMat, pCmdQ, res_mem);
        ReadWriteMem    exRead(pKernelMatMat, pCmdQ, res_mem, &result[0], true);

        status = pKernelMatMat->validSizes(deviceId, NDIM, globalsize, localsize);
        CHECK_OPENCL_ERROR(status, "invalid localsize");

        sampleCommon->resetTimer(timer);
        sampleCommon->startTimer(timer);
        status = pKernelMatMat->setArgs(0, mat_mem, vec_mem, res_mem, RM, RN, RK);
        CHECK_OPENCL_ERROR(status, "Failed to set Args");
        CHECK_OPENCL_ERROR(exKrn(), "Failed to execute kernel");

        if( !USE_MAP ) {
            CHECK_OPENCL_ERROR(exRead(), "Failed to read mem");
        }else{

            CHECK_OPENCL_ERROR(exMap.map(), "Failed to map read mem");
            memcpy(&result[0], exMap.ptr, sizeof(cl_float) * RM  * RK);
            CHECK_OPENCL_ERROR(exMap.unmap(), "Failed to unmap read mem");
        }


        sampleCommon->stopTimer(timer);
        cl_double gpuTime = sampleCommon->readTimer(timer);
        cout << " Device Timing:" << gpuTime << endl;
        return SDK_SUCCESS;
    }
    int runMultiStage()
    {
        size_t globalsize;
        /* Data and buffers */
        if( CL_SUCCESS  != pKrnMult->setArgs(0, mat_mem, vec_mem,  M, N) )
        {
            printf("Failed to set arg!\n");
            return SDK_FAILURE;
        }
        if( CL_SUCCESS  != pKrnSum->setArgs(0, mat_mem, res_mem,  M, N) )
        {
            printf("Failed to set arg!\n");
            return SDK_FAILURE;
        }

        globalsize= M*N;
        cl_event evts[3];

        pCmdQ->enqueue(pKrnMult, 1, NULL, &globalsize, NULL, 0, NULL, &evts[0]);
        globalsize= M;
        pCmdQ->enqueue(pKrnSum, 1, NULL, &globalsize, NULL, 1, &evts[0], &evts[1]);

        pCmdQ->enqueueRead(res_mem, CL_TRUE, 0, sizeof(float)*M, &result[0], 2, &evts[0], &evts[2]);

        pCmdQ->flush();

        clWaitForEvents(1, &evts[2]);
        for(int i=0; i < 3; ++i)
            clReleaseEvent(evts[i]);

        sampleCommon->stopTimer(timer);
        gpuTime = sampleCommon->readTimer(timer);
        cout << " Device Timing:" << gpuTime << endl;
        return SDK_SUCCESS;
    }

    virtual int run() {
        if( K == 1 ){
            //            return runMatVec();
            //            return runMultiStage();
            return runMatMat();
        }else{
            return runMatMat();
        }
    }

    virtual int verifyResults(){
        bool ok = true;
        for(int i=0;i<M; ++i) {
            for(int j=0;j<K; ++j)
                if( !Near(result(i,j), correct(i,j)) ) {
                    ok = false;
                    printf("(%d,%d) ERROR %f != %f \n", i, j,result(i,j), correct(i,j));
                }else{
//                    printf("(%d,%d) ERROR %f = %f \n", i, j,result(i,j), correct(i,j));
                }
        }
        if( ok) printf("GPU computation is correct!\n");
        return SDK_SUCCESS;
    }
    virtual int genBinaryImage(){
        return SDK_SUCCESS;
    }
    virtual int cleanup(){
        return SDK_SUCCESS;
    }
    void printStats()
    {
    }
};

int main(int argc, char *argv[])
{
//    ClPlatform::createSimpleKernel(std::cout);
    MatVec app("OpenCL MatVec");

    if(app.initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    if(app.parseCommandLine(argc, argv) != SDK_SUCCESS)
        return SDK_FAILURE;

    if(app.isDumpBinaryEnabled())
    {
        return app.genBinaryImage();
    }
    else
    {
        if(app.setup() != SDK_SUCCESS)
            return SDK_FAILURE;

        if(app.run() != SDK_SUCCESS)
            return SDK_FAILURE;

        if(app.verifyResults() != SDK_SUCCESS)
            return SDK_FAILURE;

        if(app.cleanup() != SDK_SUCCESS)
            return SDK_FAILURE;

        app.printStats();
    }

    return SDK_SUCCESS;

}
