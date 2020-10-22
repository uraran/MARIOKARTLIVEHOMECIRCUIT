#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include "v4l2_hdmi_dev.h"
#include "phoenix_mipi_i2c_ctl.h"
#include "phoenix_hdmiInternal.h"

#define RLE0551C_patterngen_addr 0x7A // tuner:0x1A
#define RTS5845_addr             0x3c
#define OV5645_addr				0x3c
#define OV5647_addr				0x36
#define OV9782_addr				0x60

#define READ_RETRY_TIMES 6
#define WRITE_RETRY_TIMES 10
#define RETRY_INTERVAL_MS 2

int mipi_i2c_read(struct i2c_adapter *p_adap, int len, unsigned char *buf)
{
    unsigned int pattern_addr;
    struct i2c_msg msgs[2];
	unsigned int i;

	switch (hdmi.mipi_camera_type) {
	case MIPI_CAMERA_RLE0551C:
		pattern_addr = RLE0551C_patterngen_addr;
		break;
	case MIPI_CAMERA_RTS5845:
		pattern_addr = RTS5845_addr;
		break;
	case MIPI_CAMERA_OV5645:
		pattern_addr = OV5645_addr;
		break;
	case MIPI_CAMERA_OV5647:
		pattern_addr = OV5647_addr;
		break;
	case MIPI_CAMERA_OV9782:
		pattern_addr = OV9782_addr;
		break;
	default:
		pattern_addr = RTS5845_addr;
		break;
	};

	msgs[0].addr = pattern_addr;
	msgs[0].flags = 0;
	msgs[0].len = 2;
	msgs[0].buf = buf;
	msgs[1].addr = pattern_addr;
	msgs[1].flags = I2C_M_RD | I2C_M_NOSTART;
	msgs[1].len = len;
	msgs[1].buf = buf;

	for (i = 0; i < READ_RETRY_TIMES; i++) {
		if (i2c_transfer(p_adap, msgs, 2) == 2)
			return 0;

		msleep(RETRY_INTERVAL_MS);
	}

	pr_err("[MIPI] I2C read fail, RETRY_TIMES=%u\n", i);
    return -1;
}

struct i2c_adapter * mipi_i2c_init(void)
{
    struct i2c_adapter *p_adap;
    unsigned char bus_id = 4;// tuner:5

    //pr_info("%s\n", __func__);

	p_adap = i2c_get_adapter(bus_id);

    if (p_adap == NULL)
		pr_err("[MIPI] Get i2c adapter %d failed\n", bus_id);

    return p_adap;
}

int mipi_i2c_write(struct i2c_adapter *p_adap, int len, unsigned char *buf)
{
    unsigned int pattern_addr;
    struct i2c_msg msgs;
	unsigned int i;

	switch (hdmi.mipi_camera_type) {
	case MIPI_CAMERA_RLE0551C:
		pattern_addr = RLE0551C_patterngen_addr;
		break;
	case MIPI_CAMERA_RTS5845:
		pattern_addr = RTS5845_addr;
		break;
	case MIPI_CAMERA_OV5645:
		pattern_addr = OV5645_addr;
		break;
	case MIPI_CAMERA_OV5647:
		pattern_addr = OV5647_addr;
		break;
	case MIPI_CAMERA_OV9782:
		pattern_addr = OV9782_addr;
		break;
	default:
		pattern_addr = RTS5845_addr;
		break;
	};

#if 0
    int i;
    for(i=0;i<len+1;i++)
      pr_info("buf[%d]=0x%x \n", i, buf[i]);
#endif

    msgs.addr = pattern_addr;
    msgs.flags = 0;
    msgs.len = len;
    msgs.buf = buf;

	for (i = 0; i < WRITE_RETRY_TIMES; i++) {
		if (i2c_transfer(p_adap, &msgs, 1) == 1)
			return 0;

		msleep(RETRY_INTERVAL_MS);
	}

	pr_err("[MIPI] I2C write fail, RETRY_TIMES=%u\n", i);
    return -1;
}

static DEFINE_MUTEX(mipi_sysfs_lock);
static ssize_t mipi_write_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret_count=0;

	mutex_lock(&mipi_sysfs_lock);
	ret_count = sprintf(buf, "%s\n", "echo \"AddrByte1 AddrByte2 DataByte\" > mipi_write");
	mutex_unlock(&mipi_sysfs_lock);

	return ret_count;
}

static ssize_t mipi_write_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	char *cur;
	char *token1;
	char *token2;
	char *token3;
	unsigned int val1;
	unsigned int val2;
	unsigned int val3;
	struct i2c_adapter *p_adap;
	unsigned char data[3];

	mutex_lock(&mipi_sysfs_lock);

	cur = (char *)buf;

	token1 = strsep(&cur, " ");

	if (token1 != NULL)
		token2 = strsep(&cur, " ");
	else
		goto ret_error;

	if (token2 != NULL)
		token3 = strsep(&cur, " ");
	else
		goto ret_error;

	ret = kstrtouint(token1, 0, &val1);
	if (ret < 0)
		goto ret_error;

	ret = kstrtouint(token2, 0, &val2);
	if (ret < 0)
		goto ret_error;

	ret = kstrtouint(token3, 0, &val3);
	if (ret < 0)
		goto ret_error;

	data[0] = (unsigned char) val1;
	data[1] = (unsigned char) val2;
	data[2] = (unsigned char) val3;
	/* pr_err("Set Reg 0x%02x%02x = 0x%02x\n", data[0], data[1], data[2]); */

	p_adap = mipi_i2c_init();
	mipi_i2c_write(p_adap, 3, data);

	mutex_unlock(&mipi_sysfs_lock);
	return size;
ret_error:
	pr_err("Wrong argumet");
	return -ENOMEM;
}

static ssize_t mipi_read_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret_count=0;

	mutex_lock(&mipi_sysfs_lock);
	ret_count = sprintf(buf, "%s\n", "echo \"AddrByte1 AddrByte2\" > mipi_read");
	mutex_unlock(&mipi_sysfs_lock);

	return ret_count;
}

static ssize_t mipi_read_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = -ENOMEM;
	char *cur;
	char *token1;
	char *token2;
	unsigned int val1;
	unsigned int val2;
	struct i2c_adapter *p_adap;
	unsigned char data[2];

	mutex_lock(&mipi_sysfs_lock);

	cur = (char *)buf;

	token1 = strsep(&cur, " ");

	if (token1 != NULL)
		token2 = strsep(&cur, " ");
	else
		goto err;

	if (token2 == NULL)
		goto err;

	ret = kstrtouint(token1, 0, &val1);

	ret = kstrtouint(token2, 0, &val2);

	data[0] = (unsigned char) val1;
	data[1] = (unsigned char) val2;

	p_adap = mipi_i2c_init();
	mipi_i2c_read(p_adap, 1, data);

	pr_err("0x%02x\n", data[0]);

err:
	mutex_unlock(&mipi_sysfs_lock);
	return size;
}

/* /sys/devices/1800f000.hdmirx/mipi_write */
static DEVICE_ATTR(mipi_write, 0644, mipi_write_show, mipi_write_store);
/* /sys/devices/1800f000.hdmirx/mipi_read */
static DEVICE_ATTR(mipi_read, 0644, mipi_read_show, mipi_read_store);

void register_mipi_i2c_sysfs(struct platform_device *pdev)
{
	device_create_file(&pdev->dev, &dev_attr_mipi_write);
	device_create_file(&pdev->dev, &dev_attr_mipi_read);
}

//2048x1536/YUV2/30fps/MIPI/4Lane
void RLE0551C_patterngen_A(struct i2c_adapter *p_adap)
{
    //unsigned int A[]={9, 0x04, 0x2a, 0x200, 0x04, 0x10, 0x200, 0x04, 0x09, 0x201};
    unsigned char data1[]={0x04, 0x2a, 0x00};
    unsigned char data2[]={0x04, 0x10, 0x00};
    unsigned char data3[]={0x04, 0x09, 0x01};

    //pr_info("RLE0551C_patterngen_A p_adap=%x\n",p_adap);
    mipi_i2c_write(p_adap, 3, data1);

    mipi_i2c_write(p_adap, 3, data2);

    mipi_i2c_write(p_adap, 3, data3);
}

#if 1
void RLE0551C_patterngen_B(struct i2c_adapter *p_adap)
{
    //unsigned int B[]={9, 0x04, 0x00, 0x201, 0x04, 0x67, 0x200, 0x03, 0x1a, 0x200};
    unsigned char data1[]={0x04, 0x00, 0x01};
    unsigned char data2[]={0x04, 0x67, 0x00};
    unsigned char data3[]={0x03, 0x1A, 0x00};

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    mipi_i2c_write(p_adap, 3, data3);

}

void RLE0551C_patterngen_C(struct i2c_adapter *p_adap)
{
    //unsigned int C[]={3, 0x02, 0x1a, 0x200};
    unsigned char data1[]={0x02, 0x1a, 0x00};

    mipi_i2c_write(p_adap, 3, data1);
}

void RLE0551C_patterngen_D(struct i2c_adapter *p_adap)
{
    //unsigned int D[]={9, 0x04, 0x26, 0x200, 0x04, 0x15, 0x200, 0x04, 0x24, 0x200};
    unsigned char data1[]={0x04, 0x26, 0x00};
    unsigned char data2[]={0x04, 0x15, 0x00};
    unsigned char data3[]={0x04, 0x24, 0x00};

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    mipi_i2c_write(p_adap, 3, data3);
}

void RLE0551C_patterngen_E(struct i2c_adapter *p_adap)
{
    //unsigned int E[]={9, 0x04, 0x2e, 0x200, 0x04, 0x34, 0x200, 0x04, 0x35, 0x200};
    unsigned char data1[]={0x04, 0x2e, 0x00};
    unsigned char data2[]={0x04, 0x34, 0x00};
    unsigned char data3[]={0x04, 0x35, 0x00};

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    mipi_i2c_write(p_adap, 3, data3);
}

void RLE0551C_patterngen_F(struct i2c_adapter *p_adap)
{
    //unsigned int F[]={6, 0x04, 0x36, 0x21f, 0x04, 0x37, 0x200};
    unsigned char data1[]={0x04, 0x36, 0x1F};
    unsigned char data2[]={0x04, 0x37, 0x00};

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
}

void RLE0551C_patterngen_G(struct i2c_adapter *p_adap)
{
    //unsigned int G[]={9, 0x02, 0x00, 0x210, 0x02, 0x01, 0x200, 0x02, 0x02, 0x206};
    unsigned char data1[]={0x02, 0x00, 0x10};
    unsigned char data2[]={0x02, 0x01, 0x00};
    unsigned char data3[]={0x02, 0x02, 0x06};

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    mipi_i2c_write(p_adap, 3, data3);
}

void RLE0551C_patterngen_H(struct i2c_adapter *p_adap)
{
    //unsigned int H[]={9, 0x02, 0x03, 0x200, 0x02, 0x04, 0x200, 0x02, 0x05, 0x2ac};
    unsigned char data1[]={0x02, 0x03, 0x00};
    unsigned char data2[]={0x02, 0x04, 0x00};
    unsigned char data3[]={0x02, 0x05, 0xac};

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    mipi_i2c_write(p_adap, 3, data3);
}

void RLE0551C_patterngen_I(struct i2c_adapter *p_adap)
{
    //unsigned int I[]={9, 0x02, 0x06, 0x200, 0x02, 0x07, 0x20a, 0x02, 0x08, 0x200};
    unsigned char data1[]={0x02, 0x06, 0x00};
    unsigned char data2[]={0x02, 0x07, 0x0a};
    unsigned char data3[]={0x02, 0x08, 0x00};

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    mipi_i2c_write(p_adap, 3, data3);
}

void RLE0551C_patterngen_J(struct i2c_adapter *p_adap)
{
    //unsigned int J[]={3, 0x02, 0x09, 0x200};
    unsigned char data1[]={0x02, 0x09, 0x00};

    mipi_i2c_write(p_adap, 3, data1);
}

void RLE0551C_patterngen_K(struct i2c_adapter *p_adap)
{
    //unsigned int K[]={9, 0x03, 0x03, 0x200, 0x03, 0x04, 0x231, 0x03, 0x05, 0x204};
    unsigned char data1[]={0x03, 0x03, 0x00};
    unsigned char data2[]={0x03, 0x04, 0x31};
    unsigned char data3[]={0x03, 0x05, 0x04};

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    mipi_i2c_write(p_adap, 3, data3);
}

void RLE0551C_patterngen_L(struct i2c_adapter *p_adap)
{
    //unsigned int K[]={9, 0x03, 0x06, 0x203, 0x03, 0x07, 0x20d, 0x03, 0x08, 0x209};
    unsigned char data1[]={0x03, 0x06, 0x03};
    unsigned char data2[]={0x03, 0x07, 0x0d};
    unsigned char data3[]={0x03, 0x08, 0x09};

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    mipi_i2c_write(p_adap, 3, data3);
}

void RLE0551C_patterngen_M(struct i2c_adapter *p_adap)
{
    //unsigned int M[]={9, 0x03, 0x09, 0x21d, 0x03, 0x0a, 0x201, 0x03, 0x0b, 0x203};
    unsigned char data1[]={0x03, 0x09, 0x1d};
    unsigned char data2[]={0x03, 0x0a, 0x01};
    unsigned char data3[]={0x03, 0x0b, 0x03};

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    mipi_i2c_write(p_adap, 3, data3);
}

void RLE0551C_patterngen_N(struct i2c_adapter *p_adap)
{
    //unsigned int N[]={9, 0x03, 0x0c, 0x203, 0x03, 0x0d, 0x203, 0x03, 0x0e, 0x202};
    unsigned char data1[]={0x03, 0x0c, 0x03};
    unsigned char data2[]={0x03, 0x0d, 0x03};
    unsigned char data3[]={0x03, 0x0e, 0x02};

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    mipi_i2c_write(p_adap, 3, data3);
}

void RLE0551C_patterngen_O(struct i2c_adapter *p_adap)
{
    //unsigned int O[]={9, 0x03, 0x0f, 0x200, 0x03, 0x01, 0x200, 0x03, 0x02, 0x2c3};
    unsigned char data1[]={0x03, 0x0f, 0x00};
    unsigned char data2[]={0x03, 0x01, 0x00};
    unsigned char data3[]={0x03, 0x02, 0xc3};

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    mipi_i2c_write(p_adap, 3, data3);
}

void RLE0551C_patterngen_P(struct i2c_adapter *p_adap)
{
    //unsigned int P[]={9, 0x03, 0x11, 0x200, 0x03, 0x17, 0x203, 0x03, 0x18, 0x2e4};
    unsigned char data1[]={0x03, 0x11, 0x00};
    unsigned char data2[]={0x03, 0x17, 0x03};
    unsigned char data3[]={0x03, 0x18, 0xe4};

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    mipi_i2c_write(p_adap, 3, data3);
}

void RLE0551C_patterngen_Q(struct i2c_adapter *p_adap)
{
    //unsigned int Q[]={9, 0x03, 0x19, 0x21e, 0x04, 0x18, 0x207, 0x03, 0x00, 0x201};
    unsigned char data1[]={0x03, 0x19, 0x1e};
    unsigned char data2[]={0x04, 0x18, 0x07};
    unsigned char data3[]={0x03, 0x00, 0x01};

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    mipi_i2c_write(p_adap, 3, data3);
}

void RLE0551C_patterngen_R(struct i2c_adapter *p_adap)
{
    //unsigned int R[]={9, 0x02, 0x11, 0x270, 0x02, 0x12, 0x2a0, 0x02, 0x13, 0x200};
    unsigned char data1[]={0x02, 0x11, 0x70};
    unsigned char data2[]={0x02, 0x12, 0xa0};
    unsigned char data3[]={0x02, 0x13, 0x00};

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    mipi_i2c_write(p_adap, 3, data3);
}

void RLE0551C_patterngen_S(struct i2c_adapter *p_adap)
{
    //unsigned int S[]={9, 0x02, 0x14, 0x210, 0x02, 0x15, 0x210, 0x02, 0x16, 0x211};
    unsigned char data1[]={0x02, 0x14, 0x10};
    unsigned char data2[]={0x02, 0x15, 0x10};
    unsigned char data3[]={0x02, 0x16, 0x11};

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    mipi_i2c_write(p_adap, 3, data3);
}

void RLE0551C_patterngen_T(struct i2c_adapter *p_adap)
{
    //unsigned int T[]={9, 0x02, 0x17, 0x210, 0x02, 0x18, 0x210, 0x02, 0x19, 0x200};
    unsigned char data1[]={0x02, 0x17, 0x10};
    unsigned char data2[]={0x02, 0x18, 0x10};
    unsigned char data3[]={0x02, 0x19, 0x00};

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    mipi_i2c_write(p_adap, 3, data3);
}

void RLE0551C_patterngen_U(struct i2c_adapter *p_adap)
{
    //unsigned int U[]={3, 0x02, 0x10, 0x2c0};
    unsigned char data1[]={0x02, 0x10, 0xc0};

    mipi_i2c_write(p_adap, 3, data1);
}
#endif

void RLE0551C_patterngen_2048x1536(void)
{
//	i2c_ini();			//i2c_ini
    struct i2c_adapter *p_adap;
    //int ret;
    pr_info("RLE0551C_patterngen_2048x1536\n");
    p_adap = mipi_i2c_init();

    if(p_adap == NULL)
    {
        pr_err("[Init i2c fail]\n");
        return;
    }
    //pr_info("RLE0551C_patterngen_2048x1536 p_adap = %x\n",p_adap);
  /*RLE0551C pattern gen 2048x1536/YUV2/30fps/MIPI/4Lane */
  RLE0551C_patterngen_A(p_adap);
  //RLE0551C_patterngen_A(p_adap);
  //RLE0551C_patterngen_A(p_adap);

#if 1
  RLE0551C_patterngen_B(p_adap);
  RLE0551C_patterngen_C(p_adap);
  RLE0551C_patterngen_D(p_adap);
  RLE0551C_patterngen_E(p_adap);
  RLE0551C_patterngen_F(p_adap);
  RLE0551C_patterngen_G(p_adap);
  RLE0551C_patterngen_H(p_adap);
  RLE0551C_patterngen_I(p_adap);
  RLE0551C_patterngen_J(p_adap);
  RLE0551C_patterngen_K(p_adap);
  RLE0551C_patterngen_L(p_adap);
  RLE0551C_patterngen_M(p_adap);
  RLE0551C_patterngen_N(p_adap);
  RLE0551C_patterngen_O(p_adap);
  RLE0551C_patterngen_P(p_adap);
  RLE0551C_patterngen_Q(p_adap);
  RLE0551C_patterngen_R(p_adap);
  RLE0551C_patterngen_S(p_adap);
  RLE0551C_patterngen_T(p_adap);
  RLE0551C_patterngen_U(p_adap);
#endif
}

void RTS5845_Rear_Preview_Format(struct i2c_adapter *p_adap)
{
    //unsigned int PALBG[]={3, 0x03, 0x50, 0x212};
    unsigned char data1[]={0x03, 0x50, 0x12};

    mipi_i2c_write(p_adap, 3, data1);
    //i2c_write(addr,PALBG);
    //wait();
}

void RTS5845_Rear_Preview_Width(struct i2c_adapter *p_adap, unsigned int h_input_len)
{
    //unsigned int PALBG[]={6, 0x03, 0x51, 0x20a, 0x03, 0x52, 0x220};
    unsigned char data1[]={0x03, 0x51, 0x0a};
    unsigned char data2[]={0x03, 0x52, 0x20};

    data1[2] = (unsigned char)((h_input_len >> 8) & 0x00FF);
    data2[2] = (unsigned char)((h_input_len) & 0x00FF);
    pr_info("[DATA_W:%x,%x]\n",data1[2],data2[2]);
    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    //i2c_write(addr,PALBG);
    //wait();
}

void RTS5845_Rear_Preview_Heigth(struct i2c_adapter *p_adap, unsigned int v_input_len)
{
    //unsigned int PALBG[]={6, 0x03, 0x53, 0x207, 0x03, 0x54, 0x298};
    unsigned char data1[]={0x03, 0x53, 0x07};
    unsigned char data2[]={0x03, 0x54, 0x98};

    data1[2] = (unsigned char)((v_input_len >> 8) & 0x00FF);
    data2[2] = (unsigned char)((v_input_len) & 0x00FF);

    pr_info("[DATA_H:%x,%x]\n",data1[2],data2[2]);
    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    //i2c_write(addr,PALBG);
    //wait();
}


void RTS5845_Rear_Preview_FPS_1(struct i2c_adapter *p_adap)
{
    //unsigned int PALBG[]={6, 0x03, 0x55, 0x200, 0x03, 0x56, 0x205};
    unsigned char data1[]={0x03, 0x55, 0x00};
    unsigned char data2[]={0x03, 0x56, 0x05};

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    //i2c_write(addr,PALBG);
    //wait();
}

void RTS5845_Rear_Preview_FPS_2(struct i2c_adapter *p_adap)
{
    //unsigned int PALBG[]={6, 0x03, 0x57, 0x216, 0x03, 0x58, 0x215};
    unsigned char data1[]={0x03, 0x57, 0x16};
    unsigned char data2[]={0x03, 0x58, 0x15};

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    //i2c_write(addr,PALBG);
    //wait();
}

void RTS5845_Rear_Still_Format(struct i2c_adapter *p_adap)
{
    //unsigned int PALBG[]={3, 0x03, 0x59, 0x212};
    unsigned char data1[]={0x03, 0x59, 0x12};

    mipi_i2c_write(p_adap, 3, data1);
    //i2c_write(addr,PALBG);
    //wait();
}

void RTS5845_Rear_Still_Width(struct i2c_adapter *p_adap, unsigned int h_input_len)
{
    //unsigned int PALBG[]={6, 0x03, 0x5a, 0x20a, 0x03, 0x5b, 0x220};
    unsigned char data1[]={0x03, 0x5a, 0x0a};
    unsigned char data2[]={0x03, 0x5b, 0x20};

    data1[2] = (unsigned char)((h_input_len >> 8) & 0x00FF);
    data2[2] = (unsigned char)((h_input_len) & 0x00FF);

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    //i2c_write(addr,PALBG);
    //wait();
}

void RTS5845_Rear_Still_Heigth(struct i2c_adapter *p_adap, unsigned int v_input_len)
{
    //unsigned int PALBG[]={6, 0x03, 0x5c, 0x207, 0x03, 0x5d, 0x298};
    unsigned char data1[]={0x03, 0x5c, 0x07};
    unsigned char data2[]={0x03, 0x5d, 0x98};

    data1[2] = (unsigned char)((v_input_len >> 8) & 0x00FF);
    data2[2] = (unsigned char)((v_input_len) & 0x00FF);

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    //i2c_write(addr,PALBG);
    //wait();
}

void RTS5845_Rear_Still_FPS_1(struct i2c_adapter *p_adap)
{
    //unsigned int PALBG[]={6, 0x03, 0x5e, 0x200, 0x03, 0x5f, 0x205};
    unsigned char data1[]={0x03, 0x5e, 0x00};
    unsigned char data2[]={0x03, 0x5f, 0x05};

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    //i2c_write(addr,PALBG);
    //wait();
}

void RTS5845_Rear_Still_FPS_2(struct i2c_adapter *p_adap)
{
    //unsigned int PALBG[]={6, 0x03, 0x60, 0x216, 0x03, 0x61, 0x215};
    unsigned char data1[]={0x03, 0x60, 0x16};
    unsigned char data2[]={0x03, 0x61, 0x15};

    mipi_i2c_write(p_adap, 3, data1);
    mipi_i2c_write(p_adap, 3, data2);
    //i2c_write(addr,PALBG);
    //wait();
}

void RTS5845_Rear_Still_Frame_No(struct i2c_adapter *p_adap)
{
    //unsigned int PALBG[]={3, 0x03, 0x75, 0x201};
    unsigned char data1[]={0x03, 0x75, 0x01};

    mipi_i2c_write(p_adap, 3, data1);
    //i2c_write(addr,PALBG);
    //wait();
}

void RTS5845_Rear_switch(struct i2c_adapter *p_adap)
{
    //unsigned int PALBG[]={3, 0x0c, 0x1e, 0x201};
    unsigned char data1[]={0x0c, 0x1e, 0x01};

    mipi_i2c_write(p_adap, 3, data1);
    //i2c_write(addr,PALBG);
    //wait();
}


void RTS5845_Rear_start(struct i2c_adapter *p_adap)
{
    //unsigned int PALBG[]={3, 0x0c, 0x00, 0x202};
    unsigned char data1[]={0x0c, 0x00, 0x02};

    mipi_i2c_write(p_adap, 3, data1);
//    i2c_write(addr,PALBG);
//    wait();
}

void RTS5845(unsigned int h_input_len, unsigned int v_input_len)
{
//	i2c_ini();			//i2c_ini
    struct i2c_adapter *p_adap;

    p_adap = mipi_i2c_init();

    pr_info("RTS5845\n");
    if(p_adap == NULL)
    {
        pr_err("[Init i2c fail]\n");
        return;
    }
    RTS5845_Rear_Preview_Format(p_adap);
    RTS5845_Rear_Preview_Width(p_adap, h_input_len);
    RTS5845_Rear_Preview_Heigth(p_adap, v_input_len);
    RTS5845_Rear_Preview_FPS_1(p_adap);
    RTS5845_Rear_Preview_FPS_2(p_adap);
    RTS5845_Rear_Still_Format(p_adap);
    RTS5845_Rear_Still_Width(p_adap, h_input_len);
    RTS5845_Rear_Still_Heigth(p_adap, v_input_len);
    RTS5845_Rear_Still_FPS_1(p_adap);
    RTS5845_Rear_Still_FPS_2(p_adap);
    RTS5845_Rear_Still_Frame_No(p_adap);
    RTS5845_Rear_switch(p_adap);
    //wait10();
    RTS5845_Rear_start(p_adap);
}

