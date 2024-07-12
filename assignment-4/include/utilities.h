#ifndef UITILIITIES_H
#define UITILIITIES_H

#include <time.h>

void dump_ppm(const void *p, int size, unsigned int tag, struct timespec *time);

void dump_pgm(const void *p, int size, unsigned int tag, struct timespec *time);

double getDuration(struct timespec const start, struct timespec const end);

double getDurationInMilliseconds(struct timespec const start, struct timespec const end);

void yuv2rgb(int y, int u, int v, unsigned char *r, unsigned char *g,
             unsigned char *b);

void yuv2rgb_float(float y, float u, float v, unsigned char *r,
                   unsigned char *g, unsigned char *b);

#endif // UTILITIES_H
