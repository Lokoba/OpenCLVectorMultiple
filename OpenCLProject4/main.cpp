#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
 
using namespace std;

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
 
#define MEM_SIZE (128)
#define MAX_SOURCE_SIZE (0x100000)

// заполняем массив значениями
void shrFillArray(float* pfData, int iSize)
{
    int i; 
    const float fScale = 1.0f / (float)RAND_MAX;
    for (i = 0; i < iSize; ++i) 
    {
        pfData[i] = fScale * rand();
    }
}

// округление
size_t shrRoundUp(int group_size, int global_size) 
{
    int r = global_size % group_size;
    if(r == 0) 
    {
        return global_size;
    } else 
    {
        return global_size + group_size - r;
    }
}

 
int main()
{
	cl_device_id device_id = NULL;
	cl_context context = NULL;
	cl_command_queue command_queue = NULL;
	cl_mem memobj = NULL;
	cl_mem cmDevSrcA;               // буффер с массивом A
	cl_mem cmDevSrcB;               // буффер с массивом B 
	cl_mem cmDevDst;                // результирующий буффе
	cl_program program = NULL;
	cl_kernel kernel = NULL;
	cl_platform_id platform_id = NULL;
	cl_uint ret_num_devices;
	cl_uint ret_num_platforms;
	cl_int ret;
	size_t szGlobalWorkSize;        // общее количество работающих итемов
	size_t szLocalWorkSize;		    // количество работающих итемов в группе	



	int iNumElements = 11444777;	// длина массивов 11444777

	char string[MEM_SIZE];
	float *srcA, *srcB;        // Host для OpenCL 
	void* Golden;             

	float *dst;
	szLocalWorkSize = 256;
	szGlobalWorkSize = shrRoundUp((int)szLocalWorkSize, iNumElements);  
	srcA = (float *)malloc(sizeof(cl_float) * szGlobalWorkSize);
	srcB = (float *)malloc(sizeof(cl_float) * szGlobalWorkSize);
	dst = (float *)malloc(sizeof(cl_float) * szGlobalWorkSize);
	Golden = (void *)malloc(sizeof(cl_float) * iNumElements);
	shrFillArray((float*)srcA, iNumElements);
	shrFillArray((float*)srcB, iNumElements);
	dst[0] = 0;


	FILE *fp;
	char fileName[] = "./hello.cl";
	char *source_str;
	size_t source_size;
 
	/* загружаем исходный код kernel */
	fp = fopen(fileName, "r");
	if (!fp) {
	fprintf(stderr, "Failed to load kernel.\n");
	exit(1);
	}
	source_str = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	fclose(fp);
 
	/* получаем информацию о девайсе и платформе */
	ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);
 
	/* создаем OpenCL контекст */
	context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
 
	/* создаем Command Queue */
	command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
 
	/* создаем буффер памяти */
	memobj = clCreateBuffer(context, CL_MEM_READ_WRITE,MEM_SIZE * sizeof(cl_float), NULL, &ret);
	cmDevSrcA = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_float) * szGlobalWorkSize, NULL, &ret);
	cmDevSrcB = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_float) * szGlobalWorkSize, NULL, &ret);
	cmDevDst = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float) * szGlobalWorkSize, NULL, &ret);
 
	/* создаем программу кернелла из файла */
	program = clCreateProgramWithSource(context, 1, (const char **)&source_str,
	(const size_t *)&source_size, &ret);
 
	/* построение Kernel Program */
	ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
 
	/* создание OpenCL Kernel */
	kernel = clCreateKernel(program, "hello", &ret);
 
	/* устанавливаем параметры Kernel */
	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&cmDevSrcA);
	ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&cmDevSrcB);
	ret |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&cmDevDst);
	ret |= clSetKernelArg(kernel, 3, sizeof(cl_int), (void*)&iNumElements);
	
	/* записываем массивы в буффер памяти */ 
	ret = clEnqueueWriteBuffer(command_queue, cmDevSrcA, CL_FALSE, 0, sizeof(cl_float) * szGlobalWorkSize, srcA, 0, NULL, NULL);
	ret |= clEnqueueWriteBuffer(command_queue, cmDevSrcB, CL_FALSE, 0, sizeof(cl_float) * szGlobalWorkSize, srcB, 0, NULL, NULL);
	ret |= clEnqueueWriteBuffer(command_queue, cmDevDst, CL_FALSE, 0, sizeof(cl_float) * szGlobalWorkSize, dst, 0, NULL, NULL);


	/* выполняем OpenCL Kernel */
	ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,&szGlobalWorkSize, &szLocalWorkSize, 0, NULL, NULL);
 
	/* получаем результаты из буфера памяти */
	ret = clEnqueueReadBuffer(command_queue, cmDevDst, CL_TRUE, 0,
							  sizeof(cl_float) * szGlobalWorkSize, dst, 0, NULL, NULL);
	
 
	/* Display Result */
	/*for ( int i = 0; i < iNumElements; i++){
		printf("srcA[%d] = %f\n", i, srcA[i]);
	}
	for ( int i = 0; i < iNumElements; i++){
		printf("srcB[%d] = %f\n", i, srcB[i]);
	}*/
	float sum = 0;
	for( int i = 0; i < iNumElements; i++){
		//printf("result[%d] = %f\n", i, dst[i]);
		sum += dst[i];
		//printf("result = %f\n", sum);
	}
	printf("result = %f\n", sum);
	system("pause");

	/* Освобождаем память */
	ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(memobj);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);
	if(cmDevSrcA)clReleaseMemObject(cmDevSrcA);
	if(cmDevSrcB)clReleaseMemObject(cmDevSrcB);
	if(cmDevDst)clReleaseMemObject(cmDevDst);

 
	free(source_str);
	free(srcA); 
	free(srcB);
	free (dst);
	free(Golden);
	//free(result);

	return 0;
}

