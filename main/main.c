#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "dht.h"
#include "esp_err.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/i2s_common.h"
#include "driver/i2s_std.h"
#include "driver/i2s_types.h"

#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"

#define SAMPLES 256
#define FREQ_SAMPLING 48000

#define DHT_PIN 13
#define DHT_TYPE DHT_TYPE_DHT11
#define QUEUE_LENGTH 2
#define TEMP_UNIT 'C'

QueueHandle_t dht11_queue;


typedef struct {
	int16_t temperature; 
} SensorRead;

void readSensorDHT11(void* params) {
	SensorRead read;
	gpio_reset_pin(DHT_PIN);
	vTaskDelay(pdMS_TO_TICKS(1000));
	while (1) {
		esp_err_t res = dht_read_data(DHT_TYPE, DHT_PIN, NULL, &read.temperature);
		if (res != ESP_OK) {
			printf("There was a PROBLEM with reading sensor. Retrying...\n");	
		}
		xQueueSend(dht11_queue, &res, pdMS_TO_TICKS(1000));


	}
}


void app_main(void) {

	xQueueCreate(QUEUE_LENGTH, sizeof(int16_t));

	i2s_chan_handle_t rx_i2s0;
	i2s_chan_handle_t rx_i2s1;
	i2s_chan_config_t chan_cfg0 = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER); //mic1 + mic2 
	i2s_chan_config_t chan_cfg1 = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_SLAVE); //mic3
	
	ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg0, NULL, &rx_i2s0));
	ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg1, NULL, &rx_i2s1));

	//I2S0 master: Mic1 (L) + Mic2 (R) => teraz dla próby jest zrobione
	i2s_std_config_t cfg_0 = {
		.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
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
	i2s_std_config_t cfg_1 = {
		.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
		.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
		.gpio_cfg = {
      		.mclk = I2S_GPIO_UNUSED,
			.bclk = GPIO_NUM_26,
			.ws = GPIO_NUM_25,
			.dout = I2S_GPIO_UNUSED,
			.din = GPIO_NUM_35,
		},
	};

	
	ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_i2s0, &cfg_0));
	ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_i2s1, &cfg_1));

	ESP_ERROR_CHECK(i2s_channel_enable(rx_i2s1));
	ESP_ERROR_CHECK(i2s_channel_enable(rx_i2s0));
	
	size_t bytes_read_i2s0 = 0;
	size_t bytes_read_i2s1 = 0;
	int32_t* buffer_i2s0 = (int32_t*)malloc(sizeof(int32_t) * SAMPLES * 2);
	int32_t* buffer_i2s1 = (int32_t*)malloc(sizeof(int32_t) * SAMPLES * 2);
	if (buffer_i2s0 == NULL) {
		ESP_LOGE("MIC", "Error with memory allocation for buffer (i2s0)");	
		return;
	}

	if (buffer_i2s1 == NULL) {
		ESP_LOGE("MIC", "Error with memory allocation for buffer (i2s1)");	
		free(buffer_i2s0);
		return;
	}

	while(1) {
		esp_err_t read_i2s0 = i2s_channel_read(rx_i2s0, buffer_i2s0, sizeof(int32_t) * SAMPLES * 2, &bytes_read_i2s0, portMAX_DELAY);	
		esp_err_t read_i2s1 = i2s_channel_read(rx_i2s1, buffer_i2s1, sizeof(int32_t) * SAMPLES * 2, &bytes_read_i2s1, portMAX_DELAY);
		
		SensorRead read;
		xQueueReceive(dht11_queue, &read.temperature, pdMS_TO_TICKS(1000));

		if (read_i2s0 != ESP_OK) {
			ESP_LOGW("MIC", "Error with read from trunk line (i2s0)");
			continue;
		}

		if (read_i2s1 != ESP_OK) {
			ESP_LOGW("MIC", "Error with read from trunk line (i2s1)");
			continue;
		}
		
		int32_t max_l_i2s0 = 0;
		int32_t max_l_i2s1 = 0;

		
		for(size_t i = 0; i < SAMPLES; i++) {
			//lewy kanał obu magistral (i2s0 oraz i2s1)  
			int32_t l_val_i2s0 = abs(buffer_i2s0[2 * i] >> 8);
			if (l_val_i2s0 > max_l_i2s0) 
				max_l_i2s0 = l_val_i2s0;
			int32_t l_val_i2s1 = abs(buffer_i2s1[2 * i] >> 8);
			if (l_val_i2s1 > max_l_i2s1)
				max_l_i2s1 = l_val_i2s1;

		}
		
		printf("Mic 1 -> L: %ld		Mic 3 -> L: %ld\n", (long)max_l_i2s0, (long)max_l_i2s1); //TODO
	}	
}
