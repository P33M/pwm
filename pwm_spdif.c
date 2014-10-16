/*
 * pwm_spdif.c
 *
 *  Created on: 15 May 2014
 *      Author: Jonathan
 *
 * Alternate mode for PWM: it is possible for the peripheral to serially output data.
 *
 * Generates the SPDIF-formatted raw file suitable for playback directly via the PWM output.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include <getopt.h>
#include <sndfile.h>

const char usage[] = {
		"pwm_spdif: Generate 32-bit SPDIF words from an input sound file\n"
		"-i \t Input filename (typically .wav extension)\n"
		"-o \t Output filename\n"
		"-n \t Number of bits - pad the input bitstream to n bits (16-24)\n"
};

//static const struct option longopts[] = {
//	{ "order", required_argument, 0, 1 },
//	{ "osr", required_argument, 0, 2 },
//	{ "filter", required_argument, 0, 3 },
//	{ 0, 0, 0, 0 },
//};

struct encoder_params {
	char *inputfile;
	char *outputfile;
	int nbits;
	int nchannels;
} encoder_params;

/* 192-bit subchannel data */
uint32_t subchannel_data[6] = {
	0, 0, 0, 0, 0, 0,
};

typedef enum {
	/* Channel A, start-of-datablock */
	B  = 0,
	nB = 1,
	/* Channel A, not start-of-datablock */
	M  = 2,
	nM = 3,
	/* Channel B (or N for multichannel) */
	W  = 4,
	nW = 5,
} bmc_preamble_t;

uint8_t preambles[6] = {
	0b11101000,
	0b00010111,
	0b11100010,
	0b00011101,
	0b11100100,
	0b00011011,
};

typedef enum {
	CHAN_L,
	CHAN_R,
} channel_t;

struct encoder_state {
	/* The subchannel data is 192 bits of information, most of it unused. */
	int subchannel_ptr;
	/* Preamble for the next 32-bit word is dependent on the bit of the last word */
	bmc_preamble_t next_preamble;
	int biphase;
} enc_state;


/* Biphase mark-encoder for a 32-bit SPDIF word. writes two 32-bit words to *out. */
inline void bmc_enc(uint32_t in, uint32_t *out) {
	uint32_t tmp = 0;
	int i;
	for (i = 31; i > 0; i--) {
		if (i == 31) {
			/* Add preamble */
			tmp |= preambles[enc_state.next_preamble] << 24;
			i -= 7;
			enc_state.biphase = preambles[enc_state.next_preamble] & 0x1;
			continue;
		}
		/* Flip bit at start of Biphase encode */
		tmp |= ((!enc_state.biphase) << i);
		/* Input bit high? transition in middle of biphase */
		if (in & (1 << (i/2 + 16))) {
			tmp |= ((enc_state.biphase) << (i-1));
		} else {
			tmp |= ((!enc_state.biphase) << (i-1));
		}
		enc_state.biphase = (tmp & (1 << (i-1))) ? 1 : 0;
		i--;
	}
	tmp = __builtin_bswap32(tmp);
	*out = tmp;
	out++;
	tmp = 0;
	for (i = 31; i > 0; i -= 2) {
		/* Flip bit at start of Biphase encode */
		tmp |= ((!enc_state.biphase) << i);
		/* Input bit high? transition in middle of biphase */
		if (in & (1 << (i/2))) {
			tmp |= ((enc_state.biphase) << (i-1));
		} else {
			tmp |= ((!enc_state.biphase) << (i-1));
		}
		enc_state.biphase = (tmp & (1 << (i-1))) ? 1 : 0;
	}
	/* __cpu_to_be32() maybe */
	tmp = __builtin_bswap32(tmp);
	*out = tmp;
}

/* Sets the last bit of a SPDIF subframe depending on odd parity */
inline void set_word_parity(uint32_t *word) {
	uint32_t tmp = *word & 0x0FFFFFFE;
	tmp = __builtin_popcount(tmp);
	*word &= ~0x1;
	*word |= ~tmp & 0x1;
}


inline uint32_t bitrev32(uint32_t word) {
	// swap odd and even bits
	word = ((word >> 1) & 0x55555555) | ((word & 0x55555555) << 1);
	// swap consecutive pairs
	word = ((word >> 2) & 0x33333333) | ((word & 0x33333333) << 2);
	// swap nibbles ...
	word = ((word >> 4) & 0x0F0F0F0F) | ((word & 0x0F0F0F0F) << 4);
	// swap bytes
	word = ((word >> 8) & 0x00FF00FF) | ((word & 0x00FF00FF) << 8);
	// swap 2-byte long pairs
	word = ( word >> 16             ) | ( word               << 16);
	return word;
}

static int framepos = 0;
void spdif_format(uint16_t *in, uint32_t *out, int nchannels) {
	int i;
	uint32_t tmp = 0;
	uint16_t rev = 0;
	for (i = 0; i < nchannels; i++) {
		if (i == 0) {
			if (framepos == 0) {
				if (enc_state.biphase)
					enc_state.next_preamble = B;
				else
					enc_state.next_preamble = nB;
			} else {
				if (enc_state.biphase)
					enc_state.next_preamble = M;
				else
					enc_state.next_preamble = nM;
			}

		} else {
			 if (enc_state.biphase)
				 enc_state.next_preamble = W;
			 else
				 enc_state.next_preamble = nW;
		}
		/* Arrrgh this is terrible. The SPDIF data is transmitted *LSB* first
		 * and right-aligned. Who the hell thought that this was a good idea?
		 */
		tmp = *in << 16;
		tmp = bitrev32(tmp);
		rev = (uint16_t) (tmp >> 16);
		set_word_parity(&tmp);
		bmc_enc(tmp, out);
	}
	framepos++;
	if (framepos >= 192) {
		framepos = 0;
	}
}


#define BE4(x) ((x)>>24)&0xff,((x)>>16)&0xff,((x)>>8)&0xff,((x)>>0)&0xff

//int main (void) {
//	uint32_t word;
//	uint32_t output[2];
//	enc_state.next_preamble = B;
//	enc_state.biphase = 0;
//	word = 0xF0F01010;
//	set_word_parity(&word);
//	bmc_enc(word, &output[0]);
//	printf("word = 0x%08x, %02x%02x%02x%02x%02x%02x%02x%02x, \n",
//			word, BE4(output[0]), BE4(output[1]));
//	printf("out = 0x%08x, 0x%08x\n", output[0], output[1]);
//	enc_state.next_preamble = (output[1] & 0x1) ? nW : W;
//	bmc_enc(word, &output[0]);
//	printf("word = 0x%08x, %02x%02x%02x%02x%02x%02x%02x%02x, \n",
//			word, BE4(output[0]), BE4(output[1]));
//
//	/* Open infile */
//	/* Open outfile */
//	/* While infile_samples, convert and write to outfile */
//	return 0;
//}
#define CHUNK_SIZE 1024

int main (int argc, char **argv) {
	SNDFILE *infile = NULL;
	FILE *outfile = NULL;
	SF_INFO snd_info = {
			.format = 0,
	};
	sf_count_t read;
	int i, frame_index = 0;
	opterr = 0;
	short *input_buf = NULL;
	uint32_t *output_buf = NULL;
	uint32_t *subframe = NULL;
	encoder_params.nbits = 16;

	while ((i = getopt(argc, argv, "i:o:n:")) != -1) {
		switch (i) {
		case 'i':
			encoder_params.inputfile = optarg;
			break;
		case 'o':
			encoder_params.outputfile = optarg;
			break;
		case 'n':
			encoder_params.nbits = atoi(optarg);
		}
	}
	uint32_t derp = 0x12345678;
	printf("derp = 0x%08x 0x%08x\n", derp, bitrev32(derp));

	infile = sf_open(encoder_params.inputfile, SFM_READ, &snd_info);
	/* What did we get? */
	if (infile) {
		printf("Infile opened: format = 0x%08x nrchannels = %d\n", snd_info.format, snd_info.channels);
		if (snd_info.channels == 2) {
			printf("Stero source file\n");
		} else if (snd_info.channels != 1) {
			printf("Invalid number of channels (must be mono or stereo only)\n");
			goto bail;
		}
		encoder_params.nchannels = snd_info.channels;
	} else {
		printf("File open failed: %s\n", sf_strerror(NULL));
		goto bail;
	}

	outfile = fopen(encoder_params.outputfile, "w");

	if(!outfile) {
		printf("Can't open outputfile\n");
		goto bail;
	}
	input_buf = malloc(CHUNK_SIZE * sizeof(short) * encoder_params.nchannels);
	output_buf = malloc(CHUNK_SIZE * 2 * sizeof(uint32_t) * encoder_params.nchannels);
	subframe = malloc(sizeof(uint32_t) * encoder_params.nchannels);
	if (!input_buf || !output_buf || !subframe) {
		printf("Malloc died\n");
		goto bail;
	}

	do {
		read = sf_readf_short(infile, input_buf, CHUNK_SIZE);
		/* I read N frames */
		for (i = 0; i < read; i++) {
			spdif_format(input_buf, output_buf, encoder_params.nchannels);
			input_buf += encoder_params.nchannels;
			output_buf += encoder_params.nchannels * 2;
		}
		fwrite(output_buf, sizeof(uint32_t), 2 * encoder_params.nchannels * read, outfile);
	} while (read != 0);
bail:
	return 0;
}

