/* Copyright (c) 2016-2017, The Linux Foundation. All rights reserved.
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

#define pr_fmt(fmt)	"FG: %s: " fmt, __func__

#include <linux/ktime.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_batterydata.h>
#include <linux/platform_device.h>
#include <linux/iio/consumer.h>
#include <linux/qpnp/qpnp-revid.h>
#include <linux/proc_fs.h>
#include <linux/switch.h>
#include "fg-core.h"
#include "fg-reg.h"
#include "../../bq27546_gauge.h"
#include <linux/ioctl.h>

#define FG_GEN3_DEV_NAME	"qcom,fg-gen3"

#define PERPH_SUBTYPE_REG		0x05
#define FG_BATT_SOC_PMI8998		0x10
#define FG_BATT_INFO_PMI8998		0x11
#define FG_MEM_INFO_PMI8998		0x0D

/* SRAM address and offset in ascending order */
#define ESR_PULSE_THRESH_WORD		2
#define ESR_PULSE_THRESH_OFFSET		3
#define SLOPE_LIMIT_WORD		3
#define SLOPE_LIMIT_OFFSET		0
#define CUTOFF_VOLT_WORD		5
#define CUTOFF_VOLT_OFFSET		0
#define SYS_TERM_CURR_WORD		6
#define SYS_TERM_CURR_OFFSET		0
#define VBATT_FULL_WORD			7
#define VBATT_FULL_OFFSET		0
#define ESR_FILTER_WORD			8
#define ESR_UPD_TIGHT_OFFSET		0
#define ESR_UPD_BROAD_OFFSET		1
#define ESR_UPD_TIGHT_LOW_TEMP_OFFSET	2
#define ESR_UPD_BROAD_LOW_TEMP_OFFSET	3
#define KI_COEFF_MED_DISCHG_WORD	9
#define TIMEBASE_OFFSET			1
#define KI_COEFF_MED_DISCHG_OFFSET	3
#define KI_COEFF_HI_DISCHG_WORD		10
#define KI_COEFF_HI_DISCHG_OFFSET	0
#define KI_COEFF_LOW_DISCHG_WORD	10
#define KI_COEFF_LOW_DISCHG_OFFSET	2
#define KI_COEFF_FULL_SOC_WORD		12
#define KI_COEFF_FULL_SOC_OFFSET	2
#define DELTA_MSOC_THR_WORD		12
#define DELTA_MSOC_THR_OFFSET		3
#define DELTA_BSOC_THR_WORD		13
#define DELTA_BSOC_THR_OFFSET		2
#define RECHARGE_SOC_THR_WORD		14
#define RECHARGE_SOC_THR_OFFSET		0
#define CHG_TERM_CURR_WORD		14
#define CHG_TERM_CURR_OFFSET		1
#define EMPTY_VOLT_WORD			15
#define EMPTY_VOLT_OFFSET		0
#define VBATT_LOW_WORD			15
#define VBATT_LOW_OFFSET		1
#define ESR_TIMER_DISCHG_MAX_WORD	17
#define ESR_TIMER_DISCHG_MAX_OFFSET	0
#define ESR_TIMER_DISCHG_INIT_WORD	17
#define ESR_TIMER_DISCHG_INIT_OFFSET	2
#define ESR_TIMER_CHG_MAX_WORD		18
#define ESR_TIMER_CHG_MAX_OFFSET	0
#define ESR_TIMER_CHG_INIT_WORD		18
#define ESR_TIMER_CHG_INIT_OFFSET	2
#define PROFILE_LOAD_WORD		24
#define PROFILE_LOAD_OFFSET		0
#define ESR_RSLOW_DISCHG_WORD		34
#define ESR_RSLOW_DISCHG_OFFSET		0
#define ESR_RSLOW_CHG_WORD		51
#define ESR_RSLOW_CHG_OFFSET		0
#define NOM_CAP_WORD			58
#define NOM_CAP_OFFSET			0
#define ACT_BATT_CAP_BKUP_WORD		74
#define ACT_BATT_CAP_BKUP_OFFSET	0
#define CYCLE_COUNT_WORD		75
#define CYCLE_COUNT_OFFSET		0
#define PROFILE_INTEGRITY_WORD		79
#define SW_CONFIG_OFFSET		0
#define PROFILE_INTEGRITY_OFFSET	3
#define BATT_SOC_WORD			91
#define BATT_SOC_OFFSET			0
#define FULL_SOC_WORD			93
#define FULL_SOC_OFFSET			2
#define MONOTONIC_SOC_WORD		94
#define MONOTONIC_SOC_OFFSET		2
#define CC_SOC_WORD			95
#define CC_SOC_OFFSET			0
#define CC_SOC_SW_WORD			96
#define CC_SOC_SW_OFFSET		0
#define VOLTAGE_PRED_WORD		97
#define VOLTAGE_PRED_OFFSET		0
#define OCV_WORD			97
#define OCV_OFFSET			2
#define ESR_WORD			99
#define ESR_OFFSET			0
#define RSLOW_WORD			101
#define RSLOW_OFFSET			0
#define ACT_BATT_CAP_WORD		117
#define ACT_BATT_CAP_OFFSET		0
#define LAST_BATT_SOC_WORD		119
#define LAST_BATT_SOC_OFFSET		0
#define LAST_MONOTONIC_SOC_WORD		119
#define LAST_MONOTONIC_SOC_OFFSET	2
#define ALG_FLAGS_WORD			120
#define ALG_FLAGS_OFFSET		1

/* v2 SRAM address and offset in ascending order */
#define KI_COEFF_LOW_DISCHG_v2_WORD	9
#define KI_COEFF_LOW_DISCHG_v2_OFFSET	3
#define KI_COEFF_MED_DISCHG_v2_WORD	10
#define KI_COEFF_MED_DISCHG_v2_OFFSET	0
#define KI_COEFF_HI_DISCHG_v2_WORD	10
#define KI_COEFF_HI_DISCHG_v2_OFFSET	1
#define DELTA_BSOC_THR_v2_WORD		12
#define DELTA_BSOC_THR_v2_OFFSET	3
#define DELTA_MSOC_THR_v2_WORD		13
#define DELTA_MSOC_THR_v2_OFFSET	0
#define RECHARGE_SOC_THR_v2_WORD	14
#define RECHARGE_SOC_THR_v2_OFFSET	1
#define CHG_TERM_CURR_v2_WORD		15
#define CHG_TERM_BASE_CURR_v2_OFFSET	0
#define CHG_TERM_CURR_v2_OFFSET		1
#define EMPTY_VOLT_v2_WORD		15
#define EMPTY_VOLT_v2_OFFSET		3
#define VBATT_LOW_v2_WORD		16
#define VBATT_LOW_v2_OFFSET		0
#define RECHARGE_VBATT_THR_v2_WORD	16
#define RECHARGE_VBATT_THR_v2_OFFSET	1
#define FLOAT_VOLT_v2_WORD		16
#define FLOAT_VOLT_v2_OFFSET		2

int gauge_get_prop =0;
struct fg_chip * g_fgChip = NULL;
#define REPORT_CAPACITY_POLLING_TIME 180

static struct delayed_work fix_maint_soc_work;

#define FAKE_TEMP_INIT	180
int fake_temp = FAKE_TEMP_INIT;

#define BATT_GENERIC "batterydata_Generic"
#define BATT_LIBRA_SMP "Libra_2900mah_4p38v_SMP"
#define BATT_LIBRA_COSLIGHT "Libra_2900mah_4p38v_Coslight"
#define BATT_LIBRA_LG "Libra_2900mah_4p38v_LG"
#define BATT_LEO_SMP "batterydata_Leo_2530mAh_4p38V_SMP"
#define BATT_TYPE_CSL "2826813_asus_c11p1618cos_3150mah_averaged_masterslave_mar17th2017"
#define BATT_TYPE_ASCENT	"ascent_3450mah_averaged_masterslave_jan6th2017"
#define BATT_TYPE_ZEUS_4P3V	"c11p1701_3400mah_apr18th2017_4p3v"
#define BATT_TYPE_ZEUS_4P35V	"c11p1701_3400mah_apr18th2017_4p35v"
#define BATT_TYPE_ZEUS_4P25V	"c11p1701_3400mah_apr18th2017_4p25v"
#define BATT_MODELNAME_LIBRA "C11P1511"
#define BATT_MODELNAME_LEO "C11P1601"
#define BATT_MODELNAME_ZEUS "C11P1701"
#define BATT_ID_51K_INDEX		1
#define BATT_ID_10K_INDEX		2
#define BATT_ID_100K_INDEX	3
#define BATT_ID_75K_INDEX	4
#define FIRST_PROFILE_VER	1
#define SECOND_PROFILE_VER	2



#define BATT_ASCENT	"ascent_51K"
#define BATT_UNKNOWN	 "Unknown Battery"


#define BATT_PROFILE_THD1	100
#define BATT_PROFILE_THD2	200
#define BATT_PROFILE_THD3	300

const char* target_profile_name[]={
	[0] = BATT_TYPE_ZEUS_4P25V,
	[1] = BATT_TYPE_ZEUS_4P3V,
	[2] = BATT_TYPE_ZEUS_4P35V,
};


#define DEFAULT_ASUS_FULL_MSOC_THR	 247

static int fg_decode_voltage_15b(struct fg_sram_param *sp,
	enum fg_sram_param_id id, int val);
static int fg_decode_value_16b(struct fg_sram_param *sp,
	enum fg_sram_param_id id, int val);
static int fg_decode_default(struct fg_sram_param *sp,
	enum fg_sram_param_id id, int val);
static int fg_decode_cc_soc(struct fg_sram_param *sp,
	enum fg_sram_param_id id, int value);
static void fg_encode_voltage(struct fg_sram_param *sp,
	enum fg_sram_param_id id, int val_mv, u8 *buf);
static void fg_encode_current(struct fg_sram_param *sp,
	enum fg_sram_param_id id, int val_ma, u8 *buf);
static void fg_encode_default(struct fg_sram_param *sp,
	enum fg_sram_param_id id, int val, u8 *buf);

static struct fg_irq_info fg_irqs[FG_IRQ_MAX];

#define PARAM(_id, _addr_word, _addr_byte, _len, _num, _den, _offset,	\
	      _enc, _dec)						\
	[FG_SRAM_##_id] = {						\
		.addr_word	= _addr_word,				\
		.addr_byte	= _addr_byte,				\
		.len		= _len,					\
		.numrtr		= _num,					\
		.denmtr		= _den,					\
		.offset		= _offset,				\
		.encode		= _enc,					\
		.decode		= _dec,					\
	}								\

static struct fg_sram_param pmi8998_v1_sram_params[] = {
	PARAM(BATT_SOC, BATT_SOC_WORD, BATT_SOC_OFFSET, 4, 1, 1, 0, NULL,
		fg_decode_default),
	PARAM(FULL_SOC, FULL_SOC_WORD, FULL_SOC_OFFSET, 2, 1, 1, 0, NULL,
		fg_decode_default),
	PARAM(VOLTAGE_PRED, VOLTAGE_PRED_WORD, VOLTAGE_PRED_OFFSET, 2, 1000,
		244141, 0, NULL, fg_decode_voltage_15b),
	PARAM(OCV, OCV_WORD, OCV_OFFSET, 2, 1000, 244141, 0, NULL,
		fg_decode_voltage_15b),
	PARAM(ESR, ESR_WORD, ESR_OFFSET, 2, 1000, 244141, 0, fg_encode_default,
		fg_decode_value_16b),
	PARAM(RSLOW, RSLOW_WORD, RSLOW_OFFSET, 2, 1000, 244141, 0, NULL,
		fg_decode_value_16b),
	PARAM(ALG_FLAGS, ALG_FLAGS_WORD, ALG_FLAGS_OFFSET, 1, 1, 1, 0, NULL,
		fg_decode_default),
	PARAM(CC_SOC, CC_SOC_WORD, CC_SOC_OFFSET, 4, 1, 1, 0, NULL,
		fg_decode_cc_soc),
	PARAM(CC_SOC_SW, CC_SOC_SW_WORD, CC_SOC_SW_OFFSET, 4, 1, 1, 0, NULL,
		fg_decode_cc_soc),
	PARAM(ACT_BATT_CAP, ACT_BATT_CAP_BKUP_WORD, ACT_BATT_CAP_BKUP_OFFSET, 2,
		1, 1, 0, NULL, fg_decode_default),
	/* Entries below here are configurable during initialization */
	PARAM(CUTOFF_VOLT, CUTOFF_VOLT_WORD, CUTOFF_VOLT_OFFSET, 2, 1000000,
		244141, 0, fg_encode_voltage, NULL),
	PARAM(EMPTY_VOLT, EMPTY_VOLT_WORD, EMPTY_VOLT_OFFSET, 1, 100000, 390625,
		-2500, fg_encode_voltage, NULL),
	PARAM(VBATT_LOW, VBATT_LOW_WORD, VBATT_LOW_OFFSET, 1, 100000, 390625,
		-2500, fg_encode_voltage, NULL),
	PARAM(VBATT_FULL, VBATT_FULL_WORD, VBATT_FULL_OFFSET, 2, 1000,
		244141, 0, fg_encode_voltage, fg_decode_voltage_15b),
	PARAM(SYS_TERM_CURR, SYS_TERM_CURR_WORD, SYS_TERM_CURR_OFFSET, 3,
		1000000, 122070, 0, fg_encode_current, NULL),
	PARAM(CHG_TERM_CURR, CHG_TERM_CURR_WORD, CHG_TERM_CURR_OFFSET, 1,
		100000, 390625, 0, fg_encode_current, NULL),
	PARAM(DELTA_MSOC_THR, DELTA_MSOC_THR_WORD, DELTA_MSOC_THR_OFFSET, 1,
		2048, 100, 0, fg_encode_default, NULL),
	PARAM(DELTA_BSOC_THR, DELTA_BSOC_THR_WORD, DELTA_BSOC_THR_OFFSET, 1,
		2048, 100, 0, fg_encode_default, NULL),
	PARAM(RECHARGE_SOC_THR, RECHARGE_SOC_THR_WORD, RECHARGE_SOC_THR_OFFSET,
		1, 256, 100, 0, fg_encode_default, NULL),
	PARAM(ESR_TIMER_DISCHG_MAX, ESR_TIMER_DISCHG_MAX_WORD,
		ESR_TIMER_DISCHG_MAX_OFFSET, 2, 1, 1, 0, fg_encode_default,
		NULL),
	PARAM(ESR_TIMER_DISCHG_INIT, ESR_TIMER_DISCHG_INIT_WORD,
		ESR_TIMER_DISCHG_INIT_OFFSET, 2, 1, 1, 0, fg_encode_default,
		NULL),
	PARAM(ESR_TIMER_CHG_MAX, ESR_TIMER_CHG_MAX_WORD,
		ESR_TIMER_CHG_MAX_OFFSET, 2, 1, 1, 0, fg_encode_default, NULL),
	PARAM(ESR_TIMER_CHG_INIT, ESR_TIMER_CHG_INIT_WORD,
		ESR_TIMER_CHG_INIT_OFFSET, 2, 1, 1, 0, fg_encode_default, NULL),
	PARAM(ESR_PULSE_THRESH, ESR_PULSE_THRESH_WORD, ESR_PULSE_THRESH_OFFSET,
		1, 100000, 390625, 0, fg_encode_default, NULL),
	PARAM(KI_COEFF_MED_DISCHG, KI_COEFF_MED_DISCHG_WORD,
		KI_COEFF_MED_DISCHG_OFFSET, 1, 1000, 244141, 0,
		fg_encode_default, NULL),
	PARAM(KI_COEFF_HI_DISCHG, KI_COEFF_HI_DISCHG_WORD,
		KI_COEFF_HI_DISCHG_OFFSET, 1, 1000, 244141, 0,
		fg_encode_default, NULL),
	PARAM(KI_COEFF_FULL_SOC, KI_COEFF_FULL_SOC_WORD,
		KI_COEFF_FULL_SOC_OFFSET, 1, 1000, 244141, 0,
		fg_encode_default, NULL),
	PARAM(ESR_TIGHT_FILTER, ESR_FILTER_WORD, ESR_UPD_TIGHT_OFFSET,
		1, 512, 1000000, 0, fg_encode_default, NULL),
	PARAM(ESR_BROAD_FILTER, ESR_FILTER_WORD, ESR_UPD_BROAD_OFFSET,
		1, 512, 1000000, 0, fg_encode_default, NULL),
	PARAM(SLOPE_LIMIT, SLOPE_LIMIT_WORD, SLOPE_LIMIT_OFFSET, 1, 8192, 1000,
		0, fg_encode_default, NULL),
};

static struct fg_sram_param pmi8998_v2_sram_params[] = {
	PARAM(BATT_SOC, BATT_SOC_WORD, BATT_SOC_OFFSET, 4, 1, 1, 0, NULL,
		fg_decode_default),
	PARAM(FULL_SOC, FULL_SOC_WORD, FULL_SOC_OFFSET, 2, 1, 1, 0, NULL,
		fg_decode_default),
	PARAM(VOLTAGE_PRED, VOLTAGE_PRED_WORD, VOLTAGE_PRED_OFFSET, 2, 1000,
		244141, 0, NULL, fg_decode_voltage_15b),
	PARAM(OCV, OCV_WORD, OCV_OFFSET, 2, 1000, 244141, 0, NULL,
		fg_decode_voltage_15b),
	PARAM(ESR, ESR_WORD, ESR_OFFSET, 2, 1000, 244141, 0, fg_encode_default,
		fg_decode_value_16b),
	PARAM(RSLOW, RSLOW_WORD, RSLOW_OFFSET, 2, 1000, 244141, 0, NULL,
		fg_decode_value_16b),
	PARAM(ALG_FLAGS, ALG_FLAGS_WORD, ALG_FLAGS_OFFSET, 1, 1, 1, 0, NULL,
		fg_decode_default),
	PARAM(CC_SOC, CC_SOC_WORD, CC_SOC_OFFSET, 4, 1, 1, 0, NULL,
		fg_decode_cc_soc),
	PARAM(CC_SOC_SW, CC_SOC_SW_WORD, CC_SOC_SW_OFFSET, 4, 1, 1, 0, NULL,
		fg_decode_cc_soc),
	PARAM(ACT_BATT_CAP, ACT_BATT_CAP_BKUP_WORD, ACT_BATT_CAP_BKUP_OFFSET, 2,
		1, 1, 0, NULL, fg_decode_default),
	PARAM(TIMEBASE, KI_COEFF_MED_DISCHG_WORD, TIMEBASE_OFFSET, 2, 1000,
		61000, 0, fg_encode_default, NULL),
	/* Entries below here are configurable during initialization */
	PARAM(CUTOFF_VOLT, CUTOFF_VOLT_WORD, CUTOFF_VOLT_OFFSET, 2, 1000000,
		244141, 0, fg_encode_voltage, NULL),
	PARAM(EMPTY_VOLT, EMPTY_VOLT_v2_WORD, EMPTY_VOLT_v2_OFFSET, 1, 1000,
		15625, -2000, fg_encode_voltage, NULL),
	PARAM(VBATT_LOW, VBATT_LOW_v2_WORD, VBATT_LOW_v2_OFFSET, 1, 1000,
		15625, -2000, fg_encode_voltage, NULL),
	PARAM(FLOAT_VOLT, FLOAT_VOLT_v2_WORD, FLOAT_VOLT_v2_OFFSET, 1, 1000,
		15625, -2000, fg_encode_voltage, NULL),
	PARAM(VBATT_FULL, VBATT_FULL_WORD, VBATT_FULL_OFFSET, 2, 1000,
		244141, 0, fg_encode_voltage, fg_decode_voltage_15b),
	PARAM(SYS_TERM_CURR, SYS_TERM_CURR_WORD, SYS_TERM_CURR_OFFSET, 3,
		1000000, 122070, 0, fg_encode_current, NULL),
	PARAM(CHG_TERM_CURR, CHG_TERM_CURR_v2_WORD, CHG_TERM_CURR_v2_OFFSET, 1,
		100000, 390625, 0, fg_encode_current, NULL),
	PARAM(CHG_TERM_BASE_CURR, CHG_TERM_CURR_v2_WORD,
		CHG_TERM_BASE_CURR_v2_OFFSET, 1, 1024, 1000, 0,
		fg_encode_current, NULL),
	PARAM(DELTA_MSOC_THR, DELTA_MSOC_THR_v2_WORD, DELTA_MSOC_THR_v2_OFFSET,
		1, 2048, 100, 0, fg_encode_default, NULL),
	PARAM(DELTA_BSOC_THR, DELTA_BSOC_THR_v2_WORD, DELTA_BSOC_THR_v2_OFFSET,
		1, 2048, 100, 0, fg_encode_default, NULL),
	PARAM(RECHARGE_SOC_THR, RECHARGE_SOC_THR_v2_WORD,
		RECHARGE_SOC_THR_v2_OFFSET, 1, 256, 100, 0, fg_encode_default,
		NULL),
	PARAM(RECHARGE_VBATT_THR, RECHARGE_VBATT_THR_v2_WORD,
		RECHARGE_VBATT_THR_v2_OFFSET, 1, 1000, 15625, -2000,
		fg_encode_voltage, NULL),
	PARAM(ESR_TIMER_DISCHG_MAX, ESR_TIMER_DISCHG_MAX_WORD,
		ESR_TIMER_DISCHG_MAX_OFFSET, 2, 1, 1, 0, fg_encode_default,
		NULL),
	PARAM(ESR_TIMER_DISCHG_INIT, ESR_TIMER_DISCHG_INIT_WORD,
		ESR_TIMER_DISCHG_INIT_OFFSET, 2, 1, 1, 0, fg_encode_default,
		NULL),
	PARAM(ESR_TIMER_CHG_MAX, ESR_TIMER_CHG_MAX_WORD,
		ESR_TIMER_CHG_MAX_OFFSET, 2, 1, 1, 0, fg_encode_default, NULL),
	PARAM(ESR_TIMER_CHG_INIT, ESR_TIMER_CHG_INIT_WORD,
		ESR_TIMER_CHG_INIT_OFFSET, 2, 1, 1, 0, fg_encode_default, NULL),
	PARAM(ESR_PULSE_THRESH, ESR_PULSE_THRESH_WORD, ESR_PULSE_THRESH_OFFSET,
		1, 100000, 390625, 0, fg_encode_default, NULL),
	PARAM(KI_COEFF_MED_DISCHG, KI_COEFF_MED_DISCHG_v2_WORD,
		KI_COEFF_MED_DISCHG_v2_OFFSET, 1, 1000, 244141, 0,
		fg_encode_default, NULL),
	PARAM(KI_COEFF_HI_DISCHG, KI_COEFF_HI_DISCHG_v2_WORD,
		KI_COEFF_HI_DISCHG_v2_OFFSET, 1, 1000, 244141, 0,
		fg_encode_default, NULL),
	PARAM(KI_COEFF_FULL_SOC, KI_COEFF_FULL_SOC_WORD,
		KI_COEFF_FULL_SOC_OFFSET, 1, 1000, 244141, 0,
		fg_encode_default, NULL),
	PARAM(ESR_TIGHT_FILTER, ESR_FILTER_WORD, ESR_UPD_TIGHT_OFFSET,
		1, 512, 1000000, 0, fg_encode_default, NULL),
	PARAM(ESR_BROAD_FILTER, ESR_FILTER_WORD, ESR_UPD_BROAD_OFFSET,
		1, 512, 1000000, 0, fg_encode_default, NULL),
	PARAM(SLOPE_LIMIT, SLOPE_LIMIT_WORD, SLOPE_LIMIT_OFFSET, 1, 8192, 1000,
		0, fg_encode_default, NULL),
};

static struct fg_alg_flag pmi8998_v1_alg_flags[] = {
	[ALG_FLAG_SOC_LT_OTG_MIN]	= {
		.name	= "SOC_LT_OTG_MIN",
		.bit	= BIT(0),
	},
	[ALG_FLAG_SOC_LT_RECHARGE]	= {
		.name	= "SOC_LT_RECHARGE",
		.bit	= BIT(1),
	},
	[ALG_FLAG_IBATT_LT_ITERM]	= {
		.name	= "IBATT_LT_ITERM",
		.bit	= BIT(2),
	},
	[ALG_FLAG_IBATT_GT_HPM]		= {
		.name	= "IBATT_GT_HPM",
		.bit	= BIT(3),
	},
	[ALG_FLAG_IBATT_GT_UPM]		= {
		.name	= "IBATT_GT_UPM",
		.bit	= BIT(4),
	},
	[ALG_FLAG_VBATT_LT_RECHARGE]	= {
		.name	= "VBATT_LT_RECHARGE",
		.bit	= BIT(5),
	},
	[ALG_FLAG_VBATT_GT_VFLOAT]	= {
		.invalid = true,
	},
};

static struct fg_alg_flag pmi8998_v2_alg_flags[] = {
	[ALG_FLAG_SOC_LT_OTG_MIN]	= {
		.name	= "SOC_LT_OTG_MIN",
		.bit	= BIT(0),
	},
	[ALG_FLAG_SOC_LT_RECHARGE]	= {
		.name	= "SOC_LT_RECHARGE",
		.bit	= BIT(1),
	},
	[ALG_FLAG_IBATT_LT_ITERM]	= {
		.name	= "IBATT_LT_ITERM",
		.bit	= BIT(2),
	},
	[ALG_FLAG_IBATT_GT_HPM]		= {
		.name	= "IBATT_GT_HPM",
		.bit	= BIT(4),
	},
	[ALG_FLAG_IBATT_GT_UPM]		= {
		.name	= "IBATT_GT_UPM",
		.bit	= BIT(5),
	},
	[ALG_FLAG_VBATT_LT_RECHARGE]	= {
		.name	= "VBATT_LT_RECHARGE",
		.bit	= BIT(6),
	},
	[ALG_FLAG_VBATT_GT_VFLOAT]	= {
		.name	= "VBATT_GT_VFLOAT",
		.bit	= BIT(7),
	},
};

static int fg_gen3_debug_mask;
module_param_named(
	debug_mask, fg_gen3_debug_mask, int, S_IRUSR | S_IWUSR
);

static bool fg_profile_dump;
module_param_named(
	profile_dump, fg_profile_dump, bool, S_IRUSR | S_IWUSR
);

static int fg_sram_dump_period_ms = 20000;
module_param_named(
	sram_dump_period_ms, fg_sram_dump_period_ms, int, S_IRUSR | S_IWUSR
);

static int fg_restart;
static bool fg_sram_dump;

/* All getters HERE */

#define VOLTAGE_15BIT_MASK	GENMASK(14, 0)
static int fg_decode_voltage_15b(struct fg_sram_param *sp,
				enum fg_sram_param_id id, int value)
{
	value &= VOLTAGE_15BIT_MASK;
	sp[id].value = div_u64((u64)value * sp[id].denmtr, sp[id].numrtr);
	pr_debug("id: %d raw value: %x decoded value: %x\n", id, value,
		sp[id].value);
	return sp[id].value;
}

static int fg_decode_cc_soc(struct fg_sram_param *sp,
				enum fg_sram_param_id id, int value)
{
	sp[id].value = div_s64((s64)value * sp[id].denmtr, sp[id].numrtr);
	sp[id].value = sign_extend32(sp[id].value, 31);
	pr_debug("id: %d raw value: %x decoded value: %x\n", id, value,
		sp[id].value);
	return sp[id].value;
}

static int fg_decode_value_16b(struct fg_sram_param *sp,
				enum fg_sram_param_id id, int value)
{
	sp[id].value = div_u64((u64)(u16)value * sp[id].denmtr, sp[id].numrtr);
	pr_debug("id: %d raw value: %x decoded value: %x\n", id, value,
		sp[id].value);
	return sp[id].value;
}

static int fg_decode_default(struct fg_sram_param *sp, enum fg_sram_param_id id,
				int value)
{
	sp[id].value = value;
	return sp[id].value;
}

static int fg_decode(struct fg_sram_param *sp, enum fg_sram_param_id id,
			int value)
{
	if (!sp[id].decode) {
		pr_err("No decoding function for parameter %d\n", id);
		return -EINVAL;
	}

	return sp[id].decode(sp, id, value);
}

static void fg_encode_voltage(struct fg_sram_param *sp,
				enum fg_sram_param_id  id, int val_mv, u8 *buf)
{
	int i, mask = 0xff;
	int64_t temp;

	val_mv += sp[id].offset;
	temp = (int64_t)div_u64((u64)val_mv * sp[id].numrtr, sp[id].denmtr);
	pr_debug("temp: %llx id: %d, val_mv: %d, buf: [ ", temp, id, val_mv);
	for (i = 0; i < sp[id].len; i++) {
		buf[i] = temp & mask;
		temp >>= 8;
		pr_debug("%x ", buf[i]);
	}
	pr_debug("]\n");
}

static void fg_encode_current(struct fg_sram_param *sp,
				enum fg_sram_param_id  id, int val_ma, u8 *buf)
{
	int i, mask = 0xff;
	int64_t temp;
	s64 current_ma;

	current_ma = val_ma;
	temp = (int64_t)div_s64(current_ma * sp[id].numrtr, sp[id].denmtr);
	pr_debug("temp: %llx id: %d, val: %d, buf: [ ", temp, id, val_ma);
	for (i = 0; i < sp[id].len; i++) {
		buf[i] = temp & mask;
		temp >>= 8;
		pr_debug("%x ", buf[i]);
	}
	pr_debug("]\n");
}

static void fg_encode_default(struct fg_sram_param *sp,
				enum fg_sram_param_id  id, int val, u8 *buf)
{
	int i, mask = 0xff;
	int64_t temp;

	temp = DIV_ROUND_CLOSEST(val * sp[id].numrtr, sp[id].denmtr);
	pr_debug("temp: %llx id: %d, val: %d, buf: [ ", temp, id, val);
	for (i = 0; i < sp[id].len; i++) {
		buf[i] = temp & mask;
		temp >>= 8;
		pr_debug("%x ", buf[i]);
	}
	pr_debug("]\n");
}

static void fg_encode(struct fg_sram_param *sp, enum fg_sram_param_id id,
			int val, u8 *buf)
{
	if (!sp[id].encode) {
		pr_err("No encoding function for parameter %d\n", id);
		return;
	}

	sp[id].encode(sp, id, val, buf);
}

/*
 * Please make sure *_sram_params table has the entry for the parameter
 * obtained through this function. In addition to address, offset,
 * length from where this SRAM parameter is read, a decode function
 * need to be specified.
 */
static int fg_get_sram_prop(struct fg_chip *chip, enum fg_sram_param_id id,
				int *val)
{
	int temp, rc, i;
	u8 buf[4];

	if (id < 0 || id > FG_SRAM_MAX || chip->sp[id].len > sizeof(buf))
		return -EINVAL;

	if (chip->battery_missing)
		return -ENODATA;

	rc = fg_sram_read(chip, chip->sp[id].addr_word, chip->sp[id].addr_byte,
		buf, chip->sp[id].len, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error reading address 0x%04x[%d] rc=%d\n",
			chip->sp[id].addr_word, chip->sp[id].addr_byte, rc);
		return rc;
	}

	for (i = 0, temp = 0; i < chip->sp[id].len; i++)
		temp |= buf[i] << (8 * i);

	*val = fg_decode(chip->sp, id, temp);
	return 0;
}

#define CC_SOC_30BIT	GENMASK(29, 0)
static int fg_get_charge_raw(struct fg_chip *chip, int *val)
{
	int rc, cc_soc;

	rc = fg_get_sram_prop(chip, FG_SRAM_CC_SOC, &cc_soc);
	if (rc < 0) {
		pr_err("Error in getting CC_SOC, rc=%d\n", rc);
		return rc;
	}

	*val = div_s64(cc_soc * chip->cl.nom_cap_uah, CC_SOC_30BIT);
	return 0;
}

static int fg_get_charge_counter(struct fg_chip *chip, int *val)
{
	int rc, cc_soc;

	rc = fg_get_sram_prop(chip, FG_SRAM_CC_SOC_SW, &cc_soc);
	if (rc < 0) {
		pr_err("Error in getting CC_SOC_SW, rc=%d\n", rc);
		return rc;
	}

	*val = div_s64(cc_soc * chip->cl.learned_cc_uah, CC_SOC_30BIT);
	return 0;
}

#define BATT_TEMP_NUMR		1
#define BATT_TEMP_DENR		1
static int fg_get_battery_temp(struct fg_chip *chip, int *val)
{
	int rc = 0, temp;
	u8 buf[2];

	rc = fg_read(chip, BATT_INFO_BATT_TEMP_LSB(chip), buf, 2);
	if (rc < 0) {
		pr_err("failed to read addr=0x%04x, rc=%d\n",
			BATT_INFO_BATT_TEMP_LSB(chip), rc);
		return rc;
	}

	temp = ((buf[1] & BATT_TEMP_MSB_MASK) << 8) |
		(buf[0] & BATT_TEMP_LSB_MASK);
	temp = DIV_ROUND_CLOSEST(temp, 4);

	/* Value is in Kelvin; Convert it to deciDegC */
	temp = (temp - 273) * 10;
	*val = temp;
	return 0;
}

static int fg_get_battery_resistance(struct fg_chip *chip, int *val)
{
	int rc, esr_uohms, rslow_uohms;

	rc = fg_get_sram_prop(chip, FG_SRAM_ESR, &esr_uohms);
	if (rc < 0) {
		pr_err("failed to get ESR, rc=%d\n", rc);
		return rc;
	}

	rc = fg_get_sram_prop(chip, FG_SRAM_RSLOW, &rslow_uohms);
	if (rc < 0) {
		pr_err("failed to get Rslow, rc=%d\n", rc);
		return rc;
	}

	*val = esr_uohms + rslow_uohms;
	return 0;
}

#define BATT_CURRENT_NUMR	488281
#define BATT_CURRENT_DENR	1000
static int fg_get_battery_current(struct fg_chip *chip, int *val)
{
	int rc = 0;
	int64_t temp = 0;
	u8 buf[2];

	rc = fg_read(chip, BATT_INFO_IBATT_LSB(chip), buf, 2);
	if (rc < 0) {
		pr_err("failed to read addr=0x%04x, rc=%d\n",
			BATT_INFO_IBATT_LSB(chip), rc);
		return rc;
	}

	if (chip->wa_flags & PMI8998_V1_REV_WA)
		temp = buf[0] << 8 | buf[1];
	else
		temp = buf[1] << 8 | buf[0];

	pr_debug("buf: %x %x temp: %llx\n", buf[0], buf[1], temp);
	/* Sign bit is bit 15 */
	temp = twos_compliment_extend(temp, 15);
	*val = div_s64((s64)temp * BATT_CURRENT_NUMR, BATT_CURRENT_DENR);
	return 0;
}

#define BATT_VOLTAGE_NUMR	122070
#define BATT_VOLTAGE_DENR	1000
static int fg_get_battery_voltage(struct fg_chip *chip, int *val)
{
	int rc = 0;
	u16 temp = 0;
	u8 buf[2];

	rc = fg_read(chip, BATT_INFO_VBATT_LSB(chip), buf, 2);
	if (rc < 0) {
		pr_err("failed to read addr=0x%04x, rc=%d\n",
			BATT_INFO_VBATT_LSB(chip), rc);
		return rc;
	}

	if (chip->wa_flags & PMI8998_V1_REV_WA)
		temp = buf[0] << 8 | buf[1];
	else
		temp = buf[1] << 8 | buf[0];

	pr_debug("buf: %x %x temp: %x\n", buf[0], buf[1], temp);
	*val = div_u64((u64)temp * BATT_VOLTAGE_NUMR, BATT_VOLTAGE_DENR);
	return 0;
}

#define MAX_TRIES_SOC		5
static int fg_get_msoc_raw(struct fg_chip *chip, int *val)
{
	u8 cap[2];
	int rc, tries = 0;

	while (tries < MAX_TRIES_SOC) {
		rc = fg_read(chip, BATT_SOC_FG_MONOTONIC_SOC(chip), cap, 2);
		if (rc < 0) {
			pr_err("failed to read addr=0x%04x, rc=%d\n",
				BATT_SOC_FG_MONOTONIC_SOC(chip), rc);
			return rc;
		}

		if (cap[0] == cap[1])
			break;

		tries++;
	}

	if (tries == MAX_TRIES_SOC) {
		pr_err("shadow registers do not match\n");
		return -EINVAL;
	}

	fg_dbg(chip, FG_POWER_SUPPLY, "raw: 0x%02x\n", cap[0]);
	*val = cap[0];
	return 0;
}

#define FULL_CAPACITY	100
#define FULL_SOC_RAW	255
static int fg_get_msoc(struct fg_chip *chip, int *msoc)
{
	int rc;

	rc = fg_get_msoc_raw(chip, msoc);
	if (rc < 0)
		return rc;

	*msoc = DIV_ROUND_CLOSEST(*msoc * FULL_CAPACITY, FULL_SOC_RAW);
	return 0;
}

static bool is_batt_empty(struct fg_chip *chip)
{
	u8 status;
	int rc, vbatt_uv, msoc;

	rc = fg_read(chip, BATT_SOC_INT_RT_STS(chip), &status, 1);
	if (rc < 0) {
		pr_err("failed to read addr=0x%04x, rc=%d\n",
			BATT_SOC_INT_RT_STS(chip), rc);
		return false;
	}

	if (!(status & MSOC_EMPTY_BIT))
		return false;

	rc = fg_get_battery_voltage(chip, &vbatt_uv);
	if (rc < 0) {
		pr_err("failed to get battery voltage, rc=%d\n", rc);
		return false;
	}

	rc = fg_get_msoc(chip, &msoc);
	if (!rc)
		pr_warn("batt_soc_rt_sts: %x vbatt: %d uV msoc:%d\n", status,
			vbatt_uv, msoc);

	return ((vbatt_uv < chip->dt.cutoff_volt_mv * 1000) ? true : false);
}

static int fg_get_debug_batt_id(struct fg_chip *chip, int *batt_id)
{
	int rc;
	u64 temp;
	u8 buf[2];

	rc = fg_read(chip, ADC_RR_FAKE_BATT_LOW_LSB(chip), buf, 2);
	if (rc < 0) {
		pr_err("failed to read addr=0x%04x, rc=%d\n",
			ADC_RR_FAKE_BATT_LOW_LSB(chip), rc);
		return rc;
	}

	/*
	 * Fake battery threshold is encoded in the following format.
	 * Threshold (code) = (battery_id in Ohms) * 0.00015 * 2^10 / 2.5
	 */
	temp = (buf[1] << 8 | buf[0]) * 2500000;
	do_div(temp, 150 * 1024);
	batt_id[0] = temp;
	rc = fg_read(chip, ADC_RR_FAKE_BATT_HIGH_LSB(chip), buf, 2);
	if (rc < 0) {
		pr_err("failed to read addr=0x%04x, rc=%d\n",
			ADC_RR_FAKE_BATT_HIGH_LSB(chip), rc);
		return rc;
	}

	temp = (buf[1] << 8 | buf[0]) * 2500000;
	do_div(temp, 150 * 1024);
	batt_id[1] = temp;
	pr_debug("debug batt_id range: [%d %d]\n", batt_id[0], batt_id[1]);
	return 0;
}

static bool is_debug_batt_id(struct fg_chip *chip)
{
	int debug_batt_id[2], rc;

	if (!chip->batt_id_ohms)
		return false;

	rc = fg_get_debug_batt_id(chip, debug_batt_id);
	if (rc < 0) {
		pr_err("Failed to get debug batt_id, rc=%d\n", rc);
		return false;
	}

	if (is_between(debug_batt_id[0], debug_batt_id[1],
		chip->batt_id_ohms)) {
		fg_dbg(chip, FG_POWER_SUPPLY, "Debug battery id: %dohms\n",
			chip->batt_id_ohms);
		return true;
	}

	return false;
}

#define DEBUG_BATT_SOC	67
#define BATT_MISS_SOC	50
#define EMPTY_SOC	0
static int fg_get_prop_capacity(struct fg_chip *chip, int *val)
{
	int rc, msoc;

	if (is_debug_batt_id(chip)) {
		*val = DEBUG_BATT_SOC;
		return 0;
	}

	if (chip->fg_restarting) {
		*val = chip->last_soc;
		return 0;
	}

	if (chip->battery_missing) {
		*val = BATT_MISS_SOC;
		return 0;
	}

	if (is_batt_empty(chip)) {
		*val = EMPTY_SOC;
		return 0;
	}

	if (chip->charge_full) {
		*val = FULL_CAPACITY;
		return 0;
	}

	rc = fg_get_msoc(chip, &msoc);
	if (rc < 0)
		return rc;

	if (chip->delta_soc > 0)
		*val = DIV_ROUND_CLOSEST(chip->maint_soc * FULL_CAPACITY, FULL_SOC_RAW);
	else
		*val = msoc;
	return 0;
}

#define DEFAULT_BATT_TYPE	"Unknown Battery"
#define MISSING_BATT_TYPE	"Missing Battery"
#define LOADING_BATT_TYPE	"Loading Battery"
static const char *fg_get_battery_type(struct fg_chip *chip)
{
	if (chip->battery_missing)
		return MISSING_BATT_TYPE;

	if (chip->bp.batt_type_str) {
		if (chip->profile_loaded)
			return chip->bp.batt_type_str;
		else if (chip->profile_available)
			return LOADING_BATT_TYPE;
	}

	return DEFAULT_BATT_TYPE;
}

static int fg_batt_missing_config(struct fg_chip *chip, bool enable)
{
	int rc;

	rc = fg_masked_write(chip, BATT_INFO_BATT_MISS_CFG(chip),
			BM_FROM_BATT_ID_BIT, enable ? BM_FROM_BATT_ID_BIT : 0);
	if (rc < 0)
		pr_err("Error in writing to %04x, rc=%d\n",
			BATT_INFO_BATT_MISS_CFG(chip), rc);
	return rc;
}

static int fg_get_batt_id(struct fg_chip *chip)
{
	int rc, ret, batt_id = 0;

	if (!chip->batt_id_chan)
		return -EINVAL;

	rc = fg_batt_missing_config(chip, false);
	if (rc < 0) {
		pr_err("Error in disabling BMD, rc=%d\n", rc);
		return rc;
	}

	rc = iio_read_channel_processed(chip->batt_id_chan, &batt_id);
	if (rc < 0) {
		pr_err("Error in reading batt_id channel, rc:%d\n", rc);
		goto out;
	}

	/* Wait for 200ms before enabling BMD again */
	msleep(200);

	fg_dbg(chip, FG_STATUS, "batt_id: %d\n", batt_id);
	chip->batt_id_ohms = batt_id;
out:
	ret = fg_batt_missing_config(chip, true);
	if (ret < 0) {
		pr_err("Error in enabling BMD, ret=%d\n", ret);
		return ret;
	}

	return rc;
}



int determine_target_profile(struct fg_chip *chip){

	int i=0;
	u32 val=0;
	int target_index=0;

	for(i=0;i< BUCKET_COUNT;i++)
		val += chip->cyc_ctr.count[i];
	
	BAT_DBG("read cycle count raw: %d\n", (int)val);
	val /= (BUCKET_COUNT-1);

	if(is_between(0,BATT_PROFILE_THD1,val))
		target_index = 0;
	else if(is_between(BATT_PROFILE_THD1, BATT_PROFILE_THD2,val))
		target_index = 1;
	else 
		target_index = 2;

	BAT_DBG("cycle count: %d, target index %d\n", (int)val, target_index);

	return target_index;

}

static void restore_cycle_counter(struct fg_chip *chip);



#define	BATTID_100K_THD_LOWER	85000
#define	BATTID_100K_THD_UPPER	115000

static int fg_get_batt_profile(struct fg_chip *chip)
{
	struct device_node *node = chip->dev->of_node;
	struct device_node *batt_node, *profile_node;
	const char *data;
	int rc, len, index=0; 

	restore_cycle_counter(chip);
	index = determine_target_profile(chip);

	batt_node = of_find_node_by_name(node, "qcom,battery-data");
	if (!batt_node) {
		pr_err("Batterydata not available\n");
		return -ENXIO;
	}

	if(chip->batt_id_ohms >BATTID_100K_THD_LOWER &&
		chip->batt_id_ohms < BATTID_100K_THD_UPPER){
		profile_node = of_batterydata_get_best_profile(batt_node,
					chip->batt_id_ohms / 1000, BATT_TYPE_ZEUS_4P35V);
	} else {
		profile_node = of_batterydata_get_best_profile(batt_node,
					chip->batt_id_ohms / 1000, BATT_TYPE_ZEUS_4P25V);
	}	
	if (IS_ERR(profile_node))
		return PTR_ERR(profile_node);

	if (!profile_node) {
		pr_err("couldn't find profile handle\n");
		return -ENODATA;
	}

	rc = of_property_read_string(profile_node, "qcom,battery-type",
			&chip->bp.batt_type_str);
	if (rc < 0) {
		pr_err("battery type unavailable, rc:%d\n", rc);
		return rc;
	}

	rc = of_property_read_u32(profile_node, "qcom,max-voltage-uv",
			&chip->bp.float_volt_uv);
	if (rc < 0) {
		pr_err("battery float voltage unavailable, rc:%d\n", rc);
		chip->bp.float_volt_uv = -EINVAL;
	}
	BAT_DBG("float voltage: %d",chip->bp.float_volt_uv);

	rc = of_property_read_u32(profile_node, "qcom,fastchg-current-ma",
			&chip->bp.fastchg_curr_ma);
	if (rc < 0) {
		pr_err("battery fastchg current unavailable, rc:%d\n", rc);
		chip->bp.fastchg_curr_ma = -EINVAL;
	}
	BAT_DBG("fastchg-current %d ma\n",chip->bp.fastchg_curr_ma);


	rc = of_property_read_u32(profile_node, "qcom,fg-cc-cv-threshold-mv",
			&chip->bp.vbatt_full_mv);
	if (rc < 0) {
		pr_err("battery cc_cv threshold unavailable, rc:%d\n", rc);
		chip->bp.vbatt_full_mv = -EINVAL;
	}
	BAT_DBG("cc-cv threshold %d mv\n",chip->bp.vbatt_full_mv);


	data = of_get_property(profile_node, "qcom,fg-profile-data", &len);
	if (!data) {
		pr_err("No profile data available\n");
		return -ENODATA;
	}

	if (len != PROFILE_LEN) {
		pr_err("battery profile incorrect size: %d\n", len);
		return -EINVAL;
	}

	chip->profile_available = true;
	memcpy(chip->batt_profile, data, len);

	return 0;
}

static inline void get_temp_setpoint(int threshold, u8 *val)
{
	/* Resolution is 0.5C. Base is -30C. */
	*val = DIV_ROUND_CLOSEST((threshold + 30) * 10, 5);
}

static inline void get_batt_temp_delta(int delta, u8 *val)
{
	switch (delta) {
	case 2:
		*val = BTEMP_DELTA_2K;
		break;
	case 4:
		*val = BTEMP_DELTA_4K;
		break;
	case 6:
		*val = BTEMP_DELTA_6K;
		break;
	case 10:
		*val = BTEMP_DELTA_10K;
		break;
	default:
		*val = BTEMP_DELTA_2K;
		break;
	};
}

static inline void get_esr_meas_current(int curr_ma, u8 *val)
{
	switch (curr_ma) {
	case 60:
		*val = ESR_MEAS_CUR_60MA;
		break;
	case 120:
		*val = ESR_MEAS_CUR_120MA;
		break;
	case 180:
		*val = ESR_MEAS_CUR_180MA;
		break;
	case 240:
		*val = ESR_MEAS_CUR_240MA;
		break;
	default:
		*val = ESR_MEAS_CUR_120MA;
		break;
	};

	*val <<= ESR_PULL_DOWN_IVAL_SHIFT;
}

static int fg_set_esr_timer(struct fg_chip *chip, int cycles, bool charging,
				int flags)
{
	u8 buf[2];
	int rc, timer_max, timer_init;

	if (charging) {
		timer_max = FG_SRAM_ESR_TIMER_CHG_MAX;
		timer_init = FG_SRAM_ESR_TIMER_CHG_INIT;
	} else {
		timer_max = FG_SRAM_ESR_TIMER_DISCHG_MAX;
		timer_init = FG_SRAM_ESR_TIMER_DISCHG_INIT;
	}

	fg_encode(chip->sp, timer_max, cycles, buf);
	rc = fg_sram_write(chip,
			chip->sp[timer_max].addr_word,
			chip->sp[timer_max].addr_byte, buf,
			chip->sp[timer_max].len, flags);
	if (rc < 0) {
		pr_err("Error in writing esr_timer_dischg_max, rc=%d\n",
			rc);
		return rc;
	}

	fg_encode(chip->sp, timer_init, cycles, buf);
	rc = fg_sram_write(chip,
			chip->sp[timer_init].addr_word,
			chip->sp[timer_init].addr_byte, buf,
			chip->sp[timer_init].len, flags);
	if (rc < 0) {
		pr_err("Error in writing esr_timer_dischg_init, rc=%d\n",
			rc);
		return rc;
	}

	return 0;
}

/* Other functions HERE */

static void fg_notify_charger(struct fg_chip *chip)
{
	union power_supply_propval prop = {0, };
	int rc;

	if (!chip->batt_psy)
		return;

	if (!chip->profile_available)
		return;

	prop.intval = chip->bp.float_volt_uv;
	rc = power_supply_set_property(chip->batt_psy,
			POWER_SUPPLY_PROP_VOLTAGE_MAX, &prop);
	if (rc < 0) {
		pr_err("Error in setting voltage_max property on batt_psy, rc=%d\n",
			rc);
		return;
	}

	prop.intval = chip->bp.fastchg_curr_ma * 1000;
	rc = power_supply_set_property(chip->batt_psy,
			POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX, &prop);
	if (rc < 0) {
		pr_err("Error in setting constant_charge_current_max property on batt_psy, rc=%d\n",
			rc);
		return;
	}

	fg_dbg(chip, FG_STATUS, "Notified charger on float voltage and FCC\n");
}

static int fg_awake_cb(struct votable *votable, void *data, int awake,
			const char *client)
{
	struct fg_chip *chip = data;

	if (awake)
		pm_stay_awake(chip->dev);
	else
		pm_relax(chip->dev);

	pr_debug("client: %s awake: %d\n", client, awake);
	return 0;
}

static bool usb_psy_initialized(struct fg_chip *chip)
{
	if (chip->usb_psy)
		return true;

	chip->usb_psy = power_supply_get_by_name("usb");
	if (!chip->usb_psy)
		return false;

	return true;
}



static bool batt_psy_initialized(struct fg_chip *chip)
{
	if (chip->batt_psy)
		return true;

	chip->batt_psy = power_supply_get_by_name("battery");
	if (!chip->batt_psy)
		return false;

	/* batt_psy is initialized, set the fcc and fv */
	fg_notify_charger(chip);

	return true;
}

static bool is_parallel_charger_available(struct fg_chip *chip)
{
	if (!chip->parallel_psy)
		chip->parallel_psy = power_supply_get_by_name("parallel");

	if (!chip->parallel_psy)
		return false;

	return true;
}

static int fg_save_learned_cap_to_sram(struct fg_chip *chip)
{
	int16_t cc_mah;
	int rc;

	if (chip->battery_missing || !chip->cl.learned_cc_uah)
		return -EPERM;

	cc_mah = div64_s64(chip->cl.learned_cc_uah, 1000);
	/* Write to a backup register to use across reboot */
	rc = fg_sram_write(chip, chip->sp[FG_SRAM_ACT_BATT_CAP].addr_word,
			chip->sp[FG_SRAM_ACT_BATT_CAP].addr_byte, (u8 *)&cc_mah,
			chip->sp[FG_SRAM_ACT_BATT_CAP].len, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in writing act_batt_cap_bkup, rc=%d\n", rc);
		return rc;
	}

	/* Write to actual capacity register for coulomb counter operation */
	rc = fg_sram_write(chip, ACT_BATT_CAP_WORD, ACT_BATT_CAP_OFFSET,
			(u8 *)&cc_mah, chip->sp[FG_SRAM_ACT_BATT_CAP].len,
			FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in writing act_batt_cap, rc=%d\n", rc);
		return rc;
	}

	fg_dbg(chip, FG_CAP_LEARN, "learned capacity %llduah/%dmah stored\n",
		chip->cl.learned_cc_uah, cc_mah);
	return 0;
}

#define CAPACITY_DELTA_DECIPCT	500
static int fg_load_learned_cap_from_sram(struct fg_chip *chip)
{
	int rc, act_cap_mah;
	int64_t delta_cc_uah, pct_nom_cap_uah;

	rc = fg_get_sram_prop(chip, FG_SRAM_ACT_BATT_CAP, &act_cap_mah);
	if (rc < 0) {
		pr_err("Error in getting ACT_BATT_CAP, rc=%d\n", rc);
		return rc;
	}

	chip->cl.learned_cc_uah = act_cap_mah * 1000;
	BAT_DBG(" get learned_cc_uah:%lld\n",chip->cl.learned_cc_uah);
	

	if (chip->cl.learned_cc_uah != chip->cl.nom_cap_uah) {
		if (chip->cl.learned_cc_uah == 0)
			chip->cl.learned_cc_uah = chip->cl.nom_cap_uah;

		delta_cc_uah = abs(chip->cl.learned_cc_uah -
					chip->cl.nom_cap_uah);
		pct_nom_cap_uah = div64_s64((int64_t)chip->cl.nom_cap_uah *
				CAPACITY_DELTA_DECIPCT, 1000);
		/*
		 * If the learned capacity is out of range by 50% from the
		 * nominal capacity, then overwrite the learned capacity with
		 * the nominal capacity.
		 */
		if (chip->cl.nom_cap_uah && delta_cc_uah > pct_nom_cap_uah) {
			fg_dbg(chip, FG_CAP_LEARN, "learned_cc_uah: %lld is higher than expected, capping it to nominal: %lld\n",
				chip->cl.learned_cc_uah, chip->cl.nom_cap_uah);
			chip->cl.learned_cc_uah = chip->cl.nom_cap_uah;
		}

		rc = fg_save_learned_cap_to_sram(chip);
		if (rc < 0)
			pr_err("Error in saving learned_cc_uah, rc=%d\n", rc);
	}

	BAT_DBG(" set learned_cc_uah:%lld nom_cap_uah: %lld\n",
		chip->cl.learned_cc_uah, chip->cl.nom_cap_uah);
	return 0;
}

static bool is_temp_valid_cap_learning(struct fg_chip *chip)
{
	int rc, batt_temp;

	rc = fg_get_battery_temp(chip, &batt_temp);
	if (rc < 0) {
		pr_err("Error in getting batt_temp\n");
		return false;
	}

	if (batt_temp > chip->dt.cl_max_temp ||
		batt_temp < chip->dt.cl_min_temp) {
		fg_dbg(chip, FG_CAP_LEARN, "batt temp %d out of range [%d %d]\n",
			batt_temp, chip->dt.cl_min_temp, chip->dt.cl_max_temp);
		return false;
	}

	return true;
}

static void fg_cap_learning_post_process(struct fg_chip *chip)
{
	int64_t max_inc_val, min_dec_val, old_cap;
	int rc;

	max_inc_val = chip->cl.learned_cc_uah
			* (1000 + chip->dt.cl_max_cap_inc);
	do_div(max_inc_val, 1000);

	min_dec_val = chip->cl.learned_cc_uah
			* (1000 - chip->dt.cl_max_cap_dec);
	do_div(min_dec_val, 1000);

	old_cap = chip->cl.learned_cc_uah;
	if (chip->cl.final_cc_uah > max_inc_val)
		chip->cl.learned_cc_uah = max_inc_val;
	else if (chip->cl.final_cc_uah < min_dec_val)
		chip->cl.learned_cc_uah = min_dec_val;
	else
		chip->cl.learned_cc_uah =
			chip->cl.final_cc_uah;

	if (chip->dt.cl_max_cap_limit) {
		max_inc_val = (int64_t)chip->cl.nom_cap_uah * (1000 +
				chip->dt.cl_max_cap_limit);
		do_div(max_inc_val, 1000);
		if (chip->cl.final_cc_uah > max_inc_val) {
			fg_dbg(chip, FG_CAP_LEARN, "learning capacity %lld goes above max limit %lld\n",
				chip->cl.final_cc_uah, max_inc_val);
			chip->cl.learned_cc_uah = max_inc_val;
		}
	}

	if (chip->dt.cl_min_cap_limit) {
		min_dec_val = (int64_t)chip->cl.nom_cap_uah * (1000 -
				chip->dt.cl_min_cap_limit);
		do_div(min_dec_val, 1000);
		if (chip->cl.final_cc_uah < min_dec_val) {
			fg_dbg(chip, FG_CAP_LEARN, "learning capacity %lld goes below min limit %lld\n",
				chip->cl.final_cc_uah, min_dec_val);
			chip->cl.learned_cc_uah = min_dec_val;
		}
	}

	rc = fg_save_learned_cap_to_sram(chip);
	if (rc < 0)
		pr_err("Error in saving learned_cc_uah, rc=%d\n", rc);

	fg_dbg(chip, FG_CAP_LEARN, "final cc_uah = %lld, learned capacity %lld -> %lld uah\n",
		chip->cl.final_cc_uah, old_cap, chip->cl.learned_cc_uah);
}

static int fg_cap_learning_process_full_data(struct fg_chip *chip)
{
	int rc, cc_soc_sw, cc_soc_delta_pct;
	int64_t delta_cc_uah;

	rc = fg_get_sram_prop(chip, FG_SRAM_CC_SOC_SW, &cc_soc_sw);
	if (rc < 0) {
		pr_err("Error in getting CC_SOC_SW, rc=%d\n", rc);
		return rc;
	}

	cc_soc_delta_pct = DIV_ROUND_CLOSEST(
				abs(cc_soc_sw - chip->cl.init_cc_soc_sw) * 100,
				CC_SOC_30BIT);
	delta_cc_uah = div64_s64(chip->cl.learned_cc_uah * cc_soc_delta_pct,
				100);
	chip->cl.final_cc_uah = chip->cl.init_cc_uah + delta_cc_uah;
	fg_dbg(chip, FG_CAP_LEARN, "Current cc_soc=%d cc_soc_delta_pct=%d total_cc_uah=%lld\n",
		cc_soc_sw, cc_soc_delta_pct, chip->cl.final_cc_uah);
	return 0;
}

#define BATT_SOC_32BIT	GENMASK(31, 0)
static int fg_cap_learning_begin(struct fg_chip *chip, u32 batt_soc)
{
	int rc, cc_soc_sw, batt_soc_msb;

	batt_soc_msb = batt_soc >> 24;
	if (DIV_ROUND_CLOSEST(batt_soc_msb * 100, FULL_SOC_RAW) >
		chip->dt.cl_start_soc) {
		fg_dbg(chip, FG_CAP_LEARN, "Battery SOC %d is high!, not starting\n",
			batt_soc_msb);
		return -EINVAL;
	}

	chip->cl.init_cc_uah = div64_s64(chip->cl.learned_cc_uah * batt_soc_msb,
					FULL_SOC_RAW);

	/* Prime cc_soc_sw with battery SOC when capacity learning begins */
	cc_soc_sw = div64_s64((int64_t)batt_soc * CC_SOC_30BIT,
				BATT_SOC_32BIT);
	rc = fg_sram_write(chip, chip->sp[FG_SRAM_CC_SOC_SW].addr_word,
		chip->sp[FG_SRAM_CC_SOC_SW].addr_byte, (u8 *)&cc_soc_sw,
		chip->sp[FG_SRAM_CC_SOC_SW].len, FG_IMA_ATOMIC);
	if (rc < 0) {
		pr_err("Error in writing cc_soc_sw, rc=%d\n", rc);
		goto out;
	}

	chip->cl.init_cc_soc_sw = cc_soc_sw;
	chip->cl.active = true;
	fg_dbg(chip, FG_CAP_LEARN, "Capacity learning started @ battery SOC %d init_cc_soc_sw:%d\n",
		batt_soc_msb, chip->cl.init_cc_soc_sw);
out:
	return rc;
}

static int fg_cap_learning_done(struct fg_chip *chip)
{
	int rc, cc_soc_sw;

	rc = fg_cap_learning_process_full_data(chip);
	if (rc < 0) {
		pr_err("Error in processing cap learning full data, rc=%d\n",
			rc);
		goto out;
	}

	/* Write a FULL value to cc_soc_sw */
	cc_soc_sw = CC_SOC_30BIT;
	rc = fg_sram_write(chip, chip->sp[FG_SRAM_CC_SOC_SW].addr_word,
		chip->sp[FG_SRAM_CC_SOC_SW].addr_byte, (u8 *)&cc_soc_sw,
		chip->sp[FG_SRAM_CC_SOC_SW].len, FG_IMA_ATOMIC);
	if (rc < 0) {
		pr_err("Error in writing cc_soc_sw, rc=%d\n", rc);
		goto out;
	}

	fg_cap_learning_post_process(chip);
out:
	return rc;
}

#define FULL_SOC_RAW	255
static void fg_cap_learning_update(struct fg_chip *chip)
{
	int rc, batt_soc, batt_soc_msb;

	mutex_lock(&chip->cl.lock);

	if (!is_temp_valid_cap_learning(chip) || !chip->cl.learned_cc_uah ||
		chip->battery_missing) {
		fg_dbg(chip, FG_CAP_LEARN, "Aborting cap_learning %lld\n",
			chip->cl.learned_cc_uah);
		chip->cl.active = false;
		chip->cl.init_cc_uah = 0;
		goto out;
	}

	rc = fg_get_sram_prop(chip, FG_SRAM_BATT_SOC, &batt_soc);
	if (rc < 0) {
		pr_err("Error in getting ACT_BATT_CAP, rc=%d\n", rc);
		goto out;
	}

	batt_soc_msb = (u32)batt_soc >> 24;
	fg_dbg(chip, FG_CAP_LEARN, "Chg_status: %d cl_active: %d batt_soc: %d\n",
		chip->charge_status, chip->cl.active, batt_soc_msb);

	/* Initialize the starting point of learning capacity */
	if (!chip->cl.active) {
		if (chip->charge_status == POWER_SUPPLY_STATUS_CHARGING || chip->charge_status == POWER_SUPPLY_STATUS_QUICK_CHARGING 
			|| chip->charge_status == POWER_SUPPLY_STATUS_NOT_QUICK_CHARGING) {
			rc = fg_cap_learning_begin(chip, batt_soc);
			chip->cl.active = (rc == 0);
		}

	} else {
		if (chip->charge_done) {
			rc = fg_cap_learning_done(chip);
			if (rc < 0)
				pr_err("Error in completing capacity learning, rc=%d\n",
					rc);

			chip->cl.active = false;
			chip->cl.init_cc_uah = 0;
		}

		if (chip->charge_status == POWER_SUPPLY_STATUS_NOT_CHARGING) {
			fg_dbg(chip, FG_CAP_LEARN, "Capacity learning aborted @ battery SOC %d\n",
				batt_soc_msb);
			chip->cl.active = false;
			chip->cl.init_cc_uah = 0;
		}
	}

out:
	mutex_unlock(&chip->cl.lock);
}

#define KI_COEFF_MED_DISCHG_DEFAULT	1500
#define KI_COEFF_HI_DISCHG_DEFAULT	2200
static int fg_adjust_ki_coeff_dischg(struct fg_chip *chip)
{
	int rc, i, msoc;
	int ki_coeff_med = KI_COEFF_MED_DISCHG_DEFAULT;
	int ki_coeff_hi = KI_COEFF_HI_DISCHG_DEFAULT;
	u8 val;

	if (!chip->ki_coeff_dischg_en)
		return 0;

	rc = fg_get_prop_capacity(chip, &msoc);
	if (rc < 0) {
		pr_err("Error in getting capacity, rc=%d\n", rc);
		return rc;
	}

	if (chip->charge_status == POWER_SUPPLY_STATUS_DISCHARGING) {
		for (i = KI_COEFF_SOC_LEVELS - 1; i >= 0; i--) {
			if (msoc < chip->dt.ki_coeff_soc[i]) {
				ki_coeff_med = chip->dt.ki_coeff_med_dischg[i];
				ki_coeff_hi = chip->dt.ki_coeff_hi_dischg[i];
			}
		}
	}

	fg_encode(chip->sp, FG_SRAM_KI_COEFF_MED_DISCHG, ki_coeff_med, &val);
	rc = fg_sram_write(chip,
			chip->sp[FG_SRAM_KI_COEFF_MED_DISCHG].addr_word,
			chip->sp[FG_SRAM_KI_COEFF_MED_DISCHG].addr_byte, &val,
			chip->sp[FG_SRAM_KI_COEFF_MED_DISCHG].len,
			FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in writing ki_coeff_med, rc=%d\n", rc);
		return rc;
	}

	fg_encode(chip->sp, FG_SRAM_KI_COEFF_HI_DISCHG, ki_coeff_hi, &val);
	rc = fg_sram_write(chip,
			chip->sp[FG_SRAM_KI_COEFF_HI_DISCHG].addr_word,
			chip->sp[FG_SRAM_KI_COEFF_HI_DISCHG].addr_byte, &val,
			chip->sp[FG_SRAM_KI_COEFF_HI_DISCHG].len,
			FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in writing ki_coeff_hi, rc=%d\n", rc);
		return rc;
	}

	fg_dbg(chip, FG_STATUS, "Wrote ki_coeff_med %d ki_coeff_hi %d\n",
		ki_coeff_med, ki_coeff_hi);
	return 0;
}

#define KI_COEFF_FULL_SOC_DEFAULT	733
static int fg_adjust_ki_coeff_full_soc(struct fg_chip *chip, int batt_temp)
{
	int rc, ki_coeff_full_soc;
	u8 val;

	if (batt_temp < 0)
		ki_coeff_full_soc = 0;
	else
		ki_coeff_full_soc = KI_COEFF_FULL_SOC_DEFAULT;

	if (chip->ki_coeff_full_soc == ki_coeff_full_soc)
		return 0;

	fg_encode(chip->sp, FG_SRAM_KI_COEFF_FULL_SOC, ki_coeff_full_soc, &val);
	rc = fg_sram_write(chip,
			chip->sp[FG_SRAM_KI_COEFF_FULL_SOC].addr_word,
			chip->sp[FG_SRAM_KI_COEFF_FULL_SOC].addr_byte, &val,
			chip->sp[FG_SRAM_KI_COEFF_FULL_SOC].len,
			FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in writing ki_coeff_full_soc, rc=%d\n", rc);
		return rc;
	}

	chip->ki_coeff_full_soc = ki_coeff_full_soc;
	fg_dbg(chip, FG_STATUS, "Wrote ki_coeff_full_soc %d\n",
		ki_coeff_full_soc);
	return 0;
}

static int fg_set_recharge_voltage(struct fg_chip *chip, int voltage_mv)
{
	u8 buf;
	int rc;

	if (chip->dt.auto_recharge_soc)
		return 0;

	/* This configuration is available only for pmicobalt v2.0 and above */
	if (chip->wa_flags & PMI8998_V1_REV_WA)
		return 0;

	fg_dbg(chip, FG_STATUS, "Setting recharge voltage to %dmV\n",
		voltage_mv);
	fg_encode(chip->sp, FG_SRAM_RECHARGE_VBATT_THR, voltage_mv, &buf);
	rc = fg_sram_write(chip,
			chip->sp[FG_SRAM_RECHARGE_VBATT_THR].addr_word,
			chip->sp[FG_SRAM_RECHARGE_VBATT_THR].addr_byte,
			&buf, chip->sp[FG_SRAM_RECHARGE_VBATT_THR].len,
			FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in writing recharge_vbatt_thr, rc=%d\n",
			rc);
		return rc;
	}

	return 0;
}

#define AUTO_RECHG_VOLT_LOW_LIMIT_MV	3700
#define 	MAINT_SOC_DELTA_LIMIT	20
//this should larger than 255 - DEFAULT_ASUS_FULL_MSOC_THR

static int fg_charge_full_update(struct fg_chip *chip)
{
	union power_supply_propval prop = {0, };
	int rc, msoc, bsoc, recharge_soc,msoc2;
	u8 full_soc[2] = {0xFF, 0xFF};

	if (!chip->dt.hold_soc_while_full)
		return 0;

	if (!batt_psy_initialized(chip))
		return 0;

	mutex_lock(&chip->charge_full_lock);
	if (!chip->charge_done && chip->bsoc_delta_irq_en) {
		disable_irq_wake(fg_irqs[BSOC_DELTA_IRQ].irq);
		disable_irq_nosync(fg_irqs[BSOC_DELTA_IRQ].irq);
		chip->bsoc_delta_irq_en = false;
	} else if (chip->charge_done && !chip->bsoc_delta_irq_en) {
		enable_irq(fg_irqs[BSOC_DELTA_IRQ].irq);
		enable_irq_wake(fg_irqs[BSOC_DELTA_IRQ].irq);
		chip->bsoc_delta_irq_en = true;
	}

	rc = power_supply_get_property(chip->batt_psy, POWER_SUPPLY_PROP_HEALTH,
		&prop);
	if (rc < 0) {
		pr_err("Error in getting battery health, rc=%d\n", rc);
		goto out;
	}

	chip->health = prop.intval;
	recharge_soc = chip->dt.recharge_soc_thr;
	recharge_soc = DIV_ROUND_CLOSEST(recharge_soc * FULL_SOC_RAW,
				FULL_CAPACITY);
	recharge_soc =  DEFAULT_ASUS_FULL_MSOC_THR; //cover it
	
	rc = fg_get_sram_prop(chip, FG_SRAM_BATT_SOC, &bsoc);
	if (rc < 0) {
		pr_err("Error in getting BATT_SOC, rc=%d\n", rc);
		goto out;
	}

	/* We need 2 most significant bytes here */
	bsoc = (u32)bsoc >> 16;
	rc = fg_get_msoc(chip, &msoc);
	if (rc < 0) {
		pr_err("Error in getting msoc, rc=%d\n", rc);
		goto out;
	}
	fg_get_msoc_raw(chip, &msoc2);
	
	BAT_DBG("msoc: %d bsoc: %d msoc_255: %d health: %d status: %d full: %d\n",
		msoc, bsoc >> 8, msoc2, chip->health, chip->charge_status,
		chip->charge_full);
	if (chip->charge_done && !chip->charge_full) {
		if (msoc >= 99 && chip->health == POWER_SUPPLY_HEALTH_GOOD) {
			BAT_DBG("Setting charge_full to true\n");
			chip->charge_full = true;
			/*
			 * Lower the recharge voltage so that VBAT_LT_RECHG
			 * signal will not be asserted soon.
			 */
			rc = fg_set_recharge_voltage(chip,
					AUTO_RECHG_VOLT_LOW_LIMIT_MV);
			if (rc < 0) {
				pr_err("Error in reducing recharge voltage, rc=%d\n",
					rc);
				goto out;
			}
		} else {
			fg_dbg(chip, FG_STATUS, "Terminated charging @ SOC%d\n",
				msoc);
		}
	} else if ((bsoc >> 8) <= recharge_soc && chip->charge_full) {
		
		chip->delta_soc = FULL_SOC_RAW - msoc2;

		/*
		 * We're spreading out the delta SOC over every 10% change
		 * in monotonic SOC. We cannot spread more than 9% in the
		 * range of 0-100 skipping the first 10%.
		 */
		if (chip->delta_soc > MAINT_SOC_DELTA_LIMIT) {
			chip->delta_soc = 0;
			chip->maint_soc = 0;
		} else {
			chip->maint_soc = FULL_SOC_RAW;
			chip->last_msoc = msoc2;
		}

		chip->charge_full = false;

		/*
		 * Raise the recharge voltage so that VBAT_LT_RECHG signal
		 * will be asserted soon as battery SOC had dropped below
		 * the recharge SOC threshold.
		 */
		rc = fg_set_recharge_voltage(chip,
					chip->dt.recharge_volt_thr_mv);
		if (rc < 0) {
			pr_err("Error in setting recharge voltage, rc=%d\n",
				rc);
			goto out;
		}

		BAT_DBG("bsoc: %d recharge_soc: %d delta_soc: %d\n",
			bsoc >> 8, recharge_soc, chip->delta_soc);
	} else {
		goto out;
	}

	if (!chip->charge_full)
		goto out;

	/*
	 * During JEITA conditions, charge_full can happen early. FULL_SOC
	 * and MONOTONIC_SOC needs to be updated to reflect the same. Write
	 * battery SOC to FULL_SOC and write a full value to MONOTONIC_SOC.
	 */
	rc = fg_sram_write(chip, FULL_SOC_WORD, FULL_SOC_OFFSET, (u8 *)&bsoc, 2,
			FG_IMA_ATOMIC);
	if (rc < 0) {
		pr_err("failed to write full_soc rc=%d\n", rc);
		goto out;
	}

	rc = fg_sram_write(chip, MONOTONIC_SOC_WORD, MONOTONIC_SOC_OFFSET,
			full_soc, 2, FG_IMA_ATOMIC);
	if (rc < 0) {
		pr_err("failed to write monotonic_soc rc=%d\n", rc);
		goto out;
	}

	fg_dbg(chip, FG_STATUS, "Set charge_full to true @ soc %d\n", msoc);
out:
	mutex_unlock(&chip->charge_full_lock);
	return rc;
}

#define RCONN_CONFIG_BIT	BIT(0)
static int fg_rconn_config(struct fg_chip *chip)
{
	int rc, esr_uohms;
	u64 scaling_factor;
	u32 val = 0;

	if (!chip->dt.rconn_mohms)
		return 0;

	rc = fg_sram_read(chip, PROFILE_INTEGRITY_WORD,
			SW_CONFIG_OFFSET, (u8 *)&val, 1, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in reading SW_CONFIG_OFFSET, rc=%d\n", rc);
		return rc;
	}

	if (val & RCONN_CONFIG_BIT) {
		fg_dbg(chip, FG_STATUS, "Rconn already configured: %x\n", val);
		return 0;
	}

	rc = fg_get_sram_prop(chip, FG_SRAM_ESR, &esr_uohms);
	if (rc < 0) {
		pr_err("failed to get ESR, rc=%d\n", rc);
		return rc;
	}

	scaling_factor = div64_u64((u64)esr_uohms * 1000,
				esr_uohms + (chip->dt.rconn_mohms * 1000));

	rc = fg_sram_read(chip, ESR_RSLOW_CHG_WORD,
			ESR_RSLOW_CHG_OFFSET, (u8 *)&val, 1, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in reading ESR_RSLOW_CHG_OFFSET, rc=%d\n", rc);
		return rc;
	}

	val *= scaling_factor;
	do_div(val, 1000);
	rc = fg_sram_write(chip, ESR_RSLOW_CHG_WORD,
			ESR_RSLOW_CHG_OFFSET, (u8 *)&val, 1, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in writing ESR_RSLOW_CHG_OFFSET, rc=%d\n", rc);
		return rc;
	}
	fg_dbg(chip, FG_STATUS, "esr_rslow_chg modified to %x\n", val & 0xFF);

	rc = fg_sram_read(chip, ESR_RSLOW_DISCHG_WORD,
			ESR_RSLOW_DISCHG_OFFSET, (u8 *)&val, 1, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in reading ESR_RSLOW_DISCHG_OFFSET, rc=%d\n", rc);
		return rc;
	}

	val *= scaling_factor;
	do_div(val, 1000);
	rc = fg_sram_write(chip, ESR_RSLOW_DISCHG_WORD,
			ESR_RSLOW_DISCHG_OFFSET, (u8 *)&val, 1, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in writing ESR_RSLOW_DISCHG_OFFSET, rc=%d\n", rc);
		return rc;
	}
	fg_dbg(chip, FG_STATUS, "esr_rslow_dischg modified to %x\n",
		val & 0xFF);

	val = RCONN_CONFIG_BIT;
	rc = fg_sram_write(chip, PROFILE_INTEGRITY_WORD,
			SW_CONFIG_OFFSET, (u8 *)&val, 1, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in writing SW_CONFIG_OFFSET, rc=%d\n", rc);
		return rc;
	}

	return 0;
}

static int fg_set_constant_chg_voltage(struct fg_chip *chip, int volt_uv)
{
	u8 buf[2];
	int rc;

	if (volt_uv <= 0 || volt_uv > 15590000) {
		pr_err("Invalid voltage %d\n", volt_uv);
		return -EINVAL;
	}

	fg_encode(chip->sp, FG_SRAM_VBATT_FULL, volt_uv, buf);

	rc = fg_sram_write(chip, chip->sp[FG_SRAM_VBATT_FULL].addr_word,
		chip->sp[FG_SRAM_VBATT_FULL].addr_byte, buf,
		chip->sp[FG_SRAM_VBATT_FULL].len, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in writing vbatt_full, rc=%d\n", rc);
		return rc;
	}

	return 0;
}

static int fg_set_recharge_soc(struct fg_chip *chip, int recharge_soc)
{
	u8 buf;
	int rc;

	if (!chip->dt.auto_recharge_soc)
		return 0;

	if (recharge_soc < 0 || recharge_soc > FULL_CAPACITY)
		return 0;

	fg_encode(chip->sp, FG_SRAM_RECHARGE_SOC_THR, recharge_soc, &buf);
	rc = fg_sram_write(chip,
			chip->sp[FG_SRAM_RECHARGE_SOC_THR].addr_word,
			chip->sp[FG_SRAM_RECHARGE_SOC_THR].addr_byte, &buf,
			chip->sp[FG_SRAM_RECHARGE_SOC_THR].len, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in writing recharge_soc_thr, rc=%d\n", rc);
		return rc;
	}

	return 0;
}

static int fg_adjust_recharge_soc(struct fg_chip *chip)
{
	int rc, msoc, recharge_soc, new_recharge_soc = 0;
	bool recharge_soc_status;

	if (!chip->dt.auto_recharge_soc)
		return 0;

	recharge_soc = chip->dt.recharge_soc_thr;
	recharge_soc_status = chip->recharge_soc_adjusted;
	/*
	 * If the input is present and charging had been terminated, adjust
	 * the recharge SOC threshold based on the monotonic SOC at which
	 * the charge termination had happened.
	 */
	if (is_input_present(chip)) {
		if (chip->charge_done) {
			if (!chip->recharge_soc_adjusted) {
				/* Get raw monotonic SOC for calculation */
				rc = fg_get_msoc(chip, &msoc);
				if (rc < 0) {
					pr_err("Error in getting msoc, rc=%d\n",
						rc);
					return rc;
				}

				/* Adjust the recharge_soc threshold */
				new_recharge_soc = msoc - (FULL_CAPACITY -
								recharge_soc);
				chip->recharge_soc_adjusted = true;
			} else {
				/* adjusted already, do nothing */
				return 0;
			}
		} else {
			/* Charging, do nothing */
			return 0;
		}
	} else {
		/* Restore the default value */
		new_recharge_soc = recharge_soc;
		chip->recharge_soc_adjusted = false;
	}

	rc = fg_set_recharge_soc(chip, new_recharge_soc);
	if (rc < 0) {
		chip->recharge_soc_adjusted = recharge_soc_status;
		pr_err("Couldn't set resume SOC for FG, rc=%d\n", rc);
		return rc;
	}

	fg_dbg(chip, FG_STATUS, "resume soc set to %d\n", new_recharge_soc);
	return 0;
}

static int fg_slope_limit_config(struct fg_chip *chip, int batt_temp)
{
	enum slope_limit_status status;
	int rc;
	u8 buf;

	if (!chip->slope_limit_en)
		return 0;

	if (chip->charge_status == POWER_SUPPLY_STATUS_CHARGING || chip->charge_status == POWER_SUPPLY_STATUS_QUICK_CHARGING 
		|| chip->charge_status == POWER_SUPPLY_STATUS_NOT_QUICK_CHARGING || chip->charge_status == POWER_SUPPLY_STATUS_FULL) {
		if (batt_temp < chip->dt.slope_limit_temp)
			status = LOW_TEMP_CHARGE;
		else
			status = HIGH_TEMP_CHARGE;
	} else {
		if (batt_temp < chip->dt.slope_limit_temp)
			status = LOW_TEMP_DISCHARGE;
		else
			status = HIGH_TEMP_DISCHARGE;
	}

	if (chip->slope_limit_sts == status)
		return 0;

	fg_encode(chip->sp, FG_SRAM_SLOPE_LIMIT,
		chip->dt.slope_limit_coeffs[status], &buf);
	rc = fg_sram_write(chip, chip->sp[FG_SRAM_SLOPE_LIMIT].addr_word,
			chip->sp[FG_SRAM_SLOPE_LIMIT].addr_byte, &buf,
			chip->sp[FG_SRAM_SLOPE_LIMIT].len, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in configuring slope_limit coefficient, rc=%d\n",
			rc);
		return rc;
	}

	chip->slope_limit_sts = status;
	fg_dbg(chip, FG_STATUS, "Slope limit status: %d value: %x\n", status,
		buf);
	return 0;
}

static int fg_esr_filter_config(struct fg_chip *chip, int batt_temp)
{
	u8 esr_tight_lt_flt, esr_broad_lt_flt;
	bool cold_temp = false;
	int rc;

	/*
	 * If the battery temperature is lower than -20 C, then skip modifying
	 * ESR filter.
	 */
	if (batt_temp < -210)
		return 0;

	/*
	 * If battery temperature is lesser than 10 C (default), then apply the
	 * ESR low temperature tight and broad filter values to ESR room
	 * temperature tight and broad filters. If battery temperature is higher
	 * than 10 C, then apply back the room temperature ESR filter
	 * coefficients to ESR room temperature tight and broad filters.
	 */
	if (batt_temp > chip->dt.esr_flt_switch_temp
		&& chip->esr_flt_cold_temp_en) {
		fg_encode(chip->sp, FG_SRAM_ESR_TIGHT_FILTER,
			chip->dt.esr_tight_flt_upct, &esr_tight_lt_flt);
		fg_encode(chip->sp, FG_SRAM_ESR_BROAD_FILTER,
			chip->dt.esr_broad_flt_upct, &esr_broad_lt_flt);
	} else if (batt_temp <= chip->dt.esr_flt_switch_temp
			&& !chip->esr_flt_cold_temp_en) {
		fg_encode(chip->sp, FG_SRAM_ESR_TIGHT_FILTER,
			chip->dt.esr_tight_lt_flt_upct, &esr_tight_lt_flt);
		fg_encode(chip->sp, FG_SRAM_ESR_BROAD_FILTER,
			chip->dt.esr_broad_lt_flt_upct, &esr_broad_lt_flt);
		cold_temp = true;
	} else {
		return 0;
	}

	rc = fg_sram_write(chip, chip->sp[FG_SRAM_ESR_TIGHT_FILTER].addr_word,
			chip->sp[FG_SRAM_ESR_TIGHT_FILTER].addr_byte,
			&esr_tight_lt_flt,
			chip->sp[FG_SRAM_ESR_TIGHT_FILTER].len, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in writing ESR LT tight filter, rc=%d\n", rc);
		return rc;
	}

	rc = fg_sram_write(chip, chip->sp[FG_SRAM_ESR_BROAD_FILTER].addr_word,
			chip->sp[FG_SRAM_ESR_BROAD_FILTER].addr_byte,
			&esr_broad_lt_flt,
			chip->sp[FG_SRAM_ESR_BROAD_FILTER].len, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in writing ESR LT broad filter, rc=%d\n", rc);
		return rc;
	}

	chip->esr_flt_cold_temp_en = cold_temp;
	fg_dbg(chip, FG_STATUS, "applied %s ESR filter values\n",
		cold_temp ? "cold" : "normal");
	return 0;
}

static int fg_esr_fcc_config(struct fg_chip *chip)
{
	union power_supply_propval prop = {0, };
	int rc;
	bool parallel_en = false;

	if (is_parallel_charger_available(chip)) {
		rc = power_supply_get_property(chip->parallel_psy,
			POWER_SUPPLY_PROP_CHARGING_ENABLED, &prop);
		if (rc < 0) {
			pr_err("Error in reading charging_enabled from parallel_psy, rc=%d\n",
				rc);
			return rc;
		}
		parallel_en = prop.intval;
	}

	fg_dbg(chip, FG_POWER_SUPPLY, "charge_status: %d parallel_en: %d esr_fcc_ctrl_en: %d\n",
		chip->charge_status, parallel_en, chip->esr_fcc_ctrl_en);

	if ((chip->charge_status == POWER_SUPPLY_STATUS_CHARGING || chip->charge_status == POWER_SUPPLY_STATUS_QUICK_CHARGING 
		|| chip->charge_status == POWER_SUPPLY_STATUS_NOT_QUICK_CHARGING) && parallel_en) {
		if (chip->esr_fcc_ctrl_en)
			return 0;

		/*
		 * When parallel charging is enabled, configure ESR FCC to
		 * 300mA to trigger an ESR pulse. Without this, FG can ask
		 * the main charger to increase FCC when it is supposed to
		 * decrease it.
		 */
		rc = fg_masked_write(chip, BATT_INFO_ESR_FAST_CRG_CFG(chip),
				ESR_FAST_CRG_IVAL_MASK |
				ESR_FAST_CRG_CTL_EN_BIT,
				ESR_FCC_300MA | ESR_FAST_CRG_CTL_EN_BIT);
		if (rc < 0) {
			pr_err("Error in writing to %04x, rc=%d\n",
				BATT_INFO_ESR_FAST_CRG_CFG(chip), rc);
			return rc;
		}

		chip->esr_fcc_ctrl_en = true;
	} else {
		if (!chip->esr_fcc_ctrl_en)
			return 0;

		/*
		 * If we're here, then it means either the device is not in
		 * charging state or parallel charging is disabled. Disable
		 * ESR fast charge current control in SW.
		 */
		rc = fg_masked_write(chip, BATT_INFO_ESR_FAST_CRG_CFG(chip),
				ESR_FAST_CRG_CTL_EN_BIT, 0);
		if (rc < 0) {
			pr_err("Error in writing to %04x, rc=%d\n",
				BATT_INFO_ESR_FAST_CRG_CFG(chip), rc);
			return rc;
		}

		chip->esr_fcc_ctrl_en = false;
	}

	fg_dbg(chip, FG_STATUS, "esr_fcc_ctrl_en set to %d\n",
		chip->esr_fcc_ctrl_en);
	return 0;
}

static void fg_batt_avg_update(struct fg_chip *chip)
{
	if (chip->charge_status == chip->prev_charge_status)
		return;

	cancel_delayed_work_sync(&chip->batt_avg_work);
	fg_circ_buf_clr(&chip->ibatt_circ_buf);
	fg_circ_buf_clr(&chip->vbatt_circ_buf);

	if (chip->charge_status == POWER_SUPPLY_STATUS_CHARGING || chip->charge_status == POWER_SUPPLY_STATUS_QUICK_CHARGING 
		|| chip->charge_status == POWER_SUPPLY_STATUS_NOT_QUICK_CHARGING || chip->charge_status == POWER_SUPPLY_STATUS_DISCHARGING)
		schedule_delayed_work(&chip->batt_avg_work,
							msecs_to_jiffies(2000));
}

static void status_change_work(struct work_struct *work)
{
	struct fg_chip *chip = container_of(work,
			struct fg_chip, status_change_work);
	union power_supply_propval prop = {0, };
	int rc, batt_temp;

	if (!batt_psy_initialized(chip)) {
		fg_dbg(chip, FG_STATUS, "Charger not available?!\n");
		goto out;
	}

	mutex_lock(&chip->charge_status_lock);
	gauge_get_prop = 1;
	mutex_unlock(&chip->charge_status_lock);
	
	rc = power_supply_get_property(chip->batt_psy, POWER_SUPPLY_PROP_STATUS,
			&prop);
	if (rc < 0) {
		pr_err("Error in getting charging status, rc=%d\n", rc);
		
		goto out;
	}

	chip->prev_charge_status = chip->charge_status;
	chip->charge_status = prop.intval;
	rc = power_supply_get_property(chip->batt_psy,
			POWER_SUPPLY_PROP_CHARGE_TYPE, &prop);
	if (rc < 0) {
		pr_err("Error in getting charge type, rc=%d\n", rc);
		goto out;
	}

	chip->charge_type = prop.intval;
	rc = power_supply_get_property(chip->batt_psy,
			POWER_SUPPLY_PROP_CHARGE_DONE, &prop);
	if (rc < 0) {
		pr_err("Error in getting charge_done, rc=%d\n", rc);
		goto out;
	}

	chip->charge_done = prop.intval;
	if (chip->cyc_ctr.en)
		schedule_work(&chip->cycle_count_work);

	fg_cap_learning_update(chip);

	rc = fg_charge_full_update(chip);
	if (rc < 0)
		pr_err("Error in charge_full_update, rc=%d\n", rc);

	rc = fg_adjust_recharge_soc(chip);
	if (rc < 0)
		pr_err("Error in adjusting recharge_soc, rc=%d\n", rc);

	rc = fg_adjust_ki_coeff_dischg(chip);
	if (rc < 0)
		pr_err("Error in adjusting ki_coeff_dischg, rc=%d\n", rc);

	rc = fg_esr_fcc_config(chip);
	if (rc < 0)
		pr_err("Error in adjusting FCC for ESR, rc=%d\n", rc);

	rc = fg_get_battery_temp(chip, &batt_temp);
	if (!rc) {
		rc = fg_slope_limit_config(chip, batt_temp);
		if (rc < 0)
			pr_err("Error in configuring slope limiter rc:%d\n",
				rc);

		rc = fg_adjust_ki_coeff_full_soc(chip, batt_temp);
		if (rc < 0)
			pr_err("Error in configuring ki_coeff_full_soc rc:%d\n",
				rc);
	}

	fg_batt_avg_update(chip);

out:
	fg_dbg(chip, FG_POWER_SUPPLY, "charge_status:%d charge_type:%d charge_done:%d\n",
		chip->charge_status, chip->charge_type, chip->charge_done);
	pm_relax(chip->dev);
}

static void restore_cycle_counter(struct fg_chip *chip)
{
	int rc = 0, i;
	u8 data[2];

	mutex_lock(&chip->cyc_ctr.lock);
	for (i = 0; i < BUCKET_COUNT; i++) {
		rc = fg_sram_read(chip, CYCLE_COUNT_WORD + (i / 2),
				CYCLE_COUNT_OFFSET + (i % 2) * 2, data, 2,
				FG_IMA_DEFAULT);
		if (rc < 0)
			pr_err("failed to read bucket %d rc=%d\n", i, rc);
		else
			chip->cyc_ctr.count[i] = data[0] | data[1] << 8;
	}
	mutex_unlock(&chip->cyc_ctr.lock);
}

static void clear_cycle_counter(struct fg_chip *chip)
{
	int rc = 0, i;

	if (!chip->cyc_ctr.en)
		return;

	mutex_lock(&chip->cyc_ctr.lock);
	memset(chip->cyc_ctr.count, 0, sizeof(chip->cyc_ctr.count));
	for (i = 0; i < BUCKET_COUNT; i++) {
		chip->cyc_ctr.started[i] = false;
		chip->cyc_ctr.last_soc[i] = 0;
	}
	rc = fg_sram_write(chip, CYCLE_COUNT_WORD, CYCLE_COUNT_OFFSET,
			(u8 *)&chip->cyc_ctr.count,
			sizeof(chip->cyc_ctr.count) / sizeof(u8 *),
			FG_IMA_DEFAULT);
	if (rc < 0)
		pr_err("failed to clear cycle counter rc=%d\n", rc);

	mutex_unlock(&chip->cyc_ctr.lock);
}

static int fg_inc_store_cycle_ctr(struct fg_chip *chip, int bucket)
{
	int rc = 0;
	u16 cyc_count;
	u8 data[2];

	if (bucket < 0 || (bucket > BUCKET_COUNT - 1))
		return 0;

	cyc_count = chip->cyc_ctr.count[bucket];
	cyc_count++;
	data[0] = cyc_count & 0xFF;
	data[1] = cyc_count >> 8;

	rc = fg_sram_write(chip, CYCLE_COUNT_WORD + (bucket / 2),
			CYCLE_COUNT_OFFSET + (bucket % 2) * 2, data, 2,
			FG_IMA_DEFAULT);
	if (rc < 0)
		pr_err("failed to write BATT_CYCLE[%d] rc=%d\n",
			bucket, rc);
	else
		chip->cyc_ctr.count[bucket] = cyc_count;
	return rc;
}

static void cycle_count_work(struct work_struct *work)
{
	int rc = 0, bucket, i, batt_soc;
	struct fg_chip *chip = container_of(work,
				struct fg_chip,
				cycle_count_work);

	mutex_lock(&chip->cyc_ctr.lock);
	rc = fg_get_sram_prop(chip, FG_SRAM_BATT_SOC, &batt_soc);
	if (rc < 0) {
		pr_err("Failed to read battery soc rc: %d\n", rc);
		goto out;
	}

	/* We need only the most significant byte here */
	batt_soc = (u32)batt_soc >> 24;

	if (chip->charge_status == POWER_SUPPLY_STATUS_CHARGING || chip->charge_status == POWER_SUPPLY_STATUS_QUICK_CHARGING 
		|| chip->charge_status == POWER_SUPPLY_STATUS_NOT_QUICK_CHARGING) {
		/* Find out which bucket the SOC falls in */
		bucket = batt_soc / BUCKET_SOC_PCT;
		pr_debug("batt_soc: %d bucket: %d\n", batt_soc, bucket);

		/*
		 * If we've started counting for the previous bucket,
		 * then store the counter for that bucket if the
		 * counter for current bucket is getting started.
		 */
		if (bucket > 0 && chip->cyc_ctr.started[bucket - 1] &&
			!chip->cyc_ctr.started[bucket]) {
			rc = fg_inc_store_cycle_ctr(chip, bucket - 1);
			if (rc < 0) {
				pr_err("Error in storing cycle_ctr rc: %d\n",
					rc);
				goto out;
			} else {
				chip->cyc_ctr.started[bucket - 1] = false;
				chip->cyc_ctr.last_soc[bucket - 1] = 0;
			}
		}
		if (!chip->cyc_ctr.started[bucket]) {
			chip->cyc_ctr.started[bucket] = true;
			chip->cyc_ctr.last_soc[bucket] = batt_soc;
		}
	} else {
		for (i = 0; i < BUCKET_COUNT; i++) {
			if (chip->cyc_ctr.started[i] &&
				batt_soc > chip->cyc_ctr.last_soc[i]) {
				rc = fg_inc_store_cycle_ctr(chip, i);
				if (rc < 0)
					pr_err("Error in storing cycle_ctr rc: %d\n",
						rc);
				chip->cyc_ctr.last_soc[i] = 0;
			}
			chip->cyc_ctr.started[i] = false;
		}
	}
out:
	mutex_unlock(&chip->cyc_ctr.lock);
}

static int fg_get_cycle_count(struct fg_chip *chip)
{
	int count;

	if (!chip->cyc_ctr.en)
		return 0;

	if ((chip->cyc_ctr.id <= 0) || (chip->cyc_ctr.id > BUCKET_COUNT))
		return -EINVAL;

	mutex_lock(&chip->cyc_ctr.lock);
	count = chip->cyc_ctr.count[chip->cyc_ctr.id - 1];
	mutex_unlock(&chip->cyc_ctr.lock);
	return count;
}

#define PROFILE_LOAD_BIT	BIT(0)
#define BOOTLOADER_LOAD_BIT	BIT(1)
#define BOOTLOADER_RESTART_BIT	BIT(2)
#define HLOS_RESTART_BIT	BIT(3)
static bool is_profile_load_required(struct fg_chip *chip)
{
	u8 buf[PROFILE_COMP_LEN], val;
	bool profiles_same = false;
	int rc;

	rc = fg_sram_read(chip, PROFILE_INTEGRITY_WORD,
			PROFILE_INTEGRITY_OFFSET, &val, 1, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("failed to read profile integrity rc=%d\n", rc);
		return false;
	}

	/* Check if integrity bit is set */
	if (val & PROFILE_LOAD_BIT) {
		fg_dbg(chip, FG_STATUS, "Battery profile integrity bit is set\n");
		rc = fg_sram_read(chip, PROFILE_LOAD_WORD, PROFILE_LOAD_OFFSET,
				buf, PROFILE_COMP_LEN, FG_IMA_DEFAULT);
		if (rc < 0) {
			pr_err("Error in reading battery profile, rc:%d\n", rc);
			return false;
		}
		profiles_same = memcmp(chip->batt_profile, buf,
					PROFILE_COMP_LEN) == 0;
		if (profiles_same) {
			fg_dbg(chip, FG_STATUS, "Battery profile is same, not loading it\n");
			return false;
		}

		if (!chip->dt.force_load_profile) {
			pr_warn("Profiles doesn't match, skipping loading it since force_load_profile is disabled\n");
			if (fg_profile_dump) {
				pr_info("FG: loaded profile:\n");
				dump_sram(buf, PROFILE_LOAD_WORD,
					PROFILE_COMP_LEN);
				pr_info("FG: available profile:\n");
				dump_sram(chip->batt_profile, PROFILE_LOAD_WORD,
					PROFILE_LEN);
			}
			return false;
		}

		fg_dbg(chip, FG_STATUS, "Profiles are different, loading the correct one\n");
	} else {
		fg_dbg(chip, FG_STATUS, "Profile integrity bit is not set\n");
		if (fg_profile_dump) {
			pr_info("FG: profile to be loaded:\n");
			dump_sram(chip->batt_profile, PROFILE_LOAD_WORD,
				PROFILE_LEN);
		}
	}
	return true;
}

static void clear_battery_profile(struct fg_chip *chip)
{
	u8 val = 0;
	int rc;

	rc = fg_sram_write(chip, PROFILE_INTEGRITY_WORD,
			PROFILE_INTEGRITY_OFFSET, &val, 1, FG_IMA_DEFAULT);
	if (rc < 0)
		pr_err("failed to write profile integrity rc=%d\n", rc);
}

#define SOC_READY_WAIT_MS		2000
static int __fg_restart(struct fg_chip *chip)
{
	int rc, msoc;
	bool tried_again = false;

	rc = fg_get_prop_capacity(chip, &msoc);
	if (rc < 0) {
		pr_err("Error in getting capacity, rc=%d\n", rc);
		return rc;
	}

	chip->last_soc = msoc;
	chip->fg_restarting = true;
	reinit_completion(&chip->soc_ready);
	rc = fg_masked_write(chip, BATT_SOC_RESTART(chip), RESTART_GO_BIT,
			RESTART_GO_BIT);
	if (rc < 0) {
		pr_err("Error in writing to %04x, rc=%d\n",
			BATT_SOC_RESTART(chip), rc);
		goto out;
	}

wait:
	rc = wait_for_completion_interruptible_timeout(&chip->soc_ready,
		msecs_to_jiffies(SOC_READY_WAIT_MS));

	/* If we were interrupted wait again one more time. */
	if (rc == -ERESTARTSYS && !tried_again) {
		tried_again = true;
		goto wait;
	} else if (rc <= 0) {
		pr_err("wait for soc_ready timed out rc=%d\n", rc);
	}

	rc = fg_masked_write(chip, BATT_SOC_RESTART(chip), RESTART_GO_BIT, 0);
	if (rc < 0) {
		pr_err("Error in writing to %04x, rc=%d\n",
			BATT_SOC_RESTART(chip), rc);
		goto out;
	}
out:
	chip->fg_restarting = false;
	return rc;
}

static void profile_load_work(struct work_struct *work)
{
	struct fg_chip *chip = container_of(work,
				struct fg_chip,
				profile_load_work.work);
	u8 buf[2], val;
	int rc;

	vote(chip->awake_votable, PROFILE_LOAD, true, 0);
	if (!is_profile_load_required(chip))
		goto done;

	clear_cycle_counter(chip);
	mutex_lock(&chip->cl.lock);
	chip->cl.learned_cc_uah = 0;
	chip->cl.active = false;
	mutex_unlock(&chip->cl.lock);

	fg_dbg(chip, FG_STATUS, "profile loading started\n");
	rc = fg_masked_write(chip, BATT_SOC_RESTART(chip), RESTART_GO_BIT, 0);
	if (rc < 0) {
		pr_err("Error in writing to %04x, rc=%d\n",
			BATT_SOC_RESTART(chip), rc);
		goto out;
	}

	/* load battery profile */
	rc = fg_sram_write(chip, PROFILE_LOAD_WORD, PROFILE_LOAD_OFFSET,
			chip->batt_profile, PROFILE_LEN, FG_IMA_ATOMIC);
	if (rc < 0) {
		pr_err("Error in writing battery profile, rc:%d\n", rc);
		goto out;
	}

	rc = __fg_restart(chip);
	if (rc < 0) {
		pr_err("Error in restarting FG, rc=%d\n", rc);
		goto out;
	}

	fg_dbg(chip, FG_STATUS, "SOC is ready\n");

	/* Set the profile integrity bit */
	val = HLOS_RESTART_BIT | PROFILE_LOAD_BIT;
	rc = fg_sram_write(chip, PROFILE_INTEGRITY_WORD,
			PROFILE_INTEGRITY_OFFSET, &val, 1, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("failed to write profile integrity rc=%d\n", rc);
		goto out;
	}

done:
	rc = fg_sram_read(chip, NOM_CAP_WORD, NOM_CAP_OFFSET, buf, 2,
			FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in reading %04x[%d] rc=%d\n", NOM_CAP_WORD,
			NOM_CAP_OFFSET, rc);
	} else {
		chip->cl.nom_cap_uah = (int)(buf[0] | buf[1] << 8) * 1000;
		BAT_DBG("nominal capacity %lld mah \n",chip->cl.nom_cap_uah/1000);
		rc = fg_load_learned_cap_from_sram(chip);
		if (rc < 0)
			pr_err("Error in loading capacity learning data, rc:%d\n",
				rc);
	}

	batt_psy_initialized(chip);
	fg_notify_charger(chip);
	chip->profile_loaded = true;
	chip->soc_reporting_ready = true;
	fg_dbg(chip, FG_STATUS, "profile loaded successfully");
out:
	vote(chip->awake_votable, PROFILE_LOAD, false, 0);
}



static void sram_dump_work(struct work_struct *work)
{
	struct fg_chip *chip = container_of(work, struct fg_chip,
					    sram_dump_work.work);
	u8 buf[FG_SRAM_LEN];
	int rc;
	s64 timestamp_ms, quotient;
	s32 remainder;

	rc = fg_sram_read(chip, 0, 0, buf, FG_SRAM_LEN, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in reading FG SRAM, rc:%d\n", rc);
		goto resched;
	}

	timestamp_ms = ktime_to_ms(ktime_get_boottime());
	quotient = div_s64_rem(timestamp_ms, 1000, &remainder);
	fg_dbg(chip, FG_STATUS, "SRAM Dump Started at %lld.%d\n",
		quotient, remainder);
	dump_sram(buf, 0, FG_SRAM_LEN);
	timestamp_ms = ktime_to_ms(ktime_get_boottime());
	quotient = div_s64_rem(timestamp_ms, 1000, &remainder);
	fg_dbg(chip, FG_STATUS, "SRAM Dump done at %lld.%d\n",
		quotient, remainder);
resched:
	schedule_delayed_work(&chip->sram_dump_work,
			msecs_to_jiffies(fg_sram_dump_period_ms));
}

static int fg_sram_dump_sysfs(const char *val, const struct kernel_param *kp)
{
	int rc;
	struct power_supply *bms_psy;
	struct fg_chip *chip;
	bool old_val = fg_sram_dump;

	rc = param_set_bool(val, kp);
	if (rc) {
		pr_err("Unable to set fg_sram_dump: %d\n", rc);
		return rc;
	}

	if (fg_sram_dump == old_val)
		return 0;

	bms_psy = power_supply_get_by_name("bms");
	if (!bms_psy) {
		pr_err("bms psy not found\n");
		return -ENODEV;
	}

	chip = power_supply_get_drvdata(bms_psy);
	if (fg_sram_dump)
		schedule_delayed_work(&chip->sram_dump_work,
				msecs_to_jiffies(fg_sram_dump_period_ms));
	else
		cancel_delayed_work_sync(&chip->sram_dump_work);

	return 0;
}

static struct kernel_param_ops fg_sram_dump_ops = {
	.set = fg_sram_dump_sysfs,
	.get = param_get_bool,
};

module_param_cb(sram_dump_en, &fg_sram_dump_ops, &fg_sram_dump, 0644);

static int fg_restart_sysfs(const char *val, const struct kernel_param *kp)
{
	int rc;
	struct power_supply *bms_psy;
	struct fg_chip *chip;

	rc = param_set_int(val, kp);
	if (rc) {
		pr_err("Unable to set fg_restart: %d\n", rc);
		return rc;
	}

	if (fg_restart != 1) {
		pr_err("Bad value %d\n", fg_restart);
		return -EINVAL;
	}


		bms_psy = power_supply_get_by_name("bms");
		if (!bms_psy) {
			pr_err("bms psy not found\n");
			return 0;
		}

		chip = power_supply_get_drvdata(bms_psy);
		rc = __fg_restart(chip);
		if (rc < 0) {
			pr_err("Error in restarting FG, rc=%d\n", rc);
			return rc;
		}

		BAT_DBG("%s: FG restart done\n", __func__);

	
	return rc;
}

static struct kernel_param_ops fg_restart_ops = {
	.set = fg_restart_sysfs,
	.get = param_get_int,
};

module_param_cb(restart, &fg_restart_ops, &fg_restart, 0644);

#define BATT_AVG_POLL_PERIOD_MS	10000
static void batt_avg_work(struct work_struct *work)
{
	struct fg_chip *chip = container_of(work, struct fg_chip,
					    batt_avg_work.work);
	int rc, ibatt_now, vbatt_now;

	mutex_lock(&chip->batt_avg_lock);
	rc = fg_get_battery_current(chip, &ibatt_now);
	if (rc < 0) {
		pr_err("failed to get battery current, rc=%d\n", rc);
		goto reschedule;
	}

	rc = fg_get_battery_voltage(chip, &vbatt_now);
	if (rc < 0) {
		pr_err("failed to get battery voltage, rc=%d\n", rc);
		goto reschedule;
	}

	fg_circ_buf_add(&chip->ibatt_circ_buf, ibatt_now);
	fg_circ_buf_add(&chip->vbatt_circ_buf, vbatt_now);

reschedule:
	mutex_unlock(&chip->batt_avg_lock);
	schedule_delayed_work(&chip->batt_avg_work,
			      msecs_to_jiffies(BATT_AVG_POLL_PERIOD_MS));
}

#define HOURS_TO_SECONDS	3600
#define OCV_SLOPE_UV		10869
#define MILLI_UNIT		1000
#define MICRO_UNIT		1000000
static int fg_get_time_to_full(struct fg_chip *chip, int *val)
{
	int rc, ibatt_avg, vbatt_avg, rbatt, msoc, ocv_cc2cv, full_soc,
		act_cap_uah;
	s32 i_cc2cv, soc_cc2cv, ln_val, centi_tau_scale;
	s64 t_predicted_cc = 0, t_predicted_cv = 0;

	if (chip->bp.float_volt_uv <= 0) {
		pr_err("battery profile is not loaded\n");
		return -ENODATA;
	}

	if (!batt_psy_initialized(chip)) {
		fg_dbg(chip, FG_TTF, "charger is not available\n");
		return -ENODATA;
	}

	rc = fg_get_prop_capacity(chip, &msoc);
	if (rc < 0) {
		pr_err("failed to get msoc rc=%d\n", rc);
		return rc;
	}
	fg_dbg(chip, FG_TTF, "msoc=%d\n", msoc);

	if (msoc >= 100) {
		*val = 0;
		return 0;
	}

	mutex_lock(&chip->batt_avg_lock);
	rc = fg_circ_buf_avg(&chip->ibatt_circ_buf, &ibatt_avg);
	if (rc < 0) {
		/* try to get instantaneous current */
		rc = fg_get_battery_current(chip, &ibatt_avg);
		if (rc < 0) {
			mutex_unlock(&chip->batt_avg_lock);
			pr_err("failed to get battery current, rc=%d\n", rc);
			return rc;
		}
	}

	rc = fg_circ_buf_avg(&chip->vbatt_circ_buf, &vbatt_avg);
	if (rc < 0) {
		/* try to get instantaneous voltage */
		rc = fg_get_battery_voltage(chip, &vbatt_avg);
		if (rc < 0) {
			mutex_unlock(&chip->batt_avg_lock);
			pr_err("failed to get battery voltage, rc=%d\n", rc);
			return rc;
		}
	}

	mutex_unlock(&chip->batt_avg_lock);
	fg_dbg(chip, FG_TTF, "vbatt_avg=%d\n", vbatt_avg);

	/* clamp ibatt_avg to -150mA */
	if (ibatt_avg > -150000)
		ibatt_avg = -150000;
	fg_dbg(chip, FG_TTF, "ibatt_avg=%d\n", ibatt_avg);

	/* reverse polarity to be consistent with unsigned current settings */
	ibatt_avg = abs(ibatt_avg);

	/* estimated battery current at the CC to CV transition */
	i_cc2cv = div_s64((s64)ibatt_avg * vbatt_avg, chip->bp.float_volt_uv);
	fg_dbg(chip, FG_TTF, "i_cc2cv=%d\n", i_cc2cv);

	rc = fg_get_battery_resistance(chip, &rbatt);
	if (rc < 0) {
		pr_err("failed to get battery resistance rc=%d\n", rc);
		return rc;
	}

	/* clamp rbatt to 50mOhms */
	if (rbatt < 50000)
		rbatt = 50000;

	fg_dbg(chip, FG_TTF, "rbatt=%d\n", rbatt);

	rc = fg_get_sram_prop(chip, FG_SRAM_ACT_BATT_CAP, &act_cap_uah);
	if (rc < 0) {
		pr_err("failed to get ACT_BATT_CAP rc=%d\n", rc);
		return rc;
	}
	act_cap_uah *= MILLI_UNIT;
	fg_dbg(chip, FG_TTF, "actual_capacity_uah=%d\n", act_cap_uah);

	rc = fg_get_sram_prop(chip, FG_SRAM_FULL_SOC, &full_soc);
	if (rc < 0) {
		pr_err("failed to get full soc rc=%d\n", rc);
		return rc;
	}
	full_soc = DIV_ROUND_CLOSEST(((u16)full_soc >> 8) * FULL_CAPACITY,
								FULL_SOC_RAW);
	fg_dbg(chip, FG_TTF, "full_soc=%d\n", full_soc);

	/* if we are already in CV state then we can skip estimating CC */
	if (chip->charge_type == POWER_SUPPLY_CHARGE_TYPE_TAPER)
		goto skip_cc_estimate;

	/* if the charger is current limited then use power approximation */
	if (ibatt_avg > chip->bp.fastchg_curr_ma * MILLI_UNIT - 50000)
		ocv_cc2cv = div_s64((s64)rbatt * ibatt_avg, MICRO_UNIT);
	else
		ocv_cc2cv = div_s64((s64)rbatt * i_cc2cv, MICRO_UNIT);
	ocv_cc2cv = chip->bp.float_volt_uv - ocv_cc2cv;
	fg_dbg(chip, FG_TTF, "ocv_cc2cv=%d\n", ocv_cc2cv);

	soc_cc2cv = div_s64(chip->bp.float_volt_uv - ocv_cc2cv, OCV_SLOPE_UV);
	/* estimated SOC at the CC to CV transition */
	soc_cc2cv = 100 - soc_cc2cv;
	fg_dbg(chip, FG_TTF, "soc_cc2cv=%d\n", soc_cc2cv);

	/* the esimated SOC may be lower than the current SOC */
	if (soc_cc2cv - msoc <= 0)
		goto skip_cc_estimate;

	t_predicted_cc = div_s64((s64)full_soc * act_cap_uah, 100);
	t_predicted_cc = div_s64(t_predicted_cc * (soc_cc2cv - msoc), 100);
	t_predicted_cc *= HOURS_TO_SECONDS;
	t_predicted_cc = div_s64(t_predicted_cc, (ibatt_avg + i_cc2cv) / 2);

skip_cc_estimate:
	fg_dbg(chip, FG_TTF, "t_predicted_cc=%lld\n", t_predicted_cc);

	/* CV estimate starts here */
	if (chip->charge_type >= POWER_SUPPLY_CHARGE_TYPE_TAPER)
		ln_val = ibatt_avg / (abs(chip->dt.sys_term_curr_ma) + 200);
	else
		ln_val = i_cc2cv / (abs(chip->dt.sys_term_curr_ma) + 200);

	if (msoc < 95)
		centi_tau_scale = 100;
	else
		centi_tau_scale = 20 * (100 - msoc);

	fg_dbg(chip, FG_TTF, "ln_in=%d\n", ln_val);
	rc = fg_lerp(fg_ln_table, ARRAY_SIZE(fg_ln_table), ln_val, &ln_val);
	fg_dbg(chip, FG_TTF, "ln_out=%d\n", ln_val);
	t_predicted_cv = div_s64((s64)act_cap_uah * rbatt, MICRO_UNIT);
	t_predicted_cv = div_s64(t_predicted_cv * centi_tau_scale, 100);
	t_predicted_cv = div_s64(t_predicted_cv * ln_val, MILLI_UNIT);
	t_predicted_cv = div_s64(t_predicted_cv * HOURS_TO_SECONDS, MICRO_UNIT);
	fg_dbg(chip, FG_TTF, "t_predicted_cv=%lld\n", t_predicted_cv);
	*val = t_predicted_cc + t_predicted_cv;
	return 0;
}

#define CENTI_ICORRECT_C0	105
#define CENTI_ICORRECT_C1	20
static int fg_get_time_to_empty(struct fg_chip *chip, int *val)
{
	int rc, ibatt_avg, msoc, act_cap_uah;
	s32 divisor;
	s64 t_predicted;

	rc = fg_circ_buf_avg(&chip->ibatt_circ_buf, &ibatt_avg);
	if (rc < 0) {
		/* try to get instantaneous current */
		rc = fg_get_battery_current(chip, &ibatt_avg);
		if (rc < 0) {
			pr_err("failed to get battery current, rc=%d\n", rc);
			return rc;
		}
	}

	/* clamp ibatt_avg to 150mA */
	if (ibatt_avg < 150000)
		ibatt_avg = 150000;

	rc = fg_get_sram_prop(chip, FG_SRAM_ACT_BATT_CAP, &act_cap_uah);
	if (rc < 0) {
		pr_err("Error in getting ACT_BATT_CAP, rc=%d\n", rc);
		return rc;
	}
	act_cap_uah *= MILLI_UNIT;

	rc = fg_get_prop_capacity(chip, &msoc);
	if (rc < 0) {
		pr_err("Error in getting capacity, rc=%d\n", rc);
		return rc;
	}

	t_predicted = div_s64((s64)msoc * act_cap_uah, 100);
	t_predicted *= HOURS_TO_SECONDS;
	divisor = CENTI_ICORRECT_C0 * 100 + CENTI_ICORRECT_C1 * msoc;
	divisor = div_s64((s64)divisor * ibatt_avg, 10000);
	if (divisor > 0)
		t_predicted = div_s64(t_predicted, divisor);

	*val = t_predicted;
	return 0;
}


#define	COMPENSATION_COUNT_THD	25

static int get_bat_charging_status(struct fg_chip *chip);


void fix_maint_soc_worker(struct work_struct *work){

	struct fg_chip *chip = g_fgChip;
	union power_supply_propval prop = {0, };
	int rc=0;
	
	if(usb_psy_initialized(chip)){
		rc = power_supply_get_property(chip->usb_psy, POWER_SUPPLY_PROP_ONLINE,
			&prop);
		if (rc < 0) 
			pr_err("Error in getting usb online, rc=%d\n", rc);	
	}
	else
		pr_err("Error in getting usb_psy\n");
		

	if(prop.intval == POWER_SUPPLY_PL_NONE){
		chip->maint_soc -= 1;
		chip->delta_soc -= 1;
		BAT_DBG("delay work fix maint_soc by 1%% for compensation\n");
		if (batt_psy_initialized(chip))
			power_supply_changed(chip->batt_psy);
	}
	else
		BAT_DBG("cable online, abort fix maint_soc\n");
	
}


static int fg_update_maint_soc(struct fg_chip *chip)
{
	int rc = 0, msoc, delta=0;
	static int msoc_counter=0,count=0;

	mutex_lock(&chip->charge_full_lock);
	if (chip->delta_soc <= 0)
		goto out;

	rc = fg_get_msoc_raw(chip, &msoc);
	if (rc < 0) {
		pr_err("Error in getting msoc, rc=%d\n", rc);
		goto out;
	}

	if (msoc >= chip->maint_soc) {
		/*
		 * When the monotonic SOC goes above maintenance SOC, we should
		 * stop showing the maintenance SOC.
		 */
		chip->delta_soc = 0;
		chip->maint_soc = 0;
		msoc_counter=0;
	} 
	else if (msoc <= chip->last_msoc) {
	
		delta= chip->last_msoc-msoc;
		/* MSOC is decreasing. Decrease maintenance SOC as well */
		chip->maint_soc -= delta;
		msoc_counter+= delta;
		if (msoc_counter >=COMPENSATION_COUNT_THD) {
			/*
			 * Reduce the maintenance SOC additionally by 1 whenever
			 * it crosses a SOC multiple of 10.
			 */

			if(DIV_ROUND_CLOSEST(chip->maint_soc * FULL_CAPACITY, FULL_SOC_RAW)
				> DIV_ROUND_CLOSEST((chip->maint_soc-1) * FULL_CAPACITY, FULL_SOC_RAW)){

				schedule_delayed_work(&fix_maint_soc_work, 60*HZ);
			}
			else{			
				chip->maint_soc -= 1;
				chip->delta_soc -= 1;
				BAT_DBG("counter exceed %d, fix maint_soc by 1%% for compensation\n"
					,COMPENSATION_COUNT_THD);			
			}
			msoc_counter=0;
		}
	}else{
		count += msoc - chip->last_msoc;
		if(count >= 2
			&&98 >= DIV_ROUND_CLOSEST(chip->maint_soc * FULL_CAPACITY, FULL_SOC_RAW)){
			count=0;
			chip->maint_soc += 1;
			chip->delta_soc = chip->maint_soc - msoc;
			BAT_DBG("counter exceed %d, fix maint_soc by 1%% for compensation\n",2);

		}
	}	

	BAT_DBG("msoc: %d last_msoc: %d maint_soc: %d delta_soc: %d cnt: %d\n",
		msoc, chip->last_msoc, chip->maint_soc, chip->delta_soc, msoc_counter);
	chip->last_msoc = msoc;
out:
	mutex_unlock(&chip->charge_full_lock);
	return rc;
}

static int fg_esr_validate(struct fg_chip *chip)
{
	int rc, esr_uohms;
	u8 buf[2];

	if (chip->dt.esr_clamp_mohms <= 0)
		return 0;

	rc = fg_get_sram_prop(chip, FG_SRAM_ESR, &esr_uohms);
	if (rc < 0) {
		pr_err("failed to get ESR, rc=%d\n", rc);
		return rc;
	}

	if (esr_uohms >= chip->dt.esr_clamp_mohms * 1000) {
		pr_debug("ESR %d is > ESR_clamp\n", esr_uohms);
		return 0;
	}

	esr_uohms = chip->dt.esr_clamp_mohms * 1000;
	fg_encode(chip->sp, FG_SRAM_ESR, esr_uohms, buf);
	rc = fg_sram_write(chip, chip->sp[FG_SRAM_ESR].addr_word,
			chip->sp[FG_SRAM_ESR].addr_byte, buf,
			chip->sp[FG_SRAM_ESR].len, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in writing ESR, rc=%d\n", rc);
		return rc;
	}

	fg_dbg(chip, FG_STATUS, "ESR clamped to %duOhms\n", esr_uohms);
	return 0;
}

/* PSY CALLBACKS STAY HERE */

#define CHARGE_FULL_CURRENT_MASK_MA		15000
bool is_bms_charging(void);

static int fg_psy_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *pval)
{
	struct fg_chip *chip = power_supply_get_drvdata(psy);
	int rc = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_CAPACITY:
		rc = fg_get_prop_capacity(chip, &pval->intval);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		if (chip->battery_missing)
			pval->intval = 3700000;
		else
			rc = fg_get_battery_voltage(chip, &pval->intval);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		rc = fg_get_battery_current(chip, &pval->intval);
		if(chip->charge_full && abs(pval->intval)<CHARGE_FULL_CURRENT_MASK_MA
			&& is_bms_charging())
			pval->intval =0;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		if(fake_temp == FAKE_TEMP_INIT)
			rc = fg_get_battery_temp(chip, &pval->intval);
		else
			pval->intval = fake_temp;
		break;
	case POWER_SUPPLY_PROP_RESISTANCE:
		rc = fg_get_battery_resistance(chip, &pval->intval);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_OCV:
		rc = fg_get_sram_prop(chip, FG_SRAM_OCV, &pval->intval);
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		pval->intval = chip->cl.nom_cap_uah;
		break;
	case POWER_SUPPLY_PROP_RESISTANCE_ID:
		pval->intval = chip->batt_id_ohms;
		break;
	case POWER_SUPPLY_PROP_BATTERY_TYPE:
		pval->strval = fg_get_battery_type(chip);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		pval->intval = chip->bp.float_volt_uv;
		break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		pval->intval = fg_get_cycle_count(chip);
		break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT_ID:
		pval->intval = chip->cyc_ctr.id;
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW_RAW:
		rc = fg_get_charge_raw(chip, &pval->intval);
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		pval->intval = chip->cl.init_cc_uah;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		pval->intval = chip->cl.learned_cc_uah;
		break;
	case POWER_SUPPLY_PROP_CHARGE_COUNTER:
		rc = fg_get_charge_counter(chip, &pval->intval);
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_AVG:
		rc = fg_get_time_to_full(chip, &pval->intval);
		break;
	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG:
		rc = fg_get_time_to_empty(chip, &pval->intval);
		break;
	case POWER_SUPPLY_PROP_SOC_REPORTING_READY:
		pval->intval = chip->soc_reporting_ready;
		break;
	case POWER_SUPPLY_PROP_DEBUG_BATTERY:
		pval->intval = is_debug_batt_id(chip);
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		rc = fg_get_sram_prop(chip, FG_SRAM_VBATT_FULL, &pval->intval);
		break;
	default:
		pr_err("unsupported property %d\n", psp);
		rc = -EINVAL;
		break;
	}

	if (rc < 0)
		return -ENODATA;

	return 0;
}

static int fg_psy_set_property(struct power_supply *psy,
				  enum power_supply_property psp,
				  const union power_supply_propval *pval)
{
	struct fg_chip *chip = power_supply_get_drvdata(psy);
	int rc = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_CYCLE_COUNT_ID:
		if ((pval->intval > 0) && (pval->intval <= BUCKET_COUNT)) {
			chip->cyc_ctr.id = pval->intval;
		} else {
			pr_err("rejecting invalid cycle_count_id = %d\n",
				pval->intval);
			return -EINVAL;
		}
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		rc = fg_set_constant_chg_voltage(chip, pval->intval);
		break;
	default:
		break;
	}

	return rc;
}

static int fg_property_is_writeable(struct power_supply *psy,
						enum power_supply_property psp)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_CYCLE_COUNT_ID:
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		return 1;
	default:
		break;
	}

	return 0;
}

static void fg_external_power_changed(struct power_supply *psy)
{
	pr_debug("power supply changed\n");
}

static int fg_notifier_cb(struct notifier_block *nb,
		unsigned long event, void *data)
{
	struct power_supply *psy = data;
	struct fg_chip *chip = container_of(nb, struct fg_chip, nb);

	if (event != PSY_EVENT_PROP_CHANGED)
		return NOTIFY_OK;

	if (work_pending(&chip->status_change_work))
		return NOTIFY_OK;

	if ((strcmp(psy->desc->name, "battery") == 0)
		|| (strcmp(psy->desc->name, "usb") == 0)) {
		/*
		 * We cannot vote for awake votable here as that takes
		 * a mutex lock and this is executed in an atomic context.
		 */
		pm_stay_awake(chip->dev);
		schedule_work(&chip->status_change_work);
	}

	return NOTIFY_OK;
}

static enum power_supply_property fg_psy_props[] = {
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_OCV,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_RESISTANCE_ID,
	POWER_SUPPLY_PROP_RESISTANCE,
	POWER_SUPPLY_PROP_BATTERY_TYPE,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_CYCLE_COUNT,
	POWER_SUPPLY_PROP_CYCLE_COUNT_ID,
	POWER_SUPPLY_PROP_CHARGE_NOW_RAW,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
	POWER_SUPPLY_PROP_TIME_TO_FULL_AVG,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG,
	POWER_SUPPLY_PROP_SOC_REPORTING_READY,
	POWER_SUPPLY_PROP_DEBUG_BATTERY,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE,
};

static const struct power_supply_desc fg_psy_desc = {
	.name = "bms",
	.type = POWER_SUPPLY_TYPE_BMS,
	.properties = fg_psy_props,
	.num_properties = ARRAY_SIZE(fg_psy_props),
	.get_property = fg_psy_get_property,
	.set_property = fg_psy_set_property,
	.external_power_changed = fg_external_power_changed,
	.property_is_writeable = fg_property_is_writeable,
};

/* INIT FUNCTIONS STAY HERE */

static int fg_hw_init(struct fg_chip *chip)
{
	int rc;
	u8 buf[4], val;

	fg_encode(chip->sp, FG_SRAM_CUTOFF_VOLT, chip->dt.cutoff_volt_mv, buf);
	rc = fg_sram_write(chip, chip->sp[FG_SRAM_CUTOFF_VOLT].addr_word,
			chip->sp[FG_SRAM_CUTOFF_VOLT].addr_byte, buf,
			chip->sp[FG_SRAM_CUTOFF_VOLT].len, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in writing cutoff_volt, rc=%d\n", rc);
		return rc;
	}

	fg_encode(chip->sp, FG_SRAM_EMPTY_VOLT, chip->dt.empty_volt_mv, buf);
	rc = fg_sram_write(chip, chip->sp[FG_SRAM_EMPTY_VOLT].addr_word,
			chip->sp[FG_SRAM_EMPTY_VOLT].addr_byte, buf,
			chip->sp[FG_SRAM_EMPTY_VOLT].len, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in writing empty_volt, rc=%d\n", rc);
		return rc;
	}

	/* This SRAM register is only present in v2.0 and above */
	if (!(chip->wa_flags & PMI8998_V1_REV_WA) &&
					chip->bp.float_volt_uv > 0) {
		fg_encode(chip->sp, FG_SRAM_FLOAT_VOLT,
			chip->bp.float_volt_uv / 1000, buf);
		rc = fg_sram_write(chip, chip->sp[FG_SRAM_FLOAT_VOLT].addr_word,
			chip->sp[FG_SRAM_FLOAT_VOLT].addr_byte, buf,
			chip->sp[FG_SRAM_FLOAT_VOLT].len, FG_IMA_DEFAULT);
		if (rc < 0) {
			pr_err("Error in writing float_volt, rc=%d\n", rc);
			return rc;
		}
	}

	if (chip->bp.vbatt_full_mv > 0) {
		rc = fg_set_constant_chg_voltage(chip,
				chip->bp.vbatt_full_mv * 1000);
		if (rc < 0)
			return rc;
	}

	fg_encode(chip->sp, FG_SRAM_CHG_TERM_CURR, chip->dt.chg_term_curr_ma,
		buf);
	rc = fg_sram_write(chip, chip->sp[FG_SRAM_CHG_TERM_CURR].addr_word,
			chip->sp[FG_SRAM_CHG_TERM_CURR].addr_byte, buf,
			chip->sp[FG_SRAM_CHG_TERM_CURR].len, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in writing chg_term_curr, rc=%d\n", rc);
		return rc;
	}

	fg_encode(chip->sp, FG_SRAM_SYS_TERM_CURR, chip->dt.sys_term_curr_ma,
		buf);
	rc = fg_sram_write(chip, chip->sp[FG_SRAM_SYS_TERM_CURR].addr_word,
			chip->sp[FG_SRAM_SYS_TERM_CURR].addr_byte, buf,
			chip->sp[FG_SRAM_SYS_TERM_CURR].len, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in writing sys_term_curr, rc=%d\n", rc);
		return rc;
	}

	if (!(chip->wa_flags & PMI8998_V1_REV_WA)) {
		fg_encode(chip->sp, FG_SRAM_CHG_TERM_BASE_CURR,
			chip->dt.chg_term_base_curr_ma, buf);
		rc = fg_sram_write(chip,
				chip->sp[FG_SRAM_CHG_TERM_BASE_CURR].addr_word,
				chip->sp[FG_SRAM_CHG_TERM_BASE_CURR].addr_byte,
				buf, chip->sp[FG_SRAM_CHG_TERM_BASE_CURR].len,
				FG_IMA_DEFAULT);
		if (rc < 0) {
			pr_err("Error in writing chg_term_base_curr, rc=%d\n",
				rc);
			return rc;
		}
	}

	if (chip->dt.vbatt_low_thr_mv > 0) {
		fg_encode(chip->sp, FG_SRAM_VBATT_LOW,
			chip->dt.vbatt_low_thr_mv, buf);
		rc = fg_sram_write(chip, chip->sp[FG_SRAM_VBATT_LOW].addr_word,
				chip->sp[FG_SRAM_VBATT_LOW].addr_byte, buf,
				chip->sp[FG_SRAM_VBATT_LOW].len,
				FG_IMA_DEFAULT);
		if (rc < 0) {
			pr_err("Error in writing vbatt_low_thr, rc=%d\n", rc);
			return rc;
		}
	}

	if (chip->dt.delta_soc_thr > 0 && chip->dt.delta_soc_thr < 100) {
		fg_encode(chip->sp, FG_SRAM_DELTA_MSOC_THR,
			chip->dt.delta_soc_thr, buf);
		rc = fg_sram_write(chip,
				chip->sp[FG_SRAM_DELTA_MSOC_THR].addr_word,
				chip->sp[FG_SRAM_DELTA_MSOC_THR].addr_byte,
				buf, chip->sp[FG_SRAM_DELTA_MSOC_THR].len,
				FG_IMA_DEFAULT);
		if (rc < 0) {
			pr_err("Error in writing delta_msoc_thr, rc=%d\n", rc);
			return rc;
		}

		fg_encode(chip->sp, FG_SRAM_DELTA_BSOC_THR,
			chip->dt.delta_soc_thr, buf);
		rc = fg_sram_write(chip,
				chip->sp[FG_SRAM_DELTA_BSOC_THR].addr_word,
				chip->sp[FG_SRAM_DELTA_BSOC_THR].addr_byte,
				buf, chip->sp[FG_SRAM_DELTA_BSOC_THR].len,
				FG_IMA_DEFAULT);
		if (rc < 0) {
			pr_err("Error in writing delta_bsoc_thr, rc=%d\n", rc);
			return rc;
		}
	}

	/*
	 * configure battery thermal coefficients c1,c2,c3
	 * if its value is not zero.
	 */
	if (chip->dt.batt_therm_coeffs[0] > 0) {
		rc = fg_write(chip, BATT_INFO_THERM_C1(chip),
			chip->dt.batt_therm_coeffs, BATT_THERM_NUM_COEFFS);
		if (rc < 0) {
			pr_err("Error in writing battery thermal coefficients, rc=%d\n",
				rc);
			return rc;
		}
	}


	if (chip->dt.recharge_soc_thr > 0 && chip->dt.recharge_soc_thr < 100) {
		rc = fg_set_recharge_soc(chip, chip->dt.recharge_soc_thr);
		if (rc < 0) {
			pr_err("Error in setting recharge_soc, rc=%d\n", rc);
			return rc;
		}
	}

	if (chip->dt.recharge_volt_thr_mv > 0) {
		rc = fg_set_recharge_voltage(chip,
			chip->dt.recharge_volt_thr_mv);
		if (rc < 0) {
			pr_err("Error in setting recharge_voltage, rc=%d\n",
				rc);
			return rc;
		}
	}

	if (chip->dt.rsense_sel >= SRC_SEL_BATFET &&
			chip->dt.rsense_sel < SRC_SEL_RESERVED) {
		rc = fg_masked_write(chip, BATT_INFO_IBATT_SENSING_CFG(chip),
				SOURCE_SELECT_MASK, chip->dt.rsense_sel);
		if (rc < 0) {
			pr_err("Error in writing rsense_sel, rc=%d\n", rc);
			return rc;
		}
	}

	get_temp_setpoint(chip->dt.jeita_thresholds[JEITA_COLD], &val);
	rc = fg_write(chip, BATT_INFO_JEITA_TOO_COLD(chip), &val, 1);
	if (rc < 0) {
		pr_err("Error in writing jeita_cold, rc=%d\n", rc);
		return rc;
	}

	get_temp_setpoint(chip->dt.jeita_thresholds[JEITA_COOL], &val);
	rc = fg_write(chip, BATT_INFO_JEITA_COLD(chip), &val, 1);
	if (rc < 0) {
		pr_err("Error in writing jeita_cool, rc=%d\n", rc);
		return rc;
	}

	get_temp_setpoint(chip->dt.jeita_thresholds[JEITA_WARM], &val);
	rc = fg_write(chip, BATT_INFO_JEITA_HOT(chip), &val, 1);
	if (rc < 0) {
		pr_err("Error in writing jeita_warm, rc=%d\n", rc);
		return rc;
	}

	get_temp_setpoint(chip->dt.jeita_thresholds[JEITA_HOT], &val);
	rc = fg_write(chip, BATT_INFO_JEITA_TOO_HOT(chip), &val, 1);
	if (rc < 0) {
		pr_err("Error in writing jeita_hot, rc=%d\n", rc);
		return rc;
	}

	if (chip->dt.esr_timer_charging > 0) {
		rc = fg_set_esr_timer(chip, chip->dt.esr_timer_charging, true,
				      FG_IMA_DEFAULT);
		if (rc < 0) {
			pr_err("Error in setting ESR timer, rc=%d\n", rc);
			return rc;
		}
	}

	if (chip->dt.esr_timer_awake > 0) {
		rc = fg_set_esr_timer(chip, chip->dt.esr_timer_awake, false,
				      FG_IMA_DEFAULT);
		if (rc < 0) {
			pr_err("Error in setting ESR timer, rc=%d\n", rc);
			return rc;
		}
	}

	if (chip->cyc_ctr.en)
		restore_cycle_counter(chip);

	if (chip->dt.jeita_hyst_temp >= 0) {
		val = chip->dt.jeita_hyst_temp << JEITA_TEMP_HYST_SHIFT;
		rc = fg_masked_write(chip, BATT_INFO_BATT_TEMP_CFG(chip),
			JEITA_TEMP_HYST_MASK, val);
		if (rc < 0) {
			pr_err("Error in writing batt_temp_cfg, rc=%d\n", rc);
			return rc;
		}
	}

	get_batt_temp_delta(chip->dt.batt_temp_delta, &val);
	rc = fg_masked_write(chip, BATT_INFO_BATT_TMPR_INTR(chip),
			CHANGE_THOLD_MASK, val);
	if (rc < 0) {
		pr_err("Error in writing batt_temp_delta, rc=%d\n", rc);
		return rc;
	}

	rc = fg_rconn_config(chip);
	if (rc < 0) {
		pr_err("Error in configuring Rconn, rc=%d\n", rc);
		return rc;
	}

	fg_encode(chip->sp, FG_SRAM_ESR_TIGHT_FILTER,
		chip->dt.esr_tight_flt_upct, buf);
	rc = fg_sram_write(chip, chip->sp[FG_SRAM_ESR_TIGHT_FILTER].addr_word,
			chip->sp[FG_SRAM_ESR_TIGHT_FILTER].addr_byte, buf,
			chip->sp[FG_SRAM_ESR_TIGHT_FILTER].len, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in writing ESR tight filter, rc=%d\n", rc);
		return rc;
	}

	fg_encode(chip->sp, FG_SRAM_ESR_BROAD_FILTER,
		chip->dt.esr_broad_flt_upct, buf);
	rc = fg_sram_write(chip, chip->sp[FG_SRAM_ESR_BROAD_FILTER].addr_word,
			chip->sp[FG_SRAM_ESR_BROAD_FILTER].addr_byte, buf,
			chip->sp[FG_SRAM_ESR_BROAD_FILTER].len, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in writing ESR broad filter, rc=%d\n", rc);
		return rc;
	}

	fg_encode(chip->sp, FG_SRAM_ESR_PULSE_THRESH,
		chip->dt.esr_pulse_thresh_ma, buf);
	rc = fg_sram_write(chip, chip->sp[FG_SRAM_ESR_PULSE_THRESH].addr_word,
			chip->sp[FG_SRAM_ESR_PULSE_THRESH].addr_byte, buf,
			chip->sp[FG_SRAM_ESR_PULSE_THRESH].len, FG_IMA_DEFAULT);
	if (rc < 0) {
		pr_err("Error in writing esr_pulse_thresh_ma, rc=%d\n", rc);
		return rc;
	}

	get_esr_meas_current(chip->dt.esr_meas_curr_ma, &val);
	rc = fg_masked_write(chip, BATT_INFO_ESR_PULL_DN_CFG(chip),
			ESR_PULL_DOWN_IVAL_MASK, val);
	if (rc < 0) {
		pr_err("Error in writing esr_meas_curr_ma, rc=%d\n", rc);
		return rc;
	}

	return 0;
}

static int fg_memif_init(struct fg_chip *chip)
{
	return fg_ima_init(chip);
}

static int fg_adjust_timebase(struct fg_chip *chip)
{
	int rc = 0, die_temp;
	s32 time_base = 0;
	u8 buf[2] = {0};

	if ((chip->wa_flags & PM660_TSMC_OSC_WA) && chip->die_temp_chan) {
		rc = iio_read_channel_processed(chip->die_temp_chan, &die_temp);
		if (rc < 0) {
			pr_err("Error in reading die_temp, rc:%d\n", rc);
			return rc;
		}

		rc = fg_lerp(fg_tsmc_osc_table, ARRAY_SIZE(fg_tsmc_osc_table),
					die_temp / 1000, &time_base);
		if (rc < 0) {
			pr_err("Error to lookup fg_tsmc_osc_table rc=%d\n", rc);
			return rc;
		}

		fg_encode(chip->sp, FG_SRAM_TIMEBASE, time_base, buf);
		rc = fg_sram_write(chip,
			chip->sp[FG_SRAM_TIMEBASE].addr_word,
			chip->sp[FG_SRAM_TIMEBASE].addr_byte, buf,
			chip->sp[FG_SRAM_TIMEBASE].len, FG_IMA_DEFAULT);
		if (rc < 0) {
			pr_err("Error in writing timebase, rc=%d\n", rc);
			return rc;
		}
	}

	return 0;
}

/* INTERRUPT HANDLERS STAY HERE */

static irqreturn_t fg_mem_xcp_irq_handler(int irq, void *data)
{
	struct fg_chip *chip = data;
	u8 status;
	int rc;

	rc = fg_read(chip, MEM_IF_INT_RT_STS(chip), &status, 1);
	if (rc < 0) {
		pr_err("failed to read addr=0x%04x, rc=%d\n",
			MEM_IF_INT_RT_STS(chip), rc);
		return IRQ_HANDLED;
	}

	fg_dbg(chip, FG_IRQ, "irq %d triggered, status:%d\n", irq, status);

	mutex_lock(&chip->sram_rw_lock);
	rc = fg_clear_dma_errors_if_any(chip);
	if (rc < 0)
		pr_err("Error in clearing DMA error, rc=%d\n", rc);

	if (status & MEM_XCP_BIT) {
		rc = fg_clear_ima_errors_if_any(chip, true);
		if (rc < 0 && rc != -EAGAIN)
			pr_err("Error in checking IMA errors rc:%d\n", rc);
	}

	mutex_unlock(&chip->sram_rw_lock);
	return IRQ_HANDLED;
}

static irqreturn_t fg_vbatt_low_irq_handler(int irq, void *data)
{
	struct fg_chip *chip = data;

	fg_dbg(chip, FG_IRQ, "irq %d triggered\n", irq);
	return IRQ_HANDLED;
}

static irqreturn_t fg_batt_missing_irq_handler(int irq, void *data)
{
	struct fg_chip *chip = data;
	u8 status;
	int rc;

	rc = fg_read(chip, BATT_INFO_INT_RT_STS(chip), &status, 1);
	if (rc < 0) {
		pr_err("failed to read addr=0x%04x, rc=%d\n",
			BATT_INFO_INT_RT_STS(chip), rc);
		return IRQ_HANDLED;
	}

	fg_dbg(chip, FG_IRQ, "irq %d triggered sts:%d\n", irq, status);
	chip->battery_missing = (status & BT_MISS_BIT);

	if (chip->battery_missing) {
		chip->profile_available = false;
		chip->profile_loaded = false;
		chip->soc_reporting_ready = false;
		return IRQ_HANDLED;
	}

	rc = fg_get_batt_id(chip);
	if (rc < 0) {
		chip->soc_reporting_ready = true;
		pr_err("Error in getting battery id, rc:%d\n", rc);
		return IRQ_HANDLED;
	}

	rc = fg_get_batt_profile(chip);
	if (rc < 0) {
		chip->soc_reporting_ready = true;
		pr_err("Error in getting battery profile, rc:%d\n", rc);
		return IRQ_HANDLED;
	}

	clear_battery_profile(chip);
	schedule_delayed_work(&chip->profile_load_work, 0);

	if (chip->fg_psy)
		power_supply_changed(chip->fg_psy);

	return IRQ_HANDLED;
}

static irqreturn_t fg_delta_batt_temp_irq_handler(int irq, void *data)
{
	struct fg_chip *chip = data;
	union power_supply_propval prop = {0, };
	int rc, batt_temp;

	fg_dbg(chip, FG_IRQ, "irq %d triggered\n", irq);
	rc = fg_get_battery_temp(chip, &batt_temp);
	if (rc < 0) {
		pr_err("Error in getting batt_temp\n");
		return IRQ_HANDLED;
	}

	rc = fg_esr_filter_config(chip, batt_temp);
	if (rc < 0)
		pr_err("Error in configuring ESR filter rc:%d\n", rc);

	rc = fg_slope_limit_config(chip, batt_temp);
	if (rc < 0)
		pr_err("Error in configuring slope limiter rc:%d\n", rc);

	rc = fg_adjust_ki_coeff_full_soc(chip, batt_temp);
	if (rc < 0)
		pr_err("Error in configuring ki_coeff_full_soc rc:%d\n", rc);

	if (!batt_psy_initialized(chip)) {
		chip->last_batt_temp = batt_temp;
		return IRQ_HANDLED;
	}

	power_supply_get_property(chip->batt_psy, POWER_SUPPLY_PROP_HEALTH,
		&prop);
	chip->health = prop.intval;

	if (chip->last_batt_temp != batt_temp) {
		rc = fg_adjust_timebase(chip);
		if (rc < 0)
			pr_err("Error in adjusting timebase, rc=%d\n", rc);

		chip->last_batt_temp = batt_temp;
		power_supply_changed(chip->batt_psy);
	}

	if (abs(chip->last_batt_temp - batt_temp) > 30)
		pr_warn("Battery temperature last:%d current: %d\n",
			chip->last_batt_temp, batt_temp);
	return IRQ_HANDLED;
}

static irqreturn_t fg_first_est_irq_handler(int irq, void *data)
{
	struct fg_chip *chip = data;

	fg_dbg(chip, FG_IRQ, "irq %d triggered\n", irq);
	complete_all(&chip->soc_ready);
	return IRQ_HANDLED;
}

static irqreturn_t fg_soc_update_irq_handler(int irq, void *data)
{
	struct fg_chip *chip = data;

	fg_dbg(chip, FG_IRQ, "irq %d triggered\n", irq);
	complete_all(&chip->soc_update);
	return IRQ_HANDLED;
}

static irqreturn_t fg_delta_bsoc_irq_handler(int irq, void *data)
{
	struct fg_chip *chip = data;
	int rc;

	BAT_DBG("irq %d triggered\n", irq);
	rc = fg_charge_full_update(chip);
	if (rc < 0)
		pr_err("Error in charge_full_update, rc=%d\n", rc);

	return IRQ_HANDLED;
}

static irqreturn_t fg_delta_msoc_irq_handler(int irq, void *data)
{
	struct fg_chip *chip = data;
	int rc;

	BAT_DBG("irq %d triggered\n", irq);
	if (chip->cyc_ctr.en)
		schedule_work(&chip->cycle_count_work);

	if (chip->cl.active)
		fg_cap_learning_update(chip);

	rc = fg_charge_full_update(chip);
	if (rc < 0)
		pr_err("Error in charge_full_update, rc=%d\n", rc);

	rc = fg_adjust_ki_coeff_dischg(chip);
	if (rc < 0)
		pr_err("Error in adjusting ki_coeff_dischg, rc=%d\n", rc);

	rc = fg_update_maint_soc(chip);
	if (rc < 0)
		pr_err("Error in updating maint_soc, rc=%d\n", rc);

	rc = fg_esr_validate(chip);
	if (rc < 0)
		pr_err("Error in validating ESR, rc=%d\n", rc);

	rc = fg_adjust_timebase(chip);
	if (rc < 0)
		pr_err("Error in adjusting timebase, rc=%d\n", rc);

	if (batt_psy_initialized(chip))
		power_supply_changed(chip->batt_psy);

	return IRQ_HANDLED;
}

static irqreturn_t fg_empty_soc_irq_handler(int irq, void *data)
{
	struct fg_chip *chip = data;

	fg_dbg(chip, FG_IRQ, "irq %d triggered\n", irq);
	if (batt_psy_initialized(chip))
		power_supply_changed(chip->batt_psy);

	return IRQ_HANDLED;
}

static irqreturn_t fg_soc_irq_handler(int irq, void *data)
{
	struct fg_chip *chip = data;

	fg_dbg(chip, FG_IRQ, "irq %d triggered\n", irq);
	return IRQ_HANDLED;
}

static irqreturn_t fg_dummy_irq_handler(int irq, void *data)
{
	pr_debug("irq %d triggered\n", irq);
	return IRQ_HANDLED;
}

static struct fg_irq_info fg_irqs[FG_IRQ_MAX] = {
	/* BATT_SOC irqs */
	[MSOC_FULL_IRQ] = {
		.name		= "msoc-full",
		.handler	= fg_soc_irq_handler,
	},
	[MSOC_HIGH_IRQ] = {
		.name		= "msoc-high",
		.handler	= fg_soc_irq_handler,
		.wakeable	= true,
	},
	[MSOC_EMPTY_IRQ] = {
		.name		= "msoc-empty",
		.handler	= fg_empty_soc_irq_handler,
		.wakeable	= true,
	},
	[MSOC_LOW_IRQ] = {
		.name		= "msoc-low",
		.handler	= fg_soc_irq_handler,
		.wakeable	= true,
	},
	[MSOC_DELTA_IRQ] = {
		.name		= "msoc-delta",
		.handler	= fg_delta_msoc_irq_handler,
		.wakeable	= true,
	},
	[BSOC_DELTA_IRQ] = {
		.name		= "bsoc-delta",
		.handler	= fg_delta_bsoc_irq_handler,
		.wakeable	= true,
	},
	[SOC_READY_IRQ] = {
		.name		= "soc-ready",
		.handler	= fg_first_est_irq_handler,
		.wakeable	= true,
	},
	[SOC_UPDATE_IRQ] = {
		.name		= "soc-update",
		.handler	= fg_soc_update_irq_handler,
	},
	/* BATT_INFO irqs */
	[BATT_TEMP_DELTA_IRQ] = {
		.name		= "batt-temp-delta",
		.handler	= fg_delta_batt_temp_irq_handler,
		.wakeable	= true,
	},
	[BATT_MISSING_IRQ] = {
		.name		= "batt-missing",
		.handler	= fg_batt_missing_irq_handler,
		.wakeable	= true,
	},
	[ESR_DELTA_IRQ] = {
		.name		= "esr-delta",
		.handler	= fg_dummy_irq_handler,
	},
	[VBATT_LOW_IRQ] = {
		.name		= "vbatt-low",
		.handler	= fg_vbatt_low_irq_handler,
		.wakeable	= true,
	},
	[VBATT_PRED_DELTA_IRQ] = {
		.name		= "vbatt-pred-delta",
		.handler	= fg_dummy_irq_handler,
	},
	/* MEM_IF irqs */
	[DMA_GRANT_IRQ] = {
		.name		= "dma-grant",
		.handler	= fg_dummy_irq_handler,
	},
	[MEM_XCP_IRQ] = {
		.name		= "mem-xcp",
		.handler	= fg_mem_xcp_irq_handler,
	},
	[IMA_RDY_IRQ] = {
		.name		= "ima-rdy",
		.handler	= fg_dummy_irq_handler,
	},
};

static int fg_get_irq_index_byname(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(fg_irqs); i++) {
		if (strcmp(fg_irqs[i].name, name) == 0)
			return i;
	}

	pr_err("%s is not in irq list\n", name);
	return -ENOENT;
}

static int fg_register_interrupts(struct fg_chip *chip)
{
	struct device_node *child, *node = chip->dev->of_node;
	struct property *prop;
	const char *name;
	int rc, irq, irq_index;

	for_each_available_child_of_node(node, child) {
		of_property_for_each_string(child, "interrupt-names", prop,
						name) {
			irq = of_irq_get_byname(child, name);
			if (irq < 0) {
				dev_err(chip->dev, "failed to get irq %s irq:%d\n",
					name, irq);
				return irq;
			}

			irq_index = fg_get_irq_index_byname(name);
			if (irq_index < 0)
				return irq_index;

			rc = devm_request_threaded_irq(chip->dev, irq, NULL,
					fg_irqs[irq_index].handler,
					IRQF_ONESHOT, name, chip);
			if (rc < 0) {
				dev_err(chip->dev, "failed to register irq handler for %s rc:%d\n",
					name, rc);
				return rc;
			}

			fg_irqs[irq_index].irq = irq;
			if (fg_irqs[irq_index].wakeable)
				enable_irq_wake(fg_irqs[irq_index].irq);
		}
	}

	return 0;
}

static int fg_parse_slope_limit_coefficients(struct fg_chip *chip)
{
	struct device_node *node = chip->dev->of_node;
	int rc, i;

	rc = of_property_read_u32(node, "qcom,slope-limit-temp-threshold",
			&chip->dt.slope_limit_temp);
	if (rc < 0)
		return 0;

	rc = of_property_count_elems_of_size(node, "qcom,slope-limit-coeffs",
			sizeof(u32));
	if (rc != SLOPE_LIMIT_NUM_COEFFS)
		return -EINVAL;

	rc = of_property_read_u32_array(node, "qcom,slope-limit-coeffs",
			chip->dt.slope_limit_coeffs, SLOPE_LIMIT_NUM_COEFFS);
	if (rc < 0) {
		pr_err("Error in reading qcom,slope-limit-coeffs, rc=%d\n", rc);
		return rc;
	}

	for (i = 0; i < SLOPE_LIMIT_NUM_COEFFS; i++) {
		if (chip->dt.slope_limit_coeffs[i] > SLOPE_LIMIT_COEFF_MAX ||
			chip->dt.slope_limit_coeffs[i] < 0) {
			pr_err("Incorrect slope limit coefficient\n");
			return -EINVAL;
		}
	}

	chip->slope_limit_en = true;
	return 0;
}

static int fg_parse_ki_coefficients(struct fg_chip *chip)
{
	struct device_node *node = chip->dev->of_node;
	int rc, i;

	rc = of_property_count_elems_of_size(node, "qcom,ki-coeff-soc-dischg",
		sizeof(u32));
	if (rc != KI_COEFF_SOC_LEVELS)
		return 0;

	rc = of_property_read_u32_array(node, "qcom,ki-coeff-soc-dischg",
			chip->dt.ki_coeff_soc, KI_COEFF_SOC_LEVELS);
	if (rc < 0) {
		pr_err("Error in reading ki-coeff-soc-dischg, rc=%d\n",
			rc);
		return rc;
	}

	rc = of_property_count_elems_of_size(node, "qcom,ki-coeff-med-dischg",
		sizeof(u32));
	if (rc != KI_COEFF_SOC_LEVELS)
		return 0;

	rc = of_property_read_u32_array(node, "qcom,ki-coeff-med-dischg",
			chip->dt.ki_coeff_med_dischg, KI_COEFF_SOC_LEVELS);
	if (rc < 0) {
		pr_err("Error in reading ki-coeff-med-dischg, rc=%d\n",
			rc);
		return rc;
	}

	rc = of_property_count_elems_of_size(node, "qcom,ki-coeff-hi-dischg",
		sizeof(u32));
	if (rc != KI_COEFF_SOC_LEVELS)
		return 0;

	rc = of_property_read_u32_array(node, "qcom,ki-coeff-hi-dischg",
			chip->dt.ki_coeff_hi_dischg, KI_COEFF_SOC_LEVELS);
	if (rc < 0) {
		pr_err("Error in reading ki-coeff-hi-dischg, rc=%d\n",
			rc);
		return rc;
	}

	for (i = 0; i < KI_COEFF_SOC_LEVELS; i++) {
		if (chip->dt.ki_coeff_soc[i] < 0 ||
			chip->dt.ki_coeff_soc[i] > FULL_CAPACITY) {
			pr_err("Error in ki_coeff_soc_dischg values\n");
			return -EINVAL;
		}

		if (chip->dt.ki_coeff_med_dischg[i] < 0 ||
			chip->dt.ki_coeff_med_dischg[i] > KI_COEFF_MAX) {
			pr_err("Error in ki_coeff_med_dischg values\n");
			return -EINVAL;
		}

		if (chip->dt.ki_coeff_med_dischg[i] < 0 ||
			chip->dt.ki_coeff_med_dischg[i] > KI_COEFF_MAX) {
			pr_err("Error in ki_coeff_med_dischg values\n");
			return -EINVAL;
		}
	}
	chip->ki_coeff_dischg_en = true;
	return 0;
}

#define DEFAULT_CUTOFF_VOLT_MV		3200
#define DEFAULT_EMPTY_VOLT_MV		2800
#define DEFAULT_RECHARGE_VOLT_MV	4250
#define DEFAULT_CHG_TERM_CURR_MA	100
#define DEFAULT_CHG_TERM_BASE_CURR_MA	75
#define DEFAULT_SYS_TERM_CURR_MA	-125
#define DEFAULT_DELTA_SOC_THR		1
#define DEFAULT_RECHARGE_SOC_THR	95
#define DEFAULT_BATT_TEMP_COLD		0	//0
#define DEFAULT_BATT_TEMP_COOL		10	//5
#define DEFAULT_BATT_TEMP_WARM		50	//45
#define DEFAULT_BATT_TEMP_HOT		60	//50
#define DEFAULT_CL_START_SOC		15
#define DEFAULT_CL_MIN_TEMP_DECIDEGC	150
#define DEFAULT_CL_MAX_TEMP_DECIDEGC	450
#define DEFAULT_CL_MAX_INC_DECIPERC	5
#define DEFAULT_CL_MAX_DEC_DECIPERC	100
#define DEFAULT_CL_MIN_LIM_DECIPERC	0
#define DEFAULT_CL_MAX_LIM_DECIPERC	0
#define BTEMP_DELTA_LOW			2
#define BTEMP_DELTA_HIGH		10
#define DEFAULT_ESR_FLT_TEMP_DECIDEGC	100
#define DEFAULT_ESR_TIGHT_FLT_UPCT	3907
#define DEFAULT_ESR_BROAD_FLT_UPCT	99610
#define DEFAULT_ESR_TIGHT_LT_FLT_UPCT	48829
#define DEFAULT_ESR_BROAD_LT_FLT_UPCT	148438
#define DEFAULT_ESR_CLAMP_MOHMS		20
#define DEFAULT_ESR_PULSE_THRESH_MA	110
#define DEFAULT_ESR_MEAS_CURR_MA	120
static int fg_parse_dt(struct fg_chip *chip)
{
	struct device_node *child, *revid_node, *node = chip->dev->of_node;
	u32 base, temp;
	u8 subtype;
	int rc;

	if (!node)  {
		dev_err(chip->dev, "device tree node missing\n");
		return -ENXIO;
	}

	revid_node = of_parse_phandle(node, "qcom,pmic-revid", 0);
	if (!revid_node) {
		pr_err("Missing qcom,pmic-revid property - driver failed\n");
		return -EINVAL;
	}

	chip->pmic_rev_id = get_revid_data(revid_node);
	if (IS_ERR_OR_NULL(chip->pmic_rev_id)) {
		pr_err("Unable to get pmic_revid rc=%ld\n",
			PTR_ERR(chip->pmic_rev_id));
		/*
		 * the revid peripheral must be registered, any failure
		 * here only indicates that the rev-id module has not
		 * probed yet.
		 */
		return -EPROBE_DEFER;
	}

	pr_debug("PMIC subtype %d Digital major %d\n",
		chip->pmic_rev_id->pmic_subtype, chip->pmic_rev_id->rev4);

	switch (chip->pmic_rev_id->pmic_subtype) {
	case PMI8998_SUBTYPE:
		if (chip->pmic_rev_id->rev4 < PMI8998_V2P0_REV4) {
			chip->sp = pmi8998_v1_sram_params;
			chip->alg_flags = pmi8998_v1_alg_flags;
			chip->wa_flags |= PMI8998_V1_REV_WA;
		} else if (chip->pmic_rev_id->rev4 == PMI8998_V2P0_REV4) {
			chip->sp = pmi8998_v2_sram_params;
			chip->alg_flags = pmi8998_v2_alg_flags;
		} else {
			return -EINVAL;
		}
		break;
	case PM660_SUBTYPE:
		chip->sp = pmi8998_v2_sram_params;
		chip->alg_flags = pmi8998_v2_alg_flags;
		chip->use_ima_single_mode = true;
		if (chip->pmic_rev_id->fab_id == PM660_FAB_ID_TSMC)
			chip->wa_flags |= PM660_TSMC_OSC_WA;
		break;
	default:
		return -EINVAL;
	}

	if (of_get_available_child_count(node) == 0) {
		dev_err(chip->dev, "No child nodes specified!\n");
		return -ENXIO;
	}

	for_each_available_child_of_node(node, child) {
		rc = of_property_read_u32(child, "reg", &base);
		if (rc < 0) {
			dev_err(chip->dev, "reg not specified in node %s, rc=%d\n",
				child->full_name, rc);
			return rc;
		}

		rc = fg_read(chip, base + PERPH_SUBTYPE_REG, &subtype, 1);
		if (rc < 0) {
			dev_err(chip->dev, "Couldn't read subtype for base %d, rc=%d\n",
				base, rc);
			return rc;
		}

		switch (subtype) {
		case FG_BATT_SOC_PMI8998:
			chip->batt_soc_base = base;
			break;
		case FG_BATT_INFO_PMI8998:
			chip->batt_info_base = base;
			break;
		case FG_MEM_INFO_PMI8998:
			chip->mem_if_base = base;
			break;
		default:
			dev_err(chip->dev, "Invalid peripheral subtype 0x%x\n",
				subtype);
			return -ENXIO;
		}
	}

	rc = of_property_read_u32(node, "qcom,rradc-base", &base);
	if (rc < 0) {
		dev_err(chip->dev, "rradc-base not specified, rc=%d\n", rc);
		return rc;
	}
	chip->rradc_base = base;

	/* Read all the optional properties below */
	rc = of_property_read_u32(node, "qcom,fg-cutoff-voltage", &temp);
	if (rc < 0)
		chip->dt.cutoff_volt_mv = DEFAULT_CUTOFF_VOLT_MV;
	else
		chip->dt.cutoff_volt_mv = temp;

	rc = of_property_read_u32(node, "qcom,fg-empty-voltage", &temp);
	if (rc < 0)
		chip->dt.empty_volt_mv = DEFAULT_EMPTY_VOLT_MV;
	else
		chip->dt.empty_volt_mv = temp;

	BAT_DBG("cutoff %d mv, emtpy %d mv\n",chip->dt.cutoff_volt_mv,chip->dt.empty_volt_mv);


	rc = of_property_read_u32(node, "qcom,fg-vbatt-low-thr", &temp);
	if (rc < 0)
		chip->dt.vbatt_low_thr_mv = -EINVAL;
	else
		chip->dt.vbatt_low_thr_mv = temp;

	rc = of_property_read_u32(node, "qcom,fg-chg-term-current", &temp);
	if (rc < 0)
		chip->dt.chg_term_curr_ma = DEFAULT_CHG_TERM_CURR_MA;
	else
		chip->dt.chg_term_curr_ma = temp;

	rc = of_property_read_u32(node, "qcom,fg-sys-term-current", &temp);
	if (rc < 0)
		chip->dt.sys_term_curr_ma = DEFAULT_SYS_TERM_CURR_MA;
	else
		chip->dt.sys_term_curr_ma = temp;

	rc = of_property_read_u32(node, "qcom,fg-chg-term-base-current", &temp);
	if (rc < 0)
		chip->dt.chg_term_base_curr_ma = DEFAULT_CHG_TERM_BASE_CURR_MA;
	else
		chip->dt.chg_term_base_curr_ma = temp;

	rc = of_property_read_u32(node, "qcom,fg-delta-soc-thr", &temp);
	if (rc < 0)
		chip->dt.delta_soc_thr = DEFAULT_DELTA_SOC_THR;
	else
		chip->dt.delta_soc_thr = temp;

	rc = of_property_read_u32(node, "qcom,fg-recharge-soc-thr", &temp);
	if (rc < 0)
		chip->dt.recharge_soc_thr = DEFAULT_RECHARGE_SOC_THR;
	else
		chip->dt.recharge_soc_thr = temp;

	rc = of_property_read_u32(node, "qcom,fg-recharge-voltage", &temp);
	if (rc < 0)
		chip->dt.recharge_volt_thr_mv = DEFAULT_RECHARGE_VOLT_MV;
	else
		chip->dt.recharge_volt_thr_mv = temp;


	chip->dt.auto_recharge_soc = of_property_read_bool(node,
					"qcom,fg-auto-recharge-soc");

	BAT_DBG("auto-recharge %d, recharge-soc %d%%, recharge-voltage %d mv\n",
		chip->dt.auto_recharge_soc,chip->dt.recharge_soc_thr,chip->dt.recharge_volt_thr_mv);
	
	rc = of_property_read_u32(node, "qcom,fg-rsense-sel", &temp);
	if (rc < 0)
		chip->dt.rsense_sel = SRC_SEL_BATFET_SMB;
	else
		chip->dt.rsense_sel = (u8)temp & SOURCE_SELECT_MASK;

	chip->dt.jeita_thresholds[JEITA_COLD] = DEFAULT_BATT_TEMP_COLD;
	chip->dt.jeita_thresholds[JEITA_COOL] = DEFAULT_BATT_TEMP_COOL;
	chip->dt.jeita_thresholds[JEITA_WARM] = DEFAULT_BATT_TEMP_WARM;
	chip->dt.jeita_thresholds[JEITA_HOT] = DEFAULT_BATT_TEMP_HOT;
	if (of_property_count_elems_of_size(node, "qcom,fg-jeita-thresholds",
		sizeof(u32)) == NUM_JEITA_LEVELS) {
		rc = of_property_read_u32_array(node,
				"qcom,fg-jeita-thresholds",
				chip->dt.jeita_thresholds, NUM_JEITA_LEVELS);
		if (rc < 0)
			pr_warn("Error reading Jeita thresholds, default values will be used rc:%d\n",
				rc);
	}

	if (of_property_count_elems_of_size(node,
		"qcom,battery-thermal-coefficients",
		sizeof(u8)) == BATT_THERM_NUM_COEFFS) {
		rc = of_property_read_u8_array(node,
				"qcom,battery-thermal-coefficients",
				chip->dt.batt_therm_coeffs,
				BATT_THERM_NUM_COEFFS);
		if (rc < 0)
			pr_warn("Error reading battery thermal coefficients, rc:%d\n",
				rc);
	}

	rc = of_property_read_u32(node, "qcom,fg-esr-timer-charging", &temp);
	if (rc < 0)
		chip->dt.esr_timer_charging = -EINVAL;
	else
		chip->dt.esr_timer_charging = temp;

	rc = of_property_read_u32(node, "qcom,fg-esr-timer-awake", &temp);
	if (rc < 0)
		chip->dt.esr_timer_awake = -EINVAL;
	else
		chip->dt.esr_timer_awake = temp;

	rc = of_property_read_u32(node, "qcom,fg-esr-timer-asleep", &temp);
	if (rc < 0)
		chip->dt.esr_timer_asleep = -EINVAL;
	else
		chip->dt.esr_timer_asleep = temp;

	chip->cyc_ctr.en = of_property_read_bool(node, "qcom,cycle-counter-en");
	if (chip->cyc_ctr.en)
		chip->cyc_ctr.id = 1;

	chip->dt.force_load_profile = of_property_read_bool(node,
					"qcom,fg-force-load-profile");

	rc = of_property_read_u32(node, "qcom,cl-start-capacity", &temp);
	if (rc < 0)
		chip->dt.cl_start_soc = DEFAULT_CL_START_SOC;
	else
		chip->dt.cl_start_soc = temp;

	rc = of_property_read_u32(node, "qcom,cl-min-temp", &temp);
	if (rc < 0)
		chip->dt.cl_min_temp = DEFAULT_CL_MIN_TEMP_DECIDEGC;
	else
		chip->dt.cl_min_temp = temp;

	rc = of_property_read_u32(node, "qcom,cl-max-temp", &temp);
	if (rc < 0)
		chip->dt.cl_max_temp = DEFAULT_CL_MAX_TEMP_DECIDEGC;
	else
		chip->dt.cl_max_temp = temp;

	rc = of_property_read_u32(node, "qcom,cl-max-increment", &temp);
	if (rc < 0)
		chip->dt.cl_max_cap_inc = DEFAULT_CL_MAX_INC_DECIPERC;
	else
		chip->dt.cl_max_cap_inc = temp;

	rc = of_property_read_u32(node, "qcom,cl-max-decrement", &temp);
	if (rc < 0)
		chip->dt.cl_max_cap_dec = DEFAULT_CL_MAX_DEC_DECIPERC;
	else
		chip->dt.cl_max_cap_dec = temp;

	rc = of_property_read_u32(node, "qcom,cl-min-limit", &temp);
	if (rc < 0)
		chip->dt.cl_min_cap_limit = DEFAULT_CL_MIN_LIM_DECIPERC;
	else
		chip->dt.cl_min_cap_limit = temp;

	rc = of_property_read_u32(node, "qcom,cl-max-limit", &temp);
	if (rc < 0)
		chip->dt.cl_max_cap_limit = DEFAULT_CL_MAX_LIM_DECIPERC;
	else
		chip->dt.cl_max_cap_limit = temp;

	rc = of_property_read_u32(node, "qcom,fg-jeita-hyst-temp", &temp);
	if (rc < 0)
		chip->dt.jeita_hyst_temp = -EINVAL;
	else
		chip->dt.jeita_hyst_temp = temp;

	rc = of_property_read_u32(node, "qcom,fg-batt-temp-delta", &temp);
	if (rc < 0)
		chip->dt.batt_temp_delta = -EINVAL;
	else if (temp > BTEMP_DELTA_LOW && temp <= BTEMP_DELTA_HIGH)
		chip->dt.batt_temp_delta = temp;

	chip->dt.hold_soc_while_full = of_property_read_bool(node,
					"qcom,hold-soc-while-full");

	rc = fg_parse_ki_coefficients(chip);
	if (rc < 0)
		pr_err("Error in parsing Ki coefficients, rc=%d\n", rc);

	rc = of_property_read_u32(node, "qcom,fg-rconn-mohms", &temp);
	if (!rc)
		chip->dt.rconn_mohms = temp;

	rc = of_property_read_u32(node, "qcom,fg-esr-filter-switch-temp",
			&temp);
	if (rc < 0)
		chip->dt.esr_flt_switch_temp = DEFAULT_ESR_FLT_TEMP_DECIDEGC;
	else
		chip->dt.esr_flt_switch_temp = temp;

	rc = of_property_read_u32(node, "qcom,fg-esr-tight-filter-micro-pct",
			&temp);
	if (rc < 0)
		chip->dt.esr_tight_flt_upct = DEFAULT_ESR_TIGHT_FLT_UPCT;
	else
		chip->dt.esr_tight_flt_upct = temp;

	rc = of_property_read_u32(node, "qcom,fg-esr-broad-filter-micro-pct",
			&temp);
	if (rc < 0)
		chip->dt.esr_broad_flt_upct = DEFAULT_ESR_BROAD_FLT_UPCT;
	else
		chip->dt.esr_broad_flt_upct = temp;

	rc = of_property_read_u32(node, "qcom,fg-esr-tight-lt-filter-micro-pct",
			&temp);
	if (rc < 0)
		chip->dt.esr_tight_lt_flt_upct = DEFAULT_ESR_TIGHT_LT_FLT_UPCT;
	else
		chip->dt.esr_tight_lt_flt_upct = temp;

	rc = of_property_read_u32(node, "qcom,fg-esr-broad-lt-filter-micro-pct",
			&temp);
	if (rc < 0)
		chip->dt.esr_broad_lt_flt_upct = DEFAULT_ESR_BROAD_LT_FLT_UPCT;
	else
		chip->dt.esr_broad_lt_flt_upct = temp;

	rc = fg_parse_slope_limit_coefficients(chip);
	if (rc < 0)
		pr_err("Error in parsing slope limit coeffs, rc=%d\n", rc);

	rc = of_property_read_u32(node, "qcom,fg-esr-clamp-mohms", &temp);
	if (rc < 0)
		chip->dt.esr_clamp_mohms = DEFAULT_ESR_CLAMP_MOHMS;
	else
		chip->dt.esr_clamp_mohms = temp;

	chip->dt.esr_pulse_thresh_ma = DEFAULT_ESR_PULSE_THRESH_MA;
	rc = of_property_read_u32(node, "qcom,fg-esr-pulse-thresh-ma", &temp);
	if (!rc) {
		/* ESR pulse qualification threshold range is 1-997 mA */
		if (temp > 0 && temp < 997)
			chip->dt.esr_pulse_thresh_ma = temp;
	}

	chip->dt.esr_meas_curr_ma = DEFAULT_ESR_MEAS_CURR_MA;
	rc = of_property_read_u32(node, "qcom,fg-esr-meas-curr-ma", &temp);
	if (!rc) {
		/* ESR measurement current range is 60-240 mA */
		if (temp >= 60 || temp <= 240)
			chip->dt.esr_meas_curr_ma = temp;
	}

	return 0;
}

#define ID_TOLERANCE	15
#define BATT_ID_CRITERIA	51000

bool ATD_Is_battID_within_range(int battID_criteria){

	int delta= abs(battID_criteria - g_fgChip->batt_id_ohms);
	int range= battID_criteria*(ID_TOLERANCE)/100;
	bool result = false;
	
	result = (delta<= range);

	BAT_DBG("batt id(%d) is%s within %d k range\n ",g_fgChip->batt_id_ohms,result?"":" not"
		,battID_criteria/1000);	

	return result;
	
}


/*+++ proc batt_current Interface+++*/
static int batt_mili_temp_proc_read(struct seq_file *buf, void *v)
{
	int result = 0;

	fg_get_battery_temp(g_fgChip, &result);
	//scale 0.1 to 0.001 
	result *=100;
	BAT_DBG("%s: %d\n", __func__, result);
	seq_printf(buf, "%d\n", result);
	return 0;
}
static int batt_mili_temp_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, batt_mili_temp_proc_read, NULL);
}

static void create_batt_mili_temp_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  batt_mili_temp_proc_open,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/batt_miliTemp", 0444, NULL, &proc_fops);
	if (!proc_file) {
		BAT_DBG_E("[Proc]%s failed!\n", __func__);
	}
	return;
}
static int batt_current_proc_read(struct seq_file *buf, void *v)
{
	int result = 0;

	fg_get_battery_current(g_fgChip, &result);
	BAT_DBG("%s: %d\n", __func__, result);
	seq_printf(buf, "%d\n", result);
	return 0;
}
static int batt_current_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, batt_current_proc_read, NULL);
}

static void create_batt_current_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  batt_current_proc_open,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/batt_current", 0444, NULL, &proc_fops);
	if (!proc_file) {
		BAT_DBG_E("[Proc]%s failed!\n", __func__);
	}
	return;
}
/*---proc batt_current Interface---*/
/*+++proc batt_voltage Interface+++*/
static int batt_voltage_proc_read(struct seq_file *buf, void *v)
{
	int result = 0;
	fg_get_battery_voltage(g_fgChip, &result);
	BAT_DBG("%s: %d\n", __func__, result);
	seq_printf(buf, "%d\n", result);
	return 0;
}
static int batt_voltage_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, batt_voltage_proc_read, NULL);
}

static void create_batt_voltage_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  batt_voltage_proc_open,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/batt_voltage", 0444, NULL, &proc_fops);
	if (!proc_file) {
		BAT_DBG_E("[Proc]%s failed!\n", __func__);
	}
	return;
}
/*---proc batt_voltage Interface---*/

static int batt_type_proc_read(struct seq_file *buf, void *v)
{
	int result = 0;
	char model[32] = "";

	result = g_fgChip->batt_id_ohms;
	if(!result){
		snprintf(model, sizeof(model), "%s", BATT_UNKNOWN);
		goto end;
	}	

	if (ATD_Is_battID_within_range(BATT_ID_CRITERIA))
		snprintf(model, sizeof(model), "%s", BATT_ASCENT);
	else
		snprintf(model, sizeof(model), "%s", BATT_UNKNOWN);
	
end:
	seq_printf(buf,"%s\n",model);
	BAT_DBG("%s: %dohms, %s\n", __func__, result, model);
	
	return 0;
}
static int batt_type_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, batt_type_proc_read, NULL);
}

static void create_batt_type_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  batt_type_proc_open,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/batt_type", 0444, NULL, &proc_fops);
	if (!proc_file) {
		BAT_DBG_E("[Proc]%s failed!\n", __func__);
	}
	return;
}



static int battID_status_proc_read(struct seq_file *buf, void *v)
{
	int result = 0;

	result= g_fgChip->batt_id_ohms;
	
	if(!result)
		seq_printf(buf, "FAIL\n");
	else
		seq_printf(buf, "PASS\n");

	BAT_DBG("%s: %d\n", __func__, result);
	
	return 0;
}
static int battID_status_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, battID_status_proc_read, NULL);
}

static void create_battID_status_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  battID_status_proc_open,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/battID_status", 0444, NULL, &proc_fops);
	if (!proc_file) {
		BAT_DBG_E("[Proc]%s failed!\n", __func__);
	}
	return;
}

/*+++ proc gaugeIC_status Interface+++*/
static int gaugeIC_status_proc_read(struct seq_file *buf, void *v)
{
	int result = 0,val=0;
	if (fg_get_battery_current(g_fgChip, &val) == 0) {
		result = 1;
	}
	
	BAT_DBG("%s: %d\n", __func__, result);
	seq_printf(buf, "%d\n", result);
	return 0;
}
static int gaugeIC_status_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, gaugeIC_status_proc_read, NULL);
}

static void create_gaugeIC_status_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  gaugeIC_status_proc_open,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/gaugeIC_status", 0444, NULL, &proc_fops);
	if (!proc_file) {
		BAT_DBG_E("[Proc]%s failed!\n", __func__);
	}
	return;
}



void force_soc_change_test(int l_soc)
{

	int rc;
	u8 buffer[2];
	//int bsoc;
	//u8 reg;
	u8 msoc[2],cap[2];

	msoc[0]= (u8)l_soc;
	msoc[1]= (u8)0;
	rc = fg_read(g_fgChip, BATT_SOC_FG_MONOTONIC_SOC(g_fgChip), cap, 2);

	BAT_DBG("%s:%u,%u;%u,%u\n",__func__,cap[0],cap[1],msoc[0],msoc[1]);
	
	rc = fg_sram_write(g_fgChip, MONOTONIC_SOC_WORD, MONOTONIC_SOC_OFFSET,
			msoc, 2, FG_IMA_ATOMIC);


	rc = fg_sram_read(g_fgChip, MONOTONIC_SOC_WORD, MONOTONIC_SOC_OFFSET,
		buffer, 2, FG_IMA_DEFAULT);


	rc = fg_read(g_fgChip, BATT_SOC_FG_MONOTONIC_SOC(g_fgChip), cap, 2);

	BAT_DBG("%s:%u,%u;%u,%u,%d\n",__func__,cap[0],cap[1],buffer[0],buffer[1],rc);
}



#define	MSOC_TEST	1
#define	SET_LOG_MASK 2
static ssize_t gauge_test_proc_write(struct file *filp, const char __user *buff, size_t len, loff_t *data){
	int val, cmd=0;
	char messages[8]="";

	BAT_DBG("Enter\n");
	
	len =(len > 8 ?8:len);
	if (copy_from_user(messages, buff, len)) {
		return -EFAULT;
	}
	val = (int)simple_strtol(messages, NULL, 10);
	cmd = val%10; 
	val /=10;
	
	switch (cmd) {
		case MSOC_TEST:
			if((0<val && val<256))
				force_soc_change_test(val);
			else{		
				BAT_DBG("arg invalid %d\n",val);
			}
			break;

		case SET_LOG_MASK:
			val &= BIT(8)-1;
			*g_fgChip->debug_mask	= cmd;
			BAT_DBG("set debug mask as 0x%x\n",cmd);
		default:
			break;
	}
	return len;
}


static int test_proc_read(struct seq_file *buf, void *v)
{
	int result = 0,val=0;
	if (fg_get_battery_current(g_fgChip, &val) == 0) {
		result = 1;
	}
	
	BAT_DBG("%s: %d\n", __func__, result);
	seq_printf(buf, "%d\n", result);
	return 0;
}
static int gauge_test_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, test_proc_read, NULL);
}

static void create_gauge_test_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  gauge_test_proc_open,
		.write = gauge_test_proc_write,
		.read = seq_read,
		.llseek = seq_lseek,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/gauge_test", 0666, NULL, &proc_fops);
	if (!proc_file) {
		BAT_DBG_E("[Proc]%s failed!\n", __func__);
	}
	return;
}

static int local_log_print_interval = -1;
void qpnp_smbcharger_local_polling_data_worker(int time) ;

static ssize_t local_print_log_write(struct file *filp, const char __user *buff, size_t len, loff_t *data){
	int val;
	char messages[8]="";

	
	len =(len > 8 ?8:len);
	if (copy_from_user(messages, buff, len)) {
		return -EFAULT;
	}
	val = (int)simple_strtol(messages, NULL, 10);
	if(val <=10)
		return len;

	local_log_print_interval = val;
	qpnp_smbcharger_local_polling_data_worker(local_log_print_interval);

	BAT_DBG("set log interval as %d\n",val);
	
	return len;
}


static int local_print_log_read(struct seq_file *buf, void *v)
{
	
	BAT_DBG("%s: %d\n", __func__, local_log_print_interval);
	seq_printf(buf, "%d\n", local_log_print_interval);
	return 0;
}
static int gauge_local_print_log_open(struct inode *inode, struct  file *file)
{
    return single_open(file, local_print_log_read, NULL);
}



static void create_local_print_log_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  gauge_local_print_log_open,
		.write = local_print_log_write,
		.read = seq_read,
		.llseek = seq_lseek,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/printLog", 0666, NULL, &proc_fops);
	if (!proc_file) {
		BAT_DBG_E("[Proc]%s failed!\n", __func__);
	}
	return;

}



static ssize_t thermal_test_proc_write(struct file *filp, const char __user *buff, size_t len, loff_t *data){
	int val;
	char messages[8]="";

	
	len =(len > 8 ?8:len);
	if (copy_from_user(messages, buff, len)) {
		return -EFAULT;
	}
	val = (int)simple_strtol(messages, NULL, 10);

	if(val < -200 ||val > 700)
		fake_temp = FAKE_TEMP_INIT;
	else 
		fake_temp = val;
	
	BAT_DBG("%s: set fake temperature as %d\n",__func__,fake_temp);

	return len;
}


static int thermal_proc_read(struct seq_file *buf, void *v)
{
	int result = fake_temp;
	
	BAT_DBG("%s: %d\n", __func__, result);
	seq_printf(buf, "%d\n", result);
	return 0;
}
static int thermal_test_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, thermal_proc_read, NULL);
}

static void create_thermal_test_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  thermal_test_proc_open,
		.write = thermal_test_proc_write,
		.read = seq_read,
		.llseek = seq_lseek,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/ThermalTemp", 0666, NULL, &proc_fops);
	if (!proc_file) {
		BAT_DBG_E("[Proc]%s failed!\n", __func__);
	}
	return;
}




/*--- proc gaugeIC_status Interface---*/


static void fg_cleanup(struct fg_chip *chip)
{
	power_supply_unreg_notifier(&chip->nb);
	debugfs_remove_recursive(chip->dfs_root);
	if (chip->awake_votable)
		destroy_votable(chip->awake_votable);

	if (chip->batt_id_chan)
		iio_channel_release(chip->batt_id_chan);

	dev_set_drvdata(chip->dev, NULL);
}

//[+++]Add log to show charging status in ASUSEvtlog.txt
static char *charging_stats[] = {
	"UNKNOWN",
	"CHARGING",
	"DISCHARGING",
	"NOT_CHARGING",
	"FULL",
	"QUICK_CHARGING",
	"NOT_QUICK_CHARGING"
};
static char *charging_mode[] = {
	"UNKNOWN",
	"NONE",
	"TRICKLE",
	"FAST",
	"TAPER"
};

extern char *ufp_type[];
extern char *health_type[];
//[---]Add log to show charging status in ASUSEvtlog.txt

//[+++]Add log to show charging status/type in ASUSEvtlog.txt
static int get_bat_charging_status(struct fg_chip *chip)
{
	int rc = 0;
	union power_supply_propval prop = {0, };

	if (!chip->batt_psy)
		chip->batt_psy = power_supply_get_by_name("battery");

	if (chip->batt_psy) {
		rc = chip->batt_psy->desc->get_property(chip->batt_psy,
				POWER_SUPPLY_PROP_STATUS,
				&prop);
		if (rc)
			pr_err("could't get charging status : %d\n", rc);
	}
	return prop.intval;
}
static int get_bat_charging_mode(struct fg_chip *chip)
{
	int rc = 0;
	union power_supply_propval prop = {0, };

	if (!chip->batt_psy)
		chip->batt_psy = power_supply_get_by_name("battery");

	if (chip->batt_psy) {
		rc = chip->batt_psy->desc->get_property(chip->batt_psy,
				POWER_SUPPLY_PROP_CHARGE_TYPE,
				&prop);
		if (rc)
			pr_err("could't get charging mode : %d\n", rc);
	}
	return prop.intval;
}
//[---]Add log to show charging status/type in ASUSEvtlog.txt



//[+++]Add to print the gauge status regularly

static struct delayed_work local_update_gauge_status_work;
static struct timespec g_local_last_print_time;

extern const char *asus_get_apsd_result(void);
extern int asus_get_ufp_mode(void);
extern int asus_get_batt_health(void);

static int local_print_battery_status(void) {
	int bat_vol, bat_cur, bat_cap, bat_temp, ufp_mode, bat_health;
	char battInfo[256], additionBattInfo[256];
	int charge_status, charge_mode, mSoc = 0, bSoc = 0, cSoc = 0, ocv = 0, rc=0;
	u8 socSts = 0, battSts = 0;
	const char *apsd_result;

	fg_get_battery_voltage(g_fgChip, &bat_vol);
	fg_get_battery_current(g_fgChip, &bat_cur);
	fg_get_prop_capacity(g_fgChip, &bat_cap);
	fg_get_battery_temp(g_fgChip, &bat_temp);
	
	charge_status = get_bat_charging_status(g_fgChip);
	charge_mode = get_bat_charging_mode(g_fgChip);
	apsd_result = asus_get_apsd_result();
	ufp_mode = asus_get_ufp_mode();
	bat_health = asus_get_batt_health();
	rc = fg_read(g_fgChip, BATT_INFO_INT_RT_STS(g_fgChip), &battSts, 1);
	rc = fg_read(g_fgChip, BATT_SOC_INT_RT_STS(g_fgChip), &socSts, 1);

	fg_get_msoc_raw(g_fgChip, &mSoc);
	fg_get_sram_prop(g_fgChip, FG_SRAM_BATT_SOC, &bSoc);
	bSoc = (u32)bSoc >> 24;
	fg_get_charge_counter(g_fgChip, &cSoc);	
	fg_get_sram_prop(g_fgChip, FG_SRAM_OCV, &ocv);
	
	snprintf(battInfo, sizeof(battInfo), "report Capacity ==>%d, FCC:%dmAh, BMS:%d, V:%dmV, Cur:%dmA, ",
		bat_cap,
		(int)g_fgChip->cl.nom_cap_uah/1000,
		bat_cap,
		bat_vol/1000,
		bat_cur/1000);
	snprintf(battInfo, sizeof(battInfo), "%sTemp:%d.%dC, BATID:%d, CHG_Status:%d(%s), CHG_Mode:%s, APSD_Result:%s, UFP_Mode:%s, BAT_HEALTH:%s\n",
		battInfo,
		bat_temp/10,
		bat_temp%10,
		g_fgChip->batt_id_ohms,
		charge_status,
		charging_stats[charge_status],
		charging_mode[charge_mode],
		apsd_result,
		ufp_type[ufp_mode],
		health_type[bat_health]);
	snprintf(additionBattInfo, sizeof(additionBattInfo), "csoc=%d, bsoc=%d, msoc=%d, ocv=%d, SocSts=%x, BatSts=%x\n",
		cSoc,
		bSoc,
		mSoc,
		ocv,
		socSts,
		battSts);
	
	BAT_DBG("%s: %s", __func__, battInfo);
	BAT_DBG("%s: %s", __func__, additionBattInfo);
	g_local_last_print_time = current_kernel_time();
	return 0;
}



static struct delayed_work update_gauge_status_work;
static struct timespec g_last_print_time;
void qpnp_smbcharger_polling_data_worker(int time);

static int print_battery_status(void) {
	int bat_vol, bat_cur, bat_cap, bat_temp, ufp_mode, bat_health;
	char battInfo[256], additionBattInfo[256];
	int charge_status, charge_mode, mSoc = 0, bSoc = 0, cSoc = 0, ocv = 0, rc=0;
	u8 socSts = 0, battSts = 0;
	const char *apsd_result;

	fg_get_battery_voltage(g_fgChip, &bat_vol);
	fg_get_battery_current(g_fgChip, &bat_cur);
	fg_get_prop_capacity(g_fgChip, &bat_cap);
	fg_get_battery_temp(g_fgChip, &bat_temp);
	
	charge_status = get_bat_charging_status(g_fgChip);
	charge_mode = get_bat_charging_mode(g_fgChip);
	apsd_result = asus_get_apsd_result();
	ufp_mode = asus_get_ufp_mode();
	bat_health = asus_get_batt_health();
	rc = fg_read(g_fgChip, BATT_INFO_INT_RT_STS(g_fgChip), &battSts, 1);
	rc = fg_read(g_fgChip, BATT_SOC_INT_RT_STS(g_fgChip), &socSts, 1);

	fg_get_msoc_raw(g_fgChip, &mSoc);
	fg_get_sram_prop(g_fgChip, FG_SRAM_BATT_SOC, &bSoc);
	bSoc = (u32)bSoc >> 24;
	fg_get_charge_counter(g_fgChip, &cSoc);	
	fg_get_sram_prop(g_fgChip, FG_SRAM_OCV, &ocv);
	
	snprintf(battInfo, sizeof(battInfo), "report Capacity ==>%d, FCC:%dmAh, BMS:%d, V:%dmV, Cur:%dmA, ",
		bat_cap,
		(int)g_fgChip->cl.nom_cap_uah/1000,
		bat_cap,
		bat_vol/1000,
		bat_cur/1000);
	snprintf(battInfo, sizeof(battInfo), "%sTemp:%d.%dC, BATID:%d, CHG_Status:%d(%s), CHG_Mode:%s, APSD_Result:%s, UFP_Mode:%s, BAT_HEALTH:%s\n",
		battInfo,
		bat_temp/10,
		bat_temp%10,
		g_fgChip->batt_id_ohms,
		charge_status,
		charging_stats[charge_status],
		charging_mode[charge_mode],
		apsd_result,
		ufp_type[ufp_mode],
		health_type[bat_health]);
	snprintf(additionBattInfo, sizeof(additionBattInfo), "csoc=%d, bsoc=%d, msoc=%d, ocv=%d, SocSts=%x, BatSts=%x\n",
		cSoc,
		bSoc,
		mSoc,
		ocv,
		socSts,
		battSts);
	
	ASUSEvtlog("[BAT][Ser]%s", battInfo);
	BAT_DBG("%s: %s", __func__, additionBattInfo);
	g_last_print_time = current_kernel_time();
	return 0;
}

void qpnp_smbcharger_local_polling_data_worker(int time) {
	int fix_time = time;
	if(fix_time < 10)
		fix_time=10;

	cancel_delayed_work(&local_update_gauge_status_work);
	schedule_delayed_work(&local_update_gauge_status_work, fix_time * HZ);
}

void qpnp_smbcharger_polling_data_worker(int time) {
	cancel_delayed_work(&update_gauge_status_work);
	schedule_delayed_work(&update_gauge_status_work, time * HZ);
}

void static local_update_gauge_status_worker(struct work_struct *dat)
{
	int ret = 0;

	ret = local_print_battery_status();
	qpnp_smbcharger_local_polling_data_worker(local_log_print_interval);
}


void static update_gauge_status_worker(struct work_struct *dat)
{
	int ret = 0;
	
	if (!g_fgChip->profile_loaded) {
		BAT_DBG("%s: profile not loaded yet, delay 5s\n", __func__);
		qpnp_smbcharger_polling_data_worker(5);
	} else{
		ret = print_battery_status();
		if (ret == 0) {
			qpnp_smbcharger_polling_data_worker(180);
		} else{
			BAT_DBG("%s: charger not ready yet, delay 5s\n", __func__);
			qpnp_smbcharger_polling_data_worker(5);
		}
	}
}
//[---]Add to print the gauge status regularly

struct switch_dev batt_dev;
static ssize_t batt_switch_name_Zeus(struct switch_dev *sdev, char *buf)
{
	char bat_modelName[16] = "";
	char bat_cellCode = 'X';
	int bat_ID = 0;
	int bat_profileVersion = 1;
	const char* bat_driverVersion = "14.2400.1704.25";
	if (g_fgChip) {
		if (g_fgChip->profile_available  && 
			(!strcmp(g_fgChip->bp.batt_type_str, BATT_TYPE_ZEUS_4P25V))) {
		snprintf(bat_modelName, sizeof(bat_modelName), "%s", BATT_MODELNAME_ZEUS);
		bat_cellCode = 'T';
		bat_ID = BATT_ID_51K_INDEX;
		bat_profileVersion = 3;	
		}
		else if (g_fgChip->profile_available  && 
			(!strcmp(g_fgChip->bp.batt_type_str, BATT_TYPE_ZEUS_4P35V))) {
		snprintf(bat_modelName, sizeof(bat_modelName), "%s", BATT_MODELNAME_ZEUS);
		bat_cellCode = 'T';
		bat_ID = BATT_ID_100K_INDEX;
		bat_profileVersion = 3;	
		}		
	}
	return sprintf(buf, "%s-%c-%02d-%04d-%s\n", bat_modelName, bat_cellCode, bat_ID, bat_profileVersion, bat_driverVersion);
}

void register_battery_version(void)
{
	/* register switch device for battery information versions report */
	batt_dev.name = "battery";
	batt_dev.print_name = batt_switch_name_Zeus;
	if (switch_dev_register(&batt_dev) < 0) {
		BAT_DBG_E("%s: fail to register battery switch\n", __func__);
	}
}



struct switch_dev cyclecount_dev;
static ssize_t batt_cycle_count_Zeus(struct switch_dev *sdev, char *buf)
{

	return sprintf(buf, "10\n");
}

void register_battery_cycle_count(void)
{
	/* register switch device for battery cyclecount */
	cyclecount_dev.name = "cycle_count";
	cyclecount_dev.print_name = batt_cycle_count_Zeus;
	if (switch_dev_register(&cyclecount_dev) < 0) {
		BAT_DBG_E("%s: fail to register cyclecount switch\n", __func__);
	}
}


static ssize_t cycle_count_proc_write(struct file *filp, const char __user *buff, size_t len, loff_t *data){
	int cnt=0,i=0,rc=0;
	char messages[8]="";
	
	len =(len > 8 ?8:len);
	if (copy_from_user(messages, buff, len)) {
		return -EFAULT;
	}
	cnt = (int)simple_strtol(messages, NULL, 10);

	for(i=0;i<BUCKET_COUNT;i++){
		g_fgChip->cyc_ctr.count[i] = cnt-1;
		rc += fg_inc_store_cycle_ctr(g_fgChip,i);
	}
	BAT_DBG("write cycle count as %d, rc = %d\n",cnt+1, rc);

	return len;
}

static int cycle_count_proc_read(struct seq_file *buf, void *v)
{
	//int result = 0;
	int id=0;
	u16 val[BUCKET_COUNT];
	u32 count=0;
	
	mutex_lock(&g_fgChip->cyc_ctr.lock);

	for(id=0;id<BUCKET_COUNT;id++){
		val[id] = g_fgChip->cyc_ctr.count[id];
		count += val[id];
		BAT_DBG("bucket %d count: %d\n",id ,val[id]);
	}

	count /= (BUCKET_COUNT-1);	// take 87.5% for 1 cycle
	
	mutex_unlock(&g_fgChip->cyc_ctr.lock);
	BAT_DBG("%s: %d\n", __func__, count);
	seq_printf(buf, "%d\n", count);
	return 0;
}
static int cycle_count_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, cycle_count_proc_read, NULL);
}

static void create_cycle_count_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  cycle_count_proc_open,
		.read = seq_read,
		.write = cycle_count_proc_write,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/batt_cycle_count", 0644, NULL, &proc_fops);
	if (!proc_file) {
		BAT_DBG_E("[Proc]%s failed!\n", __func__);
	}
	return;
}


static int fg_gen3_probe(struct platform_device *pdev)
{
	struct fg_chip *chip;
	struct power_supply_config fg_psy_cfg;
	int rc, msoc, volt_uv, batt_temp;

	/* Defer bms driver until bq27546 gauge driver i2c being checked */

	BAT_DBG("%s: start\n", __func__);
	
	
	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->dev = &pdev->dev;
	chip->debug_mask = &fg_gen3_debug_mask;
	chip->irqs = fg_irqs;
	chip->charge_status = -EINVAL;
	chip->prev_charge_status = -EINVAL;
	chip->ki_coeff_full_soc = -EINVAL;
	chip->regmap = dev_get_regmap(chip->dev->parent, NULL);
	if (!chip->regmap) {
		dev_err(chip->dev, "Parent regmap is unavailable\n");
		return -ENXIO;
	}

	chip->batt_id_chan = iio_channel_get(chip->dev, "rradc_batt_id");
	if (IS_ERR(chip->batt_id_chan)) {
		if (PTR_ERR(chip->batt_id_chan) != -EPROBE_DEFER)
			pr_err("batt_id_chan unavailable %ld\n",
				PTR_ERR(chip->batt_id_chan));
		rc = PTR_ERR(chip->batt_id_chan);
		chip->batt_id_chan = NULL;
		return rc;
	}

	rc = of_property_match_string(chip->dev->of_node,
				"io-channel-names", "rradc_die_temp");
	if (rc >= 0) {
		chip->die_temp_chan = iio_channel_get(chip->dev,
						"rradc_die_temp");
		if (IS_ERR(chip->die_temp_chan)) {
			if (PTR_ERR(chip->die_temp_chan) != -EPROBE_DEFER)
				pr_err("rradc_die_temp unavailable %ld\n",
					PTR_ERR(chip->die_temp_chan));
			rc = PTR_ERR(chip->die_temp_chan);
			chip->die_temp_chan = NULL;
			return rc;
		}
	}

	chip->awake_votable = create_votable("FG_WS", VOTE_SET_ANY, fg_awake_cb,
					chip);
	if (IS_ERR(chip->awake_votable)) {
		rc = PTR_ERR(chip->awake_votable);
		return rc;
	}

	g_fgChip = chip;

	rc = fg_parse_dt(chip);
	if (rc < 0) {
		dev_err(chip->dev, "Error in reading DT parameters, rc:%d\n",
			rc);
		goto exit;
	}

	mutex_init(&chip->bus_lock);
	mutex_init(&chip->sram_rw_lock);
	mutex_init(&chip->cyc_ctr.lock);
	mutex_init(&chip->cl.lock);
	mutex_init(&chip->batt_avg_lock);
	mutex_init(&chip->charge_full_lock);
	mutex_init(&chip->charge_status_lock);	
	init_completion(&chip->soc_update);
	init_completion(&chip->soc_ready);
	INIT_DELAYED_WORK(&chip->profile_load_work, profile_load_work);
	INIT_WORK(&chip->status_change_work, status_change_work);
	INIT_WORK(&chip->cycle_count_work, cycle_count_work);
	INIT_DELAYED_WORK(&chip->batt_avg_work, batt_avg_work);
	INIT_DELAYED_WORK(&chip->sram_dump_work, sram_dump_work);
	INIT_DELAYED_WORK(&update_gauge_status_work, update_gauge_status_worker);
	INIT_DELAYED_WORK(&local_update_gauge_status_work, local_update_gauge_status_worker);
	INIT_DELAYED_WORK(&fix_maint_soc_work, fix_maint_soc_worker);

	rc = fg_get_batt_id(chip);
	if (rc < 0) {
		pr_err("Error in getting battery id, rc:%d\n", rc);
		return rc;
	}

	rc = fg_get_batt_profile(chip);
	if (rc < 0) {
		chip->soc_reporting_ready = true;
		pr_warn("profile for batt_id=%dKOhms not found..using OTP, rc:%d\n",
			chip->batt_id_ohms / 1000, rc);
	}

	rc = fg_memif_init(chip);
	if (rc < 0) {
		dev_err(chip->dev, "Error in initializing FG_MEMIF, rc:%d\n",
			rc);
		goto exit;
	}

	rc = fg_hw_init(chip);
	if (rc < 0) {
		dev_err(chip->dev, "Error in initializing FG hardware, rc:%d\n",
			rc);
		goto exit;
	}

	platform_set_drvdata(pdev, chip);


	/* Register the power supply */

		BAT_DBG("%s: use BMS gauge\n", __func__);
		fg_psy_cfg.drv_data = chip;
		fg_psy_cfg.of_node = NULL;
		fg_psy_cfg.supplied_to = NULL;
		fg_psy_cfg.num_supplicants = 0;
		chip->fg_psy = devm_power_supply_register(chip->dev, &fg_psy_desc,
				&fg_psy_cfg);
		if (IS_ERR(chip->fg_psy)) {
			pr_err("failed to register fg_psy rc = %ld\n",
					PTR_ERR(chip->fg_psy));
			goto exit;
		}




	chip->nb.notifier_call = fg_notifier_cb;
	rc = power_supply_reg_notifier(&chip->nb);
	if (rc < 0) {
		pr_err("Couldn't register psy notifier rc = %d\n", rc);
		goto exit;
	}

	rc = fg_register_interrupts(chip);
	if (rc < 0) {
		dev_err(chip->dev, "Error in registering interrupts, rc:%d\n",
			rc);
		goto exit;
	}

	/* Keep SOC_UPDATE irq disabled until we require it */
	if (fg_irqs[SOC_UPDATE_IRQ].irq)
		disable_irq_nosync(fg_irqs[SOC_UPDATE_IRQ].irq);

	/* Keep BSOC_DELTA_IRQ irq disabled until we require it */
	if (fg_irqs[BSOC_DELTA_IRQ].irq) {
		disable_irq_wake(fg_irqs[BSOC_DELTA_IRQ].irq);
		disable_irq_nosync(fg_irqs[BSOC_DELTA_IRQ].irq);
		chip->bsoc_delta_irq_en = false;
	}

	rc = fg_debugfs_create(chip);
	if (rc < 0) {
		dev_err(chip->dev, "Error in creating debugfs entries, rc:%d\n",
			rc);
		goto exit;
	}

	rc = fg_get_battery_voltage(chip, &volt_uv);
	if (!rc)
		rc = fg_get_prop_capacity(chip, &msoc);

	if (!rc)
		rc = fg_get_battery_temp(chip, &batt_temp);

	if (!rc) {
		pr_info("battery SOC:%d voltage: %duV temp: %d id: %dKOhms\n",
			msoc, volt_uv, batt_temp, chip->batt_id_ohms / 1000);
		rc = fg_esr_filter_config(chip, batt_temp);
		if (rc < 0)
			pr_err("Error in configuring ESR filter rc:%d\n", rc);
	}

	device_init_wakeup(chip->dev, true);
	if (chip->profile_available)
		schedule_delayed_work(&chip->profile_load_work, 0);

		create_gauge_test_proc_file();
		create_local_print_log_proc_file();

		create_batt_mili_temp_proc_file();
		create_batt_current_proc_file();
		create_batt_voltage_proc_file();
		create_gaugeIC_status_proc_file();
		create_battID_status_proc_file();
		create_batt_type_proc_file();

		create_thermal_test_proc_file();
		create_cycle_count_proc_file();
		
	
	register_battery_version();
	register_battery_cycle_count();
	qpnp_smbcharger_polling_data_worker(5);//Start the status report of fuel gauge

	BAT_DBG("FG GEN3 driver probed successfully\n");
	return 0;
exit:
	fg_cleanup(chip);
	return rc;
}


#ifdef CONFIG_QPNP_FG_SUSPEND_PREDICT

#define RESET_TIME_THD	180
#define MODIFY_SOC_TIME_THD	18000
int g_late_resume_time;
int g_late_resume_soc;
int g_func_time_stamp;
int g_expect_soc = 100;
int g_csoc_value_stamp;
bool g_func_enable;

void set_stamps(int cSoc, struct timespec mtNow){
	g_csoc_value_stamp = cSoc;
	g_func_time_stamp = mtNow.tv_sec;

#ifdef	CHECK_PARAM
	BAT_DBG("%d,%d\n",g_csoc_value_stamp,g_func_time_stamp);
#endif
}


bool is_bms_charging(void)
{

	int rc;
	union power_supply_propval prop = {0, };

	rc = power_supply_get_property(g_fgChip->batt_psy, POWER_SUPPLY_PROP_STATUS,
			&prop);
	
	if (rc < 0) {
		BAT_DBG_E("Error in getting charging status, rc=%d\n", rc);
		return rc;
	}

	return (prop.intval != POWER_SUPPLY_STATUS_DISCHARGING);

}
void bms_modify_soc_early_suspend(void)
{
	static bool g_func_1st_init = true;
	struct timespec mtNow;
	int bat_cap = 0, cSoc = 0;
	if (g_fgChip) {
		mtNow = current_kernel_time();
		fg_get_prop_capacity(g_fgChip, &bat_cap);
		fg_get_charge_counter(g_fgChip, &cSoc);
		/*BSP david: only modify soc when discharging*/
		if (!is_bms_charging()) {
			g_func_enable = true;
			/*BSP david: if first modify soc, or resume more than 3 min, or soc different with late resume,
						restart modify soc*/
			if (g_func_1st_init || mtNow.tv_sec - g_late_resume_time > RESET_TIME_THD || bat_cap != g_late_resume_soc) {
				g_func_1st_init = false;
				g_expect_soc = bat_cap;
				set_stamps(cSoc,mtNow);
				BAT_DBG("%s:first determine, soc = %d, csoc = %d, start_time = %d\n", __func__, 
					g_expect_soc, g_csoc_value_stamp, g_func_time_stamp);
			} else{
				BAT_DBG("%s:not first determine, don't need to change!\n", __func__);
			}
		} else{
			BAT_DBG("%s:suspend with charging, don't need to fix soc!\n", __func__);
			g_func_enable = false;
			g_late_resume_time = 0;
		}
	} else{
		BAT_DBG_E("%s: bms register error!\n", __func__);
	}
}
void bms_modify_soc_late_resume(void)
{
	struct timespec mtNow;
	mtNow = current_kernel_time();
	if (g_fgChip && g_func_enable) {
		fg_get_prop_capacity(g_fgChip, &g_late_resume_soc);
		g_late_resume_time = mtNow.tv_sec;
		BAT_DBG("%s:resume without charging, record time: %d\n", __func__, g_late_resume_time);
	}
}
static bool exceed_modify_soc_time_thd(int l_input)
{
	return (l_input - g_func_time_stamp > MODIFY_SOC_TIME_THD);
}
static bool exceed_unit_csoc_thd(int l_input)
{
	int unit_csoc = (g_fgChip->cl.learned_cc_uah)/100;

	if(!unit_csoc){
		unit_csoc = g_fgChip->cl.nom_cap_uah/100;
		BAT_DBG("%s: apply nom_cap_uah(%lld)\n",__func__, g_fgChip->cl.nom_cap_uah);
	}

	return (g_csoc_value_stamp - l_input > unit_csoc);
}

#define EMPTY_CAPACITY		0
static u8 convert_soc_to_msoc(int l_soc)
{
	int msoc, i;

	if (l_soc == EMPTY_CAPACITY)
		return 0;
	else if (l_soc == FULL_CAPACITY)
		return FULL_SOC_RAW;
	
	msoc = DIV_ROUND_CLOSEST(l_soc*FULL_SOC_RAW, FULL_CAPACITY);

	for (i = 1; i <= 2; i++){
		if (DIV_ROUND_CLOSEST((msoc+1) * FULL_CAPACITY, FULL_SOC_RAW)  == l_soc)
			msoc++;
		else	break;	
	}

	BAT_DBG("%s: %d(%%) -> %d(raw)\n",__func__,l_soc,msoc);

	return (u8)msoc;
}
static void force_soc_change(int l_soc)
{

	int rc;
	u8 msoc[2],cap[2];

	msoc[0]= convert_soc_to_msoc(l_soc);
	msoc[1]= msoc[0];
	
	rc = fg_sram_write(g_fgChip, MONOTONIC_SOC_WORD, MONOTONIC_SOC_OFFSET,
			msoc, 2, FG_IMA_ATOMIC);
	if(rc)
		BAT_DBG_E("%s: writing sram reg %d fail, rc %d\n ",__func__,MONOTONIC_SOC_WORD,rc);
	
	rc = fg_sram_read(g_fgChip, MONOTONIC_SOC_WORD, MONOTONIC_SOC_OFFSET,
		cap, 2, FG_IMA_DEFAULT);

	BAT_DBG("read:%u,%u; write:%u,%u\n",cap[0],cap[1],msoc[0],msoc[1]);

}
static void bms_modify_soc(void)
{
	int cSoc = 0, bat_cap = 0, curr_tme;
	struct timespec mtNow;
	mtNow = current_kernel_time();
	fg_get_prop_capacity(g_fgChip, &bat_cap);
	fg_get_charge_counter(g_fgChip, &cSoc);	
	curr_tme = mtNow.tv_sec;
	
	/*BSP david: if soc <= 1, don't do anything*/
	if (g_expect_soc > 1) {
		/*BSP david: if soc has changed, reset the timer; if not, check the csoc (coulomb counter)*/
		if (bat_cap >= g_expect_soc) {
			/*BSP david: when soc not changed for long time(5 hour), and csoc changed more than 1%, force soc change*/
			if (exceed_modify_soc_time_thd(curr_tme) && exceed_unit_csoc_thd(cSoc)) {
				set_stamps(cSoc,mtNow);
				force_soc_change(--g_expect_soc);
				BAT_DBG("%s: fix soc, expect = %d, soc = %d, csoc = %d, start_time = %d\n", __func__,
					g_expect_soc, bat_cap, cSoc, g_func_time_stamp);
			} else{
				BAT_DBG("%s: cur_time = %d, csoc = %d\n", __func__, curr_tme, cSoc);
			}
		} else{
			g_expect_soc = bat_cap;
			set_stamps(cSoc,mtNow);
			BAT_DBG("%s:soc changed, reset timer, soc = %d, csoc = %d, start_time = %d\n", __func__, 
				g_expect_soc, g_csoc_value_stamp, g_func_time_stamp);
		}
	}
}
#endif



static int fg_gen3_suspend(struct device *dev)
{
	struct fg_chip *chip = dev_get_drvdata(dev);
	int rc;

	if (chip->dt.esr_timer_awake > 0 && chip->dt.esr_timer_asleep > 0) {
		rc = fg_set_esr_timer(chip, chip->dt.esr_timer_asleep, false,
				      FG_IMA_NO_WLOCK);
		if (rc < 0) {
			pr_err("Error in setting ESR timer during suspend, rc=%d\n",
			       rc);
			return rc;
		}
	}

	cancel_delayed_work_sync(&chip->batt_avg_work);
	if (fg_sram_dump)
		cancel_delayed_work_sync(&chip->sram_dump_work);
	return 0;
}

static int fg_gen3_resume(struct device *dev)
{
	struct fg_chip *chip = dev_get_drvdata(dev);
	int rc;
	struct timespec mtNow;

	mtNow = current_kernel_time();
	if (mtNow.tv_sec - g_last_print_time.tv_sec >= REPORT_CAPACITY_POLLING_TIME) {
		qpnp_smbcharger_polling_data_worker(0);
	}
	if (local_log_print_interval > 10 &&
		mtNow.tv_sec - g_local_last_print_time.tv_sec >= local_log_print_interval) {
		qpnp_smbcharger_local_polling_data_worker(0);
	}
	
#ifdef CONFIG_QPNP_FG_SUSPEND_PREDICT
	/*BSP david: modify last soc if it doesn't decrease after long suspend*/
	if (g_fgChip && g_func_enable) {
		bms_modify_soc();
	}
#endif

	if (chip->dt.esr_timer_awake > 0 && chip->dt.esr_timer_asleep > 0) {
		rc = fg_set_esr_timer(chip, chip->dt.esr_timer_awake, false,
				      FG_IMA_DEFAULT);
		if (rc < 0) {
			pr_err("Error in setting ESR timer during resume, rc=%d\n",
			       rc);
			return rc;
		}
	}

	fg_circ_buf_clr(&chip->ibatt_circ_buf);
	fg_circ_buf_clr(&chip->vbatt_circ_buf);
	schedule_delayed_work(&chip->batt_avg_work, 0);
	if (fg_sram_dump)
		schedule_delayed_work(&chip->sram_dump_work,
				msecs_to_jiffies(fg_sram_dump_period_ms));
	return 0;
}

static const struct dev_pm_ops fg_gen3_pm_ops = {
	.suspend	= fg_gen3_suspend,
	.resume		= fg_gen3_resume,
};

static int fg_gen3_remove(struct platform_device *pdev)
{
	struct fg_chip *chip = dev_get_drvdata(&pdev->dev);

	fg_cleanup(chip);
	return 0;
}

static const struct of_device_id fg_gen3_match_table[] = {
	{.compatible = FG_GEN3_DEV_NAME},
	{},
};

static struct platform_driver fg_gen3_driver = {
	.driver = {
		.name = FG_GEN3_DEV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = fg_gen3_match_table,
		.pm		= &fg_gen3_pm_ops,
	},
	.probe		= fg_gen3_probe,
	.remove		= fg_gen3_remove,
};

static int __init fg_gen3_init(void)
{
	return platform_driver_register(&fg_gen3_driver);
}

static void __exit fg_gen3_exit(void)
{
	return platform_driver_unregister(&fg_gen3_driver);
}

module_init(fg_gen3_init);
module_exit(fg_gen3_exit);

MODULE_DESCRIPTION("QPNP Fuel gauge GEN3 driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" FG_GEN3_DEV_NAME);