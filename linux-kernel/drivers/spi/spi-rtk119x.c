/*
 * Realtek SPI driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>

#define SPI_CONTROL		0x0
	#define SPI_CLK_DIV(n)		((fls(n)-1)<<16)

#define SPI_CS_TIMING	0x4
	#define TCS_high(n)		(n<<16)
	#define TCS_end(n)		(n<<8)
	#define TCS_start(n)	(n<<0)
	#define DEFAULT_TIMING	(TCS_high(0)|TCS_end(0x10)|TCS_start(0))

#define SPI_AUX_CTRL	0x8
	#define CS0_EN			(1<<0)
	#define CS1_EN			(1<<1)
	#define MASTER_MODE_EN	(0<<8)
	#define SLAVE_MODE_EN	(1<<8)
	#define DEFAULT_AUX		(CS0_EN | MASTER_MODE_EN)

#define SPI_CMD_DUMMY	0xC
	#define INST_BIT_LEN(n)	(n<<16)

#define SPI_SO_CTRL		0x10
	#define READ_CMD		(0<<31)
	#define WRITE_CMD		(1<<31)
	#define ADDR_BIT_LEN(n)	(n<<22)
	#define DATA_BIT_LEN(n)	(n<<0)

#define SPI_SI_CTRL		0x14
	#define RD_BIT_LEN(n)	(n<<0)

#define SPI_W_FIFO		0x18
#define SPI_R_FIFO		0x1C
#define SPI_INST		0x20
#define SPI_ADDR		0X24

#define SPI_CTRL		0x28
	#define CMD_ENABLE		(1<<0)

#define SPI_INT			0x30
	#define CMD_DONE		(1<<0)


#define RTK_SPI_RXDATA	0
#define RTK_SPI_TXDATA	4
#define RTK_SPI_STATUS	8
#define RTK_SPI_CONTROL	12
#define RTK_SPI_SLAVE_SEL	20

#define RTK_SPI_STATUS_ROE_MSK	0x8
#define RTK_SPI_STATUS_TOE_MSK	0x10
#define RTK_SPI_STATUS_TMT_MSK	0x20
#define RTK_SPI_STATUS_TRDY_MSK	0x40
#define RTK_SPI_STATUS_RRDY_MSK	0x80
#define RTK_SPI_STATUS_E_MSK		0x100

#define RTK_SPI_CONTROL_IROE_MSK	0x8
#define RTK_SPI_CONTROL_ITOE_MSK	0x10
#define RTK_SPI_CONTROL_ITRDY_MSK	0x40
#define RTK_SPI_CONTROL_IRRDY_MSK	0x80
#define RTK_SPI_CONTROL_IE_MSK	0x100
#define RTK_SPI_CONTROL_SSO_MSK	0x400

#define RTK_SPI_DUMMY_LEN 0
#define RTK_SPI_MAX_WRITE_LEN 17
#define RTK_SPI_MAX_READ_LEN 16

static int cs_ctrl_gpio = -1;

#if 0
	#define RTK_SPI_DBG(args...) printk(args)
#else
	#define RTK_SPI_DBG(args...)
#endif

struct rtk119x_spi {
	/* bitbang has to be first */
	struct spi_bitbang bitbang;
	struct completion done;

	void __iomem *base;
	int irq;
	int len;
	int count;
	int bytes_per_word;
	unsigned long imr;

	/* data buffers */
	const unsigned char *tx;
	unsigned char *rx;
};

static inline struct rtk119x_spi *rtk119x_spi_to_hw(struct spi_device *sdev)
{
	return spi_master_get_devdata(sdev->master);
}


static void rtk119x_spi_chipsel(struct spi_device *spi, int value)
{
	unsigned int cspol;
	cspol = spi->mode & SPI_CS_HIGH ? 1 : 0;
	if (value == BITBANG_CS_INACTIVE)
		cspol = !cspol;

	RTK_SPI_DBG("chipsel(%d), set GPIO%d value(%u)\n", value, cs_ctrl_gpio, cspol);
	gpio_set_value(cs_ctrl_gpio, cspol);

}


static int rtk119x_spi_setupxfer(struct spi_device *spi, struct spi_transfer *t)
{
	return 0;
}

static int rtk119x_spi_setup(struct spi_device *spi)
{
	return 0;
}

static void rtk119x_spi_write(struct rtk119x_spi *hw)
{	
	unsigned int txd,temp,i,j;
	unsigned int write_len,data_byte_len,fifo_data_byte_len,offset=0;

	RTK_SPI_DBG("%s len(%u)\n", __func__,hw->len);

	write_len = hw->len;

	if((write_len<1) || (hw->tx==NULL))
		return;

	while(write_len)
	{
		if(write_len>=RTK_SPI_MAX_WRITE_LEN)
			data_byte_len = RTK_SPI_MAX_WRITE_LEN;
		else
			data_byte_len = write_len;

		fifo_data_byte_len = data_byte_len-1;

		if(data_byte_len==1)//Write FIFO can't be empty
		{
			writel((INST_BIT_LEN(0x4) | RTK_SPI_DUMMY_LEN), hw->base + SPI_CMD_DUMMY);//tr
			writel((ADDR_BIT_LEN(0x0) | WRITE_CMD | DATA_BIT_LEN(0x4)), hw->base + SPI_SO_CTRL);
			RTK_SPI_DBG("%s tx[%u] 0x%02x\n", __func__,offset,hw->tx[offset]);
			RTK_SPI_DBG("%s write 0x%08x to INST\n", __func__,hw->tx[offset]>>4);
			RTK_SPI_DBG("%s write 0x%08x to FIFO\n", __func__,hw->tx[offset]&0xF);
			writel((hw->tx[offset]>>4), hw->base + SPI_INST);//4  //4bit
			writel((hw->tx[offset]&0xF), hw->base + SPI_W_FIFO);//
		}
		else
		{
			writel((INST_BIT_LEN(0x8) | RTK_SPI_DUMMY_LEN), hw->base + SPI_CMD_DUMMY);//tr
			writel((ADDR_BIT_LEN(0x0) | WRITE_CMD | DATA_BIT_LEN(fifo_data_byte_len<<3)), hw->base + SPI_SO_CTRL);
			RTK_SPI_DBG("%s tx[%u] 0x%02x\n", __func__,offset,hw->tx[offset]);
			RTK_SPI_DBG("%s write 0x%08x to INST\n", __func__,hw->tx[offset]);
			writel(hw->tx[offset], hw->base + SPI_INST);//First byte
		}
		writel(RD_BIT_LEN(0x0), hw->base + SPI_SI_CTRL);//Note:read bit len cat't be 0x20

		//Write FIFO
		txd = 0;
		j=1;
		for(i=fifo_data_byte_len+offset; i>=offset+1; i--)
		{
			temp = hw->tx[i]<<(8*(j-1));
			txd = txd + temp;
			RTK_SPI_DBG("%s tx[%u] 0x%02x\n", __func__,i,hw->tx[i]);
			if(j==4)
			{
				RTK_SPI_DBG("%s write 0x%08x to FIFO\n", __func__,txd);
				writel(txd, hw->base + SPI_W_FIFO);
				txd = 0;
				j=1;
				continue;
			}
			else if(i==(offset+1))
			{
				RTK_SPI_DBG("%s write 0x%08x to FIFO\n", __func__,txd);
				writel(txd, hw->base + SPI_W_FIFO);
			}
			j++;
		}

		writel(0x100, hw->base + SPI_CTRL); // reset r/w fifo
		writel(CMD_ENABLE, hw->base + SPI_CTRL);

		while (!(readl(hw->base + SPI_INT) & CMD_DONE))
			cpu_relax();

		writel(CMD_DONE, hw->base + SPI_INT);// clear
		offset += RTK_SPI_MAX_WRITE_LEN;
		write_len = write_len - data_byte_len;
	}

	return;

}

static void rtk119x_spi_read(struct rtk119x_spi *hw)
{	
	unsigned int rxd,i;
	unsigned int read_len,data_byte_len,offset=0;

	RTK_SPI_DBG("%s len(%u)\n", __func__,hw->len);

	read_len = hw->len;

	while(read_len)
	{
		if(read_len>=RTK_SPI_MAX_READ_LEN)
			data_byte_len = RTK_SPI_MAX_READ_LEN;
		else
			data_byte_len = read_len;

		if(data_byte_len!=4)
		{
			writel((INST_BIT_LEN(0x0) | RTK_SPI_DUMMY_LEN), hw->base + SPI_CMD_DUMMY);//tr
			writel((ADDR_BIT_LEN(0x0) | READ_CMD), hw->base + SPI_SO_CTRL);
			writel(0x100, hw->base + SPI_CTRL); // reset r/w fifo
			writel(RD_BIT_LEN(data_byte_len<<3), hw->base + SPI_SI_CTRL);//Note:read bit len cat't be 0x20
			writel(CMD_ENABLE, hw->base + SPI_CTRL);
			RTK_SPI_DBG("%s READ CMD, read len(%u)\n", __func__,data_byte_len);
			while (!(readl(hw->base + SPI_INT) & CMD_DONE))
				cpu_relax();
			writel(CMD_DONE, hw->base + SPI_INT);

			//Read FIFO
			rxd = readl(hw->base + SPI_R_FIFO);
			RTK_SPI_DBG("%s read 0x%08x from FIFO\n", __func__,rxd);
			for(i=1; i<=data_byte_len; i++)
			{
				hw->rx[offset+data_byte_len-i] = rxd&0xff;
				RTK_SPI_DBG("%s rx[%u] 0x%02x\n", __func__,offset+data_byte_len-i,hw->rx[offset+data_byte_len-i]);
				rxd = rxd >>8;
				if(i%4==0&&(i<data_byte_len))
				{
					rxd = readl(hw->base + SPI_R_FIFO);
					RTK_SPI_DBG("%s read 0x%08x from FIFO\n", __func__,rxd);
				}
			}
			offset += RTK_SPI_MAX_READ_LEN;
		}
		else//Read data byte is 4
		{
				//Read 2 bytes
				rxd = 0;
				writel((INST_BIT_LEN(0x0) | RTK_SPI_DUMMY_LEN), hw->base + SPI_CMD_DUMMY);//tr
				writel((ADDR_BIT_LEN(0x0) | READ_CMD), hw->base + SPI_SO_CTRL);
				writel(0x100, hw->base + SPI_CTRL); // reset r/w fifo
				writel(RD_BIT_LEN(2<<3), hw->base + SPI_SI_CTRL);//Note:read bit len cat't be 0x20
				writel(CMD_ENABLE, hw->base + SPI_CTRL);
				RTK_SPI_DBG("%s READ CMD, read len(%u)\n", __func__,2);
				while (!(readl(hw->base + SPI_INT) & CMD_DONE))
					cpu_relax();
				writel(CMD_DONE, hw->base + SPI_INT);
				rxd = readl(hw->base + SPI_R_FIFO);
				RTK_SPI_DBG("%s read 0x%08x from FIFO\n", __func__,rxd);

				hw->rx[(hw->len)-offset-3] = rxd&0xff;
				RTK_SPI_DBG("%s rx[%u] 0x%02x\n", __func__,(hw->len)-offset-3,hw->rx[(hw->len)-offset-3]);
				rxd = rxd >>8;
				hw->rx[(hw->len)-offset-4] = rxd&0xff;
				RTK_SPI_DBG("%s rx[%u] 0x%02x\n", __func__,(hw->len)-offset-4,hw->rx[(hw->len)-offset-4]);

				//Read 2 bytes
				rxd = 0;
				writel((INST_BIT_LEN(0x0) | RTK_SPI_DUMMY_LEN), hw->base + SPI_CMD_DUMMY);//tr
				writel((ADDR_BIT_LEN(0x0) | READ_CMD), hw->base + SPI_SO_CTRL);
				writel(0x100, hw->base + SPI_CTRL); // reset r/w fifo
				writel(RD_BIT_LEN(2<<3), hw->base + SPI_SI_CTRL);//Note:read bit len cat't be 0x20
				writel(CMD_ENABLE, hw->base + SPI_CTRL);
				RTK_SPI_DBG("%s READ CMD, read len(%u)\n", __func__,2);
				while (!(readl(hw->base + SPI_INT) & CMD_DONE))
					cpu_relax();
				writel(CMD_DONE, hw->base + SPI_INT);
				rxd = readl(hw->base + SPI_R_FIFO);
				RTK_SPI_DBG("%s read 0x%08x from FIFO\n", __func__,rxd);

				hw->rx[(hw->len)-offset-1] = rxd&0xff;
				RTK_SPI_DBG("%s rx[%u] 0x%02x\n", __func__,(hw->len)-offset-1,hw->rx[(hw->len)-offset-1]);
				rxd = rxd >>8;
				hw->rx[(hw->len)-offset-2] = rxd&0xff;
				RTK_SPI_DBG("%s rx[%u] 0x%02x\n", __func__,(hw->len)-offset-2,hw->rx[(hw->len)-offset-2]);
				offset += 4;
		}

		read_len = read_len - data_byte_len;
	}

	return;

}

static int rtk119x_spi_txrx(struct spi_device *spi, struct spi_transfer *t)
{
	struct rtk119x_spi *hw = rtk119x_spi_to_hw(spi);
	struct spi_transfer *transfer;
	
	transfer = t;

	hw->tx = transfer->tx_buf;
	hw->rx = transfer->rx_buf;
	hw->len = transfer->len;

	if(hw->tx!=NULL)
		rtk119x_spi_write(hw);
	else if(hw->rx!=NULL)
		rtk119x_spi_read(hw);

	return hw->len;

}


static int rtk119x_spi_probe(struct platform_device *pdev)
{
	struct rtk119x_spi_platform_data *platp = pdev->dev.platform_data;
	struct rtk119x_spi *hw;
	struct spi_master *master;
	struct device_node *p_cs_ctrl_nd;

	int err = -ENODEV;

	master = spi_alloc_master(&pdev->dev, sizeof(struct rtk119x_spi));
	if (!master)
		return err;

	/* enable clk and release reset module */
	writel(readl(IOMEM(0xfe000000))|BIT(3), IOMEM(0xfe000000));	// release reset
	writel(readl(IOMEM(0xfe00000c))|BIT(3), IOMEM(0xfe00000c));	// enable clk

	/* setup the master state. */
	master->bus_num = 0;
	master->num_chipselect = 16;
	master->mode_bits = SPI_MODE_0;
	master->setup = rtk119x_spi_setup;

	hw = spi_master_get_devdata(master);
	platform_set_drvdata(pdev, hw);

	/* setup the state for the bitbang driver */
	hw->bitbang.master = spi_master_get(master);
	if (!hw->bitbang.master)
		return err;
	hw->bitbang.setup_transfer = rtk119x_spi_setupxfer;
	hw->bitbang.chipselect = rtk119x_spi_chipsel;
	hw->bitbang.txrx_bufs = rtk119x_spi_txrx;

	/* find and map our resources */
	hw->base = of_iomap(pdev->dev.of_node, 0);
	if (!hw->base)
		goto exit_busy;
	writel(SPI_CLK_DIV(16) | 2, hw->base + SPI_CONTROL);//Clock 15.xMHz
	writel(DEFAULT_TIMING,  hw->base + SPI_CS_TIMING);
	writel(DEFAULT_AUX,     hw->base + SPI_AUX_CTRL);

	/* Switch CS pin contrl to IGPIO2, and request IGPIO2 */
	writel((readl(IOMEM(0xfe000370))&0xFFFFF000)|0x864, IOMEM(0xfe000370));
	p_cs_ctrl_nd = of_find_compatible_node(NULL, NULL, "Realtek,rtk119x-spi");
	if (p_cs_ctrl_nd  && of_device_is_available(p_cs_ctrl_nd )) {
		cs_ctrl_gpio = of_get_named_gpio(p_cs_ctrl_nd , "spi-cs-gpio", 0);
		if (!gpio_is_valid(cs_ctrl_gpio))
		{
			RTK_SPI_DBG("%s ERROR spi-cs-gpio gpio is not valid\n",__func__);
		}
		if(gpio_request(cs_ctrl_gpio, p_cs_ctrl_nd ->name))
		{
			RTK_SPI_DBG("%s ERROR Request spi-cs-gpio fail\n",__func__);
		}
		else
			RTK_SPI_DBG("%s  spi-cs-gpio sucess\n",__func__);
	}
	else
		RTK_SPI_DBG("%s  DTS spi_cs_ctrl not found\n",__func__);

	/* find platform data */
	if (!platp)
		hw->bitbang.master->dev.of_node = pdev->dev.of_node;

	/* register our spi controller */
	err = spi_bitbang_start(&hw->bitbang);
	if (err)
		goto exit;
	dev_info(&pdev->dev, "base %p, irq %d\n", hw->base, hw->irq);
	return 0;

exit_busy:
	err = -EBUSY;
exit:
	platform_set_drvdata(pdev, NULL);
	spi_master_put(master);
	return err;
}

static int rtk119x_spi_remove(struct platform_device *dev)
{
	struct rtk119x_spi *hw = platform_get_drvdata(dev);
	struct spi_master *master = hw->bitbang.master;

	spi_bitbang_stop(&hw->bitbang);
	platform_set_drvdata(dev, NULL);
	spi_master_put(master);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id rtk119x_spi_match[] = {
	{ .compatible = "Realtek,rtk119x-spi", },
	{},
};
MODULE_DEVICE_TABLE(of, rtk119x_spi_match);
#endif /* CONFIG_OF */

static struct platform_driver rtk119x_spi_driver = {
	.probe = rtk119x_spi_probe,
	.remove = rtk119x_spi_remove,
	.driver = {
		.name = "rtk119x-spi",
		.owner = THIS_MODULE,
		.pm = NULL,
		.of_match_table = of_match_ptr(rtk119x_spi_match),
	},
};
module_platform_driver(rtk119x_spi_driver);

MODULE_DESCRIPTION("Realtek SPI driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:rtk119x-spi");
