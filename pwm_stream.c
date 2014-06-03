/*
 * pwm_stream.c
 *
 *  Created on: 3 Jun 2014
 *      Author: jonathan
 *
 * Interface (and example) to the streaming PWM-DMA driver.
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>
#include <sys/ioctl.h>

enum {
	/* Ringbuffer mode streaming - for continuous streams */
	PWM_DMA_IOCTL_STREAMON = _IO('S', 0x01),
	PWM_DMA_IOCTL_STREAMOFF = _IO('S', 0x02),
	/* Use value in DAT register */
	PWM_DMA_IOCTL_USE_DAT = _IO('S', 0x03),

	/* Do not use the following IOCTLs without first stopping streaming. Output glitches may result. */
	/* Output generation mode select */
	PWM_DMA_IOCTL_MS_MODE = _IO('S', 0x10),
	PWM_DMA_IOCTL_PDM_MODE = _IO('S', 0x11),
	PWM_DMA_IOCTL_SERIAL_MODE = _IO('S', 0x12),

	/* Note: divider is frequency divide from PLL output channel. */
	PWM_DMA_IOCTL_DIVIDER = _IOWR('S', 0x20, uint32_t),
	/* For MS/PDM modes - set the range and DAT value */
	PWM_DMA_IOCTL_RANGE = _IOWR('S', 0x21, uint32_t),
	PWM_DMA_IOCTL_DUTY = _IOWR('S', 0x22, uint32_t),
};

const char usage[] = {
	"pwm_stream: usage\n"
	"Interface to /dev/pwm_dma and stream a raw data file (modified by options):\n"
	"--pwm-mode \t \n\t\tpdm: use 1-bit pulse-density modulation\n\t\tpwm: use mark-space PWM\n\t\tserial: serialise output bitstream",
	"--infile \t Sample data to stream. Use - for stdin (default)\n",
	"--range \t PWM/serialiser cycle count - duty cycle for PWM calculated by input_value/range",
	"--divider \t PWM block clock divider from the dedicated PLL",
};

static const struct option longopts[] = {
	{ "pwm-mode", required_argument, 0, 1 },
	{ "infile", required_argument, 0, 2 },
	{ "range", required_argument, 0, 3 },
	{ "divider", required_argument, 0, 4 },
	{ 0, 0, 0, 0 },
};

struct pwm_params {
	uint32_t divisor,
	uint32_t range,
	int mode,
} pwm_params;

void do_tests(FILE *chardev)
{

}


void init_driver(FILE *chardev)
{

}

#define READ_BUFFER_SIZE 0x08000000 // 128M

int main (int argc, char **argv)
{
	FILE *infile;
	FILE *dev_node;
	char filename[] = "out.dat";
	char devnode[] = "/dev/pwm_dma";
	uint32_t *buffer = NULL;
	uint32_t *bufptr = NULL;

	dev_node = fopen(devnode, "w");
	if (!dev_node) {
		printf("Can't open /dev/pwm_dma. Check that it exists.\n");
		return -1;
	}

	do_tests(devnode);

	infile = fopen(filename, "r");
	if (!infile) {
		printf("Can't open input file.\n");
		fclose(dev_node);
		return -1;
	}

	buffer = (uint32_t *) malloc(READ_BUFFER_SIZE);
	if (!buffer) {
		printf("Can't allocate memory\n");
		return -1;
	}
	bufptr = buffer;

	fread(buffer, sizeof(uint32_t), READ_BUFFER_SIZE/4, infile);

	fwrite(buffer, sizeof(uint32_t), READ_BUFFER_SIZE/4, devnode);

	return 0;
}



