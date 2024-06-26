
#ifndef bitmap_h
#define bitmap_h

#include <stdio.h>

typedef enum _bitmap_buffer_format {
    bitmap_buffer_format_RGBA = 0,  /* R8G8B8A8 (default; same as the file storage) */
    
    /* The following formats are provided for convenience if you need the image
       encoded to and from something besides the default. You can also add others! */
    bitmap_buffer_format_ABGR,      /* A8B8G8R8 */
    bitmap_buffer_format_ARGB,      /* A8R8G8B8 */
    bitmap_buffer_format_BGRA,      /* B8G8R8A8 */
    bitmap_buffer_format_RGB,       /* R8G8B8   */
    bitmap_buffer_format_BGR,       /* B8G8R8   */
} bitmap_buffer_format;

/*!
    Encodes an in-memory bitmap to a new buffer containing a complete RGBA file.
    The bitmap input can be in one of mutiple byte formats. It is assumed to run from the
    top-left of the image to the bottom-right, row by row.
    @param input_buffer The bitmap to encode.
    @param width The width (in pixels) of the valid bitmap data.
    @param height The height (in pixels) of the bitmap data.
    @param input_row_size The size (in bytes) of one row of input bitmap data, if the "stride" includes padding. Use 0 if no padding.
    @param input_format The format of the pixel data in the input bitmap. Default is RGBA.
    @param p_output_size A pointer to a variable to contain the size of the allocated file data.
    @return The allocated file data. (NULL on error.) Caller must free.
*/
unsigned char * encode_to_rgbaMagic(
    const unsigned char * input_buffer,
    unsigned long width,
    unsigned long height,
    size_t input_row_size,
    bitmap_buffer_format input_format,
    size_t * p_output_size
);

/*!
    Decodes an buffer containing the contents of a complete RGBA file to a newly-allocated
    in-memory bitmap. This routine is able to decode into any of multiple bitmap formats.
    The bitmap output will run from the top-left of the image to the bottom-right, row by row.
    @param file_data The data containing the entire contents of the input file.
    @param file_data_length The length of the contents of the file_data buffer.
    @param desired_output_format The requested format of the pixel data in the output bitmap. Default is RGBA.
    @param row_alignment_bytes Padding can be added to the end of output row data if required to align rows.
        0 (or 1) will indicate a packed output bitmap.
    @param p_width Pointer to the output variable for the width (in pixels) of the valid bitmap data.
    @param p_height Pointer to the output variable for the height (in pixels) of the bitmap data.
    @param p_output_size A pointer to a variable to contain the size of the allocated bitmap (optional; can be NULL)
    @return The allocated bitmap buffer. (NULL on error.) Caller must free.
*/
unsigned char * rgbaMagic_decode(
    const unsigned char * file_data,
    unsigned long file_data_length,
    bitmap_buffer_format desired_output_format,
    unsigned int row_alignment_bytes,
    unsigned long * p_width,
    unsigned long * p_height,
    size_t * p_output_size
);

unsigned char * rgba_to_rgb_brg(
    const unsigned char * file_data,
    unsigned long file_data_length,
    bitmap_buffer_format desired_output_format,
    unsigned int row_alignment_bytes,
    unsigned long width,
    unsigned long  height,
    size_t * p_output_size
);


void write_bmp(unsigned char* img, size_t w, size_t h, const char* filename) ;

#endif /* bitmap_h */
