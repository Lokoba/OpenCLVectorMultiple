
// TODO: Add OpenCL kernel code here.

__kernel void hello(__global const float* a, __global const float* b, __global float *c, int iNumElements)
{
	// получаем индекс в глобальном массиве данных
    int iGID = get_global_id(0);


    // имитируем for
    if (iGID >= iNumElements)
    {   
      //  c[0] = n;
		return; 
    }
    
    // получаем суммы для скалярного произведения
    c[iGID] = a[iGID] * b[iGID];
	//n += a[iGID] * b[iGID];
}