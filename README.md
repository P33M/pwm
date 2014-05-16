pwm
===

Proof-of-concept program (hack) for testing sigma-delta modulated PWM audio output on Raspberry Pi.

pwm_lut
===
Generates a linearisation lookup table for a CMOS "DAC" - like the one found on the Pi's audio output.

The voltage output by a CMOS totem-pole output stage is not a fixed quantity. It is a function of the silicon parameters of each MOSFET (p-MOS and n-MOS), the remenant output voltage on the filter capacitor, the output resistor and the driving voltage.

This program computes an X * Y matrix that takes a quantized version of the output duty cycle and remenant voltage, then provides a corrected value to a resolution of 16 bits.


pwm_dma
===
Super hacky kernel module that overrides Videocore's control of the PWM peripheral. Note that VC4 must have first used the peripheral at least once: this is to set up PLL_D and PWMDIV with the correct frequency. The clock supplied to the block is 2048 * 44100Hz.

After insmod, run `mknod -c /dev/pwm_dma 1337 0` to make the device node.

Writes will be interpreted as LSB-justified 32-bit words, as per the PWM datasheet section on the input fifo. The ring buffer is 64k in size which gives a playback length of 11ms.

Reads are invalid.

TODO: fix IOCTLS for messing about with clock frequency and streaming.
