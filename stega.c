/*
 * A simple libpng example program
 * http://zarb.org/~gc/html/libpng.html
 *
 * Modified by Yoshimasa Niwa to make it much simpler
 * and support all defined color_type.
 *
 * To build, use the next instruction on OS X.
 * $ brew install libpng
 * $ clang -lz -lpng15 libpng_test.c
 *
 * Copyright 2002-2010 Guillaume Cottenceau.
 *
 * This software may be freely redistributed under the terms
 * of the X11 license.
 *
 */

 // http://www.labbookpages.co.uk/software/imgProc/libPNG.html compile command
 // https://gist.github.com/niw/5963798
 
#include <stdlib.h>
#include <stdio.h>
#include <png.h>
#include <string.h>
 
int width, height, char_capacity;
png_byte color_type;
png_byte bit_depth;
png_bytep *row_pointers;

// DEBUGGING PURPOSE
// binary string representation of an int
const char *byte_to_binary(int x){
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}
// END DEBUGGING PURPOSE

// -----------------> CODING FUNCTIONS <-----------------

// 1) Simple Halves method
// Takes two CHAR a and CHAR b and do the following
//  Arguments: 'a' = 0110 0001 'r' = 0111 0010 (RGBA) = (00000000,10010100,00100100,11111111)
//  Return: (RGBA) = (0000 0110,1001 0001, 00100111, 11110010)
//  - 4 MSB of the FIRST char are hidden in the RED component of the pixel
//  - 4 LSB of the FIRST char are hidden in the BLUE component of the pixel
//  - 4 MSB of the SECOND char are hidden in the GREEN component of the pixel
//  - 4 LSB of the SECOND char are hidden in the ALPHA component of the pixel
void simple_halves(char a, char b,  png_bytep px){
  // MSBmask 11110000 (240) LSBmask 00001111 (15)
  unsigned char msb_mask=240, lsb_mask=15; 
  unsigned char msb=0, lsb=0;

  msb=a & msb_mask; // char a
  lsb=a & lsb_mask;

  px[0] = (px[0] & msb_mask) | (msb >> 4);
  px[1] = (px[1] & msb_mask) | lsb;

  msb=b & msb_mask; // char b
  lsb=b & lsb_mask;

  px[2] = (px[2] & msb_mask) | (msb >> 4);
  px[3] = (px[3] & msb_mask) | lsb;
}

// Encodes the size of the text to be hidden in the first two pixels (2^32 chars of size)
void encode_header(int size, png_bytep px, char * mode){
  if(strcmp(mode,"MSB")==0){
    // first pixel holds 16 MSB of the size of the message
    int int_msb_mask=4278190080; // 11111111000000000000000000000000
    int int_lsb_mask=16711680; // 00000000111111110000000000000000
    unsigned char si, ze;
    si=(size & int_msb_mask) >> 24;
    ze=(size & int_lsb_mask) >> 16;
    simple_halves(si,ze,px);
  } else {
    // second pixel holds 16 LSB of the size of the message
    int short_msb_mask=65280; // 1111111100000000
    int short_lsb_mask=255; // 0000000011111111
    unsigned char si, ze;
    si=(size & short_msb_mask) >> 8;
    ze=size & short_lsb_mask;
    simple_halves(si,ze,px);
  }
}
// END CODING FUNCTIONS

// -----------------> DECODING FUNCTIONS <-----------------
void inv_simple_halves(png_bytep px, FILE * fp){
  unsigned char words[2]={0,0};
  // MSBmask 11110000 (240) LSBmask 00001111 (15)
  unsigned char msb_mask=240, lsb_mask=15; 
  unsigned char lsb1=0, lsb2=0;

  lsb1=px[0] & lsb_mask; // first char
  lsb2=px[1] & lsb_mask;

  words[0] = ( words[0] | (lsb1 << 4) ) | lsb2;
  
  lsb1=px[2] & lsb_mask; // second char
  lsb2=px[3] & lsb_mask;

  words[1] = ( words[1] | (lsb1 << 4) ) | lsb2;

  fputc( words[0], fp );
  fputc( words[1], fp );
}

int inv_header(png_bytep px, char * mode){
  unsigned char words[2]={0,0};
  unsigned char lsb_mask=15; // LSBmask 00001111 (15)
  unsigned char lsb1=0, lsb2=0;
  int ret;

  if(strcmp(mode, "MSB")==0){
    // we get the 16 MSB of the size of the text
    lsb1=px[0] & lsb_mask; // first char
    lsb2=px[1] & lsb_mask;

    words[0] = ( words[0] | (lsb1 << 4) ) | lsb2;
    
    lsb1=px[2] & lsb_mask; // second char
    lsb2=px[3] & lsb_mask;

    words[1] = ( words[1] | (lsb1 << 4) ) | lsb2;

    ret = ret | (words[0] << 24);
    ret = ret | (words[1] << 16);
  } else {
    // we get the 16 LSB of the size of the text
    lsb1=px[0] & lsb_mask; // first char
    lsb2=px[1] & lsb_mask;

    words[0] = ( words[0] | (lsb1 << 4) ) | lsb2;
    
    lsb1=px[2] & lsb_mask; // second char
    lsb2=px[3] & lsb_mask;

    words[1] = ( words[1] | (lsb1 << 4) ) | lsb2;

    ret = ret | (words[0] << 8);
    ret = ret | words[1];
  }
  return ret;
}
// END DECODING FUNCTIONS
 
void read_png_file(char *filename) {
  FILE *fp = fopen(filename, "rb");
 
  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(!png) abort();
 
  png_infop info = png_create_info_struct(png);
  if(!info) abort();
 
  if(setjmp(png_jmpbuf(png))) abort();
 
  png_init_io(png, fp);
 
  png_read_info(png, info);
 
  width      = png_get_image_width(png, info);
  height     = png_get_image_height(png, info);
  color_type = png_get_color_type(png, info);
  bit_depth  = png_get_bit_depth(png, info);

  char_capacity = (height*width*2)-4; // -4 because of the two first pixels (size)

  // Read any color_type into 8bit depth, RGBA format.
  // See http://www.libpng.org/pub/png/libpng-manual.txt
 
  if(bit_depth == 16)
    png_set_strip_16(png);
 
  if(color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png);
 
  // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
  if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png);
 
  if(png_get_valid(png, info, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png);
 
  // These color_type don't have an alpha channel then fill it with 0xff.
  if(color_type == PNG_COLOR_TYPE_RGB ||
     color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
 
  if(color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png);
 
  png_read_update_info(png, info);
 
  row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
  int y;
  for(y = 0; y < height; y++) {
    row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
  }
 
  png_read_image(png, row_pointers);
 
  fclose(fp);
}
 
void write_png_file(char *filename) {
  int y;
 
  FILE *fp = fopen(filename, "wb");
  if(!fp) abort();
 
  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) abort();
 
  png_infop info = png_create_info_struct(png);
  if (!info) abort();
 
  if (setjmp(png_jmpbuf(png))) abort();
 
  png_init_io(png, fp);
 
  // Output is 8bit depth, RGBA format.
  png_set_IHDR(
    png,
    info,
    width, height,
    8,
    PNG_COLOR_TYPE_RGBA,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT
  );
  png_write_info(png, info);
 
  // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
  // Use png_set_filler().
  //png_set_filler(png, 0, PNG_FILLER_AFTER);
 
  png_write_image(png, row_pointers);
  png_write_end(png, NULL);
  for(y = 0; y < height; y++) {
    free(row_pointers[y]);
  }
  free(row_pointers);
 
  fclose(fp);
}
 
void process_png_file(char *filename) {
  // read text
  int size,c,i=0;
  FILE * fp;
  fp = fopen(filename, "r");
  if(fp == NULL){
    perror("Error in opening file");
  }

  fscanf(fp, "%i",&size);
  char msg[size];

  do
  {
    c = fgetc(fp);
    if(feof(fp))
    {
      break ;
    }
    msg[i]=c;
    i++;
  }while(1);

  fclose(fp);

  if(size>char_capacity){
    printf("Your image is not big enough to hide that message.\n");
    return;
  }

  // Process image
  int y,x;
  i=0;

  for(y = 0; y < height; y++) {
    png_bytep row = row_pointers[y];
    for(x = 0; x < width; x++) {
      png_bytep px = &(row[x * 4]);
      // Do something awesome for each pixel here...
      //printf("%4d, %4d = RGBA(%3d, %3d, %3d, %3d)\n", x, y, px[0], px[1], px[2], px[3]);
      
      // x = X coordinate (0-width)
      // y = Y coordinate (0-height)
      // px[0] = RED value (0-255)
      // px[1] = GREEN value (0-255)
      // px[2] = BLUE value (0-255)
      // px[3] = alpha channel (opacity 0-255)
      if(x==0 && y==0){
        // first pixel holds 16 MSB of the size of the message
        encode_header(size, px, "MSB");
      } else if(x==1 && y==0){
        encode_header(size, px, "LSB");
      } else {
        if(i<=size){
          simple_halves(msg[i],msg[i+1],px);
          i+=2;
        }
      }

      // printf("%c, %s = RGBA(%i, %i \n", msg[i], byte_to_binary(msg[i]), px[0], px[1]);
      // printf("%c, %s = %i, %i ) \n", msg[i+1], byte_to_binary(msg[i+1]), px[2], px[3]);

    }
  }
}

void process_image_text(char * filename) {
  int y,x,i=0,size;
  FILE * fp;
  fp = fopen(filename, "w");
  if(fp == NULL){
    perror("Error in opening file");
  }
  
  for(y = 0; y < height; y++) {
    png_bytep row = row_pointers[y];
    for(x = 0; x < width; x++) {
      png_bytep px = &(row[x * 4]);
      // Read and decode each pixel here...

      if(x==0 && y==0){
        size = size | inv_header(px, "MSB");
      } else if(x==1 && y==0) {
        size = size | inv_header(px, "LSB");
      } else {
        if(i<=size){
          inv_simple_halves(px, fp);
          i+=2;
        }
      }
    }
  }
  fclose(fp);
}

// USAGE:
//   stega -hide TEXT.txt IMAGE.png OUTPUT.png
//   stega -read IMAGE.png OUTPUT.txt
//   stega -size IMAGE.png
int main(int argc, char *argv[]) {
 
  if(argc == 5 && strcmp(argv[1],"-hide") == 0){
    read_png_file(argv[3]);
    process_png_file(argv[2]);
    write_png_file(argv[4]);
  }
  else if(argc == 4 && strcmp(argv[1],"-read") == 0){
    read_png_file(argv[2]);
    process_image_text(argv[3]);
  } else if(argc == 3 && strcmp(argv[1],"-size") == 0) {
    read_png_file(argv[2]);
    printf("Your image is %ix%i pixels. \n", height, width);
    printf("You will be able to hide up to %i chars.\n", char_capacity);
  } else {
    printf("Please use: \n stega -hide TEXT.txt IMAGE.png OUTPUT.png \n stega -read IMAGE.png OUTPUT.txt \n");
  }

  return 0;
}