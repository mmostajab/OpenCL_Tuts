__kernel void simplyMultiply(
  __global float* outputC,
  int widthA,
  int heightA,
  int widthB,
  int heightB,
  __global float* inputA, 
  __global float* inputB) {

    int row = get_global_id(1);
    int col = get_global_id(0);

    if( row >= heightA || row >= heightB || col >= widthA || col >= widthB )
      return;

    float sum = 0.0f;

    for(int i = 0; i < widthA; i++)
      sum += inputA[row*widthA + i] * inputB[i*widthB+col];

    outputC[row*widthB+col] = sum;    
}