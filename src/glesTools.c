#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GLES2/gl2.h>
#include "glesTools.h"
#include <math.h>
#include <png.h>

char * loadShader( const char * _fileName )
{
    FILE * inputFile = fopen( _fileName, "rb" );
    if ( inputFile == NULL ) {
        fprintf( stderr, "%s: Error Loading %s", __FILE__, _fileName );
        exit(1);
    }
    fseek( inputFile, 0, SEEK_END );
    long fileSize = ftell( inputFile );
    rewind( inputFile );

    char * buffer;
    buffer = (char *) malloc(sizeof(char) * (fileSize+1));
    if (buffer == NULL) {
        fprintf( stderr, "%s: Memory Error", __FILE__ );
        exit(1);
    }
    size_t result = fread(buffer, 1, fileSize, inputFile);
    if ( result != fileSize ) {
        fprintf(stderr, "%s: Read error", __FILE__);
    }
    fclose(inputFile);
    buffer[fileSize] = '\0';
    return buffer;
}

float randFloat( void )
{
    return ((float)rand() / RAND_MAX);
}

GLuint loadObj( const char* _fileName, GLfloat **_vertices, GLushort **_elements )
{
    FILE * inputFile = fopen( _fileName, "r" );
    if ( inputFile == NULL ) {
        fprintf( stderr, "%s: Error Loading %s\n", __FILE__, _fileName );
        exit(1);
    }
    int mem = 64;
    char *str = malloc(mem);

    int verticesSize = 100;
    int elementsSize = 100;
    (*_vertices) = (GLfloat *)malloc(sizeof(GLfloat) * verticesSize);
    (*_elements) = (GLushort *)malloc(sizeof(GLushort) * elementsSize);
    int verticesPosition = 0;
    int elementsPosition = 0;
    if (str == NULL) {
        fprintf( stderr, "%s: Memory Error", __FILE__ );
        exit(1);
    }
    while (fgets(str, mem, inputFile) != NULL) {
        while (str[strlen(str)-1]!='\n') {
            mem *= 2;
            str = realloc(str,mem);
            fgets(str + mem/2 - 1, mem/2 + 1,inputFile);
        }
        float x, y;
        int a, b, c;
        if (sscanf(str, "v %f %f %*f", &x, &y) == 2) {
            if (verticesPosition + 2 > verticesSize-1) {
                verticesSize *= 2;
                (*_vertices) = realloc(*_vertices, sizeof(GLfloat) * verticesSize);
            }
            (*_vertices)[verticesPosition++] = x;
            (*_vertices)[verticesPosition++] = y;
        } else if(sscanf(str, "f %d %d %d", &a, &b, &c) == 3) {
            if(elementsPosition + 3 > elementsSize-1) {
                elementsSize *= 2;
                (*_elements) = realloc((*_elements), sizeof(GLushort) * elementsSize);
            }
            (*_elements)[elementsPosition++] = a-1;
            (*_elements)[elementsPosition++] = b-1;
            (*_elements)[elementsPosition++] = c-1;
        } else if(sscanf(str, "f %d %d", &a, &b) == 2) {
            if(elementsPosition + 2 > elementsSize-1) {
                elementsSize *= 2;
                (*_elements) = realloc((*_elements), sizeof(GLushort) * elementsSize);
            }
            (*_elements)[elementsPosition++] = a-1;
            (*_elements)[elementsPosition++] = b-1;
        }
    }
    (*_vertices) = realloc((*_vertices), sizeof(GLfloat) * verticesPosition);
    (*_elements) = realloc((*_elements), sizeof(GLushort) * elementsPosition);
    free(str);
    return elementsPosition;
}

GLfloat * ComputeSurfaceNormals( const GLfloat *points, const GLushort
        *elements, const GLuint elementsSize )
{
    GLfloat *normals = malloc(sizeof(GLfloat) * elementsSize);
    GLfloat *normalsIterator = normals;
    unsigned int i = 0;
    for( i = 1 ; i < elementsSize ; i+=2 ) {
        GLfloat vec[2];
        vec[0] = points[2*elements[i]] - points[2*elements[i-1]];
        vec[1] = points[2*elements[i]+1] - points[2*elements[i-1]+1];

        *normalsIterator++ = -vec[1];
        *normalsIterator++ =  vec[0];
    }
    return normals;
}

unsigned char* PngTexture(const char * _fileName, int **_width, int **_height)
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
    (*_width) = malloc(sizeof(int));
    (*_height) = malloc(sizeof(int));
    memcpy((*_width), &width, sizeof(int));
    memcpy((*_height), &height, sizeof(int));

    png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    int number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "%s: %s Error reading image\n", __FILE__, _fileName);
        exit(1);
    }
    png_bytep * row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
    int x, y;
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
    unsigned char *buffer = malloc( 4 * width * height );
    unsigned char *bufferPos = buffer;
    for( y = 0 ; y < height ; ++y ) {
        png_byte* row = row_pointers[y];
        for ( x = 0 ; x < width ; ++x ){
            png_byte* ptr = &(row[x*4]);
            int i = 0;
            for ( ; i < 4 ; ++i )
                (*bufferPos++) = (*ptr++);
            //printf("Pixel at position [ %d - %d ] has RGBA values: %d - %d - %d - %d\n",
            //       x, y, ptr[0], ptr[1], ptr[2], ptr[3]);
        }
    }
    /* cleanup heap allocation */
    for (y=0; y<height; y++)
        free(row_pointers[y]);
    free(row_pointers);

    return buffer;
}
