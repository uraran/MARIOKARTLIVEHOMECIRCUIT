#include "ir_input.h"
#include "venus_ir.h"
#include <linux/pinctrl/consumer.h>

static struct venus_ir_data *data = NULL;
long int Input_Keybit_Length = sizeof(data->input_dev->keybit); 

static int _venus_ir_setkeycode(struct input_dev *dev, const struct input_keymap_entry *ke, unsigned int *old_keycode);
static int _venus_ir_getkeycode(struct input_dev *dev, struct input_keymap_entry *ke);

int venus_ir_input_init(void)
{
	struct input_dev *input_dev;
	int result;
//	struct pinctrl *pinctrl;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		result = -ENOMEM;
		goto exit;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		result = -ENOMEM;
		printk(KERN_ERR "venus IR: can't allocate input device.\n");
		goto fail_alloc_input_dev;
	}

    atomic_set(&data->reportCount, 0);

	data->input_dev = input_dev;

//	data->input_dev->evbit[0] = BIT_MASK(EV_KEY) |BIT(EV_REL) |BIT(EV_REP);
	data->input_dev->evbit[0] = BIT_MASK(EV_KEY) |BIT(EV_REL);
	data->input_dev->name = "venus_IR_input";
	data->input_dev->phys = "venus/input0";
	
	data->input_dev->setkeycode = _venus_ir_setkeycode;
	data->input_dev->getkeycode = _venus_ir_getkeycode;

	venus_ir_set_keybit(data->input_dev->keybit);
	venus_ir_set_relbit(data->input_dev->relbit);

	result = input_register_device(data->input_dev);
	if (result) {
		printk(KERN_ERR "Venus IR: cannot register input device.\n");
		goto fail_register_input_dev;
	}
/*
	pinctrl = devm_pinctrl_get_select_default(&(data->input_dev->dev));
	if (IS_ERR(pinctrl)) {
		printk(KERN_ERR "Venus IR: pinctrl cannot select default state.\n");
		goto fail_pinctrl_select_default;
	}
*/
	return 0;
/*
fail_pinctrl_select_default:
	input_unregister_device(data->input_dev);
*/
fail_register_input_dev:
	input_free_device(data->input_dev);
fail_alloc_input_dev:
	kfree(data);
exit:
	return -1;
}

void venus_ir_input_cleanup(void)
{
	input_unregister_device(data->input_dev);
	kfree(data);
}

void venus_ir_input_report_key(uint32_t keycode, bool* p_CursorEnable)
{
    atomic_inc(&data->reportCount);
	data->keycode = keycode;

	RTK_debug(KERN_ALERT "[%s]%s  %d  report key %d\n",__FILE__,__FUNCTION__,__LINE__, keycode);

/*	if(data->keycode == KEY_TOUCHPAD_ON)
	{
		*p_GestureEnable = true;
		return;
	}
	else */

	/*
	if(data->keycode == KEY_TOUCHPAD_OFF)
	{
		*p_CursorEnable = (*p_CursorEnable + 1) % 2;
		return;
	}

	
	if(*p_CursorEnable)
	{
		if(data->keycode == KEY_RIGHT)
		{
			input_report_rel(data->input_dev, REL_X, 15);
			input_sync(data->input_dev);
		}
		else if(data->keycode == KEY_LEFT)
		{
			input_report_rel(data->input_dev, REL_X, -15);
			input_sync(data->input_dev);
		}
		else if(data->keycode == KEY_UP)
		{
			input_report_rel(data->input_dev, REL_Y, -15);
			input_sync(data->input_dev);
		}
		else if(data->keycode == KEY_DOWN)
		{
			input_report_rel(data->input_dev, REL_Y, 15);
			input_sync(data->input_dev);
		}
		else if(data->keycode == KEY_OK)
		{
			input_report_key(data->input_dev, BTN_LEFT, 1);
			input_sync(data->input_dev);
		}		
		else
		{
			input_report_key(data->input_dev, data->keycode, 1);
			input_sync(data->input_dev);
		}
	}else
	*/
	{
		input_report_key(data->input_dev, data->keycode, 1);
		input_sync(data->input_dev);
	}
	return;
}

void venus_ir_input_report_end(bool* p_CursorEnable)
{
    if (atomic_read(&data->reportCount) != 1)//Should not happen
        pr_err("[IR input] report key not in pairs, reportCount(%d)\n",atomic_read(&data->reportCount));

    atomic_set(&data->reportCount, 0);

	RTK_debug(KERN_ALERT "[%s]%s  %d  \n",__FILE__,__FUNCTION__,__LINE__);

	/*	
	if((data->keycode == KEY_TOUCHPAD_ON)||(data->keycode == KEY_TOUCHPAD_OFF))
		return;


	if(*p_CursorEnable)
	{
		if((data->keycode == KEY_RIGHT)||(data->keycode == KEY_LEFT)||(data->keycode == KEY_UP)||(data->keycode == KEY_DOWN))
		{
			//no report sync			
		}
		else if(data->keycode == KEY_OK)
		{
			input_report_key(data->input_dev, BTN_LEFT, 0);
			input_sync(data->input_dev);
		}		
		else
		{
			input_report_key(data->input_dev, data->keycode, 0);
			input_sync(data->input_dev);
		}
	}else
	*/
	{
		input_report_key(data->input_dev, data->keycode, 0);
		input_sync(data->input_dev);
	}
}

static int _venus_ir_setkeycode(struct input_dev *dev, const struct input_keymap_entry *ke, unsigned int *old_keycode)
{
	unsigned int scancode;
	unsigned int new_keycode;

	RTK_debug(KERN_ALERT "venus_ir_setkeycode\n");
	if (input_scancode_to_scalar(ke, &scancode))
		return -EINVAL;
	new_keycode = ke->keycode;

	venus_ir_setkeycode(scancode, new_keycode, old_keycode, data->input_dev->keybit);
	return 0;
}

static int _venus_ir_getkeycode(struct input_dev *dev, struct input_keymap_entry *ke)
{
	unsigned int scancode;
	unsigned int keycode;

	RTK_debug(KERN_ALERT "venus_ir_getkeycode\n");

	if (input_scancode_to_scalar(ke, &scancode))
		return -EINVAL;

	if (-1 != venus_ir_getkeycode(scancode, &keycode))
	{
		ke->keycode = keycode;
		ke->len = sizeof(scancode);
		memcpy(&ke->scancode, &scancode, sizeof(scancode));
		return 0;
	}

	RTK_debug(KERN_ALERT "[getkeycode]not found, return KEY_RESERVED for %d\n", scancode);

	keycode = KEY_RESERVED; //not found, set to KEY_RESERVED
	ke->keycode = keycode;
	ke->len = sizeof(scancode);
	memcpy(&ke->scancode, &scancode, sizeof(scancode));

	return 0;
}

