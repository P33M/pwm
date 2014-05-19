/*
 * pwm_lut.c
 *
 *  Created on: 15 May 2014
 *      Author: Jonathan
 *
 * Generates a N x M lookup table for a CMOS
 * totem-pole output driver operating as a PWM DAC.
 */


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>

const char usage[] = {
	"pwm_lut: usage\n"
	"Creates a CSV table of computed output voltages of a CMOS n-bit PWM DAC\n"
	"--an \t N-MOSFET vds^2 curve-fit term\n"
	"--bn \t N-MOSFET vds curve-fit term\n"
	"--ap \t P-MOSFET vds^2 curve-fit term\n"
	"--bp \t P-MOSFET vds curve-fit term\n"
	"--ra \t R1 resistor value\n"
	"--rb \t R2 resistor value\n"
	"--vdac \t DAC driving voltage\n"
	"-x n \t Number of quantization levels of the PWM output duty\n"
	"-y n \t Number of quantization levels of the output voltage Vc\n"
	"--file \t Output filename\n"
	"--csv \t Generate CSV data (default to generate C fragment)\n",
};

static const struct option longopts[] = {
	{ "an", required_argument, 0, 1 },
	{ "bn", required_argument, 0, 2 },
	{ "ap", required_argument, 0, 3 },
	{ "bp", required_argument, 0, 4 },
	{ "ra", required_argument, 0, 5 },
	{ "rb", required_argument, 0, 6 },
	{ "vdac", required_argument, 0, 7 },
	{ "file", required_argument, 0, 8 },
	{ "csv", no_argument, 0, 9 },
	{ 0, 0, 0, 0 },
};

struct dac_params {
	float an;
	float bn;
	float ap;
	float bp;
	float ra;
	float rb;
	float vdac;
	float max_vc;
	int x;
	int y;
	int csv_out;
} dac_params;

float converge (float vc, float vdac, float r1, float a, float b, int leg)
{
	int i = 0;
	float vds = 0.0f;
	float vds1, vds2;
	float ids = 0.0f;
	/* Iteratively solve the intersection of the VDS/IDS load-line with the
	 * -1/R1 slope load-line. */
	for (i = 0; i < 10; i++) {
		/* Find IDS through R1 */
		if (leg)
			ids = (vc - vds) / r1;
		else
			ids = (vdac - vc - vds) / r1;
		if (ids < 0.0f)
			break;
		/* Solve quadratic fit curve to find VDS */
		vds1 = (0.0f - b) + sqrt(b*b - 4.0f * a * (0.0f - ids));
		vds1 = vds1 / (2.0f * a);
		vds2 = (0.0f - b) - sqrt(b*b - 4.0f * a * (0.0f - ids));
		vds2 = vds1 / (2.0f * a);
		if (vds1 > 0) {
			vds = vds1;
		} else if (vds2 > 0) {
			vds = vds2;
		}
	}
	return vds;
}

float ** make_array (int x, int y)
{
	int i = 0;
	float ** ret = malloc(x * sizeof(float *));
	if (!ret)
		return NULL;
	for (i = 0; i < x; i++) {
		ret[i] = malloc(y * sizeof(float));
		if (!ret[i])
			return NULL;
	}
	return ret;
}

void write_csv (int x, int y, float **array, FILE *fp)
{
	int i = 0, j = 0;
	for (i = 0; i < x; i++) {
		for (j = 0; j < y; j ++) {
			fprintf(fp, "%.6e, ", array[i][j]);
		}
		fprintf(fp, "\n");
	}
}

void write_c (int x, int y, float **array, FILE *fp)
{
	int i = 0, j = 0;
	fprintf(fp, "/**\n"
			" * CMOS DAC duty-cycle linearisation LUT\n"
			" * Computed with the following parameters:\n"
			" * PWM range=%d Vc nlevels=%d\n"
			" * PMOS k'n*(Vgs-Vt)=%f -0.5*k'n=%f\n"
			" * NMOS k'n*(Vgs-Vt)=%f -0.5*k'n=%f\n"
			" * Vdac=%f R1=%f R2=%f\n"
			" **/\n", dac_params.x, dac_params.y,
			dac_params.ap, dac_params.bp, dac_params.an, dac_params.bn,
			dac_params.vdac, dac_params.ra, dac_params.rb);
	fprintf(fp, "const int16_t dac_lut[%d][%d] = {\n", x, y);
	for (i = 0; i < x; i++) {
		fprintf(fp, "\t{");
		for (j = 0; j < y; j++) {
			/* Convert to duty-referenced */
			float out_scaled = 4294967294.0f * (array[i][j] / dac_params.vdac);
			/* 16-bit precision is enough for anybody */
			if (i >= 60) {
				printf("out_scale=%f\n", out_scaled);
			}
			uint16_t out_quantised = ((((uint32_t) out_scaled) & 0xFFFF0000) + ((((uint32_t) out_scaled) & 0x00008000) << 1)) >> 16;
			fprintf(fp, " 0x%04x, ", out_quantised);
		}
		fprintf(fp, "},\n");
	}
	fprintf(fp, "};\n");
}

float calc_max_vc (float ra, float rb, float vdac)
{
	return (rb / (ra + rb)) * vdac;
}

int main (int argc, char **argv) {
	int index = 0;
	int i, j;
	float **array;

	char *filename = "out.csv";
	FILE *outfile = NULL;

	dac_params.csv_out = 0;
	opterr = 0;
	i = getopt_long(argc, argv, "x:y:", longopts, &index);
	while (i != -1) {
		switch(i) {
		case 'x':
			dac_params.x = atoi(optarg);
			break;
		case 'y':
			dac_params.y = atoi(optarg);
			break;
		case 0:
			break;
		case 1:
			dac_params.an = atof(optarg);
			break;
		case 2:
			dac_params.bn = atof(optarg);
			break;
		case 3:
			dac_params.ap = atof(optarg);
			break;
		case 4:
			dac_params.bp = atof(optarg);
			break;
		case 5:
			dac_params.ra = atof(optarg);
			break;
		case 6:
			dac_params.rb = atof(optarg);
			break;
		case 7:
			dac_params.vdac = atof(optarg);
			break;
		case 8:
			filename = optarg;
			break;
		case 9:
			dac_params.csv_out = 1;
			break;
		case '?':
			printf("option=%d\n", optopt);
			printf(usage);
			exit(-1);
			break;
		case ':':
			printf("Error: Option %s requires an argument.\n", argv[optopt]);
			printf(usage);
			exit(-1);
			break;
		default:
			printf("wat\n");
			break;
		}
		i = getopt_long(argc, argv, "x:y:", longopts, &index);
	}
	if(opterr) {
		printf("Invalid argument\n");
		printf(usage);
		exit(-1);
	}
	printf("Parms:\n"
		"an=%f bn=%f ap=%f bp=%f\n"
		"vdac=%f ra=%f rb=%f\n", dac_params.an, dac_params.bn, dac_params.ap, dac_params.bp,
		dac_params.vdac, dac_params.ra, dac_params.rb);

	array = make_array(dac_params.x, dac_params.y);
	if(!array) {
		printf("Error: failed to allocate memory for LUT\n");
		exit(-1);
	}

	outfile = fopen(filename, "w");
	if(!outfile) {
		printf("Error: Cannot open output file '%s' for writing.\n", filename);
		free(array);
		exit(-1);
	}
	/* For each duty, calculate Vo given Vc. Note: Vc is the voltage across Rb. */
	dac_params.max_vc = calc_max_vc(dac_params.ra, dac_params.rb, dac_params.vdac);
	for (i = 0; i < dac_params.x; i++) {
		for (j = 0; j < dac_params.y; j++) {
			float vn, vp, vc;
			vc = dac_params.max_vc * ((float) j / ((float) dac_params.y - 1.0f));
			vn = converge (vc, dac_params.vdac, dac_params.ra, dac_params.an, dac_params.bn, 1);
			vp = converge (vc, dac_params.vdac, dac_params.ra, dac_params.ap, dac_params.bp, 0);
			printf ("vn=%f vp=%f i=%d j=%d\n", vn, vp, i, j);
			float d = ((float) i / ((float) dac_params.x - 1.0f));
			array[i][j] = d * (dac_params.vdac - vp) + (1.0f - d) * vn;
		}
	}
	if (dac_params.csv_out) {
		write_csv(dac_params.x, dac_params.y, array, outfile);
	} else {
		/* Write LUT as a set of hex digits suitable for array definition */
		write_c(dac_params.x, dac_params.y, array, outfile);
	}
	fprintf(outfile, "\n");
	fclose(outfile);
	return 0;
}

