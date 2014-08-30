#include <stdio.h>
#include <stdlib.h>
#include <png.h>

png_byte color_type;
png_byte bit_depth;

png_bytep * ReadPngTexture(const char * _fileName)
{
    FILE * inputFile = fopen( _fileName, "rb" );
    if ( inputFile == NULL ) {
        fprintf( stderr, "%s: Error Loading %s\n", __FILE__, _fileName );
        exit(1);
    }
    unsigned char sig[8];
    fread(sig, 1, 8, inputFile);
    if( !png_check_sig(sig, 8) ) {
        fprintf( stderr, "%s: Error: %s is not a png file\n", __FILE__, _fileName );
        exit(1);
    }
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,0,0,0);
    if(!png_ptr) {
        fprintf(stderr, "%s: %s png_create_read_struct\n", __FILE__, _fileName);
        exit(1);
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, 0, 0);
        fprintf(stderr, "%s: %s png_create_info_struct\n", __FILE__, _fileName);
        exit(1);
    }
    if(setjmp(png_ptr->jmpbuf)) {
        fprintf(stderr, "%s: %s init_io\n", __FILE__, _fileName);
        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
        exit(1);
    }
    png_init_io(png_ptr, inputFile);
    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);

    int width = png_get_image_width(png_ptr, info_ptr);
    int height = png_get_image_height(png_ptr, info_ptr);

    color_type = png_get_color_type(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    int number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "%s: %s Error reading image\n", __FILE__, _fileName);
        exit(1);
    }
    png_bytep * row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
    int y;
    for ( y=0 ; y < height ; ++y ) {
        row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));
    }

    png_read_image(png_ptr, row_pointers);

    fclose(inputFile);

    if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGB) {
        fprintf(stderr, "%s: %s input file is PNG_COLOR_TYPE_RGB but must be PNG_COLOR_TYPE_RGBA (lacks the alpha channel)\n", __FILE__, _fileName);
        exit(1);
    }  
    if (png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_RGBA) {
        fprintf(stderr, "%s: %s color_type of input file must be PNG_COLOR_TYPE_RGBA (%d) (is %d)\n", __FILE__, _fileName, PNG_COLOR_TYPE_RGBA, png_get_color_type(png_ptr, info_ptr));
        exit(1);
    }

    return row_pointers;
}

void write_png_file( char *_fileName, png_bytep *row_pointers, int width, int height )
{
    FILE * inputFile = fopen( _fileName, "wb" );
    if ( inputFile == NULL ) {
        fprintf( stderr, "%s: Error Loading %s\n", __FILE__, _fileName );
        exit(1);
    }
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png_ptr) {
        fprintf(stderr, "%s: %s png_create_read_struct\n", __FILE__, _fileName);
        exit(1);
    }
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, 0, 0);
        fprintf(stderr, "%s: %s png_create_info_struct\n", __FILE__, _fileName);
        exit(1);
    }
    if ( setjmp(png_jmpbuf(png_ptr)) ) {
        fprintf(stderr, "%s: %s init_io\n", __FILE__, _fileName);
        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
        exit(1);
    }
    png_init_io(png_ptr, inputFile);
    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "%s: %s Error during writing bytes", __FILE__, _fileName);
        exit(1);
    }

    png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type,
            PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
            PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);

    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "%s: %s Error during writing bytes");
        exit(1);
    }

    png_write_image(png_ptr, row_pointers);
    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "%s: %s Error during end of write");
        exit(1);
    }

    png_write_end(png_ptr, NULL);

    fclose(inputFile);
}

int main(int argc, char **argv)
{
    png_bytep *textures[16];
    int i = 0;
    for ( ; i < 16 ; ++i ) {
        char str[21];
        sprintf(str, "texture/resize/%d.png", i);
        textures[i] = ReadPngTexture(str);
    }
    int x, y;
    png_byte *outputImage[256];
    for ( i=0 ; i < 256 ; ++i ) {
        outputImage[i] = malloc(4 * 256 * sizeof(png_byte));
    }
    i = 0;
    for ( ; i < 16 ; ++i) {
        int startX, startY;
        startX = (i % 4) * 64;
        startY = (i / 4) * 64;

        for( y = 0 ; y < 64 ; ++y) {
            png_byte *row = textures[i][y];
            for( x = 0 ; x < 64 ; ++x ) {
                png_byte* ptr = &(row[x*4]);
                //memcpy(&(outputImage[startY + y][(startX + x) * 4]), row, 4 * sizeof(png_byte));
                memcpy(&(outputImage[startY + y][(startX + x)*4]), ptr, 4 * sizeof(png_byte));
                //printf("(%d, %d) (%d, %d, %d, %d)\n", startY + y, (startX + x), outputImage[startY + y][(startX + x) * 4], outputImage[startY + y][((startX + x) * 4) + 1], outputImage[startY + y][((startX + x) * 4) + 2], outputImage[startY + y][((startX + x) * 4) + 3]);
                //printf("YO: (%d, %d)\n", startY + y, (startX + x) * 4);
            }
        }
    }
    write_png_file("texture/resize/foo.png", outputImage, 256, 256);
    for ( i = 0 ; i < 256 ; ++i) {
        free(outputImage[i]);
    }
    for ( i = 0 ; i < 16 ; ++i ) {
        free(textures[i]);
    }
}
