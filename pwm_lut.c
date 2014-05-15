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
#include <getopt.h>
#include <string.h>
#include <sys/types.h>

const char usage[] = {
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
		{	"an", required_argument, 0, 0 },
		{	"bn", required_argument, 0, 0 },
		{	"ap", required_argument, 0, 0 },
		{	"bp", required_argument, 0, 0 },
		{	"ra", required_argument, 0, 0 },
		{	"rb", required_argument, 0, 0 },
		{	"vdac", required_argument, 0, 0 },
		{	"file", required_argument, 0, 0 },
		{	"csv", no_argument, 0, 0 },
		{ 0, 0, 0, 0 },
};

typedef struct {
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
	/* Calculate VDSn given Ro, Vc, a and b. For VDSp, use vdac also */
	for (i = 0; i < 10; i++) {
		/* Find IDS through resistor */
		if (leg)
			ids = (vc - vds) / r1;
		else
			ids = (vdac - vc - vds) / r1;
		//printf("ids = %f\n", ids);
		vds1 = (0.0f - b) + sqrt(b*b - 4.0f * a * (0.0f - ids));
		vds1 = vds1 / (2.0f * a);
		vds2 = (0.0f - b) - sqrt(b*b - 4.0f * a * (0.0f - ids));
		vds2 = vds1 / (2.0f * a);
		if (vds1 > 0) {
			vds = vds1;
		} else if (vds2 > 0) {
			vds = vds2;
		}
		//printf("vds = %f\n", vds);
	}
	return vds;
}

float ** make_array (int x, int y)
{
	int i = 0;
	float ** ret = malloc(x * sizeof(float *));
	for (i = 0; i < x; i++) {
		ret[i] = malloc(y * sizeof(float));
	}
	printf("allocated x = %d y = %d\n", x, y);
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

float calc_max_vc (float ra, float rb, float vdac)
{
	return (rb / (ra + rb)) * vdac;
}

int main (int argc, char **argv) {
	dac_params dac_params;
	int index = 0;
	int i, j;
	float **array;
	char *filename = "out.csv";
	FILE *outfile = NULL;

	dac_params.an = -0.02f;
	dac_params.bn = 0.08f;
	dac_params.ap = -0.02f;
	dac_params.bp = 0.08f;
	dac_params.ra = 100.0f;
	dac_params.rb = 100.0f;
	dac_params.vdac = 2.50f;
	dac_params.x = 64;
	dac_params.y = 32;

	array = make_array(dac_params.x, dac_params.y);

	outfile = fopen(filename, "w");

	/* For each duty, calculate Vo given Vc. Note: Vc is the voltage across Rb. */
	dac_params.max_vc = calc_max_vc(dac_params.ra, dac_params.rb, dac_params.vdac);
	for (i = 0; i < dac_params.x; i++) {
		for (j = 0; j < dac_params.y; j++) {
			float vc = dac_params.max_vc * ((float) j / ((float) dac_params.y - 1.0f));
			float vn = converge (vc, dac_params.vdac, dac_params.ra, dac_params.an, dac_params.bn, 1);
			float vp = converge (vc, dac_params.vdac, dac_params.ra, dac_params.ap, dac_params.bp, 0);
			float d = ((float) i / ((float) dac_params.x - 1.0f));
			array[i][j] = d * (dac_params.vdac - vp - vn) + (1.0f - d) * vn;
			printf("vc=%f  vp=%f vn=%f x=%d y=%d\n", vc, vp, vn, i, j);
		}
	}
	write_csv(dac_params.x, dac_params.y, array, outfile);
	fprintf(outfile, "\n");
	fclose(outfile);
	return 0;
}

