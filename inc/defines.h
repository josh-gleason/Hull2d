#ifndef DEFINES_H
#define DEFINES_H

#include <stdbool.h>
#include <stdint.h>
#include <memory.h>
#include <stdlib.h>
#include <assert.h>
#include <float.h>
#include <math.h>

#define CORNERS_PER_BLOB      (8)
#define MAX_BLOBS_PER_GROUP   (256)

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

typedef struct Point2f_s
{
    float x;
    float y;
} Point2f;

typedef bool bool_t;

#define LOGASSERT(a) assert(a)

#endif
