/* Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef __ARCH_ARM_MACH_MSM_DEVICES_MSM8X60_H
#define __ARCH_ARM_MACH_MSM_DEVICES_MSM8X60_H

/* GSBI related defines */
#define MSM_GSBI3_QUP_I2C_BUS_ID 0
#define MSM_GSBI4_QUP_I2C_BUS_ID 1
#define MSM_GSBI9_QUP_I2C_BUS_ID 2
#define MSM_GSBI8_QUP_I2C_BUS_ID 3
#define MSM_GSBI7_QUP_I2C_BUS_ID 4
#define MSM_GSBI12_QUP_I2C_BUS_ID 5
#define MSM_SSBI1_I2C_BUS_ID     6
#define MSM_SSBI2_I2C_BUS_ID     7
#define MSM_SSBI3_I2C_BUS_ID     8
#if defined (CONFIG_LGE_SENSOR_DCOMPASS)|| defined (CONFIG_LGE_SENSOR_PROXIMITY)
#define MSM_GSBI10_QUP_I2C_BUS_ID 9
#endif
#ifdef CONFIG_LGE_TOUCHSCREEN_SYNAPTICS_RMI4_I2C
#define MSM_GSBI1_QUP_I2C_BUS_ID 10
#endif
#if defined(CONFIG_LGE_FUEL_GAUGE) || defined(CONFIG_LGE_SWITCHING_CHARGER_MAX8971) || defined(CONFIG_LGE_SWITCHING_CHARGER_BQ24160_DOCOMO_ONLY)
#define MSM_GSBI5_QUP_I2C_BUS_ID 11
#endif

#define LGE_GSBI_BUS_ID_AUDIO_AMP_WM9093    MSM_GSBI3_QUP_I2C_BUS_ID
#define LGE_GSBI_BUS_ID_VIB_ISA1200         MSM_GSBI3_QUP_I2C_BUS_ID

/* GPIO related defines */
/* Macros assume PMIC GPIOs start at 0 */
#define PM8058_GPIO_BASE			NR_MSM_GPIOS
#define PM8058_GPIO_PM_TO_SYS(pm_gpio)		(pm_gpio + PM8058_GPIO_BASE)
#define PM8058_GPIO_SYS_TO_PM(sys_gpio)		(sys_gpio - PM8058_GPIO_BASE)
#define PM8058_MPP_BASE			(PM8058_GPIO_BASE + PM8058_GPIOS)
#define PM8058_MPP_PM_TO_SYS(pm_gpio)		(pm_gpio + PM8058_MPP_BASE)
#define PM8058_MPP_SYS_TO_PM(sys_gpio)		(sys_gpio - PM8058_MPP_BASE)
#define PM8058_IRQ_BASE				(NR_MSM_IRQS + NR_GPIO_IRQS)

#define PM8901_GPIO_BASE			(PM8058_GPIO_BASE + \
						PM8058_GPIOS + PM8058_MPPS)
#define PM8901_GPIO_PM_TO_SYS(pm_gpio)		(pm_gpio + PM8901_GPIO_BASE)
#define PM8901_GPIO_SYS_TO_PM(sys_gpio)		(sys_gpio - PM901_GPIO_BASE)
#define PM8901_IRQ_BASE				(PM8058_IRQ_BASE + \
						NR_PMIC8058_IRQS)

#define PMIC_GPIO_EAR_SENSE_N			19
#define PMIC_GPIO_EAR_KEY_INT			20

/* gpios for audio control */
#define PMIC_GPIO_RCV_AMP_RESET			7
#define PMIC_GPIO_CAMCORDER_MIC_EN		8

#define GPIO_RCV_AMP_RESET				PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_RCV_AMP_RESET - 1)
#define GPIO_CAMCORDER_MIC_EN			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_CAMCORDER_MIC_EN - 1)

/* gpios for ear jack detecion */
#define GPIO_EAR_SENSE_N			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_EAR_SENSE_N - 1)
#define GPIO_EAR_MIC_EN				58
#define GPIO_EARPOL_DETECT			126
#define GPIO_EAR_KEY_INT			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_EAR_KEY_INT - 1)

/* gpio and clock control for vibrator */
#define GPIO_LIN_MOTOR_EN		        158
#define GPIO_LIN_MOTOR_PWM	        	31

/* MIC BIAS configuration */
#define OTHC_MICBIAS_MAIN     OTHC_MICBIAS_0
#define OTHC_MICBIAS_SUB      OTHC_MICBIAS_1
#define OTHC_MICBIAS_HEADSET  OTHC_MICBIAS_2

#ifdef CONFIG_SPI_QUP
extern struct platform_device msm_gsbi1_qup_spi_device;
extern struct platform_device msm_gsbi10_qup_spi_device;
#ifdef CONFIG_LGE_BROADCAST
extern struct platform_device msm_gsbi11_qup_spi_device;	//LGE_BROADCAST SPI
#endif
#endif

#ifdef CONFIG_LGE_FUEL_GAUGE
extern struct platform_device msm_gsbi5_qup_i2c_device;
#endif 

#ifdef CONFIG_MSM_BUS_SCALING
extern struct platform_device msm_bus_apps_fabric;
extern struct platform_device msm_bus_sys_fabric;
extern struct platform_device msm_bus_mm_fabric;
extern struct platform_device msm_bus_sys_fpb;
extern struct platform_device msm_bus_cpss_fpb;
#endif

extern struct platform_device msm_device_smd;
extern struct platform_device msm_kgsl_3d0;
#ifdef CONFIG_MSM_KGSL_2D
extern struct platform_device msm_kgsl_2d0;
extern struct platform_device msm_kgsl_2d1;
#endif
extern struct platform_device msm_device_gpio;
extern struct platform_device msm_device_vidc;

extern struct platform_device msm_charm_modem;

#ifdef CONFIG_HW_RANDOM_MSM
extern struct platform_device msm_device_rng;
#endif

void __init msm_fb_register_device(char *name, void *data);
//[LGE_UPDATE_S] 20110421, sangyeol.lee@lge.com, [START]
extern struct platform_device msm_device_uart_dm1;
//[LGE_UPDATE_E] 20110421, sangyeol.lee@lge.com,  [END]

void __init msm8x60_init_irq(void);

#ifdef CONFIG_MSM_KGSL_2D
void __init msm8x60_check_2d_hardware(void);
#endif

#ifdef CONFIG_MSM_DSPS
extern struct platform_device msm_dsps_device;
#endif

#if defined(CONFIG_MSM_RPM_STATS_LOG)
extern struct platform_device msm_rpm_stat_device;
#endif
#endif
