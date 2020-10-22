#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/rfkill.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>

#include <mach/rtkbt_rfkill.h>

#define VERSION "TV 1.0"

static enum rfkill_user_states btrfk_state = RFKILL_USER_STATE_SOFT_BLOCKED;

static int bt_set_block(void *data, bool blocked)
{
	int rc = 0;
	struct rtkbt_platform_data *pdata = data;

	if(btrfk_state == RFKILL_USER_STATE_HARD_BLOCKED)
		return 0;

	if (blocked) {
		if (btrfk_state != RFKILL_USER_STATE_SOFT_BLOCKED) {
			if (pdata->poweroff != NULL)
				rc = pdata->poweroff();
		}
		btrfk_state = RFKILL_USER_STATE_SOFT_BLOCKED;
	} else {
		if (btrfk_state != RFKILL_USER_STATE_UNBLOCKED) {
			if (pdata->poweron != NULL)
				rc = pdata->poweron();
		}
		btrfk_state = RFKILL_USER_STATE_UNBLOCKED;
	}

	return rc;
}

static irqreturn_t bt2host_wakeup_irq(int irq, void *dev_id)
{
	struct wake_lock *bt2host_wakelock = dev_id;

	wake_lock_timeout(bt2host_wakelock, 50*HZ);

	return IRQ_HANDLED;
}

static struct rfkill_ops bt_rfkill_ops = {
	.set_block = bt_set_block,
};

static int rfkill_bluetooth_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct rfkill *bt_rfk;
	struct rtkbt_platform_data *pdata = pdev->dev.platform_data;
	int irq = 0;
	struct wake_lock bt2host_wakelock;

	/* Bluetooth BT Wake AP */
	if (pdata->bt_wake_ap != NULL) {
		irq = pdata->bt_wake_ap();

		wake_lock_init(&bt2host_wakelock, WAKE_LOCK_SUSPEND, "bt2host_wakeup");

		rc = request_irq(irq, bt2host_wakeup_irq, IRQF_TRIGGER_FALLING | IRQF_SHARED,
					"bt2host_wakeup", &bt2host_wakelock);
		if (rc) {
			printk(KERN_ERR "bt2host wakeup irq request failed!\n");
			wake_lock_destroy(&bt2host_wakelock);
			return -ENOMEM;
		}

		rc = enable_irq_wake(irq);
		if (rc < 0) {
			printk("Couldn't enable irq to wake up host\n");
			free_irq(irq, NULL);
			return -ENOMEM;
		}
	}

	/* Bluetooth AP Wake BT */
	if (pdata->ap_wake_bt != NULL)
		pdata->ap_wake_bt(1);

	bt_rfk = rfkill_alloc(pdata->name, &pdev->dev, RFKILL_TYPE_BLUETOOTH, &bt_rfkill_ops, pdata);
	if (!bt_rfk) {
		rc = -ENOMEM;
		goto err_rfkill_alloc;
	}

	/* userspace cannot take exclusive control */
	rfkill_init_sw_state(bt_rfk, false);

	rc = rfkill_register(bt_rfk);
	if (rc)
		goto err_rfkill_reg;

	rfkill_set_sw_state(bt_rfk, true);
	platform_set_drvdata(pdev, bt_rfk);

	return 0;

err_rfkill_reg:
        rfkill_destroy(bt_rfk);
err_rfkill_alloc:
        return rc;
}

int rfkill_bluetooth_remove(struct platform_device * pdev)
{
	struct rfkill *bt_rfk = platform_get_drvdata(pdev);
	struct rtkbt_platform_data *pdata = pdev->dev.platform_data;
	int irq = 0;

	if(bt_rfk)
		rfkill_destroy(bt_rfk);

	/* Bluetooth BT Wake AP */
	if (pdata->bt_wake_ap != NULL) {
		irq = pdata->bt_wake_ap();
		if(disable_irq_wake(irq))
			printk("Couldn't disable hostwake IRQ wakeup mode\n");
		free_irq(irq, NULL);
	}

	/* Bluetooth AP Wake BT */
	if (pdata->ap_wake_bt != NULL)
		pdata->ap_wake_bt(0);

	return 0;
}

int rfkill_bluetooth_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct rtkbt_platform_data *pdata = pdev->dev.platform_data;

	/* Bluetooth AP Wake BT */
	if (pdata->ap_wake_bt != NULL)
		pdata->ap_wake_bt(0);

	return 0;
}

int rfkill_bluetooth_resume(struct platform_device *pdev)
{
	struct rtkbt_platform_data *pdata = pdev->dev.platform_data;

	/* Bluetooth AP Wake BT */
	if (pdata->ap_wake_bt != NULL)
		pdata->ap_wake_bt(1);

	return 0;
}

static struct platform_driver rfkill_bluetooth_driver = {
	.probe = rfkill_bluetooth_probe,
	.remove = rfkill_bluetooth_remove,
	.suspend = rfkill_bluetooth_suspend,
	.resume = rfkill_bluetooth_resume,
	.driver = {
		.name = "rtkbt-rfkill",
		.owner = THIS_MODULE,
	},
};

static int __init rfkill_bluetooth_init(void)
{
	printk(KERN_INFO "\nRealtek Bluetooth RFKILL driver module init, version %s\n", VERSION);
	return platform_driver_register(&rfkill_bluetooth_driver);
}

static void __exit rfkill_bluetooth_exit(void)
{
	printk(KERN_INFO "\nRealtek Bluetooth RFKILL driver module exit.\n");
	platform_driver_unregister(&rfkill_bluetooth_driver);
}

late_initcall(rfkill_bluetooth_init);
module_exit(rfkill_bluetooth_exit);

MODULE_DESCRIPTION("Realtek Bluetooth rfkill driver version");
MODULE_AUTHOR("Realtek Corporation <derek_ju@realsil.com.cn>");
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");
