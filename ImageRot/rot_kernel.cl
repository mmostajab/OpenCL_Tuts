//#define USE_IMAGE

#ifdef USE_IMAGE
__constant sampler_t sampler =
  CLK_NORMALIZED_COORDS_FALSE
| CLK_ADDRESS_CLAMP_TO_EDGE
| CLK_FILTER_NEAREST;
#endif

__kernel void rotate_img(
#ifdef USE_IMAGE
  __write_only image2d_t output,
  __read_only image2d_t input,
#else
  __global char* output,
  __global char* input,
#endif
  int width, int height,
  float cosThetha, float sinThetha) {

    const int2 pos = { get_global_id(0), get_global_id(1) };

    if(pos.x >= width || pos.y >= height) return;

    float x0 = width  / 2.0f;
    float y0 = height / 2.0f;

    float xoffset = pos.x - x0;
    float yoffset = pos.y - y0;
    int xpos = (int)(xoffset*cosThetha+yoffset*sinThetha+x0);
    int ypos = (int)(xoffset*cosThetha-yoffset*sinThetha+y0);

    if(((int)xpos >= 0) && ((int)xpos < width) && ((int)ypos >= 0) && ((int)ypos < height)){
#ifdef USE_IMAGE
      float4 c = read_imagef(input, sampler, (int2)(xpos, ypos));
      write_imagef(output, (int2)(pos.x, pos.y), c);
#else
      output[3*(pos.y*width+pos.x)+0] = input[3*(ypos*width+xpos)+0];
      output[3*(pos.y*width+pos.x)+1] = input[3*(ypos*width+xpos)+1];
      output[3*(pos.y*width+pos.x)+2] = input[3*(ypos*width+xpos)+2];

    } else {

      output[3*(pos.y*width+pos.x)+0] = 0;
      output[3*(pos.y*width+pos.x)+1] = 0;
      output[3*(pos.y*width+pos.x)+2] = 0;
#endif
    }
}