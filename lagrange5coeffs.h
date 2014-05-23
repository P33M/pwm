/*
 * Filter Coefficients (C Source) generated by the Filter Design and Analysis Tool
 *
 * Generated by MATLAB(R) 7.5 and the Filter Design Toolbox 4.2.
 *
 * Generated on: 27-Apr-2014 22:01:07
 *
 */

/*
 * Discrete-Time FIR Filter (real)
 * -------------------------------
 * Filter Structure  : Direct-Form FIR
 * Filter Length     : 191
 * Stable            : Yes
 * Linear Phase      : Yes (Type 1)
 * Arithmetic        : fixed
 * Numerator         : s16,15 -> [-1 1)
 * Input             : s16,15 -> [-1 1)
 * Filter Internals  : Specify Precision
 *   Output          : s32,31 -> [-1 1)
 *   Product         : s48,47 -> [-1 1)
 *   Accumulator     : s48,47 -> [-1 1)
 *   Round Mode      : convergent
 *   Overflow Mode   : saturate
 */

/* General type conversion for MATLAB generated C-code  */
#include "tmwtypes.h"
/* 
 * Expected path to tmwtypes.h 
 * C:\Program Files\MATLAB\R2007b\extern\include\tmwtypes.h 
 */
const int BL = 191;
const int16_T B[191] = {
       34,     68,    101,    134,    165,    196,    225,    252,    277,
      300,    321,    340,    355,    368,    378,    384,    387,    387,
      383,    375,    364,    349,    330,    308,    282,    252,    218,
      181,    141,     97,     50,      0,   -257,   -515,   -772,  -1026,
    -1276,  -1520,  -1755,  -1980,  -2193,  -2393,  -2577,  -2743,  -2890,
    -3017,  -3121,  -3200,  -3253,  -3279,  -3276,  -3242,  -3176,  -3076,
    -2942,  -2772,  -2565,  -2319,  -2035,  -1711,  -1346,   -939,   -491,
        0,   1045,   2129,   3248,   4399,   5577,   6780,   8002,   9240,
    10489,  11746,  13006,  14264,  15517,  16760,  17989,  19200,  20388,
    21549,  22679,  23774,  24829,  25841,  26806,  27720,  28579,  29379,
    30118,  30791,  31396,  31929,  32387,  32767,  32387,  31929,  31396,
    30791,  30118,  29379,  28579,  27720,  26806,  25841,  24829,  23774,
    22679,  21549,  20388,  19200,  17989,  16760,  15517,  14264,  13006,
    11746,  10489,   9240,   8002,   6780,   5577,   4399,   3248,   2129,
     1045,      0,   -491,   -939,  -1346,  -1711,  -2035,  -2319,  -2565,
    -2772,  -2942,  -3076,  -3176,  -3242,  -3276,  -3279,  -3253,  -3200,
    -3121,  -3017,  -2890,  -2743,  -2577,  -2393,  -2193,  -1980,  -1755,
    -1520,  -1276,  -1026,   -772,   -515,   -257,      0,     50,     97,
      141,    181,    218,    252,    282,    308,    330,    349,    364,
      375,    383,    387,    387,    384,    378,    368,    355,    340,
      321,    300,    277,    252,    225,    196,    165,    134,    101,
       68,     34
};
