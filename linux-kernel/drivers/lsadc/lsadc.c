/*
 * drivers/lsadc/lsadc.c
 *
 * RTD1195 Low Speed ADC driver
 *
 * Copyright (c) 2014, Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

///////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/hwmon.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>

#include "lsadc.h"

/*
#define rtd_inb(offset)			(*(volatile unsigned char *)(offset))
#define rtd_inw(offset)			(*(volatile unsigned short *)(offset))
#define rtd_inl(offset)			(*(volatile unsigned long *)(offset))

#define rtd_outb(offset,val)	(*(volatile unsigned char *)(offset) = val)
#define rtd_outw(offset,val)	(*(volatile unsigned short *)(offset) = val)
#define rtd_outl(offset,val)	(*(volatile unsigned long *)(offset) = val)
*/
#define LSADC_IRQ_DEFINED 	(1)
#define SA_SHIRQ        IRQF_SHARED

struct rtd1195_lsadc_pad_info {
	uint			activate;	
	uint			ctrl_mode;
	uint			pad_sw;
	uint			threshold;
};

struct rtd1195_lsadc_device {
	struct device			*dev;
	struct rtd1195_lsadc_pad_info padInfoSet[2];
	uint					pad0_adc_val;
	uint					pad1_adc_val;
	uint					irq;
};

#ifdef LSADC_IRQ_DEFINED
static irqreturn_t lsadc_interrupt_pad(int irq, void *dev)
{
	struct rtd1195_lsadc_device *pc = dev;
	uint status_reg;
	uint pad0_reg, pad1_reg;
	uint lsadc_status_reg;
	uint pad0_int, pad1_int;
	uint new_adc_val=0;
	int interrupt_flag=0;

	interrupt_flag = MIS_ISR_MASK_LSADC_INT & readl(ioremap(MIS_ISR_REG_VADDR, 4)); 

	if(interrupt_flag) {
		status_reg = readl(ioremap(LSADC_STATUS_VADDR, 4));
		
		pad0_int = LSADC_STATUS_MASK_PAD0_STATUS & status_reg;
		pad1_int = LSADC_STATUS_MASK_PAD1_STATUS & status_reg;
		
		pad0_reg = readl(ioremap(LSADC_PAD0_VADDR, 4));
		pad1_reg = readl(ioremap(LSADC_PAD1_VADDR, 4));
		pr_err("--- lsadc_interrupt_pad :  pad0_adc=0x%0x, pad1_adc=0x%0x \n",pc->pad0_adc_val,pc->pad1_adc_val);

		if(pad0_int) {
			new_adc_val = pad0_reg & LSADC_PAD_MASK_ADC_VAL;
			pr_err(" lsadc_interrupt_pad : pad0 interrupt! status_reg=0x%x , adc_val=> from [%d] to [%d] \n", 
			status_reg, pc->pad0_adc_val,new_adc_val);
			if( pc->pad0_adc_val > new_adc_val ) {
				pr_err(" lsadc_interrupt_pad : pad0 adc became smaller! \n\n");
			}
			else {
				pr_err(" lsadc_interrupt_pad : pad0 adc became bigger! \n\n");
			}
			pc->pad0_adc_val=new_adc_val;
		}

		if(pad1_int) {
			new_adc_val = pad1_reg & LSADC_PAD_MASK_ADC_VAL;
			pr_err(" lsadc_interrupt_pad : pad1 interrupt! status_reg=0x%x , adc_val=> from [%d] to [%d] \n", 
			status_reg, pc->pad1_adc_val,new_adc_val);
			if( pc->pad1_adc_val > new_adc_val ) {
				pr_err(" lsadc_interrupt_pad : pad1 adc became smaller! \n\n");
			}
			else {
				pr_err(" lsadc_interrupt_pad : pad1 adc became bigger! \n\n");
			}
			pc->pad1_adc_val=new_adc_val;
		}
		
		status_reg = status_reg | ( LSADC_STATUS_MASK_PAD0_STATUS | LSADC_STATUS_MASK_PAD1_STATUS);  // reset INT flag
		writel(status_reg, ioremap(LSADC_STATUS_VADDR, 4)); 	

		return IRQ_HANDLED;
	}
	else {
		return IRQ_NONE;
	}
}
#endif

static ssize_t rtd1195_lsadc_show_info(struct device *dev, struct device_attribute
			      *devattr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);		
	struct rtd1195_lsadc_device *pc = platform_get_drvdata(pdev);	

	uint ctrl_reg;
	uint analog_ctrl_reg;
	uint pad0_reg, pad1_reg;
	uint lsadc_status_reg;
	uint pad0_set[5], pad1_set[5];
	char padBuffer[512], tmpBuf[64];
	int isr_value=0;
	int i=0;

	isr_value = readl(ioremap(MIS_ISR_REG_VADDR, 4));
	ctrl_reg = readl(ioremap(LSADC_CTRL_VADDR, 4));
	lsadc_status_reg = readl(ioremap(LSADC_STATUS_VADDR, 4));
	analog_ctrl_reg = readl(ioremap(LSADC_ANALOG_CTRL_VADDR, 4));

	if( (ctrl_reg & LSADC_CTRL_MASK_ENABLE) == 0 ) {
		ctrl_reg = ctrl_reg | LSADC_CTRL_MASK_ENABLE ;
		writel(ctrl_reg, ioremap(LSADC_CTRL_VADDR, 4));		
		pr_info("--- rtd1195_lsadc_show_info  :    write ctrl_enable	ctrl_reg=0x%x  ---  \n",ctrl_reg);
	}

	pad0_reg = readl(ioremap(LSADC_PAD0_VADDR, 4));
	pad1_reg = readl(ioremap(LSADC_PAD1_VADDR, 4));

	pc->pad0_adc_val = pad0_reg & LSADC_PAD_MASK_ADC_VAL;
	pc->pad1_adc_val = pad1_reg & LSADC_PAD_MASK_ADC_VAL;

	strcpy(padBuffer, "info:\n");
	for(i=0; i<5; i++) {
		pad0_set[i] = readl(ioremap(LSADC_PAD0_LEVEL_SET0_VADDR+(i*4), 4));
		pad1_set[i] = readl(ioremap(LSADC_PAD1_LEVEL_SET0_VADDR+(i*4), 4));
		sprintf(tmpBuf, "set_idx=%d:pad0_set=0x%0x pad1_set=0x%0x\n",i, pad0_set[i],pad1_set[i]);
		strcat(padBuffer, tmpBuf);
	}

	return sprintf(buf, "rtd1195_lsadc_show_info--\n isr_value=0x%x \n ctrl_reg=0x%x \n lsadc_status_reg=0x%x\n analog_ctrl_reg=0x%x\n pad0_reg=0x%x\n pad1_reg=0x%x \n pad0_adc=0x%x\n pad1_adc=0x%x  \n %s" ,
		isr_value, ctrl_reg ,lsadc_status_reg,analog_ctrl_reg ,pad0_reg,pad1_reg,pc->pad0_adc_val ,pc->pad1_adc_val ,padBuffer);
}

ssize_t rtd1195_lsadc_show_debounce(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);		
	//struct rtd1195_lsadc_device *pc = platform_get_drvdata(pdev);	

	uint ctrl_reg;
	uint lsadc_status_reg;
	int isr_value = 0 ;
	int debounce_cnt = 0;

	isr_value = readl(ioremap(MIS_ISR_REG_VADDR, 4));
	ctrl_reg = readl(ioremap(LSADC_CTRL_VADDR, 4));
	lsadc_status_reg = readl(ioremap(LSADC_STATUS_VADDR, 4));
	
	debounce_cnt = ( ctrl_reg & LSADC_CTRL_DEBOUNCE_MASK ) >> 20;
	
	pr_info("--- debug : rtd1195_lsadc_show_debounce : ctrl_reg=0x%x, isr_value=0x%x, lsadc_status_reg=0x%x \n    debounce_cnt=%d (0~15) ---- \n",
		ctrl_reg, isr_value, lsadc_status_reg, debounce_cnt);

	return sprintf(buf, "%d\n", debounce_cnt);
}

ssize_t rtd1195_lsadc_store_debounce(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)	 
{
	struct platform_device *pdev = to_platform_device(dev);		
	//struct rtd1195_lsadc_device *pc = platform_get_drvdata(pdev);	

	uint ctrl_reg;
	uint lsadc_status_reg;
	int isr_value = 0 ;
	int debounce_cnt = 0;
	int value=0;

	isr_value = readl(ioremap(MIS_ISR_REG_VADDR, 4));
	ctrl_reg = readl(ioremap(LSADC_CTRL_VADDR, 4));
	lsadc_status_reg = readl(ioremap(LSADC_STATUS_VADDR, 4));
	
	debounce_cnt = ( ctrl_reg & LSADC_CTRL_DEBOUNCE_MASK ) >> 20;
	
	if(buf ==NULL) {
		pr_err("--- debug : rtd1195_lsadc_store_debounce	====  buffer is null, return \n");
		return count;
	} 
	sscanf(buf, "%d", &value);

	pr_err("--- debug : rtd1195_lsadc_store_debounce : get value=%d \n", value);

	if( debounce_cnt == value ) {
		pr_err("--- debug : rtd1195_lsadc_store_debounce	====  the same, do nothing (value=%d) \n", value);
		return count;
	}
	if( debounce_cnt > 15 || debounce_cnt < 0 ) {
		pr_err("--- debug : rtd1195_lsadc_store_debounce	====  value (%d) out of range, (valid data => 0-15) \n", value);
		return count;
	}

	debounce_cnt = value;

	ctrl_reg =(ctrl_reg & (~LSADC_CTRL_DEBOUNCE_MASK))|(debounce_cnt << 20);
	writel(ctrl_reg, ioremap(LSADC_CTRL_VADDR, 4));		
	pr_err("--- debug : rtd1195_lsadc_store_debounce : write ctrl_reg=0x%x debounce_cnt=%d \n", ctrl_reg, debounce_cnt);
	
	isr_value = readl(ioremap(MIS_ISR_REG_VADDR, 4));
	ctrl_reg = readl(ioremap(LSADC_CTRL_VADDR, 4));
	lsadc_status_reg = readl(ioremap(LSADC_STATUS_VADDR, 4));
	
	pr_info("--- debug : rtd1195_lsadc_store_debounce : ctrl_reg=0x%x, isr_value=0x%x, lsadc_status_reg=0x%x \n    debounce_cnt=%d ---- \n",
		ctrl_reg, isr_value, lsadc_status_reg, debounce_cnt);

	return count;
}

static DEVICE_ATTR(debounce, S_IRWXUGO, rtd1195_lsadc_show_debounce, rtd1195_lsadc_store_debounce);
static DEVICE_ATTR(info, S_IRUGO, rtd1195_lsadc_show_info, NULL);

static struct attribute *rtd1195_attr_base[] = {
	&dev_attr_info.attr,
	&dev_attr_debounce.attr,
	NULL
};

static const struct attribute_group rtd1195_group_base = {
	.attrs = rtd1195_attr_base,
};

static int __init rtd1195_lsadc_probe(struct platform_device *pdev)
{
	struct rtd1195_lsadc_device *priv;
	struct device_node *pad_idx0_node, *pad_idx1_node;
	int ret, val;
	uint ctrl_reg;
	uint analog_ctrl_reg;
	uint pad0_reg, pad1_reg;
	uint lsadc_status_reg;
	uint irq_num;

	pr_info("--- debug : rtd1195_lsadc_probe \n");

	// Initial Ananlog_ctrl value to 0x00011101
	writel(LSADC_ANALOG_CTRL_VALUE, ioremap(LSADC_ANALOG_CTRL_VADDR, 4));		

	pad_idx0_node = of_get_child_by_name(pdev->dev.of_node, "rtd1195-lsadc-pad0");
	if (!pad_idx0_node) {
		dev_err(&pdev->dev, "could not find [rtd1195-lsadc-pad0] sub-node\n");
		return -EINVAL;
	}
	pad_idx1_node = of_get_child_by_name(pdev->dev.of_node, "rtd1195-lsadc-pad1");
	if (!pad_idx1_node) {
		dev_err(&pdev->dev, "could not find [rtd1195-lsadc-pad1] sub-node\n");
		return -EINVAL;
	}

	/* Request IRQ */
	irq_num = irq_of_parse_and_map(pdev->dev.of_node, 0);	

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&pdev->dev, "failed to allocate memory\n");
		return -ENOMEM;
	}

	priv->dev = &pdev->dev;
	priv->irq = irq_num;
	
	platform_set_drvdata(pdev, priv);

	// set pad0 from device tree : [pad_idx0]
	if (!of_property_read_u32(pad_idx0_node, "activate", &val))
		priv->padInfoSet[0].activate=val;

	if (!of_property_read_u32(pad_idx0_node, "ctrl_mode", &val))
		priv->padInfoSet[0].ctrl_mode=val;
		
	if (!of_property_read_u32(pad_idx0_node, "sw_idx", &val)){
		if(val == 1)
			priv->padInfoSet[0].pad_sw=val;
		else	
			priv->padInfoSet[0].pad_sw=0;
	}	

	if (!of_property_read_u32(pad_idx0_node, "voltage_threshold", &val))
		priv->padInfoSet[0].threshold=val;

	// set pad1 from device tree : [pad_idx1]
	if (!of_property_read_u32(pad_idx1_node, "activate", &val))
		priv->padInfoSet[1].activate=val;
	else
		priv->padInfoSet[1].activate=0;

	if (!of_property_read_u32(pad_idx1_node, "ctrl_mode", &val))
		priv->padInfoSet[1].ctrl_mode=val;

	if (!of_property_read_u32(pad_idx1_node, "sw_idx", &val)){
		if(val == 1)
			priv->padInfoSet[1].pad_sw=val;
		else	
			priv->padInfoSet[1].pad_sw=0;
	}	
	
	if (!of_property_read_u32(pad_idx1_node, "voltage_threshold", &val))
		priv->padInfoSet[1].threshold=val;

	ctrl_reg = readl(ioremap(LSADC_CTRL_VADDR, 4));
	analog_ctrl_reg = readl(ioremap(LSADC_ANALOG_CTRL_VADDR, 4));
	lsadc_status_reg = readl(ioremap(LSADC_STATUS_VADDR, 4));
	pad0_reg = readl(ioremap(LSADC_PAD0_VADDR, 4));
	pad1_reg = readl(ioremap(LSADC_PAD1_VADDR, 4));
	
	ctrl_reg = ctrl_reg | LSADC_CTRL_MASK_ENABLE | LSADC_CTRL_DEBOUNCE_CNT | LSADC_CTRL_MASK_SEL_WAIT ;
	writel(ctrl_reg, ioremap(LSADC_CTRL_VADDR, 4));		
	pr_err("--- debug :    rtd1195_lsadc_probe	ctrl_reg=0x%x  irq_num=%d---  \n",ctrl_reg, irq_num);

	pr_err("--- debug :   from device tree: pad0=[activate=%d, ctrl_mode=%d, pad_sw=%d, threshold=%d]---  \n",
			priv->padInfoSet[0].activate, priv->padInfoSet[0].ctrl_mode, priv->padInfoSet[0].pad_sw, priv->padInfoSet[0].threshold);
	pr_err("--- debug :   from device tree: pad1=[activate=%d, ctrl_mode=%d, pad_sw=%d, threshold=%d]---  \n\n",
			priv->padInfoSet[1].activate, priv->padInfoSet[1].ctrl_mode, priv->padInfoSet[1].pad_sw, priv->padInfoSet[1].threshold);

	pr_err("--- debug :  current value =   ctrl_reg=0x%x, lsadc_status_reg=0x%x, pad0_reg=0x%x, pad1_reg=0x%x ---- \n",
		ctrl_reg, lsadc_status_reg,pad0_reg,pad1_reg );

	if( priv->padInfoSet[0].activate==1) {
		pad0_reg = pad0_reg | LSADC_PAD_MASK_ACTIVE ;
		if( priv->padInfoSet[0].ctrl_mode == 1 ) 
			pad0_reg = pad0_reg | LSADC_PAD_MASK_CTRL;
		else
			pad0_reg = pad0_reg & ~(LSADC_PAD_MASK_CTRL);

		if( priv->padInfoSet[0].pad_sw == 1 ) 
			pad0_reg = pad0_reg | LSADC_PAD_MASK_SW;
		else
			pad0_reg = pad0_reg & ~(LSADC_PAD_MASK_SW);

		if(priv->padInfoSet[0].threshold <=0xFF && priv->padInfoSet[0].threshold >=0 )
			pad0_reg = pad0_reg | (LSADC_PAD_MASK_THRESHOLD & (priv->padInfoSet[0].threshold << 16 ));

		writel(pad0_reg, ioremap(LSADC_PAD0_VADDR, 4));	
		pr_info("--- debug :    write pad0_reg=0x%x ---  \n",pad0_reg);
	}
	if( priv->padInfoSet[1].activate==1) {
		pad1_reg = pad1_reg | LSADC_PAD_MASK_ACTIVE ;
		if( priv->padInfoSet[1].ctrl_mode == 1 ) 
			pad1_reg = pad1_reg | LSADC_PAD_MASK_CTRL;
		else
			pad1_reg = pad1_reg & ~(LSADC_PAD_MASK_CTRL);

		if( priv->padInfoSet[1].pad_sw == 1 ) 
			pad1_reg = pad1_reg | LSADC_PAD_MASK_SW;
		else
			pad1_reg = pad1_reg & ~(LSADC_PAD_MASK_SW);

		if(priv->padInfoSet[1].threshold <=0xFF && priv->padInfoSet[1].threshold >=0 )
			pad1_reg = pad1_reg | (LSADC_PAD_MASK_THRESHOLD & (priv->padInfoSet[1].threshold << 16 ));

		writel(pad1_reg, ioremap(LSADC_PAD1_VADDR, 4));	
		pr_info("--- debug :    write pad1_reg=0x%x  ---  \n",pad0_reg);
	}

	if( (lsadc_status_reg & LSADC_STATUS_MASK_IRQ_EN) != LSADC_STATUS_MASK_IRQ_EN ) {
		lsadc_status_reg = lsadc_status_reg | LSADC_STATUS_MASK_IRQ_EN ;
		writel(lsadc_status_reg, ioremap(LSADC_STATUS_VADDR, 4));	
		pr_info("--- debug :    write lsadc_status_reg=0x%x  --  \n\n",lsadc_status_reg);		
	}

#ifdef LSADC_IRQ_DEFINED

	if (request_irq(priv->irq, lsadc_interrupt_pad, IRQF_SHARED, "lsadc", priv)< 0) 
	{
		pr_err("--- debug : unable to request irq#%d\n", priv->irq);						
		goto err;
	}
	// Enable IRQ for pad0/pad1, set LSADC_STATUS to 0x0300000
	writel(LSADC_STATUS_ENABLE_IQR, ioremap(LSADC_STATUS_VADDR, 4));	
	
#endif // LSADC_IRQ_DEFINED

	ctrl_reg = readl(ioremap(LSADC_CTRL_VADDR, 4));
	lsadc_status_reg = readl(ioremap(LSADC_STATUS_VADDR, 4));
	analog_ctrl_reg = readl(ioremap(LSADC_ANALOG_CTRL_VADDR, 4));
	pad0_reg = readl(ioremap(LSADC_PAD0_VADDR, 4));
	pad1_reg = readl(ioremap(LSADC_PAD1_VADDR, 4));

	priv->pad0_adc_val = pad0_reg & LSADC_PAD_MASK_ADC_VAL;
	priv->pad1_adc_val = pad1_reg & LSADC_PAD_MASK_ADC_VAL;

	pr_err("--- debug :  set value =   ctrl_reg=0x%x, lsadc_status_reg=0x%x, pad0_reg=0x%x, pad1_reg=0x%x ---- \n",
		ctrl_reg, lsadc_status_reg,pad0_reg,pad1_reg );

	/* Register sysfs hooks */
	ret = sysfs_create_group(&pdev->dev.kobj, &rtd1195_group_base);	
	if (ret)
		goto out_err_register;

	return 0;

out_err_register:
	sysfs_remove_group(&pdev->dev.kobj, &rtd1195_group_base);
err:
	return ret;
}

static int rtd1195_lsadc_remove(struct platform_device *pdev)
{
	struct rtd1195_lsadc_device *priv = platform_get_drvdata(pdev);
	sysfs_remove_group(&pdev->dev.kobj, &rtd1195_group_base);
#ifdef LSADC_IRQ_DEFINED
	free_irq(priv->irq, priv);
#endif
	return 0;
}

static const struct of_device_id rtd1195_lsadc_of_match[] = {
	{ .compatible = "realtek,rtd1195-lsadc" },
	{ }
};
MODULE_DEVICE_TABLE(of, rtd1195_lsadc_of_match);


static struct platform_driver rtd1195_lsadc_platform_driver = {
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "rtd1195-lsadc",
		.of_match_table = rtd1195_lsadc_of_match,
	},
	.probe 		= rtd1195_lsadc_probe,
	.remove 	= rtd1195_lsadc_remove,
};
module_platform_driver(rtd1195_lsadc_platform_driver);

MODULE_DESCRIPTION("RTD1195 LSADC driver");
MODULE_AUTHOR("Toto Chen <l.totochen@realtek.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:rtd1195-lsadc");


