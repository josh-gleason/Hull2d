#ifndef CONVEX2D_H
#define CONVEX2D_H

#include "defines.h"
#include "stack.h"
#include "qsort.h"

#define MAX_POINTS_PER_HULL   (MAX_BLOBS_PER_GROUP * CORNERS_PER_BLOB)

typedef struct flaggedindex_s
{
    uint32_t pointIdx;
    bool_t   remove;
} flaggedindex_t;

typedef struct hull2d_s
{
    // Points that make up the hull
    Point2f        points[MAX_POINTS_PER_HULL];
    uint32_t       pointCount;

    // Indices of points that make up the boundary of the hull
    flaggedindex_t boundaryIdx[MAX_POINTS_PER_HULL];
    uint32_t       boundaryCount;

    // An index to the index which references the lowest point (indirection :P)
    // The index is a location in the boundaryIdx list
    uint32_t       lowestIdx;

    // flag indicating hull needs to be (re)computed
    bool_t         dirty;
} hull2d_t;

/**
* @brief Initialize hull object
* @param[in/out] hull Pointer to the hull object
*/
void hull2d_init(hull2d_t* hull);

/**
* @brief Initialize the stack object needed for cull creation
* @param[in/out] stack Pointer to the stack object
*/
void hull2d_initStack(stack_t* stack);

/**
* @brief Clear an already existing hull object
* @param[in/out] hull Pointer to the hull object
*/
void hull2d_clear(hull2d_t* hull);

/**
* @brief Add a point to the point list of a hull object
* @param[in/out] hull Pointer to the hull object
* @param[in] point Pointer to the point that will be added
*/
void hull2d_addPoint(hull2d_t* hull, const Point2f* point);

/**
* @brief Add multiple points to the point list of a hull object
* @param[in/out] hull Pointer to the hull object
* @param[in] points Array of points to be added
* @param[in] count Number of points in points array
*/
void hull2d_addPoints(hull2d_t* hull, const Point2f* points, uint32_t count);

/**
* @brief Compute the hull using the points in the hull's point list
* @param hull Pointer to the hull object
* @param stack A stack of points used as scratch space
* @return Returns true if able to create a hull false otherwise, only fails if
*         two or less points are in the hull or all points fall on same line.
*/
bool_t hull2d_computeHull(hull2d_t* hull, stack_t* stack);

/**
* @brief Check if two convex hulls intersect
* @param[in] h1 The first hull
* @param[in] h2 The second hull
* @return True if h1 intesects h2, false otherwise
*/
bool_t hull2d_checkIntersect(const hull2d_t* h1, const hull2d_t* h2);

#endif // CONVEX2D_H

