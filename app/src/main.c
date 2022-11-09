/*
 * Copyright (c) 2022 Gaël PORTAY
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/notify.h>
#include <zephyr/drivers/gpio.h>

struct k_poll_signal signal;

#if !DT_NODE_HAS_STATUS(DT_ALIAS(sw0), okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec sw = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0),
							  gpios,
							  {0});
static struct gpio_callback cb;

void pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	k_poll_signal_raise(&signal, k_uptime_get_32());
}

void main(void)
{
	struct k_poll_event event =
		K_POLL_EVENT_INITIALIZER(K_POLL_STATE_SIGNALED,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &signal);
	struct sys_notify notify;
	int ret;

	k_poll_signal_init(&signal);
	sys_notify_init_signal(&notify, &signal);

	if (!device_is_ready(sw.port)) {
		printk("Error: %s is not ready\n", sw.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&sw, GPIO_INPUT);
	if (ret != 0) {
		printk("Failed to configure %s PIN%d: %d\n", sw.port->name,
		       sw.pin, ret);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&sw, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Failed to configure interrupt on %s PIN%d: %d\n",
		       sw.port->name, sw.pin, ret);
		return;
	}

	gpio_init_callback(&cb, pressed, BIT(sw.pin));

	ret = gpio_add_callback(sw.port, &cb);
	if (ret != 0) {
		printk("Failed to add callback on %s: %d\n", sw.port->name,
		       ret);
		return;
	}

	/* Loop forever and beyond */
	for (;;) {
		int ret = k_poll(&event, 1, K_FOREVER);
		if (ret < 0) {
			if (ret == -EAGAIN) continue;
			printk("Failed to poll: %d\n", ret);
			break;
		}

		printk("Uptime: %dms\n", signal.result);
		k_poll_signal_reset(&signal);
	}
}
