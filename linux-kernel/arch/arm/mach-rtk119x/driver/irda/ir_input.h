#ifndef __IR_INPUT_H
#define __IR_INPUT_H

#include <linux/input.h>
#include <linux/slab.h>
#include <linux/types.h>

struct venus_ir_data {
	unsigned int irq;
	struct input_dev *input_dev;
	u8 keypressed;
	u32 keycode;
    atomic_t reportCount;
};

int venus_ir_input_init(void);
void venus_ir_input_cleanup(void);
void venus_ir_input_report_key(uint32_t keycode, bool* p_CursorEnable);
void venus_ir_input_report_end(bool* p_CursorEnable);

#endif
