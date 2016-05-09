#include "hull2d.h"

/**
* Builds convex hull boundary using Graham's algorithm based on Chapter 3 of
* Computational Geometery in C by Rourke (1997)
*
* I simplified the convex intersection routine in Section 7.6 to just check if
* the intersection exists or not. I choose this approach because its faster
* than a direct application of the separating axis theorem.
*
* Complexity
* For points list of size s
*   Hull construction: O(s log(s))
* For two convex hulls with n and m points on their boundaries respectively
*   Intersection test: O(n + m)
*
* Programmer: Josh Gleason
* Date: 05/09/2016
*/

/**
* @brief Initialize hull object
* @param[in/out] hull Pointer to the hull object
*/
void hull2d_init(hull2d_t* hull)
{
    hull->dirty = TRUE;
    hull->pointCount = 0;
    hull->boundaryCount = 0;
    hull->lowestIdx = 0;

    // necessary for initial comparison for lowest point
    hull->boundaryIdx[0].pointIdx = 0;
    hull->boundaryIdx[0].remove = FALSE;
}

/**
* @brief Initialize the stack object needed during cull creation
* @param[in/out] stack Pointer to the stack object
*/
void hull2d_initStack(stack_t* stack)
{
    LOGASSERT(stack_init(stack, MAX_POINTS_PER_HULL, sizeof(flaggedindex_t)));
}

/**
* @brief Clear an already existing hull object
* @param[in/out] hull Pointer to the hull object
*/
void hull2d_clear(hull2d_t* hull)
{
    hull2d_init(hull);
}

/**
* @brief Add a point to the point list of a hull object
* @param[in/out] hull Pointer to the hull object
* @param[in] point Pointer to the point that will be added
*/
void hull2d_addPoint(hull2d_t* hull, const Point2f* point)
{
    Point2f *p0;
    memcpy(&hull->points[hull->pointCount], point, sizeof(Point2f));
    hull->dirty = TRUE;

    // add a reference to the new point in the boundaryIdx list
    hull->boundaryIdx[hull->boundaryCount].pointIdx = hull->pointCount;
    hull->boundaryIdx[hull->boundaryCount].remove = FALSE;

    // Check if this is the lowest point (if same choose the right-most)
    p0 = &hull->points[hull->boundaryIdx[hull->lowestIdx].pointIdx];
    if ((point->y < p0->y) || 
        (fabs(point->y - p0->y) <= FLT_EPSILON && point->x > p0->x))
    {
        hull->lowestIdx = hull->boundaryCount;
    }

    // update the point counts
    hull->pointCount += 1;
    hull->boundaryCount += 1;
}

/**
* @brief Add multiple points to the point list of a hull object
* @param[in/out] hull Pointer to the hull object
* @param[in] points Array of points to be added
* @param[in] count Number of points in points array
*/
void hull2d_addPoints(hull2d_t* hull, const Point2f* points, uint32_t count)
{
    uint32_t i;
    Point2f *p0, *p;

    memcpy(&hull->points[hull->pointCount], points, sizeof(Point2f)*count);
    hull->dirty = TRUE;

    p0 = &hull->points[hull->boundaryIdx[hull->lowestIdx].pointIdx];
    for (i = 0; i < count; ++i)
    {
        // add a reference to the new point in the boundaryIdx list
        hull->boundaryIdx[hull->boundaryCount + i].pointIdx =
            hull->pointCount + i;
        hull->boundaryIdx[hull->boundaryCount + i].remove = FALSE;

        // Keep track of lowest point (if same choose the right-most)
        p = &hull->points[hull->pointCount + i];
        if ((p->y < p0->y) ||
            (fabs(p->y - p0->y) <= FLT_EPSILON && p->x > p0->x))
        {
            p0 = p;
            hull->lowestIdx = hull->boundaryCount + i;
        }
    }

    // Update point count
    hull->pointCount += count;
    hull->boundaryCount += count;
}

/**
* @brief If triangle abc is CCW winding then return 1, if CW winding return -1
*        and if all points are on a line return 0
* @param[in] a Pointer to first point of triangle
* @param[in] b Pointer to second point of triangle
* @param[in] c Pointer to third point of triangle
*/
static int32_t hull2d_areaSign(const Point2f* a, const Point2f* b,
    const Point2f* c)
{
    double area2;

    area2 = (double)( b->x - a->x ) * (double)( c->y - a->y ) -
            (double)( c->x - a->x ) * (double)( b->y - a->y );

    if      (area2 >  FLT_EPSILON) return  1;
    else if (area2 < -FLT_EPSILON) return -1;
    else                          return  0;
}

/**
* @brief Return true if point c is strictly left of vector ab
* @param[in] a Back of the vector
* @param[in] b End of the vector
* @param[in] c Point to test
* @return Return true if a < b and false otherwise
*/
static bool_t hull2d_left(const Point2f* a, const Point2f* b, const Point2f* c)
{
    return hull2d_areaSign(a, b, c) > 0;
}

/**
* @brief Return true if point c is left of or on ab
* @param[in] a Back of the vector (one point on the line)
* @param[in] b End of the vector (second point on the line)
* @param[in] c Point to test
* @return Return true if a < b and false otherwise
*/
static bool_t hull2d_leftOn(const Point2f* a, const Point2f* b,
    const Point2f* c)
{   
    return hull2d_areaSign(a, b, c) >= 0;
}

/**
* @brief Return true if all 3 points are on the same line
* @param[in] a Back of the vector (one point on the line)
* @param[in] b End of the vector (second point on the line)
* @param[in] c Point to test
* @return Return true if a < b and false otherwise
*/
static bool_t hull2d_collinear(const Point2f* a, const Point2f* b,
    const Point2f* c)
{
    return hull2d_areaSign(a, b, c) == 0;
}

/**
* @brief Compare c to ab
*        if c is strictly left of ab then return false
*        if c is strictly right of ab then return true
*        if c is on the line defined by ab then
*                |b - a| < |c - a| -> return true and mark b for deletion
*                |c - a| < |b - a| -> return false and mark c for deletion
*                and mark the c or b to remove (whichever is greater)
*        if b and c are colocated -> return false and one for removal
* @param[in] hull     Pointer to hull object
* @param[in] a        Origin to compare about
* @param[in/out] bidx index of first point to compare
* @param[in/out] cidx index of second point to compare
*/
static bool_t hull2d_compareAndFlag(hull2d_t* hull, const Point2f* a,
    flaggedindex_t* bidx, flaggedindex_t* cidx)
{
    Point2f *b, *c;

    if (bidx->pointIdx == cidx->pointIdx)
    {
        return FALSE;
    }

    b = &hull->points[bidx->pointIdx];
    c = &hull->points[cidx->pointIdx];

    int areaSign;
    areaSign = hull2d_areaSign( a, b, c );
    if (areaSign > 0)
    {
        return TRUE;
    }
    else if (areaSign < 0)
    {
        return FALSE;
    }
    else
    {
        // a,b,c are collinear
        float dx = fabsf(b->x - a->x) - fabsf(c->x - a->x);
        float dy = fabsf(b->y - a->y) - fabsf(c->y - a->y);
        if (dx < -FLT_EPSILON || dy < -FLT_EPSILON)
        {
            bidx->remove = TRUE;
            return FALSE;
        }
        else if (dx > FLT_EPSILON || dy > FLT_EPSILON)
        {
            cidx->remove = TRUE;
            return TRUE;
        }
        else
        {
            // remove the earlier of the two
            if (bidx->pointIdx > cidx->pointIdx)
            {
                bidx->remove = TRUE;
            }
            else 
            {
                cidx->remove = TRUE;
            }
            return FALSE;
        }
    }
}

/**
* @brief Sort the points in the hull structure relative angle to lowest point
* @param hull Pointer to the hull structure
*/
static void hull2d_sort(hull2d_t* hull)
{
    // define the less than routine for the QSORT macro
#define hull2d_sort_lt(a,b) hull2d_compareAndFlag(hull, p0, a, b)

    uint32_t        pcount;
    flaggedindex_t* indices;
    flaggedindex_t  temp;
    Point2f*        p0;

    // move the reference to the lowest point to the front
    temp = hull->boundaryIdx[0];
    hull->boundaryIdx[0] = hull->boundaryIdx[hull->lowestIdx];
    hull->boundaryIdx[hull->lowestIdx] = temp;
    hull->lowestIdx = 0;

    // get parameters for sorting ready
    indices = &hull->boundaryIdx[1];
    pcount  =  hull->boundaryCount - 1;
    p0      = &hull->points[hull->boundaryIdx[0].pointIdx];

    // sort
    QSORT(flaggedindex_t, indices, pcount, hull2d_sort_lt);
}

/**
* @brief Remove all the indices marked for removal, tightening the list
* @param[in/out] hull Pointer to the hull object
*/
static void hull2d_squash(hull2d_t* hull)
{
    uint32_t i, j;
    i = 0; j = 0;
    while (i < hull->boundaryCount)
    {
        if (!hull->boundaryIdx[i].remove)
        {
            if (i != j)
            {
                hull->boundaryIdx[j] = hull->boundaryIdx[i];
            }
            ++j;
        }
        ++i;
    }
    hull->boundaryCount = j;
}

/**
* @brief The last step of Graham's algorithm, build the convex hull and store
*        the result in the stack.
* @param[in] hull   Pointer to the hull object
* @param[out] stack Will contain indices of points on hull when done
*/
void hull2d_grahams(const hull2d_t* hull, stack_t* stack)
{
    uint32_t i;
    flaggedindex_t p1idx, p2idx, p3idx;
    const Point2f *p1, *p2, *p3;

    // clear the stack
    stack_clear(stack);

    // push the first two onto the stack, these will never be removed
    (void)stack_push(stack, &hull->boundaryIdx[0]);
    (void)stack_push(stack, &hull->boundaryIdx[1]);

    i = 2U;
    while (i < hull->boundaryCount)
    {
        // grab the top two points on the stack
        LOGASSERT(stack_peek(stack, 1, &p1idx));
        LOGASSERT(stack_peek(stack, 0, &p2idx));

        p1 = &hull->points[p1idx.pointIdx];
        p2 = &hull->points[p2idx.pointIdx];

        p3idx = hull->boundaryIdx[i];
        p3 = &hull->points[p3idx.pointIdx];

        // if the path (p1, p1, p3) curves left then add p3
        if ( hull2d_left(p1, p2, p3) )
        {
            (void)stack_push(stack, &p3idx);
            i++;
        }
        else
        {
            LOGASSERT(stack_pop(stack));
        }
    }
}

/**
* @brief Copy points from the stack into the list of hull points
* @param[in/out] hull  Pointer to the hull object
* @param[in/out] stack Pointer to stack containing indices of points that make
*                      up hull boundary.
*/
void hull2d_copyStack(hull2d_t* hull, stack_t* stack)
{
    int32_t n = stack_count(stack);

    int32_t i;
    for (i = n-1; i >= 0; --i)
    {
        LOGASSERT(stack_peek(stack, 0, & hull->boundaryIdx[i]));
        (void)stack_pop(stack);
    }

    hull->boundaryCount = n;

    LOGASSERT(stack_count(stack) == 0);
}

/**
* @brief Compute the hull using the points in the hull's point list
* @param[in/out] hull Pointer to the hull object
* @param[in/out] stack A stack of uint32_t large enough to store indicies to
*                all points. Will be used as scratch space.
* @return Returns true if able to create a hull false otherwise, only fails if
*         two or less points are in the hull or all points fall on same line.
*/
bool_t hull2d_computeHull(hull2d_t* hull, stack_t* stack)
{
    // Compute the hull using Graham's algorithm O(n log(n))

    // nothing to do
    if (hull->dirty == FALSE)
    {
        return TRUE;
    }

    LOGASSERT(stack->itemSize == sizeof(flaggedindex_t));
    LOGASSERT(stack->maxItems >= MAX_POINTS_PER_HULL);

    // verify there are enough points to build a hull
    if (hull->boundaryCount < 3)
    {
        return FALSE;
    }

    // Sort by angle from the lowest point (relative to +x vector)
    hull2d_sort(hull);

    // Remove points marked for removal
    hull2d_squash(hull);

    // verify there are still enough points to build a hull
    if (hull->boundaryCount < 3)
    {
        return FALSE;
    }

    // Run grahams algorithm
    hull2d_grahams(hull, stack);

    // Copy data from stack back to hull indices
    hull2d_copyStack(hull, stack);

    hull->dirty = false;

    return TRUE;
}

/**
* @brief Check if two line segments (a0, a1) and (b0, b1) intersect
* @param[in] a0 An endpoint of the first line segement
* @param[in] a1 An endpoint of the first line segement
* @param[in] b0 An endpoint of the second line segement
* @param[in] b1 An endpoint of the second line segement
* @return Returns TRUE if the segements intersect, FALSE otherwise
*/
bool_t hull2d_segSegIntersect(const Point2f* a0, const Point2f* a1,
    const Point2f* b0, const Point2f* b1)
{
    double s, t;
    double num, denom;

    denom = (double)a0->x * (double)( b1->y - b0->y ) +
            (double)a1->x * (double)( b0->y - b1->y ) +
            (double)b1->x * (double)( a1->y - a0->y ) +
            (double)b0->x * (double)( a0->y - a1->y );

    // check if segements are parallel (don't intersect)
    if (fabs(denom) < FLT_EPSILON)
    {
        return FALSE;
    }

    num = (double)a0->x * (double)( b1->y - b0->y ) +
          (double)b0->x * (double)( a0->y - b1->y ) +
          (double)b1->x * (double)( b0->y - a0->y );
    s = num / denom;

    num = -( (double)a0->x * (double)( b0->y - a1->y ) +
             (double)a1->x * (double)( a0->y - b0->y ) +
             (double)b0->x * (double)( a1->y - a0->y ) );
    t = num / denom;
    
    // intersection point is
    // x = a0->x + s * (a1->x - a0->x)
    // y = a0->y + t * (a1->y - a0->y)

    // Intersects if s and t are both between 0 and 1
    return ( (0.0 <= s) && (s <= 1.0) && (0.0 <= t) && (t <= 1.0) );
}

/**
* @brief Check if a point is inside a convex hull
* @param[in] hull Pointer to the hull
* @param[in] p    Pointer to the point
* @return Returns TRUE if the point is inside the hull, FALSE otherwise
*/
bool_t hull2d_pointInHull(const hull2d_t* hull, const Point2f* p)
{
    // Point must be to left of all segements
    uint32_t i, j;
    const Point2f *p0, *p1;
    p0 = &hull->points[hull->boundaryIdx[0].pointIdx];
    for (i = 0; i < hull->boundaryCount; ++i)
    {
        j = (i + 1) % hull->boundaryCount;
        p0 = &hull->points[hull->boundaryIdx[i].pointIdx];
        p1 = &hull->points[hull->boundaryIdx[j].pointIdx];
        if (!hull2d_leftOn(p0, p1, p))
        {
            return FALSE;
        }
        p0 = p1;
    }
    return TRUE;
}

/**
* @brief Check if two convex hulls intersect
* @param[in] h1 The first hull
* @param[in] h2 The second hull
* @return True if h1 intesects h2, false otherwise
*/
bool_t hull2d_checkIntersect(const hull2d_t* ha, const hull2d_t* hb)
{
    uint32_t idxA0, idxA1;
    uint32_t idxB0, idxB1;
    uint32_t aMax, bMax;
    float crossMag;
    bool_t aLeftB;
    bool_t bLeftA;

    const Point2f *a0, *a1;
    const Point2f *b0, *b1;

    // assert that hulls have been caluculated
    LOGASSERT(!ha->dirty && !ha->dirty);

    aMax = ha->boundaryCount;
    bMax = hb->boundaryCount;

    idxA0 = 0;
    idxB0 = 0;
    do
    {
        idxA1 = (idxA0 + 1) % aMax;
        idxB1 = (idxB0 + 1) % bMax;

        a0 = &ha->points[ha->boundaryIdx[idxA0].pointIdx];
        a1 = &ha->points[ha->boundaryIdx[idxA1].pointIdx];
        b0 = &hb->points[hb->boundaryIdx[idxB0].pointIdx];
        b1 = &hb->points[hb->boundaryIdx[idxB1].pointIdx];

        // test for two line segements intersecting
        if (hull2d_segSegIntersect(a0, a1, b0, b1))
        {
            return TRUE;
        }

        crossMag = (a1->x - a0->x) * (b1->y - b0->y) -
                   (a1->y - a0->y) * (b1->x - b0->x);

        aLeftB = hull2d_left(b0, b1, a1);
        bLeftA = hull2d_left(a0, a1, b1);

        // advance pointers
        if (crossMag < -FLT_EPSILON)
        {
            if ( aLeftB )
            {
                idxB0++;
            }
            else
            {
                idxA0++;
            }
        }
        else
        {
            if ( bLeftA )
            {
                idxA0++;
            }
            else
            {
                idxB0++;
            }
        }
    } while (idxA0 < aMax && idxB0 < bMax);
    // If we got here then no edges intersect

    // Check for case A subset of B
    if (hull2d_pointInHull(hb, &ha->points[ha->boundaryIdx[0].pointIdx]))
    {
        return TRUE;
    }
    // Check for case B subset of A
    else if (hull2d_pointInHull(ha, &hb->points[hb->boundaryIdx[0].pointIdx]))
    {
        return TRUE;
    }

    // otherwise they don't intersect
    return FALSE;
}
