/*
 * Copyright (c) 2022 Gaël PORTAY
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/notify.h>

struct k_poll_signal signal;

void handler(struct k_work *work)
{
	k_poll_signal_raise(&signal, k_uptime_get_32());
}

K_WORK_DEFINE(worker, handler);

void expired(struct k_timer *timer_id)
{
	/*
	 * Defer the work out of that interrupt context; by submiting it to the
	 * system workqueue.
	 */
	k_work_submit(&worker);
}

K_TIMER_DEFINE(timer, expired, NULL);

void main(void)
{
	struct k_poll_event event =
		K_POLL_EVENT_INITIALIZER(K_POLL_STATE_SIGNALED,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &signal);
	struct sys_notify notify;

	k_poll_signal_init(&signal);
	sys_notify_init_signal(&notify, &signal);
	k_timer_start(&timer, K_SECONDS(1), K_SECONDS(1));

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
