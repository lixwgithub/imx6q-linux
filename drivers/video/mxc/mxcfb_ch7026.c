#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/i2c.h>
#include <linux/mxcfb.h>
#include <linux/ipu.h>
#include <linux/fsl_devices.h>
#include <mach/hardware.h>
static struct fb_videomode vga_modedb[] = {
	{
	"VGA0",60, 640,480, 39683, 48, 16, 33, 10, 96, 2,
	FB_SYNC_CLK_LAT_FALL,
	FB_VMODE_NONINTERLACED,
	0,},
	{
	"VGA1",60, 800,600, 25000, 88, 40, 23, 1, 128, 4,
	FB_SYNC_CLK_LAT_FALL,
	FB_VMODE_NONINTERLACED,
	0,},
	{
	"VGA2",60, 1024,768, 15386, 160, 24,29,3,136, 6,
	FB_SYNC_CLK_LAT_FALL,
	FB_VMODE_NONINTERLACED,
	0,},
	{
	"VGA3",60, 1280,1024, 9262, 248, 48, 38, 1, 112, 3,
	FB_SYNC_CLK_LAT_FALL,
	FB_VMODE_NONINTERLACED,
	0,},
	{
	"VGA4",60, 1280,960, 9793, 216, 80, 30, 1, 136, 3,
	FB_SYNC_CLK_LAT_FALL,
	FB_VMODE_NONINTERLACED,
	0,},
};

int width,height,HO,HW,HP,VO,VW,VP,totalwidth,totalheight;
static int set_essential_param(void)
{
	struct fb_info* fb;
	int i;
	struct fb_videomode* current_mode;
	for (i = 0; i < num_registered_fb; i++)
			{
				if (strcmp(registered_fb[i]->fix.id, "DISP3 BG") == 0)
					break;
			}
	if(i<num_registered_fb)
	{
		printk(KERN_ALERT"find ipu 0 di 0 is active\n");
		printk(KERN_ALERT"but I can't make true this di is using by lcdif\n");
		//lcd_init_fb(registered_fb[i]);
		//this opearation may work badly in android.
	}
	else
	{
		printk(KERN_ALERT"find ipu 0 di 0 is not active\n");
		printk(KERN_ALERT"so there must be no signal input for ch7026\n");
		printk(KERN_ALERT"please try add a new ipuv3fb platform device to use this di\n");
		return -1;
	}
	fb=registered_fb[i];
	for(i=0;i<sizeof(vga_modedb)/sizeof(struct fb_videomode);i++)
		if(strcmp(fb->mode->name,vga_modedb[i].name)==0)
			break;
	if(i>=sizeof(vga_modedb)/sizeof(struct fb_videomode))
	{
		printk(KERN_ALERT"please set a vga mode supported by ch7026 to fb%d\n",fb->node);
		printk(KERN_ALERT"copy the modedb of ch7026 module to lcdif's modedb and set modestr for lcd_fb's platform data");
		return -1;
	}
	current_mode=&vga_modedb[i];
	width=current_mode->xres;
	height=current_mode->yres;
	HP=current_mode->left_margin;
	HO=current_mode->right_margin;
	HW=current_mode->hsync_len;
	VP=current_mode->upper_margin;
	VO=current_mode->lower_margin;
	VW=current_mode->vsync_len;
	totalwidth=width+HP+HO+HW;
	totalheight=height+VP+VO+VW;
	return 0;
}

//change mode to be accordent with the configuration of ch7026.
//but there exists problem,and bits_per_pixel also be changed.


#define REGMAP_LENGTH (sizeof(reg_init) / (2*sizeof(u8)))
static int ch7026_device_init(struct i2c_client* ch7026_client)
{
	int i;
	int dat;
	u8 reg_init[][2] = {
	{ 0x02, 0x01 },
	{ 0x02, 0x03 },
	{ 0x03, 0x00 },
	{ 0x06, 0x73 },//0x6b
			  {0x07,0x1C},
	{ 0x08, 0x08 },
	{ 0x09, 0x80 },
	{ 0x0C, 0x00 },
	{ 0x0D, 0x89 },


	//core params input
	{ 0x0F, ((totalwidth&0x700)>>5)|((width&0x700)>>8)},
	{ 0x10, width&0xff },
	{ 0x11, totalwidth&0xff },
	{ 0x12, 0x40 },//else 0
	{ 0x13, HO },
	{ 0x14, HW },

	{ 0x15, 0x40|((totalheight&0x700)>>5)|((height&0x700)>>8)},//else subs 0x40
	{ 0x16, height&0xff},
	{ 0x17, totalheight&0xff  },
	{ 0x18, 0x00 },
	{ 0x19, VO },
	{ 0x1A, VW },

	//core params output
	{ 0x1B, ((totalwidth&0x700)>>5)|((width&0x700)>>8)},
	{ 0x1C, width&0xff },
	{ 0x1D, totalwidth&0xff },
	{ 0x1E, 0x00 },
	{ 0x1F, HO },
	{ 0x20, HW },

	{ 0x21, ((totalheight&0x700)>>5)|((height&0x700)>>8)},
	{ 0x22, height&0xff },
	{ 0x23, totalheight&0xff },
	{ 0x24, 0x00 },
	{ 0x25, VO },
	{ 0x26, VW },


	//from reg 27 to 36,there ten regs define image zoom and other image processing.
	//....
	//end image zoom and other image processing.

	{ 0x37, 0x20 },
	{ 0x39, 0x20 },
	{ 0x3B, 0x20 },
	{ 0x41, 0xA2 },
	{ 0x4D, 0x03 },
	{ 0x4E, 0x13 },
	{ 0x4F, 0xB1 },
	{ 0x50, 0x3B },
	{ 0x51, 0x54 },
	{ 0x52, 0x12 },
	{ 0x53, 0x13 },
	{ 0x55, 0xE5 },
	{ 0x5E, 0x80 },
	{ 0x69, 0x64 },
	{ 0x7D, 0x62 },
	{ 0x04, 0x00 },
	{ 0x06, 0x69 },

	/*
	NOTE: The following five repeated sentences are used here to wait memory initial complete, please don't remove...(you could refer to Appendix A of programming guide document (CH7025(26)B Programming Guide Rev2.03.pdf) for detailed information about memory initialization!
	*/
	{ 0x03, 0x00 },
	{ 0x03, 0x00 },
	{ 0x03, 0x00 },
	{ 0x03, 0x00 },
	{ 0x03, 0x00 },

	{ 0x06, 0x68 },
	{ 0x02, 0x02 },
	{ 0x02, 0x03 },
	};

	//here means vga can startup.
	msleep(100);
	dat = i2c_smbus_read_byte_data(ch7026_client, 0x00);
	printk(KERN_ALERT"read id = 0x%x and its addr is 0x%x\n",dat,ch7026_client->addr);

	if (dat != 0x54)
	{
		printk(KERN_ALERT"read id is not equal 0x54,so probe failed");
		return -ENODEV;
	}
	for (i = 0; i < REGMAP_LENGTH; ++i)
	{
		if (i2c_smbus_write_byte_data(ch7026_client, reg_init[i][0], reg_init[i][1]) < 0)
			{
			   printk(KERN_ALERT"error happended when set the value of %dth unit of ch7026 chip\n",i);
			   return -EIO;
			}
		//printk(KERN_ALERT"%x %x",reg_init[i][0], reg_init[i][1]);
	}
	printk(KERN_ALERT"ch7026_device_init end...\n");
	return 0;
}
//------------------------------------------------------------------------------------------
//following is routine

static int __devinit ch7026_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	if(set_essential_param()==0)
	    return ch7026_device_init(client);
	else
		return 0;
}
static int __devexit ch7026_remove(struct i2c_client *client)
{
	return 0;
}
static int ch7026_suspend(struct i2c_client *client, pm_message_t message)
{
	return 0;
}
static int ch7026_resume(struct i2c_client *client)
{
	return 0;
}
static const struct i2c_device_id ch7026_id[] = {
	{"ch7026", 0},
};

static struct i2c_driver ch7026_driver = {
	.driver = {
		   .name = "ch7026_driver",
		   },
	.probe = ch7026_probe,
	.remove = ch7026_remove,
	.suspend = ch7026_suspend,
	.resume = ch7026_resume,
	.id_table = ch7026_id,
};

static int __init ch7026_init(void)
{
	printk(KERN_ALERT"mxcfb_ch7026 driver load----------------\n");
	return i2c_add_driver(&ch7026_driver);
}

static void __exit ch7026_exit(void)
{
	i2c_del_driver(&ch7026_driver);
	printk(KERN_ALERT"mxcfb_ch7026 driver exit----------------\n");
}

module_init(ch7026_init);
module_exit(ch7026_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhanghang");
 /*
*following is some usage of kernel funtion of i2c moudule.
static struct i2c_board_info info[] __initdata =
{
	{I2C_BOARD_INFO("ch7026", 0x00)},
};
static void add_all_i2c_device(int busnum)
{
	struct i2c_adapter *ad;
	int i;
	//add some device.
	 ad=i2c_get_adapter(0);
	 for(i=0;i<0x7f;i++)
	 {
	   info[0].addr=i;
	   i2c_new_device(ad,info);
	 }
	 i2c_put_adapter(ad);
}
*/
/*
 * struct mxcfb_pfmt {
	u32 fb_pix_fmt;
	int bpp;
	struct fb_bitfield red;
	struct fb_bitfield green;
	struct fb_bitfield blue;
	struct fb_bitfield transp;
};

static const struct mxcfb_pfmt mxcfb_pfmts[] = {
	//     pixel         bpp    red         green        blue      transp
	{IPU_PIX_FMT_RGB565, 16, {11, 5, 0}, { 5, 6, 0}, { 0, 5, 0}, { 0, 0, 0} },
	{IPU_PIX_FMT_RGB24,  24, { 0, 8, 0}, { 8, 8, 0}, {16, 8, 0}, { 0, 0, 0} },
	{IPU_PIX_FMT_BGR24,  24, {16, 8, 0}, { 8, 8, 0}, { 0, 8, 0}, { 0, 0, 0} },
	{IPU_PIX_FMT_RGB32,  32, { 0, 8, 0}, { 8, 8, 0}, {16, 8, 0}, {24, 8, 0} },
	{IPU_PIX_FMT_BGR32,  32, {16, 8, 0}, { 8, 8, 0}, { 0, 8, 0}, {24, 8, 0} },
	{IPU_PIX_FMT_ABGR32, 32, {24, 8, 0}, {16, 8, 0}, { 8, 8, 0}, { 0, 8, 0} },
};
 *
static struct fb_videomode mode_vga= {
	NULL,  60, width,height, 15386, HP, HO, VP, VO, HW, VW,
	FB_SYNC_CLK_LAT_FALL,
	FB_VMODE_NONINTERLACED,
	0,
};
static void lcd_init_fb(struct fb_info *info)
{
	struct fb_var_screeninfo var;
	//struct fb_bitfield fi;
	memset(&var, 0, sizeof(var));

	fb_videomode_to_var(&var, &mode_vga);


	fi=info->var.red;
	printk(KERN_ALERT"red field is %d %d %d",fi.offset,fi.length,fi.msb_right);
	fi=info->var.green;
	printk(KERN_ALERT"green field is %d %d %d",fi.offset,fi.length,fi.msb_right);
	fi=info->var.blue;
	printk(KERN_ALERT"blue field is %d %d %d",fi.offset,fi.length,fi.msb_right);
	fi=info->var.transp;
	printk(KERN_ALERT"transp field is %d %d %d",fi.offset,fi.length,fi.msb_right);

	//current mode is rgb32.
	var.bits_per_pixel=32;
	var.red=mxcfb_pfmts[3].red;
	var.green=mxcfb_pfmts[3].green;
	var.blue=mxcfb_pfmts[3].blue;
	var.transp=mxcfb_pfmts[3].transp;
	console_lock();
	info->flags |= FBINFO_MISC_USEREVENT;
	fb_set_var(info, &var);
	fb_blank(info, FB_BLANK_UNBLANK);
	info->flags &= ~FBINFO_MISC_USEREVENT;
	console_unlock();
}
struct fb_videomode {
	const char *name;
	u32 refresh;
	u32 xres;
	u32 yres;
	u32 pixclock;
	u32 left_margin;
	u32 right_margin;
	u32 upper_margin;
	u32 lower_margin;
	u32 hsync_len;
	u32 vsync_len;
	u32 sync;
	u32 vmode;
	u32 flag;
};
*/
