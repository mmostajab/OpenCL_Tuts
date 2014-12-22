__kernel 										
void vecadd(	__global int* A,				
				__global int* B,				
				__global int* C) 				
{												
	// Get the work item unique id 				
	int idx = get_global_id(0);					
												
	// Add the corresponding location of 		
	// 'A' and 'B' and store the result in 'C'	
	C[idx] = A[idx] + B[idx];					
}												