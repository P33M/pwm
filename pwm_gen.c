/*
 * pwm_gen.c
 *
 *  Created on: 19 May 2014
 *      Author: jonathan
 *
 * Generator for SDM samples to be used in conjunction with the pwm_dma module.
 *
 * Generates offline sample streams (u8 packed) from an input WAV file (any of the supported libsndfile
 * bitrates or encodings would be acceptable).
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include <math.h>
#include <getopt.h>
#include <sndfile.h>

#include "lut_modelb.h"
#include "6point0.h"

const char usage[] = {
		"pwm_gen: Generate sigma-delta modulated n-bit samples\n"
		"-i \t Input filename (typically .wav extension)\n"
		"-o \t Output filename\n"
		"-n \t Number of bits of resolution of the output samples\n"
		"-e \t Use Vc estimator - attempt to improve DAC linearity\n"
		"--alpha \t Vc estimator Alpha value (calculated from output RC and sample period by T / (RC + T)\n"
		"--order \t Modulator order: 1st or 2nd order noise shaping\n"
		"--osr \t Oversample ratio\n"
		"--filter \t Interpolation filter to use - none, linear, lagrange3, lagrange5, matlab5\n"
};

static const struct option longopts[] = {
	{ "order", required_argument, 0, 1 },
	{ "osr", required_argument, 0, 2 },
	{ "filter", required_argument, 0, 3 },
	{ 0, 0, 0, 0 },
};

struct gen_params {
	char *inputfile;
	char *outputfile;
	int dither;
	int nbits;
	int order;
	int use_estimator;
	int osr;
	char *filtermode;
	float alpha;
} gen_params;

struct sdm_state {
	int16_t integrate1;
	int16_t integrate2;
	/* The estimate is implicitly normalised from 0V to VC_MAX by the LUT */
	uint16_t vc_estimate;
	uint16_t last_vo;
} sdm_state;


/* Prime factors of 2^16-1 = 3 5 17 257 */
uint16_t lcg1_state = 0xBEEF;
/* prime factors a-1 = 2 2 7 29 31 */
uint16_t a1 = 25173;
/* prime factors of c = 11 1259 */
uint16_t c1 = 13847;

/* Prime factors of 2^15-1 = 7 31 151 */
uint16_t lcg2_state = 0x700D;
/* prime factors of a-1 = 2 2 61 101 */
uint16_t a2 = 24645;
/* prime factors of c = 13 131 */
uint16_t c2 = 1703;

/* LCG1 + LCG2 gives a CLGC length of 2^31 */

uint32_t lcg3_state = 0xDEADBEEF;
uint32_t a3 = 214013;
uint32_t c3 = 2531011;

uint32_t lcg4_state = 0xD00DFEED;

/**
 * Returns the most-significant n bits of val. Rounds to nearest.
 */
int16_t quantise(uint16_t *val, int nbits)
{
	uint16_t error;
	uint16_t quant_mask = ~((1 << (16 - nbits)) - 1);
	//printf("quant_mask = 0x%04x val = 0x%04x ", quant_mask, *val);
	error = *val;
	*val = (*val & quant_mask) + ((*val & (1 << (16 - nbits - 1))) << 1);
	error =  error - *val;
	//printf("val2 = 0x%04x\n", *val);
	return error;
}

/* Single-LCG RPDF dither */
void dither1(uint16_t *val, int nbits)
{
	lcg1_state = (lcg1_state * a1 + c1) & 0xFFFFFFFF;
	*val += ((int16_t) lcg1_state) >> (nbits);
	//printf("dither = %d\n", ((int16_t) lcg1_state) >> (nbits));
}

/* CLCG TPDF dither */
void dither2(uint16_t *val, int nbits)
{
	lcg1_state = (lcg1_state * a1 + c1) & 0xFFFF;
	lcg2_state = (lcg2_state * a2 + c2) & 0x7FFF;
	/* Dither the LSB of the 16-bit value by both LCGs - gives TPDF */
	//printf("LCG2 = %u\n", lcg2_state);
	*val += ((int16_t) lcg1_state) >> (nbits);
	*val += ((int16_t) lcg2_state) >> (nbits - 1);
}

void dither3(uint16_t *val, int nbits)
{
	lcg3_state = (lcg3_state * a3 + c3) & 0xFFFFFFFF;
	lcg4_state = (lcg4_state * a3 + c3) & 0xFFFFFFFF;
	/* Dither the LSB of the 16-bit value by both LCGs - gives TPDF */
	//printf("LCG2 = %u\n", lcg2_state);
	/* 0.5LSB */
	*val += ((int32_t) lcg3_state) >> (nbits + 2 + 16);
	*val += ((int32_t) lcg4_state) >> (nbits + 2 + 16);
	printf("%d\n", (((int32_t) lcg3_state) >> (nbits + 18)) + (((int32_t) lcg4_state) >> (nbits + 18)));
}
/**
 * Get the computed Vo value from the DAC LUT using duty cycle and the Vc estimate
 * Caution: Vc is the output of an IIR. It needs to be quantised to the number of
 * levels in the LUT.
 */
uint16_t get_vo(uint16_t duty, uint16_t vc)
{
	return dac_lut[duty >> (16-LUT_V0_SIZE_POW2)][vc >> (16 - LUT_VC_SIZE_POW2)];
}

/**
 * Not a brain fart: this emulates the VC4 word length (16bit) used for vmul operations.
 * The returned 16bit sample is shifted to be MSB-justified in 32-bit space.
 */
uint32_t sdm_a_sample(int32_t *sample)
{
	uint16_t y;
	*sample += (1<<31);

	//y = (*sample >> 16) + ((*sample & 0x00008000) >> 15);
	y = (*sample >> 16);
	y += 2 * sdm_state.integrate1;
	y -= sdm_state.integrate2;

	if (gen_params.dither == 1)
		dither1(&y, gen_params.nbits);
	else if (gen_params.dither == 2)
		dither2(&y, gen_params.nbits);
	else if (gen_params.dither == 3)
		dither3(&y, gen_params.nbits);

	/* store last_vo */
	if (gen_params.use_estimator) {
		float val;
		uint16_t vo;
		int16_t error = y;
		quantise(&y, gen_params.nbits);
		vo = get_vo(y, sdm_state.vc_estimate);
		val = (gen_params.alpha * (float) vo) + ((1.0f - gen_params.alpha) * (float) sdm_state.vc_estimate);
		sdm_state.vc_estimate = (uint16_t) val;
		error =  error - vo;
		sdm_state.integrate2 = sdm_state.integrate1;
		sdm_state.integrate1 = error;
	} else {
		sdm_state.integrate2 = sdm_state.integrate1;
		sdm_state.integrate1 = quantise(&y, gen_params.nbits);
		sdm_state.last_vo = y;
	}
	return y >> (16 - gen_params.nbits);
}

/**
 * In-place packing function. Take the MSByte of each 32-bit sample and pack in-place
 * into 8 bytes. Must be passed a length divisible by 4.
 * Caution: makes no assumption about endianness and access thereof.
 */
void pack_in_place(int32_t *src, int length)
{
	int32_t tmp = 0;
	int i;
	for (i = 0; i < length; i+=4) {
		tmp |= (src[i] >> 24) << 0;
		tmp |= (src[i+1] >> 24) << 8;
		tmp |= (src[i+2] >> 24) << 16;
		tmp |= (src[i+3] >> 24) << 24;
		src[i / 4] = tmp;
	}
}

/*
 * Insert N-1 zeroes for every N samples.
 */
void oversample (int *input, int32_t *output, int src_length, int nr_channels)
{
	int i, j;
	if (nr_channels == 1) {
		for (i = 0; i < src_length; i++) {
			output[i * gen_params.osr] = input[i];
			//printf("output oversample = 0x%08x n = %d\n", output[(i * gen_params.osr)], i * gen_params.osr);
			for(j = 1; j < gen_params.osr; j++) {
				output[(i * gen_params.osr) + j] = 0;
				//printf("output oversample = 0x%08x n = %d\n", output[(i * gen_params.osr) + j], i * gen_params.osr + j);
			}
		}
	} else {
		/* Stereo. Use L channel only */
		for (i = 0; i < src_length; i+=2) {
			output[i * gen_params.osr] = input[i];
			for(j = 1; j < gen_params.osr; j++) {
				output[(i * gen_params.osr) + j] = 0;
			}
		}
	}
}


float last_sample_saved[FILTER_LEN];

void oversample_and_fir (int *input, int32_t *output, int src_length, int nr_channels)
{
	int i, j, k;
	float accumulator = 0.0f;
	int sample;
	float last_sample[FILTER_LEN];

	for (i = 0; i < FILTER_LEN; i++) {
		last_sample[i] = last_sample_saved[i];
	}
	for (i = 0; i < src_length; i+= nr_channels) {
		for (j = 0; j < gen_params.osr; j++) {
			if(j == 0)
				sample = input[i];
			else
				sample = 0;
			/* Compute FIR output sample given input */
			last_sample[(i * gen_params.osr + j) % FILTER_LEN] = (float) sample / 2147483647.0f;
			accumulator = 0.0f;
			for (k = 0; k < FILTER_LEN; k++) {
				int in_ptr = ((i * gen_params.osr + j - k) % FILTER_LEN);
				if (in_ptr >= 0) {
					accumulator += last_sample[in_ptr] * ((float) coefs[k] / 32767.0f);
				} else {
					accumulator += last_sample[in_ptr + FILTER_LEN] * ((float) coefs[k] / 32767.0f);
				}
			}
			output[i * gen_params.osr + j] = (int32_t) (accumulator * 2147483647.0f);
			//printf("%f, %d\n", accumulator, output[i * gen_params.osr + j]);
		}
	}
	for (i = 0; i < FILTER_LEN; i++) {
		last_sample_saved[i] = last_sample[i];
	}
}

void reset_sdm_state (void)
{
	int i;
	sdm_state.integrate1 = 0;
	sdm_state.integrate2 = 0;
	sdm_state.vc_estimate = 0;
	for (i = 0; i < FILTER_LEN; i++) {
		last_sample_saved[i] = 0.0f;
	}
}

/**
 * Scaryfunc
 */
void interpolate_filter (int32_t *input, int input_length, int osr)
{
	/* Fudge linear for now (sinc^2) */
	int i, j = 0;
	int32_t incr_val;
	int32_t sample1, sample2;
	/* Every OSRth sample is a real sample. */
	for (i = 0; i < input_length; i+=osr) {
		sample1 = input[i];
		sample2 = input[i+osr];
		incr_val = (sample2 - sample1) / osr;
		for (j = 1; j < osr; j ++) {
			//if (i == 0)
			//	continue;
			input[i + j] = sample1 + incr_val;
			input[i + j] = sample1;
			//sample1 += incr_val;
		}
	}
}

/* Maximum number of input samples to process at once */
#define INPUT_CHUNK_SIZE 4096

int main (int argc, char **argv)
{
	SNDFILE *infile = NULL;
	FILE *outfile = NULL;

	int output_buf_len;
	int i = 0;

	sf_count_t read;

	int *input_buf = NULL;
	int32_t *output_buf = NULL;
	//unsigned char *out_buf_packed = NULL;
	/* Defaults */
	gen_params.nbits = 6;
	gen_params.order = 2;
	gen_params.osr = 32;
	gen_params.dither = 3;
	gen_params.filtermode = "matlab5";
	gen_params.use_estimator = 0;
	gen_params.alpha = 0.0737f;
	gen_params.inputfile = "in.wav";

	SF_INFO snd_info = {
		.format = 0,
	};

	infile = sf_open(gen_params.inputfile, SFM_READ, &snd_info);

	/* What did we get? */
	if (infile) {
		printf("Infile opened: format = 0x%08x nrchannels = %d\n", snd_info.format, snd_info.channels);
		if (snd_info.channels == 2) {
			printf("Note: Stereo source file: only the left channel will be processed\n");
		} else if (snd_info.channels != 1) {
			printf("Invalid number of channels (must be mono or stereo only)\n");
			goto bail;
		}
	} else {
		printf("File open failed: %s\n", sf_strerror(NULL));
	}

	outfile = fopen("out.dat", "w");
	input_buf = malloc(INPUT_CHUNK_SIZE * snd_info.channels * sizeof(int));
	if (!input_buf) {
		printf("Malloc died\n");
		goto bail;
	}
	output_buf_len = gen_params.osr * INPUT_CHUNK_SIZE;
	output_buf = malloc(sizeof(int32_t) * output_buf_len);
	printf("output buf len = %d\n", output_buf_len);
	reset_sdm_state();
	printf("sdm_state.last_vo=%d sdm_state.integrate1=%d sdm_state.integrate2=%d\n", sdm_state.last_vo, sdm_state.integrate1, sdm_state.integrate2);
	/* Read input buffer, chunky samples at a time */
	do {
		read = sf_readf_int(infile, input_buf, INPUT_CHUNK_SIZE * snd_info.channels);
		output_buf_len = (read / snd_info.channels) * gen_params.osr;
		//printf("read %d samples\n", read);
		/* oversample into output buffer - note, discards all channel info */
		//oversample(input_buf, output_buf, read, snd_info.channels);
		/* FIR filter */
		//interpolate_filter(output_buf, output_buf_len, gen_params.osr);
		oversample_and_fir(input_buf, output_buf, read, snd_info.channels);
		/* SDM + quantise */
		for (i = 0; i < output_buf_len; i++) {
			output_buf[i] = sdm_a_sample(&output_buf[i]);
			//printf("%d\n", output_buf[i]);
		}
		/* Pack - we only need 6 bits of resolution so we can store that in 8. */
		//pack_in_place(output_buf, output_buf_len);
		//out_buf_packed = (unsigned char *) output_buf;
		/* Write to output file */
		fwrite(output_buf, sizeof(uint32_t), output_buf_len, outfile);

	} while (read != 0);
	printf("am i dead yet?\n");
	fflush(stdout);
	fclose(outfile);
bail:
	free(input_buf);
	free(output_buf);
	sf_close(infile);
	return 0;
}
