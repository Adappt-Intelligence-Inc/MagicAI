

#include "rgba_bitmap.h"
#include <stdlib.h>
#include <string.h>


/*
 
    The format of the RGBA file is *very simple*. There's a magic number, which is the
    characters 'RGBA' in that order. Then a 4-byte width and then a 4-byte height, in
    that order, both stored in big-endian format. Then follows the bitmap data, which
    is RGBA quads, in that order: R, G, B, A bytes for each pixel. (This is sometimes
    referred to as R8G8B8A8.) First pixel is (0,0) at upper left, and they go across
    the first line, then the next line, etc, to the end. There is no padding, and
    every pixel is naturally 32-bit aligned.
 
    The majority of the code in this file is there to provide convenience tranforms
    to other in-memory buffer formats. The file itself is SUPER SIMPLE.
 
 */


#define RGBA_BITMAP_MAGIC_NUMBER            0x52474241   /* 'RGBA' */

#define READ_BIG_ENDIAN_UINT32(ptr)         (*ptr << 24 | *(ptr+1) << 16 | *(ptr+2)<<8 | *(ptr+3))
#define WRITE_BIG_ENDIAN_UINT32(ptr, u32)   *ptr = (u32 >> 24) & 0xFF; \
                                            *(ptr+1) = (u32 >> 16) & 0xFF; \
                                            *(ptr+2) = (u32 >> 8) & 0xFF; \
                                            *(ptr+3) = u32 & 0xFF;


unsigned char * encode_to_rgbaMagic(
    const unsigned char * input_buffer,
    unsigned long width,
    unsigned long height,
    size_t input_row_size,
    bitmap_buffer_format input_format,
    size_t * p_output_size
)
{
    if (!input_buffer || !p_output_size || width == 0 || height == 0) {
        return NULL;
    }
    
    size_t input_pixel_size = 4;
    if (input_format == bitmap_buffer_format_RGB ||
        input_format == bitmap_buffer_format_BGR) {
        input_pixel_size = 3;
    }
    if (input_row_size == 0) {
        input_row_size = input_pixel_size * width;
    } else if (input_row_size < input_pixel_size * width) {
        return NULL;
    }

    *p_output_size = 0;
    
    size_t output_size = 12 + (width * height * 4);
    unsigned char * rgba_file = ( unsigned char * ) malloc(output_size);
    if (!rgba_file) {
        return NULL;
    }
    
    WRITE_BIG_ENDIAN_UINT32(&rgba_file[0], RGBA_BITMAP_MAGIC_NUMBER);
    WRITE_BIG_ENDIAN_UINT32(&rgba_file[4], width);
    WRITE_BIG_ENDIAN_UINT32(&rgba_file[8], height);
    
    if (input_format == bitmap_buffer_format_RGBA && input_row_size == (width * 4)) {
        /* Fast path if the desired buffer is the exact same format as the file data */
        memcpy(&rgba_file[12], input_buffer, (width * height * 4));
    } else {
        /* Transform the input bitmap data to the standard RGBA format */
        unsigned char * output_ptr = &rgba_file[12];
        for (unsigned long y = 0; y < height; y++) {
            const unsigned char * input_row_start = &input_buffer[y * input_row_size];
            const unsigned char * input_ptr = input_row_start;
            for (unsigned long x = 0; x < width; x++) {
                unsigned char r, g, b, a;
                switch (input_format) {
                    case bitmap_buffer_format_RGBA:
                        r = *input_ptr++; g = *input_ptr++; b = *input_ptr++; a = *input_ptr++;
                        break;
                    case bitmap_buffer_format_ABGR:
                        a = *input_ptr++; b = *input_ptr++; g = *input_ptr++; r = *input_ptr++;
                        break;
                    case bitmap_buffer_format_ARGB:
                        a = *input_ptr++; r = *input_ptr++; g = *input_ptr++; b = *input_ptr++;
                        break;
                    case bitmap_buffer_format_BGRA:
                        b = *input_ptr++; g = *input_ptr++; r = *input_ptr++; a = *input_ptr++;
                        break;
                    case bitmap_buffer_format_RGB:
                        r = *input_ptr++; g = *input_ptr++; b = *input_ptr++; a = 0xFF;
                        break;
                    case bitmap_buffer_format_BGR:
                        b = *input_ptr++; g = *input_ptr++; r = *input_ptr++; a = 0xFF;
                        break;
                }
                *output_ptr++ = r;
                *output_ptr++ = g;
                *output_ptr++ = b;
                *output_ptr++ = a;
            }
        }
    }
    
    *p_output_size = output_size;
    return rgba_file;
}

unsigned char * rgbaMagic_decode(
    const unsigned char * file_data,
    unsigned long file_data_length,
    bitmap_buffer_format desired_output_format,
    unsigned int row_alignment_bytes,
    unsigned long * p_width,
    unsigned long * p_height,
    size_t * p_output_size
)
{
    if (!file_data || !p_width || !p_height) {
        return NULL;
    }
    
    *p_output_size = 0;
    
    if (file_data_length < 12) {
        return NULL;
    }
    
    unsigned long magic = READ_BIG_ENDIAN_UINT32(&file_data[0]);
    if (magic != RGBA_BITMAP_MAGIC_NUMBER) {
        return NULL;
    }
    
    unsigned long width = READ_BIG_ENDIAN_UINT32(&file_data[4]);
    unsigned long height = READ_BIG_ENDIAN_UINT32(&file_data[8]);

    if (file_data_length < (12 + (width * height * 4))) {
        return NULL;
    }
    
    size_t output_pixel_size = 4;
    if (desired_output_format == bitmap_buffer_format_RGB ||
        desired_output_format == bitmap_buffer_format_BGR) {
        output_pixel_size = 3;
    }

    size_t output_row_size = width * output_pixel_size;
    if (row_alignment_bytes > 1) {
        int remainder = output_row_size % row_alignment_bytes;
        if (remainder > 0) {
            output_row_size += (row_alignment_bytes - remainder);
        }
    }
    
    unsigned char * bitmap = ( unsigned char *) malloc(output_row_size * height);
    if (!bitmap) {
        return NULL;
    }
    
    size_t input_row_size = width * 4;
    if (desired_output_format == bitmap_buffer_format_RGBA && output_row_size == input_row_size) {
        /* Fast path if the desired buffer is the exact same format as the file data */
        memcpy(bitmap, &file_data[12], (width * height * 4));
    } else {
        /* Transform the file data to the desired bitmap format */
        unsigned const char * in_ptr = &file_data[12];
        for (unsigned long y = 0; y < height; y++) {
            unsigned char * output_row_start = &bitmap[y * output_row_size];
            unsigned char * output_ptr = output_row_start;
            for (unsigned long x = 0; x < width; x++) {
                unsigned char r = *in_ptr++;
                unsigned char g = *in_ptr++;
                unsigned char b = *in_ptr++;
                unsigned char a = *in_ptr++;
                switch (desired_output_format) {
                    case bitmap_buffer_format_RGBA:
                        *output_ptr++ = r; *output_ptr++ = g; *output_ptr++ = b; *output_ptr++ = a;
                        break;
                    case bitmap_buffer_format_ABGR:
                        *output_ptr++ = a; *output_ptr++ = b; *output_ptr++ = g; *output_ptr++ = r;
                        break;
                    case bitmap_buffer_format_ARGB:
                        *output_ptr++ = a; *output_ptr++ = r; *output_ptr++ = g; *output_ptr++ = b;
                        break;
                    case bitmap_buffer_format_BGRA:
                        *output_ptr++ = b; *output_ptr++ = g; *output_ptr++ = r; *output_ptr++ = a;
                        break;
                    case bitmap_buffer_format_RGB:
                        *output_ptr++ = r; *output_ptr++ = g; *output_ptr++ = b;
                        break;
                    case bitmap_buffer_format_BGR:
                        *output_ptr++ = b; *output_ptr++ = g; *output_ptr++ = r;
                        break;
                }
            }
        }
    }
    
    *p_width = width;
    *p_height = height;
    if (p_output_size) {
        *p_output_size = (output_row_size * height);
    }
    return bitmap;
}


//w=640 h=360
unsigned char * rgba_to_rgb_brg(
    const unsigned char * file_data,
    unsigned long file_data_length,
    bitmap_buffer_format desired_output_format,
    unsigned int row_alignment_bytes,
    unsigned long width,
    unsigned long  height,
    size_t * p_output_size
)
{
    if (!file_data || !width || !height) {
        return NULL;
    }
    
    *p_output_size = 0;
    
    
    size_t output_pixel_size = 4;
    if (desired_output_format == bitmap_buffer_format_RGB ||
        desired_output_format == bitmap_buffer_format_BGR) {
        output_pixel_size = 3;
    }

    size_t output_row_size = width * output_pixel_size;
    if (row_alignment_bytes > 1) {
        int remainder = output_row_size % row_alignment_bytes;
        if (remainder > 0) {
            output_row_size += (row_alignment_bytes - remainder);
        }
    }
    
    unsigned char * bitmap = ( unsigned char *) malloc(output_row_size * height);
    if (!bitmap) {
        return NULL;
    }
    
    size_t input_row_size = width * 4;
    if (desired_output_format == bitmap_buffer_format_RGBA && output_row_size == input_row_size) {
        /* Fast path if the desired buffer is the exact same format as the file data */
        memcpy(bitmap, &file_data[0], (width * height * 4));
    } else {
        /* Transform the file data to the desired bitmap format */
        unsigned const char * in_ptr = &file_data[0];
        for (unsigned long y = 0; y < height; y++) {
            unsigned char * output_row_start = &bitmap[y * output_row_size];
            unsigned char * output_ptr = output_row_start;
            for (unsigned long x = 0; x < width; x++) {
                unsigned char r = *in_ptr++;
                unsigned char g = *in_ptr++;
                unsigned char b = *in_ptr++;
                unsigned char a = *in_ptr++;
                switch (desired_output_format) {
                    case bitmap_buffer_format_RGBA:
                        *output_ptr++ = r; *output_ptr++ = g; *output_ptr++ = b; *output_ptr++ = a;
                        break;
                    case bitmap_buffer_format_ABGR:
                        *output_ptr++ = a; *output_ptr++ = b; *output_ptr++ = g; *output_ptr++ = r;
                        break;
                    case bitmap_buffer_format_ARGB:
                        *output_ptr++ = a; *output_ptr++ = r; *output_ptr++ = g; *output_ptr++ = b;
                        break;
                    case bitmap_buffer_format_BGRA:
                        *output_ptr++ = b; *output_ptr++ = g; *output_ptr++ = r; *output_ptr++ = a;
                        break;
                    case bitmap_buffer_format_RGB:
                        *output_ptr++ = r; *output_ptr++ = g; *output_ptr++ = b;
                        break;
                    case bitmap_buffer_format_BGR:
                        *output_ptr++ = b; *output_ptr++ = g; *output_ptr++ = r;
                        break;
                }
            }
        }
    }
    

    if (p_output_size) {
        *p_output_size = (output_row_size * height);
    }
    return bitmap;
}



void write_bmp(unsigned char* img, size_t w, size_t h, const char* filename) 
{
  
  FILE *f;
  int filesize = 54 + 3*w*h;  //w is your image width, h is image height, both int

  unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
  unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
  unsigned char bmppad[3] = {0,0,0};
  
  bmpfileheader[ 2] = (unsigned char)(filesize    );
  bmpfileheader[ 3] = (unsigned char)(filesize>> 8);
  bmpfileheader[ 4] = (unsigned char)(filesize>>16);
  bmpfileheader[ 5] = (unsigned char)(filesize>>24);
  
  bmpinfoheader[ 4] = (unsigned char)(       w    );
  bmpinfoheader[ 5] = (unsigned char)(       w>> 8);
  bmpinfoheader[ 6] = (unsigned char)(       w>>16);
  bmpinfoheader[ 7] = (unsigned char)(       w>>24);
  bmpinfoheader[ 8] = (unsigned char)(       h    );
  bmpinfoheader[ 9] = (unsigned char)(       h>> 8);
  bmpinfoheader[10] = (unsigned char)(       h>>16);
  bmpinfoheader[11] = (unsigned char)(       h>>24);

  f = fopen(filename,"wb");
  fwrite(bmpfileheader,1,14,f);
  fwrite(bmpinfoheader,1,40,f);
  for(size_t i=0; i<h; ++i) {
      fwrite(img+(w*(h-i-1)*3),3,w,f);
      fwrite(bmppad,1,(4-(w*3)%4)%4,f);
  }
  fclose(f);

}