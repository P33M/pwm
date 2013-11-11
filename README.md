pwm
===

Proof-of-concept program (hack) for testing sigma-delta modulated PWM audio output on Raspberry Pi.


Description
===

This small program, originally derived from PiFM, takes direct control of the PWM peripheral (and an associated DMA channel) to play a precomputed sample sequence. Currently, the program will only play 1 or two sine tones. Support for basic WAV playback is planned.

Configuration
===

Several variables can be set before compilation (I have not bothered to do command line parsing yet).

static int oversample_bits = 6;
static int pwm_bits = 5;

oversample_bits + pwm_bits must be <= 11 for a 48k or 44.1k sample rate. Any higher (samplerate or bits) will overclock the PWM peripheral. Oversample_bits causes the PWM carrier to be 2^os_bits * fs.

static int source_fs = 48000;
Source sample rate.


static int source_freq = 2600;
Sine tone frequency

static int play_length = 48;
Number of samples to create

static int resample = 1;
1 = apply linear resampling, 0 = Zero-order hold.

static int dither = 1;
static int dither_shift = -1;
Apply white noise dither to the error integrator signal for sigma-delta. dither_shift is the number of bits shifts relative to LSB, therefore -1 = 0.5LSB amplitude dither added.

float scale = 0.3f;
Linear scaling factor for source_freq sinewave.

Compilation
===

gcc -Wall -lm -o pwmhax pwmhax.c
