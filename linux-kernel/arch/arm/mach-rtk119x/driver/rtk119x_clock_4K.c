#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include "rtk119x_clock_4K.h"
#include "RPCDriver.h"
#include "../../../drivers/staging/android/ion/ion.h"
#include "../../../drivers/staging/android/uapi/rtk_phoenix_ion.h"


struct class *clock_4K_class;
int clock_4K_enable = 1;
extern struct ion_device *rtk_phoenix_ion_device;
struct ion_client *clock4K_client;


void send_clock4K_rpc(int val) {
	AUDIO_RPC_AIO_PRIVATEINFO_PARAMETERS *cmd;
	AUDIO_RPC_PRIVATEINFO_RETURNVAL*res;
	int ret = 0;
	unsigned long RPC_ret;
	struct ion_handle *handle = NULL;
	ion_phys_addr_t dat;
	size_t len;
	unsigned int offset;

	handle = ion_alloc(clock4K_client, 4096, 1024, RTK_PHOENIX_ION_HEAP_AUDIO_MASK, AUDIO_ION_FLAG);

	if (IS_ERR(handle)) {
		pr_err("[%s %d ion_alloc fail]\n", __FUNCTION__, __LINE__);
		goto exit;
	}

	if (ion_phys(clock4K_client, handle, &dat, &len) != 0) {
		pr_err("[%s phy malloc fail ]\n", __FUNCTION__);
		goto exit;
	}

	cmd = ion_map_kernel(clock4K_client, handle);
	res = (AUDIO_RPC_PRIVATEINFO_RETURNVAL *)((unsigned long)((unsigned int)cmd + sizeof(AUDIO_RPC_AIO_PRIVATEINFO_PARAMETERS) + 8) & 0xFFFFFFFC);

	offset = (unsigned long)res - (unsigned long)cmd;

	memset(cmd, 0, sizeof(AUDIO_RPC_AIO_PRIVATEINFO_PARAMETERS));
	cmd->type = htonl(ENUM_PRIVATEINFO_AIO_GENERATE_AIN_RCLK);
	cmd->privateInfo[0] = htonl(val);

	if (send_rpc_command(RPC_AUDIO,
		ENUM_KERNEL_RPC_AIO_PRIVATEINFO,
		CONVERT_FOR_AVCPU(dat),
		CONVERT_FOR_AVCPU(dat+offset),
		&RPC_ret)) {
		pr_err("[%s %d RPC fail]\n", __FUNCTION__, __LINE__);
		goto exit;
	}

	if (RPC_ret != S_OK ) {
		pr_err("[%s %d RPC fail]\n", __FUNCTION__, __LINE__);
		goto exit;
	}

exit:
	if(handle != NULL && !IS_ERR(handle)) {
		ion_unmap_kernel(clock4K_client, handle);
		ion_free(clock4K_client, handle);
	}
}


static ssize_t clock_4K_show(struct class *class,
		struct class_attribute *attr, char *buf)
{

	return sprintf(buf, "%d\n", clock_4K_enable);
}



ssize_t clock_4K_store(struct class *class, struct class_attribute *attr,
			const char *buf, size_t count)
{
	long val;
	int ret = kstrtol(buf, 10, &val);
	if (ret < 0)
		return -ENOMEM;

	if (val == 0) {
		clock_4K_enable = 0;
		send_clock4K_rpc(0);
	} else if (val == 1){
		clock_4K_enable = 1;
		send_clock4K_rpc(16000);
	}

	return count;


}

static CLASS_ATTR_RW(clock_4K);

static int clock_4K_init(void)
{
        int ret;

	clock4K_client = ion_client_create(rtk_phoenix_ion_device, "clock4K_Driver");

	clock_4K_class = class_create(THIS_MODULE, "clock_4K");
	if (clock_4K_class == NULL)
		pr_err("failed to create clock_4K attribute!\n");
	ret = class_create_file(clock_4K_class, &class_attr_clock_4K);

	send_clock4K_rpc(16000);
	return ret;

}

// module exit function .......
static void clock_4K_exit(void)
{
	class_destroy(clock_4K_class);
}


late_initcall(clock_4K_init);
module_exit(clock_4K_exit);
MODULE_LICENSE("GPL");

