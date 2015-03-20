__kernel void convolution( 
  __read_only image2d_t sourceImage,
  __write_only image2d_t outputImage,
  int rows, int cols,
  __constant float* filter,
  int filterWidth,
  float kernel_integral,
  sampler_t sampler) {

    int2 coords = { get_global_id(0), get_global_id(1) };

    int halfWidth = (int)( filterWidth / 2 );

    float4 sum = { 0.0f, 0.0f, 0.0f, 0.0f };

    int filterIdx = 0;
    int2 offset_coords;
    for(int i = -halfWidth; i <= halfWidth; i++){

      offset_coords.y = coords.y + i;

      for(int j = -halfWidth; j <= halfWidth; j++){
        
        offset_coords.x = coords.x + j;
        float4 pixel = read_imagef( sourceImage, sampler, offset_coords );
        sum += pixel * filter[filterIdx++];

      }

    }
    sum /= kernel_integral;
    if(coords.x < rows && coords.y < cols){
      write_imagef( outputImage, coords, sum );
    }
}