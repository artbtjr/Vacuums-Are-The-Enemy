#define _CRT_SECURE_NO_WARNINGS
#include<GL/glew.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>

GLuint glTexImageTGAFile( const char* filename, int* outWidth, int* outHeight );
void glDrawSprite( GLuint tex, int x, int y, int w, int h );