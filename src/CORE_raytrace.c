/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implement a raytracer from "Raytracing in One Weekend" as a test 
/// application for various CORE subsystems.
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////////
//   Preprocessor   //
////////////////////*/
/// @summary Define some useful macros for specifying common resource sizes.
#ifndef Kilobytes
    #define Kilobytes(x)                            (size_t((x)) * size_t(1024))
#endif
#ifndef Megabytes
    #define Megabytes(x)                            (size_t((x)) * size_t(1024) * size_t(1024))
#endif
#ifndef Gigabytes
    #define Gigabytes(x)                            (size_t((x)) * size_t(1024) * size_t(1024) * size_t(1024))
#endif

/// @summary Define macros for controlling compiler inlining.
#ifndef never_inline
    #define never_inline                            __declspec(noinline)
#endif
#ifndef force_inline
    #define force_inline                            __forceinline
#endif

/// @summary Helper macro to align a size value up to the next even multiple of a given power-of-two.
#ifndef align_up
    #define align_up(x, a)                          ((x) == 0) ? (a) : (((x) + ((a)-1)) & ~((a)-1))
#endif

/// @summary Helper macro to write a message to stdout.
#ifndef ConsoleOutput
    #ifndef NO_CONSOLE_OUTPUT
        #define ConsoleOutput(fmt_str, ...)         _ftprintf(stdout, _T(fmt_str), __VA_ARGS__)
    #else
        #define ConsoleOutput(fmt_str, ...)         
    #endif
#endif

/// @summary Helper macro to write a message to stderr.
#ifndef ConsoleError
    #ifndef NO_CONSOLE_OUTPUT
        #define ConsoleError(fmt_str, ...)          _ftprintf(stderr, _T(fmt_str), __VA_ARGS__)
    #else
        #define ConsoleError(fmt_str, ...)          
    #endif
#endif

#define CORE_TASK_IMPLEMENTATION

/*////////////////
//   Includes   //
////////////////*/
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include <process.h>

#include <tchar.h>
#include <Windows.h>
#include <intrin.h>

#include "CORE_task.h"

/*//////////////////
//   Data Types   //
//////////////////*/
typedef struct _ray_t
{
    float Origin[4];
    float Direction[4];
} ray_t;

/*///////////////
//   Globals   //
///////////////*/

/*//////////////////////////
//   Internal Functions   //
//////////////////////////*/
/* @summary Initialize a four-component vector value.
 * @param dst The four-component value to initialize.
 * @param x The x-component of the vector.
 * @param y The y-component of the vector.
 * @param z The z-component of the vector.
 * @param w The w-component of the vector. For points, specify 1.0, for vectors specify 0.0.
 * @return The output argument dst.
 */
static float*
vec4_set
(
    float *dst, 
    float    x, 
    float    y, 
    float    z, 
    float    w
)
{
    dst[0] = x;
    dst[1] = y;
    dst[2] = z;
    dst[3] = w;
    return dst;
}

/* @summary Negate a three-component vector value in-place.
 * @param dst The vector to negate.
 * @return The input and output argument dst.
 */
static float*
vec3_neg
(
    float *dst
)
{
    dst[0] = -dst[0];
    dst[1] = -dst[1];
    dst[2] = -dst[2];
}

/* @summary Set a four component vector value to the negation of an existing value.
 * @param dst The output vector value.
 * @param src The input value.
 * @return The output argument dst.
 */
static float*
vec3_neg_cpy
(
    float * __restrict dst, 
    float * __restrict src
)
{
    dst[0] = -src[0];
    dst[1] = -src[1];
    dst[2] = -src[2];
    dst[3] =  src[3];
    return dst;
}

/* @summary Add two three component vector values. Equivalent to dst = dst + src.
 * @param dst The input and output value.
 * @param src The value to add to dst.
 * @return The output argument dst.
 */
static float*
vec3_add
(
    float * __restrict dst, 
    float * __restrict src
)
{
    dst[0] += src[0];
    dst[1] += src[1];
    dst[2] += src[2];
    return dst;
}

/* @summary Subtract two three-component vector values. Equivalent to dst = dst - src;
 * @param dst The input and output value.
 * @param src The value to subtract from dst.
 * @return The output argument dst.
 */
static float*
vec3_sub
(
    float * __restrict dst, 
    float * __restrict src
)
{
    dst[0] -= src[0];
    dst[1] -= src[1];
    dst[2] -= src[2];
    return dst;
}

/* @summary Multiply two four-component values representing colors. Equivalent to dst = dst * src;
 * @param dst The input and output value.
 * @param src The value to component-wise multiply with dst.
 * @return The output argument dst.
 */
static float*
vec4_mul
(
    float * __restrict dst, 
    float * __restrict src
)
{
    dst[0] *= src[0];
    dst[1] *= src[1];
    dst[2] *= src[2];
    dst[3] *= src[3];
    return dst;
}

/* @summary Normalize a vector value, making it unit-length.
 * @param dst The three component vector to normalize.
 * @return The input and output value dst.
 */
static float*
vec3_nrm
(
    float * dst
)
{
    float len = (float) sqrt(dst[0] * dst[0] + dst[1] * dst[1] + dst[2] * dst[2]);
    float inv =  1.0f / len;
    dst[0] *= inv;
    dst[1] *= inv;
    dst[2] *= inv;
    return dst;
}

/* @summary Set one vector value equal to the cross product of two three-component vectors a and b.
 * @param dst The destination vector value.
 * @param a The first input vector.
 * @param b The second input vector.
 * @return The destination value dst.
 */
static float*
vec3_cross
(
    float * __restrict dst, 
    float * __restrict   a, 
    float * __restrict   b
)
{
    dst[0] = (a[1] * b[2] - a[2] * b[1]);
    dst[1] =-(a[0] * b[2] - a[2] * b[0]);
    dst[2] = (a[0] * b[1] - a[1] * b[0]);
    dst[3] =  0.0f;
    return dst;
}

/* @summary Calculate the squared length of a vector.
 * @param vec The three component vector value.
 * @return The squared length of the input vector.
 */
static float
vec3_len_sqr
(
    float * vec
)
{
    return vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2];
}

/* @summary Calculate the length of a vector.
 * @param vec The three component vector value.
 * @return The length of the input vector.
 */
static float
vec3_len
(
    float * vec
)
{
    return (float) sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
}

/* @summary Calculate the dot product of the two vectors.
 * @param a The first vector value.
 * @param b The second vector value.
 * @return The dot product of a and b.
 */
static float
vec3_dot
(
    float * a, 
    float * b
)
{
    return (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]);
}

/* @summary Initialize a ray from a given origin and direction.
 * @param dst The ray to initialize.
 * @param origin The three-component vector representing the ray origin point.
 * @param direction The three-component vector representing the ray direction.
 */
static void
ray_init
(
    ray_t * __restrict       dst,
    float * __restrict    origin, 
    float * __restrict direction
)
{
    dst->Origin[0]    = origin[0];
    dst->Origin[1]    = origin[1];
    dst->Origin[2]    = origin[2];
    dst->Origin[3]    = 1.0f;
    dst->Direction[0] = direction[0];
    dst->Direction[1] = direction[1];
    dst->Direction[2] = direction[2];
    dst->Direction[3] = 0.0f;
}

/* @summary Calculate the point dst along a ray at a given parameter value t.
 * @param dst The four-component vector to store the computed point value.
 * @param ray The ray.
 * @param t The position along the ray at which to compute the point.
 * @return The output value dst.
 */
static float*
point_at_t
(
    float * __restrict dst, 
    ray_t * __restrict ray, 
    float                t
)
{
    dst[0] = ray->Origin[0] + t * ray->Direction[0];
    dst[1] = ray->Origin[1] + t * ray->Direction[1];
    dst[2] = ray->Origin[2] + t * ray->Direction[2];
    dst[3] = 1.0f;
    return dst;
}

/* @summary Extract an RGB color value from an RGBA color value.
 * @param dst The location to write the RGB value. Three floats will be written.
 * @param src The three or four-component color value.
 * @return A pointer to the destination location.
 */
static float*
rgb
(
    float * __restrict dst, 
    float * __restrict src
)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    return dst;
}

static float*
raytrace_test_color
(
    float * __restrict dst, 
    ray_t * __restrict ray
)
{
    float     white[4];
    float      blue[4];
    float whiteness[4];
    float  blueness[4];
    float            t;
    float        t_inv;

    vec4_set(white, 1.0f, 1.0f, 1.0f, 1.0f);
    vec4_set(blue , 0.5f, 0.7f, 1.0f, 1.0f);

    /* linearly blend between white and blue */
    /* calculate t, where 0 < t < 1 used to blend between white and blue */
    vec3_nrm(ray->Direction);
    t     = 0.5f * (ray->Direction[1] + 1.0f);
    t_inv = 1.0f - t;

    /* calculate the whiteness value */
    whiteness[0] = white[0] * t_inv;
    whiteness[1] = white[1] * t_inv;
    whiteness[2] = white[2] * t_inv;
    whiteness[3] = white[3] * t_inv;

    /* calculate the blueness value */
    blueness [0] = blue [0] * t;
    blueness [1] = blue [1] * t;
    blueness [2] = blue [2] * t;
    blueness [3] = blue [3] * t;

    return rgb(dst, vec3_add(whiteness, blueness));
}

static void
raytrace_test
(
    float *framebuffer, 
    int          width,
    int         height
)
{
    /* the camera is located at (0, 0, 0) */
    /* the x-axis increases to the right */
    /* the y-axis increases going up */
    /* the z-axis decreases going into the screen */
    float lower_left_corner[4];
    float        horizontal[4];
    float          vertical[4];
    float            origin[4];
    float                 x[4];
    float                 y[4];
    float               dir[4];
    ray_t                  ray;
    float                 u, v;
    int                   i, j;

    vec4_set(origin           , 0.0f                   ,   0.0f                    ,  0.0f, 0.0f);
    vec4_set(vertical         , 0.0f                   ,   2.0f * (height / 100.0f),  0.0f, 0.0f);
    vec4_set(horizontal       , 2.0f * (width / 100.0f),   0.0f                    ,  0.0f, 0.0f);
    vec4_set(lower_left_corner,       -(width / 100.0f),         -(height / 100.0f), -1.0f, 1.0f); 

    for (j = height - 1; j >= 0; --j)
    {
        for (i = 0; i < width; ++i)
        {
            u    = (float) i / (float) width;
            v    = (float) j / (float) height;
            x[0] =  u * horizontal[0];
            x[1] =  u * horizontal[1];
            x[2] =  u * horizontal[2];
            x[3] =  u * horizontal[3];
            y[0] =  v * vertical[0];
            y[1] =  v * vertical[1];
            y[2] =  v * vertical[2];
            y[3] =  v * vertical[3];
            vec3_add(  x, y);
            vec3_add(dir, x);
            vec3_add(dir, lower_left_corner);
            ray_init(&ray, origin, dir);
            raytrace_test_color(framebuffer, &ray);
            framebuffer += 3;
        }
    }
}

/* @summary Allocate a three-channel (RGB) floating point image with given dimensions.
 * @param width The image width, in pixels.
 * @param height The image height, in pixels.
 * @return A pointer to the start of the first pixel, or NULL if the image data could not be allocated.
 */
static float*
AllocateImage
(
    int  width, 
    int height
)
{
    return (float*) malloc(width * height * 3 * sizeof(float));
}

/* @summary Free an image buffer allocated by a prior call to AllocateImage.
 * @param image The image buffer to free.
 */
static void
FreeImage
(
    float *image
)
{
    free(image);
}

/* @summary Write a three-channel RGB image, where each channel is a floating-point number, to a PPM file.
 * @param path A nul-terminated string specifying the path and filename of the PPM file to create.
 * @param data An array of width * height pixels, where each pixel is represented by three floating point values representing the red, green and blue channel.
 * @param width The width of the image, in pixels.
 * @param height The height of the image, in pixels.
 * @return Zero if the PPM file is written successfully, or -1 if an error occurs.
 */
static int
WritePPM
(
    char const *path, 
    float      *data, 
    int        width, 
    int       height
)
{
    int i, j;
    FILE *fp  = fopen(path, "w+");
    if   (fp == NULL)
    {
        ConsoleError("ERROR: Failed to open \"%S\" for writing.\n", path);
        return -1;
    }
    /* write the header data. P3 means colors are in ASCII RGB triplets, max value 255 */
    fprintf(fp, "P3\n");
    fprintf(fp, "%d %d\n", width, height);
    fprintf(fp, "255\n");
    for (j = height - 1; j >= 0; --j)
    {
        for (i = 0; i < width; ++i)
        {
            float r = *data++;
            float g = *data++;
            float b = *data++;
            fprintf(fp, "%d %d %d\n", (int)(255.99f * r), (int)(255.99f * g), (int)(255.99f * b));
        }
    }
    fprintf(fp, "\n");
    fclose(fp);
    return 0;
}

/*////////////////////////
//   Public Functions   //
////////////////////////*/
/// @summary Implement the entry point of the application.
/// @param argc The number of arguments passed on the command line.
/// @param argv An array of @a argc zero-terminated strings specifying the command-line arguments.
/// @return Zero if the function completes successfully, or non-zero otherwise.
int 
main
(
    int    argc, 
    char **argv
)
{
    char const *ppm = "rtout.ppm";
    float       *fb = NULL;
    int           w =  800;
    int           h =  600;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    fb = AllocateImage(w, h);
    raytrace_test(fb, w, h);
    WritePPM(ppm, fb, w, h);
    FreeImage(fb);
    return 0;
}

