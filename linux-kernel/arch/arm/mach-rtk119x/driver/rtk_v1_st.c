#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/mm.h>
#include <linux/platform_device.h>
#include <linux/module.h>

#define V1_REG_CTL_ADDR     0x18005008
#define V1_REG_HEIGHT_ADDR  0x180051A8
#define V1_REG_WIDTH_ADDR   0x18005234
#define V1_REG_POSX_ADDR    0x18005618
#define V1_REG_POSY_ADDR    0x1800561C
#define V1_REG_DWORD 4 

static void __iomem *v1_reg_virt_addr;

static ssize_t v1_status_show(struct kobject *kobj, struct device_attribute *attr, char *buf)
{
    volatile unsigned int reg_ctl, reg_v1_H, reg_v1_W, reg_v1_X, reg_v1_Y;
    //v1 status: 4th bit
    v1_reg_virt_addr = ioremap(V1_REG_CTL_ADDR, V1_REG_DWORD);
    reg_ctl = (__raw_readl(v1_reg_virt_addr) & (1 << 4)) >> 4;

    // height 0:12
    v1_reg_virt_addr = ioremap(V1_REG_HEIGHT_ADDR, V1_REG_DWORD);
    reg_v1_H = __raw_readl(v1_reg_virt_addr) & 0xFFF;

    //width 0:12
    v1_reg_virt_addr = ioremap(V1_REG_WIDTH_ADDR, V1_REG_DWORD);
    reg_v1_W = __raw_readl(v1_reg_virt_addr) & 0xFFF;

    // x_start 0:12, x_end 16:28
    v1_reg_virt_addr = ioremap(V1_REG_POSX_ADDR, V1_REG_DWORD);
    reg_v1_X = __raw_readl(v1_reg_virt_addr);

    // y_start 0:12, y_end 16:28
    v1_reg_virt_addr = ioremap(V1_REG_POSY_ADDR, V1_REG_DWORD);
    reg_v1_Y = __raw_readl(v1_reg_virt_addr);

    return scnprintf(buf, PAGE_SIZE,"V1 enable: %d \nwidth, height: %d,%d \nx_start, y_start: %d,%d \nx_end, y_end: %d,%d\n"
            , reg_ctl, reg_v1_W, reg_v1_H
            , reg_v1_X & 0xFFF, reg_v1_Y & 0xFFF
            , (reg_v1_X >> 16) & 0xFFF, (reg_v1_Y >> 16)  & 0xFFF);

}

static struct device_attribute v1_status_attr = __ATTR(v1_status, 0644, v1_status_show, NULL);

static struct attribute *v1_attrs[] = {
    &v1_status_attr.attr,
    NULL,
};

static struct attribute_group rtk_v1_attr_group = {
    .attrs = v1_attrs,
};

static struct kobject *v1_kobj;

static int v1_probe(struct platform_device *pdev)
{
    //Do nothing
    return 0;
}

static int v1_remove(struct platform_device *pdev)
{
    //Do nothing
    return 0;
}

static struct platform_driver v1_status_driver = {
    .driver = {
        .name = "rtk_v1",
    },
    .probe = v1_probe,
    .remove = v1_remove,
};

static int __init v1_status_init(void)
{
    int res;

    v1_kobj = kobject_create_and_add("rtk_v1", kernel_kobj);
    if (v1_kobj)
    {
        res = sysfs_create_group(v1_kobj, &rtk_v1_attr_group);
        if (res)
            printk(KERN_WARNING "[v1_st] %d: unable to create sysfs entry\n", __LINE__);
    }
    else
        printk(KERN_WARNING "[v1_st] %d: unable to create sysfs entry\n", __LINE__);

    printk(KERN_INFO "[v1_st] begin v1_status_init\n");
    res = platform_driver_register(&v1_status_driver);
    printk(KERN_INFO "[v1_st] end v1_status_init result=0x%x\n", res);
    return res;
}

static void __exit v1_status_exit(void)
{
    printk(KERN_INFO "[v1_st] v1_status_exit\n");
    platform_driver_unregister(&v1_status_driver);
    return;
}

MODULE_AUTHOR("REALTEK Semiconductor Corp.");
MODULE_DESCRIPTION("V1 Status Linux driver");
MODULE_LICENSE("GPL");

module_init(v1_status_init);
module_exit(v1_status_exit);
