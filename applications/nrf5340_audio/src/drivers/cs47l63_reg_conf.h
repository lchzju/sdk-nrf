/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _CS47L63_REG_CONF_H_
#define _CS47L63_REG_CONF_H_

#include "cs47l63_spec.h"

/* Magic value to signal a sleep instead of register address.
 * This can be used e.g. after resets where time is needed before
 * a device is ready.
 * Note that this is a busy wait, and should only be used sparingly where fast
 * execution is not critical.
 *
 * 0001 is used as the reg addr. In case of a fault, this reg is read only.
 */
#define SPI_BUSY_WAIT 0x0001
#define SPI_BUSY_WAIT_US_1000 1000
#define SPI_BUSY_WAIT_US_3000 3000

#define MAX_VOLUME_REG_VAL 0x80
#define MAX_VOLUME_DB 64
#define OUT_VOLUME_DEFAULT 0x62
#define VOLUME_UPDATE_BIT (1 << 9)

#define CS47L63_SOFT_RESET_VAL 0x5A000000

/* clang-format off */
/* Set up clocks */
const uint32_t clock_configuration[][2] = {
	{ CS47L63_SAMPLE_RATE3, 0x0012 },
	{ CS47L63_SAMPLE_RATE2, 0x0002 },
	{ CS47L63_SAMPLE_RATE1, 0x0003 },
	{ CS47L63_SYSTEM_CLOCK1, 0x034C },
	{ CS47L63_ASYNC_CLOCK1, 0x034C },
	{ CS47L63_FLL1_CONTROL2, 0x88200008 },
	{ CS47L63_FLL1_CONTROL3, 0x10000 },
	{ CS47L63_FLL1_GPIO_CLOCK, 0x0005 },
	{ CS47L63_FLL1_CONTROL1, 0x0001 },
};
/* clang-format on */

/* Set up GPIOs */
const uint32_t GPIO_configuration[][2] = {
	{ CS47L63_GPIO6_CTRL1, 0x61000001 },
	{ CS47L63_GPIO7_CTRL1, 0x61000001 },
	{ CS47L63_GPIO8_CTRL1, 0x61000001 },

	/* Enable CODEC LED */
	{ CS47L63_GPIO10_CTRL1, 0x41008001 },
};

const uint32_t pdm_mic_enable_configure[][2] = {
	/* Set MICBIASes */
	{ CS47L63_LDO2_CTRL1, 0x0005 },
	{ CS47L63_MICBIAS_CTRL1, 0x00EC },
	{ CS47L63_MICBIAS_CTRL5, 0x0272 },

	/* Enable IN1L */
	{ CS47L63_INPUT_CONTROL, 0x000F },

	/* Enable PDM mic as digital input */
	{ CS47L63_INPUT1_CONTROL1, 0x50021 },

	/* Un-mute and set gain to 0dB */
	{ CS47L63_IN1L_CONTROL2, 0x800080 },
	{ CS47L63_IN1R_CONTROL2, 0x800080 },

	/* Volume Update */
	{ CS47L63_INPUT_CONTROL3, 0x20000000 },

#if 1
	/* Route PDM MIC to I2S Tx */
	{ CS47L63_ASP1TX1_INPUT1, 0x800010 }, //reg addr 0x8200, ASP1TX1_SRC1 [8:0] = 0x10 = IN1L
	{ CS47L63_ASP1TX2_INPUT1, 0x800011 }, //reg addr 0x8210,ASP1TX2_SRC1 [8:0] = 0x11 = IN1R

#else
	/* With only one microphone, the audio data typically appears only on one of the I2S channels (usually the left channel).
	 * The right channel might contain invalid data or zeroes unless specifically configured otherwise.
	 * The microphone data is sent to the left channel of the I2S bus, and the right channel might be muted or contain no valid data. 
	 * You can configure the codec to duplicate the mono microphone data to both I2S channels for compatibility with stereo processing.
	 */
	{ CS47L63_ASP1TX1_INPUT1, 0x800010 },  // Route PDM MIC to ASP1TX1 (left channel)
	{ CS47L63_ASP1TX2_INPUT1, 0x800010 },  // Optionally, duplicate the same PDM MIC to ASP1TX2 (right channel)
#endif
};

/* Set up input */
const uint32_t line_in_enable[][2] = {
	/* Select LINE-IN as analog input */
	{ CS47L63_INPUT2_CONTROL1, 0x50020 },

	/* Set IN2L and IN2R to single-ended */
	{ CS47L63_IN2L_CONTROL1, 0x10000000 },
	{ CS47L63_IN2R_CONTROL1, 0x10000000 },

	/* Un-mute and set gain to 0dB */
	{ CS47L63_IN2L_CONTROL2, 0x800080 },
	{ CS47L63_IN2R_CONTROL2, 0x800080 },

	/* Enable IN2L and IN2R */
	{ CS47L63_INPUT_CONTROL, 0x000F },

	/* Volume Update */
	{ CS47L63_INPUT_CONTROL3, 0x20000000 },

	/* Route IN2L and IN2R to I2S Tx */
	{ CS47L63_ASP1TX1_INPUT1, 0x800012 }, // reg addr 0x8200, ASP1TX1_SRC1 [8:0] = 0x012 = IN2L signal path
	{ CS47L63_ASP1TX2_INPUT1, 0x800013 }, // reg addr 0x8210,ASP1TX2_SRC1 [8:0] = 0x013 = IN2R signal path
};

/* Set up output */
const uint32_t output_enable[][2] = {
	{ CS47L63_OUTPUT_ENABLE_1, 0x0002 },
	{ CS47L63_OUT1L_INPUT1, 0x800020 }, // reg addr 0x8100, OUT1L_SRC1 [8:0] x_SRCn = 0x020 = ASP1 RX1
	{ CS47L63_OUT1L_INPUT2, 0x800021 }, // reg addr 0x8104, OUT1L_SRC2 [8:0] x_SRCn = 0x021 = ASP1 RX2
};

const uint32_t output_disable[][2] = {
	{ CS47L63_OUTPUT_ENABLE_1, 0x00 },
};

/* Set up ASP1 (I2S) */
const uint32_t asp1_enable[][2] = {
	/* Enable ASP1 GPIOs */
	{ CS47L63_GPIO1_CTRL1, 0x61000000 },
	{ CS47L63_GPIO2_CTRL1, 0xE1000000 },
	{ CS47L63_GPIO3_CTRL1, 0xE1000000 },
	{ CS47L63_GPIO4_CTRL1, 0xE1000000 },
	{ CS47L63_GPIO5_CTRL1, 0x61000001 },

/* Set correct sample rate */
#if CONFIG_AUDIO_SAMPLE_RATE_16000_HZ
	{ CS47L63_SAMPLE_RATE1, 0x000000012 },
#elif CONFIG_AUDIO_SAMPLE_RATE_24000_HZ
	{ CS47L63_SAMPLE_RATE1, 0x000000002 },
#elif CONFIG_AUDIO_SAMPLE_RATE_48000_HZ
	{ CS47L63_SAMPLE_RATE1, 0x000000003 },
#endif
	/* Disable unused sample rates */
	{ CS47L63_SAMPLE_RATE2, 0 },
	{ CS47L63_SAMPLE_RATE3, 0 },
	{ CS47L63_SAMPLE_RATE4, 0 },

	/* Set ASP1 in slave mode and 16 bit per channel */
	{ CS47L63_ASP1_CONTROL2, 0x10100200 },  //reg addr 0x6008, ASP1_FMT[2:0] = 010, I2S Mode, slave
	{ CS47L63_ASP1_CONTROL3, 0x0000 },      //reg addr 0x600C, ASP1_DOUT_HIZ_CTRL[1:0] = 00， Logic 0 during unused time slots, Logic 0 if all transmit channels are disabled
	{ CS47L63_ASP1_DATA_CONTROL1, 0x0020 }, //reg addr 0x6030, ASP1_DATA_CONTROL1：ASP1_TX_WL[5:0]=32， ASP1 TX Data Width (Number of valid data bits per slot)
	{ CS47L63_ASP1_DATA_CONTROL5, 0x0020 }, //reg addr 0x6040, ASP1_DATA_CONTROL5：ASP1_RX_WL[5:0]=32， ASP1 RX Data Width (Number of valid data bits per slot)
	{ CS47L63_ASP1_ENABLES1, 0x30003 },     //reg addr 0x6000, ASP1_ENABLES1: ASP1_TX1_EN=1, ASP1_TX2_EN=1, ASP1_RX1_EN=1, ASP1_RX2_EN=1
};

const uint32_t FLL_toggle[][2] = {
	{ CS47L63_FLL1_CONTROL1, 0x0000 },
	{ SPI_BUSY_WAIT, SPI_BUSY_WAIT_US_1000 },
	{ CS47L63_FLL1_CONTROL1, 0x0001 },
};

const uint32_t soft_reset[][2] = {
	{ CS47L63_SFT_RESET, CS47L63_SOFT_RESET_VAL },
	{ SPI_BUSY_WAIT, SPI_BUSY_WAIT_US_3000 },
};

#endif /* _CS47L63_REG_CONF_H_ */
