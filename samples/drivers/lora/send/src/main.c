/*
 * Copyright (c) 2022 Gaël PORTAY
 *               2019 Manivannan Sadhasivam
 *               2018 PHYTEC Messtechnik GmbH
 *               2016 Open-RnD Sp. z o.o.
 *               2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/display/cfb.h>
#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <stdio.h>

#define SLEEP_TIME_MS	1

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE	DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
							      {0});
static struct gpio_callback button_cb_data;

/*
 * The led0 devicetree alias is optional. If present, we'll use it
 * to turn on the LED whenever the button is pressed.
 */
static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios,
						     {0});

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
}

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay),
	     "No default LoRa radio specified in DT");

#define MAX_DATA_LEN 10

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lora_send);

char data[MAX_DATA_LEN] = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd'};

void main(void)
{
	const struct device *const lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
	const struct device *dev;
	struct lora_modem_config config;
	uint16_t rows;
	uint8_t ppt;
	uint8_t font_width;
	uint8_t font_height;
	int ret;

	if (!device_is_ready(button.port)) {
		printk("Error: button device %s is not ready\n",
		       button.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       ret, button.port->name, button.pin);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&button,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, button.port->name, button.pin);
		return;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	printk("Set up button at %s pin %d\n", button.port->name, button.pin);

	if (led.port && !device_is_ready(led.port)) {
		printk("Error %d: LED device %s is not ready; ignoring it\n",
		       ret, led.port->name);
		led.port = NULL;
	}
	if (led.port) {
		ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
		if (ret != 0) {
			printk("Error %d: failed to configure LED device %s pin %d\n",
			       ret, led.port->name, led.pin);
			led.port = NULL;
		} else {
			printk("Set up LED at %s pin %d\n", led.port->name, led.pin);
		}
	}

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(dev)) {
		printf("Device %s not ready\n", dev->name);
		return;
	}

	if (display_set_pixel_format(dev, PIXEL_FORMAT_MONO10) != 0) {
		printf("Failed to set required pixel format\n");
		return;
	}

	printf("Initialized %s\n", dev->name);

	if (cfb_framebuffer_init(dev)) {
		printf("Framebuffer initialization failed!\n");
		return;
	}

	cfb_framebuffer_clear(dev, true);

	cfb_framebuffer_invert(dev);

	cfb_framebuffer_finalize(dev);

	display_blanking_off(dev);

	rows = cfb_get_display_parameter(dev, CFB_DISPLAY_ROWS);
	ppt = cfb_get_display_parameter(dev, CFB_DISPLAY_PPT);

	for (int idx = 0; idx < 42; idx++) {
		if (cfb_get_font_size(dev, idx, &font_width, &font_height)) {
			break;
		}
		cfb_framebuffer_set_font(dev, idx);
		printf("font width %d, font height %d\n",
		       font_width, font_height);
	}

	printf("x_res %d, y_res %d, ppt %d, rows %d, cols %d\n",
	       cfb_get_display_parameter(dev, CFB_DISPLAY_WIDTH),
	       cfb_get_display_parameter(dev, CFB_DISPLAY_HEIGH),
	       ppt,
	       rows,
	       cfb_get_display_parameter(dev, CFB_DISPLAY_COLS));

	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s Device not ready", lora_dev->name);
		return;
	}

	config.frequency = 865100000;
	config.bandwidth = BW_125_KHZ;
	config.datarate = SF_10;
	config.preamble_len = 8;
	config.coding_rate = CR_4_5;
	config.iq_inverted = false;
	config.public_network = false;
	config.tx_power = 4;
	config.tx = true;

	ret = lora_config(lora_dev, &config);
	if (ret < 0) {
		LOG_ERR("LoRa config failed");
		return;
	}

	printk("Press the button\n");
	while (1) {
		if (led.port) {
			/* If we have an LED, match its state to the button's. */
			int val = gpio_pin_get_dt(&button);

			if (val >= 0) {
				gpio_pin_set_dt(&led, val);
			}

			if (val == 0) {
				k_msleep(SLEEP_TIME_MS);
				continue;
			}

		}

		ret = lora_send(lora_dev, data, MAX_DATA_LEN);
		if (ret < 0) {
			LOG_ERR("LoRa send failed");
			return;
		}

		LOG_INF("Data sent!");

		if (dev) {
			static int i = 0;
			char buf[10+1]; /* INT_MAX + NUL */

			cfb_framebuffer_clear(dev, false);
			if (cfb_print(dev, "TX:", 0, 0)) {
				LOG_ERR("Failed to print a string\n");
				continue;
			}

			sprintf(buf, "%d", ++i);
			if (cfb_print(dev, buf, 0, 1 * font_height)) {
				LOG_ERR("Failed to print a string\n");
				continue;
			}

			cfb_framebuffer_finalize(dev);
		}

		/* Send data at 1s interval */
		k_sleep(K_MSEC(1000));
	}
}
