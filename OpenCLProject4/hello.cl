
// TODO: Add OpenCL kernel code here.

__kernel void hello(__global const float* a, __global const float* b, __global float *c, int iNumElements)
{
	// �������� ������ � ���������� ������� ������
    int iGID = get_global_id(0);


    // ��������� for
    if (iGID >= iNumElements)
    {   
      //  c[0] = n;
		return; 
    }
    
    // �������� ����� ��� ���������� ������������
    c[iGID] = a[iGID] * b[iGID];
	//n += a[iGID] * b[iGID];
}