/* linux/drivers/video/samsung/s3cfb_dummymipilcd.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Modified by Samsung Electronics (UK) on May 2010
 *
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/backlight.h>
#include <linux/lcd.h>

#include <plat/gpio-cfg.h>
#include <plat/regs-dsim.h>

#include <mach/dsim.h>
#include <mach/mipi_ddi.h>

#include <linux/regulator/driver.h>

#include "s5p-dsim.h"
#include "s3cfb.h"

static struct mipi_ddi_platform_data *ddi_pd;

void ips_bl_on(int on)
{
	int err;
	
	err = gpio_request(EXYNOS4_GPD0(1), "PWMEN");
	if (err) {
		printk(KERN_ERR "failed to request PWMEN for "
			"I2C control\n");
		//return err;
	}
#if !defined(CONFIG_BACKLIGHT_PWM)
	gpio_direction_output(EXYNOS4_GPD0(1), 1);
	gpio_free(EXYNOS4_GPD0(1));

	printk("SCP(%s, %d): LCD_PWM_ON\n", __FUNCTION__, __LINE__);
#else
	gpio_direction_output(EXYNOS4_GPD0(1), 0);

	s3c_gpio_cfgpin(EXYNOS4_GPD0(1), (0x2 << 4));

	gpio_free(EXYNOS4_GPD0(1));
	printk("SCP(%s, %d): LCD_PWM_ON\n", __FUNCTION__, __LINE__);
#endif
}

void s6d6aa0_write_0(unsigned char dcs_cmd)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, dcs_cmd, 0);
}

static void s6d6aa0_write_1(unsigned char dcs_cmd, unsigned char param1)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, dcs_cmd, param1);
}

static void s6d6aa0_write_3(unsigned char dcs_cmd, unsigned char param1, unsigned char param2, unsigned char param3)
{
       unsigned char buf[4];
	buf[0] = dcs_cmd;
	buf[1] = param1;
	buf[2] = param2;
	buf[3] = param3;

	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, 4);
}

static void s6d6aa0_write(void)
{
	unsigned char buf[15] = {0xf8, 0x01, 0x27, 0x27, 0x07, 0x07, 0x54,
							0x9f, 0x63, 0x86, 0x1a,
							0x33, 0x0d, 0x00, 0x00};
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, sizeof(buf));
}

static void s6d6aa0_display_off(struct device *dev)
{
	printk("*************** %s(%d) ***************\n", __FUNCTION__, __LINE__);
	
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0x28, 0x00);
}

void s6d6aa0_sleep_in(struct device *dev)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x10, 0);
}

void s6d6aa0_sleep_out(struct device *dev)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x11, 0);
}

static void s6d6aa0_display_on(struct device *dev)
{
	printk("*************** %s(%d) ***************\n", __FUNCTION__, __LINE__);
	
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x29, 0);
}

void lcd_pannel_on(void)
{
#if 0
	/* password */
	s6d6aa0_write_1(0xb0, 0x09);
	s6d6aa0_write_1(0xb0, 0x09);
	s6d6aa0_write_1(0xd5, 0x64);
	s6d6aa0_write_1(0xb0, 0x09);

	s6d6aa0_write_1(0xd5, 0x84);
	s6d6aa0_write_3(0xf2, 0x02, 0x03, 0x1b);
	s6d6aa0_write();
	s6d6aa0_write_3(0xf6, 0x00, 0x8c, 0x07);

	/* reset */
	s6d6aa0_write_1(0xfa, 0x01);
#endif
	unsigned int data_array[16];

	data_array[0]=0x00000011;
	data_array[1]=0x00005050;

	/* Exit sleep */
	//mdelay(200);
	//s6d6aa0_write_0(0x11);
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int)data_array, 2);
	mdelay(100);

	/* Set Display ON */
	//s6d6aa0_write_0(0x29);
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x29, 0);
	mdelay(100);
}

void lcd_panel_init(void)
{
	lcd_pannel_on();

	//mdelay(500);
	ips_bl_on(1);
}

static int dummy_panel_init(void)
{
	printk("*************** %s(%d) ***************\n", __FUNCTION__, __LINE__);
	//mdelay(100);
	lcd_panel_init();

	return 0;
}

static int s6d6aa0_set_link(void *pd, unsigned int dsim_base,
	unsigned char (*cmd_write) (unsigned int dsim_base, unsigned int data0,
	    unsigned int data1, unsigned int data2),
	unsigned char (*cmd_read) (unsigned int dsim_base, unsigned int data0,
	    unsigned int data1, unsigned int data2))
{
	struct mipi_ddi_platform_data *temp_pd = NULL;

	temp_pd = (struct mipi_ddi_platform_data *) pd;
	if (temp_pd == NULL) {
		printk(KERN_ERR "mipi_ddi_platform_data is null.\n");
		return -1;
	}
	printk("*************** %s(%d) ***************\n", __FUNCTION__, __LINE__);
	ddi_pd = temp_pd;

	ddi_pd->dsim_base = dsim_base;

	if (cmd_write)
		ddi_pd->cmd_write = cmd_write;
	else
		printk(KERN_WARNING "cmd_write function is null.\n");

	if (cmd_read)
		ddi_pd->cmd_read = cmd_read;
	else
		printk(KERN_WARNING "cmd_read function is null.\n");

	return 0;
}

#define EXYNOS4_GPD_0_0_TOUT_0  (0x2)
#define EXYNOS4_GPD_0_1_TOUT_1  (0x2 << 4)
#define EXYNOS4_GPD_0_2_TOUT_2  (0x2 << 8)
#define EXYNOS4_GPD_0_3_TOUT_3  (0x2 << 12)

static int s6d6aa0_probe(struct device *dev)
{
	//int err;
	
	printk("*************** %s(%d) ***************\n", __FUNCTION__, __LINE__);

	mdelay(5);

	return 0;
}

#ifdef CONFIG_PM
static int s6d6aa0_suspend(void)
{
	printk("*************** %s(%d) ***************\n", __FUNCTION__, __LINE__);
	s6d6aa0_write_0(0x28);
	mdelay(20);
	s6d6aa0_write_0(0x10);
	mdelay(20);

	return 0;
}

static int s6d6aa0_resume(struct device *dev)
{
	printk("*************** %s(%d) ***************\n", __FUNCTION__, __LINE__);
	return 0;
}
#else
#define s6d6aa0_suspend	NULL
#define s6d6aa0_resume	NULL
#endif

static struct mipi_lcd_driver s6d6aa0_mipi_driver = {
	.name = "dummy_mipi_lcd",
	.init = dummy_panel_init,
	.display_on = s6d6aa0_display_on,
	.set_link = s6d6aa0_set_link,
	.probe = s6d6aa0_probe,
	.suspend = s6d6aa0_suspend,
	.resume = s6d6aa0_resume,
	.display_off = s6d6aa0_display_off,
};

static int s6d6aa0_init(void)
{
	s5p_dsim_register_lcd_driver(&s6d6aa0_mipi_driver);
	return 0;
}

static void s6d6aa0_exit(void)
{
	return;
}

static struct s3cfb_lcd dummy_mipi_lcd = {
	.width	= 768,//480,
	.height	= 1024,//800,
	.bpp	= 24,
	.freq	= 60,

	.timing = {
		.h_fp = 60,
		.h_bp = 56,
		.h_sw = 64,
		.v_fp = 36,
		//.v_fpe = 2,
		.v_bp = 30,
		//.v_bpe = 1,
		.v_sw = 50,
		.cmd_allow_len = 4,
	},

	.polarity = {
		.rise_vclk = 0,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};

/* name should be fixed as 's3cfb_set_lcd_info' */
void s3cfb_set_lcd_info(struct s3cfb_global *ctrl)
{
	dummy_mipi_lcd.init_ldi = NULL;
	ctrl->lcd = &dummy_mipi_lcd;
}

module_init(s6d6aa0_init);
module_exit(s6d6aa0_exit);

MODULE_LICENSE("GPL");
