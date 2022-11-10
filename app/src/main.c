/*
 * Copyright (c) 2022 Gaël PORTAY
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/notify.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/display/cfb.h>

struct k_poll_signal signal;

#if !DT_NODE_HAS_STATUS(DT_ALIAS(sw0), okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec sw = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0),
							  gpios,
							  {0});
static struct gpio_callback cb;

static const struct device *lcd = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

void handler(struct k_work *work)
{
	static int i = 0;
	char buf[10+1]; /* INT_MAX + NUL */
	int ret;

	k_poll_signal_raise(&signal, k_uptime_get_32());

	sprintf(buf, "%d", ++i);
	ret = cfb_print(lcd, buf, 0, 1 * 16);
	if (ret != 0) {
		printk("Failed to print string on %s: %d\n", lcd->name, ret);
		return;
	}

	ret = cfb_framebuffer_finalize(lcd);
	if (ret != 0) {
		printk("Failed to finalize framebuffer on %s: %d\n", lcd->name,
		       ret);
		return;
	}
}

K_WORK_DEFINE(worker, handler);

void pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	k_work_submit(&worker);
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

	if (!device_is_ready(lcd)) {
		printk("Error: %s is not ready\n", lcd->name);
		return;
	}

	ret = display_set_pixel_format(lcd, PIXEL_FORMAT_MONO10);
	if (ret != 0) {
		printk("Failed to set pixel format on %s: %d\n", lcd->name,
		       ret);
		return;
	}

	ret = cfb_framebuffer_init(lcd);
	if (ret != 0) {
		printk("Failed to initialize framebuffer on %s: %d\n",
		       lcd->name, ret);
		return;
	}

	ret = cfb_framebuffer_clear(lcd, true);
	if (ret != 0) {
		printk("Failed to clear framebuffer on %s: %d\n", lcd->name,
		       ret);
		return;
	}

	ret = cfb_framebuffer_invert(lcd);
	if (ret != 0) {
		printk("Failed to invert framebuffer on %s: %d\n", lcd->name,
		       ret);
		return;
	}

	ret = cfb_print(lcd, "Press PRG", 0, 0);
	if (ret != 0) {
		printk("Failed to print string on %s: %d\n", lcd->name, ret);
		return;
	}

	ret = cfb_framebuffer_finalize(lcd);
	if (ret != 0) {
		printk("Failed to finalize framebuffer on %s: %d\n", lcd->name,
		       ret);
		return;
	}

	ret = display_blanking_off(lcd);
	if (ret != 0) {
		printk("Failed to turn display blanking off on %s: %d\n",
		       lcd->name, ret);
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
