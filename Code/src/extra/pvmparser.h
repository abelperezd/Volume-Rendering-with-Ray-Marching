#ifndef PVMPARSER_H
#define PVMPARSER_H

/*
Format and parse code by Stefan Roettger
*/

unsigned char* parsePVM(const char *filename, unsigned int *width, unsigned int *height, unsigned int *depth, unsigned int *components, float *scalex, float *scaley, float *scalez);

#endif