/*
 * Filter Coefficients (C Source) generated by the Filter Design and Analysis Tool
 *
 * Generated by MATLAB(R) 7.5 and the Filter Design Toolbox 4.2.
 *
 * Generated on: 27-Apr-2014 21:38:16
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

//#define FILTER_LEN 192
//const int16_t coefs[192] = {
//       56,    111,    165,    219,    270,    320,    367,    412,    453,
//      491,    525,    555,    580,    600,    616,    626,    630,    629,
//      622,    609,    590,    565,    534,    496,    453,    404,    350,
//      289,    224,    154,     79,      0,   -303,   -607,   -911,  -1213,
//    -1509,  -1799,  -2078,  -2346,  -2600,  -2838,  -3056,  -3254,  -3428,
//    -3578,  -3700,  -3792,  -3853,  -3881,  -3874,  -3830,  -3748,  -3626,
//    -3464,  -3259,  -3010,  -2718,  -2380,  -1997,  -1568,  -1092,   -569,
//        0,   1052,   2147,   3281,   4450,   5649,   6875,   8123,   9388,
//    10666,  11951,  13240,  14527,  15807,  17076,  18329,  19560,  20766,
//    21941,  23080,  24180,  25235,  26241,  27195,  28091,  28926,  29696,
//    30398,  31028,  31582,  32059,  32455,  32767,  32455,  32059,  31582,
//    31028,  30398,  29696,  28926,  28091,  27195,  26241,  25235,  24180,
//    23080,  21941,  20766,  19560,  18329,  17076,  15807,  14527,  13240,
//    11951,  10666,   9388,   8123,   6875,   5649,   4450,   3281,   2147,
//     1052,      0,   -569,  -1092,  -1568,  -1997,  -2380,  -2718,  -3010,
//    -3259,  -3464,  -3626,  -3748,  -3830,  -3874,  -3881,  -3853,  -3792,
//    -3700,  -3578,  -3428,  -3254,  -3056,  -2838,  -2600,  -2346,  -2078,
//    -1799,  -1509,  -1213,   -911,   -607,   -303,      0,     79,    154,
//      224,    289,    350,    404,    453,    496,    534,    565,    590,
//      609,    622,    629,    630,    626,    616,    600,    580,    555,
//      525,    491,    453,    412,    367,    320,    270,    219,    165,
//      111,     56,    0,
//};


/*
 * Filter Coefficients (C Source) generated by the Filter Design and Analysis Tool
 *
 * Generated by MATLAB(R) 7.5 and the Filter Design Toolbox 4.2.
 *
 * Generated on: 31-May-2014 14:17:52
 *
 */

/*
 * Discrete-Time FIR Filter (real)
 * -------------------------------
 * Filter Structure  : Direct-Form FIR
 * Filter Length     : 511
 * Stable            : Yes
 * Linear Phase      : Yes (Type 1)
 * Arithmetic        : fixed
 * Numerator         : s16,15 -> [-1 1)
 * Input             : s16,15 -> [-1 1)
 * Filter Internals  : Specify Precision
 *   Output          : s37,30 -> [-64 64)
 *   Product         : s31,30 -> [-1 1)
 *   Accumulator     : s37,30 -> [-64 64)
 *   Round Mode      : convergent
 *   Overflow Mode   : wrap
 */

#define FILTER_LEN 512
const int16_t coefs[FILTER_LEN] = {
       -8,    -16,    -24,    -31,    -39,    -46,    -52,    -59,    -64,
      -69,    -73,    -77,    -80,    -82,    -84,    -84,    -84,    -83,
      -82,    -79,    -76,    -72,    -67,    -61,    -55,    -49,    -41,
      -34,    -26,    -17,     -9,      0,     23,     46,     68,     90,
      111,    131,    150,    168,    184,    199,    212,    223,    231,
      238,    242,    245,    244,    242,    237,    230,    220,    209,
      195,    179,    161,    142,    121,     99,     75,     51,     26,
        0,    -50,    -99,   -148,   -196,   -243,   -287,   -329,   -368,
     -404,   -437,   -465,   -489,   -509,   -524,   -535,   -540,   -540,
     -535,   -525,   -509,   -489,   -464,   -433,   -399,   -360,   -317,
     -271,   -221,   -169,   -114,    -58,      0,     94,    188,    281,
      372,    460,    545,    626,    701,    771,    834,    889,    937,
      977,   1007,   1028,   1040,   1041,   1033,   1015,    986,    948,
      900,    843,    777,    702,    620,    530,    434,    332,    225,
      114,      0,   -164,   -328,   -491,   -651,   -808,   -959,  -1102,
    -1238,  -1363,  -1477,  -1579,  -1667,  -1740,  -1798,  -1840,  -1864,
    -1871,  -1861,  -1832,  -1785,  -1719,  -1636,  -1536,  -1419,  -1285,
    -1137,   -975,   -800,   -613,   -417,   -212,      0,    276,    555,
      833,   1109,   1379,   1642,   1894,   2133,   2356,   2562,   2748,
     2911,   3049,   3162,   3247,   3302,   3327,   3320,   3281,   3210,
     3105,   2967,   2797,   2595,   2362,   2099,   1808,   1490,   1148,
      784,    400,      0,   -483,   -976,  -1475,  -1974,  -2471,  -2959,
    -3435,  -3894,  -4331,  -4742,  -5121,  -5465,  -5769,  -6029,  -6240,
    -6400,  -6504,  -6549,  -6532,  -6450,  -6301,  -6084,  -5796,  -5437,
    -5005,  -4501,  -3925,  -3277,  -2559,  -1772,   -918,      0,   1056,
     2169,   3335,   4549,   5805,   7099,   8423,   9773,  11141,  12521,
    13907,  15291,  16666,  18026,  19363,  20671,  21942,  23171,  24350,
    25473,  26535,  27529,  28450,  29293,  30054,  30727,  31310,  31799,
    32192,  32485,  32677,  32767,  32677,  32485,  32192,  31799,  31310,
    30727,  30054,  29293,  28450,  27529,  26535,  25473,  24350,  23171,
    21942,  20671,  19363,  18026,  16666,  15291,  13907,  12521,  11141,
     9773,   8423,   7099,   5805,   4549,   3335,   2169,   1056,      0,
     -918,  -1772,  -2559,  -3277,  -3925,  -4501,  -5005,  -5437,  -5796,
    -6084,  -6301,  -6450,  -6532,  -6549,  -6504,  -6400,  -6240,  -6029,
    -5769,  -5465,  -5121,  -4742,  -4331,  -3894,  -3435,  -2959,  -2471,
    -1974,  -1475,   -976,   -483,      0,    400,    784,   1148,   1490,
     1808,   2099,   2362,   2595,   2797,   2967,   3105,   3210,   3281,
     3320,   3327,   3302,   3247,   3162,   3049,   2911,   2748,   2562,
     2356,   2133,   1894,   1642,   1379,   1109,    833,    555,    276,
        0,   -212,   -417,   -613,   -800,   -975,  -1137,  -1285,  -1419,
    -1536,  -1636,  -1719,  -1785,  -1832,  -1861,  -1871,  -1864,  -1840,
    -1798,  -1740,  -1667,  -1579,  -1477,  -1363,  -1238,  -1102,   -959,
     -808,   -651,   -491,   -328,   -164,      0,    114,    225,    332,
      434,    530,    620,    702,    777,    843,    900,    948,    986,
     1015,   1033,   1041,   1040,   1028,   1007,    977,    937,    889,
      834,    771,    701,    626,    545,    460,    372,    281,    188,
       94,      0,    -58,   -114,   -169,   -221,   -271,   -317,   -360,
     -399,   -433,   -464,   -489,   -509,   -525,   -535,   -540,   -540,
     -535,   -524,   -509,   -489,   -465,   -437,   -404,   -368,   -329,
     -287,   -243,   -196,   -148,    -99,    -50,      0,     26,     51,
       75,     99,    121,    142,    161,    179,    195,    209,    220,
      230,    237,    242,    244,    245,    242,    238,    231,    223,
      212,    199,    184,    168,    150,    131,    111,     90,     68,
       46,     23,      0,     -9,    -17,    -26,    -34,    -41,    -49,
      -55,    -61,    -67,    -72,    -76,    -79,    -82,    -83,    -84,
      -84,    -84,    -82,    -80,    -77,    -73,    -69,    -64,    -59,
      -52,    -46,    -39,    -31,    -24,    -16,     -8,      0,
};
