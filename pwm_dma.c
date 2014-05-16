/*
 * pwm_dma.c
 *
 *  Created on: 6 May 2014
 *      Author: jonathan
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fcntl.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/io.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <mach/platform.h>
#include <mach/dma.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

//static char module_name[] = "pwm_dma";

static int pwm_range = 64;
static int scb_len = 4096;
static int nr_scbs = 4;
module_param(pwm_range, int, S_IRUGO);
module_param(scb_len, int, S_IRUGO);
module_param(nr_scbs, int, S_IRUGO);

struct pwm_dma_state {
	/* Single-user */
	bool open;
	wait_queue_head_t writeq;
	wait_queue_head_t readq;
	void __iomem *dma_chan_base;
	void __iomem *pwm_base;
	int dma_chan_irq;
	int dma_chan;
	int dma_flags; /* state flags */
#define DMA_RUNNING (1<<0)
#define DMA_STOPPING (1<<1)
#define DMA_STOPPED (1<<2)
	void *circ_buffer;
	void *circ_buffer_end;
	dma_addr_t circ_dma_base;
	void *circ_readptr;
	void *circ_writeptr;
	struct bcm2708_dma_cb *scbs;
	dma_addr_t scb_handle;
	struct cdev cdev;
	//void *userspace_fifo;
};

#define CTL 0x0
#define STA 0x4
#define PWM_DMAC 0x8
#define RNG1 0x10
#define DAT1 0x14
#define FIFO1 0x18

struct pwm_dma_state *state;

/**
  * Ringbuffer.
  * Simple implementation: four SCBs, each describing a 4k transfer length.
  * Writeptr is never less than 4 bytes behind the readptr, which is going to be updated
  * on a block-by-block basis. This gives four "threshold" points.
  * Userspace blocks on waiting for space to become available, then you end up with the DMA
  * IRQ updating the readptr and thus the sleeping write wakes up.
  */

/* IOCTL: enable/disable ringbuffer streaming */

static dev_t devid = MKDEV(1337, 0);

static int space_available(void)
{
	/* Circular buffer. */
	return (state->circ_writeptr != state->circ_readptr);
}

static long pwm_dma_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
		
	}
	return 0;
}

static int pwm_dma_release(struct inode *inode, struct file *file)
{
	state->open = false;
	return 0;
}

static int pwm_dma_open(struct inode *inode, struct file *file)
{
	return 0;
}

/* Attempt to write into the circular buffer. Write until we end up running out of space.
 * If we do run out of space and there is still stuff to be written, then go to sleep and
 * wait for the IRQ to punt the readptr along. We will write at most 4k per iteration. */
ssize_t pwm_dma_write(struct file *filp, const char __user *buf, size_t count, loff_t *pos) 
{
	if (state->open)
		return -EAGAIN;
	else
		state->open = true;
		
	while (!space_available()) {
		//if (filp->f_flags & O_NONBLOCK)
		//	return -EAGAIN;
		if (wait_event_interruptible(state->writeq, space_available())) {
			pr_info("bugged\n");
			return -ERESTARTSYS;
		}
	}
	if (state->circ_readptr > state->circ_writeptr)
		count = min(count, (size_t)(state->circ_readptr - state->circ_writeptr));
	else /* Wrapped */
		count = min(count, (size_t)(state->circ_buffer_end - state->circ_writeptr));
		
	if(copy_from_user(state->circ_writeptr, buf, count))
		return -EFAULT;

	state->circ_writeptr += count;
	if (state->circ_writeptr >= state->circ_buffer_end) {
		state->circ_writeptr = state->circ_buffer;
	}
	pr_info("Write count=%d readptr=%08x writeptr=%08x\n", count, state->circ_readptr, state->circ_writeptr);
	state->open = false;
	return count;
}


static void prep_scbs(struct pwm_dma_state *state) 
{
	int i = 0;
	u32 info = BCM2708_DMA_INT_EN | BCM2708_DMA_S_INC |
			BCM2708_DMA_D_DREQ | BCM2708_DMA_PER_MAP(5);
	state->circ_readptr = state->circ_buffer;
	state->circ_writeptr = state->circ_buffer;
	for (i = 0; i < nr_scbs; i++) {
		state->scbs[i].info = info;
		state->scbs[i].src = state->circ_dma_base + (i * scb_len);
		state->scbs[i].dst = 0x7E20C018;
		state->scbs[i].length = scb_len;
		state->scbs[i].stride = 0;
		state->scbs[i].next = state->scb_handle + ((i+1) % nr_scbs)*sizeof(struct bcm2708_dma_cb);
		pr_info("SCB %d: next = 0x%08x src = 0x%08x\n", i, state->scbs[i].next, state->scbs[i].src);
	}
}

static irqreturn_t pwm_dma_irq(int irq, void *dev_id)
{
	/* Are we safe? */
	switch (state->dma_flags) {
	case DMA_RUNNING:
		state->circ_readptr += scb_len;
		if (state->circ_readptr >= state->circ_buffer_end)
			state->circ_readptr = state->circ_buffer;
		break;
	case DMA_STOPPING:
		/* Mask IRQ, pause DMA */
		writel(readl(state->dma_chan_base + BCM2708_DMA_CS) & ~((1 << 0)| (1<<1)), state->dma_chan_base + BCM2708_DMA_CS);
		disable_irq(state->dma_chan_irq);
		state->dma_flags = DMA_STOPPED;
		break;
	case DMA_STOPPED:
	default:
		BUG();
		break;
	}
	/* read the DMA IRQ reason */
	wake_up(&state->writeq);
	/* Ack DMA IRQ */
	writel(readl(state->dma_chan_base+BCM2708_DMA_CS) | (1 << 2), state->dma_chan_base + BCM2708_DMA_CS);
	return IRQ_HANDLED;
}

struct file_operations pwm_dma_fops = {
	.owner = THIS_MODULE,
	.llseek = NULL,
	.read = NULL,
	.write = pwm_dma_write,
	.unlocked_ioctl = pwm_dma_ioctl,
	.open = pwm_dma_open,
	.release = pwm_dma_release,
};

static int __init pwm_dma_init(void)
{
	u32 reg = 0;
	int i = 0;
	state = kmalloc(sizeof(struct pwm_dma_state), GFP_KERNEL);
	if (!state) {
		pr_err("Can't allocate state\n");
		goto fail;
	}
	state->scbs = dma_alloc_coherent(NULL, sizeof(struct bcm2708_dma_cb) * nr_scbs, &state->scb_handle, GFP_KERNEL);
	if (!state->scbs) {
		pr_err("can't allocate SCBs\n");
		kfree(state);
		goto fail;
	}
	/* request a DMA channel */
	state->dma_chan = bcm_dma_chan_alloc(-1, &state->dma_chan_base, &state->dma_chan_irq);
	if (state->dma_chan < 0) {
		pr_err("Can't allocate DMA channel\n");
		kfree(state->scbs);
		kfree(state);
		goto fail;
	} else {
		pr_info("Got channel %d\n", state->dma_chan);
	}
	state->circ_buffer = dma_alloc_coherent(NULL, (size_t)scb_len * nr_scbs, &state->circ_dma_base, GFP_KERNEL);
	if (!state->circ_buffer) {
		pr_err("can't allocate DMA mem\n");
		kfree(state->scbs);
		kfree(state);
		goto fail;
	}
	state->circ_buffer_end = state->circ_buffer + scb_len * nr_scbs;
	init_waitqueue_head(&state->writeq);
	state->dma_flags = DMA_RUNNING;
	if(request_irq(state->dma_chan_irq, &pwm_dma_irq, 0, "PWM DMA IRQ", NULL)) {
		pr_err("Can't request IRQ %d\n", state->dma_chan_irq);
	}
	/* Setup PWMDIV */
	state->pwm_base = ioremap(PWM_BASE, SZ_4K);
	/* setup PWM block */
	
	reg = 64;
	writel(reg, state->pwm_base + RNG1);
	reg = 32;
	writel(reg, state->pwm_base + DAT1);
	/* Channel 1 only, range=64, M/S mode, DMA enable */
	reg = (1 << 0) | /* CH1EN */
		//	(1 << 1)
			(1 << 2) | 
			//(1 << 3) |
			(1 << 5) |
			(1 << 6) | /* M/S */
			(1 << 7);
	writel(reg, state->pwm_base + CTL);
	reg =	(1 << 31) |
			(4 << 8) |
			(8 << 0);
	writel(reg, state->pwm_base + PWM_DMAC);
	
	/* Setup SCBs */
	prep_scbs(state);
	
	/* 50% duty */
	reg = pwm_range >> 1;
	for (i = 0; i < (scb_len * nr_scbs); i+=4) {
		memcpy(state->circ_buffer + i, &reg, 4);
	}
	pr_info("set initial values\n");
	/* Hack - disable dma channel 3 because VC has this running */
	writel(0, ext_dmaman->dma_base + (3 << 8));
	pr_warn("hack: disabling channel 0x%08x\n", ext_dmaman->dma_base + (3 << 8));
	writel((1<<31), state->dma_chan_base + BCM2708_DMA_ADDR);
	writel(state->scb_handle, state->dma_chan_base + BCM2708_DMA_ADDR);
	/* Setup DMA engine */
	reg = (1 << 8) | 
		(1 << 2) | 
		(1 << 0);
	writel(reg, state->dma_chan_base + BCM2708_DMA_CS);
	/* Set DMA to first CB */
	udelay(3);
	reg = readl(state->dma_chan_base + BCM2708_DMA_CS);
	pr_info("DMA state: 0x%08x\n", reg);
	if(register_chrdev_region(devid, 1, "pwm_dma")) {
		pr_err("derp no chardev\n");
	}
	state->cdev.owner = THIS_MODULE;
	cdev_init(&state->cdev, &pwm_dma_fops);
	
	if(cdev_add(&state->cdev, devid, 1)) {
		pr_err("CDEV failed\n");
	}
	// enable_irq() on ioctl
	return 0;
fail:
	return -1;
}


static void __exit pwm_dma_exit(void)
{
	/* Disable DMA */
	cdev_del(&state->cdev);
	state->dma_flags = DMA_STOPPING;
	disable_irq(state->dma_chan_irq);
	free_irq(state->dma_chan_irq, NULL);
	unregister_chrdev(devid, "pwm_dma");
	bcm_dma_abort(state->dma_chan_base);
	pr_err("arf\n");
	dma_free_coherent(NULL, (size_t)scb_len * nr_scbs, state->circ_buffer, state->circ_dma_base);
	bcm_dma_chan_free(state->dma_chan);
	pr_err("arf2\n");
	dma_free_coherent(NULL, sizeof(struct bcm2708_dma_cb) * nr_scbs, state->scbs, state->scb_handle);
}

module_init(pwm_dma_init);
module_exit(pwm_dma_exit);