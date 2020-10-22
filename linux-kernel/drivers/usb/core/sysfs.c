/*
 * drivers/usb/core/sysfs.c
 *
 * (C) Copyright 2002 David Brownell
 * (C) Copyright 2002,2004 Greg Kroah-Hartman
 * (C) Copyright 2002,2004 IBM Corp.
 *
 * All of the sysfs file attributes for usb devices and interfaces.
 *
 */


#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/usb.h>
#include <linux/usb/quirks.h>
#include "usb.h"

/* Active configuration fields */
#define usb_actconfig_show(field, format_string)			\
static ssize_t  show_##field(struct device *dev,			\
		struct device_attribute *attr, char *buf)		\
{									\
	struct usb_device *udev;					\
	struct usb_host_config *actconfig;				\
									\
	udev = to_usb_device(dev);					\
	actconfig = udev->actconfig;					\
	if (actconfig)							\
		return sprintf(buf, format_string,			\
				actconfig->desc.field);			\
	else								\
		return 0;						\
}									\

#define usb_actconfig_attr(field, format_string)		\
	usb_actconfig_show(field, format_string)		\
	static DEVICE_ATTR(field, S_IRUGO, show_##field, NULL);

usb_actconfig_attr(bNumInterfaces, "%2d\n")
usb_actconfig_attr(bmAttributes, "%2x\n")

static ssize_t show_bMaxPower(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device(dev);
	actconfig = udev->actconfig;
	if (!actconfig)
		return 0;
	return sprintf(buf, "%dmA\n", usb_get_max_power(udev, actconfig));
}
static DEVICE_ATTR(bMaxPower, S_IRUGO, show_bMaxPower, NULL);

static ssize_t show_configuration_string(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device(dev);
	actconfig = udev->actconfig;
	if ((!actconfig) || (!actconfig->string))
		return 0;
	return sprintf(buf, "%s\n", actconfig->string);
}
static DEVICE_ATTR(configuration, S_IRUGO, show_configuration_string, NULL);

/* configuration value is always present, and r/w */
usb_actconfig_show(bConfigurationValue, "%u\n");

static ssize_t
set_bConfigurationValue(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct usb_device	*udev = to_usb_device(dev);
	int			config, value;

	if (sscanf(buf, "%d", &config) != 1 || config < -1 || config > 255)
		return -EINVAL;
	usb_lock_device(udev);
	value = usb_set_configuration(udev, config);
	usb_unlock_device(udev);
	return (value < 0) ? value : count;
}

static DEVICE_ATTR_IGNORE_LOCKDEP(bConfigurationValue, S_IRUGO | S_IWUSR,
		show_bConfigurationValue, set_bConfigurationValue);

/* String fields */
#define usb_string_attr(name)						\
static ssize_t  show_##name(struct device *dev,				\
		struct device_attribute *attr, char *buf)		\
{									\
	struct usb_device *udev;					\
	int retval;							\
									\
	udev = to_usb_device(dev);					\
	usb_lock_device(udev);						\
	retval = sprintf(buf, "%s\n", udev->name);			\
	usb_unlock_device(udev);					\
	return retval;							\
}									\
static DEVICE_ATTR(name, S_IRUGO, show_##name, NULL);

usb_string_attr(product);
usb_string_attr(manufacturer);
usb_string_attr(serial);

static ssize_t
show_speed(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	char *speed;

	udev = to_usb_device(dev);

	switch (udev->speed) {
	case USB_SPEED_LOW:
		speed = "1.5";
		break;
	case USB_SPEED_UNKNOWN:
	case USB_SPEED_FULL:
		speed = "12";
		break;
	case USB_SPEED_HIGH:
		speed = "480";
		break;
	case USB_SPEED_WIRELESS:
		speed = "480";
		break;
	case USB_SPEED_SUPER:
		speed = "5000";
		break;
	default:
		speed = "unknown";
	}
	return sprintf(buf, "%s\n", speed);
}
static DEVICE_ATTR(speed, S_IRUGO, show_speed, NULL);

static ssize_t
show_busnum(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;

	udev = to_usb_device(dev);
	return sprintf(buf, "%d\n", udev->bus->busnum);
}
static DEVICE_ATTR(busnum, S_IRUGO, show_busnum, NULL);

static ssize_t
show_devnum(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;

	udev = to_usb_device(dev);
	return sprintf(buf, "%d\n", udev->devnum);
}
static DEVICE_ATTR(devnum, S_IRUGO, show_devnum, NULL);

static ssize_t
show_devpath(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;

	udev = to_usb_device(dev);
	return sprintf(buf, "%s\n", udev->devpath);
}
static DEVICE_ATTR(devpath, S_IRUGO, show_devpath, NULL);

static ssize_t
show_version(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	u16 bcdUSB;

	udev = to_usb_device(dev);
	bcdUSB = le16_to_cpu(udev->descriptor.bcdUSB);
	return sprintf(buf, "%2x.%02x\n", bcdUSB >> 8, bcdUSB & 0xff);
}
static DEVICE_ATTR(version, S_IRUGO, show_version, NULL);

static ssize_t
show_maxchild(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;

	udev = to_usb_device(dev);
	return sprintf(buf, "%d\n", udev->maxchild);
}
static DEVICE_ATTR(maxchild, S_IRUGO, show_maxchild, NULL);

static ssize_t
show_quirks(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;

	udev = to_usb_device(dev);
	return sprintf(buf, "0x%x\n", udev->quirks);
}
static DEVICE_ATTR(quirks, S_IRUGO, show_quirks, NULL);

static ssize_t
show_avoid_reset_quirk(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;

	udev = to_usb_device(dev);
	return sprintf(buf, "%d\n", !!(udev->quirks & USB_QUIRK_RESET));
}

static ssize_t
set_avoid_reset_quirk(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct usb_device	*udev = to_usb_device(dev);
	int			val;

	if (sscanf(buf, "%d", &val) != 1 || val < 0 || val > 1)
		return -EINVAL;
	usb_lock_device(udev);
	if (val)
		udev->quirks |= USB_QUIRK_RESET;
	else
		udev->quirks &= ~USB_QUIRK_RESET;
	usb_unlock_device(udev);
	return count;
}

static DEVICE_ATTR(avoid_reset_quirk, S_IRUGO | S_IWUSR,
		show_avoid_reset_quirk, set_avoid_reset_quirk);

static ssize_t
show_urbnum(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;

	udev = to_usb_device(dev);
	return sprintf(buf, "%d\n", atomic_read(&udev->urbnum));
}
static DEVICE_ATTR(urbnum, S_IRUGO, show_urbnum, NULL);

static ssize_t
show_removable(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	char *state;

	udev = to_usb_device(dev);

	switch (udev->removable) {
	case USB_DEVICE_REMOVABLE:
		state = "removable";
		break;
	case USB_DEVICE_FIXED:
		state = "fixed";
		break;
	default:
		state = "unknown";
	}

	return sprintf(buf, "%s\n", state);
}
static DEVICE_ATTR(removable, S_IRUGO, show_removable, NULL);

static ssize_t
show_ltm_capable(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (usb_device_supports_ltm(to_usb_device(dev)))
		return sprintf(buf, "%s\n", "yes");
	return sprintf(buf, "%s\n", "no");
}
static DEVICE_ATTR(ltm_capable, S_IRUGO, show_ltm_capable, NULL);

#ifdef	CONFIG_PM

static ssize_t
show_persist(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev = to_usb_device(dev);

	return sprintf(buf, "%d\n", udev->persist_enabled);
}

static ssize_t
set_persist(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct usb_device *udev = to_usb_device(dev);
	int value;

	/* Hubs are always enabled for USB_PERSIST */
	if (udev->descriptor.bDeviceClass == USB_CLASS_HUB)
		return -EPERM;

	if (sscanf(buf, "%d", &value) != 1)
		return -EINVAL;

	usb_lock_device(udev);
	udev->persist_enabled = !!value;
	usb_unlock_device(udev);
	return count;
}

static DEVICE_ATTR(persist, S_IRUGO | S_IWUSR, show_persist, set_persist);

static int add_persist_attributes(struct device *dev)
{
	int rc = 0;

	if (is_usb_device(dev)) {
		struct usb_device *udev = to_usb_device(dev);

		/* Hubs are automatically enabled for USB_PERSIST,
		 * no point in creating the attribute file.
		 */
		if (udev->descriptor.bDeviceClass != USB_CLASS_HUB)
			rc = sysfs_add_file_to_group(&dev->kobj,
					&dev_attr_persist.attr,
					power_group_name);
	}
	return rc;
}

static void remove_persist_attributes(struct device *dev)
{
	sysfs_remove_file_from_group(&dev->kobj,
			&dev_attr_persist.attr,
			power_group_name);
}
#else

#define add_persist_attributes(dev)	0
#define remove_persist_attributes(dev)	do {} while (0)

#endif	/* CONFIG_PM */

#ifdef	CONFIG_PM_RUNTIME

static ssize_t
show_connected_duration(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct usb_device *udev = to_usb_device(dev);

	return sprintf(buf, "%u\n",
			jiffies_to_msecs(jiffies - udev->connect_time));
}

static DEVICE_ATTR(connected_duration, S_IRUGO, show_connected_duration, NULL);

/*
 * If the device is resumed, the last time the device was suspended has
 * been pre-subtracted from active_duration.  We add the current time to
 * get the duration that the device was actually active.
 *
 * If the device is suspended, the active_duration is up-to-date.
 */
static ssize_t
show_active_duration(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct usb_device *udev = to_usb_device(dev);
	int duration;

	if (udev->state != USB_STATE_SUSPENDED)
		duration = jiffies_to_msecs(jiffies + udev->active_duration);
	else
		duration = jiffies_to_msecs(udev->active_duration);
	return sprintf(buf, "%u\n", duration);
}

static DEVICE_ATTR(active_duration, S_IRUGO, show_active_duration, NULL);

static ssize_t
show_autosuspend(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", dev->power.autosuspend_delay / 1000);
}

static ssize_t
set_autosuspend(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int value;

	if (sscanf(buf, "%d", &value) != 1 || value >= INT_MAX/1000 ||
			value <= -INT_MAX/1000)
		return -EINVAL;

	pm_runtime_set_autosuspend_delay(dev, value * 1000);
	return count;
}

static DEVICE_ATTR(autosuspend, S_IRUGO | S_IWUSR,
		show_autosuspend, set_autosuspend);

static const char on_string[] = "on";
static const char auto_string[] = "auto";

static void warn_level(void) {
	static int level_warned;

	if (!level_warned) {
		level_warned = 1;
		printk(KERN_WARNING "WARNING! power/level is deprecated; "
				"use power/control instead\n");
	}
}

static ssize_t
show_level(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev = to_usb_device(dev);
	const char *p = auto_string;

	warn_level();
	if (udev->state != USB_STATE_SUSPENDED && !udev->dev.power.runtime_auto)
		p = on_string;
	return sprintf(buf, "%s\n", p);
}

static ssize_t
set_level(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct usb_device *udev = to_usb_device(dev);
	int len = count;
	char *cp;
	int rc = count;

	warn_level();
	cp = memchr(buf, '\n', count);
	if (cp)
		len = cp - buf;

	usb_lock_device(udev);

	if (len == sizeof on_string - 1 &&
			strncmp(buf, on_string, len) == 0)
		usb_disable_autosuspend(udev);

	else if (len == sizeof auto_string - 1 &&
			strncmp(buf, auto_string, len) == 0)
		usb_enable_autosuspend(udev);

	else
		rc = -EINVAL;

	usb_unlock_device(udev);
	return rc;
}

static DEVICE_ATTR(level, S_IRUGO | S_IWUSR, show_level, set_level);

static ssize_t
show_usb2_hardware_lpm(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct usb_device *udev = to_usb_device(dev);
	const char *p;

	if (udev->usb2_hw_lpm_enabled == 1)
		p = "enabled";
	else
		p = "disabled";

	return sprintf(buf, "%s\n", p);
}

static ssize_t
set_usb2_hardware_lpm(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct usb_device *udev = to_usb_device(dev);
	bool value;
	int ret;

	usb_lock_device(udev);

	ret = strtobool(buf, &value);

	if (!ret)
		ret = usb_set_usb2_hardware_lpm(udev, value);

	usb_unlock_device(udev);

	if (!ret)
		return count;

	return ret;
}

static DEVICE_ATTR(usb2_hardware_lpm, S_IRUGO | S_IWUSR, show_usb2_hardware_lpm,
			set_usb2_hardware_lpm);

static struct attribute *usb2_hardware_lpm_attr[] = {
	&dev_attr_usb2_hardware_lpm.attr,
	NULL,
};
static struct attribute_group usb2_hardware_lpm_attr_group = {
	.name	= power_group_name,
	.attrs	= usb2_hardware_lpm_attr,
};

static struct attribute *power_attrs[] = {
	&dev_attr_autosuspend.attr,
	&dev_attr_level.attr,
	&dev_attr_connected_duration.attr,
	&dev_attr_active_duration.attr,
	NULL,
};
static struct attribute_group power_attr_group = {
	.name	= power_group_name,
	.attrs	= power_attrs,
};

static int add_power_attributes(struct device *dev)
{
	int rc = 0;

	if (is_usb_device(dev)) {
		struct usb_device *udev = to_usb_device(dev);
		rc = sysfs_merge_group(&dev->kobj, &power_attr_group);
		if (udev->usb2_hw_lpm_capable == 1)
			rc = sysfs_merge_group(&dev->kobj,
					&usb2_hardware_lpm_attr_group);
	}

	return rc;
}

static void remove_power_attributes(struct device *dev)
{
	sysfs_unmerge_group(&dev->kobj, &usb2_hardware_lpm_attr_group);
	sysfs_unmerge_group(&dev->kobj, &power_attr_group);
}

#else

#define add_power_attributes(dev)	0
#define remove_power_attributes(dev)	do {} while (0)

#endif	/* CONFIG_PM_RUNTIME */


/* Descriptor fields */
#define usb_descriptor_attr_le16(field, format_string)			\
static ssize_t								\
show_##field(struct device *dev, struct device_attribute *attr,	\
		char *buf)						\
{									\
	struct usb_device *udev;					\
									\
	udev = to_usb_device(dev);					\
	return sprintf(buf, format_string, 				\
			le16_to_cpu(udev->descriptor.field));		\
}									\
static DEVICE_ATTR(field, S_IRUGO, show_##field, NULL);

usb_descriptor_attr_le16(idVendor, "%04x\n")
usb_descriptor_attr_le16(idProduct, "%04x\n")
usb_descriptor_attr_le16(bcdDevice, "%04x\n")

#define usb_descriptor_attr(field, format_string)			\
static ssize_t								\
show_##field(struct device *dev, struct device_attribute *attr,	\
		char *buf)						\
{									\
	struct usb_device *udev;					\
									\
	udev = to_usb_device(dev);					\
	return sprintf(buf, format_string, udev->descriptor.field);	\
}									\
static DEVICE_ATTR(field, S_IRUGO, show_##field, NULL);

usb_descriptor_attr(bDeviceClass, "%02x\n")
usb_descriptor_attr(bDeviceSubClass, "%02x\n")
usb_descriptor_attr(bDeviceProtocol, "%02x\n")
usb_descriptor_attr(bNumConfigurations, "%d\n")
usb_descriptor_attr(bMaxPacketSize0, "%d\n")



/* show if the device is authorized (1) or not (0) */
static ssize_t usb_dev_authorized_show(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	struct usb_device *usb_dev = to_usb_device(dev);
	return snprintf(buf, PAGE_SIZE, "%u\n", usb_dev->authorized);
}


/*
 * Authorize a device to be used in the system
 *
 * Writing a 0 deauthorizes the device, writing a 1 authorizes it.
 */
static ssize_t usb_dev_authorized_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	ssize_t result;
	struct usb_device *usb_dev = to_usb_device(dev);
	unsigned val;
	result = sscanf(buf, "%u\n", &val);
	if (result != 1)
		result = -EINVAL;
	else if (val == 0)
		result = usb_deauthorize_device(usb_dev);
	else
		result = usb_authorize_device(usb_dev);
	return result < 0? result : size;
}

static DEVICE_ATTR_IGNORE_LOCKDEP(authorized, 0644,
	    usb_dev_authorized_show, usb_dev_authorized_store);

/* "Safely remove a device" */
static ssize_t usb_remove_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct usb_device *udev = to_usb_device(dev);
	int rc = 0;

	usb_lock_device(udev);
	if (udev->state != USB_STATE_NOTATTACHED) {

		/* To avoid races, first unconfigure and then remove */
		usb_set_configuration(udev, -1);
		rc = usb_remove_device(udev);
	}
	if (rc == 0)
		rc = count;
	usb_unlock_device(udev);
	return rc;
}
static DEVICE_ATTR_IGNORE_LOCKDEP(remove, 0200, NULL, usb_remove_store);

#ifdef CONFIG_USB_XHCI_HCD_TEST_MODE

#include <linux/slab.h>
#include <linux/usb/ch11.h>
#define UsbTestModeNumber 6
#define UsbSuspendResume 1
#define UsbGetDescriptor 3
static int gUsbHubPort = 1; //default = 1

int gUsbGetDescriptor = 0;
// gUsbGetDescriptor = 1 for the command phase
// gUsbGetDescriptor = 2 for the data phase
// gUsbGetDescriptor = 3 for the status phase

int usb_test_mode_suspend_flag = 0;
EXPORT_SYMBOL(usb_test_mode_suspend_flag);

static int gUsbHubTestMode = 0;
static int gUsbTestModeChanged = 0;

static unsigned int volatile regs_addr = 0;
static ssize_t show_regs_addr(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%x\n", regs_addr);
}
static ssize_t
store_regs_addr(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int			config;

	if (sscanf(buf, "%x", &config) != 1)
		return -EINVAL;

	regs_addr = config;
	printk("0x%x\n", regs_addr);
	return count;
}
static DEVICE_ATTR(regs_addr, S_IRUGO | S_IWUSR, show_regs_addr, store_regs_addr);

static ssize_t show_regs_value(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "0x%x = %x\n", regs_addr, *(unsigned int volatile*)GET_MAPPED_RBUS_ADDR(regs_addr));
}
static ssize_t
store_regs_value(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int			config;

	if (sscanf(buf, "%x", &config) != 1)
		return -EINVAL;

	*(unsigned int volatile*)GET_MAPPED_RBUS_ADDR(regs_addr) = config;
	printk("0x%x = 0x%x\n", regs_addr, *(unsigned int volatile*)GET_MAPPED_RBUS_ADDR(regs_addr));
	return count;
}
static DEVICE_ATTR(regs_value, S_IRUGO | S_IWUSR, show_regs_value, store_regs_value);

static ssize_t  show_bPortNumber (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	if(udev->descriptor.bDeviceClass != USB_CLASS_HUB)
		return sprintf (buf, "%d\n", -1);

	return sprintf (buf, "%d\n", gUsbHubPort);

	return 0;
}

static ssize_t
set_bPortNumber (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	int			config, value;

	if ((value = sscanf (buf, "%u", &config)) != 1 || config > udev->maxchild)
		return -EINVAL;

	if(udev->descriptor.bDeviceClass != USB_CLASS_HUB)
		return value;

	if(gUsbHubPort != config)
	{
		gUsbHubPort = config;
	}

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bPortNumber, S_IRUGO | S_IWUSR,
		show_bPortNumber, set_bPortNumber);

static ssize_t  show_bPortDescriptor (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	return sprintf (buf, "%d\n", gUsbGetDescriptor);

	return 0;
}

extern int get_hub_descriptor_port(struct usb_device *hdev, void *data, int size, int port1);

static ssize_t
set_bPortDescriptor (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	int			config, value;
	unsigned char		*data;
	int			size, i, port;
	int					  ret;

	if ((value = sscanf (buf, "%u", &config)) != 1 || config > UsbGetDescriptor)
		return -EINVAL;

	if(udev->descriptor.bDeviceClass != USB_CLASS_HUB)
		return sprintf ((char *)buf, "%d\n", -1);

	if((config == 0) || (gUsbGetDescriptor != config))
	{
		gUsbGetDescriptor = config;
		port = gUsbHubPort - 1;
		size=0x12;
		data = (unsigned char*)kmalloc(size, GFP_KERNEL);
		if (!data)
			return -ENOMEM;
		memset (data, 0, size);
		ret = get_hub_descriptor_port(udev, data, size, gUsbHubPort);

		if(gUsbGetDescriptor == 0)
		{
			printk(" get device descriptor\n");
			for( i = 0; i < size; i++)
			{
				printk(" %.2x", data[i]);
				if((i % 15) == 0 && (i != 0))
					printk("\n<1>");
			}
			printk("\n");
		}

		if(gUsbGetDescriptor == 3)
			gUsbGetDescriptor = 0;
		kfree(data);
	}

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bPortDescriptor, S_IRUGO | S_IWUSR,
		show_bPortDescriptor, set_bPortDescriptor);

static ssize_t  show_bEnumBus (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	if(udev->descriptor.bDeviceClass != USB_CLASS_HUB)
		return sprintf (buf, "%d\n", -1);

	return sprintf (buf, "%d\n", gUsbHubPort);

	return 0;
}

static ssize_t
set_bEnumBus (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = to_usb_device (dev);
	int			config, value;

	if ((value = sscanf (buf, "%u", &config)) != 1 || config > udev->maxchild)
		return -EINVAL;

	if(udev->descriptor.bDeviceClass != USB_CLASS_HUB)
		return value;

	printk("reset device ...\n");
	usb_reset_device(udev);
	printk("reset OK !!!\n");

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bEnumBus, S_IRUGO | S_IWUSR,
		show_bEnumBus, set_bEnumBus);

int WarmReset_flag = 0;

static ssize_t  show_bWarmReset (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	if(udev->descriptor.bDeviceClass != USB_CLASS_HUB)
		return sprintf (buf, "%d\n", -1);

	return sprintf (buf, "%d\n", gUsbHubPort);

	return 0;
}

static ssize_t
set_bWarmReset (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = to_usb_device (dev);
	int			config, value;

	if ((value = sscanf (buf, "%u", &config)) != 1 || config > udev->maxchild)
		return -EINVAL;

	if(udev->descriptor.bDeviceClass != USB_CLASS_HUB)
		return value;

	printk("Warm reset device ...\n");
	WarmReset_flag = 1;
	printk("#@# %s(%d) WarmReset_flag %d\n", __func__, __LINE__, WarmReset_flag);
	usb_reset_device(udev);
	printk("reset OK !!!\n");

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bWarmReset, S_IRUGO | S_IWUSR,
		show_bWarmReset, set_bWarmReset);

#include <linux/usb/hcd.h>
int gUsbHubSuspendResume[4] = {-1, -1, -1, -1}; // 0:Resume, 1:Suspend
EXPORT_SYMBOL(gUsbHubSuspendResume);
static int gUsbHubPortSuspendResume = 0; // 0:Resume, 1:Suspend

/*
 * USB 2.0 spec Section 11.24.2.7
 */
static int hub_get_port_status(struct usb_device *hdev, int port1, void *buf, int length)
{
	return usb_control_msg(hdev, usb_rcvctrlpipe(hdev, 0),
			GetPortStatus, USB_RT_PORT | USB_DIR_IN, 0, port1,
			buf, length, 1000);
}

static int hub_check_port_suspend_resume(struct usb_device *hdev, int port1)
{
	unsigned char data[4];
	int ret = -1;

	hub_get_port_status(hdev, port1, (void*)&data[0], sizeof data);
	ret = (data[0] >> 2) & 0x1; //bit 2

	return ret;
}

static ssize_t  show_bPortSuspendResume (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	if(udev->descriptor.bDeviceClass != USB_CLASS_HUB)
		return sprintf (buf, "%d\n", -1);

	if(udev->parent == NULL) // root hub
	{
		return sprintf (buf, "%d %d %d %d\n", gUsbHubSuspendResume[0], \
				gUsbHubSuspendResume[1], gUsbHubSuspendResume[2], gUsbHubSuspendResume[3]);
	}
	else // not root hub
	{
		gUsbHubPortSuspendResume = hub_check_port_suspend_resume(udev, gUsbHubPort);
		return sprintf (buf, "%d\n", gUsbHubPortSuspendResume);
	}

	return 0;
}

// copy from hub.c and rename
/*
 * USB 2.0 spec Section 11.24.2.2
 */
static int hub_clear_port_feature(struct usb_device *hdev, int port1, int feature)
{
	return usb_control_msg(hdev, usb_sndctrlpipe(hdev, 0),
			ClearPortFeature, USB_RT_PORT, feature, port1,
			NULL, 0, 1000);
}

/*
 * USB 2.0 spec Section 11.24.2.13
 */
static int hub_set_port_feature(struct usb_device *hdev, int port1, int feature)
{
	return usb_control_msg(hdev, usb_sndctrlpipe(hdev, 0),
			SetPortFeature, USB_RT_PORT, feature, port1,
			NULL, 0, 1000);
}

static ssize_t
set_bPortSuspendResume (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = to_usb_device (dev);
	struct usb_hcd		*hcd = container_of(udev->bus, struct usb_hcd, self);
	int			config, value;
	unsigned int		port1;

	if ((value = sscanf (buf, "%u", &config)) != 1 || config > UsbSuspendResume)
		return -EINVAL;

	if(udev->descriptor.bDeviceClass != USB_CLASS_HUB)
		return value;

	if(udev->parent == NULL) // root hub
	{
		port1 = (unsigned int) gUsbHubPort;
		if(gUsbHubSuspendResume[port1 - 1] != config)
		{
			gUsbHubSuspendResume[port1 - 1] = config;
			printk("Root Hub port %d - %s\n", port1, (gUsbHubSuspendResume[port1 - 1] == 1) ? "Suspend" : "Resume");
			if(gUsbHubSuspendResume[port1 - 1] == 1) //Suspend
			{
				usb_test_mode_suspend_flag = 1;

				printk("call bus_suspend() to the root hub port %d...\n", gUsbHubPort);
				usb_lock_device (hcd->self.root_hub);

				hcd->driver->bus_suspend(hcd);

				usb_unlock_device (hcd->self.root_hub);
				printk("call bus_suspend() OK !!!\n");

			}
			else //Resume
			{
				usb_test_mode_suspend_flag = 0;

				printk("call bus_resume() to the root hub port %d...\n", gUsbHubPort);
				usb_lock_device (hcd->self.root_hub);

				hcd->driver->bus_resume(hcd);

				usb_unlock_device (hcd->self.root_hub);
				printk("call bus_resume() OK !!!\n");

			}
			msleep(1000);
		}
	}
	else // the hub which is not root hub
	{
		gUsbHubPortSuspendResume = hub_check_port_suspend_resume(udev, gUsbHubPort);

		if(gUsbHubPortSuspendResume != config)
		{
			gUsbHubPortSuspendResume = config;
			if(gUsbHubPortSuspendResume == 1) //Suspend
			{
				printk("set USB_PORT_FEAT_SUSPEND to the port %d of the hub ...\n", gUsbHubPort);
				hub_set_port_feature(udev, gUsbHubPort, USB_PORT_FEAT_SUSPEND);
				printk("set OK !!!\n");
			}
			else //Resume
			{
				printk("clear USB_PORT_FEAT_SUSPEND to the port %d of the hub ...\n", gUsbHubPort);
				hub_clear_port_feature(udev, gUsbHubPort, USB_PORT_FEAT_SUSPEND);
				printk("clear OK !!!\n");
			}
			msleep(1000);
		}
	}

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bPortSuspendResume, S_IRUGO | S_IWUSR,
		show_bPortSuspendResume, set_bPortSuspendResume);

static ssize_t  show_bPortReset (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	if(udev->descriptor.bDeviceClass != USB_CLASS_HUB)
		return sprintf (buf, "%d\n", -1);

	return sprintf (buf, "%d\n", gUsbHubPort);

	return 0;
}

static ssize_t
set_bPortReset (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	int			config, value;

	if ((value = sscanf (buf, "%u", &config)) != 1 || config != 1)
		return -EINVAL;

	if(udev->descriptor.bDeviceClass != USB_CLASS_HUB)
		return value;


	printk("reset device ...\n");

	if(udev->parent == NULL) // root hub
	{
		if(udev->speed == USB_SPEED_SUPER) {
			usb_reset_device(udev);
		} else {
			hub_clear_port_feature(udev, gUsbHubPort, USB_PORT_FEAT_POWER);
			msleep(1000);
			hub_set_port_feature(udev, gUsbHubPort, USB_PORT_FEAT_POWER);
		}
	}
	else // not root hub
	{
		usb_reset_device(udev);
	}

	printk("reset OK !!!\n");

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bPortReset, S_IRUGO | S_IWUSR,
		show_bPortReset, set_bPortReset);

static ssize_t  show_bPortTestMode (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device *udev;
	struct usb_host_config *actconfig;

	udev = to_usb_device (dev);
	actconfig = udev->actconfig;

	if(udev->descriptor.bDeviceClass != USB_CLASS_HUB)
		return sprintf (buf, "%d\n", -1);

	return sprintf (buf, "%d\n", gUsbHubTestMode);

	return 0;
}

#define _inl(addr)			rtd_inl(addr)
#define _outl(value, addr)	rtd_outl(addr, value)

static int gUsbForceHsMode = 1; //default = 1

#include "../host/ehci.h"

static int usb_test_mode_ehci_interrupt = 0;
static int ehci_disable_interrupt_reg(struct ehci_hcd *ehci)
{
	if(usb_test_mode_ehci_interrupt == 0)
	{
		usb_test_mode_ehci_interrupt = ehci_readl(ehci, &ehci->regs->intr_enable);
		if(usb_test_mode_ehci_interrupt != 0)
		{
			ehci_writel(ehci, 0, &ehci->regs->intr_enable);
		}
	}

	return usb_test_mode_ehci_interrupt;
}

static int ehci_enable_interrupt_reg(struct ehci_hcd *ehci)
{
	if(usb_test_mode_ehci_interrupt != 0)
	{
		if (ehci_readl(ehci, &ehci->regs->intr_enable) == 0)
		{
			ehci_writel(ehci, usb_test_mode_ehci_interrupt, &ehci->regs->intr_enable);
		}
		usb_test_mode_ehci_interrupt = 0;
	}

	return usb_test_mode_ehci_interrupt;
}

static int ehci_root_hub_test_mode(struct usb_device *udev, int port1, int test_mode)
{
	struct usb_hcd *ehci_hcd = container_of(udev->bus, struct usb_hcd, self);
	struct ehci_hcd *ehci = hcd_to_ehci(ehci_hcd);
	unsigned int volatile tmp;
	unsigned int suspend_mux = 0;
	unsigned int port_address = 0;
	unsigned int usb_host_usbip_input_offset = 0;
	unsigned int usb_host_self_loop_back_offset = 0;

	//usb2 spec 4.14 p.114
	printk("Root Hub port %d, test mode = %d\n", port1, test_mode /*test[test_mode]*/);
	port_address = (unsigned int)ehci->regs + 0x44 + (port1 - 1) * 0x4;
	usb_host_usbip_input_offset = (unsigned int)ehci->caps + 0x808 + (port1 - 1) * 0x20;
	usb_host_self_loop_back_offset = (unsigned int)ehci->caps + 0x810 + (port1 - 1) * 0x20;


	if(0 != test_mode)
	{
		ehci_disable_interrupt_reg(ehci);
		mdelay(2000);
	}

	writel( readl(IOMEM(usb_host_usbip_input_offset)) | (1 << 27) | (1 << 30), \
			IOMEM(usb_host_usbip_input_offset));  // UTMI_Suspend_mux = 1
	suspend_mux = ((readl(IOMEM(usb_host_usbip_input_offset)) & (1 <<27)) == (1 <<27));
	printk("UTMI suspend mux (VENUS_USB_HOST_USBIP_INPUT bit27)= %d\n",suspend_mux);

	if (5 == test_mode){ // occupy this mode
		unsigned int volatile tmp1, tmp0;

		// Enter test mode TEST_FORCE_ENABLE (Fake)
		printk("Enter test mode TEST_FORCE_ENABLE (Fake)\n");
			printk("...Clear ASE = PSE = 0\n");
			ehci_writel(ehci,
					ehci_readl(ehci, &ehci->regs->command) & ~(CMD_ASE|CMD_PSE),
					&ehci->regs->command);
			printk("...Wait for ASS = PSS = 0");
			do{
				tmp0 = ehci_readl(ehci, &ehci->regs->status); // polling USBSTS
			} while ((unsigned int)(3 << 14) == ((unsigned int)(3 << 14) & tmp0));  // wait until ASS = PSS = 0
			printk("...ok\n");

		// detect device removal
		printk("Detect device removal\n");

			tmp0 = (3<<10);
			while(1){
				tmp1 = ehci_readl(ehci, &ehci->regs->port_status[port1-1]) & ~(3<<10);
				if (tmp0 != tmp1)
				{
					printk("ehci regs (port status) connect ? %s - 0x%.8x     \n",
					((1<<0) == (tmp1 & (1<<0))) ? "Yes" : "NO ", tmp1);
					tmp0 = tmp1;
				}
				if((1<<0) != (tmp1 & (1<<0)))
					break;
			}
			printk("...Set RUN = 0\n");
			ehci_writel(ehci, ehci_readl(ehci, &ehci->regs->command) & ~(CMD_RUN), &ehci->regs->command);
			return 0;
	}

	if(test_mode == 0)//HCReset
	{
		mdelay(2000);

		ehci_enable_interrupt_reg(ehci);

		//step 1
		tmp = ehci_readl(ehci, &ehci->regs->command);
		tmp |= (1<<1);
		ehci_writel(ehci, tmp, &ehci->regs->command);
		printk("Step 1: set Host reset ehci regs (command) = 0x%.8x -> 0x%.8x\n",tmp,ehci_readl(ehci, &ehci->regs->command));
		do
		{
			tmp = ehci_readl(ehci, &ehci->regs->command);
		} while (tmp & (1<<1));

		//step 2
		tmp = ehci_readl(ehci, &ehci->regs->command);
		tmp |= (1<<0);
		ehci_writel(ehci, tmp, &ehci->regs->command);

		ehci_hcd->driver->bus_resume(ehci_hcd);
		printk("Step 2: run ehci_resume(), ehci command regs = 0x%.8x\n",ehci_readl(ehci, &ehci->regs->command));

		tmp = ehci_readl(ehci, &ehci->regs->configured_flag);
		if(!tmp) {
			printk("Step 2: set CONFIG_FLAG\n");
			ehci_writel(ehci, 1, &ehci->regs->configured_flag);
			ehci_writel(ehci, 0x1000, &ehci->regs->port_status[0]);
			ehci_writel(ehci, 0x1000, &ehci->regs->port_status[1]);
		}

		mdelay(2000);

		ehci_enable_interrupt_reg(ehci);
	}
	else
	{
		//step 1 test mode procedure
		tmp = ehci_readl(ehci, &ehci->regs->command);
		tmp &= ~((1<<5) | (1<<4));
		ehci_writel(ehci, tmp, &ehci->regs->command);
		printk("Step 1: ehci regs (command) = 0x%.8x -> 0x%.8x\n",tmp,ehci_readl(ehci, &ehci->regs->command));

		if(suspend_mux)  // UTMI_Suspend_mux = 1
		{
			//step 2
			tmp = ehci_readl(ehci, &ehci->regs->command);
			tmp |= (1<<7);
			ehci_writel(ehci, tmp, &ehci->regs->command);
			printk("Step 2: ehci regs (port status) = 0x%.8x -> 0x%.8x\n",tmp,ehci_readl(ehci, &ehci->regs->port_status[port1-1]));
		}

		//step 3
		tmp = ehci_readl(ehci, &ehci->regs->command);
		tmp &= ~((1<<0));
		ehci_writel(ehci, tmp, &ehci->regs->command);

		// ensure HC_Halted = 1
		printk("Step 3: ehci regs (command) = 0x%.8x -> 0x%.8x\n",tmp,ehci_readl(ehci, &ehci->regs->command));

		//for delay 2mS
		{
			int i;
			for (i=0;i<3000;i++)
			{
				tmp = readl(IOMEM(ehci->caps));
			}
		}

		if(!suspend_mux)  // UTMI_Suspend_mux = 0
		{
			//step 2
			tmp = ehci_readl(ehci, &ehci->regs->port_status[port1-1]);
			tmp |= (1<<7);
			ehci_writel(ehci, tmp, &ehci->regs->port_status[port1-1]);
			printk("Step 2: ehci regs (port status) = 0x%.8x -> 0x%.8x\n",tmp,ehci_readl(ehci, &ehci->regs->port_status[port1-1]));
		}

		//step 4
		tmp = ehci_readl(ehci, &ehci->regs->port_status[port1-1]);
		tmp &= ~(0xf << 16);
		tmp |= test_mode << 16;
		ehci_writel(ehci, tmp, &ehci->regs->port_status[port1-1]);
		printk("Step 4: ehci regs (port status) = 0x%.8x -> 0x%.8x\n",tmp,ehci_readl(ehci, &ehci->regs->port_status[port1-1]));
	}

	return 0;
}

static ssize_t
set_bPortTestMode (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device	*udev = udev = to_usb_device (dev);
	int			config, value, i, prev_config = -1;
	unsigned int		port1 = 0;

	if ((value = sscanf (buf, "%u", &config)) != 1 || config > UsbTestModeNumber)
		return -EINVAL;

	if(udev->descriptor.bDeviceClass != USB_CLASS_HUB)
		return value;

	// there is no test mode 6,
	// it's just for toogle gUsbForceHsMode
	if(config == 6)
	{
		gUsbForceHsMode ? (gUsbForceHsMode = 0) : (gUsbForceHsMode = 1);
	}
	else
	{
		if(gUsbHubTestMode != config)
		{
			if(!((gUsbHubTestMode > 0) && (config > 0)))
			{
				prev_config = gUsbHubTestMode;
				gUsbHubTestMode = config;
				gUsbTestModeChanged = 1;
			}
		}
		else
		{
			//if(config == 0)
			//	gUsbTestModeChanged = 1;
		}
		port1 = (unsigned int) gUsbHubPort;
	}

	if(gUsbTestModeChanged == 1)
	{
		gUsbTestModeChanged = 0;

		if(config != 0)
		{
			printk("Call usb_lock_device().\n");
			usb_lock_device(udev);
		}

		if(udev->parent == NULL) // root hub
		{
			ehci_root_hub_test_mode(udev, port1, config);
		}
		else // not root hub
		{
			if(config == 0)
			{
				printk("Leave test mode ...\n");

				printk("clear USB_PORT_FEAT_POWER to the parent of the hub\n");
				for (i=1;i<=udev->parent->maxchild;i++)
				{
					printk("processing port %d of %d...\n", i, udev->parent->maxchild);
					hub_clear_port_feature(udev->parent, i, USB_PORT_FEAT_POWER);
					msleep(1000);
				}

				printk("set USB_PORT_FEAT_POWER to the parent of the hub\n");
				for (i=1;i<=udev->parent->maxchild;i++)
				{
					printk("processing port %d of %d...\n", i, udev->parent->maxchild);
					hub_set_port_feature(udev->parent, i, USB_PORT_FEAT_POWER);
					msleep(1000);
				}

				printk("Leave test mode , OK !!!\n");
			}
			else
			{
				printk("Enter test mode ...\n");

				printk("set USB_PORT_FEAT_SUSPEND to all ports of the hub\n");
				for (i=1;i<=udev->maxchild;i++)
				{
					printk("processing port %d of %d...\n", i, udev->maxchild);
					hub_set_port_feature(udev, i, USB_PORT_FEAT_SUSPEND);
					msleep(1000);
				}

				printk("set USB_PORT_FEAT_TEST mode %d to port %d ...\n", config, port1);
				hub_set_port_feature(udev,(config << 8) | port1, USB_PORT_FEAT_TEST);
				msleep(1000);

				printk("Enter test mode , OK !!!\n");
			}
		}

		if(config == 0)
		{
			printk("Call usb_unlock_device().\n");
			usb_unlock_device(udev);
		}
	}

	return (value < 0) ? value : count;
}

static DEVICE_ATTR(bPortTestMode, S_IRUGO | S_IWUSR,
		show_bPortTestMode, set_bPortTestMode);
#endif /* CONFIG_USB_XHCI_HCD_TEST_MODE */


static struct attribute *dev_attrs[] = {
	/* current configuration's attributes */
	&dev_attr_configuration.attr,
	&dev_attr_bNumInterfaces.attr,
	&dev_attr_bConfigurationValue.attr,
	&dev_attr_bmAttributes.attr,
	&dev_attr_bMaxPower.attr,
	/* device attributes */
	&dev_attr_urbnum.attr,
	&dev_attr_idVendor.attr,
	&dev_attr_idProduct.attr,
	&dev_attr_bcdDevice.attr,
	&dev_attr_bDeviceClass.attr,
	&dev_attr_bDeviceSubClass.attr,
	&dev_attr_bDeviceProtocol.attr,
	&dev_attr_bNumConfigurations.attr,
	&dev_attr_bMaxPacketSize0.attr,
	&dev_attr_speed.attr,
	&dev_attr_busnum.attr,
	&dev_attr_devnum.attr,
	&dev_attr_devpath.attr,
	&dev_attr_version.attr,
	&dev_attr_maxchild.attr,
	&dev_attr_quirks.attr,
	&dev_attr_avoid_reset_quirk.attr,
	&dev_attr_authorized.attr,
	&dev_attr_remove.attr,
	&dev_attr_removable.attr,
	&dev_attr_ltm_capable.attr,
#ifdef CONFIG_USB_XHCI_HCD_TEST_MODE
	&dev_attr_regs_addr.attr,
	&dev_attr_regs_value.attr,
	&dev_attr_bPortNumber.attr,
	&dev_attr_bPortDescriptor.attr,
	&dev_attr_bEnumBus.attr,
	&dev_attr_bWarmReset.attr,
	&dev_attr_bPortSuspendResume.attr,
	&dev_attr_bPortReset.attr,
	&dev_attr_bPortTestMode.attr,
#endif // CONFIG_USB_XHCI_HCD_TEST_MODE
	NULL,
};
static struct attribute_group dev_attr_grp = {
	.attrs = dev_attrs,
};

/* When modifying this list, be sure to modify dev_string_attrs_are_visible()
 * accordingly.
 */
static struct attribute *dev_string_attrs[] = {
	&dev_attr_manufacturer.attr,
	&dev_attr_product.attr,
	&dev_attr_serial.attr,
	NULL
};

static umode_t dev_string_attrs_are_visible(struct kobject *kobj,
		struct attribute *a, int n)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct usb_device *udev = to_usb_device(dev);

	if (a == &dev_attr_manufacturer.attr) {
		if (udev->manufacturer == NULL)
			return 0;
	} else if (a == &dev_attr_product.attr) {
		if (udev->product == NULL)
			return 0;
	} else if (a == &dev_attr_serial.attr) {
		if (udev->serial == NULL)
			return 0;
	}
	return a->mode;
}

static struct attribute_group dev_string_attr_grp = {
	.attrs =	dev_string_attrs,
	.is_visible =	dev_string_attrs_are_visible,
};

const struct attribute_group *usb_device_groups[] = {
	&dev_attr_grp,
	&dev_string_attr_grp,
	NULL
};

/* Binary descriptors */

static ssize_t
read_descriptors(struct file *filp, struct kobject *kobj,
		struct bin_attribute *attr,
		char *buf, loff_t off, size_t count)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct usb_device *udev = to_usb_device(dev);
	size_t nleft = count;
	size_t srclen, n;
	int cfgno;
	void *src;

	/* The binary attribute begins with the device descriptor.
	 * Following that are the raw descriptor entries for all the
	 * configurations (config plus subsidiary descriptors).
	 */
	for (cfgno = -1; cfgno < udev->descriptor.bNumConfigurations &&
			nleft > 0; ++cfgno) {
		if (cfgno < 0) {
			src = &udev->descriptor;
			srclen = sizeof(struct usb_device_descriptor);
		} else {
			src = udev->rawdescriptors[cfgno];
			srclen = __le16_to_cpu(udev->config[cfgno].desc.
					wTotalLength);
		}
		if (off < srclen) {
			n = min(nleft, srclen - (size_t) off);
			memcpy(buf, src + off, n);
			nleft -= n;
			buf += n;
			off = 0;
		} else {
			off -= srclen;
		}
	}
	return count - nleft;
}

static struct bin_attribute dev_bin_attr_descriptors = {
	.attr = {.name = "descriptors", .mode = 0444},
	.read = read_descriptors,
	.size = 18 + 65535,	/* dev descr + max-size raw descriptor */
};

int usb_create_sysfs_dev_files(struct usb_device *udev)
{
	struct device *dev = &udev->dev;
	int retval;

	retval = device_create_bin_file(dev, &dev_bin_attr_descriptors);
	if (retval)
		goto error;

	retval = add_persist_attributes(dev);
	if (retval)
		goto error;

	retval = add_power_attributes(dev);
	if (retval)
		goto error;
	return retval;
error:
	usb_remove_sysfs_dev_files(udev);
	return retval;
}

void usb_remove_sysfs_dev_files(struct usb_device *udev)
{
	struct device *dev = &udev->dev;

	remove_power_attributes(dev);
	remove_persist_attributes(dev);
	device_remove_bin_file(dev, &dev_bin_attr_descriptors);
}

/* Interface Accociation Descriptor fields */
#define usb_intf_assoc_attr(field, format_string)			\
static ssize_t								\
show_iad_##field(struct device *dev, struct device_attribute *attr,	\
		char *buf)						\
{									\
	struct usb_interface *intf = to_usb_interface(dev);		\
									\
	return sprintf(buf, format_string,				\
			intf->intf_assoc->field); 			\
}									\
static DEVICE_ATTR(iad_##field, S_IRUGO, show_iad_##field, NULL);

usb_intf_assoc_attr(bFirstInterface, "%02x\n")
usb_intf_assoc_attr(bInterfaceCount, "%02d\n")
usb_intf_assoc_attr(bFunctionClass, "%02x\n")
usb_intf_assoc_attr(bFunctionSubClass, "%02x\n")
usb_intf_assoc_attr(bFunctionProtocol, "%02x\n")

/* Interface fields */
#define usb_intf_attr(field, format_string)				\
static ssize_t								\
show_##field(struct device *dev, struct device_attribute *attr,	\
		char *buf)						\
{									\
	struct usb_interface *intf = to_usb_interface(dev);		\
									\
	return sprintf(buf, format_string,				\
			intf->cur_altsetting->desc.field); 		\
}									\
static DEVICE_ATTR(field, S_IRUGO, show_##field, NULL);

usb_intf_attr(bInterfaceNumber, "%02x\n")
usb_intf_attr(bAlternateSetting, "%2d\n")
usb_intf_attr(bNumEndpoints, "%02x\n")
usb_intf_attr(bInterfaceClass, "%02x\n")
usb_intf_attr(bInterfaceSubClass, "%02x\n")
usb_intf_attr(bInterfaceProtocol, "%02x\n")

static ssize_t show_interface_string(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf;
	char *string;

	intf = to_usb_interface(dev);
	string = intf->cur_altsetting->string;
	barrier();		/* The altsetting might change! */

	if (!string)
		return 0;
	return sprintf(buf, "%s\n", string);
}
static DEVICE_ATTR(interface, S_IRUGO, show_interface_string, NULL);

static ssize_t show_modalias(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf;
	struct usb_device *udev;
	struct usb_host_interface *alt;

	intf = to_usb_interface(dev);
	udev = interface_to_usbdev(intf);
	alt = intf->cur_altsetting;

	return sprintf(buf, "usb:v%04Xp%04Xd%04Xdc%02Xdsc%02Xdp%02X"
			"ic%02Xisc%02Xip%02Xin%02X\n",
			le16_to_cpu(udev->descriptor.idVendor),
			le16_to_cpu(udev->descriptor.idProduct),
			le16_to_cpu(udev->descriptor.bcdDevice),
			udev->descriptor.bDeviceClass,
			udev->descriptor.bDeviceSubClass,
			udev->descriptor.bDeviceProtocol,
			alt->desc.bInterfaceClass,
			alt->desc.bInterfaceSubClass,
			alt->desc.bInterfaceProtocol,
			alt->desc.bInterfaceNumber);
}
static DEVICE_ATTR(modalias, S_IRUGO, show_modalias, NULL);

static ssize_t show_supports_autosuspend(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf;
	struct usb_device *udev;
	int ret;

	intf = to_usb_interface(dev);
	udev = interface_to_usbdev(intf);

	usb_lock_device(udev);
	/* Devices will be autosuspended even when an interface isn't claimed */
	if (!intf->dev.driver ||
			to_usb_driver(intf->dev.driver)->supports_autosuspend)
		ret = sprintf(buf, "%u\n", 1);
	else
		ret = sprintf(buf, "%u\n", 0);
	usb_unlock_device(udev);

	return ret;
}
static DEVICE_ATTR(supports_autosuspend, S_IRUGO, show_supports_autosuspend, NULL);

static struct attribute *intf_attrs[] = {
	&dev_attr_bInterfaceNumber.attr,
	&dev_attr_bAlternateSetting.attr,
	&dev_attr_bNumEndpoints.attr,
	&dev_attr_bInterfaceClass.attr,
	&dev_attr_bInterfaceSubClass.attr,
	&dev_attr_bInterfaceProtocol.attr,
	&dev_attr_modalias.attr,
	&dev_attr_supports_autosuspend.attr,
	NULL,
};
static struct attribute_group intf_attr_grp = {
	.attrs = intf_attrs,
};

static struct attribute *intf_assoc_attrs[] = {
	&dev_attr_iad_bFirstInterface.attr,
	&dev_attr_iad_bInterfaceCount.attr,
	&dev_attr_iad_bFunctionClass.attr,
	&dev_attr_iad_bFunctionSubClass.attr,
	&dev_attr_iad_bFunctionProtocol.attr,
	NULL,
};

static umode_t intf_assoc_attrs_are_visible(struct kobject *kobj,
		struct attribute *a, int n)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct usb_interface *intf = to_usb_interface(dev);

	if (intf->intf_assoc == NULL)
		return 0;
	return a->mode;
}

static struct attribute_group intf_assoc_attr_grp = {
	.attrs =	intf_assoc_attrs,
	.is_visible =	intf_assoc_attrs_are_visible,
};

const struct attribute_group *usb_interface_groups[] = {
	&intf_attr_grp,
	&intf_assoc_attr_grp,
	NULL
};

void usb_create_sysfs_intf_files(struct usb_interface *intf)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	struct usb_host_interface *alt = intf->cur_altsetting;

	if (intf->sysfs_files_created || intf->unregistering)
		return;

	if (!alt->string && !(udev->quirks & USB_QUIRK_CONFIG_INTF_STRINGS))
		alt->string = usb_cache_string(udev, alt->desc.iInterface);
	if (alt->string && device_create_file(&intf->dev, &dev_attr_interface))
		;	/* We don't actually care if the function fails. */
	intf->sysfs_files_created = 1;
}

void usb_remove_sysfs_intf_files(struct usb_interface *intf)
{
	if (!intf->sysfs_files_created)
		return;

	device_remove_file(&intf->dev, &dev_attr_interface);
	intf->sysfs_files_created = 0;
}

/* hcy modified below */
static struct delayed_work reset_delayed_work;
extern void reset_usb_func(struct work_struct *work);
extern bool freeze_khubd_stop;
static ssize_t bus_reset_store(struct bus_type *bus, const char *buf,
                                size_t count)
{
        unsigned long val;

        if (strict_strtoul(buf, 0, &val) < 0)
                return -EINVAL;

        if (val) {
		if (!freeze_khubd_stop) {		
               		INIT_DELAYED_WORK(&reset_delayed_work, reset_usb_func);
                	schedule_delayed_work(&reset_delayed_work, msecs_to_jiffies(1));
			freeze_khubd_stop = 1;
		}
		else {
			printk(KERN_ERR "please wait seconds and try again ..... !! \n"); 		
			return -EINVAL;
		}

        }
        return count;
}


struct bus_attribute usb_gbus_attrs[] = {
        __ATTR(reset, (S_IWUSR|S_IWGRP), NULL, bus_reset_store),
        __ATTR_NULL
};

