/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <mach/gpio.h>
#include <mach/board.h>
#include <mach/camera.h>
#include <mach/vreg.h>
#include <mach/camera.h>
#include <mach/clk.h>
#include <mach/msm_bus.h>
#include <mach/msm_bus_board.h>
#include <media/msm_isp.h>
#include "msm_ispif.h"
#include "msm.h"

#define V4L2_IDENT_ISPIF			50001

/* ISPIF registers */

#define ISPIF_RST_CMD_ADDR                        0X00
#define ISPIF_INTF_CMD_ADDR                       0X04
#define ISPIF_CTRL_ADDR                           0X08
#define ISPIF_INPUT_SEL_ADDR                      0X0C
#define ISPIF_PIX_INTF_CID_MASK_ADDR              0X10
#define ISPIF_RDI_INTF_CID_MASK_ADDR              0X14
#define ISPIF_PIX_1_INTF_CID_MASK_ADDR            0X38
#define ISPIF_RDI_1_INTF_CID_MASK_ADDR            0X3C
#define ISPIF_PIX_STATUS_ADDR                     0X24
#define ISPIF_RDI_STATUS_ADDR                     0X28
#define ISPIF_RDI_1_STATUS_ADDR                   0X64
#define ISPIF_IRQ_MASK_ADDR                     0X0100
#define ISPIF_IRQ_CLEAR_ADDR                    0X0104
#define ISPIF_IRQ_STATUS_ADDR                   0X0108
#define ISPIF_IRQ_MASK_1_ADDR                   0X010C
#define ISPIF_IRQ_CLEAR_1_ADDR                  0X0110
#define ISPIF_IRQ_STATUS_1_ADDR                 0X0114
#define ISPIF_IRQ_GLOBAL_CLEAR_CMD_ADDR         0x0124

/*ISPIF RESET BITS*/

#define VFE_CLK_DOMAIN_RST           31
#define RDI_CLK_DOMAIN_RST           30
#define PIX_CLK_DOMAIN_RST           29
#define AHB_CLK_DOMAIN_RST           28
#define RDI_1_CLK_DOMAIN_RST         27
#define RDI_1_VFE_RST_STB            13
#define RDI_1_CSID_RST_STB           12
#define RDI_VFE_RST_STB              7
#define RDI_CSID_RST_STB             6
#define PIX_VFE_RST_STB              4
#define PIX_CSID_RST_STB             3
#define SW_REG_RST_STB               2
#define MISC_LOGIC_RST_STB           1
#define STROBED_RST_EN               0

#define PIX_INTF_0_OVERFLOW_IRQ      12
#define RAW_INTF_0_OVERFLOW_IRQ      25
#define RAW_INTF_1_OVERFLOW_IRQ      25
#define RESET_DONE_IRQ               27

#define ISPIF_IRQ_STATUS_MASK        0xA493000
#define ISPIF_IRQ_1_STATUS_MASK      0xA493000
#define ISPIF_IRQ_STATUS_RDI_SOF_MASK	0x492000
#define ISPIF_IRQ_GLOBAL_CLEAR_CMD     0x1

#define MAX_CID 15


static struct ispif_device *ispif;

static uint32_t global_intf_cmd_mask = 0xFFFFFFFF;

static int msm_ispif_intf_reset(uint8_t intftype)
{
	int rc = 0;
	uint32_t data;

	switch (intftype) {
	case PIX0:
		data = (0x1 << STROBED_RST_EN) +
			(0x1 << PIX_VFE_RST_STB) +
			(0x1 << PIX_CSID_RST_STB);
		msm_io_w(data, ispif->base + ISPIF_RST_CMD_ADDR);
		break;

	case RDI0:
		data = (0x1 << STROBED_RST_EN) +
			(0x1 << RDI_VFE_RST_STB)  +
			(0x1 << RDI_CSID_RST_STB);
		msm_io_w(data, ispif->base + ISPIF_RST_CMD_ADDR);
		break;

	case RDI1:
		data = (0x1 << STROBED_RST_EN) +
			(0x1 << RDI_1_VFE_RST_STB) +
			(0x1 << RDI_1_CSID_RST_STB);
		msm_io_w(data, ispif->base + ISPIF_RST_CMD_ADDR);
		break;

	default:
		rc = -EINVAL;
		break;
	}
	if (rc >= 0)
		rc = wait_for_completion_interruptible(
				&ispif->reset_complete);

	return rc;
}

static int msm_ispif_reset(void)
{
	uint32_t data = (0x1 << STROBED_RST_EN) +
		(0x1 << SW_REG_RST_STB) +
		(0x1 << MISC_LOGIC_RST_STB) +
		(0x1 << PIX_VFE_RST_STB) +
		(0x1 << PIX_CSID_RST_STB) +
		(0x1 << RDI_VFE_RST_STB) +
		(0x1 << RDI_CSID_RST_STB) +
		(0x1 << RDI_1_VFE_RST_STB) +
		(0x1 << RDI_1_CSID_RST_STB);
	msm_io_w(data, ispif->base + ISPIF_RST_CMD_ADDR);
	return wait_for_completion_interruptible(&ispif->reset_complete);
}

static int msm_ispif_subdev_g_chip_ident(struct v4l2_subdev *sd,
			struct v4l2_dbg_chip_ident *chip)
{
	BUG_ON(!chip);
	chip->ident = V4L2_IDENT_ISPIF;
	chip->revision = 0;
	return 0;
}

static void msm_ispif_sel_csid_core(uint8_t intftype, uint8_t csid)
{
	uint32_t data;
	data = msm_io_r(ispif->base + ISPIF_INPUT_SEL_ADDR);
	data |= csid<<(intftype*4);
	msm_io_w(data, ispif->base + ISPIF_INPUT_SEL_ADDR);
}

static void msm_ispif_enable_intf_cids(uint8_t intftype, uint16_t cid_mask)
{
	uint32_t data;
	mutex_lock(&ispif->mutex);
	switch (intftype) {
	case PIX0:
		data = msm_io_r(ispif->base +
				ISPIF_PIX_INTF_CID_MASK_ADDR);
		data |= cid_mask;
		msm_io_w(data, ispif->base +
				ISPIF_PIX_INTF_CID_MASK_ADDR);
		break;

	case RDI0:
		data = msm_io_r(ispif->base +
				ISPIF_RDI_INTF_CID_MASK_ADDR);
		data |= cid_mask;
		msm_io_w(data, ispif->base +
				ISPIF_RDI_INTF_CID_MASK_ADDR);
		break;

	case RDI1:
		data = msm_io_r(ispif->base +
				ISPIF_RDI_1_INTF_CID_MASK_ADDR);
		data |= cid_mask;
		msm_io_w(data, ispif->base +
				ISPIF_RDI_1_INTF_CID_MASK_ADDR);
		break;
	}
	mutex_unlock(&ispif->mutex);
}

static int msm_ispif_config(struct msm_ispif_params_list *params_list)
{
	uint32_t params_len;
	struct msm_ispif_params *ispif_params;
	uint32_t data, data1;
	int rc = 0, i = 0;
	params_len = params_list->len;
	ispif_params = params_list->params;
	CDBG("Enable interface\n");
	data = msm_io_r(ispif->base + ISPIF_PIX_STATUS_ADDR);
	data1 = msm_io_r(ispif->base + ISPIF_RDI_STATUS_ADDR);
	if (((data & 0xf) != 0xf) || ((data1 & 0xf) != 0xf))
		return -EBUSY;
	msm_io_w(0x00000000, ispif->base + ISPIF_IRQ_MASK_ADDR);
	for (i = 0; i < params_len; i++) {
		msm_ispif_sel_csid_core(ispif_params[i].intftype,
			ispif_params[i].csid);
		msm_ispif_enable_intf_cids(ispif_params[i].intftype,
			ispif_params[i].cid_mask);
	}

	msm_io_w(ISPIF_IRQ_STATUS_MASK, ispif->base +
					ISPIF_IRQ_MASK_ADDR);
	msm_io_w(ISPIF_IRQ_STATUS_MASK, ispif->base +
					ISPIF_IRQ_CLEAR_ADDR);
	msm_io_w(ISPIF_IRQ_GLOBAL_CLEAR_CMD, ispif->base +
		 ISPIF_IRQ_GLOBAL_CLEAR_CMD_ADDR);
	return rc;
}

static long msm_ispif_subdev_ioctl(struct v4l2_subdev *sd, unsigned int cmd,
								void *arg)
{
	switch (cmd) {
	case VIDIOC_MSM_ISPSF_CFG:
		return msm_ispif_config((struct msm_ispif_params_list *)arg);
	default:
		return -ENOIOCTLCMD;
	}
}


static uint32_t msm_ispif_get_cid_mask(uint8_t intftype)
{
	uint32_t mask = 0;
	switch (intftype) {
	case PIX0:
		mask = msm_io_r(ispif->base +
			ISPIF_PIX_INTF_CID_MASK_ADDR);
		break;

	case RDI0:
		mask = msm_io_r(ispif->base +
			ISPIF_RDI_INTF_CID_MASK_ADDR);
		break;

	case RDI1:
		mask = msm_io_r(ispif->base +
			ISPIF_RDI_1_INTF_CID_MASK_ADDR);
		break;

	default:
		break;
	}
	return mask;
}

static void
msm_ispif_intf_cmd(uint8_t intftype, uint8_t intf_cmd_mask)
{
	uint8_t vc = 0, val = 0;
	uint32_t cid_mask = msm_ispif_get_cid_mask(intftype);

	while (cid_mask != 0) {
		if ((cid_mask & 0xf) != 0x0) {
			val = (intf_cmd_mask>>(vc*2)) & 0x3;
			global_intf_cmd_mask &= ~((0x3 & ~val)
				<<((vc*2)+(intftype*8)));
			CDBG("intf cmd  0x%x\n", global_intf_cmd_mask);
			msm_io_w(global_intf_cmd_mask,
				ispif->base + ISPIF_INTF_CMD_ADDR);
		}
		vc++;
		cid_mask >>= 4;
	}
}

static int msm_ispif_abort_intf_transfer(uint8_t intf)
{
	int rc = 0;
	uint8_t intf_cmd_mask = 0xAA;

	CDBG("abort stream request\n");
	mutex_lock(&ispif->mutex);
	msm_ispif_intf_cmd(intf, intf_cmd_mask);
	rc = msm_ispif_intf_reset(intf);
	global_intf_cmd_mask |= 0xFF<<(intf * 8);
	mutex_unlock(&ispif->mutex);
	return rc;
}

static int msm_ispif_start_intf_transfer(uint8_t intf)
{
	uint32_t data;
	uint8_t intf_cmd_mask = 0x55;
	int rc = 0;

	CDBG("start stream request\n");
	mutex_lock(&ispif->mutex);
	switch (intf) {
	case PIX0:
		data = msm_io_r(ispif->base + ISPIF_PIX_STATUS_ADDR);
		if ((data & 0xf) != 0xf) {
			CDBG("interface is busy\n");
			mutex_unlock(&ispif->mutex);
			return -EBUSY;
		}
		break;

	case RDI0:
		data  = msm_io_r(ispif->base + ISPIF_RDI_STATUS_ADDR);
		ispif->start_ack_pending = 1;
		break;

	case RDI1:
		data  = msm_io_r(ispif->base + ISPIF_RDI_1_STATUS_ADDR);
		ispif->start_ack_pending = 1;
		break;
	}
	msm_ispif_intf_cmd(intf, intf_cmd_mask);
	mutex_unlock(&ispif->mutex);
	return rc;
}

static int msm_ispif_stop_intf_transfer(uint8_t intf)
{
	int rc = 0;
	uint8_t intf_cmd_mask = 0x00;
	CDBG("stop stream request\n");
	mutex_lock(&ispif->mutex);
	msm_ispif_intf_cmd(intf, intf_cmd_mask);
	switch (intf) {
	case PIX0:
		while ((msm_io_r(ispif->base + ISPIF_PIX_STATUS_ADDR)
			& 0xf) != 0xf) {
			CDBG("Wait for Idle\n");
		}
		break;

	case RDI0:
		while ((msm_io_r(ispif->base + ISPIF_RDI_STATUS_ADDR)
			& 0xf) != 0xf) {
			CDBG("Wait for Idle\n");
		}
		break;
	default:
		break;
	}
	global_intf_cmd_mask |= 0xFF<<(intf * 8);
	mutex_unlock(&ispif->mutex);
	return rc;
}

static int msm_ispif_subdev_video_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct ispif_device *ispif =
			(struct ispif_device *)v4l2_get_subdevdata(sd);
	int32_t cmd = enable & ((1<<ISPIF_S_STREAM_SHIFT)-1);
	enum msm_ispif_intftype intf = enable >> ISPIF_S_STREAM_SHIFT;
	int rc = -EINVAL;

	BUG_ON(!ispif);
	switch (cmd) {
	case ISPIF_ON_FRAME_BOUNDARY:
		rc = msm_ispif_start_intf_transfer(intf);
		break;
	case ISPIF_OFF_FRAME_BOUNDARY:
		rc = msm_ispif_stop_intf_transfer(intf);
		break;
	case ISPIF_OFF_IMMEDIATELY:
		rc = msm_ispif_abort_intf_transfer(intf);
		break;
	default:
		break;
	}
	return rc;
}

static struct v4l2_subdev_core_ops msm_ispif_subdev_core_ops = {
	.g_chip_ident = &msm_ispif_subdev_g_chip_ident,
	.ioctl = &msm_ispif_subdev_ioctl,
};

static struct v4l2_subdev_video_ops msm_ispif_subdev_video_ops = {
	.s_stream = &msm_ispif_subdev_video_s_stream,
};

static const struct v4l2_subdev_ops msm_ispif_subdev_ops = {
	.core = &msm_ispif_subdev_core_ops,
	.video = &msm_ispif_subdev_video_ops,
};

static inline void msm_ispif_read_irq_status(struct ispif_irq_status *out)
{
	out->ispifIrqStatus0 = msm_io_r(ispif->base +
					ISPIF_IRQ_STATUS_ADDR);
	CDBG("ispif->irq: Irq_status0 = 0x%x\n",
		out->ispifIrqStatus0);
	if (out->ispifIrqStatus0 & ISPIF_IRQ_STATUS_MASK) {
		if (out->ispifIrqStatus0 & (0x1 << RESET_DONE_IRQ))
			complete(&ispif->reset_complete);
		if (out->ispifIrqStatus0 & (0x1 << PIX_INTF_0_OVERFLOW_IRQ))
			pr_err("%s: pix intf 0 overflow.\n", __func__);
		if (out->ispifIrqStatus0 & (0x1 << RAW_INTF_0_OVERFLOW_IRQ))
			pr_err("%s: rdi intf 0 overflow.\n", __func__);
		if (out->ispifIrqStatus0 & ISPIF_IRQ_STATUS_RDI_SOF_MASK) {
			if (ispif->start_ack_pending) {
				v4l2_subdev_notify(&ispif->subdev,
						NOTIFY_ISP_MSG_EVT,
						(void *)MSG_ID_START_ACK);
				ispif->start_ack_pending = 0;
				/* stop stream at frame boundary */
				msm_ispif_stop_intf_transfer(RDI0);
			}
			v4l2_subdev_notify(&ispif->subdev, NOTIFY_ISP_MSG_EVT,
					(void *)MSG_ID_SOF_ACK);
		}
	}
	msm_io_w(out->ispifIrqStatus0,
			ispif->base + ISPIF_IRQ_CLEAR_ADDR);
	msm_io_w(ISPIF_IRQ_GLOBAL_CLEAR_CMD, ispif->base +
		 ISPIF_IRQ_GLOBAL_CLEAR_CMD_ADDR);
}

static irqreturn_t msm_io_ispif_irq(int irq_num, void *data)
{
	struct ispif_irq_status irq;
	msm_ispif_read_irq_status(&irq);
	return IRQ_HANDLED;
}

int msm_ispif_init(struct v4l2_subdev **sd, struct platform_device *pdev)
{
	int rc = 0;

	ispif = kzalloc(sizeof(struct ispif_device), GFP_KERNEL);
	if (!ispif) {
		pr_err("%s: no enough memory\n", __func__);
		return -ENOMEM;
	}

	v4l2_subdev_init(&ispif->subdev, &msm_ispif_subdev_ops);
	v4l2_set_subdevdata(&ispif->subdev, ispif);
	snprintf(ispif->subdev.name, sizeof(ispif->subdev.name),
								"ispif");
	mutex_init(&ispif->mutex);

	ispif->mem = platform_get_resource_byname(pdev,
					IORESOURCE_MEM, "ispif");
	if (!ispif->mem) {
		pr_err("%s: no mem resource?\n", __func__);
		rc = -ENODEV;
		goto ispif_no_resource;
	}
	ispif->irq = platform_get_resource_byname(pdev,
					IORESOURCE_IRQ, "ispif");
	if (!ispif->irq) {
		pr_err("%s: no irq resource?\n", __func__);
		rc = -ENODEV;
		goto ispif_no_resource;
	}
	ispif->io = request_mem_region(ispif->mem->start,
		resource_size(ispif->mem), pdev->name);
	if (!ispif->io) {
		pr_err("%s: no valid mem region\n", __func__);
		rc = -EBUSY;
		goto ispif_no_resource;
	}
	ispif->base = ioremap(ispif->mem->start,
		resource_size(ispif->mem));
	if (!ispif->base) {
		rc = -ENOMEM;
		goto ispif_no_mem;
	}

	rc = request_irq(ispif->irq->start, msm_io_ispif_irq,
		IRQF_TRIGGER_RISING, "ispif", 0);
	if (rc < 0)
		goto ispif_irq_fail;

	global_intf_cmd_mask = 0xFFFFFFFF;
	init_completion(&ispif->reset_complete);

	rc = msm_ispif_reset();
	*sd = &(ispif->subdev);
	return rc;

ispif_irq_fail:
	iounmap(ispif->base);
ispif_no_mem:
	release_mem_region(ispif->mem->start,
			resource_size(ispif->mem));
ispif_no_resource:
	mutex_destroy(&ispif->mutex);
	kfree(ispif);
	return rc;
}

void msm_ispif_release(struct v4l2_subdev *sd)
{
	struct ispif_device *ispif =
			(struct ispif_device *)v4l2_get_subdevdata(sd);

	CDBG("%s, free_irq\n", __func__);
	free_irq(ispif->irq->start, 0);
	iounmap(ispif->base);
	release_mem_region(ispif->mem->start,
		resource_size(ispif->mem));
	kfree(ispif);
}

void msm_ispif_vfe_get_cid(uint8_t intftype, char *cids, int *num)
{
	uint32_t data = 0;
	int i = 0, j = 0;
	switch (intftype) {
	case PIX0:
		data = msm_io_r(ispif->base +
			ISPIF_PIX_INTF_CID_MASK_ADDR);
		break;

	case RDI0:
		data = msm_io_r(ispif->base +
			ISPIF_RDI_INTF_CID_MASK_ADDR);
		break;

	case RDI1:
		data = msm_io_r(ispif->base +
			ISPIF_RDI_1_INTF_CID_MASK_ADDR);
		break;

	default:
		break;
	}
	for (i = 0; i <= MAX_CID; i++) {
		if ((data & 0x1) == 0x1) {
			cids[j++] = i;
			(*num)++;
		}
		data >>= 1;
	}
}
