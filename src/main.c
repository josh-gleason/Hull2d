#include "hull2d.h"

#include <time.h>
#include <stdio.h>

#include <GL/glut.h>

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

// This is just data used for the glut display function
typedef struct displaydata_s
{
    hull2d_t *h1, *h2;
    stack_t  *stack;
    bool_t    intersect;
    int       appWidth, appHeight;
} displaydata_t;

displaydata_t displaydata;

/**
* @brief Generate sample from uniform distribution U(0,1)
* @return Random double precision number between 0 and 1
*/
double randf()
{
    return (double)rand() / (double)(RAND_MAX);
}

/**
* @brief Generate a random, normally distributed value from N(0,1)
* @return Random number
*/
double randn()
{
    double r;
    double u1, u2;
    do {
        u1 = randf();
    } while(u1 <= DBL_EPSILON);
    u2 = randf();
    r = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
    return r;
}

/**
* @brief Mouse callback
* @param[in] button The mouse button that was clicked one of
*                   GLUT_LEFT_BUTTON, GLUT_RIGHT_BUTTON, GLUT_MIDDLE_BUTTON
* @param[in] state The state of the button either GLUT_UP or GLUT_DOWN
* @param[in] x,y   The window relative coordinates of the mouse in pixels
*/
void mouse(int button, int state, int x, int y)
{
    if (state != GLUT_DOWN)
    {
        return;
    }

    // coordinates of the mouse in OpenGL coordinates (-1.0 to 1.0)
    Point2f p;
    p.x = 2.0f * ((float)x / (float)displaydata.appWidth) - 1.0f;
    p.y = -2.0f * ((float)y / (float)displaydata.appHeight) + 1.0f;

    if (button == GLUT_LEFT_BUTTON)
    {
        // add point to h1
        hull2d_addPoint(displaydata.h1, &p);
        (void)hull2d_computeHull(displaydata.h1, displaydata.stack);
    }
    else if (button == GLUT_RIGHT_BUTTON)
    {
        // add point to h2
        hull2d_addPoint(displaydata.h2, &p);
        (void)hull2d_computeHull(displaydata.h2, displaydata.stack);
    }
    else if (button == GLUT_MIDDLE_BUTTON)
    {
        // clear both hulls
        hull2d_clear(displaydata.h1);
        hull2d_clear(displaydata.h2);
        displaydata.intersect = FALSE;
    }

    // check for intersection
    if (!displaydata.h1->dirty && !displaydata.h2->dirty)
    {
        displaydata.intersect =
            hull2d_checkIntersect(displaydata.h1, displaydata.h2);
    }
}

/**
* @brief Resize callback
* @param[in] width Width of app after resize in pixels
* @param[in] height Height of app after resize in pixels
*/
void resize(int width, int height)
{
    displaydata.appWidth = width;
    displaydata.appHeight = height;
}

/**
* @brief Glut display function
*/
void display(void)
{
    uint32_t i, hidx;
    uint32_t nhulls = 2;
    hull2d_t* hulls[2];

    hull2d_t* h;

    hulls[0] = displaydata.h1;
    hulls[1] = displaydata.h2;

    // set clear color based on intersection test
    if (displaydata.intersect)
    {
        glClearColor(1.0f, 0.6f, 0.6f, 1.0f);
    }
    else
    {
        glClearColor(0.6f, 1.0f, 0.6f, 1.0f);
    }

    glClear(GL_COLOR_BUFFER_BIT);
    for (hidx = 0; hidx < nhulls; ++hidx)
    {
        h = hulls[hidx];

        if (!h->dirty)
        {
            // draw black outline
            glBegin(GL_LINE_LOOP);
            glColor3f(0.0f, 0.0f, 0.0f);
            for (i = 0; i < h->boundaryCount; ++i)
            {
                glVertex3f(h->points[h->boundaryIdx[i].pointIdx].x,
                           h->points[h->boundaryIdx[i].pointIdx].y, 0.0f);
            }
            glEnd();
        

            // draw edge points as large red
            glPointSize(6.0f);
            glBegin(GL_POINTS);
            glColor3f(1.0f, 0.0f, 0.0f);
            for (i = 0; i < h->boundaryCount; ++i)
            {
                glVertex3f(h->points[h->boundaryIdx[i].pointIdx].x,
                           h->points[h->boundaryIdx[i].pointIdx].y, 0.0f);
            }
            glEnd();
        }

        // draw all points as small blue
        glPointSize(3.0f);
        glBegin(GL_POINTS);
        if (hidx % 3 == 0)
        {
            glColor3f(0.0f, 0.0f, 1.0f);
        }
        else if (hidx % 3 == 1)
        {
            glColor3f(1.0f, 0.5f, 0.0f);
        }
        else
        {
            glColor3f(0.5f, 0.1f, 1.0f);
        }

        for (i = 0; i < h->pointCount; ++i)
        {
            glVertex3f(h->points[i].x, h->points[i].y, 0.0f);
        }
        glEnd();
    }

    glutSwapBuffers();
}

/**
* @brief Main program entry
*/
int main(int argc, char* argv[])
{
    stack_t stack;
    hull2d_t h1, h2;

    Point2f c1, c2;
    Point2f s1, s2;

    int32_t i;
    uint32_t seed;

    // initialize hulls
    hull2d_init(&h1);
    hull2d_init(&h2);

    // define the means and standard deviations
    c1.x = -0.5f;
    c1.y = -0.5f;
    s1.x =  0.15f;
    s1.y =  0.15f;

    c2.x =  0.5f;
    c2.y =  0.5f;
    s2.x =  0.15f;
    s2.y =  0.15f;

    // seed the random number generator
    seed = (uint32_t)time(NULL);
    srand(seed);
    fprintf(stdout, "Random Seed: %d\n", seed);

    // add random points to the hulls
    Point2f p;
    for (i = 0; i < 200; ++i)
    {
        // lower left hull nothing fancy
        p.x = (float)(randn())*s1.x + c1.x;
        p.y = (float)(randn())*s1.y + c1.y;
        hull2d_addPoint(&h1, &p);

        // lower right force some values to bottom
        p.x = (float)(randn())*s2.x + c2.x;
        p.y = (float)(randn())*s2.y + c2.y;
        hull2d_addPoint(&h2, &p);
    }

    // Compute the hulls for h1 and h2
    hull2d_initStack(&stack);
    (void)hull2d_computeHull(&h1, &stack);
    (void)hull2d_computeHull(&h2, &stack);

    // Initialize the display data structure
    displaydata.h1        = &h1;
    displaydata.h2        = &h2;
    displaydata.stack     = &stack;
    displaydata.intersect = hull2d_checkIntersect(&h1, &h2);
    displaydata.appHeight = 500;
    displaydata.appWidth = 500;

    // Initialize glut
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE);
    glutInitWindowSize(displaydata.appWidth, displaydata.appHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Hull Intersect");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

    // set callbacks
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutReshapeFunc(resize);

    // infinite loop
    glutMainLoop();

    return 0;
}

