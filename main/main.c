#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "driver/i2s_common.h"
#include "driver/i2s_std.h"
#include "driver/i2s_types.h"

#include "freertos/FreeRTOS.h"

#include "hal/i2s_types.h"

#include "soc/gpio_num.h"

#define SAMPLES 256


void app_main(void) {
	i2s_chan_handle_t rx_i2s0;
	i2s_chan_handle_t rx_i2s1;
	i2s_chan_config_t chan_cfg0 = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER); //mic1 + mic2 
	i2s_chan_config_t chan_cfg1 = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_SLAVE); //mic3
	
	ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg0, NULL, &rx_i2s0));
	ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg1, NULL, &rx_i2s1));

	//I2S0 master: Mic1 (L) + Mic2 (R)
	i2s_std_config_t cfg_0 = {
		.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(48000),
		.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
		.gpio_cfg = {
			.mclk = I2S_GPIO_UNUSED,
			.bclk = GPIO_NUM_26,
			.ws = GPIO_NUM_25,
			.dout = I2S_GPIO_UNUSED,
			.din = GPIO_NUM_34,
		},
		
	};

	//I2S1 slave: Mic3 (L)
//	i2s_std_config_t cfg_1 = {
//		.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(48000),
//		.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
//		.gpio_cfg = {
//			.mclk = I2S_GPIO_UNUSED,
//			.bclk = GPIO_NUM_26,
//			.ws = GPIO_NUM_25,
//			.dout = I2S_GPIO_UNUSED,
//			.din = GPIO_NUM_35,
//		},
//	};

	
	ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_i2s0, &cfg_0));
//	i2s_channel_init_std_mode(rx_i2s1, &cfg_1);

//	i2s_channel_enable(rx_i2s1);
	ESP_ERROR_CHECK(i2s_channel_enable(rx_i2s0));

	

}
